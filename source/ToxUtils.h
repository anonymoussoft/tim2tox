#ifndef __TOX_UTILS_H__
#define __TOX_UTILS_H__

#include <string>
#include <cstring>
#include <cstdio>
#include <cstdint>

// 错误码定义
// SDK通用错误码
#define ERR_SDK_NOT_INITIALIZED 6701
#define ERR_SDK_NOT_SUPPORTED 6789
#define ERR_OUT_OF_MEMORY 6711
#define ERR_INVALID_PARAMETERS 6770

// 好友相关错误码
#define ERR_SDK_FRIEND_ADD_FAILED 30001
#define ERR_SDK_FRIEND_DELETE_FAILED 30002
#define ERR_SDK_FRIEND_ADD_SELF 30515
#define ERR_SDK_FRIEND_REQ_SENT 30514
#define ERR_SVR_FRIENDSHIP_INVALID 10004

// Tox 二进制和十六进制转换工具函数
inline bool tox_hex_to_bytes(const char* hex_string, size_t hex_len, uint8_t* bytes, size_t bytes_len) {
    if (hex_len % 2 != 0 || bytes_len < (hex_len / 2)) {
        return false;
    }

    for (size_t i = 0; i < hex_len; i += 2) {
        if (sscanf(hex_string + i, "%2hhx", &bytes[i / 2]) != 1) {
            return false;
        }
    }
    return true;
}

inline void tox_bytes_to_hex(const uint8_t* bytes, size_t bytes_len, uint8_t* hex_string) {
    for (size_t i = 0; i < bytes_len; ++i) {
        sprintf((char*)(hex_string + i * 2), "%02X", bytes[i]);
    }
}

#endif // __TOX_UTILS_H__ 