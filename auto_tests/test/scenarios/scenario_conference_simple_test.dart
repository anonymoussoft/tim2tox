/// Conference Simple Test
/// 
/// Tests simple conference scenario with two nodes
/// Reference: c-toxcore/auto_tests/scenarios/scenario_conference_simple_test.c
/// Note: tim2tox uses Tox Group instead of Conference

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_value_callback.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_group_member_operation_result.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Conference Simple Tests (Two Nodes)', () {
    late TestScenario scenario;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['alice', 'bob']);
      await scenario.initAllNodes();
      // Parallelize login
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      await Future.wait([
        alice.login(),
        bob.login(),
      ]);
      
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
    
    test('Simple conference: create, join, send message', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      
      print('[ConferenceSimpleTest] ========== Test started ==========');
      print('[ConferenceSimpleTest] Alice userId: ${alice.userId}');
      print('[ConferenceSimpleTest] Bob userId: ${bob.userId}');
      
      // Establish friendship between Alice and Bob (required for group invitation)
      print('[ConferenceSimpleTest] Step 0: Establishing friendship between Alice and Bob...');
      await establishFriendship(alice, bob, timeout: const Duration(seconds: 20));
      print('[ConferenceSimpleTest] Friendship established');
      
      // Alice creates a group (conference)
      print('[ConferenceSimpleTest] Step 1: Alice creating group...');
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Simple Conference',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0), reason: 'createGroup failed with code ${createResult.code}');
      expect(createResult.data, isNotNull);
      final groupId = createResult.data!;
      print('[ConferenceSimpleTest] Group created successfully, groupId: $groupId');
      
      // Setup Bob's message listener BEFORE Bob joins
      print('[ConferenceSimpleTest] Step 2: Setting up Bob\'s message listener...');
      // Get Alice's Tox ID (public key) for comparison
      final aliceToxId = alice.getToxId();
      final alicePublicKey = alice.getPublicKey(); // 64 hex chars
      print('[ConferenceSimpleTest] Alice Tox ID: ${aliceToxId.substring(0, 20)}...');
      print('[ConferenceSimpleTest] Alice Public Key: ${alicePublicKey.substring(0, 20)}...');
      
      var bobReceivedMessage = false;
      var bobReceivedMessageCount = 0;
      
      // Store group member list for later use in message matching
      List<String>? groupMemberPublicKeys;
      
      final bobListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          print('[ConferenceSimpleTest] [BobListener] Received message: groupID=${message.groupID}, userID=${message.userID}, sender=${message.sender}, text=${message.textElem?.text}');
          if (message.groupID == groupId) {
            print('[ConferenceSimpleTest] [BobListener] GroupID matches!');
            // Message sender can be in either userID or sender field
            // Try sender first (from message_sender JSON field), then userID
            final messageSender = message.sender ?? message.userID ?? '';
            final senderPublicKey = messageSender.length >= 64 ? messageSender.substring(0, 64) : messageSender;
            print('[ConferenceSimpleTest] [BobListener] Extracted sender: ${senderPublicKey.length >= 20 ? senderPublicKey.substring(0, 20) : senderPublicKey}...');
            
            // Check if sender is in group member list (more reliable than comparing with Alice's public key)
            bool senderInGroup = false;
            if (groupMemberPublicKeys != null) {
              senderInGroup = groupMemberPublicKeys.contains(senderPublicKey);
              print('[ConferenceSimpleTest] [BobListener] Sender in group member list: $senderInGroup');
            } else {
              print('[ConferenceSimpleTest] [BobListener] Group member list not available yet, will accept message from any sender in this group');
              // If group member list is not yet available, accept message anyway (will be verified later)
              senderInGroup = true; // Optimistically accept
            }
            
            // Also check against Alice's public key (for compatibility)
            final matchesAlice = (senderPublicKey == alicePublicKey);
            
            if (senderInGroup || matchesAlice) {
              print('[ConferenceSimpleTest] [BobListener] Sender verified (inGroup=$senderInGroup, matchesAlice=$matchesAlice)! Marking message as received');
              bobReceivedMessage = true;
              bobReceivedMessageCount++;
              bob.addReceivedMessage(message);
            } else {
              final senderPreview = senderPublicKey.length >= 20 ? senderPublicKey.substring(0, 20) : senderPublicKey;
              print('[ConferenceSimpleTest] [BobListener] Sender not verified: sender=$senderPreview..., alicePublicKey=${alicePublicKey.substring(0, 20)}..., inGroupList=$senderInGroup');
            }
          } else {
            print('[ConferenceSimpleTest] [BobListener] GroupID mismatch: expected=$groupId, got=${message.groupID}');
          }
        },
      );
      
      bob.runWithInstance(() {
        TIMMessageManager.instance.addAdvancedMsgListener(bobListener);
      });
      print('[ConferenceSimpleTest] Bob\'s message listener added');
      
      // Alice invites Bob to the group
      print('[ConferenceSimpleTest] Step 2.5: Alice inviting Bob to group...');
      final bobToxId = bob.getToxId();
      final bobPublicKey = bob.getPublicKey(); // 64 hex chars
      print('[ConferenceSimpleTest] Bob Tox ID: ${bobToxId.substring(0, 20)}...');
      print('[ConferenceSimpleTest] Bob Public Key: ${bobPublicKey.substring(0, 20)}...');
      print('[ConferenceSimpleTest] Waiting for friend connection before inviting...');
      await pumpFriendConnection(alice, bob, duration: const Duration(seconds: 5));
      await bob.waitForConnection(timeout: const Duration(seconds: 10));
      await alice.waitForConnection(timeout: const Duration(seconds: 10));
      
      // Wait for bidirectional friend connection (both see each other as online)
      print('[ConferenceSimpleTest] Waiting for bidirectional friend connection...');
      // bobToxId and aliceToxId already declared above
      // Note: waitForFriendConnection may timeout if friend is in list but not online yet
      // This is OK - we'll proceed anyway as friend connection may establish during invite
      try {
        await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30));
        print('[ConferenceSimpleTest] Alice sees Bob as online');
      } catch (e) {
        print('[ConferenceSimpleTest] Warning: Alice did not see Bob as online yet: $e');
        print('[ConferenceSimpleTest] Continuing anyway - connection may establish during invite');
      }
      try {
        await bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30));
        print('[ConferenceSimpleTest] Bob sees Alice as online');
      } catch (e) {
        print('[ConferenceSimpleTest] Warning: Bob did not see Alice as online yet: $e');
        print('[ConferenceSimpleTest] Continuing anyway - connection may establish during invite');
      }
      print('[ConferenceSimpleTest] Friend connection check completed');
      
      // Additional wait to ensure connection is stable
      await Future.delayed(const Duration(seconds: 2));
      
      // Retry invitation if it fails (INVITE_FAIL can be transient)
      // IMPORTANT: Set Alice's instance as current before each invite attempt
      V2TimValueCallback<List<V2TimGroupMemberOperationResult>>? inviteResult;
      for (int retry = 0; retry < 3; retry++) {
        if (retry > 0) {
          print('[ConferenceSimpleTest] Retry $retry: Waiting before retry...');
          await Future.delayed(const Duration(seconds: 2));
        }
        print('[ConferenceSimpleTest] Attempting to invite Bob (attempt ${retry + 1}/3)...');
        inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [bobPublicKey], // Use public key (64 hex chars) for invitation
        ));
        final ir = inviteResult!;
        expect(ir.code, equals(0), reason: 'inviteUserToGroup failed with code ${ir.code}');
        
        // Check actual invitation results
        expect(ir.data, isNotNull, reason: 'inviteUserToGroup returned null data');
        expect(ir.data!.isNotEmpty, isTrue, reason: 'inviteUserToGroup returned empty result list');
        final bobInviteResult = ir.data!.firstWhere(
          (r) => r.memberID == bobPublicKey,
          orElse: () => throw Exception('Bob not found in invite result list'),
        );
        print('[ConferenceSimpleTest] Bob invite result: memberID=${bobInviteResult.memberID?.substring(0, 20) ?? "null"}..., result=${bobInviteResult.result}');
        // Note: result values: 0=FAIL, 1=SUCC (V2TimGroupMemberOperationResult._OPERATION_RESULT_SUCC = 1)
        if (bobInviteResult.result == 1) {
          print('[ConferenceSimpleTest] Bob invited successfully');
          break;
        } else {
          print('[ConferenceSimpleTest] Bob invitation failed: result=${bobInviteResult.result} (0=FAIL, 1=SUCC), will retry...');
          if (retry == 2) {
            throw Exception('Bob invitation failed after 3 attempts: result=${bobInviteResult.result}');
          }
        }
      }
      
      // Wait a bit for invitation to be processed
      await Future.delayed(const Duration(seconds: 2));
      
      // Bob joins the group
      print('[ConferenceSimpleTest] Step 3: Bob joining group $groupId...');
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      
      expect(joinResult.code, equals(0), reason: 'joinGroup failed with code ${joinResult.code}');
      print('[ConferenceSimpleTest] Bob joined group successfully');
      
      // Wait for Bob to be in the group and verify membership
      print('[ConferenceSimpleTest] Step 4: Waiting for Bob to be in group...');
      await Future.delayed(const Duration(seconds: 3));
      
      // Verify Bob is in the group and update group member list for message matching
      final bobMemberResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      print('[ConferenceSimpleTest] Bob member check: code=${bobMemberResult.code}, memberCount=${bobMemberResult.data?.memberInfoList?.length ?? 0}');
      if (bobMemberResult.data?.memberInfoList != null) {
        // tim2tox: C++ may return 76-char Tox ID or 64-char public key
        final memberIds = bobMemberResult.data!.memberInfoList!.map((m) => m.userID).toList();
        print('[ConferenceSimpleTest] Group members: $memberIds');
        final bobPublicKey = bob.getPublicKey();
        final bobInGroup = memberIds.any((id) => id == bobPublicKey || (id.length >= 64 && id.startsWith(bobPublicKey)));
        print('[ConferenceSimpleTest] Bob in group: $bobInGroup');
        groupMemberPublicKeys = memberIds;
        print('[ConferenceSimpleTest] Updated group member list for message matching: $groupMemberPublicKeys');
      }
      
      // Debug: Check group members from Alice's perspective before sending
      print('[ConferenceSimpleTest] Step 4.5: Checking group members from Alice\'s perspective...');
      final aliceMemberResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      print('[ConferenceSimpleTest] Alice member check: code=${aliceMemberResult.code}, memberCount=${aliceMemberResult.data?.memberInfoList?.length ?? 0}');
      if (aliceMemberResult.data?.memberInfoList != null) {
        final memberIds = aliceMemberResult.data!.memberInfoList!.map((m) => m.userID).toList();
        print('[ConferenceSimpleTest] Group members from Alice\'s perspective: $memberIds');
        final alicePublicKey = alice.getPublicKey();
        final bobPublicKey = bob.getPublicKey();
        print('[ConferenceSimpleTest] Alice public key: ${alicePublicKey.substring(0, 20)}...');
        print('[ConferenceSimpleTest] Bob public key: ${bobPublicKey.substring(0, 20)}...');
        final aliceInGroup = memberIds.any((id) => id == alicePublicKey || (id.length >= 64 && id.startsWith(alicePublicKey)));
        final bobInGroup = memberIds.any((id) => id == bobPublicKey || (id.length >= 64 && id.startsWith(bobPublicKey)));
        print('[ConferenceSimpleTest] Alice in group: $aliceInGroup');
        print('[ConferenceSimpleTest] Bob in group: $bobInGroup');
      }
      
      // Wait until Bob sees at least 2 members (so group channel is ready for message delivery)
      print('[ConferenceSimpleTest] Step 4.6: Waiting for Bob to see 2 members in group...');
      final memberSyncDeadline = DateTime.now().add(const Duration(seconds: 20));
      while (DateTime.now().isBefore(memberSyncDeadline)) {
        final list = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
          groupID: groupId,
          filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
          nextSeq: '0',
        ));
        final count = list.data?.memberInfoList?.length ?? 0;
        if (count >= 2) {
          print('[ConferenceSimpleTest] Bob sees $count members in group');
          break;
        }
        await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 1), iterationsPerPump: 120);
      }

      // Alice sends a message
      print('[ConferenceSimpleTest] Step 5: Alice sending message...');
      final sendResult = await alice.runWithInstanceAsync(() async {
        final textResult = TIMMessageManager.instance.createTextMessage(
          text: 'Hello from simple conference!',
        );
        expect(textResult.messageInfo, isNotNull);
        final message = textResult.messageInfo!;
        print('[ConferenceSimpleTest] Message created: msgID=${message.msgID}');
        return await TIMMessageManager.instance.sendMessage(
          message: message,
          receiver: null,
          groupID: groupId,
        );
      });
      
      expect(sendResult.code, equals(0), reason: 'sendMessage failed with code ${sendResult.code}');
      print('[ConferenceSimpleTest] Message sent successfully, sendResult code: ${sendResult.code}');
      
      // Wait for Bob to receive the message (pump both instances so group message is delivered).
      print('[ConferenceSimpleTest] Step 6: Waiting for Bob to receive message...');
      print('[ConferenceSimpleTest] Current state: bobReceivedMessage=$bobReceivedMessage, bobReceivedMessageCount=$bobReceivedMessageCount');
      print('[ConferenceSimpleTest] Bob receivedMessages count: ${bob.receivedMessages.length}');
      
      try {
        await waitUntilWithPump(
          () => bobReceivedMessage,
          timeout: const Duration(seconds: 60),
          description: 'Bob received group message',
          iterationsPerPump: 150,
          stepDelay: const Duration(milliseconds: 200),
        );
        print('[ConferenceSimpleTest] Bob received the message!');
      } catch (e) {
        print('[ConferenceSimpleTest] ERROR: Timeout waiting for message. Final state:');
        print('[ConferenceSimpleTest]   bobReceivedMessage=$bobReceivedMessage');
        print('[ConferenceSimpleTest]   bobReceivedMessageCount=$bobReceivedMessageCount');
        print('[ConferenceSimpleTest]   bob.receivedMessages.length=${bob.receivedMessages.length}');
        if (bob.receivedMessages.isNotEmpty) {
          print('[ConferenceSimpleTest]   First received message: groupID=${bob.receivedMessages.first.groupID}, userID=${bob.receivedMessages.first.userID}');
        }
        rethrow;
      }
      
      expect(bobReceivedMessage, isTrue);
      expect(bob.receivedMessages.length, greaterThan(0));
      
      // Verify message content
      final receivedMessage = bob.receivedMessages.first;
      expect(receivedMessage.textElem?.text, equals('Hello from simple conference!'));
      print('[ConferenceSimpleTest] ========== Test completed successfully ==========');
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Basic conference functionality verification', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      
      // Establish friendship between Alice and Bob (required for group invitation)
      await establishFriendship(alice, bob, timeout: const Duration(seconds: 20));
      
      // Create group
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Basic Conference',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      // Alice invites Bob to the group
      final bobPublicKey = bob.getPublicKey();
      // Wait for friend connection before inviting
      await bob.waitForConnection(timeout: const Duration(seconds: 5));
      await alice.waitForConnection(timeout: const Duration(seconds: 5));
      
      // Wait for bidirectional friend connection
      final bobToxId = bob.getToxId();
      try {
        await alice.waitForFriendConnection(bobToxId, timeout: const Duration(seconds: 30));
      } catch (e) {
        print('Warning: Alice did not see Bob as online yet: $e');
      }
      final aliceToxId = alice.getToxId();
      try {
        await bob.waitForFriendConnection(aliceToxId, timeout: const Duration(seconds: 30));
      } catch (e) {
        print('Warning: Bob did not see Alice as online yet: $e');
      }
      await Future.delayed(const Duration(seconds: 2));
      
      // Retry invitation if it fails
      V2TimValueCallback<List<V2TimGroupMemberOperationResult>>? inviteResult;
      for (int retry = 0; retry < 3; retry++) {
        if (retry > 0) {
          await Future.delayed(const Duration(seconds: 2));
        }
        inviteResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [bobPublicKey], // Use public key for invitation
        ));
        final ir = inviteResult!;
        expect(ir.code, equals(0));
        
        // Check actual invitation results
        expect(ir.data, isNotNull);
        expect(ir.data!.isNotEmpty, isTrue);
        final bobInviteResult = ir.data!.firstWhere(
          (r) => r.memberID == bobPublicKey,
          orElse: () => throw Exception('Bob not found in invite result list'),
        );
        // Note: result values: 0=FAIL, 1=SUCC
        if (bobInviteResult.result == 1) {
          break;
        } else if (retry == 2) {
          expect(bobInviteResult.result, equals(1), reason: 'Bob invitation failed after 3 attempts: result=${bobInviteResult.result} (0=FAIL, 1=SUCC)');
        }
      }
      
      // Wait a bit for invitation to be processed
      await Future.delayed(const Duration(seconds: 2));
      
      // Bob joins
      final joinResult = await bob.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
        groupID: groupId,
        message: '',
      ));
      expect(joinResult.code, equals(0));
      
      // Wait for group sync so joined list is populated (pump so Tox updates joined list)
      await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 4));
      final listDeadline = DateTime.now().add(const Duration(seconds: 20));
      while (DateTime.now().isBefore(listDeadline)) {
        await pumpGroupPeerDiscovery(alice, bob, duration: const Duration(seconds: 1));
        final aliceList = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
        final bobList = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
        if ((aliceList.data?.length ?? 0) >= 1 && (bobList.data?.length ?? 0) >= 1) break;
        await Future.delayed(const Duration(milliseconds: 300));
      }
      
      // Verify group info
      final groupInfoResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupsInfo(
        groupIDList: [groupId],
      ));
      
      expect(groupInfoResult.code, equals(0));
      expect(groupInfoResult.data, isNotNull);
      expect(groupInfoResult.data!.length, equals(1));
      expect(groupInfoResult.data!.first.groupInfo?.groupName, equals('Basic Conference'));
      
      // Verify both can see the group in their joined list
      final aliceGroupsResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      final bobGroupsResult = await bob.runWithInstanceAsync(() async => TIMGroupManager.instance.getJoinedGroupList());
      
      expect(aliceGroupsResult.code, equals(0));
      expect(bobGroupsResult.code, equals(0));
      expect(aliceGroupsResult.data, isNotNull);
      expect(bobGroupsResult.data, isNotNull);
      expect(aliceGroupsResult.data!.length, greaterThanOrEqualTo(1), reason: 'Alice joined list empty');
      expect(bobGroupsResult.data!.length, greaterThanOrEqualTo(1), reason: 'Bob joined list empty');
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
