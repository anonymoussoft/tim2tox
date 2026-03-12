#include "MergerMessageUtil.h"
#include "V2TIMLog.h"
#include "MessageReplyUtil.h" // 复用JSON解析函数
#include <algorithm>
#include <cstring>
#include <sstream>

const char* MergerMessageUtil::MERGER_START_MARKER = "[MERGER_START]";
const char* MergerMessageUtil::MERGER_END_MARKER = "[MERGER_END]";

std::string MergerMessageUtil::BuildMergerJson(const V2TIMMessageVector& messageList,
                                               const V2TIMString& title,
                                               const V2TIMStringVector& abstractList) {
    // 构建JSON格式：{"title":"...","abstractList":[...],"messageIDList":[...]}
    std::ostringstream json;
    json << "{";
    
    // title
    json << "\"title\":\"";
    std::string titleStr(title.CString());
    // 转义JSON特殊字符
    for (char c : titleStr) {
        if (c == '"') json << "\\\"";
        else if (c == '\\') json << "\\\\";
        else if (c == '\n') json << "\\n";
        else if (c == '\r') json << "\\r";
        else if (c == '\t') json << "\\t";
        else json << c;
    }
    json << "\",";
    
    // abstractList
    json << "\"abstractList\":[";
    for (size_t i = 0; i < abstractList.Size(); i++) {
        if (i > 0) json << ",";
        json << "\"";
        std::string abstractStr(abstractList[i].CString());
        // 转义JSON特殊字符
        for (char c : abstractStr) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else if (c == '\n') json << "\\n";
            else if (c == '\r') json << "\\r";
            else if (c == '\t') json << "\\t";
            else json << c;
        }
        json << "\"";
    }
    json << "],";
    
    // messageIDList
    json << "\"messageIDList\":[";
    for (size_t i = 0; i < messageList.Size(); i++) {
        if (i > 0) json << ",";
        json << "\"";
        std::string msgID(messageList[i].msgID.CString());
        // 转义JSON特殊字符
        for (char c : msgID) {
            if (c == '"') json << "\\\"";
            else if (c == '\\') json << "\\\\";
            else json << c;
        }
        json << "\"";
    }
    json << "]";
    
    json << "}";
    return json.str();
}

std::string MergerMessageUtil::BuildMessageWithMerger(const std::string& mergerJson, const std::string& compatibleText) {
    std::string result;
    result.reserve(mergerJson.length() + compatibleText.length() + 64);
    
    result += MERGER_START_MARKER;
    result += mergerJson;
    result += MERGER_END_MARKER;
    result += "\n";
    result += compatibleText;
    
    return result;
}

bool MergerMessageUtil::ParseMessageWithMerger(const std::string& message, std::string& mergerJson, std::string& compatibleText) {
    size_t startPos = message.find(MERGER_START_MARKER);
    size_t endPos = message.find(MERGER_END_MARKER);
    
    if (startPos == std::string::npos || endPos == std::string::npos) {
        compatibleText = message;
        mergerJson = "";
        return false;
    }
    
    if (endPos <= startPos) {
        compatibleText = message;
        mergerJson = "";
        return false;
    }
    
    // 提取合并消息JSON
    size_t jsonStart = startPos + strlen(MERGER_START_MARKER);
    size_t jsonLength = endPos - jsonStart;
    mergerJson = message.substr(jsonStart, jsonLength);
    
    // 提取兼容文本
    size_t textStart = endPos + strlen(MERGER_END_MARKER);
    if (textStart < message.length()) {
        while (textStart < message.length() && (message[textStart] == '\n' || message[textStart] == '\r')) {
            textStart++;
        }
        compatibleText = message.substr(textStart);
    } else {
        compatibleText = "";
    }
    
    return true;
}

std::vector<std::string> MergerMessageUtil::ExtractMessageIDs(const std::string& mergerJson) {
    // 如果mergerJson是完整的JSON对象，提取messageIDList
    if (mergerJson.find("messageIDList") != std::string::npos) {
        return ExtractJsonArray(mergerJson, "messageIDList");
    }
    // 如果mergerJson本身就是数组，直接解析
    if (mergerJson[0] == '[') {
        return ExtractJsonArray("{\"array\":" + mergerJson + "}", "array");
    }
    return std::vector<std::string>();
}

std::vector<std::string> MergerMessageUtil::ExtractAbstractList(const std::string& mergerJson) {
    return ExtractJsonArray(mergerJson, "abstractList");
}

bool MergerMessageUtil::IsMessageTooLong(const std::string& message) {
    return message.length() > 1372;
}

std::string MergerMessageUtil::ExtractJsonValue(const std::string& json, const std::string& key) {
    // 复用MessageReplyUtil的ExtractJsonValue
    return MessageReplyUtil::ExtractJsonValue(json, key);
}

std::vector<std::string> MergerMessageUtil::ExtractJsonArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    
    // 查找数组
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) {
        return result;
    }
    
    // 查找冒号后的左方括号
    size_t colonPos = json.find(':', keyPos);
    if (colonPos == std::string::npos) {
        return result;
    }
    
    size_t arrayStart = colonPos + 1;
    while (arrayStart < json.length() && (json[arrayStart] == ' ' || json[arrayStart] == '\t')) {
        arrayStart++;
    }
    
    if (arrayStart >= json.length() || json[arrayStart] != '[') {
        return result;
    }
    
    // 解析数组元素
    size_t pos = arrayStart + 1;
    while (pos < json.length() && json[pos] != ']') {
        // 跳过空白字符
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }
        
        if (pos >= json.length() || json[pos] == ']') {
            break;
        }
        
        // 解析字符串值
        if (json[pos] == '"') {
            size_t strStart = pos + 1;
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
                result.push_back(unescaped);
                pos = strEnd + 1;
            } else {
                break;
            }
        } else {
            // 非字符串值，跳过到下一个逗号或右方括号
            while (pos < json.length() && json[pos] != ',' && json[pos] != ']') {
                pos++;
            }
        }
        
        // 跳过逗号
        if (pos < json.length() && json[pos] == ',') {
            pos++;
        }
    }
    
    return result;
}

