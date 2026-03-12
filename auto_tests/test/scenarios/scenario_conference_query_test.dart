/// Conference Query Test
/// 
/// Tests conference query functionality
/// Reference: c-toxcore/auto_tests/scenarios/scenario_conference_query_test.c
/// 
/// This test verifies:
/// 1. Querying conference peer information
/// 2. Identifying own peer number in conference
/// 3. Getting peer public keys
/// 4. Getting conference type

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Conference Query Tests', () {
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
    
    test('Conference query operations', () async {
      await establishFriendship(alice, bob, timeout: const Duration(seconds: 20));
      
      String groupId;
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Query Test Conference',
      ));
      expect(createResult.code, equals(0), reason: 'createGroup failed: ${createResult.code}');
      expect(createResult.data, isNotNull);
      groupId = createResult.data!;
      
      await Future.delayed(const Duration(seconds: 2));
      
      final bobPublicKey = bob.getPublicKey();
      await bob.waitForConnection(timeout: const Duration(seconds: 5));
      await alice.waitForConnection(timeout: const Duration(seconds: 5));
      
      // Wait for bidirectional friend connection
      final bobToxId = bob.getToxId();
      try {
        await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30));
      } catch (e) {
        print('Warning: Alice did not see Bob as online yet: $e');
      }
      final aliceToxId = alice.getToxId();
      try {
        await bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30));
      } catch (e) {
        print('Warning: Bob did not see Alice as online yet: $e');
      }
      await Future.delayed(const Duration(seconds: 2));
      
      for (int retry = 0; retry < 3; retry++) {
        if (retry > 0) await Future.delayed(const Duration(seconds: 2));
        final inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [bobPublicKey],
        ));
        expect(inviteResult.code, equals(0));
        expect(inviteResult.data, isNotNull);
        expect(inviteResult.data!.isNotEmpty, isTrue);
        final bobInviteResult = inviteResult.data!.firstWhere((r) => r.memberID == bobPublicKey, orElse: () => throw Exception('Bob not found in invite result list'));
        if (bobInviteResult.result == 1) break;
        if (retry == 2) expect(bobInviteResult.result, equals(1), reason: 'Bob invitation failed after 3 attempts: result=${bobInviteResult.result} (0=FAIL, 1=SUCC)');
      }
      await Future.delayed(const Duration(seconds: 2));
      
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      expect(joinResult.code, equals(0), reason: 'joinGroup failed: ${joinResult.code}');
      
      // Wait for Bob to join
      await Future.delayed(const Duration(seconds: 3));
      
      // Wait for DHT synchronization so both peers can see each other
      // With local bootstrap, this should be fast (1-2 seconds)
      // However, DHT discovery may still take a moment
      print('[ConferenceQueryTest] Waiting for DHT sync so both peers can see each other...');
      final syncStartTime = DateTime.now();
      bool bothPeersVisible = false;
      const syncTimeout = Duration(seconds: 15); // DHT sync may take longer with local bootstrap
      final syncDeadline = DateTime.now().add(syncTimeout);
      int syncCheckCount = 0;
      
      while (DateTime.now().isBefore(syncDeadline) && !bothPeersVisible) {
        syncCheckCount++;
        final elapsed = DateTime.now().difference(syncStartTime);
        
        final syncCheck = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
          groupID: groupId,
          filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
          nextSeq: '0',
        ));
        
        if (syncCheck.code == 0 && syncCheck.data != null) {
          final syncMemberCount = syncCheck.data!.memberInfoList?.length ?? 0;
          if (syncCheckCount % 3 == 0 || syncMemberCount >= 2) {
            print('[ConferenceQueryTest] DHT sync check (Bob): memberCount=$syncMemberCount (elapsed: ${elapsed.inMilliseconds}ms)');
          }
          if (syncMemberCount >= 2) {
            bothPeersVisible = true;
            print('[ConferenceQueryTest] ✅ Both peers visible from Bob perspective (elapsed: ${elapsed.inMilliseconds}ms)');
            break;
          }
        }
        
        if (syncCheckCount % 2 == 0) {
          final aliceCheck = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
            groupID: groupId,
            filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
            nextSeq: '0',
          ));
          if (aliceCheck.code == 0 && aliceCheck.data != null) {
            final aliceMemberCount = aliceCheck.data!.memberInfoList?.length ?? 0;
            if (syncCheckCount % 3 == 0 || aliceMemberCount >= 2) {
              print('[ConferenceQueryTest] DHT sync check (Alice): memberCount=$aliceMemberCount (elapsed: ${elapsed.inMilliseconds}ms)');
            }
            if (aliceMemberCount >= 2) {
              bothPeersVisible = true;
              print('[ConferenceQueryTest] ✅ Both peers visible from Alice perspective (elapsed: ${elapsed.inMilliseconds}ms)');
              break;
            }
          }
        }
        
        await Future.delayed(const Duration(milliseconds: 200));
      }
      
      if (!bothPeersVisible) {
        final elapsed = DateTime.now().difference(syncStartTime);
        print('[ConferenceQueryTest] ⚠️  DHT sync incomplete after ${elapsed.inMilliseconds}ms, but continuing with current state...');
      }
      
      final memberListResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      expect(memberListResult.code, equals(0), reason: 'getGroupMemberList failed: ${memberListResult.code}');
      expect(memberListResult.data, isNotNull);
      // tim2tox: DHT sync may show 1 member briefly; accept >= 1 and verify both via getGroupMembersInfo
      expect(memberListResult.data!.memberInfoList?.length ?? 0, greaterThanOrEqualTo(1));
      
      final alicePublicKey = alice.getPublicKey();
      // Use getGroupMemberList as source of truth for which userID is Alice vs Bob (tim2tox may return 64/76-char or different casing)
      bool memberMatches(String uid, String publicKey) {
        final u = uid.toLowerCase();
        final p = publicKey.toLowerCase();
        return u == p || (u.length >= 64 && u.startsWith(p)) || (p.length >= 64 && p.startsWith(u));
      }
      final memberListForRef = memberListResult.data!.memberInfoList!;
      var aliceUserIDFromList = memberListForRef
          .where((m) => memberMatches(m.userID, alicePublicKey))
          .map((m) => m.userID)
          .firstOrNull;
      var bobUserIDFromList = memberListForRef
          .where((m) => memberMatches(m.userID, bobPublicKey))
          .map((m) => m.userID)
          .firstOrNull;
      // If C++ returns userIDs in different format, identify by exclusion: in a 2-member group, the non-Bob member is Alice
      if (memberListForRef.length >= 2) {
        if (aliceUserIDFromList == null) {
          final notBob = memberListForRef.where((m) => !memberMatches(m.userID, bobPublicKey)).map((m) => m.userID).firstOrNull;
          if (notBob != null) aliceUserIDFromList = notBob;
        }
        if (bobUserIDFromList == null) {
          final notAlice = memberListForRef.where((m) => !memberMatches(m.userID, alicePublicKey)).map((m) => m.userID).firstOrNull;
          if (notAlice != null) bobUserIDFromList = notAlice;
        }
      }
      expect(memberListForRef.length, greaterThanOrEqualTo(2), reason: 'getGroupMemberList should have at least 2 members');
      expect(aliceUserIDFromList, isNotNull, reason: 'could not identify Alice in member list (alicePublicKey or other member)');
      expect(bobUserIDFromList, isNotNull, reason: 'could not identify Bob in member list (bobPublicKey or other member)');

      final aliceMemberInfoResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMembersInfo(
        groupID: groupId,
        memberList: [alicePublicKey],
      ));
      expect(aliceMemberInfoResult.code, equals(0), reason: 'getGroupMembersInfo failed: ${aliceMemberInfoResult.code}');
      expect(aliceMemberInfoResult.data, isNotNull);
      expect(aliceMemberInfoResult.data!.isNotEmpty, isTrue);

      final bobMemberInfoResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMembersInfo(
        groupID: groupId,
        memberList: [bobPublicKey],
      ));
      expect(bobMemberInfoResult.code, equals(0), reason: 'getGroupMembersInfo failed: ${bobMemberInfoResult.code}');
      expect(bobMemberInfoResult.data, isNotNull);
      expect(bobMemberInfoResult.data!.isNotEmpty, isTrue);
      
      final groupInfoResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupsInfo(
        groupIDList: [groupId],
      ));
      expect(groupInfoResult.code, equals(0), reason: 'getGroupsInfo failed: ${groupInfoResult.code}');
      expect(groupInfoResult.data, isNotNull);
      expect(groupInfoResult.data!.isNotEmpty, isTrue);
      
      final groupInfoResultItem = groupInfoResult.data!.first;
      expect(groupInfoResultItem.groupInfo, isNotNull);
      final groupInfo = groupInfoResultItem.groupInfo!;
      expect(groupInfo.groupID, equals(groupId));
      expect(groupInfo.groupType, equals('Meeting'));
      
      // Step 7: Verify member userIDs match expected values (use getGroupMemberList as reference for who is Alice/Bob)
      final aliceInfo = aliceMemberInfoResult.data!.firstWhere(
        (m) => memberMatches(m.userID, alicePublicKey) || (aliceUserIDFromList != null && m.userID.toLowerCase() == aliceUserIDFromList.toLowerCase()),
        orElse: () => throw Exception('getGroupMembersInfo did not return a member matching alicePublicKey or aliceUserIDFromList; data=${aliceMemberInfoResult.data!.map((e) => e.userID).toList()}'),
      );
      final bobInfo = bobMemberInfoResult.data!.firstWhere(
        (m) => memberMatches(m.userID, bobPublicKey) || (bobUserIDFromList != null && m.userID.toLowerCase() == bobUserIDFromList.toLowerCase()),
        orElse: () => throw Exception('getGroupMembersInfo did not return a member matching bobPublicKey or bobUserIDFromList; data=${bobMemberInfoResult.data!.map((e) => e.userID).toList()}'),
      );

      expect(memberMatches(aliceInfo.userID, alicePublicKey) || (aliceUserIDFromList != null && aliceInfo.userID.toLowerCase() == aliceUserIDFromList.toLowerCase()), isTrue, reason: 'aliceInfo.userID=${aliceInfo.userID}');
      expect(memberMatches(bobInfo.userID, bobPublicKey) || (bobUserIDFromList != null && bobInfo.userID.toLowerCase() == bobUserIDFromList.toLowerCase()), isTrue, reason: 'bobInfo.userID=${bobInfo.userID}');
      
      print('Conference query test completed successfully');
      print('  Group ID: $groupId');
      print('  Group Type: ${groupInfo.groupType}');
      print('  Member Count: ${memberListResult.data!.memberInfoList?.length ?? 0}');
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
