# Tim2Tox API Reference — C FFI
> Language: [Chinese](API_REFERENCE_FFI.md) | [English](API_REFERENCE_FFI.en.md)

This document is the C FFI section of [API_REFERENCE.en.md](API_REFERENCE.en.md).

## C FFI Interface

The C FFI interface provides C language bindings for Dart FFI calls. All interfaces are defined in `ffi/tim2tox_ffi.h`.

### Initialization and Cleanup

```c
// Initialize SDK
int tim2tox_ffi_init(void);

// Deinitialize SDK
void tim2tox_ffi_uninit(void);

// Set the file receiving directory
int tim2tox_ffi_set_file_recv_dir(const char* dir_path);
```

### Login

```c
// Login
int tim2tox_ffi_login(const char* user_id, const char* user_sig);

// Get the current logged in user ID
int tim2tox_ffi_get_login_user(char* buffer, int buffer_len);
```

### Messages

```c
// Send C2C text message
int tim2tox_ffi_send_c2c_text(const char* user_id, const char* text);

// Send C2C custom message
int tim2tox_ffi_send_c2c_custom(const char* user_id, const unsigned char* data, int data_len);

// Send group custom message
int tim2tox_ffi_send_group_custom(const char* group_id, const unsigned char* data, int data_len);

// Poll for received text messages
int tim2tox_ffi_poll_text(char* buffer, int buffer_len);

// Poll for received custom messages
int tim2tox_ffi_poll_custom(unsigned char* buffer, int buffer_len);
```

### Friendship

```c
// Add friend
int tim2tox_ffi_add_friend(const char* user_id, const char* wording);

// Get the friends list
int tim2tox_ffi_get_friend_list(char* buffer, int buffer_len);

// Get the friend application list
int tim2tox_ffi_get_friend_applications(char* buffer, int buffer_len);

// Accept friend request
int tim2tox_ffi_accept_friend(const char* user_id);

// Delete friends
int tim2tox_ffi_delete_friend(const char* user_id);

// Set input status
int tim2tox_ffi_set_typing(const char* user_id, int typing_on);
```

### Groups

```c
// Create group
// group_type: "group" (new API) or "conference" (old API)
int tim2tox_ffi_create_group(const char* group_name, const char* group_type, char* out_group_id, int out_len);

// Join the group (fixed use group API)
int tim2tox_ffi_join_group(const char* group_id, const char* request_msg);

// Send group text message
int tim2tox_ffi_send_group_text(const char* group_id, const char* text);

// Storage group type
int tim2tox_ffi_set_group_type(const char* group_id, const char* group_type);

// Get the group type
int tim2tox_ffi_get_group_type_from_storage(const char* group_id, char* out_group_type, int out_len);
```

### File Transfer

```c
// Send file
int tim2tox_ffi_send_file(const char* user_id, const char* file_path);

// Control file transfer
int tim2tox_ffi_file_control(const char* user_id, uint32_t file_number, int control);
```

### Signaling (Calling)

```c
// Add signaling monitoring
int tim2tox_ffi_signaling_add_listener(
    tim2tox_signaling_invitation_callback_t on_invitation,
    tim2tox_signaling_cancel_callback_t on_cancel,
    tim2tox_signaling_accept_callback_t on_accept,
    tim2tox_signaling_reject_callback_t on_reject,
    tim2tox_signaling_timeout_callback_t on_timeout,
    void* user_data
);

// Invite users (1 to 1)
int tim2tox_ffi_signaling_invite(const char* invitee, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// Group invitation
int tim2tox_ffi_signaling_invite_in_group(const char* group_id, const char* invitee_list, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// Cancel invitation
int tim2tox_ffi_signaling_cancel(const char* invite_id, const char* data);

// Accept invitation
int tim2tox_ffi_signaling_accept(const char* invite_id, const char* data);

// Decline invitation
int tim2tox_ffi_signaling_reject(const char* invite_id, const char* data);
```

### Audio and Video (ToxAV)

```c
// Initialize ToxAV
int tim2tox_ffi_av_initialize(int64_t instance_id);

// Close ToxAV
void tim2tox_ffi_av_shutdown(int64_t instance_id);

// Iterate ToxAV (needs to be called periodically in the main loop)
void tim2tox_ffi_av_iterate(int64_t instance_id);

// Initiate a call
int tim2tox_ffi_av_start_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// answer the call
int tim2tox_ffi_av_answer_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// end call
int tim2tox_ffi_av_end_call(int64_t instance_id, uint32_t friend_number);

// Mute/unmute audio
int tim2tox_ffi_av_mute_audio(int64_t instance_id, uint32_t friend_number, int mute);

// Hide/show video
int tim2tox_ffi_av_mute_video(int64_t instance_id, uint32_t friend_number, int hide);

// Send audio frame
int tim2tox_ffi_av_send_audio_frame(int64_t instance_id, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);

// Send video frame (YUV420 format)
int tim2tox_ffi_av_send_video_frame(int64_t instance_id, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t y_stride, int32_t u_stride, int32_t v_stride);

// Set audio and video bit rate
int tim2tox_ffi_av_set_audio_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate);
int tim2tox_ffi_av_set_video_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t video_bit_rate);

// Register ToxAV callback
void tim2tox_ffi_av_set_call_callback(int64_t instance_id, tim2tox_av_call_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_call_state_callback(int64_t instance_id, tim2tox_av_call_state_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_audio_receive_callback(int64_t instance_id, tim2tox_av_audio_receive_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_video_receive_callback(int64_t instance_id, tim2tox_av_video_receive_callback_t callback, void* user_data);
```

### IRC Channel Bridge

```c
// Connect to the IRC server and join the channel
int tim2tox_ffi_irc_connect_channel(const char* server, int port, const char* channel, const char* password, const char* group_id, const char* sasl_username, const char* sasl_password, int use_ssl, const char* custom_nickname);

// Disconnect IRC channel
int tim2tox_ffi_irc_disconnect_channel(const char* channel);

// Send message to IRC channel
int tim2tox_ffi_irc_send_message(const char* channel, const char* message);

// Check if the IRC channel is connected
int tim2tox_ffi_irc_is_connected(const char* channel);
```

### Callback Mechanism

```c
// Event callback type
typedef void (*tim2tox_event_cb)(int event_type, const char* sender, const unsigned char* payload, int payload_len, void* user_data);

// Register event callback
void tim2tox_ffi_set_callback(tim2tox_event_cb cb, void* user_data);
```
