/// Conversation ID normalization utilities
/// 
/// Provides unified conversation ID normalization to ensure consistent
/// storage and retrieval across different layers of the application.
/// 
/// This utility handles:
/// - Removing prefixes (c2c_, group_)
/// - Normalizing length (Tox public key is 64 chars)
/// - Sanitizing for filename usage
library;

/// Conversation ID normalization utilities
class ConversationIdUtils {
  /// Normalize a conversation ID
  /// 
  /// Steps:
  /// 1. Remove prefix (c2c_ or group_) if present
  /// 2. Trim whitespace
  /// 3. Normalize length (take first 64 chars for Tox public key)
  /// 
  /// This ensures consistent ID format across all layers.
  /// 
  /// Examples:
  /// - `c2c_10F189D746EF383F...` -> `10F189D746EF383F...` (first 64 chars)
  /// - `group_1234567890` -> `1234567890`
  /// - `10F189D746EF383F1A731AFDBE72FCA99289E817721D5F25C1C67D8B351E034F123456` -> `10F189D746EF383F1A731AFDBE72FCA99289E817721D5F25C1C67D8B351E034F`
  static String normalize(String id) {
    if (id.isEmpty) return id;
    
    String baseId = id.trim();
    
    // Remove prefix if present
    if (baseId.startsWith('c2c_')) {
      baseId = baseId.substring(4).trim();
    } else if (baseId.startsWith('group_')) {
      baseId = baseId.substring(6).trim();
    }
    
    // Normalize length: Tox public key is 64 characters (32 bytes in hex)
    // If longer, take first 64 chars; if shorter, keep as is
    if (baseId.length > 64) {
      baseId = baseId.substring(0, 64);
    }
    
    return baseId;
  }
  
  /// Sanitize conversation ID for use in filenames
  /// 
  /// First normalizes the ID, then replaces invalid filename characters
  /// with underscores.
  /// 
  /// Invalid characters: < > : " / \ | ? *
  /// 
  /// Example:
  /// - `c2c_10F189D746EF383F...` -> `10F189D746EF383F...` (normalized, then sanitized)
  static String sanitizeForFilename(String id) {
    final normalized = normalize(id);
    return normalized.replaceAll(RegExp(r'[<>:"/\\|?*]'), '_');
  }
  
  /// Check if two conversation IDs refer to the same conversation
  /// 
  /// Compares normalized versions of both IDs.
  /// 
  /// Returns true if they match after normalization.
  static bool equals(String id1, String id2) {
    return normalize(id1) == normalize(id2);
  }
  
  /// Extract base ID from a conversation ID (for backward compatibility)
  /// 
  /// This is similar to normalize() but preserves the original behavior
  /// where we might want to keep the prefix information.
  /// 
  /// For new code, use normalize() instead.
  @Deprecated('Use normalize() instead')
  static String extractBaseId(String id) {
    return normalize(id);
  }
  
  /// Get conversation type from ID
  /// 
  /// Returns 'c2c' for C2C conversations, 'group' for group conversations,
  /// or null if unknown.
  static String? getConversationType(String id) {
    final trimmed = id.trim();
    if (trimmed.startsWith('c2c_')) {
      return 'c2c';
    } else if (trimmed.startsWith('group_')) {
      return 'group';
    }
    return null;
  }
  
  /// Build full conversation ID with prefix
  /// 
  /// Adds the appropriate prefix based on conversation type.
  /// 
  /// [baseId] - The base ID (without prefix)
  /// [isGroup] - Whether this is a group conversation
  /// 
  /// Returns: `c2c_<baseId>` or `group_<baseId>`
  static String buildFullId(String baseId, {required bool isGroup}) {
    final normalized = normalize(baseId);
    return isGroup ? 'group_$normalized' : 'c2c_$normalized';
  }
}
