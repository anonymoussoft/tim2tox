// Utility functions and global variables for dart_compat_layer
#include "dart_compat_internal.h"
#include "V2TIMManagerImpl.h" // For V2TIMManagerImpl and GetCurrentInstance()

// Forward declaration for GetCurrentInstanceId (defined in tim2tox_ffi.cpp)
extern int64_t GetCurrentInstanceId();

// Per-instance listener storage (instance_id -> listener*)
// Use 0 for default instance, positive IDs for test instances
std::map<int64_t, DartSDKListenerImpl*> g_sdk_listeners;
std::map<int64_t, DartAdvancedMsgListenerImpl*> g_advanced_msg_listeners;
std::map<int64_t, DartConversationListenerImpl*> g_conversation_listeners;
std::map<int64_t, DartGroupListenerImpl*> g_group_listeners;
std::map<int64_t, DartFriendshipListenerImpl*> g_friendship_listeners;
std::map<int64_t, DartSignalingListenerImpl*> g_signaling_listeners;
std::map<int64_t, DartCommunityListenerImpl*> g_community_listeners;
std::mutex g_listeners_mutex;

// Helper function to get current instance ID
static int64_t GetCurrentInstanceIdSafe() {
    // Try to get current instance ID from tim2tox_ffi
    // If not available, return 0 (default instance)
    extern int64_t GetCurrentInstanceId();
    return GetCurrentInstanceId();
}

// GetOrCreate*Listener functions are now implemented in dart_compat_listeners.cpp
// where the listener class definitions are available

// CleanupInstanceListeners is now implemented in dart_compat_listeners.cpp
// where the complete listener class definitions are available

// Store user_data for each callback type per instance
// Key: (instance_id, callback_name)
// Use std::pair<int64_t, std::string> as key for per-instance storage
std::map<std::pair<int64_t, std::string>, void*> g_callback_user_data;
std::mutex g_callback_user_data_mutex;

// Helper function to store user_data (per-instance)
void StoreCallbackUserData(int64_t instance_id, const std::string& callback_name, void* user_data) {
    std::lock_guard<std::mutex> lock(g_callback_user_data_mutex);
    if (user_data) {
        g_callback_user_data[{instance_id, callback_name}] = user_data;
    }
}

// Helper function to get user_data (per-instance)
void* GetCallbackUserData(int64_t instance_id, const std::string& callback_name) {
    std::lock_guard<std::mutex> lock(g_callback_user_data_mutex);
    auto it = g_callback_user_data.find({instance_id, callback_name});
    if (it != g_callback_user_data.end()) {
        return it->second;
    }
    // Fallback: try instance_id=0 (default instance) for backward compatibility
    if (instance_id != 0) {
        auto fallback_it = g_callback_user_data.find({0, callback_name});
        if (fallback_it != g_callback_user_data.end()) {
            return fallback_it->second;
        }
    }
    return nullptr;
}

