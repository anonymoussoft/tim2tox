// Other Miscellaneous Functions
// Extracted from dart_compat_layer.cpp for modularization
#include "dart_compat_internal.h"

extern "C" {
    // ============================================================================
    // Other Functions
    // ============================================================================
    
    // DartCallExperimentalAPI: Call experimental API
    // This function handles experimental/internal operations like setUIPlatform, setNetworkInfo, etc.
    // For Tox implementation, most of these operations are not needed, but we need to implement
    // the function to avoid symbol lookup errors.
    int DartCallExperimentalAPI(const char* json_param, void* user_data) {
        if (!json_param) {
            V2TIM_LOG(kError, "[dart_compat] DartCallExperimentalAPI: json_param is null");
            if (user_data) {
                SendApiCallbackResult(user_data, V2TIMErrorCode::ERR_INVALID_PARAMETERS, "json_param is null");
            }
            return V2TIMErrorCode::ERR_INVALID_PARAMETERS;
        }
        
        if (!user_data) {
            V2TIM_LOG(kError, "[dart_compat] DartCallExperimentalAPI: user_data is null");
            return V2TIMErrorCode::ERR_INVALID_PARAMETERS;
        }
        try {
            // Parse JSON parameters
            std::map<std::string, std::string> params = ParseJsonString(std::string(json_param));
            
            // Get the operation type
            std::string operation = params.count("request_internal_operation") > 0 
                ? params["request_internal_operation"] 
                : "";
            if (operation != "internal_operation_write_log") {
                V2TIM_LOG(kInfo, "[dart_compat] DartCallExperimentalAPI: operation={}", operation);
            }
            // Handle different operations
            if (operation == "internal_operation_set_ui_platform") {
                // setUIPlatform - not needed for Tox, just return success
                V2TIM_LOG(kInfo, "[dart_compat] DartCallExperimentalAPI: setUIPlatform (ignored for Tox)");
                SendApiCallbackResult(user_data, 0, "");
                return 0;
            } else if (operation == "internal_operation_set_network_info") {
                // setNetworkInfo - not needed for Tox, just return success
                V2TIM_LOG(kInfo, "[dart_compat] DartCallExperimentalAPI: setNetworkInfo (ignored for Tox)");
                SendApiCallbackResult(user_data, 0, "");
                return 0;
            } else if (operation == "internal_operation_write_log") {
                // writeLog - not needed for Tox, just return success (no log to avoid flooding)
                SendApiCallbackResult(user_data, 0, "");
                return 0;
            } else if (operation == "internal_operation_is_commercial_ability_enabled") {
                // checkAbility - return false (no commercial ability)
                V2TIM_LOG(kInfo, "[dart_compat] DartCallExperimentalAPI: checkAbility (returning false)");
                std::map<std::string, std::string> result_fields;
                result_fields["result"] = "false";
                std::string data_json = BuildJsonObject(result_fields);
                SendApiCallbackResult(user_data, 0, "", data_json);
                return 0;
            } else if (!operation.empty()) {
                // Unknown operation - log and return success (to avoid breaking the app)
                V2TIM_LOG(kInfo, "[dart_compat] DartCallExperimentalAPI: Unknown operation '{}' (returning success)", operation);
                SendApiCallbackResult(user_data, 0, "");
                return 0;
            } else {
                // No operation specified - return error
                V2TIM_LOG(kError, "[dart_compat] DartCallExperimentalAPI: No operation specified");
                SendApiCallbackResult(user_data, V2TIMErrorCode::ERR_INVALID_PARAMETERS, "No operation specified");
                return V2TIMErrorCode::ERR_INVALID_PARAMETERS;
            }
        } catch (const std::exception& e) {
            // Safely get exception message
            const char* what_msg = e.what();
            if (!what_msg) {
                what_msg = "Unknown exception (e.what() returned null)";
            }
            V2TIM_LOG(kError, "[dart_compat] DartCallExperimentalAPI: Exception: {}", what_msg);
            // Safely construct error message
            std::string error_msg = "Exception: ";
            error_msg += what_msg;
            SendApiCallbackResult(user_data, V2TIMErrorCode::ERR_INVALID_PARAMETERS, error_msg);
            return V2TIMErrorCode::ERR_INVALID_PARAMETERS;
        } catch (...) {
            V2TIM_LOG(kError, "[dart_compat] DartCallExperimentalAPI: Unknown exception");
            SendApiCallbackResult(user_data, V2TIMErrorCode::ERR_INVALID_PARAMETERS, "Unknown exception");
            return V2TIMErrorCode::ERR_INVALID_PARAMETERS;
        }
    }
    
    // Note: Other miscellaneous functions will be added here as they are extracted from dart_compat_layer.cpp
    // Expected functions include:
    // - DartCheckAbility (handled by DartCallExperimentalAPI)
    // - DartSetOfflinePushToken
    // - Other unclassified functions
    
} // extern "C"

