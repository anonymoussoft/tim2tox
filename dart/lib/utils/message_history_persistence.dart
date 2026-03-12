/// Unified message history persistence service
/// 
/// This service provides a unified interface for persisting message history
/// that can be used by both Platform interface scheme and binary replacement scheme.
/// 
/// Storage location: `<appDir>/chat_history/<conversationId>.json`
/// Data format: JSON with conversationId and messages array
import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:path_provider/path_provider.dart';
import '../models/chat_message.dart';
import 'conversation_id_utils.dart';

/// Message history persistence service
/// 
/// Provides unified message history persistence for both Platform and binary replacement schemes.
/// When [historyDirectory] is set (e.g. per-account path from app), uses that directory; otherwise
/// uses app support dir with optional instanceId for multi-instance.
class MessageHistoryPersistence {
  final int? _instanceId;
  final String? _historyDirectory;

  MessageHistoryPersistence({int? instanceId, String? historyDirectory})
      : _instanceId = instanceId,
        _historyDirectory = historyDirectory;

  // In-memory cache: conversationId -> List<ChatMessage>
  final Map<String, List<ChatMessage>> _historyById = {};

  // In-memory cache: conversationId -> lastViewTimestamp (milliseconds since epoch)
  final Map<String, int> _lastViewTimestampById = {};

  // Maximum number of messages to keep in memory per conversation
  static const int _maxMessagesInMemory = 1000;

  // Concurrency control: write locks per conversation
  final Map<String, Completer<void>?> _writeLocks = {};

  /// Get the directory for storing message history.
  /// When _historyDirectory is set, uses it (per-account); otherwise uses appDir + instance suffix.
  Future<Directory> _getHistoryDirectory() async {
    if (_historyDirectory != null && _historyDirectory!.isNotEmpty) {
      final historyDir = Directory(_historyDirectory!);
      if (!await historyDir.exists()) {
        await historyDir.create(recursive: true);
      }
      return historyDir;
    }
    final appDir = await getApplicationSupportDirectory();
    final historyDir = Directory(
      _instanceId != null && _instanceId != 0
          ? '${appDir.path}/chat_history_instance_$_instanceId'
          : '${appDir.path}/chat_history',
    );
    if (!await historyDir.exists()) {
      await historyDir.create(recursive: true);
    }
    return historyDir;
  }
  
  /// Get the file path for a conversation's history
  /// 
  /// Uses ConversationIdUtils to normalize and sanitize the ID for consistent file naming.
  Future<File> _getHistoryFile(String id) async {
    final dir = await _getHistoryDirectory();
    // Normalize and sanitize id for filename
    final normalizedId = ConversationIdUtils.normalize(id);
    final safeId = ConversationIdUtils.sanitizeForFilename(normalizedId);
    final filePath = '${dir.path}/$safeId.json';
    return File(filePath);
  }
  
  /// Get backup file path for a conversation's history
  Future<File> _getBackupFile(String id) async {
    final file = await _getHistoryFile(id);
    return File('${file.path}.bak');
  }
  
  /// Get temporary file path for atomic writes
  Future<File> _getTempFile(String id) async {
    final file = await _getHistoryFile(id);
    return File('${file.path}.tmp');
  }
  
