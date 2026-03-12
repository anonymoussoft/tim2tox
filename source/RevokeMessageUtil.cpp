#include "RevokeMessageUtil.h"
#include "V2TIMLog.h"
#include <chrono>
#include <sstream>
#include <cstring>

const char* RevokeMessageUtil::REVOKE_START_MARKER = "[REVOKE_START]";
const char* RevokeMessageUtil::REVOKE_END_MARKER = "[REVOKE_END]";

bool RevokeMessageUtil::CanRevokeMessage(const V2TIMMessage& message, int64_t maxRevokeTimeSeconds) {
    // 检查消息状态
    if (message.status == V2TIM_MSG_STATUS_LOCAL_REVOKED) {
        return false; // 已经撤回
    }
    
    if (message.status != V2TIM_MSG_STATUS_SEND_SUCC) {
        return false; // 只有发送成功的消息才能撤回
    }
    
    // 检查时间限制
    int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t messageTime = message.timestamp;
    int64_t timeDiff = currentTime - messageTime;
    
    if (timeDiff < 0) {
        // 消息时间戳异常（未来时间），允许撤回
        return true;
    }
    
    return timeDiff <= maxRevokeTimeSeconds;
}

std::string RevokeMessageUtil::BuildRevokeJson(const std::string& msgID, const std::string& revoker, const std::string& reason) {
    std::ostringstream json;
    json << "{";
    
    json << "\"msgID\":\"";
    for (char c : msgID) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else json << c;
    }
    json << "\",";
    
    json << "\"revoker\":\"";
    for (char c : revoker) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else json << c;
    }
    json << "\",";
    
    json << "\"timestamp\":";
    int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    json << timestamp;
    json << ",";
    
    json << "\"reason\":\"";
    for (char c : reason) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else if (c == '\n') json << "\\n";
        else if (c == '\r') json << "\\r";
        else if (c == '\t') json << "\\t";
        else json << c;
    }
    json << "\"";
    
    json << "}";
    return json.str();
}

V2TIMBuffer RevokeMessageUtil::BuildRevokeMessage(const std::string& revokeJson) {
    std::string message;
    message.reserve(revokeJson.length() + 64);
    
    message += REVOKE_START_MARKER;
    message += revokeJson;
    message += REVOKE_END_MARKER;
    
    return V2TIMBuffer(reinterpret_cast<const uint8_t*>(message.c_str()), message.length());
}

bool RevokeMessageUtil::ParseRevokeMessage(const V2TIMBuffer& data, std::string& msgID, std::string& revoker, std::string& reason) {
    if (!data.Data() || data.Size() == 0) {
        return false;
    }
    
    std::string messageStr(reinterpret_cast<const char*>(data.Data()), data.Size());
    
    size_t startPos = messageStr.find(REVOKE_START_MARKER);
    size_t endPos = messageStr.find(REVOKE_END_MARKER);
    
    if (startPos == std::string::npos || endPos == std::string::npos) {
        return false;
    }
    
    if (endPos <= startPos) {
        return false;
    }
    
    // 提取JSON
    size_t jsonStart = startPos + strlen(REVOKE_START_MARKER);
    size_t jsonLength = endPos - jsonStart;
    std::string json = messageStr.substr(jsonStart, jsonLength);
    
    // 解析JSON字段
    msgID = ExtractJsonValue(json, "msgID");
    revoker = ExtractJsonValue(json, "revoker");
    reason = ExtractJsonValue(json, "reason");
    
    return !msgID.empty() && !revoker.empty();
}

std::string RevokeMessageUtil::ExtractJsonValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return "";
    }
    
    // 查找冒号
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return "";
    }
    
    // 跳过空白字符
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() && (json[valueStart] == ' ' || json[valueStart] == '\t')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) {
        return "";
    }
    
    // 判断值的类型
    if (json[valueStart] == '"') {
        // 字符串值
        size_t strStart = valueStart + 1;
        size_t strEnd = strStart;
        bool escaped = false;
        
        while (strEnd < json.length()) {
            if (escaped) {
                escaped = false;
                strEnd++;
                continue;
            }
            if (json[strEnd] == '\\') {
                escaped = true;
                strEnd++;
                continue;
            }
            if (json[strEnd] == '"') {
                break;
            }
            strEnd++;
        }
        
        if (strEnd < json.length()) {
            std::string value = json.substr(strStart, strEnd - strStart);
            // 处理转义字符
            std::string unescaped;
            for (size_t i = 0; i < value.length(); i++) {
                if (value[i] == '\\' && i + 1 < value.length()) {
                    if (value[i + 1] == 'n') {
                        unescaped += '\n';
                        i++;
                    } else if (value[i + 1] == 'r') {
                        unescaped += '\r';
                        i++;
                    } else if (value[i + 1] == 't') {
                        unescaped += '\t';
                        i++;
                    } else if (value[i + 1] == '"') {
                        unescaped += '"';
                        i++;
                    } else if (value[i + 1] == '\\') {
                        unescaped += '\\';
                        i++;
                    } else {
                        unescaped += value[i];
                    }
                } else {
                    unescaped += value[i];
                }
            }
            return unescaped;
        }
        return "";
    } else {
        // 数字或其他简单值，找到下一个逗号或右括号
        size_t valueEnd = valueStart;
        while (valueEnd < json.length() && 
               json[valueEnd] != ',' && 
               json[valueEnd] != '}' && 
               json[valueEnd] != ']' &&
               json[valueEnd] != ' ' &&
               json[valueEnd] != '\t' &&
               json[valueEnd] != '\n' &&
               json[valueEnd] != '\r') {
            valueEnd++;
        }
        return json.substr(valueStart, valueEnd - valueStart);
    }
}

