#include <string>
#include "V2TIMUtils.h"
#include <vector>
#include <cstring>
#include <random>
#include <sstream>
#include <iomanip>

#define TOX_PUBLIC_KEY_SIZE 32
#define TOX_ADDRESS_STRING_LENGTH (TOX_PUBLIC_KEY_SIZE * 2)

// Network byte order helpers.
// MSVC on Windows may not provide htonl/ntohl without Winsock headers, so we keep
// a tiny implementation local to this translation unit.
#ifdef _WIN32
static inline uint32_t tim2tox_htonl(uint32_t host) {
    return ((host & 0x000000FFu) << 24) |
           ((host & 0x0000FF00u) << 8) |
           ((host & 0x00FF0000u) >> 8) |
           ((host & 0xFF000000u) >> 24);
}
static inline uint32_t tim2tox_ntohl(uint32_t net) {
    // For 32-bit, htonl and ntohl are the same byte swap.
    return tim2tox_htonl(net);
}
#else
static inline uint32_t tim2tox_htonl(uint32_t host) { return htonl(host); }
static inline uint32_t tim2tox_ntohl(uint32_t net) { return ntohl(net); }
#endif

std::string GenerateUniqueID() {
    // Generate a UUID-like string without relying on platform-specific uuid
    // libraries so mobile/embedded targets do not need extra system headers.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 255);

    uint8_t bytes[16];
    for (int i = 0; i < 16; ++i) {
        bytes[i] = static_cast<uint8_t>(dist(gen));
    }

    // Version 4 (random) and variant bits.
    bytes[6] = static_cast<uint8_t>((bytes[6] & 0x0F) | 0x40);
    bytes[8] = static_cast<uint8_t>((bytes[8] & 0x3F) | 0x80);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) oss << '-';
    }
    return oss.str();
}

/**
 * @brief 内部工具函数：十六进制字符转数值
 * @return 0-15为有效值，0xFF表示无效字符
 */
static uint8_t hex_char_to_byte(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
    return 0xFF; // Invalid character marker
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
    uint32_t timeoutBE = tim2tox_htonl(packet.timeout); // Convert to network byte order
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&timeoutBE), 
               reinterpret_cast<uint8_t*>(&timeoutBE) + sizeof(uint32_t));

    return buffer;
}

// 反序列化方法：从字节流解析 SignalingPacket（带边界检查，避免越界崩溃）
SignalingPacket ParsePacket(const uint8_t* data, size_t length) {
    SignalingPacket packet;
    size_t offset = 0;

    // 最小长度: type(1) + inviteIDLen(4) + dataLen(4) + timeout(4) = 13
    if (!data || length < 13) {
        packet.type = static_cast<SignalingType>(0xff);  // invalid, caller will ignore
        return packet;
    }

    // 解析 type
    packet.type = static_cast<SignalingType>(data[offset]);
    offset += 1;

    // 解析 inviteID
    uint32_t inviteIDLength;
    memcpy(&inviteIDLength, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    if (offset + inviteIDLength > length) {
        packet.type = static_cast<SignalingType>(0xff);
        return packet;
    }
    packet.inviteID.assign(reinterpret_cast<const char*>(data + offset), inviteIDLength);
    offset += inviteIDLength;

    // 解析 data
    if (offset + sizeof(uint32_t) > length) {
        packet.type = static_cast<SignalingType>(0xff);
        return packet;
    }
    uint32_t dataLength;
    memcpy(&dataLength, data + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    if (offset + dataLength > length) {
        packet.type = static_cast<SignalingType>(0xff);
        return packet;
    }
    packet.data.assign(reinterpret_cast<const char*>(data + offset), dataLength);
    offset += dataLength;

    // 解析 timeout
    if (offset + sizeof(uint32_t) > length) {
        packet.type = static_cast<SignalingType>(0xff);
        return packet;
    }
    uint32_t timeoutBE;
    memcpy(&timeoutBE, data + offset, sizeof(uint32_t));
    packet.timeout = tim2tox_ntohl(timeoutBE); // Convert to host byte order

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