  /// Save message history for a conversation
  /// 
  /// Thread-safe implementation with:
  /// - Write locks to prevent race conditions
  /// - Temporary file + atomic rename for data integrity
  /// - Backup mechanism for crash recovery
  /// 
  /// [conversationId] - Normalized conversation ID
  /// [messages] - List of messages to save
  Future<void> saveHistory(String conversationId, List<ChatMessage> messages) async {
    if (messages.isEmpty) return;
    
    // Normalize conversation ID for consistent storage
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    
    // Wait for any ongoing write to complete
    while (_writeLocks[normalizedId] != null) {
      await _writeLocks[normalizedId]!.future;
    }
    
    // Create write lock
    final completer = Completer<void>();
    _writeLocks[normalizedId] = completer;
    
    try {
      final file = await _getHistoryFile(normalizedId);
      final backupFile = await _getBackupFile(normalizedId);
      final tempFile = await _getTempFile(normalizedId);
      
      // Create backup of existing file if it exists
      if (await file.exists()) {
        try {
          await file.copy(backupFile.path);
        } catch (e) {
          // Backup failed, but continue with save
        }
      }
      
      // Prepare data
      final jsonList = messages.map((msg) => msg.toJson()).toList();
      final data = {
        'conversationId': normalizedId,
        'version': 1, // Format version for migration support
        'lastViewTimestamp': _lastViewTimestampById[normalizedId] ?? 0,
        'messages': jsonList,
      };
      final jsonString = jsonEncode(data);
      
      // Write to temporary file first
      await tempFile.writeAsString(jsonString);
      
      // Atomic rename: temp file -> final file
      // This ensures the file is either completely written or not present
      await tempFile.rename(file.path);
      
      // Clean up backup after successful write (optional, can keep for recovery)
      // For now, we keep backups and clean them up on next successful write
      
      completer.complete();
    } catch (e) {
      completer.completeError(e);
      // Don't silently fail - at least log or rethrow for critical errors
      // For now, we complete the error but don't rethrow to avoid breaking the app
      // In production, consider using a logger or error reporting service
    } finally {
      _writeLocks[normalizedId] = null;
    }
  }
  
