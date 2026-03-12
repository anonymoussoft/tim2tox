/// Group Moderation Test
/// 
/// Tests group moderation: role setting, kick member, mute member, transfer owner
/// Reference: c-toxcore/auto_tests/scenarios/scenario_group_moderation_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_role_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_callback.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Moderation Tests', () {
    late TestScenario scenario;
    late TestNode founder;
    late TestNode member1;
    late TestNode member2;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['founder', 'member1', 'member2']);
      founder = scenario.getNode('founder')!;
      member1 = scenario.getNode('member1')!;
      member2 = scenario.getNode('member2')!;
      
      await scenario.initAllNodes();
      // Parallelize login
      await Future.wait([
        founder.login(),
        member1.login(),
        member2.login(),
      ]);
      
      await waitUntil(() => founder.loggedIn && member1.loggedIn && member2.loggedIn);
      
      // Configure local bootstrap (like C test's tox_node_bootstrap)
      await configureLocalBootstrap(scenario);
      // Bootstrap and friendship for DHT/connectivity (PUBLIC groups use DHT/announce for peer discovery)
      await establishFriendship(founder, member1);
      await establishFriendship(founder, member2);
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
    
    // Use PUBLIC group (like C scenario_group_moderation_test.c) so DHT/announce peer discovery works
    test('Set group member role', () async {
      final createResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Public',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;

      final joinResult = await member1.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      expect(joinResult.code, equals(0), reason: joinResult.code != 0 ? 'member1 joinGroup failed: ${joinResult.desc}' : null);
      await pumpGroupPeerDiscovery(
        founder,
        member1,
        duration: const Duration(seconds: 15),
        iterationsPerPump: 80,
        stepDelay: const Duration(milliseconds: 60),
      );
      await Future.delayed(const Duration(seconds: 3));
      final member1UserIDInGroup = await waitUntilFounderSeesMemberInGroup(
        founder,
        member1,
        groupId,
        timeout: const Duration(seconds: 90),
      );
      expect(member1UserIDInGroup, isNotNull, reason: 'Founder did not see member1 in group');
      // Retry setGroupMemberRole: Tox may list the peer slightly after getGroupMemberList sees it
      V2TimCallback? setRoleResult;
      for (var attempt = 0; attempt < 5; attempt++) {
        setRoleResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.setGroupMemberRole(
          groupID: groupId,
          userID: member1UserIDInGroup!,
          role: GroupMemberRoleTypeEnum.V2TIM_GROUP_MEMBER_ROLE_ADMIN,
        ));
        if (setRoleResult?.code == 0) break;
        await Future.delayed(const Duration(seconds: 3));
      }
      expect(setRoleResult?.code, equals(0), reason: setRoleResult?.code != 0 ? '${setRoleResult?.desc}' : null);
    }, timeout: const Timeout(Duration(seconds: 180)));
    
    test('Kick group member', () async {
      final createResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Public',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      await member1.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      await pumpGroupPeerDiscovery(
        founder,
        member1,
        duration: const Duration(seconds: 15),
        iterationsPerPump: 80,
        stepDelay: const Duration(milliseconds: 60),
      );
      await Future.delayed(const Duration(seconds: 3));
      final member1UserIDInGroup = await waitUntilFounderSeesMemberInGroup(
        founder,
        member1,
        groupId,
        timeout: const Duration(seconds: 90),
      );
      expect(member1UserIDInGroup, isNotNull, reason: 'Founder did not see member1 in group for kick');
      final kickResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.kickGroupMember(
        groupID: groupId,
        memberList: [member1UserIDInGroup!],
      ));
      expect(kickResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 180)));
    
    test('Mute group member', () async {
      final createResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Public',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      await member1.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      await pumpGroupPeerDiscovery(
        founder,
        member1,
        duration: const Duration(seconds: 15),
        iterationsPerPump: 80,
        stepDelay: const Duration(milliseconds: 60),
      );
      await Future.delayed(const Duration(seconds: 3));
      final member1UserIDInGroup = await waitUntilFounderSeesMemberInGroup(
        founder,
        member1,
        groupId,
        timeout: const Duration(seconds: 90),
      );
      expect(member1UserIDInGroup, isNotNull, reason: 'Founder did not see member1 in group for mute');
      final muteResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.muteGroupMember(
        groupID: groupId,
        userID: member1UserIDInGroup!,
        seconds: 3600,
      ));
      expect(muteResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 180)));
    
    test('Transfer group owner', () async {
      final createResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Public',
        groupName: 'Test Group',
      ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      await member1.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      await pumpGroupPeerDiscovery(
        founder,
        member1,
        duration: const Duration(seconds: 15),
        iterationsPerPump: 80,
        stepDelay: const Duration(milliseconds: 60),
      );
      await Future.delayed(const Duration(seconds: 3));
      final member1UserIDInGroup = await waitUntilFounderSeesMemberInGroup(
        founder,
        member1,
        groupId,
        timeout: const Duration(seconds: 90),
      );
      expect(member1UserIDInGroup, isNotNull, reason: 'Founder did not see member1 in group for transfer');
      final transferResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.transferGroupOwner(
        groupID: groupId,
        userID: member1UserIDInGroup!,
      ));
      expect(transferResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 180)));
  });
}
