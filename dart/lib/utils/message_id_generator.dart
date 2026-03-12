/// Message ID generator
/// 
/// Provides unified message ID generation to ensure uniqueness
/// and prevent duplicates in high-concurrency scenarios.
/// 
/// Format: `<timestamp>_<sequence>_<userId>` (when sequence needed)
///         `<timestamp>_<userId>` (normal case)
library;

/// Message ID generator with sequence support
class MessageIdGenerator {
  final int? _instanceId;
  
  // Per-user sequence counter to ensure uniqueness within same millisecond
  final Map<String, int> _sequenceByUser = {};
  
  // Last timestamp per user to detect same-millisecond messages
  final Map<String, int> _lastTimestampByUser = {};
  
  /// Create a MessageIdGenerator with optional instance ID for multi-instance support
  MessageIdGenerator({int? instanceId}) : _instanceId = instanceId;
  
  /// Generate a unique message ID
  /// 
  /// Format:
  /// - Normal: `<timestamp>_<userId>`
  /// - Same millisecond: `<timestamp>_<sequence>_<userId>`
  /// 
  /// [userId] - The user ID (sender for sent messages, receiver for received)
  /// 
  /// Returns a unique message ID string.
  String generate(String userId) {
    final now = DateTime.now().millisecondsSinceEpoch;
    final normalizedUserId = _normalizeUserId(userId);
    
    final lastTs = _lastTimestampByUser[normalizedUserId] ?? 0;
    
    if (now <= lastTs) {
      // Same millisecond or clock went backward, use sequence
      final sequence = (_sequenceByUser[normalizedUserId] ?? 0) + 1;
      _sequenceByUser[normalizedUserId] = sequence;
      _lastTimestampByUser[normalizedUserId] = lastTs; // Keep last timestamp
      return '${lastTs}_${sequence}_$normalizedUserId';
    } else {
      // New millisecond, reset sequence
      _sequenceByUser[normalizedUserId] = 0;
      _lastTimestampByUser[normalizedUserId] = now;
      return '${now}_$normalizedUserId';
    }
  }
  
  /// Generate message ID with explicit timestamp (for message import/replay)
  /// 
  /// Useful when importing messages or replaying events with specific timestamps.
  /// 
  /// [userId] - The user ID
  /// [timestamp] - Explicit timestamp in milliseconds
  /// 
  /// Returns a message ID string.
  String generateWithTimestamp(String userId, int timestamp) {
    final normalizedUserId = _normalizeUserId(userId);
    final lastTs = _lastTimestampByUser[normalizedUserId] ?? 0;
    
    if (timestamp <= lastTs) {
      // Timestamp is not newer, use sequence
      final sequence = (_sequenceByUser[normalizedUserId] ?? 0) + 1;
      _sequenceByUser[normalizedUserId] = sequence;
      return '${timestamp}_${sequence}_$normalizedUserId';
    } else {
      // New timestamp, reset sequence
      _sequenceByUser[normalizedUserId] = 0;
      _lastTimestampByUser[normalizedUserId] = timestamp;
      return '${timestamp}_$normalizedUserId';
    }
  }
  
  /// Normalize user ID for consistent message ID generation
  /// 
  /// Takes first 64 characters (Tox public key length) if longer.
  static String _normalizeUserId(String userId) {
    final trimmed = userId.trim();
    return trimmed.length > 64 ? trimmed.substring(0, 64) : trimmed;
  }
  
  /// Reset sequence for a user (useful for testing or cleanup)
  void resetSequence(String userId) {
    final normalizedUserId = _normalizeUserId(userId);
    _sequenceByUser.remove(normalizedUserId);
    _lastTimestampByUser.remove(normalizedUserId);
  }
  
  /// Reset all sequences (useful for testing)
  void resetAll() {
    _sequenceByUser.clear();
    _lastTimestampByUser.clear();
  }
  
  /// Parse message ID to extract components
  /// 
  /// Returns a map with 'timestamp', 'sequence' (optional), and 'userId'.
  /// Returns null if format is invalid.
  static Map<String, dynamic>? parse(String msgID) {
    final parts = msgID.split('_');
    if (parts.length < 2) return null;
    
    final timestamp = int.tryParse(parts[0]);
    if (timestamp == null) return null;
    
    if (parts.length == 2) {
      // Format: timestamp_userId
      return {
        'timestamp': timestamp,
        'userId': parts[1],
      };
    } else if (parts.length == 3) {
      // Format: timestamp_sequence_userId
      final sequence = int.tryParse(parts[1]);
      if (sequence == null) return null;
      return {
        'timestamp': timestamp,
        'sequence': sequence,
        'userId': parts.sublist(2).join('_'), // userId might contain underscores
      };
    }
    
    return null;
  }
  
  // Static default instance for backward compatibility
  static final MessageIdGenerator _default = MessageIdGenerator();
  
  /// Get the default instance (for backward compatibility)
  static MessageIdGenerator get instance => _default;
}