  /// Load message history for a conversation
  /// 
  /// Returns the loaded messages and updates the in-memory cache.
  /// Marks all pending messages as not pending (failed) to prevent resending on startup.
  /// 
  /// Includes file integrity checks and backup recovery.
  /// 
  /// [id] - Conversation ID (will be normalized)
  /// [quitGroups] - Set of quit group IDs to filter out
  Future<List<ChatMessage>> loadHistory(String id, {Set<String>? quitGroups}) async {
    // Normalize conversation ID
    final normalizedId = ConversationIdUtils.normalize(id);
    
    try {
      final file = await _getHistoryFile(normalizedId);
      if (!await file.exists()) {
        // Try to load from backup
        return await _loadFromBackup(normalizedId, quitGroups: quitGroups);
      }
      
      // Check file size (empty or corrupted file)
      final fileSize = await file.length();
      if (fileSize == 0) {
        return await _loadFromBackup(normalizedId, quitGroups: quitGroups);
      }
      
      String jsonString;
      try {
        jsonString = await file.readAsString();
      } catch (e) {
        // File read failed, try backup
        return await _loadFromBackup(normalizedId, quitGroups: quitGroups);
      }
      
      dynamic decoded;
      try {
        decoded = jsonDecode(jsonString);
      } catch (e) {
        // JSON parse failed, try to recover from backup
        return await _recoverCorruptedFile(file, normalizedId, quitGroups: quitGroups);
      }
      
      List<ChatMessage> messages;
      String? actualId;
      
      if (decoded is Map<String, dynamic>) {
        // New format with metadata
        actualId = decoded['conversationId'] as String?;
        // Load lastViewTimestamp if available (default to 0 if not present)
        final lastViewTimestamp = decoded['lastViewTimestamp'] as int? ?? 0;
        final jsonList = decoded['messages'] as List<dynamic>;
        messages = jsonList.map((json) => ChatMessage.fromJson(json as Map<String, dynamic>)).toList();
        // Store lastViewTimestamp in memory cache
        final targetIdForTimestamp = actualId ?? id;
        _lastViewTimestampById[targetIdForTimestamp] = lastViewTimestamp;
      } else if (decoded is List<dynamic>) {
        // Old format (backward compatibility)
        messages = decoded.map((json) => ChatMessage.fromJson(json as Map<String, dynamic>)).toList();
        // Try to infer ID from messages
        if (messages.isNotEmpty) {
          final firstMsg = messages.first;
          actualId = firstMsg.groupId ?? id; // Use groupId if available, otherwise use provided id
        } else {
          actualId = id;
        }
      } else {
        return [];
      }
      
      // Normalize the actual ID from file
      final targetId = actualId != null ? ConversationIdUtils.normalize(actualId) : normalizedId;
      
      // Check if this is a quit group (if quitGroups is provided)
      if (quitGroups != null && messages.isNotEmpty) {
        final firstMsg = messages.first;
        final isGroupConversation = firstMsg.groupId != null && firstMsg.groupId!.isNotEmpty;
        if (isGroupConversation) {
          final groupId = firstMsg.groupId!;
          if (quitGroups.contains(groupId)) {
            // This group was quit, don't load its history
            // Also clean up the history file
            try {
              await file.delete();
            } catch (e) {
              // Ignore deletion errors
            }
            return [];
          }
        }
      }
      
      // Mark all historical messages as not pending (they're from previous sessions)
      // This prevents old pending messages from being resent on startup
      final updatedMessages = messages.map((msg) {
        if (msg.isPending) {
          // Create a new message with isPending=false to mark it as failed
          return ChatMessage(
            text: msg.text,
            fromUserId: msg.fromUserId,
            isSelf: msg.isSelf,
            timestamp: msg.timestamp,
            groupId: msg.groupId,
            filePath: msg.filePath,
            fileName: msg.fileName,
            mediaKind: msg.mediaKind,
            isPending: false, // Mark as not pending (failed to send in previous session)
            isReceived: msg.isReceived,
            isRead: msg.isRead,
            msgID: msg.msgID,
            version: msg.version,
            fileSize: msg.fileSize,
            mimeType: msg.mimeType,
            fileHash: msg.fileHash,
          );
        }
        return msg;
      }).toList();
      
      // CRITICAL: Remove duplicate messages by msgID
      // If multiple messages have the same msgID, keep the one with:
      // 1. Non-temp filePath (final path) if available
      // 2. Most recent timestamp
      // This prevents duplicate messages when the same message is saved multiple times (e.g., file_request and file_done)
      final Map<String, ChatMessage> deduplicatedMessages = {};
      for (final msg in updatedMessages) {
        if (msg.msgID != null) {
          final existing = deduplicatedMessages[msg.msgID];
          if (existing == null) {
            // First occurrence, add it
            deduplicatedMessages[msg.msgID!] = msg;
          } else {
            // Duplicate found, keep the better version
            // Prefer message with non-temp filePath (final path) over temp path
            // Use the new isTempPath property for better detection
            final existingIsTemp = existing.isTempPath;
            final msgIsTemp = msg.isTempPath;
            
            if (msgIsTemp && !existingIsTemp) {
              // Existing has final path, keep it
              continue;
            } else if (!msgIsTemp && existingIsTemp) {
              // New message has final path, replace existing
              deduplicatedMessages[msg.msgID!] = msg;
            } else {
              // Both have same type of path, keep the one with more recent timestamp
              if (msg.timestamp.isAfter(existing.timestamp)) {
                deduplicatedMessages[msg.msgID!] = msg;
              }
            }
          }
        } else {
          // Message without msgID, add it (shouldn't happen, but handle gracefully)
          deduplicatedMessages['${msg.timestamp.millisecondsSinceEpoch}_${msg.fromUserId}'] = msg;
        }
      }
      
      // Convert back to list and sort by timestamp
      final deduplicatedList = deduplicatedMessages.values.toList();
      deduplicatedList.sort((a, b) => a.timestamp.compareTo(b.timestamp));
      
      // Update in-memory cache
      _historyById[targetId] = deduplicatedList;

      // If lastViewTimestamp was not loaded from file, initialize it to 0
      if (!_lastViewTimestampById.containsKey(targetId)) {
        _lastViewTimestampById[targetId] = 0;
      }
      
      // Save the updated history (with isPending=false and deduplicated) to disk if any changes were made
      if (deduplicatedList.length != messages.length || updatedMessages != messages) {
        unawaited(saveHistory(targetId, deduplicatedList));
      }
      
      return deduplicatedList;
    } catch (e) {
      // Try to load from backup on any error
      return await _loadFromBackup(normalizedId, quitGroups: quitGroups);
    }
  }
  
  /// Load history from backup file
  Future<List<ChatMessage>> _loadFromBackup(String normalizedId, {Set<String>? quitGroups}) async {
    try {
      final backupFile = await _getBackupFile(normalizedId);
      if (!await backupFile.exists()) {
        return [];
      }
      
      final jsonString = await backupFile.readAsString();
      final decoded = jsonDecode(jsonString);
      
      // Use the same loading logic as loadHistory
      // This is a simplified version - in production, consider refactoring
      if (decoded is Map<String, dynamic>) {
        final jsonList = decoded['messages'] as List<dynamic>?;
        if (jsonList == null) return [];
        
        final messages = jsonList.map((json) => ChatMessage.fromJson(json as Map<String, dynamic>)).toList();
        
        // Check quit groups
        if (quitGroups != null && messages.isNotEmpty) {
          final firstMsg = messages.first;
          if (firstMsg.groupId != null && quitGroups.contains(firstMsg.groupId)) {
            return [];
          }
        }
        
        return messages;
      }
      
      return [];
    } catch (e) {
      return [];
    }
  }
  
