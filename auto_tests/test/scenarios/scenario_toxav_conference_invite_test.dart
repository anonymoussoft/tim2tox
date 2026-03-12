/// ToxAV Conference Invite Test
/// 
/// Tests AV conference invitation flow
/// Verifies that TOX_CONFERENCE_TYPE_AV invites are properly handled
/// Reference: c-toxcore/auto_tests/scenarios/scenario_conference_av_test.c

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/friend_type_enum.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimGroupListener.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_group_member_info.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('ToxAV Conference Invite Tests', () {
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
      
      await waitUntil(
        () => alice.loggedIn && bob.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'all nodes logged in',
      );
      
      // Configure local bootstrap
      await configureLocalBootstrap(scenario);
      
      // Enable auto-accept in Dart so friend request is accepted (no C++ default)
      alice.enableAutoAccept();
      bob.enableAutoAccept();
      
      // Add friends (use Tox ID for addFriend like scenario_toxav_conference_test)
      final bobToxId = bob.getToxId();
      await alice.runWithInstanceAsync(() async => TIMFriendshipManager.instance.addFriend(
        userID: bobToxId,
        addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
        remark: 'Bob',
        addWording: 'test',
      ));
      await bob.runWithInstanceAsync(() async => TIMFriendshipManager.instance.addFriend(
        userID: alice.getToxId(),
        addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
        remark: 'Alice',
        addWording: 'test',
      ));

      // Friend requests may need time to propagate/auto-accept over the local DHT.
      await Future.delayed(const Duration(seconds: 5));

      // Friend list stores *public keys* (64 chars), not full Tox IDs
      final alicePub = alice.getPublicKey();
      final bobPub = bob.getPublicKey();
      await waitForFriendsInList(alice, [bobPub], timeout: const Duration(seconds: 120));
      await waitForFriendsInList(bob, [alicePub], timeout: const Duration(seconds: 120));
      // Wait for P2P connection so conference invite can be delivered
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 60)),
        bob.waitForFriendConnection(alice.getToxId(), timeout: const Duration(seconds: 60)),
      ]);
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
    
    test('Receive and accept AV conference invite', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'conference',
        groupName: 'AV Conference Invite Test',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0));
      final conferenceId = createResult.data!;
      
      await alice.waitForCallback('onGroupCreated', 
        timeout: const Duration(seconds: 10));
      
      var inviteReceived = false;
      String? receivedGroupId;
      String? inviterId;
      
      final bobGroupListener = V2TimGroupListener(
        onMemberInvited: (String groupID, V2TimGroupMemberInfo opUser, List<V2TimGroupMemberInfo> memberList) {
          inviteReceived = true;
          receivedGroupId = groupID;
          inviterId = opUser.userID;
          bob.markCallbackReceived('onMemberInvited');
        },
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.addGroupListener(bobGroupListener));
      
      final bobPublicKey = bob.getPublicKey();
      final alicePublicKey = alice.getPublicKey();
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: conferenceId,
        userList: [bobPublicKey],
      ));
      
      expect(inviteResult.code, equals(0),
        reason: 'Failed to send invite: ${inviteResult.code}');
      
      await waitUntil(
        () => inviteReceived,
        timeout: const Duration(seconds: 15),
        description: 'Bob received AV conference invite',
      );
      
      expect(inviteReceived, isTrue,
        reason: 'Bob did not receive invite');
      expect(receivedGroupId, isNotNull,
        reason: 'Received group ID should not be null (may be temp tox_inv_* or conferenceId)');
      expect(inviterId, equals(alicePublicKey),
        reason: 'Inviter ID mismatch');
      
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: receivedGroupId!,
        message: 'Joining AV conference',
      ));
      
      expect(joinResult.code, equals(0),
        reason: 'Failed to join AV conference: ${joinResult.code}');
      
      await Future.delayed(const Duration(seconds: 3));
      
      final bobJoinedList = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(bobJoinedList.code, equals(0));
      expect(bobJoinedList.data, isNotNull);
      
      final bobGroupIds = bobJoinedList.data!.map((g) => g.groupID).toList();
      expect(bobGroupIds, isNotEmpty, reason: 'Bob should have joined a group');
      expect(
        bobGroupIds.contains(conferenceId) || bobGroupIds.contains(receivedGroupId),
        isTrue,
        reason: 'Bob not in conference after join (expected $conferenceId or $receivedGroupId, got $bobGroupIds)',
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.removeGroupListener(listener: bobGroupListener));
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Multiple AV conference invites', () async {
      final createResult1 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'conference',
        groupName: 'AV Conference 1',
        groupID: '',
      ));
      
      final createResult2 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'conference',
        groupName: 'AV Conference 2',
        groupID: '',
      ));
      
      expect(createResult1.code, equals(0));
      expect(createResult2.code, equals(0));
      
      final conferenceId1 = createResult1.data!;
      final conferenceId2 = createResult2.data!;
      
      await alice.waitForCallback('onGroupCreated', 
        timeout: const Duration(seconds: 10));
      
      // C++ notifies OnMemberInvited with temp ID (tox_inv_*), not conferenceId; collect and join with received IDs
      final receivedInvites = <String>[];
      
      final bobGroupListener = V2TimGroupListener(
        onMemberInvited: (String groupID, V2TimGroupMemberInfo opUser, List<V2TimGroupMemberInfo> memberList) {
          receivedInvites.add(groupID);
          bob.markCallbackReceived('onMemberInvited');
        },
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.addGroupListener(bobGroupListener));
      
      final bobPublicKey = bob.getPublicKey();
      final inviteResult1 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: conferenceId1,
        userList: [bobPublicKey],
      ));
      
      final inviteResult2 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: conferenceId2,
        userList: [bobPublicKey],
      ));
      
      expect(inviteResult1.code, equals(0));
      expect(inviteResult2.code, equals(0));
      
      await waitUntil(
        () => receivedInvites.length >= 2,
        timeout: const Duration(seconds: 45),
        description: 'Bob received both invites',
      );
      
      expect(receivedInvites.length, greaterThanOrEqualTo(2), reason: 'Bob should receive 2 invites (may be temp IDs)');
      
      final joinResult1 = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: receivedInvites[0],
        message: '',
      ));
      
      final joinResult2 = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: receivedInvites[1],
        message: '',
      ));
      
      expect(joinResult1.code, equals(0));
      expect(joinResult2.code, equals(0));
      
      await Future.delayed(const Duration(seconds: 5));
      
      final bobJoinedList = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(bobJoinedList.data, isNotNull);
      
      final bobGroupIds = bobJoinedList.data!.map((g) => g.groupID).toList();
      expect(bobGroupIds.length, greaterThanOrEqualTo(2), reason: 'Bob should have joined 2 groups');
      expect(bobGroupIds, contains(receivedInvites[0]));
      expect(bobGroupIds, contains(receivedInvites[1]));
      
      bob.runWithInstance(() => TIMGroupManager.instance.removeGroupListener(listener: bobGroupListener));
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
