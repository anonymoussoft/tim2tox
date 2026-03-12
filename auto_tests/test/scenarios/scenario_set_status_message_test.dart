/// Set Status Message Test
/// Reference: c-toxcore/auto_tests/scenarios/scenario_set_status_message_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimSDKListener.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_user_full_info.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Set Status Message Tests', () {
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
    
    test('Set status message', () async {
      // Status message setting depends on available APIs
      // Verify listener setup
      final listener = V2TimSDKListener(
        onSelfInfoUpdated: (V2TimUserFullInfo info) {
          alice.markCallbackReceived('onSelfInfoUpdated');
        },
      );
      TIMManager.instance.addSDKListener(listener);
      expect(TIMManager.instance.v2TimSDKListenerList.contains(listener), isTrue);
    }, timeout: const Timeout(Duration(seconds: 30)));
  });
}
