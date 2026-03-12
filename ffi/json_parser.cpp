#include "json_parser.h"
#include "dart_compat_internal.h"  // Includes V2TIMLog.h
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <set>

using json = nlohmann::json;

// Escape JSON string
// Note: nlohmann/json handles escaping automatically, but we keep this function for backward compatibility
std::string EscapeJsonString(const std::string& str) {
    try {
        // Use nlohmann/json to handle escaping automatically
        json j = str;
        std::string escaped = j.dump();
        // Remove surrounding quotes added by dump() for strings
        if (escaped.length() >= 2 && escaped[0] == '"' && escaped[escaped.length()-1] == '"') {
            return escaped.substr(1, escaped.length() - 2);
        }
        return escaped;
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] EscapeJsonString exception: {}", e.what());
        // Fallback to manual escaping if nlohmann/json fails
        std::ostringstream o;
        size_t len = str.length();
        for (size_t i = 0; i < len; ++i) {
            char c = str[i];
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') 
                          << static_cast<int>(c);
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }
}

// Build JSON object from map using nlohmann/json
std::string BuildJsonObject(const std::map<std::string, std::string>& fields) {
    try {
        json j;
        
        // List of fields that should always be treated as JSON strings (for Dart compatibility)
        // These fields contain JSON arrays/objects that should be sent as escaped strings
        // so Dart can decode them with json.decode()
        static const std::set<std::string> jsonStringFields = {
            "json_conv_array",
            "json_msg_array",
            "json_user_info_array",
            "json_user_status_array",
            "json_msg_locator_array",
            "json_user_profile",
            "json_msg",
            "json_application_array",
            "json_identifier_array",
            "json_friend_profile_update_array"
        };
        
        for (const auto& [key, value] : fields) {
            bool isJsonStringField = (jsonStringFields.find(key) != jsonStringFields.end());
            
            if (isJsonStringField) {
                // Always treat as string, even if it looks like JSON
                j[key] = value;
            } else if (!value.empty() && (value[0] == '{' || value[0] == '[')) {
                // Value is already JSON, try to parse it
                try {
                    json parsed_value = json::parse(value);
                    j[key] = parsed_value;
                } catch (...) {
                    // If parsing fails, treat as string
                    j[key] = value;
                }
            } else {
                // Try to parse as number or boolean, otherwise treat as string
                try {
                    // Try parsing as number
                    if (value.find('.') != std::string::npos) {
                        // Float
                        double num = std::stod(value);
                        j[key] = num;
                    } else {
                        // Integer
                        int64_t num = std::stoll(value);
                        j[key] = num;
                    }
                } catch (...) {
                    // Not a number, try boolean
                    if (value == "true" || value == "false") {
                        j[key] = (value == "true");
                    } else {
                        // Treat as string
                        j[key] = value;
                    }
                }
            }
        }
        
        return j.dump();
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] BuildJsonObject exception: {}", e.what());
        // Fallback to manual building if nlohmann/json fails
        std::ostringstream json;
        json << "{";
        bool first = true;
        for (const auto& [key, value] : fields) {
            if (!first) {
                json << ",";
            }
            first = false;
            json << "\"" << EscapeJsonString(key) << "\":";
            
            bool isJsonStringField = (key == "json_conv_array" || 
                                     key == "json_msg_array" ||
                                     key == "json_user_info_array" ||
                                     key == "json_user_status_array" ||
                                     key == "json_msg_locator_array" ||
                                     key == "json_user_profile" ||
                                     key == "json_msg" ||
                                     key == "json_application_array" ||
                                     key == "json_identifier_array" ||
                                     key == "json_friend_profile_update_array");
            
            if (isJsonStringField) {
                json << "\"" << EscapeJsonString(value) << "\"";
            } else if (!value.empty() && (value[0] == '{' || value[0] == '[')) {
                json << value;
            } else {
                // Check if it's a number or boolean
                bool is_number = !value.empty() && 
                    (std::isdigit(value[0]) || value[0] == '-' || value[0] == '+');
                bool is_bool = (value == "true" || value == "false");
                
                if (is_number || is_bool) {
                    json << value;
                } else {
                    json << "\"" << EscapeJsonString(value) << "\"";
                }
            }
        }
        json << "}";
        return json.str();
    }
}

// Build globalCallback JSON message
std::string BuildGlobalCallbackJson(GlobalCallbackType callback_type,
                                     const std::map<std::string, std::string>& json_fields,
                                     const std::string& user_data,
                                     int64_t instance_id) {
    try {
        json j;
        j["callback"] = "globalCallback";
        j["callbackType"] = static_cast<int>(callback_type);
        j["instance_id"] = instance_id;

        for (const auto& [key, value] : json_fields) {
            try {
                json parsed_value = json::parse(value);
                j[key] = parsed_value;
            } catch (...) {
                j[key] = value;
            }
        }

        if (!user_data.empty()) {
            j["user_data"] = user_data;
        }

        std::string result = j.dump();
        return result;
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] BuildGlobalCallbackJson exception: {}", e.what());
        // Fallback to BuildJsonObject
        std::map<std::string, std::string> message;
        message["callback"] = "globalCallback";
        message["callbackType"] = std::to_string(static_cast<int>(callback_type));
        message["instance_id"] = std::to_string(instance_id);
        for (const auto& [key, value] : json_fields) {
            message[key] = value;
        }
        if (!user_data.empty()) {
            message["user_data"] = user_data;
        }
        return BuildJsonObject(message);
    }
}

