import '../models/fake_models.dart';

/// Optional provider interface for conversation management
/// 
/// This allows the SDK Platform to access conversation data without
/// directly depending on client-specific implementations.
/// 
/// If not provided, some methods may return errors or empty results.
abstract class ConversationManagerProvider {
  /// Get the list of conversations
  Future<List<FakeConversation>> getConversationList();
  
  /// Set conversation pinned status
  Future<void> setPinned(String conversationID, bool isPinned);
  
  /// Delete a conversation
  Future<void> deleteConversation(String conversationID);
  
  /// Get total unread count
  Future<int> getTotalUnreadCount();
}

