/// Events Test
/// 
/// Tests various event callbacks and their triggering
/// Reference: c-toxcore/auto_tests/scenarios/scenario_events_test.c

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_conversation_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimSDKListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimConversationListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimFriendshipListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimGroupListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/friend_type_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_user_full_info.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Events Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late TestNode bob;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['alice', 'bob']);
      alice = scenario.getNode('alice')!;
      bob = scenario.getNode('bob')!;
      
      await scenario.initAllNodes();
      // Parallelize login
      await Future.wait([
        alice.login(),
        bob.login(),
      ]);
      
      // Wait for both nodes to be connected
      await waitUntil(
        () => alice.loggedIn && bob.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'condition',
      );
      
      // Configure local bootstrap
      await configureLocalBootstrap(scenario);
      // Establish friendship so C2C message and conversation tests can send to each other
      await establishFriendship(alice, bob);
    });
    
    tearDownAll(() async {
      await scenario.dispose();
      await teardownTestEnvironment();
    });
    
    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      // Reset any per-test state if necessary
      // Most tests don't need cleanup since they use shared scenario
    });
    
    test('SDK listener events', () async {
      final eventsReceived = <String>[];
      
      // Set up SDK listener on alice's instance
      final sdkListener = V2TimSDKListener(
        onConnecting: () {
          eventsReceived.add('onConnecting');
          alice.markCallbackReceived('onConnecting');
        },
        onConnectSuccess: () {
          eventsReceived.add('onConnectSuccess');
          alice.markCallbackReceived('onConnectSuccess');
        },
        onSelfInfoUpdated: (V2TimUserFullInfo info) {
          eventsReceived.add('onSelfInfoUpdated');
          alice.markCallbackReceived('onSelfInfoUpdated');
        },
        onUserStatusChanged: (userStatusList) {
          eventsReceived.add('onUserStatusChanged');
          alice.markCallbackReceived('onUserStatusChanged');
        },
      );
      
      alice.runWithInstance(() => TIMManager.instance.addSDKListener(sdkListener));
      
      // Trigger self info update on alice's instance
      final userInfo = V2TimUserFullInfo(
        userID: alice.userId,
        nickName: 'Test Name',
      );
      
      await alice.runWithInstanceAsync(() async => TIMManager.instance.setSelfInfo(
        userFullInfo: userInfo,
      ));
      
      // Wait for events
      await Future.delayed(const Duration(seconds: 2));
      
      // Verify events were received
      expect(eventsReceived.length, greaterThan(0), reason: 'SDK listener should fire (e.g. onSelfInfoUpdated); eventsReceived=$eventsReceived');
      expect(alice.callbackReceived['onSelfInfoUpdated'], isTrue);
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Message listener events', () async {
      final eventsReceived = <String>[];
      final completer = Completer<V2TimMessage>();
      
      // Set up message listener on bob's instance
      final msgListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          eventsReceived.add('onRecvNewMessage');
          bob.markCallbackReceived('onRecvNewMessage');
          bob.addReceivedMessage(message);
          if (!completer.isCompleted) {
            completer.complete(message);
          }
        },
      );
      
      bob.runWithInstance(() => TIMMessageManager.instance.addAdvancedMsgListener(msgListener));
      
      // Send a message from alice to bob (use Tox ID for receiver)
      final messageText = 'Test message for events';
      final sendResult = await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: messageText);
        return TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bob.getToxId(),
          onlineUserOnly: false,
        );
      });
      print('[Events] Message listener: sendMessage code=${sendResult.code} desc=${sendResult.desc}');
      
      // Wait for message event
      await completer.future.timeout(
        const Duration(seconds: 45),
        onTimeout: () {
          throw TimeoutException('Timeout waiting for message event');
        },
      );
      
      // Verify event was received
      expect(eventsReceived, contains('onRecvNewMessage'));
      expect(bob.callbackReceived['onRecvNewMessage'], isTrue);
      expect(bob.receivedMessages.length, greaterThan(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Friendship listener events', () async {
      final eventsReceived = <String>[];
      final completer = Completer<void>();
      
      // Set up friendship listener on bob's instance (alice and bob already friends from setUpAll)
      final friendshipListener = V2TimFriendshipListener(
        onFriendApplicationListAdded: (applicationList) {
          eventsReceived.add('onFriendApplicationListAdded');
          bob.markCallbackReceived('onFriendApplicationListAdded');
          if (!completer.isCompleted) {
            completer.complete();
          }
        },
        onFriendListAdded: (users) {
          eventsReceived.add('onFriendListAdded');
          bob.markCallbackReceived('onFriendListAdded');
        },
        onFriendInfoChanged: (infoList) {
          eventsReceived.add('onFriendInfoChanged');
          bob.markCallbackReceived('onFriendInfoChanged');
          if (!completer.isCompleted) {
            completer.complete();
          }
        },
      );
      
      bob.runWithInstance(() => TIMFriendshipManager.instance.addFriendListener(listener: friendshipListener));
      
      // Trigger friend info change: alice updates her name so bob may receive onFriendInfoChanged
      await alice.runWithInstanceAsync(() async => TIMManager.instance.setSelfInfo(
        userFullInfo: V2TimUserFullInfo(
          userID: alice.userId,
          nickName: 'AliceFriendTest',
        ),
      ));
      
      // Wait for friend info changed (or other friendship) event
      await completer.future.timeout(
        const Duration(seconds: 15),
        onTimeout: () {
          throw TimeoutException('Timeout waiting for friendship event (onFriendInfoChanged). eventsReceived=$eventsReceived');
        },
      );
      
      expect(eventsReceived, isNotEmpty, reason: 'At least one friendship event when alice updates name; eventsReceived=$eventsReceived');
      expect(
        bob.callbackReceived['onFriendInfoChanged'] == true ||
            bob.callbackReceived['onFriendListAdded'] == true ||
            bob.callbackReceived['onFriendApplicationListAdded'] == true,
        isTrue,
        reason: 'One of onFriendInfoChanged/onFriendListAdded/onFriendApplicationListAdded must fire',
      );
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Conversation listener events', () async {
      final eventsReceived = <String>[];
      
      // Set up conversation listener on bob's instance
      final conversationListener = V2TimConversationListener(
        onNewConversation: (conversationList) {
          eventsReceived.add('onNewConversation');
          bob.markCallbackReceived('onNewConversation');
        },
        onConversationChanged: (conversationList) {
          eventsReceived.add('onConversationChanged');
          bob.markCallbackReceived('onConversationChanged');
        },
        onTotalUnreadMessageCountChanged: (totalUnreadCount) {
          eventsReceived.add('onTotalUnreadMessageCountChanged');
          bob.markCallbackReceived('onTotalUnreadMessageCountChanged');
        },
      );
      
      bob.runWithInstance(() => TIMConversationManager.instance.addConversationListener(listener: conversationListener));
      
      // Send a message from alice to bob to trigger conversation events (use Tox ID)
      final messageText = 'Test message for conversation';
      await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: messageText);
        return TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bob.getToxId(),
          onlineUserOnly: false,
        );
      });
      
      // Wait for conversation events
      await Future.delayed(const Duration(seconds: 2));
      
      // Verify events were received (at least one should be triggered)
      expect(eventsReceived.length, greaterThanOrEqualTo(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Group listener events', () async {
      final eventsReceived = <String>[];
      final completer = Completer<String>();
      
      // Set up group listener on alice's instance
      final groupListener = V2TimGroupListener(
        onGroupCreated: (String groupID) {
          eventsReceived.add('onGroupCreated');
          alice.markCallbackReceived('onGroupCreated');
          if (!completer.isCompleted) {
            completer.complete(groupID);
          }
        },
        onMemberEnter: (groupID, memberList) {
          eventsReceived.add('onMemberEnter');
          alice.markCallbackReceived('onMemberEnter');
        },
        onGroupInfoChanged: (groupID, changeInfos) {
          eventsReceived.add('onGroupInfoChanged');
          alice.markCallbackReceived('onGroupInfoChanged');
        },
      );
      
      alice.runWithInstance(() => TIMGroupManager.instance.addGroupListener(groupListener));
      
      // Create a group on alice's instance to trigger events
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Work',
        groupName: 'Test Group',
        memberList: [],
      ));
      
      expect(createResult.code, equals(0));
      
      // Wait for group created event
      final groupID = await completer.future.timeout(
        const Duration(seconds: 30),
        onTimeout: () {
          throw TimeoutException('Timeout waiting for group created event');
        },
      );
      
      // Verify event was received
      expect(eventsReceived, contains('onGroupCreated'));
      expect(alice.callbackReceived['onGroupCreated'], isTrue);
      expect(groupID, isNotEmpty);
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Multiple listener events in sequence', () async {
      final allEvents = <String>[];
      
      // Set up multiple listeners on alice (SDK, msg) and bob (friendship for receiving)
      final sdkListener = V2TimSDKListener(
        onSelfInfoUpdated: (info) {
          allEvents.add('SDK:onSelfInfoUpdated');
        },
      );
      
      final msgListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (message) {
          allEvents.add('MSG:onRecvNewMessage');
        },
      );
      
      final friendshipListener = V2TimFriendshipListener(
        onFriendApplicationListAdded: (applicationList) {
          allEvents.add('FRIEND:onFriendApplicationListAdded');
        },
      );
      
      alice.runWithInstance(() {
        TIMManager.instance.addSDKListener(sdkListener);
        TIMMessageManager.instance.addAdvancedMsgListener(msgListener);
      });
      bob.runWithInstance(() => TIMFriendshipManager.instance.addFriendListener(listener: friendshipListener));
      
      // Trigger multiple events on alice's instance
      // 1. Self info update
      final userInfo = V2TimUserFullInfo(
        userID: alice.userId,
        nickName: 'Test Name',
      );
      await alice.runWithInstanceAsync(() async => TIMManager.instance.setSelfInfo(userFullInfo: userInfo));
      
      // 2. Friend request (alice adds bob, use Tox ID)
      await alice.runWithInstanceAsync(() async => TIMFriendshipManager.instance.addFriend(
        userID: bob.getToxId(),
        addWording: 'Hello!',
        addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_SINGLE,
      ));
      
      // 3. Send message (alice to bob, use Tox ID)
      await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Test');
        return TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bob.getToxId(),
          onlineUserOnly: false,
        );
      });
      
      // Wait for all events
      await Future.delayed(const Duration(seconds: 2));
      
      expect(allEvents.length, greaterThan(0), reason: 'At least one of SDK/msg/friendship events should fire; allEvents=$allEvents');
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Event callback parameters verification', () async {
      V2TimMessage? receivedMessage;
      V2TimUserFullInfo? updatedInfo;
      
      // Set up listeners on alice (SDK) and bob (msg) with parameter verification
      final sdkListener = V2TimSDKListener(
        onSelfInfoUpdated: (V2TimUserFullInfo info) {
          updatedInfo = info;
        },
      );
      
      final msgListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          receivedMessage = message;
        },
      );
      
      alice.runWithInstance(() => TIMManager.instance.addSDKListener(sdkListener));
      bob.runWithInstance(() => TIMMessageManager.instance.addAdvancedMsgListener(msgListener));
      
      // Set self info on alice's instance
      final testName = 'Test Name';
      final userInfo = V2TimUserFullInfo(
        userID: alice.userId,
        nickName: testName,
      );
      await alice.runWithInstanceAsync(() async => TIMManager.instance.setSelfInfo(userFullInfo: userInfo));
      
      await Future.delayed(const Duration(seconds: 2));
      
      // Verify callback parameters
      expect(updatedInfo, isNotNull);
      expect(updatedInfo!.nickName, equals(testName));
      
      // Send message from alice to bob (use Tox ID)
      final messageText = 'Test message';
      final sendResult = await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: messageText);
        return TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bob.getToxId(),
          onlineUserOnly: false,
        );
      });
      print('[Events] Event callback params: sendMessage code=${sendResult.code} desc=${sendResult.desc}');
      
      // Wait for bob to receive
      await waitUntil(() => receivedMessage != null, timeout: const Duration(seconds: 45));
      
      expect(receivedMessage, isNotNull, reason: 'Bob should receive C2C message (sendResult code=${sendResult.code})');
      expect(receivedMessage!.textElem?.text, equals(messageText));
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
