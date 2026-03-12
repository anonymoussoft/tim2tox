/// Custom Callback Handler Tests (Binary Replacement Path)
///
/// Tests the customCallbackHandler mechanism added to NativeLibraryManager.
/// Verifies that tim2tox-specific callbacks (clearHistoryMessage, groupQuitNotification,
/// groupChatIdStored) are correctly routed through the generic hook to Tim2ToxSdkPlatform.

import 'dart:async';
import 'dart:convert';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/bindings/native_library_manager.dart';
import 'package:tim2tox_dart/ffi/tim2tox_ffi.dart' as ffi_lib;
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Custom Callback Handler Tests', () {
    late TestNode node;
    late ffi_lib.Tim2ToxFfi ffiInstance;

    setUpAll(() async {
      await setupTestEnvironment();
      setupNativeLibraryForTim2Tox();
      node = await createTestNode('custom_cb_node');
      await node.initSDK();
      ffiInstance = ffi_lib.Tim2ToxFfi.open();
    });

    tearDownAll(() async {
      // Restore customCallbackHandler to avoid side effects
      NativeLibraryManager.customCallbackHandler = null;
      await node.dispose();
      await teardownTestEnvironment();
    });

    test('customCallbackHandler receives unknown callback types', () async {
      final completer = Completer<Map<String, dynamic>>();

      NativeLibraryManager.customCallbackHandler = (callbackName, data, apiCallbackMap) async {
        if (!completer.isCompleted) {
          completer.complete({'name': callbackName, 'data': data});
        }
      };

      // Inject a custom callback that is not apiCallback or globalCallback
      final json = jsonEncode({
        "callback": "myCustomEvent",
        "some_field": "some_value",
        "number_field": 42,
      });
      final result = ffiInstance.injectCallback(json);
      expect(result, equals(1));

      final received = await completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () => fail('customCallbackHandler was not called within timeout'),
      );
      expect(received['name'], equals('myCustomEvent'));
      expect((received['data'] as Map)['some_field'], equals('some_value'));
      expect((received['data'] as Map)['number_field'], equals(42));
    }, timeout: const Timeout(Duration(seconds: 10)));

    test('clearHistoryMessage callback is routed to customCallbackHandler', () async {
      final completer = Completer<String>();

      NativeLibraryManager.customCallbackHandler = (callbackName, data, apiCallbackMap) async {
        if (callbackName == 'clearHistoryMessage' && !completer.isCompleted) {
          completer.complete('${data["conv_id"]}:${data["conv_type"]}');
        }
      };

      final json = jsonEncode({
        "callback": "clearHistoryMessage",
        "conv_id": "user_bob",
        "conv_type": "1",
        "user_data": "clear_test_001",
      });
      ffiInstance.injectCallback(json);

      final result = await completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () => fail('clearHistoryMessage callback was not received'),
      );
      expect(result, equals('user_bob:1'));
    }, timeout: const Timeout(Duration(seconds: 10)));

    test('groupQuitNotification callback is routed to customCallbackHandler', () async {
      final completer = Completer<String>();

      NativeLibraryManager.customCallbackHandler = (callbackName, data, apiCallbackMap) async {
        if (callbackName == 'groupQuitNotification' && !completer.isCompleted) {
          completer.complete(data["group_id"] as String);
        }
      };

      final json = jsonEncode({
        "callback": "groupQuitNotification",
        "group_id": "group_test_123",
      });
      ffiInstance.injectCallback(json);

      final groupId = await completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () => fail('groupQuitNotification callback was not received'),
      );
      expect(groupId, equals('group_test_123'));
    }, timeout: const Timeout(Duration(seconds: 10)));

    test('groupChatIdStored callback is routed to customCallbackHandler', () async {
      final completer = Completer<Map<String, String>>();

      NativeLibraryManager.customCallbackHandler = (callbackName, data, apiCallbackMap) async {
        if (callbackName == 'groupChatIdStored' && !completer.isCompleted) {
          completer.complete({
            'group_id': data["group_id"] as String,
            'chat_id': data["chat_id"] as String,
          });
        }
      };

      final json = jsonEncode({
        "callback": "groupChatIdStored",
        "group_id": "group_abc",
        "chat_id": "AABBCCDD" * 8, // 64 char hex
      });
      ffiInstance.injectCallback(json);

      final result = await completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () => fail('groupChatIdStored callback was not received'),
      );
      expect(result['group_id'], equals('group_abc'));
      expect(result['chat_id']!.length, equals(64));
    }, timeout: const Timeout(Duration(seconds: 10)));

    test('apiCallbackMap is passed to customCallbackHandler', () async {
      final completer = Completer<bool>();
      const probeKey = '__probe__';

      NativeLibraryManager.customCallbackHandler = (callbackName, data, apiCallbackMap) async {
        // Verify the callback receives a mutable map reference.
        if (!completer.isCompleted) {
          apiCallbackMap[probeKey] = (_) {};
          completer.complete(apiCallbackMap.containsKey(probeKey));
        }
      };

      final json = jsonEncode({
        "callback": "testApiMapAccess",
        "data": "test",
      });
      ffiInstance.injectCallback(json);

      final hasMap = await completer.future.timeout(
        const Duration(seconds: 5),
        onTimeout: () => fail('customCallbackHandler was not called'),
      );
      expect(hasMap, isTrue);
    }, timeout: const Timeout(Duration(seconds: 10)));

    test('null customCallbackHandler does not crash on unknown callbacks', () async {
      NativeLibraryManager.customCallbackHandler = null;

      // This should not throw - it should silently ignore the callback
      final json = jsonEncode({
        "callback": "unknownCallback",
        "data": "should_be_ignored",
      });
      final result = ffiInstance.injectCallback(json);
      expect(result, equals(1));

      // Give it a moment to process
      await Future.delayed(const Duration(milliseconds: 200));
      // If we get here without crash, the test passes
    }, timeout: const Timeout(Duration(seconds: 10)));
  });
}
