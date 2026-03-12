#ifndef MESSAGE_REPLY_UTIL_H
#define MESSAGE_REPLY_UTIL_H

#include "V2TIMString.h"
#include "V2TIMBuffer.h"
#include <string>

/**
 * 消息引用回复工具类
 * 用于在Tox协议上实现UIKit的引用回复功能
 */
class MessageReplyUtil {
public:
    // 引用回复标记
    static const char* REPLY_START_MARKER;
    static const char* REPLY_END_MARKER;
    
    /**
     * 从cloudCustomData中提取引用回复JSON字符串
     * @param cloudCustomData 消息的cloudCustomData字段
     * @return 引用回复的JSON字符串，如果没有则返回空字符串
     */
    static std::string ExtractReplyJsonFromCloudCustomData(const V2TIMBuffer& cloudCustomData);
    
    /**
     * 构建包含引用回复的文本消息
     * @param replyJson 引用回复的JSON字符串
     * @param text 用户输入的文本内容
     * @return 组合后的消息文本
     */
    static std::string BuildMessageWithReply(const std::string& replyJson, const std::string& text);
    
    /**
     * 解析消息，提取引用回复信息和实际文本内容
     * @param message 接收到的消息文本
     * @param replyJson 输出：引用回复的JSON字符串（如果有）
     * @param text 输出：实际的文本内容
     * @return 是否包含引用回复信息
     */
    static bool ParseMessageWithReply(const std::string& message, std::string& replyJson, std::string& text);
    
    /**
     * 将引用回复JSON字符串转换为cloudCustomData格式
     * @param replyJson 引用回复的JSON字符串
     * @return V2TIMBuffer格式的cloudCustomData
     */
    static V2TIMBuffer BuildCloudCustomDataFromReplyJson(const std::string& replyJson);
    
    /**
     * 检查消息是否超过最大长度限制
     * @param message 消息文本
     * @return 是否超过限制
     */
    static bool IsMessageTooLong(const std::string& message);
    
    /**
     * 压缩引用回复信息（当消息过长时使用）
     * 只保留messageID，其他信息通过本地查询获取
     * @param replyJson 完整的引用回复JSON
     * @return 压缩后的JSON（只包含messageID和version）
     */
    static std::string CompressReplyJson(const std::string& replyJson);
    
    /**
     * 从JSON中提取指定key的值（用于提取messageID等）
     * @param json JSON字符串
     * @param key 要提取的key
     * @return key对应的值，如果不存在则返回空字符串
     */
    static std::string ExtractJsonValue(const std::string& json, const std::string& key);
    
    /**
     * 从cloudCustomData中移除引用回复信息（用于转发消息时）
     * @param cloudCustomData 原始cloudCustomData
     * @return 移除引用信息后的cloudCustomData，如果没有引用信息则返回原数据
     */
    static V2TIMBuffer RemoveReplyFromCloudCustomData(const V2TIMBuffer& cloudCustomData);
    
private:
    // 辅助函数：检查JSON中是否包含指定的key
    static bool HasJsonKey(const std::string& json, const std::string& key);
};

#endif // MESSAGE_REPLY_UTIL_H

