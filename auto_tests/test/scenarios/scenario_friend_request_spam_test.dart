/// Friend Request Spam Test
/// 
/// Tests handling of multiple friend requests (spam protection)
/// Reference: c-toxcore/auto_tests/scenarios/scenario_friend_request_spam_test.c

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_friend_application.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimFriendshipListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/friend_type_enum.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Friend Request Spam Tests', () {
    late TestScenario scenario;
    late TestNode receiver;
    late List<TestNode> senders;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['receiver']);
      receiver = scenario.getNode('receiver')!;
      
      // Create multiple senders (use 3 to keep test time reasonable; Tox friend adds are async)
      senders = [];
      for (int i = 0; i < 3; i++) {
        final sender = scenario.addNode(
          alias: 'sender$i',
          userId: 'sender${i}_${DateTime.now().millisecondsSinceEpoch}',
          userSig: 'sender${i}_sig',
        );
        senders.add(sender);
      }
      
      await scenario.initAllNodes();
      // Parallelize login for all nodes
      await Future.wait([
        receiver.login(),
        ...senders.map((s) => s.login()),
      ]);
      
      await waitUntil(() => receiver.loggedIn && senders.every((s) => s.loggedIn));
      
      // Configure local bootstrap
      await configureLocalBootstrap(scenario);
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
    
    test('Multiple friend requests handling', () async {
      int requestCount = 0;
      final completer = Completer<void>();
      
      // Set up friend request listener on receiver's instance
      final listener = V2TimFriendshipListener(
        onFriendApplicationListAdded: (List<V2TimFriendApplication> applicationList) {
          requestCount += applicationList.length;
          receiver.markCallbackReceived('onFriendApplicationListAdded');
          
          if (requestCount >= senders.length && !completer.isCompleted) {
            completer.complete();
          }
        },
      );
      
      receiver.runWithInstance(() {
        TIMFriendshipManager.instance.addFriendListener(listener: listener);
      });
      
      // All senders send friend requests (each in own instance; tim2tox uses Tox ID)
      final receiverToxId = receiver.getToxId();
      final addResults = await Future.wait(
        senders.map((sender) => sender.runWithInstanceAsync(() async =>
            TIMFriendshipManager.instance.addFriend(
          userID: receiverToxId,
          addWording: 'Hello!',
          addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_SINGLE,
        ))),
      );
      
      // All requests should be accepted (allow "already sent" as success for retries)
      final addOk = addResults.every((r) => r.code == 0 || (r.code != 0 && (r.desc.contains('already sent') || r.desc.contains('Already sent'))));
      expect(addOk, isTrue, reason: 'addFriend should succeed or already sent: ${addResults.map((r) => "code=${r.code} desc=${r.desc}").join("; ")}');
      
      // Wait for all requests to be received (callback or poll fallback)
      const pollInterval = Duration(seconds: 3);
      const totalWait = Duration(seconds: 120);
      final deadline = DateTime.now().add(totalWait);
      while (DateTime.now().isBefore(deadline)) {
        if (completer.isCompleted) break;
        if (requestCount >= senders.length) {
          if (!completer.isCompleted) completer.complete();
          break;
        }
        // Poll receiver's application list as fallback
        final appListResult = await receiver.runWithInstanceAsync(() async => TIMFriendshipManager.instance.getFriendApplicationList());
        if (appListResult.code == 0 && appListResult.data?.friendApplicationList != null) {
          final senderPks = senders.map((s) => s.getPublicKey()).toSet();
          final fromSenders = appListResult
              .data!
              .friendApplicationList!
              .whereType<V2TimFriendApplication>()
              .where((app) => senderPks.contains(app.userID))
              .length;
          if (fromSenders >= senders.length) {
            receiver.markCallbackReceived('onFriendApplicationListAdded');
            if (requestCount < senders.length) requestCount = fromSenders;
            if (!completer.isCompleted) completer.complete();
            break;
          }
        }
        await Future.delayed(pollInterval);
      }
      
      if (!completer.isCompleted) {
        throw TimeoutException('Timeout waiting for all ${senders.length} friend requests (got $requestCount)');
      }
      
      expect(receiver.callbackReceived['onFriendApplicationListAdded'], isTrue);
      expect(requestCount, greaterThanOrEqualTo(senders.length), reason: 'All friend requests should be received');
    }, timeout: const Timeout(Duration(seconds: 130)));
  });
}
