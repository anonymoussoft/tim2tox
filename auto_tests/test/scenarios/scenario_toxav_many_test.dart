/// ToxAV Many Test
/// 
/// Tests audio/video calls with multiple participants
/// Reference: c-toxcore/auto_tests/scenarios/scenario_toxav_many_test.c
/// 
/// Tests multiple simultaneous AV calls from Alice to multiple Bobs

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_friendship_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/friend_type_enum.dart';
import 'package:tim2tox_dart/ffi/tim2tox_ffi.dart' as ffi_lib;
import 'package:tim2tox_dart/service/toxav_service.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('ToxAV Many Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late List<TestNode> bobs;
    late ToxAVService aliceAV;
    late List<ToxAVService> bobAVs;
    
    const numBobs = 3;
    
    setUpAll(() async {
      await setupTestEnvironment();
      
      // Create scenario with Alice and multiple Bobs
      final aliases = ['alice', ...List.generate(numBobs, (i) => 'bob_$i')];
      scenario = await createTestScenario(aliases);
      
      alice = scenario.getNode('alice')!;
      bobs = List.generate(numBobs, (i) => scenario.getNode('bob_$i')!);
      
      await scenario.initAllNodes();
      // Parallelize login for all nodes
      await Future.wait([
        alice.login(),
        ...bobs.map((bob) => bob.login()),
      ]);
      
      // Wait for all nodes to be connected - optimized timeout
      await waitUntil(() => alice.loggedIn && bobs.every((bob) => bob.loggedIn),
        timeout: const Duration(seconds: 10),
      );
      
      await configureLocalBootstrap(scenario);
      
      // Enable auto-accept in Dart so friend establishment succeeds (no C++ default)
      alice.enableAutoAccept();
      for (final bob in bobs) {
        bob.enableAutoAccept();
      }
      
      // Wait for connections to be established so Tox IDs are available
      // Parallelize connection waiting
      print('[Test] setUp - Waiting for connections to establish...');
      try {
        await Future.wait([
          alice.waitForConnection(timeout: const Duration(seconds: 5)),
          ...bobs.map((bob) => bob.waitForConnection(timeout: const Duration(seconds: 5))),
        ]);
      } catch (e) {
        print('[Test] setUp - Warning: Connection wait timeout, continuing anyway: $e');
      }
      
      // Wait a bit more for Tox IDs to be available - reduced delay
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Get actual Tox IDs (not test identifiers)
      final aliceToxId = alice.getToxId();
      print('[Test] setUp - Alice Tox ID: $aliceToxId (length=${aliceToxId.length})');
      
      if (aliceToxId.isEmpty || aliceToxId.length != 76) {
        throw Exception('Invalid Alice Tox ID: $aliceToxId (expected 76 hex chars)');
      }
      
      // Get all Bobs' Tox IDs
      final bobToxIds = <String>[];
      for (int i = 0; i < numBobs; i++) {
        final bobToxId = bobs[i].getToxId();
        bobToxIds.add(bobToxId);
        print('[Test] setUp - Bob $i Tox ID: $bobToxId (length=${bobToxId.length})');
        
        if (bobToxId.isEmpty || bobToxId.length != 76) {
          throw Exception('Invalid Bob $i Tox ID: $bobToxId (expected 76 hex chars)');
        }
      }
      
      // Add friends using actual Tox IDs (Alice adds all Bobs, each Bob adds Alice)
      // Optimize: Parallelize friend additions
      print('[Test] setUp - Adding friends using actual Tox IDs...');
      final friendAddFutures = <Future>[];
      
      for (int i = 0; i < numBobs; i++) {
        final i_ = i;
        // Alice adds Bob i
        friendAddFutures.add(alice.runWithInstanceAsync(() async {
          await TIMFriendshipManager.instance.addFriend(
            userID: bobToxIds[i_],
            addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
            remark: 'Bob $i_',
            addWording: 'test',
          );
          print('[Test] setUp - Alice added Bob $i_ as friend');
        }));
        
        // Bob i adds Alice
        friendAddFutures.add(bobs[i].runWithInstanceAsync(() async {
          await TIMFriendshipManager.instance.addFriend(
            userID: aliceToxId,
            addType: FriendTypeEnum.V2TIM_FRIEND_TYPE_BOTH,
            remark: 'Alice',
            addWording: 'test',
          );
          print('[Test] setUp - Bob $i added Alice as friend');
        }));
      }
      
      // Wait for all friend additions to complete
      await Future.wait(friendAddFutures);
      
      // Friend requests may need time to propagate/auto-accept over the local DHT
      print('[Test] setUp - Waiting for friend list to populate...');
      await Future.delayed(const Duration(seconds: 5));
      final alicePub = alice.getPublicKey();
      final bobPubs = bobs.map((b) => b.getPublicKey()).toList();
      await waitForFriendsInList(alice, bobPubs, timeout: const Duration(seconds: 120));
      for (int i = 0; i < numBobs; i++) {
        await waitForFriendsInList(bobs[i], [alicePub], timeout: const Duration(seconds: 120));
      }
      // Extra delay for P2P connection status to propagate (multi-node)
      await Future.delayed(const Duration(seconds: 5));
      print('[Test] setUp - Friend list ready');
      
      // Create and initialize ToxAV inside instance context so each service registers with correct instance_id
      final ffi = ffi_lib.Tim2ToxFfi.open();
      final aliceInit = await alice.runWithInstanceAsync(() async {
        aliceAV = ToxAVService(ffi);
        return aliceAV.initialize();
      });
      expect(aliceInit, isTrue, reason: 'Alice ToxAV initialization failed');
      
      bobAVs = [];
      for (int i = 0; i < numBobs; i++) {
        final bobInit = await bobs[i].runWithInstanceAsync(() async {
          final bobAV = ToxAVService(ffi);
          final ok = await bobAV.initialize();
          bobAVs.add(bobAV);
          return ok;
        });
        expect(bobInit, isTrue, reason: 'Bob $i ToxAV initialization failed');
      }
    });
    
    tearDownAll(() async {
      aliceAV.shutdown();
      for (final bobAV in bobAVs) {
        bobAV.shutdown();
      }
      await scenario.dispose();
      await teardownTestEnvironment();
    });
    
    test('Multiple simultaneous AV calls', () async {
      // Get actual Tox IDs
      final aliceToxId = alice.getToxId();
      final bobToxIds = bobs.map((bob) => bob.getToxId()).toList();
      print('[Test] Many test - Alice Tox ID: $aliceToxId');
      for (int i = 0; i < numBobs; i++) {
        print('[Test] Many test - Bob $i Tox ID: ${bobToxIds[i]}');
      }
      
      // Get friend numbers for all Bobs using actual Tox IDs
      final bobFriendNumbers = <int>[];
      final aliceFriendNumbers = <int>[];
      
      for (int i = 0; i < numBobs; i++) {
        final bobFriendNumber = alice.runWithInstance(() => aliceAV.getFriendNumberByUserId(bobToxIds[i]));
        final aliceFriendNumber = bobs[i].runWithInstance(() => bobAVs[i].getFriendNumberByUserId(aliceToxId));
        
        print('[Test] Many test - Alice sees Bob $i as friend number: $bobFriendNumber');
        print('[Test] Many test - Bob $i sees Alice as friend number: $aliceFriendNumber');
        
        expect(bobFriendNumber, isNot(equals(0xFFFFFFFF)), 
          reason: 'Bob $i friend number not found (Tox ID: ${bobToxIds[i]})');
        expect(aliceFriendNumber, isNot(equals(0xFFFFFFFF)), 
          reason: 'Alice friend number not found for Bob $i (Tox ID: $aliceToxId)');
        
        bobFriendNumbers.add(bobFriendNumber);
        aliceFriendNumbers.add(aliceFriendNumber);
      }
      
      // Setup call state tracking for all Bobs
      final bobReceivedCalls = List.generate(numBobs, (_) => false);
      final bobCallStates = List.generate(numBobs, (_) => 0);
      
      // Setup callbacks for all Bobs
      for (int i = 0; i < numBobs; i++) {
        bobAVs[i].setCallCallback((friendNumber, audioEnabled, videoEnabled) {
          if (friendNumber == aliceFriendNumbers[i]) {
            bobReceivedCalls[i] = true;
            bobs[i].markCallbackReceived('onCall');
          }
        });
        
        bobAVs[i].setCallStateCallback((friendNumber, state) {
          if (friendNumber == aliceFriendNumbers[i]) {
            bobCallStates[i] = state;
          }
        });
      }
      
      // Wait for friend connections before starting calls
      print('[Test] Many test - Waiting for friend connections...');
      await alice.waitForFriendConnection(bobToxIds[0], timeout: const Duration(seconds: 90));
      for (int i = 0; i < numBobs; i++) {
        await bobs[i].waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 90));
      }
      
      // Iterate ToxAV to process connection events
      final ffi = ffi_lib.Tim2ToxFfi.open();
      print('[Test] Many test - Iterating ToxAV to ensure connections...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        for (int j = 0; j < numBobs; j++) {
          bobs[j].runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        }
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Alice calls all Bobs
      print('[Test] Many test - Alice calling all Bobs...');
      for (int i = 0; i < numBobs; i++) {
        print('[Test] Many test - Calling Bob $i (friend number: ${bobFriendNumbers[i]})...');
        // Note: bit rates are in kbit/sec (48 kbit/sec for audio, 3000 kbit/sec for video)
        final callResult = await alice.runWithInstanceAsync(() async => aliceAV.startCall(
          bobFriendNumbers[i],
          audioBitRate: 48,
          videoBitRate: 3000,
        ));
        expect(callResult, isTrue, reason: 'Failed to call Bob $i');
        print('[Test] Many test - Call to Bob $i result: $callResult');
      }
      
      // Wait for all Bobs to receive calls
      // Iterate ToxAV while waiting
      print('[Test] Many test - Waiting for all Bobs to receive calls...');
      int iterateCount = 0;
      await waitUntil(
        () {
          iterateCount++;
          alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
          for (int i = 0; i < numBobs; i++) {
            bobs[i].runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
          }
          if (iterateCount % 10 == 0) {
            print('[Test] Many test - Waiting for call callbacks (iteration $iterateCount, received: ${bobReceivedCalls})...');
          }
          return bobReceivedCalls.every((received) => received);
        },
        timeout: const Duration(seconds: 25),
        description: 'All Bobs received calls',
      );
      print('[Test] Many test - All Bobs received calls: ${bobReceivedCalls}');
      
      // All Bobs answer
      print('[Test] Many test - All Bobs answering calls...');
      for (int i = 0; i < numBobs; i++) {
        print('[Test] Many test - Bob $i answering call (friend number: ${aliceFriendNumbers[i]})...');
        // Note: bit rates are in kbit/sec (8 kbit/sec for audio, 500 kbit/sec for video)
        final answerResult = await bobs[i].runWithInstanceAsync(() async => bobAVs[i].answerCall(
          aliceFriendNumbers[i],
          audioBitRate: 8,
          videoBitRate: 500,
        ));
        expect(answerResult, isTrue, reason: 'Bob $i failed to answer');
        print('[Test] Many test - Bob $i answer result: $answerResult');
      }
      
      // Wait for calls to be established
      print('[Test] Many test - Waiting for calls to be established...');
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Iterate ToxAV (simulate main loop) - call frequently for local bootstrap
      print('[Test] Many test - Iterating ToxAV to process call establishment...');
      for (int i = 0; i < 20; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        for (int j = 0; j < numBobs; j++) {
          bobs[j].runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        }
        await Future.delayed(const Duration(milliseconds: 50));
      }
      
      // Alice hangs up all calls
      print('[Test] Many test - Alice hanging up all calls...');
      for (int i = 0; i < numBobs; i++) {
        print('[Test] Many test - Hanging up call with Bob $i...');
        final hangupResult = await alice.runWithInstanceAsync(() async => aliceAV.endCall(bobFriendNumbers[i]));
        expect(hangupResult, isTrue, reason: 'Failed to hang up call with Bob $i');
        print('[Test] Many test - Hangup with Bob $i result: $hangupResult');
      }
      
      // Give it a few ticks to send hangup packets
      print('[Test] Many test - Iterating ToxAV to process hangups...');
      for (int i = 0; i < 10; i++) {
        alice.runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        for (int j = 0; j < numBobs; j++) {
          bobs[j].runWithInstance(() => ffi.avIterate(ffi.getCurrentInstanceId()));
        }
        await Future.delayed(const Duration(milliseconds: 50));
      }
      print('[Test] Many test - Multiple simultaneous AV calls test completed');
    }, timeout: const Timeout(Duration(seconds: 120)));
  });
}
