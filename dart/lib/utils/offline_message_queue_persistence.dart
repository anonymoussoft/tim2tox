/// Unified offline message queue persistence service
/// 
/// This service provides a unified interface for persisting offline message queue
/// that can be used by both Platform interface scheme and binary replacement scheme.
/// 
/// Storage location: `<appDir>/offline_message_queue.json`
/// Data format: JSON map of peerId -> list of pending messages
import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:path_provider/path_provider.dart';

/// Offline message queue item
typedef OfflineMessageItem = ({
  String text,
  String? filePath,
  DateTime timestamp,
});

/// Offline message queue persistence service
/// 
/// Provides unified offline message queue persistence for both Platform and binary replacement schemes.
/// When [queueFilePath] is set (e.g. per-account path from app), uses that file; otherwise uses
/// default under app support directory.
class OfflineMessageQueuePersistence {
  final String? _queueFilePath;

  OfflineMessageQueuePersistence({String? queueFilePath}) : _queueFilePath = queueFilePath;

  // In-memory cache: peerId -> List<OfflineMessageItem>
  final Map<String, List<OfflineMessageItem>> _offlineQueue = {};

  /// Get the file path for offline message queue.
  Future<File> _getQueueFile() async {
    if (_queueFilePath != null && _queueFilePath!.isNotEmpty) {
      final file = File(_queueFilePath!);
      final parent = file.parent;
      if (!await parent.exists()) {
        await parent.create(recursive: true);
      }
      return file;
    }
    final appDir = await getApplicationSupportDirectory();
    return File('${appDir.path}/offline_message_queue.json');
  }
  
  /// Save offline message queue to disk
  Future<void> saveQueue(Map<String, List<OfflineMessageItem>> queue) async {
    try {
      final file = await _getQueueFile();
      final jsonMap = <String, dynamic>{};
      
      for (final entry in queue.entries) {
        jsonMap[entry.key] = entry.value.map((item) => {
          'text': item.text,
          'filePath': item.filePath,
          'timestamp': item.timestamp.toIso8601String(),
        }).toList();
      }
      
      await file.writeAsString(jsonEncode(jsonMap));
      // Update in-memory cache
      _offlineQueue.clear();
      _offlineQueue.addAll(queue);
    } catch (e) {
      // Silently handle errors
    }
  }
  
  /// Load offline message queue from disk
  /// 
  /// Note: Current design clears the queue file on load to prevent resending old messages
  /// from previous sessions. Historical pending messages are already marked as failed
  /// in message history loading.
  Future<Map<String, List<OfflineMessageItem>>> loadQueue({bool clearOnLoad = true}) async {
    try {
      final file = await _getQueueFile();
      if (!await file.exists()) {
        return {};
      }
      
      if (clearOnLoad) {
        // Clear the offline queue file - we don't want to resend old messages from previous sessions
        // Historical pending messages have already been marked as failed in message history loading
        // Only messages added during the current session should be in the queue
        try {
          await file.delete();
        } catch (e) {
          // Ignore deletion errors
        }
        _offlineQueue.clear();
        return {};
      }
      
      // Load from file (if clearOnLoad is false)
      final jsonString = await file.readAsString();
      final decoded = jsonDecode(jsonString) as Map<String, dynamic>;
      
      final queue = <String, List<OfflineMessageItem>>{};
      
      for (final entry in decoded.entries) {
        final peerId = entry.key;
        final items = (entry.value as List<dynamic>).map((item) {
          final map = item as Map<String, dynamic>;
          return (
            text: map['text'] as String,
            filePath: map['filePath'] as String?,
            timestamp: DateTime.parse(map['timestamp'] as String),
          );
        }).toList();
        queue[peerId] = items;
      }
      
      _offlineQueue.clear();
      _offlineQueue.addAll(queue);
      return queue;
    } catch (e) {
      return {};
    }
  }
  
  /// Add a message to the offline queue for a peer
  void addMessage(String peerId, OfflineMessageItem item) {
    final list = _offlineQueue.putIfAbsent(peerId, () => <OfflineMessageItem>[]);
    list.add(item);
    // Save asynchronously
    unawaited(saveQueue(_offlineQueue));
  }
  
  /// Get messages for a peer from the queue
  List<OfflineMessageItem> getMessages(String peerId) {
    return List.from(_offlineQueue[peerId] ?? []);
  }
  
  /// Remove messages for a peer from the queue
  Future<void> removeMessages(String peerId) async {
    _offlineQueue.remove(peerId);
    await saveQueue(_offlineQueue);
  }
  
  /// Clear all messages from the queue
  Future<void> clearQueue() async {
    _offlineQueue.clear();
    await saveQueue(_offlineQueue);
  }
  
  /// Get all peer IDs that have messages in the queue
  Set<String> getPeerIds() {
    return _offlineQueue.keys.toSet();
  }
  
  /// Get the in-memory cache (for direct access if needed)
  Map<String, List<OfflineMessageItem>> get cache => Map.unmodifiable(_offlineQueue);
  
  /// Set the in-memory cache (for initialization)
  void setCache(Map<String, List<OfflineMessageItem>> cache) {
    _offlineQueue.clear();
    _offlineQueue.addAll(cache);
  }
  
  /// Clear the queue file from disk
  Future<void> clearQueueFile() async {
    try {
      final file = await _getQueueFile();
      if (await file.exists()) {
        await file.delete();
      }
    } catch (e) {
      // Ignore errors
    }
    _offlineQueue.clear();
  }
}