// Helper function to convert user_data pointer to string
// user_data is a pointer to a UTF-8 string created by Dart's toNativeUtf8()
std::string UserDataToString(void* user_data) {
    if (!user_data) return "";
    
    // user_data points to a null-terminated UTF-8 string
    const char* str = static_cast<const char*>(user_data);
    // Safety check: ensure str is not null before constructing std::string
    if (!str) {
        V2TIM_LOG(kError, "[dart_compat] UserDataToString: user_data pointer is valid but string pointer is null");
        return "";
    }
    
    // Maximum reasonable length for user_data string (e.g., "pinConversation-123")
    const size_t max_len = 256;
    size_t len = 0;
    
    // Try to safely determine string length by scanning for null terminator
    // Use a more defensive approach: check if we can read at least one byte
    // If we can't, the memory is likely invalid
    try {
        // First, try to read the first byte to verify memory is accessible
        // If this fails, the memory is invalid and we should return empty string
        volatile char first_char = str[0];
        (void)first_char;  // Suppress unused variable warning
        
        // If we got here, at least the first byte is accessible
        // Now scan for null terminator with limit
        for (len = 0; len < max_len; ++len) {
            volatile char c = str[len];
            if (c == '\0') {
                break;
            }
        }
        
        if (len == 0) {
            // First character was null terminator, return empty string
            return "";
        }
        if (len >= max_len) {
            // String might be longer, but we'll limit it to max_len for safety
            V2TIM_LOG(kWarning, "[dart_compat] UserDataToString: String length >= {}, truncating", max_len);
        }
        // Now safely construct std::string with known length
        return std::string(str, len);
    } catch (const std::exception& e) {
        // Safely get exception message
        const char* what_msg = e.what();
        if (!what_msg) {
            what_msg = "Unknown exception (e.what() returned null)";
        }
        V2TIM_LOG(kError, "[dart_compat] UserDataToString: Exception constructing string: {}", what_msg);
        return "";
    } catch (...) {
        V2TIM_LOG(kError, "[dart_compat] UserDataToString: Unknown exception constructing string (non-std::exception)");
        return "";
    }
}

// Forward declaration for GetCurrentInstance (defined in tim2tox_ffi.cpp)
extern V2TIMManagerImpl* GetCurrentInstance();

// Helper function to safely get V2TIMManager instance
// Returns nullptr if SDK is not initialized
// For multi-instance support, uses GetCurrentInstance() if available
V2TIMManager* SafeGetV2TIMManager() {
    extern int64_t GetCurrentInstanceId();
    int64_t current_instance_id = GetCurrentInstanceId();
    // Try to use current instance first (for test scenarios with multi-instance support)
    V2TIMManagerImpl* current = GetCurrentInstance();
    if (current) {
        extern int64_t GetInstanceIdFromManager(V2TIMManagerImpl* manager);
        int64_t manager_instance_id = GetInstanceIdFromManager(current);
        if (manager_instance_id != current_instance_id) {
            V2TIM_LOG(kWarning, "[dart_compat] SafeGetV2TIMManager: WARNING - Instance ID mismatch! manager_instance_id={}, current_instance_id={}",
                      (long long)manager_instance_id, (long long)current_instance_id);
            fflush(stderr);
        }
        return current;
    }

    // Fallback to default instance
    V2TIMManager* default_instance = V2TIMManager::GetInstance();
    if (!default_instance) {
        V2TIM_LOG(kWarning, "[dart_compat] SafeGetV2TIMManager: WARNING - Both current and default instances are null!");
    }
    return default_instance;
}

// Helper: Convert C string to std::string (handles null pointer)
std::string CStringToString(const char* str) {
    return str ? std::string(str) : std::string();
}

// Helper: Send API callback result to Dart (with string user_data)
// This version accepts a string directly, avoiding use-after-free issues
void SendApiCallbackResultWithString(const std::string& user_data_str, int code, const std::string& desc, const std::string& data_json) {
    try {
        // Get current instance_id
        extern int64_t GetCurrentInstanceId();
        int64_t instance_id = GetCurrentInstanceId();
        
        // V2TimValueCallback.fromJson expects json_param field, not data field
        // Build the complete callback JSON with json_param as an escaped string
        std::ostringstream json_msg;
        json_msg << "{\"callback\":\"apiCallback\",\"instance_id\":" << instance_id << ",\"user_data\":\"" << EscapeJsonString(user_data_str) << "\",";
        json_msg << "\"code\":" << code << ",\"desc\":\"" << EscapeJsonString(desc) << "\"";
        
        if (!data_json.empty()) {
            json_msg << ",\"json_param\":\"" << EscapeJsonString(data_json) << "\"";
        }
        
        json_msg << "}";
        std::string json_msg_str = json_msg.str();
        
        if (json_msg_str.empty()) {
            return;
        }
        
        SendCallbackToDart("apiCallback", json_msg_str, nullptr);
    } catch (...) {
        // Ignore callback errors
    }
}