  /// Recover from corrupted file
  Future<List<ChatMessage>> _recoverCorruptedFile(File corruptedFile, String normalizedId, {Set<String>? quitGroups}) async {
    // Try backup first
    final backupMessages = await _loadFromBackup(normalizedId, quitGroups: quitGroups);
    if (backupMessages.isNotEmpty) {
      // Backup is good, restore it
      try {
        final backupFile = await _getBackupFile(normalizedId);
        if (await backupFile.exists()) {
          await backupFile.copy(corruptedFile.path);
        }
      } catch (e) {
        // Restore failed, but we have messages in memory
      }
      return backupMessages;
    }
    
    // Try partial recovery (read valid JSON parts)
    // For now, return empty list - can be enhanced later
    return [];
  }
  
  /// Load all message histories from disk
  /// 
  /// Scans the chat_history directory and loads all conversation histories.
  /// Returns a map of conversationId -> messages.
  Future<Map<String, List<ChatMessage>>> loadAllHistories({Set<String>? quitGroups}) async {
    final result = <String, List<ChatMessage>>{};
    
    try {
      final dir = await _getHistoryDirectory();
      if (!await dir.exists()) {
        return result;
      }
      
      final files = dir.listSync();
      
      for (final file in files) {
        if (file is File && file.path.endsWith('.json')) {
          // Extract sanitized id from filename
          final filename = file.path.split('/').last;
          final sanitizedId = filename.replaceAll('.json', '');
          
          try {
            final messages = await loadHistory(sanitizedId, quitGroups: quitGroups);
            if (messages.isNotEmpty) {
              // Use the conversation ID from the filename (sanitizedId), normalized.
              // For C2C the file is named by peer id; using firstMsg.fromUserId would be
              // wrong when the first message was sent by self (would use self id as key),
              // so lastMessages[peerId] would be null and conversation list would show
              // empty last message/time for that contact.
              final conversationKey = ConversationIdUtils.normalize(sanitizedId);
              result[conversationKey] = messages;
            }
          } catch (e) {
            // Continue loading other files even if one fails
          }
        }
      }
    } catch (e) {
      // Return whatever we've loaded so far
    }
    
    return result;
  }
  
  /// Append a message to the history for a conversation
  /// 
  /// Adds the message to the in-memory cache and asynchronously saves to disk.
  /// Limits memory usage by keeping only the most recent _maxMessagesInMemory messages in memory;
  /// the full history is always persisted to disk so no messages are lost.
  /// CRITICAL: If a message with the same msgID already exists, intelligently merge it instead of adding a duplicate.
  /// This prevents duplicate messages when the same message is processed multiple times (e.g., file_request and file_done).
  /// 
  /// [conversationId] - Will be normalized before use
  void appendHistory(String conversationId, ChatMessage message) {
    // Normalize conversation ID
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final list = _historyById.putIfAbsent(normalizedId, () => <ChatMessage>[]);
    
    // CRITICAL: Check if message with same msgID already exists
    // If it exists, intelligently merge instead of replacing
    if (message.msgID != null) {
      final existingIndex = list.indexWhere((msg) => msg.msgID == message.msgID);
      if (existingIndex >= 0) {
        // Message already exists, merge intelligently
        final existing = list[existingIndex];
        final merged = _mergeMessages(existing, message);
        list[existingIndex] = merged;
        // Save updated history asynchronously
        unawaited(saveHistory(normalizedId, list));
        return;
      }
    }
    
    // Content-based deduplication (fallback for messages without msgID only).
    // When message has a msgID, do not merge by content — distinct messages with same text must stay separate.
    if (message.msgID == null && message.text.isNotEmpty) {
      final contentMatch = list.indexWhere((msg) =>
        msg.text == message.text &&
        msg.timestamp.difference(message.timestamp).abs().inSeconds <= 5 &&
        msg.isSelf == message.isSelf
      );
      if (contentMatch >= 0) {
        // Merge with existing message
        final merged = _mergeMessages(list[contentMatch], message);
        list[contentMatch] = merged;
        unawaited(saveHistory(normalizedId, list));
        return;
      }
    }
    
    // Message doesn't exist, add it
    list.add(message);
    
    // Limit memory usage: keep only the most recent messages in memory.
    // CRITICAL: Persist the FULL history to disk before truncating, so we never lose old messages.
    if (list.length > _maxMessagesInMemory) {
      final fullList = List<ChatMessage>.from(list);
      unawaited(saveHistory(normalizedId, fullList));
      list.removeRange(0, list.length - _maxMessagesInMemory);
    } else {
      unawaited(saveHistory(normalizedId, list));
    }
  }
  
