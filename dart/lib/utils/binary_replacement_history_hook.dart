/// Binary replacement scheme history persistence hook
/// 
/// This hook provides message persistence for binary replacement scheme by:
/// 1. Wrapping V2TimAdvancedMsgListener to automatically save received messages
/// 2. Intercepting history message queries to load from persistence service
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'message_history_persistence.dart';
import 'message_converter.dart';

/// Binary replacement history hook
/// 
/// Provides hooks for binary replacement scheme to persist messages automatically
class BinaryReplacementHistoryHook {
  static MessageHistoryPersistence? _persistence;
  static String? _selfId;
  
  /// Initialize the hook with persistence service and self ID
  static void initialize(MessageHistoryPersistence persistence, String selfId) {
    _persistence = persistence;
    _selfId = selfId;
  }
  
  /// Wrap a V2TimAdvancedMsgListener to automatically save received messages
  /// 
  /// Returns a new listener that:
  /// 1. Calls the original listener's callbacks
  /// 2. Automatically saves received messages to persistence service
  static V2TimAdvancedMsgListener wrapListener(V2TimAdvancedMsgListener original) {
    return _WrappedAdvancedMsgListener(original);
  }
  
  /// Save a V2TimMessage to persistence (public for external use)
  /// 
  /// CRITICAL: For binary replacement scheme, FfiChatService already saves messages
  /// via _appendHistory when receiving/sending. This hook should only save messages
  /// that come through UIKit's listeners (onRecvNewMessage/onRecvMessageModified)
  /// and are NOT already saved by FfiChatService.
  /// 
  /// However, since we can't easily distinguish between messages from FfiChatService
  /// and messages from UIKit listeners, we check for duplicates before saving.
  static Future<void> saveMessage(V2TimMessage v2Msg) async {
    if (_persistence == null || _selfId == null) return;

    try {
      // CRITICAL: Skip messages with m-prefix IDs from native FFI callbacks.
      // These are duplicate messages created by HandleFriendMessage in C++ with temporary IDs.
      // The same message will arrive through the Dart stream path with a real ID.
      // Saving m-prefix messages causes duplicates in persistence/history.
      final msgID = v2Msg.msgID ?? v2Msg.id ?? '';
      if (msgID.startsWith('m') && RegExp(r'^m\d+-\d+$').hasMatch(msgID)) {
        return;
      }

      // Convert V2TimMessage to ChatMessage
      final chatMsg = MessageConverter.v2TimMessageToChatMessage(v2Msg, _selfId!);
      
      // Determine conversation ID
      final conversationId = v2Msg.groupID ?? v2Msg.userID ?? '';
      if (conversationId.isEmpty) return;
      
      // CRITICAL: Check if message already exists in persistence to avoid duplicates
      // For binary replacement scheme, FfiChatService already saves messages when receiving/sending
      // This hook should only save messages that are NOT already saved by FfiChatService
      final existingHistory = _persistence!.getHistory(conversationId);
      
      // Check if message already exists by msgID or by content (text + timestamp within 5 seconds)
      final chatMsgID = chatMsg.msgID;
      final messageExists = existingHistory.any((msg) {
        // Check by msgID first (most reliable)
        if (chatMsgID != null && msg.msgID == chatMsgID) return true;
        
        // Check by content (text + timestamp) as fallback
        // This handles cases where msgID might differ between UIKit and FFI layer
        if (chatMsg.text.isNotEmpty && msg.text == chatMsg.text) {
          final timeDiff = chatMsg.timestamp.difference(msg.timestamp).abs();
          if (timeDiff.inSeconds <= 5 && chatMsg.isSelf == msg.isSelf) {
            return true;
          }
        }
        
        return false;
      });
      
      if (messageExists) {
        // Message already exists, skip saving to avoid duplicate
        return;
      }
      
      // Save to persistence only if message doesn't exist
      _persistence!.appendHistory(conversationId, chatMsg);
    } catch (e) {
      // Silently handle errors to avoid breaking the app
    }
  }
}

/// Wrapped V2TimAdvancedMsgListener that automatically saves messages
class _WrappedAdvancedMsgListener extends V2TimAdvancedMsgListener {
  final V2TimAdvancedMsgListener original;
  
  _WrappedAdvancedMsgListener(this.original) : super(
    onRecvNewMessage: (V2TimMessage message) {
      // Save message to persistence before calling original callback
      BinaryReplacementHistoryHook.saveMessage(message);
      // Call original callback
      original.onRecvNewMessage(message);
    },
    onRecvMessageModified: (V2TimMessage message) {
      // CRITICAL: Also save modified messages (e.g., sent messages that change from pending to sent)
      // This ensures sent messages are persisted even if they're notified via onRecvMessageModified
      BinaryReplacementHistoryHook.saveMessage(message);
      // Call original callback
      original.onRecvMessageModified(message);
    },
    onSendMessageProgress: original.onSendMessageProgress,
    onRecvC2CReadReceipt: original.onRecvC2CReadReceipt,
    onRecvMessageRevoked: original.onRecvMessageRevoked,
    onRecvMessageReadReceipts: original.onRecvMessageReadReceipts,
    onRecvMessageExtensionsChanged: original.onRecvMessageExtensionsChanged,
    onRecvMessageExtensionsDeleted: original.onRecvMessageExtensionsDeleted,
    onMessageDownloadProgressCallback: original.onMessageDownloadProgressCallback,
    onRecvMessageReactionsChanged: original.onRecvMessageReactionsChanged,
    onRecvMessageRevokedWithInfo: original.onRecvMessageRevokedWithInfo,
    onGroupMessagePinned: original.onGroupMessagePinned,
  );
}