// Helper: Send API callback result to Dart
void SendApiCallbackResult(void* user_data, int code, const std::string& desc, const std::string& data_json) {
    if (!user_data) {
        return;
    }
    
    try {
        // Get current instance_id
        extern int64_t GetCurrentInstanceId();
        int64_t instance_id = GetCurrentInstanceId();
        
        std::string user_data_str = UserDataToString(user_data);
        if (user_data_str.empty()) {
            return;
        }
        
        // V2TimValueCallback.fromJson expects json_param field, not data field
        // Build the complete callback JSON with json_param as an escaped string
        std::ostringstream json_msg;
        json_msg << "{\"callback\":\"apiCallback\",\"instance_id\":" << instance_id << ",\"user_data\":\"" << EscapeJsonString(user_data_str) << "\",";
        json_msg << "\"code\":" << code << ",\"desc\":\"" << EscapeJsonString(desc) << "\"";
        
        if (!data_json.empty()) {
            json_msg << ",\"json_param\":\"" << EscapeJsonString(data_json) << "\"";
        }
        
        json_msg << "}";
        std::string json_msg_str = json_msg.str();
        
        if (json_msg_str.empty()) {
            return;
        }
        
        SendCallbackToDart("apiCallback", json_msg_str, user_data);
    } catch (const std::exception& e) {
        // Safely get exception message
        const char* what_msg = e.what();
        if (!what_msg) {
            what_msg = "Unknown exception (e.what() returned null)";
        }
        V2TIM_LOG(kError, "[callback] SendApiCallbackResult: Exception: {}", what_msg);
    } catch (...) {
        V2TIM_LOG(kError, "[callback] SendApiCallbackResult: Unknown exception (non-std::exception)");
    }
}

// Helper: Parse JSON config string (uses json_parser functions)
std::map<std::string, std::string> ParseJsonConfig(const char* json_str) {
    if (!json_str) {
        return std::map<std::string, std::string>();
    }
    return ParseJsonString(std::string(json_str));
}

// Helper: Safely get CString from V2TIMString
// SafeGetCString is deprecated - use .CString() directly (it has built-in protection)
// This function is kept for backward compatibility but is no longer needed
std::string SafeGetCString(const V2TIMString& str) {
    // NOTE: V2TIMString::CString() now has comprehensive safety checks built-in
    // This function is now a simple wrapper that converts the C-string to std::string
    // External code can safely use .CString() directly
    
    try {
        // CString() now handles all safety checks internally and never crashes
        const char* cstr = str.CString();
        if (!cstr) {
            return "";
        }
        
        // Get size for safer string construction (avoid relying solely on null termination)
        size_t size = str.Size();
        if (size == 0) {
            return "";
        }
        
        // Validate size is reasonable before constructing std::string
        if (size > 1000000) {
            return "";
        }
        
        // Create std::string from C string with known size for extra safety
        // This avoids relying solely on null termination if memory is corrupted
        return std::string(cstr, size);
    } catch (...) {
        // CString() should never throw, but catch just in case
        return "";
    }
}

