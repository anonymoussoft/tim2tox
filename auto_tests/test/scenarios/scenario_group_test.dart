/// Group Test
/// 
/// Tests group management: create, join, quit, dismiss, and group operations
/// Reference: c-toxcore/auto_tests/scenarios/scenario_group_general_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Tests', () {
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
    
    test('Create group', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
        notification: 'Test notification',
        introduction: 'Test introduction',
      ));
      expect(createResult.code, equals(0));
      expect(createResult.data, isNotNull);
      expect(createResult.data, isNotEmpty);
      alice.state['group_id'] = createResult.data!;
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    /// Join public group by passing 64-char chat_id as groupID (no invite; single-account / link join path).
    test('Join public group by 64-char chat_id only', () async {
      await establishFriendship(alice, bob);
      await Future.wait([
        alice.waitForFriendConnection(bob.getToxId(), timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(alice.getToxId(), timeout: const Duration(seconds: 30)),
      ]);
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'group',
        groupName: 'Public Group For ChatId Join',
        groupID: '',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      expect(alice.testInstanceHandle, isNotNull);
      final chatId = getGroupChatIdForInstance(alice.testInstanceHandle!, groupId);
      expect(chatId, isNotNull, reason: 'chat_id must be available after createGroup');
      expect(chatId!.length, equals(64), reason: 'chat_id must be 64 hex chars');
      await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 2));
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: chatId,
        message: '',
      ));
      expect(joinResult.code, equals(0), reason: 'joinGroup by chat_id failed: ${joinResult.code}');
      await Future.delayed(const Duration(seconds: 2));
      final bobListResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(bobListResult.code, equals(0));
      expect(
        bobListResult.data?.any((g) => g.groupID == chatId || g.groupID == groupId) ?? false,
        isTrue,
        reason: 'Bob should see the joined group in getJoinedGroupList',
      );
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Join group', () async {
      await establishFriendship(alice, bob);
      final aliceToxId = alice.getToxId();
      final bobToxId = bob.getToxId();
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final bobPublicKey = bob.getPublicKey();
      bob.clearCallbackReceived('onGroupInvited');
      final inviteResult = await alice.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.inviteUserToGroup(groupID: groupId, userList: [bobPublicKey]));
      expect(inviteResult.code, equals(0));
      await bob.waitForCallback('onGroupInvited', timeout: const Duration(seconds: 20));
      await Future.delayed(const Duration(milliseconds: 500));
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: 'Hello, I want to join!',
      ));
      expect(joinResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Get joined group list', () async {
      await establishFriendship(alice, bob);
      final aliceToxId = alice.getToxId();
      final bobToxId = bob.getToxId();
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final bobPublicKey = bob.getPublicKey();
      bob.clearCallbackReceived('onGroupInvited');
      await alice.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.inviteUserToGroup(groupID: groupId, userList: [bobPublicKey]));
      await bob.waitForCallback('onGroupInvited', timeout: const Duration(seconds: 20));
      await Future.delayed(const Duration(milliseconds: 500));
      await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      await Future.delayed(const Duration(seconds: 3));
      final groupListResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(groupListResult.code, equals(0), reason: 'getJoinedGroupList failed: ${groupListResult.code}');
      expect(groupListResult.data, isNotNull);
      expect(groupListResult.data!.length, greaterThan(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Get groups info', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      await Future.delayed(const Duration(seconds: 2));
      await Future.delayed(const Duration(seconds: 3));
      final groupsInfoResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupsInfo(
        groupIDList: [groupId],
      ));
      expect(groupsInfoResult.code, equals(0), reason: 'getGroupsInfo failed: ${groupsInfoResult.code}');
      expect(groupsInfoResult.data, isNotNull);
      expect(groupsInfoResult.data!.length, equals(1));
      expect(groupsInfoResult.data!.first.groupInfo?.groupID, equals(groupId));
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Quit group', () async {
      await establishFriendship(alice, bob);
      final aliceToxId = alice.getToxId();
      final bobToxId = bob.getToxId();
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final bobPublicKey = bob.getPublicKey();
      bob.clearCallbackReceived('onGroupInvited');
      await alice.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.inviteUserToGroup(groupID: groupId, userList: [bobPublicKey]));
      await bob.waitForCallback('onGroupInvited', timeout: const Duration(seconds: 20));
      await Future.delayed(const Duration(milliseconds: 500));
      await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      await Future.delayed(const Duration(seconds: 3));
      final quitResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.quitGroup(groupID: groupId));
      expect(quitResult.code, equals(0), reason: 'quitGroup failed: ${quitResult.code}');
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Dismiss group', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      await Future.delayed(const Duration(seconds: 2));
      final dismissResult = await alice.runWithInstanceAsync(() async => TIMManager.instance.dismissGroup(groupID: groupId));
      expect(dismissResult.code, equals(0), reason: 'dismissGroup failed: ${dismissResult.code}');
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
