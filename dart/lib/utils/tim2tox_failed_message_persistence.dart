import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_elem_type.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_status.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';

/// Utility class for persisting and restoring failed messages.
/// When [accountToxId] is provided, storage is scoped per account (key suffix = first 16 chars of toxId).
class Tim2ToxFailedMessagePersistence {
  static const String _persistenceKey = 'tencent_cloud_chat_failed_messages';
  static const int _accountPrefixLen = 16;

  static String _storageKey(String? accountToxId) {
    if (accountToxId == null || accountToxId.isEmpty) return _persistenceKey;
    final prefix = accountToxId.length >= _accountPrefixLen
        ? accountToxId.substring(0, _accountPrefixLen)
        : accountToxId;
    return '${_persistenceKey}_$prefix';
  }

  static const int _messageTimeoutSeconds = 5; // Default timeout: 5 seconds for text messages
  static const int _fileMessageTimeoutSeconds = 300; // 5 minutes for file/image/video messages
  static const int _baseFileTimeoutSeconds = 60; // Base timeout: 60 seconds
  static const int _fileSizePerSecondBytes = 100 * 1024; // Assume 100KB/s upload speed minimum

  /// Save a failed message to local storage.
  /// [accountToxId] optional; when set, storage is scoped to this account.
  static Future<void> saveFailedMessage({
    required V2TimMessage message,
    String? userID,
    String? groupID,
    String? accountToxId,
  }) async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final key = _storageKey(accountToxId);
      final jsonString = prefs.getString(key);
      Map<String, dynamic> failedMessagesMap = {};
      
      if (jsonString != null && jsonString.isNotEmpty) {
        failedMessagesMap = json.decode(jsonString) as Map<String, dynamic>;
      }
      
      // Use conversation key (groupID or userID) as the key
      final conversationKey = groupID ?? userID ?? '';
      if (conversationKey.isEmpty) return;
      
      // Get or create list for this conversation
      List<Map<String, dynamic>> conversationFailedMessages = [];
      if (failedMessagesMap.containsKey(conversationKey)) {
        conversationFailedMessages = List<Map<String, dynamic>>.from(
          failedMessagesMap[conversationKey] as List
        );
      }
      
      // Create message data to save
      final messageData = {
        'id': message.id,
        'msgID': message.msgID,
        'timestamp': message.timestamp,
        'elemType': message.elemType,
        'text': message.textElem?.text,
        'userID': message.userID,
        'groupID': message.groupID,
        'isSelf': message.isSelf,
        'status': MessageStatus.V2TIM_MSG_STATUS_SEND_FAIL,
        'savedAt': DateTime.now().millisecondsSinceEpoch,
      };
      
      // Add or update message in the list
      final existingIndex = conversationFailedMessages.indexWhere(
        (m) => m['id'] == message.id || m['msgID'] == message.msgID
      );
      
      if (existingIndex >= 0) {
        conversationFailedMessages[existingIndex] = messageData;
      } else {
        conversationFailedMessages.add(messageData);
      }
      
      failedMessagesMap[conversationKey] = conversationFailedMessages;
      