// Helper: Convert V2TIMConversationVector to JSON array
std::string ConversationVectorToJson(const V2TIMConversationVector& conversations) {
    std::ostringstream json;
    json << "[";
    
    // First, safely get the size
    size_t conv_count = 0;
    try {
        conv_count = conversations.Size();
    } catch (...) {
        V2TIM_LOG(kError, "[dart_compat] ConversationVectorToJson: Failed to get Size(), returning empty array");
        return "[]";
    }
    
    // Sanity check: if size is unreasonably large, return empty array
    if (conv_count > 1000000) {
        V2TIM_LOG(kError, "[dart_compat] ConversationVectorToJson: Invalid size {}, returning empty array", conv_count);
        return "[]";
    }
    
    for (size_t i = 0; i < conv_count; i++) {
        if (i > 0) {
            json << ",";
        }
        
        try {
            if (i >= conversations.Size()) {
                continue;
            }
            const V2TIMConversation& conv = conversations[i];
            json << "{";
            json << "\"conv_type\":" << static_cast<int>(conv.type) << ",";
            
            // Safely get strings (use SafeGetCString to avoid nullptr -> std::string)
            std::string conv_id_full, group_id, user_id, show_name, face_url, group_type_str, draft_text;
            try {
                conv_id_full = SafeGetCString(conv.conversationID);
                group_id = SafeGetCString(conv.groupID);
                user_id = SafeGetCString(conv.userID);
                show_name = SafeGetCString(conv.showName);
                face_url = SafeGetCString(conv.faceUrl);
                group_type_str = SafeGetCString(conv.groupType);
                draft_text = SafeGetCString(conv.draftText);
            } catch (...) {
                // If any access fails, use empty strings
            }
            
            // Extract base ID without prefix for conv_id
            // V2TimConversation.fromJson expects conv_id to be the base ID (without c2c_ or group_ prefix)
            // and will add the prefix based on conv_type
            std::string conv_id;
            if (conv.type == V2TIM_C2C) {
                // For C2C, use userID if available, otherwise extract from conversationID
                if (!user_id.empty()) {
                    conv_id = user_id;
                } else if (conv_id_full.length() > 4 && conv_id_full.substr(0, 4) == "c2c_") {
                    conv_id = conv_id_full.substr(4);
                } else {
                    conv_id = conv_id_full;
                }
            } else if (conv.type == V2TIM_GROUP) {
                // For GROUP, use groupID if available, otherwise extract from conversationID
                if (!group_id.empty()) {
                    conv_id = group_id;
                } else if (conv_id_full.length() > 6 && conv_id_full.substr(0, 6) == "group_") {
                    conv_id = conv_id_full.substr(6);
                } else {
                    conv_id = conv_id_full;
                }
            } else {
                // Fallback: use full conversationID
                conv_id = conv_id_full;
            }
            
            // Convert groupType string to integer enum value
            // CGroupType: groupPublic=0, groupPrivate=1, groupChatRoom=2, groupAVChatRoom=4, groupCommunity=5
            int group_type_enum = 0; // Default to Public
            if (group_type_str == "Work") {
                group_type_enum = 1; // groupPrivate
            } else if (group_type_str == "Public") {
                group_type_enum = 0; // groupPublic
            } else if (group_type_str == "Meeting") {
                group_type_enum = 2; // groupChatRoom
            } else if (group_type_str == "AVChatRoom") {
                group_type_enum = 4; // groupAVChatRoom
            } else if (group_type_str == "Community") {
                group_type_enum = 5; // groupCommunity
            }
            
            json << "\"conv_id\":\"" << EscapeJsonString(conv_id) << "\",";
            json << "\"conv_show_name\":\"" << EscapeJsonString(show_name) << "\",";
            json << "\"conv_face_url\":\"" << EscapeJsonString(face_url) << "\",";
            json << "\"conv_group_type\":" << group_type_enum << ",";
            json << "\"conv_unread_num\":" << conv.unreadCount << ",";
            json << "\"conv_is_pinned\":" << std::boolalpha << (conv.isPinned ? true : false) << ",";
            json << "\"conv_recv_opt\":" << static_cast<int>(conv.recvOpt) << ",";
            
            // orderkey (conv_active_time): when 0, send 0 so Dart can preserve existing order
            // (C++ cache often has orderKey=0; sending time(nullptr) would move item to top)
            uint64_t orderKey = (conv.orderKey > 0) ? static_cast<uint64_t>(conv.orderKey) : 0;
            json << "\"conv_active_time\":" << orderKey << ",";
            
            // lastMessage - wrap in try-catch to avoid segfault from invalid/dangling pointer
            if (conv.lastMessage != nullptr) {
                try {
                    std::string msg_id = SafeGetCString(conv.lastMessage->msgID);
                    std::string sender = SafeGetCString(conv.lastMessage->sender);
                    std::string user_id = SafeGetCString(conv.lastMessage->userID);
                    std::string group_id = SafeGetCString(conv.lastMessage->groupID);
                    int msg_conv_type = static_cast<int>(conv.type);
                    json << "\"conv_last_msg\":{";
                    json << "\"message_msg_id\":\"" << EscapeJsonString(msg_id) << "\",";
                    json << "\"message_seq\":" << conv.lastMessage->seq << ",";
                    json << "\"message_rand\":" << conv.lastMessage->random << ",";
                    json << "\"message_status\":" << static_cast<int>(conv.lastMessage->status) << ",";
                    json << "\"message_sender\":\"" << EscapeJsonString(sender) << "\",";
                    json << "\"message_user_id\":\"" << EscapeJsonString(user_id) << "\",";
                    json << "\"message_group_id\":\"" << EscapeJsonString(group_id) << "\",";
                    json << "\"message_conv_type\":" << msg_conv_type << ",";
                    json << "\"message_conv_id\":\"" << EscapeJsonString(conv_id) << "\",";
                    json << "\"message_client_time\":" << conv.lastMessage->timestamp << ",";
                    json << "\"message_server_time\":" << conv.lastMessage->timestamp << ",";
                    json << "\"message_is_from_self\":" << std::boolalpha << (conv.lastMessage->isSelf ? true : false) << ",";
                    json << "\"message_priority\":0,";
                    json << "\"message_is_read\":" << std::boolalpha << false << ",";
                    json << "\"message_is_peer_read\":" << std::boolalpha << false << ",";
                    json << "\"message_elem_array\":[]";
                    json << "},";
                } catch (const std::exception& e) {
                    V2TIM_LOG(kError, "[dart_compat] ConversationVectorToJson: exception serializing lastMessage: {}", e.what());
                    json << "\"conv_last_msg\":null,";
                } catch (...) {
                    V2TIM_LOG(kError, "[dart_compat] ConversationVectorToJson: unknown exception serializing lastMessage");
                    json << "\"conv_last_msg\":null,";
                }
            } else {
                json << "\"conv_last_msg\":null,";
            }
            
            // Draft information
            json << "\"conv_is_has_draft\":";
            if (!draft_text.empty()) {
                json << std::boolalpha << true << ",";
                json << "\"conv_draft\":{";
                // V2TimConversationDraft.fromJson expects: draft_msg, draft_user_define, draft_edit_time
                // Create a minimal draft message with text element
                json << "\"draft_msg\":{";
                json << "\"message_msg_id\":\"\",";
                json << "\"message_seq\":0,";
                json << "\"message_rand\":0,";
                json << "\"message_status\":0,";
                json << "\"message_client_time\":" << conv.draftTimestamp << ",";
                json << "\"message_server_time\":" << conv.draftTimestamp << ",";
                json << "\"message_is_from_self\":" << std::boolalpha << false << ",";
                json << "\"message_elem_array\":[{";
                json << "\"elem_type\":0,";
                json << "\"text_elem_content\":\"" << EscapeJsonString(draft_text) << "\"";
                json << "}]";
                json << "},";
                json << "\"draft_user_define\":null,";
                json << "\"draft_edit_time\":" << conv.draftTimestamp;
                json << "},";
            } else {
                json << std::boolalpha << false << ",";
                json << "\"conv_draft\":null,";
            }
            
            // Custom data (guard Size/Data to avoid invalid access)
            std::string custom_data_str;
            try {
                size_t cd_size = conv.customData.Size();
                if (cd_size > 0 && cd_size <= 4096 && conv.customData.Data() != nullptr) {
                    custom_data_str = std::string(reinterpret_cast<const char*>(conv.customData.Data()), cd_size);
                }
            } catch (...) {}
            json << "\"conv_custom_data\":\"" << EscapeJsonString(custom_data_str) << "\",";
            
            // Mark list (guard size)
            json << "\"conv_mark_array\":[";
            try {
                size_t mark_sz = conv.markList.Size();
                if (mark_sz <= 256) {
                    for (size_t j = 0; j < mark_sz; j++) {
                        if (j > 0) json << ",";
                        json << conv.markList[j];
                    }
                }
            } catch (...) {}
            json << "],";
            
            // Conversation group list (guard size and CString)
            json << "\"conv_conversation_group_array\":[";
            try {
                size_t cg_sz = conv.conversationGroupList.Size();
                if (cg_sz <= 64) {
                    for (size_t j = 0; j < cg_sz; j++) {
                        if (j > 0) json << ",";
                        json << "\"" << EscapeJsonString(conv.conversationGroupList[j].CString()) << "\"";
                    }
                }
            } catch (...) {}
            json << "],";
            
            // Group at info list (guard size)
            json << "\"conv_group_at_info_array\":[";
            try {
                size_t at_sz = conv.groupAtInfolist.Size();
                if (at_sz <= 64) {
                    for (size_t j = 0; j < at_sz; j++) {
                        if (j > 0) json << ",";
                        const V2TIMGroupAtInfo& atInfo = conv.groupAtInfolist[j];
                        json << "{";
                        json << "\"conv_group_at_info_seq\":" << atInfo.seq << ",";
                        json << "\"conv_group_at_info_at_type\":" << static_cast<int>(atInfo.atType);
                        json << "}";
                    }
                }
            } catch (...) {}
            json << "],";
            
            // Read timestamps
            json << "\"conv_c2c_read_timestamp\":" << conv.c2cReadTimestamp << ",";
            json << "\"conv_group_read_sequence\":" << conv.groupReadSequence;
            
            json << "}";
        } catch (const std::exception& e) {
            // Safely get exception message
            const char* what_msg = e.what();
            if (!what_msg) {
                what_msg = "Unknown exception (e.what() returned null)";
            }
            continue;
        } catch (...) {
            continue;
        }
    }
    
    json << "]";
    return json.str();
}

