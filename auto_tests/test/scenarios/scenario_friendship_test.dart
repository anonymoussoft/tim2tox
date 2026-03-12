/// Friendship Test
import 'dart:async';
/// 
/// Tests friend management: add, delete, query, and friend request handling
/// Reference: c-toxcore/auto_tests/scenarios/scenario_friend_request_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Friendship Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late TestNode bob;

    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['alice', 'bob']);
      alice = scenario.getNode('alice')!;
      bob = scenario.getNode('bob')!;

      await scenario.initAllNodes();
      await Future.wait([alice.login(), bob.login()]);
      await waitUntil(() => alice.loggedIn && bob.loggedIn);
      await configureLocalBootstrap(scenario);
    });

    tearDownAll(() async {
      await scenario.dispose();
      await teardownTestEnvironment();
    });

    setUp(() async {});

    test('Get friend list', () async {
      final result = await TIMFriendshipManager.instance.getFriendList();
      expect(result.code, equals(0));
      expect(result.data, isNotNull);
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
