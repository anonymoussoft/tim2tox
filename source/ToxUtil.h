#ifndef TOX_UTIL_H
#define TOX_UTIL_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include "tox.h"

namespace ToxUtil {

// Convert a hex string to bytes
// Returns true if conversion successful, false otherwise
inline bool tox_hex_to_bytes(const char* hex_string, size_t length, uint8_t* bytes, size_t bytes_size) {
    if (length % 2 != 0 || length / 2 > bytes_size) {
        return false;
    }
    
    for (size_t i = 0; i < length / 2; i++) {
        char hex_byte[3] = {hex_string[i * 2], hex_string[i * 2 + 1], '\0'};
        char* end_ptr;
        bytes[i] = (uint8_t)strtol(hex_byte, &end_ptr, 16);
        if (*end_ptr != '\0') {
            return false;
        }
    }
    
    return true;
}

// Convert bytes to a hex string
inline std::string tox_bytes_to_hex(const uint8_t* bytes, size_t size) {
    static const char hex_chars[] = "0123456789ABCDEF";
    std::string hex_string;
    hex_string.reserve(size * 2);
    
    for (size_t i = 0; i < size; i++) {
        hex_string.push_back(hex_chars[(bytes[i] >> 4) & 0xF]);
        hex_string.push_back(hex_chars[bytes[i] & 0xF]);
    }
    
    return hex_string;
}

// Convert Tox address to a string representation
inline std::string tox_address_to_string(const uint8_t* address) {
    return tox_bytes_to_hex(address, TOX_ADDRESS_SIZE);
}

// Convert Tox public key to a string representation
inline std::string tox_public_key_to_string(const uint8_t* public_key) {
    return tox_bytes_to_hex(public_key, TOX_PUBLIC_KEY_SIZE);
}

// Validate if a string is a valid Tox ID (public key)
inline bool is_valid_tox_id(const std::string& id) {
    if (id.length() != TOX_PUBLIC_KEY_SIZE * 2) {
        return false;
    }
    
    for (char c : id) {
        if (!isxdigit(c)) {
            return false;
        }
    }
    
    return true;
}

} // namespace ToxUtil

#endif // TOX_UTIL_H 