/// Group Save Test
/// 
/// Tests group state saving and loading
/// Reference: c-toxcore/auto_tests/scenarios/scenario_group_save_test.c

import 'package:path/path.dart' as path;
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Save Tests', () {
    late TestScenario scenario;
    late TestNode node;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['alice', 'bob']);
      node = scenario.getNode('alice')!;
      await scenario.initAllNodes();
      await Future.wait([node.login(), scenario.getNode('bob')!.login()]);
      await waitUntil(
        () => node.loggedIn && scenario.getNode('bob')!.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'nodes logged in',
      );
      await configureLocalBootstrap(scenario);
      await node.waitForConnection(timeout: const Duration(seconds: 15));
    });
    
    tearDownAll(() async {
      await scenario.dispose();
      await teardownTestEnvironment();
    });
    
    setUp(() async {});
    
    test('Group state persistence', () async {
      final testDataDir = await getTestDataDir();
      final dataDir = path.join(testDataDir, node.userId, 'init');
      
      // Node is already initialized and logged in from setUpAll
      // Create and join a group on this node's instance
      final createResult = await node.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Test Group',
      ));
      
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      expect(groupId, isNotEmpty);
      
      // Wait for group state to be persisted before logout
      await Future.delayed(const Duration(seconds: 2));
      
      // Save state (logout/uninit)
      await node.logout();
      await node.unInitSDK();
      
      // Reload state
      await node.initSDK(initPath: dataDir);
      await node.login();
      
      // Verify group is still accessible - poll with longer timeout (group load may be async)
      const pollInterval = Duration(seconds: 2);
      const totalWait = Duration(seconds: 30);
      final deadline = DateTime.now().add(totalWait);
      dynamic groupListResult;
      while (DateTime.now().isBefore(deadline)) {
        groupListResult = await node.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
        if (groupListResult.code == 0 &&
            groupListResult.data != null &&
            groupListResult.data.any((g) => g.groupID == groupId)) {
          break;
        }
        await Future.delayed(pollInterval);
      }
      expect(groupListResult.code, equals(0));
      expect(groupListResult.data, isNotNull);
      expect(groupListResult.data.any((g) => g.groupID == groupId), isTrue,
          reason: 'Group should be in list after reload');
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
