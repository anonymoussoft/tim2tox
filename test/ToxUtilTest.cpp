#include <gtest/gtest.h>
#include "ToxUtil.h"

// Test tox_hex_to_bytes function
TEST(ToxUtilTest, HexToBytes) {
    // Test valid hex string
    const char* hex_string = "0123456789ABCDEF";
    uint8_t bytes[8];
    
    bool result = ToxUtil::tox_hex_to_bytes(hex_string, strlen(hex_string), bytes, sizeof(bytes));
    EXPECT_TRUE(result);
    EXPECT_EQ(bytes[0], 0x01);
    EXPECT_EQ(bytes[1], 0x23);
    EXPECT_EQ(bytes[2], 0x45);
    EXPECT_EQ(bytes[3], 0x67);
    EXPECT_EQ(bytes[4], 0x89);
    EXPECT_EQ(bytes[5], 0xAB);
    EXPECT_EQ(bytes[6], 0xCD);
    EXPECT_EQ(bytes[7], 0xEF);

    // Test invalid hex string (non-hex character)
    const char* invalid_hex = "0123456Z89ABCDEF";
    result = ToxUtil::tox_hex_to_bytes(invalid_hex, strlen(invalid_hex), bytes, sizeof(bytes));
    EXPECT_FALSE(result);

    // Test odd length hex string
    const char* odd_hex = "0123456";
    result = ToxUtil::tox_hex_to_bytes(odd_hex, strlen(odd_hex), bytes, sizeof(bytes));
    EXPECT_FALSE(result);

    // Test buffer too small
    const char* long_hex = "0123456789ABCDEF0123456789ABCDEF";
    result = ToxUtil::tox_hex_to_bytes(long_hex, strlen(long_hex), bytes, sizeof(bytes));
    EXPECT_FALSE(result);
}

// Test tox_bytes_to_hex function
TEST(ToxUtilTest, BytesToHex) {
    uint8_t bytes[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    
    std::string hex = ToxUtil::tox_bytes_to_hex(bytes, sizeof(bytes));
    EXPECT_EQ(hex, "0123456789ABCDEF");
    
    // Test empty bytes
    hex = ToxUtil::tox_bytes_to_hex(nullptr, 0);
    EXPECT_EQ(hex, "");
}

// Test tox_public_key_to_string
TEST(ToxUtilTest, PublicKeyToString) {
    uint8_t public_key[TOX_PUBLIC_KEY_SIZE] = {0};
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        public_key[i] = i;
    }
    
    std::string key_string = ToxUtil::tox_public_key_to_string(public_key);
    EXPECT_EQ(key_string.length(), TOX_PUBLIC_KEY_SIZE * 2);
    
    // Convert back to bytes and check
    uint8_t check_bytes[TOX_PUBLIC_KEY_SIZE];
    bool result = ToxUtil::tox_hex_to_bytes(key_string.c_str(), key_string.length(), 
                                           check_bytes, TOX_PUBLIC_KEY_SIZE);
    EXPECT_TRUE(result);
    
    for (int i = 0; i < TOX_PUBLIC_KEY_SIZE; i++) {
        EXPECT_EQ(check_bytes[i], i);
    }
}

// Test is_valid_tox_id
TEST(ToxUtilTest, IsValidToxId) {
    // Generate a valid ID (64 hex chars)
    std::string valid_id;
    for (int i = 0; i < 64; i++) {
        valid_id += "0123456789ABCDEF"[i % 16];
    }
    
    EXPECT_TRUE(ToxUtil::is_valid_tox_id(valid_id));
    
    // Test invalid ID (too short)
    std::string short_id = valid_id.substr(0, 63);
    EXPECT_FALSE(ToxUtil::is_valid_tox_id(short_id));
    
    // Test invalid ID (too long)
    std::string long_id = valid_id + "0";
    EXPECT_FALSE(ToxUtil::is_valid_tox_id(long_id));
    
    // Test invalid ID (non-hex character)
    std::string invalid_id = valid_id;
    invalid_id[32] = 'Z';
    EXPECT_FALSE(ToxUtil::is_valid_tox_id(invalid_id));
} 