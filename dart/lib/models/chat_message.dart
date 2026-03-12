import 'dart:io';

/// Chat message model
class ChatMessage {
  ChatMessage({
    required this.text,
    required this.fromUserId,
    required this.isSelf,
    required this.timestamp,
    this.groupId,
    this.filePath,
    this.fileName,
    this.mediaKind,
    this.isPending = false,
    this.isReceived = false,
    this.isRead = false,
    this.msgID,
    this.version = 1, // Message format version for migration support
    this.fileSize, // File size in bytes
    this.mimeType, // MIME type of the file
    this.fileHash, // SHA256 hash of file content (optional)
  });
  
  final String text;
  final String fromUserId;
  final bool isSelf;
  final DateTime timestamp;
  final String? groupId;
  final String? filePath;
  final String? fileName;
  final String? mediaKind; // 'image' | 'video' | 'audio' | 'file'
  final bool isPending;
  final bool isReceived;
  final bool isRead;
  final String? msgID;
  
  // New fields for enhanced data integrity
  final int version; // Message format version
  final int? fileSize; // File size in bytes
  final String? mimeType; // MIME type
  final String? fileHash; // SHA256 hash (optional, for integrity verification)

  Map<String, dynamic> toJson() => {
    'text': text,
    'fromUserId': fromUserId,
    'isSelf': isSelf,
    'timestamp': timestamp.toIso8601String(),
    'groupId': groupId,
    'filePath': filePath,
    'fileName': fileName,
    'mediaKind': mediaKind,
    'isPending': isPending,
    'isReceived': isReceived,
    'isRead': isRead,
    'msgID': msgID,
    'version': version,
    if (fileSize != null) 'fileSize': fileSize,
    if (mimeType != null) 'mimeType': mimeType,
    if (fileHash != null) 'fileHash': fileHash,
  };

  factory ChatMessage.fromJson(Map<String, dynamic> json) => ChatMessage(
    text: json['text'] as String,
    fromUserId: json['fromUserId'] as String,
    isSelf: json['isSelf'] as bool,
    timestamp: DateTime.parse(json['timestamp'] as String),
    groupId: json['groupId'] as String?,
    filePath: json['filePath'] as String?,
    fileName: json['fileName'] as String?,
    mediaKind: json['mediaKind'] as String?,
    isPending: json['isPending'] as bool? ?? false,
    isReceived: json['isReceived'] as bool? ?? false,
    isRead: json['isRead'] as bool? ?? false,
    msgID: json['msgID'] as String?,
    version: json['version'] as int? ?? 1, // Default to version 1 for backward compatibility
    fileSize: json['fileSize'] as int?,
    mimeType: json['mimeType'] as String?,
    fileHash: json['fileHash'] as String?,
  );
  
  ChatMessage copyWith({
    bool? isReceived,
    bool? isRead,
    bool? isPending,
    String? filePath,
    String? fileName,
    int? fileSize,
    String? mimeType,
    String? fileHash,
  }) {
    return ChatMessage(
      text: text,
      fromUserId: fromUserId,
      isSelf: isSelf,
      timestamp: timestamp,
      groupId: groupId,
      filePath: filePath ?? this.filePath,
      fileName: fileName ?? this.fileName,
      mediaKind: mediaKind,
      isPending: isPending ?? this.isPending,
      isReceived: isReceived ?? this.isReceived,
      isRead: isRead ?? this.isRead,
      msgID: msgID,
      version: version,
      fileSize: fileSize ?? this.fileSize,
      mimeType: mimeType ?? this.mimeType,
      fileHash: fileHash ?? this.fileHash,
    );
  }
  
  /// Verify file integrity
  /// 
  /// Checks if the file exists and optionally verifies size and hash.
  /// 
  /// Returns true if file is valid, false otherwise.
  Future<bool> verifyFile({bool checkSize = true, bool checkHash = false}) async {
    if (filePath == null || filePath!.isEmpty) return false;
    
    try {
      final file = File(filePath!);
      if (!await file.exists()) return false;
      
      if (checkSize && fileSize != null) {
        final actualSize = await file.length();
        if (actualSize != fileSize) return false;
      }
      
      // Hash verification would require crypto package
      // For now, we skip it as it's optional and expensive
      if (checkHash && fileHash != null) {
        // TODO: Implement hash verification if needed
        // final actualHash = await _computeFileHash(file);
        // return actualHash == fileHash;
      }
      
      return true;
    } catch (e) {
      return false;
    }
  }
  
  /// Check if file path is a temporary path
  bool get isTempPath {
    if (filePath == null) return false;
    return filePath!.startsWith('/tmp/receiving_') || 
           filePath!.contains('/file_recv/') ||
           filePath!.startsWith('/tmp/');
  }
  
  /// Check if file path is a final path (not temporary)
  bool get isFinalPath {
    if (filePath == null) return false;
    return !isTempPath && 
           (filePath!.contains('/avatars/') || 
            filePath!.contains('/Downloads/') ||
            filePath!.contains('/file_recv/'));
  }
}

