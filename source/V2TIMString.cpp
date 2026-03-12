#include "V2TIMString.h"
#include <cstring>
#include <cstdint>

class V2TIMStringIMPL {
public:
    V2TIMStringIMPL() : data_(nullptr), size_(0) {}

    V2TIMStringIMPL(const char* str) {
        if (str) {
            try {
                size_ = strlen(str);
                data_ = new char[size_ + 1];
                memcpy(data_, str, size_);
                data_[size_] = '\0';
            } catch (...) {
                data_ = nullptr;
                size_ = 0;
            }
        } else {
            data_ = nullptr;
            size_ = 0;
        }
    }

    V2TIMStringIMPL(const char* str, size_t size) {
        if (str && size > 0) {
            try {
                size_ = size;
                data_ = new char[size_ + 1];
                memcpy(data_, str, size_);
                data_[size_] = '\0';
            } catch (...) {
                data_ = nullptr;
                size_ = 0;
            }
        } else {
            data_ = nullptr;
            size_ = 0;
        }
    }

    V2TIMStringIMPL(const V2TIMStringIMPL& other) {
        // Safely copy other's data_ with exception handling
        try {
            // Get size and validate it's reasonable
            size_t other_size = other.size_;
            if (other_size > 1000000) {
                data_ = nullptr;
                size_ = 0;
                return;
            }
            
            // Copy data if other has valid data
            if (other.data_ && other_size > 0) {
                size_ = other_size;
                data_ = new char[size_ + 1];
                memcpy(data_, other.data_, size_);
                data_[size_] = '\0';
            } else {
                data_ = nullptr;
                size_ = 0;
            }
        } catch (...) {
            if (data_) {
                delete[] data_;
                data_ = nullptr;
            }
            size_ = 0;
        }
    }

    ~V2TIMStringIMPL() {
        if (data_) {
            try {
                delete[] data_;
            } catch (...) {
                // Silently handle exceptions during deletion
            }
            data_ = nullptr; // Prevent double-delete
        }
    }

    V2TIMStringIMPL& operator=(const V2TIMStringIMPL& other) {
        if (this != &other) {
            // Safely delete existing data_ before assigning new value
            if (data_) {
                try {
                    delete[] data_;
                } catch (...) {
                    // Ignore exceptions during deletion
                }
                data_ = nullptr; // Prevent double-delete
            }
            
            // Now safely assign new value
            try {
                size_t other_size = other.size_;
                if (other.data_ && other_size > 0 && other_size < 1000000) {
                    size_ = other_size;
                    data_ = new char[size_ + 1];
                    memcpy(data_, other.data_, size_);
                    data_[size_] = '\0';
                } else {
                    data_ = nullptr;
                    size_ = 0;
                }
            } catch (...) {
                data_ = nullptr;
                size_ = 0;
            }
        }
        return *this;
    }

    size_t size() const { 
        // This method should never crash, always return a valid size (possibly 0)
        try {
            // Validate size is reasonable before returning
            if (size_ > 1000000) {
                return 0;
            }
            return size_;
        } catch (...) {
            return 0;
        }
    }
    const char* c_str() const { 
        // This method should never crash, always return a valid C-string (possibly empty)
        try {
            // Check if data_ is null or size_ is 0
            if (!data_ || size_ == 0) {
                return "";
            }
            
            // Validate size is reasonable
            if (size_ > 1000000) {
                return "";
            }
            
            // Return the data pointer
            return data_;
        } catch (...) {
            return "";
        }
    }

private:
    char* data_;
    size_t size_;
};

// V2TIMString implementation
V2TIMString::V2TIMString() {
    try {
        impl_ = new V2TIMStringIMPL();
    } catch (const std::exception& e) {
        impl_ = nullptr;
    } catch (...) {
        impl_ = nullptr;
    }
}

V2TIMString::V2TIMString(const char* str) {
    try {
        impl_ = new V2TIMStringIMPL(str);
    } catch (const std::exception& e) {
        impl_ = new V2TIMStringIMPL();
    } catch (...) {
        impl_ = new V2TIMStringIMPL();
    }
}

V2TIMString::V2TIMString(const char* str, size_t size) {
    try {
        impl_ = new V2TIMStringIMPL(str, size);
    } catch (const std::exception& e) {
        impl_ = new V2TIMStringIMPL();
    } catch (...) {
        impl_ = new V2TIMStringIMPL();
    }
}

V2TIMString::V2TIMString(const V2TIMString& str) {
    // Safely copy str.impl_ with exception handling
    if (str.impl_) {
        try {
            impl_ = new V2TIMStringIMPL(*str.impl_);
        } catch (...) {
            impl_ = new V2TIMStringIMPL();
        }
    } else {
        impl_ = new V2TIMStringIMPL();
    }
}

V2TIMString::V2TIMString(const std::string& str) {
    try {
        impl_ = new V2TIMStringIMPL(str.c_str(), str.size());
    } catch (const std::exception& e) {
        impl_ = new V2TIMStringIMPL();
    } catch (...) {
        impl_ = new V2TIMStringIMPL();
    }
}

