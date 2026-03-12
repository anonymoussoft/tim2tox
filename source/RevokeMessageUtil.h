#ifndef REVOKE_MESSAGE_UTIL_H
#define REVOKE_MESSAGE_UTIL_H

#include "V2TIMString.h"
#include "V2TIMBuffer.h"
#include "V2TIMMessage.h"
#include <string>

/**
 * 消息撤回工具类
 * 用于在Tox协议上实现UIKit的消息撤回功能
 * 注意：仅在联系人在线的情况下实现撤回通知
 */
class RevokeMessageUtil {
public:
    // 撤回消息标记
    static const char* REVOKE_START_MARKER;
    static const char* REVOKE_END_MARKER;
    
    /**
     * 检查消息是否可以撤回
     * @param message 要撤回的消息
     * @param maxRevokeTimeSeconds 最大撤回时间（秒），默认120秒（2分钟）
     * @return 是否可以撤回
     */
    static bool CanRevokeMessage(const V2TIMMessage& message, int64_t maxRevokeTimeSeconds = 120);
    
    /**
     * 构建撤回通知的JSON数据
     * @param msgID 要撤回的消息ID
     * @param revoker 撤回者ID
     * @param reason 撤回原因（可选）
     * @return JSON格式的撤回通知数据
     */
    static std::string BuildRevokeJson(const std::string& msgID, const std::string& revoker, const std::string& reason = "");
    
    /**
     * 构建包含撤回通知的自定义消息
     * @param revokeJson 撤回通知的JSON数据
     * @return 组合后的消息数据
     */
    static V2TIMBuffer BuildRevokeMessage(const std::string& revokeJson);
    
    /**
     * 解析消息，提取撤回通知信息
     * @param data 接收到的消息数据
     * @param msgID 输出：被撤回的消息ID
     * @param revoker 输出：撤回者ID
     * @param reason 输出：撤回原因
     * @return 是否包含撤回通知
     */
    static bool ParseRevokeMessage(const V2TIMBuffer& data, std::string& msgID, std::string& revoker, std::string& reason);
    
    /**
     * 从JSON中提取字段值
     * @param json JSON字符串
     * @param key 字段名
     * @return 字段值
     */
    static std::string ExtractJsonValue(const std::string& json, const std::string& key);
};

#endif // REVOKE_MESSAGE_UTIL_H