// Build apiCallback JSON message
std::string BuildApiCallbackJson(const std::string& user_data,
                                  const std::map<std::string, std::string>& result_fields,
                                  int64_t instance_id) {
    try {
        json j;
        j["callback"] = "apiCallback";
        j["instance_id"] = instance_id;
        j["user_data"] = user_data;
        
        // Add all result fields
        for (const auto& [key, value] : result_fields) {
            // Try to parse value as JSON, otherwise treat as string
            try {
                json parsed_value = json::parse(value);
                j[key] = parsed_value;
            } catch (...) {
                j[key] = value;
            }
        }
        
        return j.dump();
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] BuildApiCallbackJson exception: {}", e.what());
        // Fallback to BuildJsonObject
        std::map<std::string, std::string> message;
        message["callback"] = "apiCallback";
        message["instance_id"] = std::to_string(instance_id);
        message["user_data"] = user_data;
        for (const auto& [key, value] : result_fields) {
            message[key] = value;
        }
        return BuildJsonObject(message);
    }
}

// Parse JSON string to map using nlohmann/json
std::map<std::string, std::string> ParseJsonString(const std::string& json_str) {
    std::map<std::string, std::string> result;
    
    if (json_str.empty()) {
        return result;
    }
    
    try {
        json j = json::parse(json_str);
        
        // Convert JSON object to flat map
        if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                if (value.is_string()) {
                    result[key] = value.get<std::string>();
                } else if (value.is_number_integer()) {
                    result[key] = std::to_string(value.get<int64_t>());
                } else if (value.is_number_float()) {
                    result[key] = std::to_string(value.get<double>());
                } else if (value.is_boolean()) {
                    result[key] = value.get<bool>() ? "true" : "false";
                } else if (value.is_null()) {
                    result[key] = "";
                } else if (value.is_object() || value.is_array()) {
                    // For nested objects/arrays, serialize to JSON string
                    result[key] = value.dump();
                }
            }
        }
    } catch (const json::parse_error& e) {
        V2TIM_LOG(kError, "[json_parser] ParseJsonString parse error at byte {}: {}", e.byte, e.what());
        // Return empty map on parse error
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] ParseJsonString exception: {}", e.what());
        // Return empty map on exception
    }
    
    return result;
}

// Extract string value from JSON using nlohmann/json
std::string ExtractJsonValue(const std::string& json_str, const std::string& key, bool silent) {
    try {
        json j = json::parse(json_str);
        
        if (j.contains(key)) {
            auto& value = j[key];
            if (value.is_string()) {
                return value.get<std::string>();
            } else if (value.is_number_integer()) {
                return std::to_string(value.get<int64_t>());
            } else if (value.is_number_float()) {
                return std::to_string(value.get<double>());
            } else if (value.is_boolean()) {
                return value.get<bool>() ? "true" : "false";
            } else if (value.is_null()) {
                return "";
            } else {
                // For objects/arrays, return JSON string
                return value.dump();
            }
        }
    } catch (const json::parse_error& e) {
        if (!silent) {
            V2TIM_LOG(kError, "[json_parser] ExtractJsonValue parse error for key '{}' at byte {}: {}", 
                     key, e.byte, e.what());
        }
    } catch (const std::exception& e) {
        if (!silent) {
            V2TIM_LOG(kError, "[json_parser] ExtractJsonValue exception for key '{}': {}", key, e.what());
        }
    }
    
    return "";
}

// Extract int value from JSON using nlohmann/json
int ExtractJsonInt(const std::string& json_str, const std::string& key, int default_value, bool silent) {
    try {
        json j = json::parse(json_str);
        
        if (j.contains(key) && j[key].is_number()) {
            return j[key].get<int>();
        }
    } catch (const json::parse_error& e) {
        if (!silent) {
            V2TIM_LOG(kError, "[json_parser] ExtractJsonInt parse error for key '{}' at byte {}: {}", 
                     key, e.byte, e.what());
        }
    } catch (const std::exception& e) {
        if (!silent) {
            V2TIM_LOG(kError, "[json_parser] ExtractJsonInt exception for key '{}': {}", key, e.what());
        }
    }
    
    return default_value;
}

// Extract bool value from JSON using nlohmann/json
bool ExtractJsonBool(const std::string& json_str, const std::string& key, bool default_value) {
    try {
        json j = json::parse(json_str);
        
        if (j.contains(key) && j[key].is_boolean()) {
            return j[key].get<bool>();
        }
    } catch (const json::parse_error& e) {
        V2TIM_LOG(kError, "[json_parser] ExtractJsonBool parse error for key '{}' at byte {}: {}", 
                 key, e.byte, e.what());
    } catch (const std::exception& e) {
        V2TIM_LOG(kError, "[json_parser] ExtractJsonBool exception for key '{}': {}", key, e.what());
    }
    
    return default_value;
}
