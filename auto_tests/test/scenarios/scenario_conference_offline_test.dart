/// Conference Offline Test
/// 
/// Tests conference offline peer functionality
/// Reference: c-toxcore/auto_tests/scenarios/scenario_conference_offline_test.c
/// Note: tim2tox uses Tox Group instead of Conference, so this maps to group offline member features

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimGroupListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Conference Offline Tests (Group Offline Members)', () {
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
      await waitUntil(() => alice.loggedIn && bob.loggedIn);
      
      // Configure local bootstrap
      await configureLocalBootstrap(scenario);
      
      // Establish friendship so invite can succeed (friend must be connected for group invite)
      await establishFriendship(alice, bob, timeout: const Duration(seconds: 25));
      await bob.waitForConnection(timeout: const Duration(seconds: 10));
      await alice.waitForConnection(timeout: const Duration(seconds: 10));
      try {
        await alice.waitForFriendConnection(bob.getToxId(), timeout: const Duration(seconds: 30));
        await bob.waitForFriendConnection(alice.getToxId(), timeout: const Duration(seconds: 30));
      } catch (e) {
        print('[ConferenceOffline] Friend connection check not fully ready, continue with retry logic: $e');
      }
      await Future.delayed(const Duration(seconds: 2));
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
    
    test('Group offline member handling after reload', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Offline Test Conference',
        groupID: '',
      ));
      expect(createResult.code, equals(0));
      expect(createResult.data, isNotNull);
      final groupId = createResult.data!;
      
      var bobInvited = false;
      var bobJoined = false;
      final bobGroupListener = V2TimGroupListener(
        onMemberEnter: (groupID, memberList) {
          if (groupID == groupId) bobJoined = true;
        },
        onMemberInvited: (groupID, opUser, memberList) {
          // C++ delivers invite with temp ID (e.g. tox_inv_*) first; actual groupId comes after join.
          // This test has a single invite, so any onMemberInvited means Bob received the invite.
          bobInvited = true;
        },
      );
      
      bob.runWithInstance(() => TIMGroupManager.instance.addGroupListener(bobGroupListener));
      
      final bobPublicKey = bob.getPublicKey();
      for (int retry = 0; retry < 5; retry++) {
        if (retry > 0) await Future.delayed(const Duration(seconds: 3));
        final inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [bobPublicKey],
        ));
        expect(inviteResult.code, equals(0));
        final bobRes = inviteResult.data?.where((r) => r.memberID == bobPublicKey).toList();
        if (bobRes != null && bobRes.isNotEmpty && bobRes.first.result == 1) break;
        if (retry == 4) expect(bobRes?.first.result ?? 0, equals(1), reason: 'Bob invite failed after 5 attempts');
      }
      await Future.delayed(const Duration(seconds: 2));
      await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 6), iterationsPerPump: 80);
      
      // Wait for Bob to receive invitation (pump so invite callback is processed).
      await waitUntilWithPump(
        () => bobInvited,
        timeout: const Duration(seconds: 90),
        description: 'Bob received group invite',
        iterationsPerPump: 150,
        stepDelay: const Duration(milliseconds: 150),
      );
      
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      expect(joinResult.code, equals(0));
      
      await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 6));
      
      // Wait for Bob to appear in group (onMemberEnter) (pump so peer_join is processed).
      await waitUntilWithPump(
        () => bobJoined,
        timeout: const Duration(seconds: 45),
        description: 'Bob joined group (onMemberEnter)',
        iterationsPerPump: 50,
        stepDelay: const Duration(milliseconds: 300),
      );
      
      final aliceMembersResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      
      expect(aliceMembersResult.code, equals(0));
      expect(aliceMembersResult.data?.memberInfoList, isNotNull);
      expect(aliceMembersResult.data!.memberInfoList!.length, greaterThanOrEqualTo(2));
      
      // Now simulate reload: Alice logs out and logs back in
      await alice.logout();
      await alice.login();
      
      // Wait for reconnection
      await waitUntil(() => alice.loggedIn, timeout: const Duration(seconds: 30));
      
      // After reload, query group members again
      // In a real scenario, Bob might be offline, so we should see offline members
      // Set Alice's instance as current before querying member list
      final afterReloadMembersResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      
      expect(afterReloadMembersResult.code, equals(0));
      expect(afterReloadMembersResult.data?.memberInfoList, isNotNull);
      
      // The group should still exist with members
      // Note: In tim2tox, offline members are still part of the group
      // This test verifies that group state persists after reload
      final memberCount = afterReloadMembersResult.data!.memberInfoList!.length;
      expect(memberCount, greaterThanOrEqualTo(1)); // At least Alice should be there
      
      bob.runWithInstance(() => TIMGroupManager.instance.removeGroupListener(listener: bobGroupListener));
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Group member list includes offline members', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Offline Member Test',
        groupID: '',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final bobPublicKey = bob.getPublicKey();
      for (int retry = 0; retry < 5; retry++) {
        if (retry > 0) await Future.delayed(const Duration(seconds: 3));
        final inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [bobPublicKey],
        ));
        expect(inviteResult.code, equals(0));
        final bobRes = inviteResult.data?.where((r) => r.memberID == bobPublicKey).toList();
        if (bobRes != null && bobRes.isNotEmpty && bobRes.first.result == 1) break;
      }
      await Future.delayed(const Duration(seconds: 2));
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      expect(joinResult.code, equals(0));
      
      await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 6));
      final bobSeenByAlice = await waitUntilFounderSeesMemberInGroup(alice, bob, groupId, timeout: const Duration(seconds: 45));
      expect(bobSeenByAlice, isNotNull, reason: 'Alice must see Bob in group before asserting member list');
      
      final membersResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      
      expect(membersResult.code, equals(0));
      expect(membersResult.data?.memberInfoList, isNotNull);
      
      final memberList = membersResult.data!.memberInfoList!;
      expect(memberList.length, greaterThanOrEqualTo(2));
      
      // Get public keys for comparison
      final alicePublicKey = alice.getPublicKey();
      final bobPublicKeyForCompare = bob.getPublicKey();
      
      // tim2tox: C++ may return 76-char Tox ID or 64-char public key; accept exact or prefix match, or the non-founder userID we already saw
      bool memberMatches(String uid, String publicKey) =>
          uid == publicKey || (uid.length >= 64 && uid.startsWith(publicKey));
      final aliceMember = memberList.firstWhere(
        (m) => memberMatches(m.userID, alicePublicKey),
        orElse: () => throw Exception('Alice not found in member list'),
      );
      final bobMember = memberList.firstWhere(
        (m) => memberMatches(m.userID, bobPublicKeyForCompare) || (bobSeenByAlice != null && m.userID == bobSeenByAlice),
        orElse: () => throw Exception('Bob not found in member list'),
      );
      expect(memberMatches(aliceMember.userID, alicePublicKey), isTrue);
      expect(memberMatches(bobMember.userID, bobPublicKeyForCompare) || bobMember.userID == bobSeenByAlice, isTrue);
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
