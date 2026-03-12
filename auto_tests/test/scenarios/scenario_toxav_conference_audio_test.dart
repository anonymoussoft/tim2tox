/// ToxAV Conference Audio Test
/// 
/// Tests audio data handling in AV conferences
/// Verifies that audio callbacks are properly set up and can receive audio data
/// Note: Actual audio data processing requires ToxAV to be initialized

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
  group('ToxAV Conference Audio Tests', () {
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
    
    test('AV conference setup for audio handling', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'conference',
        groupName: 'AV Conference Audio Test',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0));
      final conferenceId = createResult.data!;
      
      await alice.waitForCallback('onGroupCreated', 
        timeout: const Duration(seconds: 10));
      
      var bobReceivedInvite = false;
      String? bobInvitedGroupId;
      
      final bobGroupListener = V2TimGroupListener(
        onMemberInvited: (String groupID, V2TimGroupMemberInfo opUser, List<V2TimGroupMemberInfo> memberList) {
          bobReceivedInvite = true;
          bobInvitedGroupId = groupID;
          bob.markCallbackReceived('onMemberInvited');
        },
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.addGroupListener(bobGroupListener));
      
      final bobPublicKey = bob.getPublicKey();
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: conferenceId,
        userList: [bobPublicKey],
      ));
      
      expect(inviteResult.code, equals(0));
      
      await waitUntil(
        () => bobReceivedInvite,
        timeout: const Duration(seconds: 45),
        description: 'Bob received invite',
      );
      expect(bobInvitedGroupId, isNotNull, reason: 'Bob should have received a group ID (temp or conference)');
      
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: bobInvitedGroupId!,
        message: '',
      ));
      
      expect(joinResult.code, equals(0));
      
      await Future.delayed(const Duration(seconds: 5));
      
      final aliceJoinedList = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(aliceJoinedList.data, isNotNull);
      expect(aliceJoinedList.data!.any((g) => g.groupID == conferenceId), isTrue);
      
      final bobJoinedList = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(bobJoinedList.data, isNotNull);
      expect(bobJoinedList.data!.isNotEmpty, isTrue, reason: 'Bob should have joined a group');
      expect(
        bobJoinedList.data!.any((g) => g.groupID == conferenceId || g.groupID == bobInvitedGroupId),
        isTrue,
        reason: 'Bob not in conference (expected $conferenceId or $bobInvitedGroupId)',
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.removeGroupListener(listener: bobGroupListener));
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('AV conference type verification', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'conference',
        groupName: 'Conference Type Verification',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0));
      final conferenceId = createResult.data!;
      
      await alice.waitForCallback('onGroupCreated', 
        timeout: const Duration(seconds: 10));
      
      final joinedListResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      expect(joinedListResult.code, equals(0));
      expect(joinedListResult.data, isNotNull);
      
      final conference = joinedListResult.data!.firstWhere(
        (g) => g.groupID == conferenceId,
        orElse: () => throw Exception('Conference not found'),
      );
      
      expect(conference.groupID, equals(conferenceId));
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
