#ifndef TIM_RESULT_DEFINE_H
#define TIM_RESULT_DEFINE_H

// General SDK errors
#define ERR_SDK_NOT_INITIALIZED         6013    // SDK not initialized
#define ERR_SDK_NOT_IMPLEMENTED         6014    // Feature not implemented
#define ERR_SDK_NOT_SUPPORTED           6015    // Feature not supported by Tox
#define ERR_SDK_FEATURE_NOT_SUPPORT     6022    // Feature not supported
#define ERR_SDK_INTERFACE_NOT_SUPPORT   7013    // Interface not supported
#define ERR_SDK_FRIEND_ADD_FAILED       7001    // Friend request failed
#define ERR_SDK_FRIEND_ADD_SELF         7002    // Cannot add yourself as friend
#define ERR_SDK_FRIEND_REQ_SENT         7003    // Friend request already sent
#define ERR_SDK_FRIEND_DELETE_FAILED    7004    // Friend deletion failed

// Server errors
#define ERR_SVR_FRIENDSHIP_INVALID      30001   // Invalid friendship relation

// Parameter errors
#define ERR_INVALID_PARAMETERS          8500    // Invalid parameters
#define ERR_OUT_OF_MEMORY               8501    // Out of memory

// Groups errors
#define ERR_SDK_GROUP_INVALID_ID        10002   // Invalid group ID

#endif // TIM_RESULT_DEFINE_H 