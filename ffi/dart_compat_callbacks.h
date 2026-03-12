// Callback class definitions header
// This file contains inline definitions of callback classes that need to be visible
// to other modules for instantiation
#pragma once

#include "dart_compat_internal.h"
#include <functional>

// Ensure SendApiCallbackResult and BuildJsonObject are declared
// These are declared in dart_compat_internal.h and implemented in dart_compat_utils.cpp
// The declarations should be available via dart_compat_internal.h

// Forward declarations for helper functions (implemented in dart_compat_callbacks.cpp)
std::string FriendOperationResultToJson(const V2TIMFriendOperationResult& result);
std::string FriendOperationResultVectorToJson(const V2TIMFriendOperationResultVector& results);
std::string FriendInfoResultVectorToJson(const V2TIMFriendInfoResultVector& results);
std::string MessageVectorToJson(const V2TIMMessageVector& messages);
std::string MessageSearchResultToJson(const V2TIMMessageSearchResult& result);

// Note: SendApiCallbackResult and BuildJsonObject are declared in dart_compat_internal.h
// and implemented in dart_compat_utils.cpp

// Base callback class for V2TIMCallback
// Note: This is a simplified version - full implementation with user_data copying is in .cpp
class DartCallback : public V2TIMCallback {
private:
    void* user_data_;
    std::function<void()> on_success_;
    std::function<void(int, const V2TIMString&)> on_error_;
    
public:
    DartCallback(
        void* user_data,
        std::function<void()> on_success,
        std::function<void(int, const V2TIMString&)> on_error
    ) : user_data_(user_data), on_success_(on_success), on_error_(on_error) {}
    
    void OnSuccess() override {
        if (on_success_) on_success_();
    }
    
    void OnError(int error_code, const V2TIMString& error_message) override {
        if (on_error_) on_error_(error_code, error_message);
    }
};

// Callback for V2TIMValueCallback<V2TIMFriendOperationResult>
class DartFriendOperationResultCallback : public V2TIMValueCallback<V2TIMFriendOperationResult> {
private:
    void* user_data_;
    
public:
    DartFriendOperationResultCallback(void* user_data) : user_data_(user_data) {}
    
    void OnSuccess(const V2TIMFriendOperationResult& result) override {
        std::string result_json = FriendOperationResultToJson(result);
        std::map<std::string, std::string> result_fields;
        result_fields["friendOperationResult"] = result_json;
        std::string data_json = BuildJsonObject(result_fields);
        // CRITICAL: Use result.resultCode instead of hardcoded 0
        // This ensures that errors like TOX_ERR_FRIEND_ADD_ALREADY_SENT (30514) are properly propagated to Dart
        V2TIM_LOG(kInfo, "[DartFriendOperationResultCallback] OnSuccess: resultCode={}, resultInfo={}",
                  result.resultCode, result.resultInfo.CString());
        SendApiCallbackResult(user_data_, result.resultCode, result.resultInfo.CString(), data_json);
    }
    
    void OnError(int error_code, const V2TIMString& error_message) override {
        std::string error_msg = error_message.CString();
        SendApiCallbackResult(user_data_, error_code, error_msg);
    }
};

// Callback for V2TIMValueCallback<V2TIMFriendOperationResultVector>
class DartFriendOperationResultVectorCallback : public V2TIMValueCallback<V2TIMFriendOperationResultVector> {
private:
    void* user_data_;
    
public:
    DartFriendOperationResultVectorCallback(void* user_data) : user_data_(user_data) {}
    
    void OnSuccess(const V2TIMFriendOperationResultVector& results) override {
        std::string results_json = FriendOperationResultVectorToJson(results);
        
        // V2TimValueCallback expects json_param field at top level as a JSON string
        // Build the complete callback JSON with json_param as an escaped string
        std::string user_data_str = UserDataToString(user_data_);
        std::ostringstream json_msg;
        json_msg << "{\"callback\":\"apiCallback\",\"user_data\":\"" << EscapeJsonString(user_data_str) << "\",";
        json_msg << "\"code\":0,\"desc\":\"\",";
        json_msg << "\"json_param\":\"" << EscapeJsonString(results_json) << "\"}";
        
        SendCallbackToDart("apiCallback", json_msg.str(), user_data_);
    }
    
    void OnError(int error_code, const V2TIMString& error_message) override {
        std::string error_msg = error_message.CString();
        SendApiCallbackResult(user_data_, error_code, error_msg);
    }
};

// Callback for V2TIMValueCallback<V2TIMFriendInfoResultVector>
class DartFriendInfoResultVectorCallback : public V2TIMValueCallback<V2TIMFriendInfoResultVector> {
private:
    void* user_data_;
    
public:
    DartFriendInfoResultVectorCallback(void* user_data) : user_data_(user_data) {}
    
    void OnSuccess(const V2TIMFriendInfoResultVector& results) override {
        std::string results_json = FriendInfoResultVectorToJson(results);
        
        // V2TimValueCallback expects json_param field at top level as a JSON string
        // Build the complete callback JSON with json_param as an escaped string
        std::string user_data_str = UserDataToString(user_data_);
        std::ostringstream json_msg;
        json_msg << "{\"callback\":\"apiCallback\",\"user_data\":\"" << EscapeJsonString(user_data_str) << "\",";
        json_msg << "\"code\":0,\"desc\":\"\",";
        json_msg << "\"json_param\":\"" << EscapeJsonString(results_json) << "\"}";
        
        SendCallbackToDart("apiCallback", json_msg.str(), user_data_);
    }
    
    void OnError(int error_code, const V2TIMString& error_message) override {
        std::string error_msg = error_message.CString();
        SendApiCallbackResult(user_data_, error_code, error_msg);
    }
};

// Callback for V2TIMCompleteCallback<V2TIMMessage>
class DartMessageCompleteCallback : public V2TIMCompleteCallback<V2TIMMessage> {
private:
    void* user_data_;
    
public:
    DartMessageCompleteCallback(void* user_data) : user_data_(user_data) {}
    
    void OnComplete(int error_code, const V2TIMString& error_message, const V2TIMMessage& message) override {
        if (error_code == 0) {
            V2TIMMessageVector msg_vector;
            msg_vector.PushBack(message);
            std::string msg_json = MessageVectorToJson(msg_vector);
            std::map<std::string, std::string> result_fields;
            result_fields["messageList"] = msg_json;
            std::string data_json = BuildJsonObject(result_fields);
            SendApiCallbackResult(user_data_, 0, "", data_json);
        } else {
            std::string error_msg = error_message.CString();
            SendApiCallbackResult(user_data_, error_code, error_msg);
        }
    }
};

// Callback for V2TIMValueCallback<V2TIMMessageSearchResult>
class DartMessageSearchResultCallback : public V2TIMValueCallback<V2TIMMessageSearchResult> {
private:
    void* user_data_;
    
public:
    DartMessageSearchResultCallback(void* user_data) : user_data_(user_data) {}
    
    void OnSuccess(const V2TIMMessageSearchResult& result) override {
        std::string result_json = MessageSearchResultToJson(result);
        std::map<std::string, std::string> result_fields;
        result_fields["messageSearchResult"] = result_json;
        std::string data_json = BuildJsonObject(result_fields);
        SendApiCallbackResult(user_data_, 0, "", data_json);
    }
    
    void OnError(int error_code, const V2TIMString& error_message) override {
        std::string error_msg = error_message.CString();
        SendApiCallbackResult(user_data_, error_code, error_msg);
    }
};

