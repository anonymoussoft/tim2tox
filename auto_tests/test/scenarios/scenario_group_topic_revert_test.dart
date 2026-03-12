/// Group Topic Revert Test
/// Reference: c-toxcore/auto_tests/scenarios/scenario_group_topic_revert_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_group_info.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Topic Revert Tests', () {
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
    
    test('Rapid topic changes', () async {
      // Create group
      final createResult = await TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      );
      
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      // Rapid topic changes
      final groupInfo1 = V2TimGroupInfo(
        groupID: groupId,
        groupType: 'kTIMGroup_Private',
        introduction: 'Topic 1',
      );
      final groupInfo2 = V2TimGroupInfo(
        groupID: groupId,
        groupType: 'kTIMGroup_Private',
        introduction: 'Topic 2',
      );
      final groupInfo3 = V2TimGroupInfo(
        groupID: groupId,
        groupType: 'kTIMGroup_Private',
        introduction: 'Topic 3',
      );
      await TIMGroupManager.instance.setGroupInfo(info: groupInfo1);
      await TIMGroupManager.instance.setGroupInfo(info: groupInfo2);
      await TIMGroupManager.instance.setGroupInfo(info: groupInfo3);
      
      await Future.delayed(const Duration(seconds: 2));
      final groupsInfoResult = await TIMGroupManager.instance.getGroupsInfo(groupIDList: [groupId]);
      expect(groupsInfoResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
