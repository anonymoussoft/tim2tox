import 'preferences_service.dart';

/// Extended preferences service interface with domain-specific methods
/// 
/// This extends the basic PreferencesService with methods specific to
/// chat application preferences. Clients can implement this interface
/// or provide a wrapper that maps these methods to their storage system.
abstract class ExtendedPreferencesService extends PreferencesService {
  // Groups
  Future<Set<String>> getGroups();
  Future<void> setGroups(Set<String> groups);
  
  // Quit groups
  Future<Set<String>> getQuitGroups();
  Future<void> setQuitGroups(Set<String> groups);
  Future<void> addQuitGroup(String groupId);
  Future<void> removeQuitGroup(String groupId);
  
  // Self profile
  Future<String?> getSelfAvatarHash();
  Future<void> setSelfAvatarHash(String? hash);
  
  // Friend data
  Future<String?> getFriendNickname(String friendId);
  Future<void> setFriendNickname(String friendId, String nickname);
  Future<String?> getFriendStatusMessage(String friendId);
  Future<void> setFriendStatusMessage(String friendId, String statusMessage);
  Future<String?> getFriendAvatarPath(String friendId);
  Future<void> setFriendAvatarPath(String friendId, String? path);
  
  // Local friends
  Future<Set<String>> getLocalFriends();
  Future<void> setLocalFriends(Set<String> ids);
  
  // Bootstrap node
  Future<String> getBootstrapNodeMode();
  Future<({String host, int port, String pubkey})?> getCurrentBootstrapNode();
  Future<void> setCurrentBootstrapNode(String host, int port, String pubkey);
  
  // Auto download
  Future<int> getAutoDownloadSizeLimit();
  Future<void> setAutoDownloadSizeLimit(int sizeInMB);
  
  // Avatar
  Future<String?> getAvatarPath();
  Future<void> setAvatarPath(String? path);
  
  // Friend avatar hash
  Future<String?> getFriendAvatarHash(String friendId);
  Future<void> setFriendAvatarHash(String friendId, String hash);
  
  // Downloads directory
  Future<String?> getDownloadsDirectory();
  Future<void> setDownloadsDirectory(String? path);
  
  // Group data
  Future<String?> getGroupName(String groupId);
  Future<void> setGroupName(String groupId, String name);
  Future<String?> getGroupAvatar(String groupId);
  Future<void> setGroupAvatar(String groupId, String? avatarPath);
  Future<String?> getGroupNotification(String groupId);
  Future<void> setGroupNotification(String groupId, String? notification);
  Future<String?> getGroupIntroduction(String groupId);
  Future<void> setGroupIntroduction(String groupId, String? introduction);
  Future<String?> getGroupOwner(String groupId);
  Future<void> setGroupOwner(String groupId, String ownerId);
  // Chat ID for group (64-char hex string, TOX_GROUP_CHAT_ID_SIZE * 2)
  Future<String?> getGroupChatId(String groupId);
  Future<void> setGroupChatId(String groupId, String chatId);
  
  // Blacklist (user-specific, bound to current user Tox ID)
  // If userToxId is not provided, the implementation should get it from the current context
  Future<Set<String>> getBlackList([String? userToxId]);
  Future<void> setBlackList(Set<String> userIDs, [String? userToxId]);
  Future<void> addToBlackList(List<String> userIDs, [String? userToxId]);
  Future<void> removeFromBlackList(List<String> userIDs, [String? userToxId]);
}

