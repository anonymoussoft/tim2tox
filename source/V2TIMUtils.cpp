#include <uuid/uuid.h> // 需要链接 libuuid
#include <string>
#include "V2TIMUtils.h"
#include <vector>
#include <cstring>

#define TOX_PUBLIC_KEY_SIZE 32
#define TOX_ADDRESS_STRING_LENGTH (TOX_PUBLIC_KEY_SIZE * 2)

std::string GenerateUniqueID() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);
    return std::string(uuid_str);
}

/**
 * @brief 内部工具函数：十六进制字符转数值
 * @return 0-15为有效值，0xFF表示无效字符
 */
static uint8_t hex_char_to_byte(char c) {
    switch(c) {
    case '0'...'9': return c - '0';
    case 'A'...'F': return c - 'A' + 10;
    case 'a'...'f': return c - 'a' + 10;
    default:        return 0xFF; // 无效字符标记
    }
}

bool tox_address_to_string(const uint8_t* public_key, char* address) {
    // 参数有效性校验
    if (!public_key || !address) return false;

    // 十六进制字符表（大写）
    static const char hex_table[] = "0123456789ABCDEF";

    for (size_t i = 0; i < TOX_PUBLIC_KEY_SIZE; ++i) {
        // 处理高四位
        address[i*2]   = hex_table[(public_key[i] >> 4) & 0x0F];
        // 处理低四位
        address[i*2+1] = hex_table[public_key[i] & 0x0F];
    }
    address[TOX_ADDRESS_STRING_LENGTH] = '\0'; // 确保终止符
    return true;
}

bool tox_address_string_to_bytes(const char* address, uint8_t* public_key) {
    // 基础参数校验
    if (!address || !public_key) return false;
    
    // 严格长度检查
    if (strlen(address) != TOX_ADDRESS_STRING_LENGTH) return false;

    for (size_t i = 0; i < TOX_PUBLIC_KEY_SIZE; ++i) {
        // 获取并统一转为大写
        const char h_char = address[i*2];
        const char l_char = address[i*2+1];
        const uint8_t h_val = hex_char_to_byte(h_char);
        const uint8_t l_val = hex_char_to_byte(l_char);

        // 有效性检查
        if (h_val > 0x0F || l_val > 0x0F) return false;

        public_key[i] = (h_val << 4) | l_val;
    }
    return true;
}

// 序列化方法：将 SignalingPacket 转换为字节流
std::vector<uint8_t> SerializePacket(const SignalingPacket& packet) {
    std::vector<uint8_t> buffer;

    // 序列化 type (1字节)
    buffer.push_back(static_cast<uint8_t>(packet.type));

    // 序列化 inviteID (长度前缀 + 字符串)
    uint32_t inviteIDLength = packet.inviteID.size();
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&inviteIDLength), 
               reinterpret_cast<uint8_t*>(&inviteIDLength) + sizeof(uint32_t));
    buffer.insert(buffer.end(), packet.inviteID.begin(), packet.inviteID.end());

    // 序列化 data (长度前缀 + 字符串)
    uint32_t dataLength = packet.data.size();
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&dataLength), 
               reinterpret_cast<uint8_t*>(&dataLength) + sizeof(uint32_t));
    buffer.insert(buffer.end(), packet.data.begin(), packet.data.end());

    // 序列化 timeout (4字节)
    uint32_t timeoutBE = htonl(packet.timeout); // 转为网络字节序
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&timeoutBE), 
               reinterpret_cast<uint8_t*>(&timeoutBE) + sizeof(uint32_t));

    return buffer;
}

// 反序列化方法：从字节流解析 SignalingPacket
SignalingPacket ParsePacket(const uint8_t* data, size_t length) {
    SignalingPacket packet;
    size_t offset = 0;

    // 解析 type
    packet.type = static_cast<SignalingType>(data[offset]);
    offset += 1;

    // 解析 inviteID
    uint32_t inviteIDLength;
    memcpy(&inviteIDLength, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    packet.inviteID.assign(reinterpret_cast<const char*>(data + offset), inviteIDLength);
    offset += inviteIDLength;

    // 解析 data
    uint32_t dataLength;
    memcpy(&dataLength, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    packet.data.assign(reinterpret_cast<const char*>(data + offset), dataLength);
    offset += dataLength;

    // 解析 timeout
    uint32_t timeoutBE;
    memcpy(&timeoutBE, data + offset, sizeof(uint32_t));
    packet.timeout = ntohl(timeoutBE); // 转为主机字节序

    return packet;
}

bool hex_string_to_bin(const char* hex_string, uint8_t* bin, size_t bin_size) {
    if (!hex_string || !bin || strlen(hex_string) != bin_size * 2) {
        return false;
    }

    for (size_t i = 0; i < bin_size; i++) {
        char hex[3] = {hex_string[i * 2], hex_string[i * 2 + 1], '\0'};
        char* end;
        bin[i] = (uint8_t)strtol(hex, &end, 16);
        if (*end != '\0') {
            return false;
        }
    }
    return true;
}