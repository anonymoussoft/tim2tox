#include "V2TIMString.h"
#include <cstring>
#include <string>

class V2TIMStringIMPL {
public:
    V2TIMStringIMPL() : data_(nullptr), size_(0) {}

    V2TIMStringIMPL(const char* str) {
        if (str) {
            size_ = strlen(str);
            data_ = new char[size_ + 1];
            memcpy(data_, str, size_);
            data_[size_] = '\0';
        } else {
            data_ = nullptr;
            size_ = 0;
        }
    }

    V2TIMStringIMPL(const char* str, size_t size) {
        if (str && size > 0) {
            size_ = size;
            data_ = new char[size_ + 1];
            memcpy(data_, str, size_);
            data_[size_] = '\0';
        } else {
            data_ = nullptr;
            size_ = 0;
        }
    }

    V2TIMStringIMPL(const V2TIMStringIMPL& other) {
        if (other.data_) {
            size_ = other.size_;
            data_ = new char[size_ + 1];
            memcpy(data_, other.data_, size_);
            data_[size_] = '\0';
        } else {
            data_ = nullptr;
            size_ = 0;
        }
    }

    ~V2TIMStringIMPL() {
        delete[] data_;
    }

    V2TIMStringIMPL& operator=(const V2TIMStringIMPL& other) {
        if (this != &other) {
            delete[] data_;
            if (other.data_) {
                size_ = other.size_;
                data_ = new char[size_ + 1];
                memcpy(data_, other.data_, size_);
                data_[size_] = '\0';
            } else {
                data_ = nullptr;
                size_ = 0;
            }
        }
        return *this;
    }

    size_t size() const { return size_; }
    const char* c_str() const { return data_ ? data_ : ""; }

private:
    char* data_;
    size_t size_;
};

// V2TIMString implementation
V2TIMString::V2TIMString() : impl_(new V2TIMStringIMPL()) {}

V2TIMString::V2TIMString(const char* str) : impl_(new V2TIMStringIMPL(str)) {}

V2TIMString::V2TIMString(const char* str, size_t size) : impl_(new V2TIMStringIMPL(str, size)) {}

V2TIMString::V2TIMString(const V2TIMString& str) : impl_(new V2TIMStringIMPL(*str.impl_)) {}

V2TIMString::V2TIMString(const std::string& str) : impl_(new V2TIMStringIMPL(str.c_str(), str.size())) {}

V2TIMString::~V2TIMString() {
    delete impl_;
}

V2TIMString& V2TIMString::operator=(const V2TIMString& str) {
    if (this != &str) {
        delete impl_;
        impl_ = new V2TIMStringIMPL(*str.impl_);
    }
    return *this;
}

V2TIMString& V2TIMString::operator=(const char* str) {
    delete impl_;
    impl_ = new V2TIMStringIMPL(str);
    return *this;
}

V2TIMString& V2TIMString::operator=(const std::string& str) {
    delete impl_;
    impl_ = new V2TIMStringIMPL(str.c_str(), str.size());
    return *this;
}

bool V2TIMString::operator==(const V2TIMString& str) const {
    return strcmp(impl_->c_str(), str.impl_->c_str()) == 0;
}

bool V2TIMString::operator!=(const V2TIMString& str) const {
    return !(*this == str);
}

bool V2TIMString::operator<(const V2TIMString& str) const {
    return strcmp(impl_->c_str(), str.impl_->c_str()) < 0;
}

char& V2TIMString::operator[](int index) {
    return const_cast<char&>(impl_->c_str()[index]);
}

size_t V2TIMString::Size() const {
    return impl_->size();
}

size_t V2TIMString::Length() const {
    return impl_->size();
}

bool V2TIMString::Empty() const {
    return impl_->size() == 0;
}

const char* V2TIMString::CString() const {
    return impl_->c_str();
} 