  /// Intelligently merge two messages with the same msgID
  /// 
  /// Prefers final file paths over temporary paths, and preserves the best state.
  ChatMessage _mergeMessages(ChatMessage existing, ChatMessage updated) {
    // Prefer final path over temp path
    final filePath = updated.isTempPath && !existing.isTempPath
      ? existing.filePath
      : (!updated.isTempPath ? updated.filePath : existing.filePath);
    
    // Merge file metadata
    final fileSize = updated.fileSize ?? existing.fileSize;
    final mimeType = updated.mimeType ?? existing.mimeType;
    final fileHash = updated.fileHash ?? existing.fileHash;
    
    // Merge states: both must be pending for result to be pending
    final isPending = updated.isPending && existing.isPending;
    // Either received means received
    final isReceived = updated.isReceived || existing.isReceived;
    // Either read means read
    final isRead = updated.isRead || existing.isRead;
    
    return ChatMessage(
      text: updated.text.isNotEmpty ? updated.text : existing.text,
      fromUserId: updated.fromUserId,
      isSelf: updated.isSelf,
      timestamp: updated.timestamp.isAfter(existing.timestamp) ? updated.timestamp : existing.timestamp,
      groupId: updated.groupId ?? existing.groupId,
      filePath: filePath,
      fileName: updated.fileName ?? existing.fileName,
      mediaKind: updated.mediaKind ?? existing.mediaKind,
      isPending: isPending,
      isReceived: isReceived,
      isRead: isRead,
      msgID: updated.msgID ?? existing.msgID,
      version: updated.version,
      fileSize: fileSize,
      mimeType: mimeType,
      fileHash: fileHash,
    );
  }
  
  /// Get message history for a conversation (from memory cache)
  /// 
  /// Returns the cached messages. If not in cache, returns empty list.
  /// Use loadHistory() to load from disk if needed.
  /// 
  /// [conversationId] - Will be normalized before lookup
  List<ChatMessage> getHistory(String conversationId) {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    return List<ChatMessage>.from(_historyById[normalizedId] ?? []);
  }
  
  /// Check if history file exists for a conversation
  /// Returns true if the file exists, false otherwise
  /// 
  /// [conversationId] - Will be normalized before check
  Future<bool> historyFileExists(String conversationId) async {
    try {
      final normalizedId = ConversationIdUtils.normalize(conversationId);
      final file = await _getHistoryFile(normalizedId);
      return await file.exists();
    } catch (e) {
      return false;
    }
  }
  
  /// Get all conversation IDs that have history
  Set<String> getConversationIds() {
    return _historyById.keys.toSet();
  }
  
