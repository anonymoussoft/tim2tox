// irc_client_api.h
// IRC Client Dynamic Library API
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize IRC client library
// Returns: 1 on success, 0 on failure
int irc_client_init(void);

// Shutdown IRC client library
void irc_client_shutdown(void);

// Connect to IRC server and join a channel
// server: IRC server address (e.g., "irc.libera.chat")
// port: IRC server port (default 6667, 6697 for SSL)
// channel: channel name (e.g., "#libera")
// password: channel password (optional, can be NULL)
// group_id: corresponding Tox group ID
// sasl_username: SASL authentication username (optional, can be NULL)
// sasl_password: SASL authentication password (optional, can be NULL)
// use_ssl: whether to use SSL/TLS (default 0)
// custom_nickname: custom IRC nickname (optional, can be NULL)
// Returns: 1 on success, 0 on failure
int irc_client_connect_channel(const char* server, int port, const char* channel, const char* password, const char* group_id, const char* sasl_username, const char* sasl_password, int use_ssl, const char* custom_nickname);

// Disconnect from an IRC channel
// channel: channel name
// Returns: 1 on success, 0 if channel not found
int irc_client_disconnect_channel(const char* channel);

// Send a message to an IRC channel
// channel: channel name
// message: message content
// Returns: 1 on success, 0 on failure
int irc_client_send_message(const char* channel, const char* message);

// Check if an IRC channel is connected
// channel: channel name
// Returns: 1 if connected, 0 otherwise
int irc_client_is_connected(const char* channel);

// Set callback for IRC messages (called when IRC message is received)
// callback: function(group_id, sender_nick, message)
// user_data: user data passed to callback
typedef void (*irc_message_callback_t)(const char* group_id, const char* sender_nick, const char* message, void* user_data);

// Register callback for IRC messages
// callback: callback function (can be NULL to disable)
// user_data: user data passed to callback
void irc_client_set_message_callback(irc_message_callback_t callback, void* user_data);

// Set callback for Tox group messages (called when Tox group message should be forwarded to IRC)
// callback: function(group_id, sender, message)
// user_data: user data passed to callback
typedef void (*tox_group_message_callback_t)(const char* group_id, const char* sender, const char* message, void* user_data);

// Register callback for Tox group messages
// callback: callback function (can be NULL to disable)
// user_data: user data passed to callback
void irc_client_set_tox_message_callback(tox_group_message_callback_t callback, void* user_data);

// Forward Tox group message to IRC (called from V2TIMManagerImpl)
// group_id: Tox group ID
// sender: sender nickname
// message: message content
// Returns: 1 if forwarded, 0 if not an IRC channel or error
int irc_client_forward_tox_message(const char* group_id, const char* sender, const char* message);

// Connection status enumeration
typedef enum {
    IRC_CONNECTION_DISCONNECTED = 0,
    IRC_CONNECTION_CONNECTING = 1,
    IRC_CONNECTION_CONNECTED = 2,
    IRC_CONNECTION_AUTHENTICATING = 3,
    IRC_CONNECTION_RECONNECTING = 4,
    IRC_CONNECTION_ERROR = 5
} irc_connection_status_t;

// Connection status callback
// channel: channel name
// status: connection status
// message: status message (optional, can be NULL)
// user_data: user data passed to callback
typedef void (*irc_connection_status_callback_t)(const char* channel, irc_connection_status_t status, const char* message, void* user_data);

// Register connection status callback
// callback: callback function (can be NULL to disable)
// user_data: user data passed to callback
void irc_client_set_connection_status_callback(irc_connection_status_callback_t callback, void* user_data);

// User list callback
// channel: channel name
// users: comma-separated list of user nicknames (can be NULL if empty)
// user_data: user data passed to callback
typedef void (*irc_user_list_callback_t)(const char* channel, const char* users, void* user_data);

// Register user list callback
// callback: callback function (can be NULL to disable)
// user_data: user data passed to callback
void irc_client_set_user_list_callback(irc_user_list_callback_t callback, void* user_data);

// User join/part callback
// channel: channel name
// nickname: user nickname
// joined: 1 if joined, 0 if parted
// user_data: user data passed to callback
typedef void (*irc_user_join_part_callback_t)(const char* channel, const char* nickname, int joined, void* user_data);

// Register user join/part callback
// callback: callback function (can be NULL to disable)
// user_data: user data passed to callback
void irc_client_set_user_join_part_callback(irc_user_join_part_callback_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

