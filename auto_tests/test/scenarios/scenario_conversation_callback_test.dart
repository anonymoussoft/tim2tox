/// Conversation callback test — wall-clock variant
///
/// Mirror of `scenario_conversation_callback_virtual_test.dart` for wall-clock
/// mode. Split out of `scenario_conversation_test.dart` for the same reason
/// as the virtual variant — the conversation-changed fan-out's stream
/// re-entry guard composes badly when 4 sub-tests share one scenario.

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
  group('Conversation Callback Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late TestNode bob;

    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['alice', 'bob']);
      alice = scenario.getNode('alice')!;
      bob = scenario.getNode('bob')!;

      await scenario.initAllNodes();

      await Future.wait([
        alice.login(),
        bob.login(),
      ]);

      await waitUntil(
        () => alice.loggedIn && bob.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'both nodes logged in',
      );

      await configureLocalBootstrap(scenario);
      await establishFriendship(alice, bob);
    });

    tearDownAll(() async {
      await scenario.dispose();
      await teardownTestEnvironment();
    });

    test('Conversation callback - onConversationChanged', () async {
      alice.clearCallbackReceived('onConversationChanged');
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 45));

      final listener = V2TimConversationListener(
        onConversationChanged: (List<V2TimConversation> conversationList) {
          alice.markCallbackReceived('onConversationChanged');
        },
      );

      await alice.runWithInstanceAsync(() async {
        TIMConversationManager.instance
            .addConversationListener(listener: listener);
        await TencentCloudChatSdkPlatform.instance
            .addConversationListener(listener: listener);
      });

      try {
        var callbackArrived = false;
        for (var attempt = 0; !callbackArrived && attempt < 3; attempt++) {
          alice.clearCallbackReceived('onConversationChanged');
          final sendResult = await alice.runWithInstanceAsync(() async {
            final messageResult =
                TIMMessageManager.instance.createTextMessage(text: 'Hello');
            return await TIMMessageManager.instance.sendMessage(
              groupID: null,
              message: messageResult.messageInfo,
              receiver: bobToxId,
              onlineUserOnly: false,
            );
          });
          expect(sendResult.code, equals(0));

          try {
            await waitUntil(
              () => alice.callbackReceived['onConversationChanged'] == true,
              timeout: const Duration(seconds: 30),
              description:
                  'onConversationChanged callback (attempt ${attempt + 1})',
            );
            callbackArrived = true;
          } catch (_) {
            // retry — packet may have been dropped
          }
        }
        expect(alice.callbackReceived['onConversationChanged'], isTrue,
            reason:
                'onConversationChanged must fire after send (after 3 retries)');
      } finally {
        alice.runWithInstance(() => TIMConversationManager.instance
            .removeConversationListener(listener: listener));
        await TencentCloudChatSdkPlatform.instance
            .removeConversationListener(listener: listener);
      }
    }, timeout: const Timeout(Duration(seconds: 240)));
  });
}
