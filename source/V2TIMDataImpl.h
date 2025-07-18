#ifndef __V2TIM_DATA_IMPL_H__
#define __V2TIM_DATA_IMPL_H__

#include <vector>
#include "V2TIMString.h"

#ifndef IMPL_VECTOR
#define IMPL_VECTOR(class_name)\
    class TX##class_name##VectorIMPL {\
    public:\
        void PushBack(const class_name& obj) { data_.push_back(obj); }\
        void PushBack(class_name&& obj) { data_.push_back(std::move(obj)); }\
        void PopBack() { data_.pop_back(); }\
        class_name& operator[](size_t index) { return data_[index]; }\
        const class_name& operator[](size_t index) const { return data_[index]; }\
        size_t Size() const { return data_.size(); }\
        bool Empty() const { return data_.empty(); }\
        void Clear() { data_.clear(); }\
        void Erase(size_t index) { data_.erase(data_.begin() + index); }\
        TX##class_name##VectorIMPL() = default;\
        TX##class_name##VectorIMPL(const TX##class_name##VectorIMPL& other)\
            : data_(other.data_) {}\
        TX##class_name##VectorIMPL(TX##class_name##VectorIMPL&& other) noexcept\
            : data_(std::move(other.data_)) {}\
        TX##class_name##VectorIMPL& operator=(\
            const TX##class_name##VectorIMPL& other) {\
            if (this != &other) {\
                data_ = other.data_;\
            }\
            return *this;\
        }\
        TX##class_name##VectorIMPL& operator=(\
            TX##class_name##VectorIMPL&& other) noexcept {\
            if (this != &other) {\
                data_ = std::move(other.data_);\
            }\
            return *this;\
        }\
    private:\
        std::vector<class_name> data_;\
    };\
    \
    /* Vector class implementation */\
    TX##class_name##Vector::TX##class_name##Vector()\
        : impl_(new TX##class_name##VectorIMPL()) {}\
    \
    TX##class_name##Vector::TX##class_name##Vector(\
        const TX##class_name##Vector& vect)\
        : impl_(new TX##class_name##VectorIMPL(*vect.impl_)) {}\
    \
    TX##class_name##Vector::TX##class_name##Vector(\
        TX##class_name##Vector&& vect) noexcept\
        : impl_(vect.impl_) {\
        vect.impl_ = nullptr;\
    }\
    \
    TX##class_name##Vector::~TX##class_name##Vector() {\
        delete impl_;\
    }\
    \
    void TX##class_name##Vector::PushBack(const class_name& obj) {\
        impl_->PushBack(obj);\
    }\
    \
    void TX##class_name##Vector::PushBack(class_name&& obj) {\
        impl_->PushBack(std::move(obj));\
    }\
    \
    void TX##class_name##Vector::PopBack() {\
        impl_->PopBack();\
    }\
    \
    class_name& TX##class_name##Vector::operator[](size_t index) {\
        return (*impl_)[index];\
    }\
    \
    const class_name& TX##class_name##Vector::operator[](size_t index) const {\
        return (*impl_)[index];\
    }\
    \
    TX##class_name##Vector& TX##class_name##Vector::operator=(\
        const TX##class_name##Vector& vec) {\
        if (this != &vec) {\
            *impl_ = *vec.impl_;\
        }\
        return *this;\
    }\
    \
    TX##class_name##Vector& TX##class_name##Vector::operator=(\
        TX##class_name##Vector&& vec) noexcept {\
        if (this != &vec) {\
            delete impl_;\
            impl_ = vec.impl_;\
            vec.impl_ = nullptr;\
        }\
        return *this;\
    }\
    \
    size_t TX##class_name##Vector::Size() const {\
        return impl_->Size();\
    }\
    \
    bool TX##class_name##Vector::Empty() const {\
        return impl_->Empty();\
    }\
    \
    void TX##class_name##Vector::Clear() {\
        impl_->Clear();\
    }\
    \
    void TX##class_name##Vector::Erase(size_t index) {\
        impl_->Erase(index);\
    }

#endif // IMPL_VECTOR

#endif // __V2TIM_DATA_IMPL_H__
        