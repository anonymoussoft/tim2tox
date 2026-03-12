/// Login/Logout Test
/// 
/// Tests login, logout, and login state verification
/// Reference: c-toxcore auto_tests patterns

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Login/Logout Tests', () {
    late TestNode node;
    
    setUpAll(() async {
      await setupTestEnvironment();
      node = await createTestNode('test_node');
      await node.initSDK();
    });
    
    tearDownAll(() async {
      await node.dispose();
      await teardownTestEnvironment();
    });
    
    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      // Ensure node is logged out before each test (except first test which expects logged out state)
      // This ensures test isolation
      if (node.loggedIn) {
        await node.logout();
      }
    });
    
    test('Normal login flow', () async {
      await node.login();
      expect(node.loggedIn, isTrue);
      
      // In tim2tox, getLoginUser() returns Tox ID (76 hex characters), not the original userId.
      // Must use node context: getLoginUser() is per-instance.
      final loginUser = node.runWithInstance(() => TIMManager.instance.getLoginUser());
      expect(loginUser.isNotEmpty, isTrue);
      expect(loginUser.length, equals(76)); // Tox ID is 76 hex characters
      expect(loginUser, matches(RegExp(r'^[0-9A-F]{76}$'))); // Valid Tox ID format
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Login state verification', () async {
      expect(node.loggedIn, isFalse);
      
      await node.login();
      expect(node.loggedIn, isTrue);
      
      // In tim2tox, getLoginUser() returns Tox ID. Must use node context (per-instance).
      final loginUser = node.runWithInstance(() => TIMManager.instance.getLoginUser());
      expect(loginUser.isNotEmpty, isTrue);
      expect(loginUser.length, equals(76)); // Tox ID is 76 hex characters
      expect(loginUser, matches(RegExp(r'^[0-9A-F]{76}$'))); // Valid Tox ID format
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Logout flow', () async {
      await node.login();
      expect(node.loggedIn, isTrue);
      
      await node.logout();
      expect(node.loggedIn, isFalse);
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Repeated login handling', () async {
      await node.login();
      expect(node.loggedIn, isTrue);
      
      // Try to login again
      await node.login();
      expect(node.loggedIn, isTrue);
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Login after logout', () async {
      await node.login();
      await node.logout();
      expect(node.loggedIn, isFalse);
      
      await node.login();
      expect(node.loggedIn, isTrue);
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
