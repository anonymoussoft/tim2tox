/// Message converter utility
/// 
/// Provides bidirectional conversion between V2TimMessage and ChatMessage
/// for use in binary replacement scheme.
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_text_elem.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_image_elem.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_image.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_video_elem.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_sound_elem.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_file_elem.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_elem_type.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_status.dart';
import 'package:tencent_cloud_chat_sdk/enum/image_types.dart';
import 'dart:io';
import '../models/chat_message.dart';

/// Message converter utility class
class MessageConverter {
  /// Convert V2TimMessage to ChatMessage
  /// 
  /// Extracts relevant fields from V2TimMessage and creates a ChatMessage.
  /// Handles different message types (text, image, video, audio, file).
  static ChatMessage v2TimMessageToChatMessage(V2TimMessage v2Msg, String selfId) {
    // Determine media kind and extract content
    String text = '';
    String? filePath;
    String? fileName;
    String? mediaKind;
    
    // Extract text and file information based on element type
    switch (v2Msg.elemType) {
      case MessageElemType.V2TIM_ELEM_TYPE_TEXT:
        text = v2Msg.textElem?.text ?? '';
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_IMAGE:
        mediaKind = 'image';
        text = ''; // Images don't have text content
        filePath = v2Msg.imageElem?.path;
        // Try to get filename from path
        if (filePath != null && filePath.isNotEmpty) {
          fileName = filePath.split('/').last;
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_VIDEO:
        mediaKind = 'video';
        text = '';
        filePath = v2Msg.videoElem?.videoPath;
        if (filePath != null && filePath.isNotEmpty) {
          fileName = filePath.split('/').last;
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_SOUND:
        mediaKind = 'audio';
        text = '';
        filePath = v2Msg.soundElem?.path;
        if (filePath != null && filePath.isNotEmpty) {
          fileName = filePath.split('/').last;
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_FILE:
        mediaKind = 'file';
        text = '';
        filePath = v2Msg.fileElem?.path;
        fileName = v2Msg.fileElem?.fileName;
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_CUSTOM:
        // Custom messages might have text in custom data
        text = v2Msg.customElem?.data ?? '';
        break;
      default:
        // For other types, try to get text from textElem as fallback
        text = v2Msg.textElem?.text ?? '';
        break;
    }
    
    // Determine if message is from self
    final isSelf = v2Msg.isSelf ?? (v2Msg.sender == selfId);
    
    // Determine message status
    final isPending = v2Msg.status == MessageStatus.V2TIM_MSG_STATUS_SENDING;
    final isReceived = v2Msg.status == MessageStatus.V2TIM_MSG_STATUS_SEND_SUCC;
    final isRead = v2Msg.isRead ?? false;
    
    // Get timestamp (convert from seconds to milliseconds)
    final timestamp = v2Msg.timestamp != null
        ? DateTime.fromMillisecondsSinceEpoch(v2Msg.timestamp! * 1000)
        : DateTime.now();
    
    // Get sender ID
    final fromUserId = v2Msg.sender ?? v2Msg.userID ?? '';
    
    // Create ChatMessage
    return ChatMessage(
      text: text,
      fromUserId: fromUserId,
      isSelf: isSelf,
      timestamp: timestamp,
      groupId: v2Msg.groupID,
      filePath: filePath,
      fileName: fileName,
      mediaKind: mediaKind,
      isPending: isPending,
      isReceived: isReceived,
      isRead: isRead,
      msgID: v2Msg.msgID,
    );
  }
  
  /// Convert ChatMessage to V2TimMessage
  /// 
  /// Creates a V2TimMessage from ChatMessage.
  /// Handles different message types (text, image, video, audio, file).
  static V2TimMessage chatMessageToV2TimMessage(ChatMessage chatMsg, String selfId) {
    // Determine element type based on mediaKind
    int elemType = MessageElemType.V2TIM_ELEM_TYPE_TEXT;
    String? mediaKind = chatMsg.mediaKind;
    
    if (mediaKind == 'image') {
      elemType = MessageElemType.V2TIM_ELEM_TYPE_IMAGE;
    } else if (mediaKind == 'video') {
      elemType = MessageElemType.V2TIM_ELEM_TYPE_VIDEO;
    } else if (mediaKind == 'audio') {
      elemType = MessageElemType.V2TIM_ELEM_TYPE_SOUND;
    } else if (mediaKind == 'file') {
      elemType = MessageElemType.V2TIM_ELEM_TYPE_FILE;
    }
    
    // Create V2TimMessage
    final msg = V2TimMessage(elemType: elemType);
    final msgID = chatMsg.msgID ?? '${chatMsg.timestamp.millisecondsSinceEpoch}_${chatMsg.fromUserId}';
    msg.msgID = msgID;
    // CRITICAL: Also set id to msgID to ensure proper matching with UIKit messages
    // UIKit messages may have id (temporary ID) while FFI messages have msgID (actual ID)
    // Setting both ensures we can match messages correctly and prevent duplicates
    // This must be consistent with platform scheme (tim2tox_sdk_platform_converters.dart)
    msg.id = msgID;
    msg.timestamp = chatMsg.timestamp.millisecondsSinceEpoch ~/ 1000;
    // DEBUG: Log timestamp conversion for troubleshooting
    print('[MessageConverter] chatMsg.timestamp: ${chatMsg.timestamp}, millisecondsSinceEpoch: ${chatMsg.timestamp.millisecondsSinceEpoch}, converted timestamp: ${msg.timestamp}');
    print('[MessageConverter] Current DateTime.now(): ${DateTime.now()}, millisecondsSinceEpoch: ${DateTime.now().millisecondsSinceEpoch}');
    msg.sender = chatMsg.fromUserId;
    // CRITICAL: userID/groupID handling must be consistent with platform scheme
    // IMPORTANT: For received messages (isSelf=false), fromUserId is the sender's ID (peer's ID), so userID should be fromUserId
    // For sent messages (isSelf=true), fromUserId is our own ID, so we need to find the peer's ID from context
    // However, in binary replacement scheme, we don't have forwardTargetUserID, so we use a simpler logic:
    // - For group messages: use groupId
    // - For C2C messages: 
    //   - If received (isSelf=false): fromUserId is the peer's ID, so use it
    //   - If sent (isSelf=true): fromUserId is our own ID, but we can't determine peer's ID here
    //     This should be handled by the caller or by checking message history
    if (chatMsg.groupId != null) {
      // Group message: use groupId
      msg.groupID = chatMsg.groupId;
      msg.userID = null;
    } else {
      // C2C message
      if (chatMsg.isSelf) {
        // For sent messages, we can't determine the peer's ID from ChatMessage alone
        // This should be handled by the caller or by checking message history
        // For now, leave userID as null - it should be set by the caller
        msg.userID = null; // Will be fixed by caller if needed
      } else {
        // For received messages, fromUserId is the sender's ID (peer's ID)
        msg.userID = chatMsg.fromUserId;
      }
    }
    msg.isSelf = chatMsg.isSelf;
    
    // Set message status - must be consistent with platform scheme
    if (chatMsg.isPending) {
      msg.status = MessageStatus.V2TIM_MSG_STATUS_SENDING;
    } else if (chatMsg.isReceived) {
      msg.status = MessageStatus.V2TIM_MSG_STATUS_SEND_SUCC;
    } else {
      msg.status = MessageStatus.V2TIM_MSG_STATUS_SEND_FAIL;
    }
    
    // Set element based on media type and populate elemList
    // elemList is used to generate message_elem_array in toJson()
    switch (elemType) {
      case MessageElemType.V2TIM_ELEM_TYPE_TEXT:
        msg.textElem = V2TimTextElem(text: chatMsg.text);
        // Add to elemList so it appears in message_elem_array
        msg.elemList.add(msg.textElem!);
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_IMAGE:
        if (chatMsg.filePath != null) {
          // Generate UUID from msgID for download identification
          // CRITICAL: Use msgID directly (consistent with platform scheme)
          // msgID is always set, so no need for fallback
          final imageUuid = msgID.replaceAll(RegExp(r'[^a-zA-Z0-9]'), '_');
          int? fileSize;
          try {
            final file = File(chatMsg.filePath!);
            if (file.existsSync()) {
              fileSize = file.lengthSync();
            }
          } catch (e) {
            // Ignore file size errors
          }
          
          // Create imageList with both thumb and origin images
          // UIKit may request either THUMB (1) or ORIGIN (0), so we need both
          // This is required for downloadMessage to work properly
          final imageList = [
            V2TimImage(
              uuid: imageUuid,
              type: V2TIM_IMAGE_TYPE.V2TIM_IMAGE_TYPE_THUMB,
              size: fileSize,
              url: chatMsg.filePath, // Use local path as URL for Tox protocol
              localUrl: chatMsg.filePath,
            ),
            V2TimImage(
              uuid: imageUuid,
              type: V2TIM_IMAGE_TYPE.V2TIM_IMAGE_TYPE_ORIGIN,
              size: fileSize,
              url: chatMsg.filePath, // Use local path as URL for Tox protocol
              localUrl: chatMsg.filePath,
            ),
          ];
          
          msg.imageElem = V2TimImageElem(
            path: chatMsg.filePath,
            imageList: imageList,
          );
          // Add to elemList so it appears in message_elem_array
          msg.elemList.add(msg.imageElem!);
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_VIDEO:
        if (chatMsg.filePath != null) {
          msg.videoElem = V2TimVideoElem(videoPath: chatMsg.filePath);
          // Add to elemList so it appears in message_elem_array
          msg.elemList.add(msg.videoElem!);
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_SOUND:
        if (chatMsg.filePath != null) {
          msg.soundElem = V2TimSoundElem(path: chatMsg.filePath);
          // Add to elemList so it appears in message_elem_array
          msg.elemList.add(msg.soundElem!);
        }
        break;
      case MessageElemType.V2TIM_ELEM_TYPE_FILE:
        if (chatMsg.filePath != null) {
          // Generate UUID from msgID for download identification
          // CRITICAL: Use msgID directly (consistent with platform scheme)
          // msgID is always set, so no need for fallback
          final fileUuid = msgID.replaceAll(RegExp(r'[^a-zA-Z0-9]'), '_');
          int? fileSize;
          try {
            final file = File(chatMsg.filePath!);
            if (file.existsSync()) {
              fileSize = file.lengthSync();
            }
          } catch (e) {
            // Ignore file size errors
          }
          
          msg.fileElem = V2TimFileElem(
            path: chatMsg.filePath,
            fileName: chatMsg.fileName,
            UUID: fileUuid, // Required for downloadMessage
            url: chatMsg.filePath, // Use local path as URL for Tox protocol
            fileSize: fileSize,
            localUrl: chatMsg.filePath,
          );
          // Add to elemList so it appears in message_elem_array
          msg.elemList.add(msg.fileElem!);
        }
        break;
      default:
        // Default to text message
        msg.textElem = V2TimTextElem(text: chatMsg.text);
        // Add to elemList so it appears in message_elem_array
        msg.elemList.add(msg.textElem!);
        break;
    }
    
    return msg;
  }
  
  /// Convert a list of ChatMessages to V2TimMessages
  static List<V2TimMessage> chatMessagesToV2TimMessages(List<ChatMessage> chatMessages, String selfId) {
    return chatMessages.map((msg) => chatMessageToV2TimMessage(msg, selfId)).toList();
  }
  
  /// Convert a list of V2TimMessages to ChatMessages
  static List<ChatMessage> v2TimMessagesToChatMessages(List<V2TimMessage> v2Messages, String selfId) {
    return v2Messages.map((msg) => v2TimMessageToChatMessage(msg, selfId)).toList();
  }
}
