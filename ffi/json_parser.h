// JSON message builder for Dart callbacks
#pragma once

#include <string>
#include <map>
#include <vector>

// Optionally use nlohmann/json for better JSON parsing
// Uncomment the following lines to use nlohmann/json instead of manual parsing:
// #define USE_NLOHMANN_JSON
#ifdef USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

// Global callback types (must match GlobalCallbackType in native_imsdk_bindings_generated.dart)
enum class GlobalCallbackType {
    NetworkStatus = 0,
    KickedOffline = 1,
    UserSigExpired = 2,
    SelfInfoUpdated = 3,
    UserStatusChanged = 4,
    UserInfoChanged = 5,
    LogCallback = 6,
    ReceiveNewMessage = 7,
    MessageElemUploadProgress = 8,
    MessageReadReceipt = 9,
    MessageRevoke = 10,
    MessageUpdate = 11,
    MessageExtensionsChanged = 12,
    MessageExtensionsDeleted = 13,
    MessageReactionChange = 14,
    AllMessageReceiveOption = 15,
    GroupPinnedMessageChanged = 16,
    GroupTipsEvent = 17,
    GroupAttributeChanged = 18,
    GroupCounterChanged = 19,
    TopicCreated = 20,
    TopicDeleted = 21,
    TopicChanged = 22,
    ReceiveTopicRESTCustomData = 23,
    CreatePermissionGroup = 24,
    DeletePermissionGroup = 25,
    ChangePermissionGroupInfo = 26,
    AddMembersToPermissionGroup = 27,
    RemoveMembersFromPermissionGroup = 28,
    AddTopicPermission = 29,
    DeleteTopicPermission = 30,
    ModifyTopicPermission = 31,
    ConversationEvent = 32,
    TotalUnreadMessageCountChanged = 33,
    TotalUnreadMessageCountChangedByFilter = 34,
    ConversationGroupCreated = 35,
    ConversationGroupDeleted = 36,
    ConversationGroupNameChanged = 37,
    ConversationsAddedToGroup = 38,
    ConversationsDeletedFromGroup = 39,
    AddFriend = 40,
    DeleteFriend = 41,
    UpdateFriendProfile = 42,
    FriendAddRequest = 43,
    FriendApplicationListDeleted = 44,
    FriendApplicationListRead = 45,
    FriendBlackListAdded = 46,
    FriendBlackListDeleted = 47,
    FriendGroupCreated = 48,
    FriendGroupDeleted = 49,
    FriendGroupNameChanged = 50,
    FriendsAddedToGroup = 51,
    FriendsDeletedFromGroup = 52,
    OfficialAccountSubscribed = 53,
    OfficialAccountUnsubscribed = 54,
    OfficialAccountDeleted = 55,
    OfficialAccountInfoChanged = 56,
    MyFollowingListChanged = 57,
    MyFollowersListChanged = 58,
    MutualFollowersListChanged = 59,
    SignalingReceiveNewInvitation = 60,
    SignalingInvitationCancelled = 61,
    SignalingInviteeAccepted = 62,
    SignalingInviteeRejected = 63,
    SignalingInvitationTimeout = 64,
    SignalingInvitationModified = 65,
    GroupCreated = 66,
    ToxAVCall = 67,
    ToxAVCallState = 68
};

// Build globalCallback JSON message
// Format: {"callback": "globalCallback", "callbackType": <type>, "instance_id": <id>, "json_...": <data>, "user_data": <user_data>}
std::string BuildGlobalCallbackJson(GlobalCallbackType callback_type, 
                                     const std::map<std::string, std::string>& json_fields,
                                     const std::string& user_data = "",
                                     int64_t instance_id = 0);

// Build apiCallback JSON message
// Format: {"callback": "apiCallback", "instance_id": <id>, "user_data": <user_data>, ...result_data...}
std::string BuildApiCallbackJson(const std::string& user_data,
                                  const std::map<std::string, std::string>& result_fields,
                                  int64_t instance_id = 0);

// Helper: Escape JSON string
std::string EscapeJsonString(const std::string& str);

// Helper: Build JSON object from map
std::string BuildJsonObject(const std::map<std::string, std::string>& fields);

// Helper: Parse JSON string to map (simple implementation)
// Note: This is a simplified parser for basic JSON objects
// For complex nested JSON, consider using a proper JSON library
std::map<std::string, std::string> ParseJsonString(const std::string& json_str);

// Helper: Extract string value from JSON
// silent: if true, don't log warnings (useful for fallback logic)
std::string ExtractJsonValue(const std::string& json_str, const std::string& key, bool silent = false);

// Helper: Extract int value from JSON
int ExtractJsonInt(const std::string& json_str, const std::string& key, int default_value = 0, bool silent = false);

// Helper: Extract bool value from JSON
bool ExtractJsonBool(const std::string& json_str, const std::string& key, bool default_value = false);

