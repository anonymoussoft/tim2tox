#ifndef __V2TIM_UTILS_H__
#define __V2TIM_UTILS_H__

#include <cstdint>
#include <string>
#include <vector>

// 信令类型枚举
enum SignalingType : uint8_t {
    SIGNALING_INVITE = 0x01,    // 邀请
    SIGNALING_ACCEPT = 0x02,    // 接受
    SIGNALING_REJECT = 0x03,    // 拒绝
    SIGNALING_CANCEL = 0x04,    // 取消
    SIGNALING_TIMEOUT = 0x05    // 超时
};

// 信令协议包结构体
struct SignalingPacket {
    SignalingType type;          // 信令类型
    std::string inviteID;        // 唯一邀请ID (例如 UUID)
    std::string data;            // 自定义数据载荷
    uint32_t timeout;            // 超时时间（秒）
    // 其他可选字段（如群组ID、扩展信息等）
};

std::string GenerateUniqueID();

/**
 * @brief 将32字节公钥转换为64字符十六进制字符串
 * @param public_key 输入公钥缓冲区，必须为TOX_PUBLIC_KEY_SIZE字节
 * @param address 输出字符串缓冲区，必须至少TOX_ADDRESS_STRING_LENGTH+1字节
 * @return true转换成功，false参数无效
 */
bool tox_address_to_string(const uint8_t* public_key, char* address);

/**
 * @brief 将64字符十六进制字符串转换为32字节公钥
 * @param address 输入字符串，必须为TOX_ADDRESS_STRING_LENGTH字符
 * @param public_key 输出缓冲区，必须至少TOX_PUBLIC_KEY_SIZE字节
 * @return true转换成功，false参数无效或格式错误
 */
bool tox_address_string_to_bytes(const char* address, uint8_t* public_key);

std::vector<uint8_t> SerializePacket(const SignalingPacket& packet);
SignalingPacket ParsePacket(const uint8_t* data, size_t length);

bool hex_string_to_bin(const char* hex_string, uint8_t* bin, size_t bin_size);

#endif  // __V2TIM_UTILS_H__