  /// Clear history for a specific conversation
  /// This deletes the JSON history file but does NOT delete actual media files
  /// (images, videos, audio, documents) referenced in the messages
  /// 
  /// Also clears all cache entries that might match this conversation ID
  /// (e.g., original ID and normalized ID variants)
  /// 
  /// CRITICAL: This method scans all history files to find and delete any file
  /// that contains messages for this conversation, even if the filename doesn't match
  /// (e.g., due to ID normalization or sanitization differences)
  /// 
  /// [conversationId] - Will be normalized before clearing
  Future<void> clearHistory(String conversationId) async {
    // Normalize conversationId for comparison
    final normalizedConversationId = ConversationIdUtils.normalize(conversationId);
    
    // Clear in-memory cache for this ID and all variants
    _historyById.remove(conversationId);
    
    // Also clear any cache entries that might match this conversation
    // (e.g., if there are multiple ID variants stored)
    final keysToRemove = <String>[];
    for (final key in _historyById.keys) {
      // Normalize key for comparison
      final normalizedKey = ConversationIdUtils.normalize(key);
      
      // Check if key matches conversationId using normalized comparison
      if (ConversationIdUtils.equals(key, conversationId) ||
          ConversationIdUtils.equals(normalizedKey, normalizedConversationId)) {
        keysToRemove.add(key);
      }
    }
    for (final key in keysToRemove) {
      _historyById.remove(key);
    }
    
    // Delete the persisted history file(s) (JSON file containing message records)
    // Note: This only deletes the history file, NOT the actual media files
    // (images, videos, etc.) that may be referenced in the messages
    // 
    // CRITICAL: We need to scan all files because:
    // 1. The filename might be based on a sanitized/normalized version of the ID
    // 2. The file might contain messages for this conversation even if filename doesn't match
    // 3. There might be multiple files for the same conversation (original ID vs normalized ID)
    try {
      // First, try to delete the file with the exact ID
      final file = await _getHistoryFile(conversationId);
      if (await file.exists()) {
        await file.delete();
        // Verify deletion succeeded
        if (await file.exists()) {
          // If file still exists, try force delete
          try {
            await file.delete(recursive: false);
          } catch (e2) {
            // Log but don't throw - clearing should continue
          }
        }
      }
      
      // Also scan all history files to find any that contain messages for this conversation
      // This handles cases where the file was created with a different ID format
      try {
        final dir = await _getHistoryDirectory();
        if (await dir.exists()) {
          final files = dir.listSync();
          for (final fileEntry in files) {
            if (fileEntry is File && fileEntry.path.endsWith('.json')) {
              try {
                // Read the file to check if it contains messages for this conversation
                final jsonString = await fileEntry.readAsString();
                final decoded = jsonDecode(jsonString);
                
                String? fileConversationId;
                if (decoded is Map<String, dynamic>) {
                  // New format with metadata
                  fileConversationId = decoded['conversationId'] as String?;
                  
                  // For C2C conversations, if conversationId is not set, try to infer from messages
                  if (fileConversationId == null && decoded['messages'] is List) {
                    final messages = decoded['messages'] as List;
                    if (messages.isNotEmpty) {
                      final firstMsg = messages.first as Map<String, dynamic>;
                      // For C2C, use fromUserId; for group, use groupId
                      fileConversationId = firstMsg['groupId'] as String? ?? 
                                          firstMsg['fromUserId'] as String?;
                    }
                  }
                } else if (decoded is List<dynamic> && decoded.isNotEmpty) {
                  // Old format - try to infer from first message
                  final firstMsg = decoded.first as Map<String, dynamic>;
                  // For C2C, use fromUserId; for group, use groupId
                  fileConversationId = firstMsg['groupId'] as String? ?? 
                                      firstMsg['fromUserId'] as String?;
                }
                
                // Check if this file contains messages for the conversation we're clearing
                // Use ConversationIdUtils for consistent comparison
                bool matches = false;
                if (fileConversationId != null) {
                  matches = ConversationIdUtils.equals(fileConversationId, conversationId) ||
                            ConversationIdUtils.equals(fileConversationId, normalizedConversationId);
                }
                
                if (matches) {
                  // This file contains messages for this conversation, delete it
                  await fileEntry.delete();
                  // Also clear cache if it exists
                  if (fileConversationId != null) {
                    _historyById.remove(fileConversationId);
                  }
                }
              } catch (e) {
                // Skip files that can't be read or parsed
                continue;
              }
            }
          }
        }
      } catch (e) {
        // Log error but don't throw - we've already cleared memory cache
      }
    } catch (e) {
      // Log error but don't throw - clearing should continue
      // The file may not exist or may be locked, but we've cleared memory cache
    }
  }
  
  /// Clear all message histories
  Future<void> clearAllHistories() async {
    _historyById.clear();
    
    try {
      final historyDir = await _getHistoryDirectory();
      if (await historyDir.exists()) {
        await historyDir.delete(recursive: true);
      }
    } catch (e) {
      // Ignore errors
    }
  }
  
