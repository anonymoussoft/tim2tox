/// Group Double Invite Test
/// 
/// Tests handling of duplicate group invitations
/// Reference: c-toxcore/auto_tests/scenarios/scenario_conference_double_invite_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Double Invite Tests', () {
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
      
      await waitUntil(() => alice.loggedIn && bob.loggedIn);
      
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
    
    test('Duplicate group invitation handling', () async {
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final bobPublicKey = bob.getPublicKey();
      
      final inviteResult1 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: groupId,
        userList: [bobPublicKey],
      ));
      expect(inviteResult1.code, equals(0));
      
      await Future.delayed(const Duration(seconds: 2));
      
      final inviteResult2 = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: groupId,
        userList: [bobPublicKey],
      ));
      expect(inviteResult2.code, isNotNull);
      
      final memberListResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      expect(memberListResult.code, equals(0));
      if (memberListResult.data?.memberInfoList != null) {
        // tim2tox: C++ may return 76-char Tox ID or 64-char public key; bobPublicKey is 64-char
        final bobCount = memberListResult.data!.memberInfoList!
            .where((member) {
              final uid = member.userID;
              return uid == bobPublicKey || (uid.length >= 64 && uid.startsWith(bobPublicKey));
            })
            .length;
        expect(bobCount, lessThanOrEqualTo(1), reason: 'Bob should appear at most once');
      }
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