      final updatedJsonString = json.encode(failedMessagesMap);
      await prefs.setString(key, updatedJsonString);
    } catch (e) {
      // Ignore errors during persistence
    }
  }

  /// Remove a message from failed messages (when it's successfully sent or deleted).
  /// [accountToxId] optional; when set, storage is scoped to this account.
  static Future<void> removeFailedMessage({
    required String messageID,
    String? userID,
    String? groupID,
    String? accountToxId,
  }) async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final key = _storageKey(accountToxId);
      final jsonString = prefs.getString(key);
      if (jsonString == null || jsonString.isEmpty) return;
      
      Map<String, dynamic> failedMessagesMap = json.decode(jsonString) as Map<String, dynamic>;
      final conversationKey = groupID ?? userID ?? '';
      if (conversationKey.isEmpty || !failedMessagesMap.containsKey(conversationKey)) return;
      
      List<Map<String, dynamic>> conversationFailedMessages = List<Map<String, dynamic>>.from(
        failedMessagesMap[conversationKey] as List
      );
      
      conversationFailedMessages.removeWhere(
        (m) => m['id'] == messageID || m['msgID'] == messageID
      );
      
      if (conversationFailedMessages.isEmpty) {
        failedMessagesMap.remove(conversationKey);
      } else {
        failedMessagesMap[conversationKey] = conversationFailedMessages;
      }
      
      final updatedJsonString = json.encode(failedMessagesMap);
      await prefs.setString(key, updatedJsonString);
    } catch (e) {
      // Ignore errors
    }
  }

  /// Load failed messages for a conversation.
  /// [accountToxId] optional; when set, storage is scoped to this account.
  static Future<List<Map<String, dynamic>>> loadFailedMessages({
    String? userID,
    String? groupID,
    String? accountToxId,
  }) async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final key = _storageKey(accountToxId);
      final jsonString = prefs.getString(key);
      if (jsonString == null || jsonString.isEmpty) return [];
      
      Map<String, dynamic> failedMessagesMap = json.decode(jsonString) as Map<String, dynamic>;
      final conversationKey = groupID ?? userID ?? '';
      if (conversationKey.isEmpty || !failedMessagesMap.containsKey(conversationKey)) return [];
      
      return List<Map<String, dynamic>>.from(
        failedMessagesMap[conversationKey] as List
      );
    } catch (e) {
      return [];
    }
  }

  /// Clear all failed messages (optional cleanup method).
  /// [accountToxId] optional; when set, only clears storage for this account.
  static Future<void> clearAllFailedMessages({String? accountToxId}) async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final key = _storageKey(accountToxId);
      await prefs.remove(key);
    } catch (e) {
      // Ignore errors
    }
  }

  /// Get timeout duration in seconds for a message
  /// For file/image/video messages, calculates timeout based on file size
  /// For text messages, returns default timeout
  static int getTimeoutSeconds(V2TimMessage? message) {
    if (message == null) {
      return _messageTimeoutSeconds;
    }
    
    // Check message type
    final elemType = message.elemType;
    
    // Text messages use short timeout
    if (elemType == MessageElemType.V2TIM_ELEM_TYPE_TEXT) {
      return _messageTimeoutSeconds;
    }
    
    // File messages (file, image, video, sound) need longer timeout
    int? fileSize;
    
    if (message.fileElem != null) {
      fileSize = message.fileElem!.fileSize;
    } else if (message.imageElem != null && message.imageElem!.imageList != null && message.imageElem!.imageList!.isNotEmpty) {
      // Use the original image size if available
      final originalImage = message.imageElem!.imageList!.firstWhere(
        (img) => img?.type == 0, // Original image type
        orElse: () => message.imageElem!.imageList!.first,
      );
      fileSize = originalImage?.size;
    } else if (message.videoElem != null) {
      fileSize = message.videoElem!.videoSize;
    } else if (message.soundElem != null) {
      fileSize = message.soundElem!.dataSize;
    }
    
    // If file size is available, calculate timeout based on size
    // Formula: base timeout + (file size / minimum upload speed)
    if (fileSize != null && fileSize > 0) {
      final sizeBasedTimeout = (fileSize / _fileSizePerSecondBytes).ceil();
      final totalTimeout = _baseFileTimeoutSeconds + sizeBasedTimeout;
      // Cap at maximum timeout
      return totalTimeout > _fileMessageTimeoutSeconds ? _fileMessageTimeoutSeconds : totalTimeout;
    }
    
    // For file messages without size info, use base file timeout
    return _baseFileTimeoutSeconds;
  }

  /// Get default timeout duration in seconds (for text messages)
  static int get timeoutSeconds => _messageTimeoutSeconds;
}