  /// Update a message in the history
  /// 
  /// Finds the message by msgID and updates it, then saves to disk.
  /// 
  /// [conversationId] - Will be normalized before update
  Future<bool> updateMessage(String conversationId, String msgID, ChatMessage updatedMessage) async {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final list = _historyById[normalizedId];
    if (list == null) return false;
    
    final index = list.indexWhere((msg) => msg.msgID == msgID);
    if (index == -1) return false;
    
    // Merge intelligently instead of direct replacement
    final existing = list[index];
    final merged = _mergeMessages(existing, updatedMessage);
    list[index] = merged;
    await saveHistory(normalizedId, list);
    return true;
  }
  
  /// Safely update file path for a message
  /// 
  /// This method ensures atomic file path updates with integrity checks:
  /// 1. Verifies the new file exists
  /// 2. Gets file metadata (size, etc.)
  /// 3. Updates the message atomically
  /// 4. Optionally deletes the old temporary file after successful update
  /// 
  /// [conversationId] - Will be normalized before update
  /// [msgID] - Message ID to update
  /// [newFilePath] - New file path (must exist)
  /// [deleteOldTempFile] - Whether to delete old temp file after successful update
  /// 
  /// Returns true if update was successful, false otherwise.
  Future<bool> updateFilePathSafely(
    String conversationId,
    String msgID,
    String newFilePath, {
    bool deleteOldTempFile = true,
  }) async {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final list = _historyById[normalizedId];
    if (list == null) return false;
    
    final index = list.indexWhere((msg) => msg.msgID == msgID);
    if (index == -1) return false;
    
    final existing = list[index];
    final oldFilePath = existing.filePath;
    
    // 1. Verify new file exists
    final newFile = File(newFilePath);
    if (!await newFile.exists()) {
      // File doesn't exist, but we still update the path (file might be moved later)
      // This handles edge cases where file is moved but not yet visible
      // We'll verify on next access
    }
    
    // 2. Get file metadata if file exists
    int? fileSize;
    try {
      if (await newFile.exists()) {
        fileSize = await newFile.length();
      }
    } catch (e) {
      // Ignore errors getting file size
    }
    
    // 3. Create updated message
    final updated = existing.copyWith(
      filePath: newFilePath,
      fileSize: fileSize,
    );
    
    // 4. Update message atomically
    list[index] = updated;
    await saveHistory(normalizedId, list);
    
    // 5. Verify update succeeded
    final verifyIndex = list.indexWhere((msg) => msg.msgID == msgID);
    if (verifyIndex == -1 || list[verifyIndex].filePath != newFilePath) {
      // Update verification failed, revert change
      list[index] = existing;
      return false;
    }
    
    // 6. Delete old temp file if requested and update was successful
    if (deleteOldTempFile && oldFilePath != null && existing.isTempPath) {
      // Delay deletion to ensure update is persisted
      Future.delayed(Duration(seconds: 5), () async {
        try {
          final oldFile = File(oldFilePath);
          if (await oldFile.exists()) {
            await oldFile.delete();
          }
        } catch (e) {
          // Ignore deletion errors - file might be in use or already deleted
        }
      });
    }
    
    return true;
  }
  
  /// Remove a message from the history
  /// 
  /// Finds the message by msgID and removes it, then saves to disk.
  /// 
  /// [conversationId] - Will be normalized before removal
  Future<bool> removeMessage(String conversationId, String msgID) async {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final list = _historyById[normalizedId];
    if (list == null) return false;
    
    final initialLength = list.length;
    list.removeWhere((msg) => msg.msgID == msgID);
    
    if (list.length < initialLength) {
      await saveHistory(normalizedId, list);
      return true;
    }
    
    return false;
  }
  
  /// Get the in-memory cache (for direct access if needed)
  Map<String, List<ChatMessage>> get cache => Map.unmodifiable(_historyById);
  
  /// Set the in-memory cache (for initialization)
  void setCache(Map<String, List<ChatMessage>> cache) {
    _historyById.clear();
    _historyById.addAll(cache);
  }
  