// ============================================================================
// Conversation ID Conversion Utilities
// ============================================================================

// Build full conversationID from base ID and type
// Used by conversation-related functions that need full conversationID (with prefix)
std::string BuildFullConversationID(const char* base_conv_id, unsigned int conv_type) {
    if (!base_conv_id) {
        return "";
    }
    
    std::string base_id(base_conv_id);
    std::string full_conv_id;
    
    // Check if base_id already has prefix (defensive check)
    if (base_id.length() >= 4 && base_id.substr(0, 4) == "c2c_") {
        // Already has c2c_ prefix, use as is
        full_conv_id = base_id;
    } else if (base_id.length() >= 6 && base_id.substr(0, 6) == "group_") {
        // Already has group_ prefix, use as is
        full_conv_id = base_id;
    } else {
        // Add prefix based on conv_type
        if (conv_type == 1) { // C2C
            full_conv_id = "c2c_" + base_id;
        } else if (conv_type == 2) { // GROUP
            full_conv_id = "group_" + base_id;
        } else {
            // Unknown type, use as is (should not happen with proper conv_type)
            full_conv_id = base_id;
        }
    }
    
    return full_conv_id;
}

// Extract base ID from conversationID (remove prefix if present)
// Used by message-related functions that need base ID (userID or groupID)
std::string ExtractBaseConversationID(const char* conv_id) {
    if (!conv_id) {
        return "";
    }
    
    std::string id(conv_id);
    
    // Remove prefix if present
    if (id.length() >= 4 && id.substr(0, 4) == "c2c_") {
        return id.substr(4);
    } else if (id.length() >= 6 && id.substr(0, 6) == "group_") {
        return id.substr(6);
    }
    
    // No prefix, return as is (already base ID)
    return id;
}



