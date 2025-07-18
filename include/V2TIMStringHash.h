#ifndef __V2TIM_STRING_HASH_H__
#define __V2TIM_STRING_HASH_H__

#include "V2TIMString.h"
#include <functional>

// Hash specialization for V2TIMString for use with STL containers
namespace std {
    template<>
    struct hash<V2TIMString> {
        size_t operator()(const V2TIMString& str) const {
            // Use std::hash<std::string> implementation for the underlying C string
            return std::hash<std::string>()(str.CString());
        }
    };
}

#endif /* __V2TIM_STRING_HASH_H__ */ 