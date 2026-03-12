#ifndef MERGER_MESSAGE_UTIL_H
#define MERGER_MESSAGE_UTIL_H

#include "V2TIMString.h"
#include "V2TIMBuffer.h"
#include "V2TIMMessage.h"
#include <string>
#include <vector>

/**
 * 合并消息工具类
 * 用于在Tox协议上实现UIKit的合并转发功能
 */
class MergerMessageUtil {
public:
    // 合并消息标记
    static const char* MERGER_START_MARKER;
    static const char* MERGER_END_MARKER;
    
    /**
     * 构建合并消息的JSON数据
     * @param messageList 要合并的消息列表
     * @param title 合并消息标题
     * @param abstractList 摘要列表
     * @return JSON格式的合并消息数据
     */
    static std::string BuildMergerJson(const V2TIMMessageVector& messageList,
                                       const V2TIMString& title,
                                       const V2TIMStringVector& abstractList);
    
    /**
     * 构建包含合并消息的文本消息
     * @param mergerJson 合并消息的JSON数据
     * @param compatibleText 兼容文本（用于不支持合并消息的客户端）
     * @return 组合后的消息文本
     */
    static std::string BuildMessageWithMerger(const std::string& mergerJson, const std::string& compatibleText);
    
    /**
     * 解析消息，提取合并消息信息和兼容文本
     * @param message 接收到的消息文本
     * @param mergerJson 输出：合并消息的JSON数据（如果有）
     * @param compatibleText 输出：兼容文本
     * @return 是否包含合并消息
     */
    static bool ParseMessageWithMerger(const std::string& message, std::string& mergerJson, std::string& compatibleText);
    
    /**
     * 从合并消息JSON中提取消息ID列表
     * @param mergerJson 合并消息的JSON数据
     * @return 消息ID列表
     */
    static std::vector<std::string> ExtractMessageIDs(const std::string& mergerJson);
    
    /**
     * 从合并消息JSON中提取摘要列表
     * @param mergerJson 合并消息的JSON数据
     * @return 摘要列表
     */
    static std::vector<std::string> ExtractAbstractList(const std::string& mergerJson);
    
    /**
     * 检查消息是否超过最大长度限制
     * @param message 消息文本
     * @return 是否超过限制
     */
    static bool IsMessageTooLong(const std::string& message);
    
private:
    // 辅助函数：JSON处理
    static std::string ExtractJsonValue(const std::string& json, const std::string& key);
    static std::vector<std::string> ExtractJsonArray(const std::string& json, const std::string& key);
};

#endif // MERGER_MESSAGE_UTIL_H

