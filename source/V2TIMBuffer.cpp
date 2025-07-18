#include "V2TIMBuffer.h"
#include <cstring> // for memcpy

// 默认构造函数
V2TIMBuffer::V2TIMBuffer() : buffer_(nullptr), length_(0) {}

// 拷贝构造函数
V2TIMBuffer::V2TIMBuffer(const V2TIMBuffer &buffer) : buffer_(nullptr), length_(buffer.length_) {
    if (buffer.length_ > 0) {
        buffer_ = new uint8_t[buffer.length_];
        std::memcpy(buffer_, buffer.buffer_, buffer.length_);
    }
}

// 从数据指针和大小构造
V2TIMBuffer::V2TIMBuffer(const uint8_t *data, size_t size) : buffer_(nullptr), length_(size) {
    if (size > 0 && data != nullptr) {
        buffer_ = new uint8_t[size];
        std::memcpy(buffer_, data, size);
    }
}

// 析构函数
V2TIMBuffer::~V2TIMBuffer() {
    if (buffer_) {
        delete[] buffer_;
        buffer_ = nullptr;
    }
    length_ = 0;
}

// 获取数据指针
const uint8_t *V2TIMBuffer::Data() const {
    return buffer_;
}

// 获取数据大小
size_t V2TIMBuffer::Size() const {
    return length_;
}

// 拷贝赋值运算符
V2TIMBuffer &V2TIMBuffer::operator=(const V2TIMBuffer &buffer) {
    if (this != &buffer) { // 避免自赋值
        // 释放当前内存
        if (buffer_) {
            delete[] buffer_;
            buffer_ = nullptr;
        }

        // 深拷贝
        length_ = buffer.length_;
        if (buffer.length_ > 0) {
            buffer_ = new uint8_t[buffer.length_];
            std::memcpy(buffer_, buffer.buffer_, buffer.length_);
        }
    }
    return *this;
}