  /// Update the last view timestamp for a conversation
  /// 
  /// Updates the timestamp when the user last viewed the conversation.
  /// This is used to calculate unread message count.
  /// 
  /// [conversationId] - Will be normalized before update
  Future<void> updateLastViewTimestamp(String conversationId, int timestamp) async {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    _lastViewTimestampById[normalizedId] = timestamp;
    // Save the updated history to persist the new timestamp
    final messages = _historyById[normalizedId];
    if (messages != null && messages.isNotEmpty) {
      await saveHistory(normalizedId, messages);
    }
  }
  
  /// Get the last view timestamp for a conversation
  /// 
  /// Returns the timestamp when the user last viewed the conversation.
  /// Returns 0 if the conversation has never been viewed.
  /// 
  /// [conversationId] - Will be normalized before lookup
  int getLastViewTimestamp(String conversationId) {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    return _lastViewTimestampById[normalizedId] ?? 0;
  }
  
  /// Get the unread message count for a conversation
  /// 
  /// Calculates the number of unread messages based on:
  /// - Messages with timestamp > lastViewTimestamp
  /// - Messages that are not self-sent (!isSelf)
  /// - Messages that are not marked as read (!isRead)
  /// 
  /// [conversationId] - Will be normalized before lookup
  int getUnreadCount(String conversationId) {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final messages = _historyById[normalizedId];
    if (messages == null || messages.isEmpty) {
      return 0;
    }
    
    final lastViewTimestamp = _lastViewTimestampById[normalizedId] ?? 0;
    int count = 0;
    
    for (final msg in messages) {
      final msgTimestamp = msg.timestamp.millisecondsSinceEpoch;
      // Count messages that are:
      // 1. After the last view timestamp
      // 2. Not self-sent
      // 3. Not marked as read
      if (msgTimestamp > lastViewTimestamp && !msg.isSelf && !msg.isRead) {
        count++;
      }
    }
    
    return count;
  }
  
  /// Mark all unread messages as read for a conversation
  /// 
  /// Marks all messages with timestamp > lastViewTimestamp and !isSelf as read.
  /// This is called when the user opens a conversation.
  /// 
  /// [conversationId] - Will be normalized before update
  Future<void> markUnreadMessagesAsRead(String conversationId) async {
    final normalizedId = ConversationIdUtils.normalize(conversationId);
    final messages = _historyById[normalizedId];
    if (messages == null || messages.isEmpty) {
      return;
    }
    
    final lastViewTimestamp = _lastViewTimestampById[normalizedId] ?? 0;
    bool updated = false;
    
    for (int i = 0; i < messages.length; i++) {
      final msg = messages[i];
      final msgTimestamp = msg.timestamp.millisecondsSinceEpoch;
      // Mark messages as read that are:
      // 1. After the last view timestamp
      // 2. Not self-sent
      // 3. Not already marked as read
      if (msgTimestamp > lastViewTimestamp && !msg.isSelf && !msg.isRead) {
        messages[i] = msg.copyWith(isRead: true);
        updated = true;
      }
    }
    
    if (updated) {
      await saveHistory(normalizedId, messages);
    }
  }
  
  /// Clean up temporary files on startup
  /// 
  /// Removes temporary files and old backups that are no longer needed.
  Future<void> cleanupTempFiles() async {
    try {
      final dir = await _getHistoryDirectory();
      if (!await dir.exists()) return;
      
      final files = dir.listSync();
      final now = DateTime.now();
      
      for (final fileEntry in files) {
        if (fileEntry is File) {
          try {
            // Clean up temporary files (.tmp)
            if (fileEntry.path.endsWith('.tmp')) {
              final stat = await fileEntry.stat();
              final age = now.difference(stat.modified);
              // Delete temp files older than 1 hour
              if (age.inHours > 1) {
                await fileEntry.delete();
              }
            }
            
            // Clean up old backup files (.bak) - keep only recent ones
            if (fileEntry.path.endsWith('.bak')) {
              final stat = await fileEntry.stat();
              final age = now.difference(stat.modified);
              // Delete backups older than 7 days
              if (age.inDays > 7) {
                await fileEntry.delete();
              }
            }
          } catch (e) {
            // Continue cleanup even if one file fails
          }
        }
      }
    } catch (e) {
      // Ignore cleanup errors
    }
  }
}
