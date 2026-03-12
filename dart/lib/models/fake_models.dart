/// Event models used by the SDK Platform
/// 
/// These models represent events that flow from the client's adapter layer
/// to the SDK Platform implementation.

class FakeConversation {
  FakeConversation({
    required this.conversationID,
    required this.title,
    required this.faceUrl,
    required this.unreadCount,
    this.isGroup = false,
    this.isPinned = false,
  });
  final String conversationID; // c2c_<uid> or group_<gid>
  final String title;
  final String? faceUrl;
  final int unreadCount;
  final bool isGroup;
  final bool isPinned;
}

class FakeUser {
  FakeUser({
    required this.userID,
    required this.nickName,
    this.faceUrl,
    this.online = false,
  });
  final String userID;
  final String nickName;
  final String? faceUrl;
  final bool online;
}

class FakeUnreadTotal {
  FakeUnreadTotal(this.total);
  final int total;
}

class FakeFriendApplication {
  FakeFriendApplication({
    required this.userID,
    required this.wording,
  });
  final String userID;
  final String wording;
}

class FakeFriendDeleted {
  FakeFriendDeleted({required this.userID});
  final String userID;
}

class FakeGroupDeleted {
  FakeGroupDeleted({required this.groupID});
  final String groupID;
}

/// Event topic constants
class EventTopics {
  static const conversation = 'FakeConversation';
  static const message = 'FakeMessage';
  static const typing = 'FakeTyping';
  static const unread = 'FakeUnread';
  static const contacts = 'FakeContacts';
  static const friendApps = 'FakeFriendApps';
  static const friendDeleted = 'FakeFriendDeleted';
  static const groupDeleted = 'FakeGroupDeleted';
}

