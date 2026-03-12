/// ToxAV Basic Test
/// 
/// Tests basic audio/video call functionality
/// Reference: c-toxcore/auto_tests/scenarios/scenario_toxav_basic_test.c
/// 
/// Tests:
/// 1. Regular AV call - Alice calls, Bob answers, Bob hangs up
/// 2. Reject flow - Alice calls, Bob rejects
/// 3. Cancel flow - Alice calls, Alice cancels

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/friend_type_enum.dart';
import 'package:tim2tox_dart/ffi/tim2tox_ffi.dart' as ffi_lib;
import 'package:tim2tox_dart/service/toxav_service.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('ToxAV Basic Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late TestNode bob;
    late ToxAVService aliceAV;
    late ToxAVService bobAV;
    int _toxavTestsCompleted = 0;
    
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
      
      // Wait for both nodes to be connected - optimized timeout for local bootstrap
      await waitUntil(() => alice.loggedIn && bob.loggedIn,
        timeout: const Duration(seconds: 10),
      );
      
      await configureLocalBootstrap(scenario);
      
      // Wait for connection to be established so Tox IDs are available
      // logged_in_user_ is set when connection is established (HandleSelfConnectionStatus)
      // Parallelize connection waiting
      print('[Test] setUp - Waiting for connections to establish...');
      await Future.wait([
        alice.waitForConnection(timeout: const Duration(seconds: 10)),
        bob.waitForConnection(timeout: const Duration(seconds: 10)),
      ]);
      
      // Get actual Tox IDs (not test identifiers)
      // Wait for Tox IDs to be available (logged_in_user_ is set after connection)
      print('[Test] setUp - Waiting for Tox IDs to be available...');
      await waitUntil(
        () {
          final aliceToxId = alice.getToxId();
          final bobToxId = bob.getToxId();
          return aliceToxId.isNotEmpty && aliceToxId.length == 76 && 
                 bobToxId.isNotEmpty && bobToxId.length == 76;
        },
        timeout: const Duration(seconds: 10),
      );
      
      final aliceToxId = alice.getToxId();
      final bobToxId = bob.getToxId();
      
      print('[Test] setUp - Alice Tox ID: $aliceToxId (length=${aliceToxId.length})');
      print('[Test] setUp - Bob Tox ID: $bobToxId (length=${bobToxId.length})');
      
      if (aliceToxId.isEmpty || aliceToxId.length != 76) {
        throw Exception('Invalid Alice Tox ID: $aliceToxId (expected 76 hex chars)');
      }
      
      if (bobToxId.isEmpty || bobToxId.length != 76) {
        throw Exception('Invalid Bob Tox ID: $bobToxId (expected 76 hex chars)');
      }
      
      // Add friends using actual Tox IDs
      print('[Test] setUp - Adding friends using actual Tox IDs...');
      await alice.runWithInstanceAsync(() async => TIMFriendshipManager.instance.addFriend(
        userID: bobToxId,
        addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
        remark: 'Bob',
        addWording: 'test',
      ));
      print('[Test] setUp - Alice added Bob as friend');
      
      await bob.runWithInstanceAsync(() async => TIMFriendshipManager.instance.addFriend(
        userID: aliceToxId,
        addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
        remark: 'Alice',
        addWording: 'test',
      ));
      print('[Test] setUp - Bob added Alice as friend');
      
      // Friend requests may need time to propagate/auto-accept over the local DHT
      print('[Test] setUp - Waiting for friend list to populate...');
      await Future.delayed(const Duration(seconds: 5));
      final alicePub = alice.getPublicKey();
      final bobPub = bob.getPublicKey();
      await waitForFriendsInList(alice, [bobPub], timeout: const Duration(seconds: 120));
      await waitForFriendsInList(bob, [alicePub], timeout: const Duration(seconds: 120));
      print('[Test] setUp - Friend list ready, waiting for connections...');
      
      // Create and initialize ToxAV for each node inside instance context so they register with correct instance_id (1=alice, 2=bob)
      final ffi = ffi_lib.Tim2ToxFfi.open();
      final aliceInit = await alice.runWithInstanceAsync(() async {
        aliceAV = ToxAVService(ffi);
        return aliceAV.initialize();
      });
      final bobInit = await bob.runWithInstanceAsync(() async {
        bobAV = ToxAVService(ffi);
        return bobAV.initialize();
      });
      
      expect(aliceInit, isTrue, reason: 'Alice ToxAV initialization failed');
      expect(bobInit, isTrue, reason: 'Bob ToxAV initialization failed');
    });
    
    tearDownAll(() async {
      aliceAV.shutdown();
      bobAV.shutdown();
      await scenario.dispose();
      await teardownTestEnvironment();
    });
    
    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      // Only clear call state before 2nd and 3rd tests (after first test has left a call)
      if (_toxavTestsCompleted == 0) return;
      final bobToxId = bob.getToxId();
      final aliceToxId = alice.getToxId();
      if (bobToxId.isEmpty || aliceToxId.isEmpty || bobToxId.length != 76 || aliceToxId.length != 76) {
        return;
      }
      final bobFriendNumber = alice.runWithInstance(() => aliceAV.getFriendNumberByUserId(bobToxId));
      final aliceFriendNumber = bob.runWithInstance(() => bobAV.getFriendNumberByUserId(aliceToxId));
      if (bobFriendNumber == 0xFFFFFFFF || aliceFriendNumber == 0xFFFFFFFF) {
        return;
      }
      await alice.runWithInstanceAsync(() async => aliceAV.endCall(bobFriendNumber));
      await bob.runWithInstanceAsync(() async => bobAV.endCall(aliceFriendNumber));
      await Future.delayed(const Duration(milliseconds: 1200));
      final ffi = ffi_lib.Tim2ToxFfi.open();
      for (int i = 0; i < 30; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        bob.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
    });
    
    test('Regular AV call - answer and hang up', () async {
      // Get actual Tox IDs (not test identifiers)
      final bobToxId = bob.getToxId();
      final aliceToxId = alice.getToxId();
      print('[Test] Alice Tox ID: $aliceToxId');
      print('[Test] Bob Tox ID: $bobToxId');
      
      // Get friend numbers using actual Tox IDs
      final bobFriendNumber = alice.runWithInstance(() => aliceAV.getFriendNumberByUserId(bobToxId));
      final aliceFriendNumber = bob.runWithInstance(() => bobAV.getFriendNumberByUserId(aliceToxId));
      
      print('[Test] Alice sees Bob as friend number: $bobFriendNumber');
      print('[Test] Bob sees Alice as friend number: $aliceFriendNumber');
      
      expect(bobFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Bob friend number not found (Tox ID: $bobToxId)');
      expect(aliceFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Alice friend number not found (Tox ID: $aliceToxId)');
      
      // Setup call state tracking
      var bobReceivedCall = false;
      
      // Setup Bob's call callback
      bobAV.setCallCallback((friendNumber, audioEnabled, videoEnabled) {
        if (friendNumber == aliceFriendNumber) {
          bobReceivedCall = true;
          bob.markCallbackReceived('onCall');
        }
      });
      
      // Setup call state callbacks (for future use)
      aliceAV.setCallStateCallback((friendNumber, state) {
        // Call state tracking can be used for verification
      });
      
      bobAV.setCallStateCallback((friendNumber, state) {
        // Call state tracking can be used for verification
      });
      
      // Wait for friend connection before starting call
      print('[Test] Waiting for friend connection before starting call...');
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      
      // Iterate ToxAV to process connection events
      final ffi = ffi_lib.Tim2ToxFfi.open();
      print('[Test] Iterating ToxAV to ensure friend connection is established...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        bob.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Alice calls Bob
      print('[Test] Alice calling Bob...');
      final callResult = await alice.runWithInstanceAsync(() async => aliceAV.startCall(
        bobFriendNumber,
        audioBitRate: 48,
        videoBitRate: 4000,
      ));
      expect(callResult, isTrue, reason: 'Failed to start call');
      print('[Test] Alice call started: $callResult');
      
      // Wait for Bob to receive call (short poll so Dart processes posted callback)
      print('[Test] Waiting for Bob to receive call...');
      int iterateCount = 0;
      await waitUntil(
        () {
          iterateCount++;
          alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
          bob.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
          if (iterateCount % 10 == 0) {
            print('[Test] Waiting for call callback (iteration $iterateCount)...');
          }
          return bobReceivedCall;
        },
        timeout: const Duration(seconds: 20),
        pollInterval: const Duration(milliseconds: 50),
        description: 'Bob received call',
      );
      print('[Test] Bob received call after $iterateCount iterations');
      
      // Bob answers
      print('[Test] Bob answering call...');
      final answerResult = await bob.runWithInstanceAsync(() async => bobAV.answerCall(
        aliceFriendNumber,
        audioBitRate: 48,
        videoBitRate: 4000,
      ));
      expect(answerResult, isTrue, reason: 'Failed to answer call');
      print('[Test] Bob answered call: $answerResult');
      
      // Wait for call to be established
      print('[Test] Waiting for call to be established...');
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Iterate ToxAV (simulate main loop)
      print('[Test] Iterating ToxAV to process call establishment...');
      for (int i = 0; i < 20; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        bob.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Bob hangs up
      final hangupResult = await bob.runWithInstanceAsync(() async => bobAV.endCall(aliceFriendNumber));
      expect(hangupResult, isTrue, reason: 'Failed to end call');
      
      // Wait for call state to update
      print('[Test] Waiting for hangup to process...');
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Iterate to process hangup
      print('[Test] Iterating ToxAV to process hangup...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        bob.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      _toxavTestsCompleted++;
      print('[Test] Regular AV call test completed');
    }, timeout: const Timeout(Duration(seconds: 120)));
    
    test('Reject flow - Bob rejects call', () async {
      // Get actual Tox IDs
      final bobToxId = bob.getToxId();
      final aliceToxId = alice.getToxId();
      print('[Test] Reject test - Alice Tox ID: $aliceToxId');
      print('[Test] Reject test - Bob Tox ID: $bobToxId');
      
      final bobFriendNumber = alice.runWithInstance(() => aliceAV.getFriendNumberByUserId(bobToxId));
      final aliceFriendNumber = bob.runWithInstance(() => bobAV.getFriendNumberByUserId(aliceToxId));
      
      print('[Test] Reject test - Alice sees Bob as friend number: $bobFriendNumber');
      print('[Test] Reject test - Bob sees Alice as friend number: $aliceFriendNumber');
      
      expect(bobFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Bob friend number not found (Tox ID: $bobToxId)');
      expect(aliceFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Alice friend number not found (Tox ID: $aliceToxId)');
      
      var bobReceivedCall = false;
      
      bobAV.setCallCallback((friendNumber, audioEnabled, videoEnabled) {
        if (friendNumber == aliceFriendNumber) {
          bobReceivedCall = true;
          bob.markCallbackReceived('onCall');
        }
      });
      
      aliceAV.setCallStateCallback((friendNumber, state) {
        // Call state tracking can be used for verification
      });
      
      print('[Test] Reject test - Waiting for friend connection...');
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      
      final ffiReject = ffi_lib.Tim2ToxFfi.open();
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
        bob.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Alice calls Bob (audio only)
      print('[Test] Reject test - Alice calling Bob...');
      final callResult = await alice.runWithInstanceAsync(() async => aliceAV.startCall(
        bobFriendNumber,
        audioBitRate: 48,
        videoBitRate: 0, // Audio only
      ));
      expect(callResult, isTrue);
      print('[Test] Reject test - Alice call started: $callResult');
      
      print('[Test] Reject test - Waiting for Bob to receive call...');
      int iterateCount = 0;
      await waitUntil(
        () {
          iterateCount++;
          alice.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
          bob.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
          return bobReceivedCall;
        },
        timeout: const Duration(seconds: 20),
        pollInterval: const Duration(milliseconds: 50),
        description: 'Bob received call',
      );
      expect(iterateCount, greaterThan(0), reason: 'avIterate should have run');
      print('[Test] Reject test - Bob received call: $bobReceivedCall');
      
      // Bob rejects
      print('[Test] Reject test - Bob rejecting call...');
      final rejectResult = await bob.runWithInstanceAsync(() async => bobAV.endCall(aliceFriendNumber));
      expect(rejectResult, isTrue, reason: 'Failed to reject call');
      print('[Test] Reject test - Bob rejected call: $rejectResult');
      
      print('[Test] Reject test - Waiting for rejection to process...');
      await Future.delayed(const Duration(milliseconds: 500));
      
      print('[Test] Reject test - Iterating ToxAV to process rejection...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
        bob.runWithInstance(() => ffiReject.avIterate(ffiReject.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      _toxavTestsCompleted++;
      print('[Test] Reject flow test completed');
    }, timeout: const Timeout(Duration(seconds: 120)));
    
    test('Cancel flow - Alice cancels call', () async {
      // Get actual Tox IDs
      final bobToxId = bob.getToxId();
      final aliceToxId = alice.getToxId();
      print('[Test] Cancel test - Alice Tox ID: $aliceToxId');
      print('[Test] Cancel test - Bob Tox ID: $bobToxId');
      
      final bobFriendNumber = alice.runWithInstance(() => aliceAV.getFriendNumberByUserId(bobToxId));
      final aliceFriendNumber = bob.runWithInstance(() => bobAV.getFriendNumberByUserId(aliceToxId));
      
      print('[Test] Cancel test - Alice sees Bob as friend number: $bobFriendNumber');
      print('[Test] Cancel test - Bob sees Alice as friend number: $aliceFriendNumber');
      
      expect(bobFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Bob friend number not found (Tox ID: $bobToxId)');
      expect(aliceFriendNumber, isNot(equals(0xFFFFFFFF)), 
        reason: 'Alice friend number not found (Tox ID: $aliceToxId)');
      
      var bobReceivedCall = false;
      
      bobAV.setCallCallback((friendNumber, audioEnabled, videoEnabled) {
        if (friendNumber == aliceFriendNumber) {
          bobReceivedCall = true;
          bob.markCallbackReceived('onCall');
        }
      });
      
      bobAV.setCallStateCallback((friendNumber, state) {
        // Call state tracking can be used for verification
      });
      
      print('[Test] Cancel test - Waiting for friend connection...');
      await Future.wait([
        alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30)),
        bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30)),
      ]);
      
      final ffiCancel = ffi_lib.Tim2ToxFfi.open();
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
        bob.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Alice calls Bob (audio only)
      print('[Test] Cancel test - Alice calling Bob...');
      final callResult = await alice.runWithInstanceAsync(() async => aliceAV.startCall(
        bobFriendNumber,
        audioBitRate: 48,
        videoBitRate: 0,
      ));
      expect(callResult, isTrue);
      print('[Test] Cancel test - Alice call started: $callResult');
      
      print('[Test] Cancel test - Waiting for Bob to receive call...');
      int iterateCount = 0;
      await waitUntil(
        () {
          iterateCount++;
          alice.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
          bob.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
          return bobReceivedCall;
        },
        timeout: const Duration(seconds: 20),
        pollInterval: const Duration(milliseconds: 50),
        description: 'Bob received call',
      );
      expect(iterateCount, greaterThan(0), reason: 'avIterate should have run');
      print('[Test] Cancel test - Bob received call: $bobReceivedCall');
      
      // Alice cancels
      print('[Test] Cancel test - Alice canceling call...');
      final cancelResult = await alice.runWithInstanceAsync(() async => aliceAV.endCall(bobFriendNumber));
      expect(cancelResult, isTrue, reason: 'Failed to cancel call');
      print('[Test] Cancel test - Alice canceled call: $cancelResult');
      
      print('[Test] Cancel test - Waiting for cancellation to process...');
      await Future.delayed(const Duration(milliseconds: 500));
      
      print('[Test] Cancel test - Iterating ToxAV to process cancellation...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
        bob.runWithInstance(() => ffiCancel.avIterate(ffiCancel.getCurrentInstanceId()));
        await Future.delayed(const Duration(milliseconds: 50));
      }
      _toxavTestsCompleted++;
      print('[Test] Cancel flow test completed');
    }, timeout: const Timeout(Duration(seconds: 120)));
  });
}
