#include "ToxAVManager.h"
#include <stdexcept>

// 单例实例获取
ToxAVManager& ToxAVManager::getInstance() {
    static ToxAVManager instance;
    return instance;
}

// 构造函数
ToxAVManager::ToxAVManager() : toxav_(nullptr, &toxavDeleter) {}

// 析构函数
ToxAVManager::~ToxAVManager() {
    shutdown();
}

// 自定义删除器实现
void ToxAVManager::toxavDeleter(ToxAV* toxav) {
    if (toxav) toxav_kill(toxav);
}

// 初始化实现
void ToxAVManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (toxav_) {
        throw std::runtime_error("ToxAV instance already initialized");
    }

    TOXAV_ERR_NEW error;
    toxav_.reset(toxav_new(ToxManager::getInstance().getTox(), &error));
    if (!toxav_ || error != TOXAV_ERR_NEW_OK) {
        throw std::runtime_error("ToxAV initialization failed: " + std::to_string(error));
    }

    // 设置回调
    toxav_callback_call(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, bool audio_enabled,
           bool video_enabled, void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->call_cb_) {
                self->call_cb_(friend_number, audio_enabled, video_enabled);
            }
        }, this);

    toxav_callback_call_state(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, uint32_t state, void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->call_state_cb_) {
                self->call_state_cb_(friend_number, state);
            }
        }, this);

    toxav_callback_audio_bit_rate(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, uint32_t audio_bit_rate,
           void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->audio_bitrate_cb_) {
                self->audio_bitrate_cb_(friend_number, audio_bit_rate);
            }
        }, this);

    toxav_callback_video_bit_rate(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, uint32_t video_bit_rate,
           void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->video_bitrate_cb_) {
                self->video_bitrate_cb_(friend_number, video_bit_rate);
            }
        }, this);

    toxav_callback_audio_receive_frame(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, const int16_t* pcm,
           size_t sample_count, uint8_t channels, uint32_t sampling_rate,
           void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->audio_receive_frame_cb_) {
                self->audio_receive_frame_cb_(friend_number, pcm, sample_count,
                                           channels, sampling_rate);
            }
        }, this);

    toxav_callback_video_receive_frame(toxav_.get(),
        [](ToxAV* av, uint32_t friend_number, uint16_t width, uint16_t height,
           const uint8_t* y, const uint8_t* u, const uint8_t* v,
           int32_t ystride, int32_t ustride, int32_t vstride, void* user_data) {
            auto self = static_cast<ToxAVManager*>(user_data);
            if (self && self->video_receive_frame_cb_) {
                self->video_receive_frame_cb_(friend_number, width, height, y, u, v);
            }
        }, this);
}

// 关闭实现
void ToxAVManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    toxav_.reset();
}

// 迭代实现
void ToxAVManager::iterate() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (toxav_) {
        toxav_iterate(toxav_.get());
    }
}

// 音频/视频控制实现
bool ToxAVManager::startCall(uint32_t friend_number, uint32_t audio_bit_rate,
                           uint32_t video_bit_rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_CALL error;
    return toxav_call(toxav_.get(), friend_number, audio_bit_rate,
                     video_bit_rate, &error) && error == TOXAV_ERR_CALL_OK;
}

bool ToxAVManager::endCall(uint32_t friend_number) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_CALL_CONTROL error;
    return toxav_call_control(toxav_.get(), friend_number,
                            TOXAV_CALL_CONTROL_CANCEL, &error) &&
           error == TOXAV_ERR_CALL_CONTROL_OK;
}

bool ToxAVManager::muteAudio(uint32_t friend_number, bool mute) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_CALL_CONTROL error;
    return toxav_call_control(toxav_.get(), friend_number,
                            mute ? TOXAV_CALL_CONTROL_MUTE_AUDIO
                                : TOXAV_CALL_CONTROL_UNMUTE_AUDIO,
                            &error) && error == TOXAV_ERR_CALL_CONTROL_OK;
}

bool ToxAVManager::muteVideo(uint32_t friend_number, bool mute) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_CALL_CONTROL error;
    return toxav_call_control(toxav_.get(), friend_number,
                            mute ? TOXAV_CALL_CONTROL_HIDE_VIDEO
                                : TOXAV_CALL_CONTROL_SHOW_VIDEO,
                            &error) && error == TOXAV_ERR_CALL_CONTROL_OK;
}

bool ToxAVManager::sendAudioFrame(uint32_t friend_number, const int16_t* pcm,
                                size_t sample_count, uint8_t channels,
                                uint32_t sampling_rate) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_SEND_FRAME error;
    return toxav_audio_send_frame(toxav_.get(), friend_number, pcm,
                                sample_count, channels, sampling_rate,
                                &error) && error == TOXAV_ERR_SEND_FRAME_OK;
}

bool ToxAVManager::sendVideoFrame(uint32_t friend_number, uint16_t width,
                                uint16_t height, const uint8_t* y,
                                const uint8_t* u, const uint8_t* v) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!toxav_) return false;
    TOXAV_ERR_SEND_FRAME error;
    return toxav_video_send_frame(toxav_.get(), friend_number, width, height,
                                y, u, v, &error) && error == TOXAV_ERR_SEND_FRAME_OK;
}

// 回调设置实现
void ToxAVManager::setCallCallback(CallCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    call_cb_ = cb;
}

void ToxAVManager::setCallStateCallback(CallStateCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    call_state_cb_ = cb;
}

void ToxAVManager::setAudioBitrateCallback(AudioBitrateCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_bitrate_cb_ = cb;
}

void ToxAVManager::setVideoBitrateCallback(VideoBitrateCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    video_bitrate_cb_ = cb;
}

void ToxAVManager::setAudioReceiveFrameCallback(AudioReceiveFrameCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_receive_frame_cb_ = cb;
}

void ToxAVManager::setVideoReceiveFrameCallback(VideoReceiveFrameCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    video_receive_frame_cb_ = cb;
} 