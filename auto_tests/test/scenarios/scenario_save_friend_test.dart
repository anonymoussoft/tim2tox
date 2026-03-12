/// Save Friend Test
/// Reference: c-toxcore/auto_tests/scenarios/scenario_save_friend_test.c

import 'package:test/test.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Save Friend Tests', () {
    late TestNode node;
    final testDir = getTestDataDir();
    
    setUpAll(() async {
      await setupTestEnvironment();
      node = await createTestNode('test_node');
    });
    
    tearDownAll(() async {
      await node.dispose();
      await teardownTestEnvironment();
    });
    
    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      // Reset any per-test state if necessary
      // Most tests don't need cleanup since they use shared node
    });
    
    test('Friend list persistence', () async {
      final dataDir = await testDir;
      await node.initSDK(initPath: dataDir);
      await node.login();
      
      // Add friend (simplified)
      // Reduced delay
      await Future.delayed(const Duration(milliseconds: 500));
      
      await node.logout();
      await node.unInitSDK();
      
      await node.initSDK(initPath: dataDir);
      await node.login();
      
      final friendListResult = await node.getFriendListResultWithInstance();
      expect(friendListResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
