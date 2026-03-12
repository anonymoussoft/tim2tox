/// Conversation Test
/// 
/// Tests conversation management: get list, get conversation, delete conversation
/// Reference: c-toxcore auto_tests patterns

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_conversation_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimConversationListener.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_conversation.dart';
import 'package:tencent_cloud_chat_sdk/tencent_cloud_chat_sdk_platform_interface.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Conversation Tests', () {
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
        description: 'both nodes logged in',
      );
      
      // Configure local bootstrap (like C test's tox_node_bootstrap)
      await configureLocalBootstrap(scenario);
      // Establish friendship so C2C messages can be sent (receiver must be Tox ID)
      await establishFriendship(alice, bob);
      // Pump both instances so Tox P2P connection establishes (friend in list + connection_status != NONE)
      await pumpFriendConnection(alice, bob);
    });
    
    tearDownAll(() async {
      // Allow pending native callbacks (e.g. OnRecvNewMessage) to drain before teardown
      await Future.delayed(const Duration(seconds: 1));
      await scenario.dispose();
      await teardownTestEnvironment();
    });
    
    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      // Reset any per-test state if necessary
      // Most tests don't need cleanup since they use shared scenario
    });
    
    test('Get conversation list', () async {
      // Use Tox ID for C2C (tim2tox requires 76-char hex Tox ID as receiver)
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 45));
      
      final sendResult = await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Hello');
        return await TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bobToxId,
          onlineUserOnly: false,
        );
      });
      expect(sendResult.code, equals(0), reason: 'Send message should succeed');
      
      // Allow OnRecvNewMessage callback to be processed before getConversationList (avoids native race)
      await Future.delayed(const Duration(seconds: 2));
      
      final convListResult = await alice.runWithInstanceAsync(() async =>
          await TIMConversationManager.instance.getConversationList(
            nextSeq: '0',
            count: 10,
          ));
      
      expect(convListResult.code, equals(0));
      expect(convListResult.data, isNotNull);
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Get conversation', () async {
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 45));
      
      await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Hello');
        final r = await TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bobToxId,
          onlineUserOnly: false,
        );
        expect(r.code, equals(0));
      });
      
      await Future.delayed(const Duration(seconds: 2));
      
      // Native stores C2C conversationID as c2c_<64-char public key>, not full 76-char Tox ID
      final bobPublicKey = bob.getPublicKey();
      final conversationID = 'c2c_$bobPublicKey';
      final convResult = await alice.runWithInstanceAsync(() async =>
          await TIMConversationManager.instance.getConversation(
            conversationID: conversationID,
          ));
      
      expect(convResult.code, equals(0));
      if (convResult.data != null) {
        expect(convResult.data?.conversationID, equals(conversationID));
      }
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Delete conversation', () async {
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 45));
      
      await alice.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Hello');
        final r = await TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo,
          receiver: bobToxId,
          onlineUserOnly: false,
        );
        expect(r.code, equals(0));
      });
      
      await Future.delayed(const Duration(seconds: 2));
      
      final conversationID = 'c2c_${bob.getPublicKey()}';
      final deleteResult = await alice.runWithInstanceAsync(() async =>
          await TIMConversationManager.instance.deleteConversation(
            conversationID: conversationID,
          ));
      
      expect(deleteResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Conversation callback - onConversationChanged', () async {
      alice.clearCallbackReceived('onConversationChanged');
      // Re-establish friend visibility after "Delete conversation" (test 3); getFriendList can be empty otherwise
      await establishFriendship(alice, bob, timeout: const Duration(seconds: 90));
      await pumpFriendConnection(alice, bob);
      final bobToxId = bob.getToxId();
      pumpAllInstancesOnce(iterations: 80);
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 90));
      
      final listener = V2TimConversationListener(
        onConversationChanged: (List<V2TimConversation> conversationList) {
          alice.markCallbackReceived('onConversationChanged');
        },
      );
      
      await alice.runWithInstanceAsync(() async {
        TIMConversationManager.instance.addConversationListener(listener: listener);
        // Ensure platform (tim2tox) also has this listener so sendMessage -> notifyConversationChangedForC2C notifies us
        await TencentCloudChatSdkPlatform.instance.addConversationListener(listener: listener);
      });
      
      try {
        final sendResult = await alice.runWithInstanceAsync(() async {
          final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Hello');
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            message: messageResult.messageInfo,
            receiver: bobToxId,
            onlineUserOnly: false,
          );
        });
        expect(sendResult.code, equals(0));
        
        // Allow native to process send and trigger conversation update (pump so event loop drains)
        await Future.delayed(const Duration(seconds: 2));
        pumpAllInstancesOnce(iterations: 100);
        
        await waitUntilWithPump(
          () => alice.callbackReceived['onConversationChanged'] == true,
          timeout: const Duration(seconds: 90),
          description: 'onConversationChanged callback',
          iterationsPerPump: 100,
          stepDelay: const Duration(milliseconds: 300),
        );
        expect(alice.callbackReceived['onConversationChanged'], isTrue, reason: 'onConversationChanged must fire after send');
      } finally {
        alice.runWithInstance(() => TIMConversationManager.instance.removeConversationListener(listener: listener));
        await TencentCloudChatSdkPlatform.instance.removeConversationListener(listener: listener);
      }
    }, timeout: const Timeout(Duration(seconds: 150)));
  });
}
