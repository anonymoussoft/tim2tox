# Tim2Tox API 参考 — C FFI
> 语言 / Language: [中文](API_REFERENCE_FFI.md) | [English](API_REFERENCE_FFI.en.md)

本文档为 [API_REFERENCE.md](API_REFERENCE.md) 的 C FFI 部分。

## C FFI 接口

C FFI 接口提供 C 语言绑定，供 Dart FFI 调用。所有接口定义在 `ffi/tim2tox_ffi.h` 中。

### 初始化和清理

```c
// 初始化 SDK
int tim2tox_ffi_init(void);

// 反初始化 SDK
void tim2tox_ffi_uninit(void);

// 设置文件接收目录
int tim2tox_ffi_set_file_recv_dir(const char* dir_path);
```

### 登录

```c
// 登录
int tim2tox_ffi_login(const char* user_id, const char* user_sig);

// 获取当前登录用户 ID
int tim2tox_ffi_get_login_user(char* buffer, int buffer_len);
```

### 消息

```c
// 发送 C2C 文本消息
int tim2tox_ffi_send_c2c_text(const char* user_id, const char* text);

// 发送 C2C 自定义消息
int tim2tox_ffi_send_c2c_custom(const char* user_id, const unsigned char* data, int data_len);

// 发送群组自定义消息
int tim2tox_ffi_send_group_custom(const char* group_id, const unsigned char* data, int data_len);

// 轮询接收的文本消息
int tim2tox_ffi_poll_text(char* buffer, int buffer_len);

// 轮询接收的自定义消息
int tim2tox_ffi_poll_custom(unsigned char* buffer, int buffer_len);
```

### 好友

```c
// 添加好友
int tim2tox_ffi_add_friend(const char* user_id, const char* wording);

// 获取好友列表
int tim2tox_ffi_get_friend_list(char* buffer, int buffer_len);

// 获取好友申请列表
int tim2tox_ffi_get_friend_applications(char* buffer, int buffer_len);

// 接受好友申请
int tim2tox_ffi_accept_friend(const char* user_id);

// 删除好友
int tim2tox_ffi_delete_friend(const char* user_id);

// 设置输入状态
int tim2tox_ffi_set_typing(const char* user_id, int typing_on);
```

### 群组

```c
// 创建群组
// group_type: "group" (新 API) 或 "conference" (旧 API)
int tim2tox_ffi_create_group(const char* group_name, const char* group_type, char* out_group_id, int out_len);

// 加入群组（固定使用 group API）
int tim2tox_ffi_join_group(const char* group_id, const char* request_msg);

// 发送群组文本消息
int tim2tox_ffi_send_group_text(const char* group_id, const char* text);

// 存储群组类型
int tim2tox_ffi_set_group_type(const char* group_id, const char* group_type);

// 获取群组类型
int tim2tox_ffi_get_group_type_from_storage(const char* group_id, char* out_group_type, int out_len);
```

### 文件传输

```c
// 发送文件
int tim2tox_ffi_send_file(const char* user_id, const char* file_path);

// 控制文件传输
int tim2tox_ffi_file_control(const char* user_id, uint32_t file_number, int control);
```

### 信令（音视频通话）

```c
// 添加信令监听
int tim2tox_ffi_signaling_add_listener(
    tim2tox_signaling_invitation_callback_t on_invitation,
    tim2tox_signaling_cancel_callback_t on_cancel,
    tim2tox_signaling_accept_callback_t on_accept,
    tim2tox_signaling_reject_callback_t on_reject,
    tim2tox_signaling_timeout_callback_t on_timeout,
    void* user_data
);

// 邀请用户（1对1）
int tim2tox_ffi_signaling_invite(const char* invitee, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// 群组邀请
int tim2tox_ffi_signaling_invite_in_group(const char* group_id, const char* invitee_list, const char* data, int online_user_only, int timeout, char* out_invite_id, int out_invite_id_len);

// 取消邀请
int tim2tox_ffi_signaling_cancel(const char* invite_id, const char* data);

// 接受邀请
int tim2tox_ffi_signaling_accept(const char* invite_id, const char* data);

// 拒绝邀请
int tim2tox_ffi_signaling_reject(const char* invite_id, const char* data);
```

### 音视频（ToxAV）

```c
// 初始化 ToxAV
int tim2tox_ffi_av_initialize(int64_t instance_id);

// 关闭 ToxAV
void tim2tox_ffi_av_shutdown(int64_t instance_id);

// 迭代 ToxAV（需要在主循环中定期调用）
void tim2tox_ffi_av_iterate(int64_t instance_id);

// 发起通话
int tim2tox_ffi_av_start_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// 接听通话
int tim2tox_ffi_av_answer_call(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate, uint32_t video_bit_rate);

// 结束通话
int tim2tox_ffi_av_end_call(int64_t instance_id, uint32_t friend_number);

// 静音/取消静音音频
int tim2tox_ffi_av_mute_audio(int64_t instance_id, uint32_t friend_number, int mute);

// 隐藏/显示视频
int tim2tox_ffi_av_mute_video(int64_t instance_id, uint32_t friend_number, int hide);

// 发送音频帧
int tim2tox_ffi_av_send_audio_frame(int64_t instance_id, uint32_t friend_number, const int16_t* pcm, size_t sample_count, uint8_t channels, uint32_t sampling_rate);

// 发送视频帧（YUV420格式）
int tim2tox_ffi_av_send_video_frame(int64_t instance_id, uint32_t friend_number, uint16_t width, uint16_t height, const uint8_t* y, const uint8_t* u, const uint8_t* v, int32_t y_stride, int32_t u_stride, int32_t v_stride);

// 设置音视频码率
int tim2tox_ffi_av_set_audio_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t audio_bit_rate);
int tim2tox_ffi_av_set_video_bit_rate(int64_t instance_id, uint32_t friend_number, uint32_t video_bit_rate);

// 注册 ToxAV 回调
void tim2tox_ffi_av_set_call_callback(int64_t instance_id, tim2tox_av_call_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_call_state_callback(int64_t instance_id, tim2tox_av_call_state_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_audio_receive_callback(int64_t instance_id, tim2tox_av_audio_receive_callback_t callback, void* user_data);
void tim2tox_ffi_av_set_video_receive_callback(int64_t instance_id, tim2tox_av_video_receive_callback_t callback, void* user_data);
```

### IRC 通道桥接

```c
// 连接到 IRC 服务器并加入频道
int tim2tox_ffi_irc_connect_channel(const char* server, int port, const char* channel, const char* password, const char* group_id, const char* sasl_username, const char* sasl_password, int use_ssl, const char* custom_nickname);

// 断开 IRC 频道连接
int tim2tox_ffi_irc_disconnect_channel(const char* channel);

// 发送消息到 IRC 频道
int tim2tox_ffi_irc_send_message(const char* channel, const char* message);

// 检查 IRC 频道是否已连接
int tim2tox_ffi_irc_is_connected(const char* channel);
```

### 回调机制

```c
// 事件回调类型
typedef void (*tim2tox_event_cb)(int event_type, const char* sender, const unsigned char* payload, int payload_len, void* user_data);

// 注册事件回调
void tim2tox_ffi_set_callback(tim2tox_event_cb cb, void* user_data);
```