V2TIMString::~V2TIMString() {
    if (impl_) {
        try {
            delete impl_;
        } catch (...) {
            // Ignore exceptions during deletion
        }
        impl_ = nullptr; // Prevent double-delete
    }
}

V2TIMString& V2TIMString::operator=(const V2TIMString& str) {
    if (this != &str) {
        // Create new impl first, then swap (exception safety)
        V2TIMStringIMPL* new_impl = nullptr;
        try {
            if (str.impl_) {
                new_impl = new V2TIMStringIMPL(*str.impl_);
            } else {
                new_impl = new V2TIMStringIMPL();
            }
        } catch (...) {
            return *this;
        }
        
        // Store old impl_ pointer before replacing
        V2TIMStringIMPL* old_impl = impl_;
        
        // Replace impl_ first (atomic operation)
        impl_ = new_impl;
        
        // Now safely delete old impl_
        if (old_impl) {
            try {
                delete old_impl;
            } catch (...) {
                // Ignore exceptions during deletion
            }
        }
    }
    return *this;
}

V2TIMString& V2TIMString::operator=(const char* str) {
    // Create new impl first, then swap (exception safety)
    V2TIMStringIMPL* new_impl = nullptr;
    try {
        new_impl = new V2TIMStringIMPL(str ? str : "");
    } catch (...) {
        return *this;
    }
    
    // Store old impl_ pointer before replacing
    V2TIMStringIMPL* old_impl = impl_;
    
    // Replace impl_ first (atomic operation)
    impl_ = new_impl;
    
    // Now safely delete old impl_
    if (old_impl) {
        try {
            delete old_impl;
        } catch (...) {
            // Ignore exceptions during deletion
        }
    }
    
    return *this;
}

V2TIMString& V2TIMString::operator=(const std::string& str) {
    // Create new impl first, then swap (exception safety)
    V2TIMStringIMPL* new_impl = nullptr;
    try {
        new_impl = new V2TIMStringIMPL(str.c_str(), str.size());
    } catch (...) {
        return *this;
    }
    
    // Store old impl_ pointer before replacing
    V2TIMStringIMPL* old_impl = impl_;
    
    // Replace impl_ first (atomic operation)
    impl_ = new_impl;
    
    // Now safely delete old impl_
    if (old_impl) {
        try {
            delete old_impl;
        } catch (...) {
            // Ignore exceptions during deletion
        }
    }
    
    return *this;
}

bool V2TIMString::operator==(const V2TIMString& str) const {
    if (!impl_ || !str.impl_) {
        return impl_ == str.impl_;
    }
    // CRITICAL: Safely get c_str() with null checks
    const char* this_cstr = impl_->c_str();
    const char* str_cstr = str.impl_->c_str();
    if (!this_cstr || !str_cstr) {
        return this_cstr == str_cstr; // Both null or one null
    }
    return strcmp(this_cstr, str_cstr) == 0;
}

bool V2TIMString::operator!=(const V2TIMString& str) const {
    return !(*this == str);
}

bool V2TIMString::operator<(const V2TIMString& str) const {
    if (!impl_ || !str.impl_) {
        return impl_ < str.impl_;
    }
    // CRITICAL: Safely get c_str() with null checks
    const char* this_cstr = impl_->c_str();
    const char* str_cstr = str.impl_->c_str();
    if (!this_cstr || !str_cstr) {
        return this_cstr < str_cstr; // Compare pointers if either is null
    }
    return strcmp(this_cstr, str_cstr) < 0;
}

char& V2TIMString::operator[](int index) {
    // CRITICAL: Add null check and bounds checking to prevent crashes
    if (!impl_) {
        static char dummy = '\0';
        return dummy;
    }
    const char* cstr = impl_->c_str();
    if (!cstr) {
        static char dummy = '\0';
        return dummy;
    }
    size_t size = impl_->size();
    if (index < 0 || static_cast<size_t>(index) >= size) {
        static char dummy = '\0';
        return dummy;
    }
    return const_cast<char&>(cstr[index]);
}

size_t V2TIMString::Size() const {
    if (!impl_) {
        return 0;
    }
    try {
        return impl_->size();
    } catch (...) {
        return 0;
    }
}

size_t V2TIMString::Length() const {
    if (!impl_) {
        return 0;
    }
    try {
        return impl_->size();
    } catch (...) {
        return 0;
    }
}

bool V2TIMString::Empty() const {
    if (!impl_) {
        return true;
    }
    try {
        return impl_->size() == 0;
    } catch (...) {
        return true;
    }
}

const char* V2TIMString::CString() const {
    // This method should never crash, always return a valid C-string (possibly empty)
    // External code can safely call .CString() without needing SafeGetCString
    
    if (!impl_) {
        return "";
    }
    
    try {
        return impl_->c_str();
    } catch (...) {
        return "";
    }
} 