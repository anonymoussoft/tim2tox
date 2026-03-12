#include "MessageReplyUtil.h"
#include "V2TIMLog.h"
#include <algorithm>
#include <cstring>

const char* MessageReplyUtil::REPLY_START_MARKER = "[REPLY_START]";
const char* MessageReplyUtil::REPLY_END_MARKER = "[REPLY_END]";

std::string MessageReplyUtil::ExtractReplyJsonFromCloudCustomData(const V2TIMBuffer& cloudCustomData) {
    if (cloudCustomData.Data() == nullptr || cloudCustomData.Size() == 0) {
        return "";
    }
    
    // cloudCustomData是JSON格式，格式为：{"messageReference":{...}} 或 {"messageReply":{...}}
    // 我们需要提取messageReference或messageReply字段的内容
    std::string cloudDataStr(reinterpret_cast<const char*>(cloudCustomData.Data()), cloudCustomData.Size());
    
    // 检查是否包含messageReference（引用回复）
    if (HasJsonKey(cloudDataStr, "messageReference")) {
        std::string replyData = ExtractJsonValue(cloudDataStr, "messageReference");
        if (!replyData.empty() && replyData[0] == '{') {
            return replyData;
        }
    }
    
    // 检查是否包含messageReply（回复消息，用于回复链）
    if (HasJsonKey(cloudDataStr, "messageReply")) {
        std::string replyData = ExtractJsonValue(cloudDataStr, "messageReply");
        if (!replyData.empty() && replyData[0] == '{') {
            return replyData;
        }
    }
    
    return "";
}

std::string MessageReplyUtil::BuildMessageWithReply(const std::string& replyJson, const std::string& text) {
    if (replyJson.empty()) {
        return text;
    }
    
    std::string result;
    result.reserve(replyJson.length() + text.length() + 64); // 预分配空间
    
    result += REPLY_START_MARKER;
    result += replyJson;
    result += REPLY_END_MARKER;
    result += "\n";
    result += text;
    
    return result;
}

bool MessageReplyUtil::ParseMessageWithReply(const std::string& message, std::string& replyJson, std::string& text) {
    size_t startPos = message.find(REPLY_START_MARKER);
    size_t endPos = message.find(REPLY_END_MARKER);
    
    if (startPos == std::string::npos || endPos == std::string::npos) {
        // 没有引用回复信息
        text = message;
        replyJson = "";
        return false;
    }
    
    if (endPos <= startPos) {
        // 格式错误
        text = message;
        replyJson = "";
        return false;
    }
    
    // 提取引用回复JSON
    size_t jsonStart = startPos + strlen(REPLY_START_MARKER);
    size_t jsonLength = endPos - jsonStart;
    replyJson = message.substr(jsonStart, jsonLength);
    
    // 提取实际文本内容（在REPLY_END_MARKER之后）
    size_t textStart = endPos + strlen(REPLY_END_MARKER);
    if (textStart < message.length()) {
        // 跳过可能的换行符
        while (textStart < message.length() && (message[textStart] == '\n' || message[textStart] == '\r')) {
            textStart++;
        }
        text = message.substr(textStart);
    } else {
        text = "";
    }
    
    return true;
}

V2TIMBuffer MessageReplyUtil::BuildCloudCustomDataFromReplyJson(const std::string& replyJson) {
    if (replyJson.empty()) {
        return V2TIMBuffer();
    }
    
    // 构建完整的cloudCustomData JSON格式
    // 格式：{"messageReference":{...}}
    std::string cloudDataStr = "{\"messageReference\":";
    cloudDataStr += replyJson;
    cloudDataStr += "}";
    
    return V2TIMBuffer(reinterpret_cast<const uint8_t*>(cloudDataStr.c_str()), cloudDataStr.length());
}

bool MessageReplyUtil::IsMessageTooLong(const std::string& message) {
    // TOX_MAX_MESSAGE_LENGTH = 1372
    return message.length() > 1372;
}

std::string MessageReplyUtil::CompressReplyJson(const std::string& replyJson) {
    // 提取messageID和version，构建最小化的JSON
    std::string messageID = ExtractJsonValue(replyJson, "messageID");
    std::string version = ExtractJsonValue(replyJson, "version");
    
    if (messageID.empty()) {
        return replyJson; // 如果提取失败，返回原JSON
    }
    
    // 构建压缩后的JSON：{"version":1,"messageID":"xxx"}
    std::string compressed = "{";
    if (!version.empty()) {
        compressed += "\"version\":" + version + ",";
    } else {
        compressed += "\"version\":1,";
    }
    compressed += "\"messageID\":\"" + messageID + "\"";
    compressed += "}";
    
    return compressed;
}

// 辅助函数：简单的JSON值提取（不处理嵌套，只处理简单情况）
std::string MessageReplyUtil::ExtractJsonValue(const std::string& json, const std::string& key) {
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
        size_t strEnd = json.find('"', strStart);
        if (strEnd == std::string::npos) {
            return "";
        }
        return json.substr(strStart, strEnd - strStart);
    } else if (json[valueStart] == '{') {
        // 对象值，需要找到匹配的右括号
        int braceCount = 1;
        size_t objStart = valueStart + 1;
        size_t objEnd = objStart;
        
        while (objEnd < json.length() && braceCount > 0) {
            if (json[objEnd] == '{') {
                braceCount++;
            } else if (json[objEnd] == '}') {
                braceCount--;
            }
            if (braceCount > 0) {
                objEnd++;
            }
        }
        
        if (braceCount == 0) {
            return "{" + json.substr(objStart, objEnd - objStart) + "}";
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

bool MessageReplyUtil::HasJsonKey(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    return json.find(searchKey) != std::string::npos;
}

V2TIMBuffer MessageReplyUtil::RemoveReplyFromCloudCustomData(const V2TIMBuffer& cloudCustomData) {
    if (cloudCustomData.Data() == nullptr || cloudCustomData.Size() == 0) {
        return cloudCustomData;
    }
    
    std::string cloudDataStr(reinterpret_cast<const char*>(cloudCustomData.Data()), cloudCustomData.Size());
    
    // 检查是否包含messageReference或messageReply
    bool hasReference = HasJsonKey(cloudDataStr, "messageReference");
    bool hasReply = HasJsonKey(cloudDataStr, "messageReply");
    
    if (!hasReference && !hasReply) {
        // 没有引用信息，直接返回原数据
        return cloudCustomData;
    }
    
    // 移除messageReference和messageReply字段
    // 简单的JSON处理：如果只有这两个字段，返回空buffer；否则尝试移除这两个字段
    // 为了简化，如果包含引用信息，我们返回空的cloudCustomData
    // 这样可以避免转发消息时错误地包含引用信息
    V2TIM_LOG(kInfo, "RemoveReplyFromCloudCustomData: Removing reply/reference info from cloudCustomData for forwarding");
    return V2TIMBuffer();
}

