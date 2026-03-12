/// Signaling Test
/// 
/// Tests signaling functionality for audio/video call invitations
/// Reference: Tencent Cloud Chat SDK signaling features
/// Note: Signaling is used for initiating and managing audio/video calls

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_signaling_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimSignalingListener.dart';
import 'package:tencent_cloud_chat_sdk/tencent_cloud_chat_sdk_platform_interface.dart';
import 'package:tim2tox_dart/ffi/tim2tox_ffi.dart' as ffi_lib;
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Signaling Tests', () {
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
      
      // Wait for both nodes to be connected
      await waitUntil(() => alice.loggedIn && bob.loggedIn);
      
      // Configure local bootstrap
      await configureLocalBootstrap(scenario);
      
      // Establish friendship (uses Tox IDs and waits for P2P connection)
      await establishFriendship(alice, bob);
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
    
    test('Send and receive signaling invite', () async {
      var bobReceivedInvite = false;
      String? receivedInviteID;
      String? receivedInviter;
      String? receivedData;
      
      // Setup Bob's signaling listener on bob's instance
      final bobSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {
          bobReceivedInvite = true;
          receivedInviteID = inviteID;
          receivedInviter = inviter;
          receivedData = data;
        },
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      // Ensure C++ current instance is Bob for the whole registration (platform addSignalingListener is async)
      await bob.runWithInstanceAsync(() async {
        ffi_lib.Tim2ToxFfi.open().setCurrentInstance(bob.testInstanceHandle!);
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(bobSignalingListener);
      });
      
      // Alice sends a signaling invite to Bob (use Tox ID for invitee)
      final inviteData = '{"type":"video_call","room_id":"test_room_123"}';
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.invite(
        invitee: bob.getToxId(),
        data: inviteData,
        timeout: 30,
        onlineUserOnly: false,
      ));
      
      print('[Signaling] Send invite: code=${inviteResult.code} desc=${inviteResult.desc} data=${inviteResult.data}');
      expect(inviteResult.code, equals(0));
      expect(inviteResult.data, isNotNull);
      expect(inviteResult.data, isNotEmpty);
      
      final inviteID = inviteResult.data!;
      
      // Give Alice's event thread a moment to run the send task, then pump so Bob can receive
      pumpAllInstancesOnce(iterations: 80);
      await Future.delayed(const Duration(milliseconds: 300));
      
      // Wait for Bob to receive the invite (pump so both instances run tox_iterate and event threads can process)
      await waitUntilWithPump(
        () => bobReceivedInvite,
        timeout: const Duration(seconds: 30),
        description: 'bobReceivedInvite (inviteID=$inviteID)',
        iterationsPerPump: 150,
        stepDelay: const Duration(milliseconds: 150),
      );
      
      expect(receivedInviteID, equals(inviteID));
      // C++ may send inviter as 64-char public key or full 76-char Tox ID
      expect(
        receivedInviter,
        anyOf(equals(alice.getToxId()), equals(alice.getToxId().length >= 64 ? alice.getToxId().substring(0, 64) : alice.getToxId())),
      );
      // Native may send data as JSON string or parsed Map.toString(); accept if content matches
      expect(receivedData, isNotNull);
      expect((receivedData ?? '').contains('video_call'), isTrue);
      expect((receivedData ?? '').contains('test_room_123'), isTrue);
      
      // Clean up
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: bobSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Accept signaling invite', () async {
      var aliceReceivedAccept = false;
      String? acceptedInviteID;
      String? acceptedInvitee;
      
      // Setup Alice's signaling listener on alice's instance
      final aliceSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {},
        onInviteeAccepted: (inviteID, invitee, data) {
          aliceReceivedAccept = true;
          acceptedInviteID = inviteID;
          acceptedInvitee = invitee;
        },
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(aliceSignalingListener);
      });
      
      // Setup Bob's signaling listener on bob's instance to receive invite
      var bobReceivedInvite = false;
      String? bobInviteID;
      
      final bobSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {
          bobReceivedInvite = true;
          bobInviteID = inviteID;
        },
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(bobSignalingListener);
      });
      
      // Alice sends invite (use Tox ID for invitee)
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.invite(
        invitee: bob.getToxId(),
        data: '{"type":"video_call"}',
        timeout: 30,
      ));
      
      expect(inviteResult.code, equals(0));
      final inviteID = inviteResult.data!;
      
      // Wait for Bob to receive invite (pump so Bob's Tox can process the packet)
      await waitUntilWithPump(() => bobReceivedInvite, timeout: const Duration(seconds: 30), description: 'bobReceivedInvite');
      
      // Bob accepts the invite on bob's instance
      final acceptResult = await bob.runWithInstanceAsync(() async => TIMSignalingManager.instance.accept(
        inviteID: bobInviteID!,
        data: '{"type":"accept"}',
      ));
      
      expect(acceptResult.code, equals(0));
      
      // Wait for Alice to receive accept notification (pump so Alice's Tox can process the packet)
      await waitUntilWithPump(() => aliceReceivedAccept, timeout: const Duration(seconds: 30), description: 'aliceReceivedAccept');
      
      expect(acceptedInviteID, equals(inviteID));
      expect(
        acceptedInvitee,
        anyOf(equals(bob.getToxId()), equals(bob.getToxId().length >= 64 ? bob.getToxId().substring(0, 64) : bob.getToxId())),
      );
      
      // Clean up
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: aliceSignalingListener);
      });
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: bobSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Reject signaling invite', () async {
      var aliceReceivedReject = false;
      String? rejectedInviteID;
      String? rejectedInvitee;
      
      // Setup Alice's signaling listener on alice's instance
      final aliceSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {},
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {
          aliceReceivedReject = true;
          rejectedInviteID = inviteID;
          rejectedInvitee = invitee;
        },
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(aliceSignalingListener);
      });
      
      // Setup Bob's signaling listener on bob's instance
      var bobReceivedInvite = false;
      String? bobInviteID;
      
      final bobSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {
          bobReceivedInvite = true;
          bobInviteID = inviteID;
        },
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(bobSignalingListener);
      });
      
      // Alice sends invite (use Tox ID for invitee)
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.invite(
        invitee: bob.getToxId(),
        data: '{"type":"video_call"}',
        timeout: 30,
      ));
      
      expect(inviteResult.code, equals(0));
      final inviteID = inviteResult.data!;
      
      // Wait for Bob to receive invite (pump so Bob's Tox can process the packet)
      await waitUntilWithPump(() => bobReceivedInvite, timeout: const Duration(seconds: 30), description: 'bobReceivedInvite');
      
      // Bob rejects the invite on bob's instance
      final rejectResult = await bob.runWithInstanceAsync(() async => TIMSignalingManager.instance.reject(
        inviteID: bobInviteID!,
        data: '{"type":"reject","reason":"busy"}',
      ));
      
      expect(rejectResult.code, equals(0));
      
      // Wait for Alice to receive reject notification (pump so Alice's Tox can process the packet)
      await waitUntilWithPump(() => aliceReceivedReject, timeout: const Duration(seconds: 30), description: 'aliceReceivedReject');
      
      expect(rejectedInviteID, equals(inviteID));
      expect(
        rejectedInvitee,
        anyOf(equals(bob.getToxId()), equals(bob.getToxId().length >= 64 ? bob.getToxId().substring(0, 64) : bob.getToxId())),
      );
      
      // Clean up
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: aliceSignalingListener);
      });
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: bobSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Cancel signaling invite', () async {
      var bobReceivedCancel = false;
      String? cancelledInviteID;
      String? cancelledInviter;
      
      // Setup Bob's signaling listener on bob's instance
      final bobSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {},
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {
          bobReceivedCancel = true;
          cancelledInviteID = inviteID;
          cancelledInviter = inviter;
        },
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(bobSignalingListener);
      });
      
      // Alice sends invite (use Tox ID for invitee)
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.invite(
        invitee: bob.getToxId(),
        data: '{"type":"video_call"}',
        timeout: 30,
      ));
      
      expect(inviteResult.code, equals(0));
      final inviteID = inviteResult.data!;
      
      // Wait a bit for invite to be sent (pump so both instances process)
      await Future.delayed(const Duration(seconds: 1));
      pumpAllInstancesOnce(iterations: 100);
      
      // Alice cancels the invite on alice's instance
      final cancelResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.cancel(
        inviteID: inviteID,
        data: '{"type":"cancel","reason":"changed_mind"}',
      ));
      
      expect(cancelResult.code, equals(0));
      
      // Wait for Bob to receive cancel notification (pump so Bob's Tox can process the packet)
      await waitUntilWithPump(() => bobReceivedCancel, timeout: const Duration(seconds: 30), description: 'bobReceivedCancel');
      
      expect(cancelledInviteID, equals(inviteID));
      expect(
        cancelledInviter,
        anyOf(equals(alice.getToxId()), equals(alice.getToxId().length >= 64 ? alice.getToxId().substring(0, 64) : alice.getToxId())),
      );
      
      // Clean up
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: bobSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Signaling invite timeout', () async {
      var aliceReceivedTimeout = false;
      String? timeoutInviteID;
      
      // Setup Alice's signaling listener on alice's instance
      final aliceSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {},
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {
          aliceReceivedTimeout = true;
          timeoutInviteID = inviteID;
        },
      );
      
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(aliceSignalingListener);
      });
      
      // Alice sends invite with short timeout (use Tox ID for invitee)
      final inviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.invite(
        invitee: bob.getToxId(),
        data: '{"type":"video_call"}',
        timeout: 5, // 5 seconds timeout
      ));
      
      expect(inviteResult.code, equals(0));
      final inviteID = inviteResult.data!;
      
      // Don't accept or reject - wait for timeout (pump so timeout can fire)
      await waitUntilWithPump(() => aliceReceivedTimeout, timeout: const Duration(seconds: 10), description: 'aliceReceivedTimeout');
      
      expect(timeoutInviteID, equals(inviteID));
      
      // Clean up
      await alice.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: aliceSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: aliceSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Group signaling invite', () async {
      // Create a group on alice's instance first
      final createResult = await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'Meeting',
        groupName: 'Signaling Test Group',
        groupID: '',
      ));
      
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      
      // Add Bob to the group (native expects 64-char public key)
      final bobPk = bob.getToxId().length >= 64 ? bob.getToxId().substring(0, 64) : bob.getToxId();
      await alice.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: groupId,
        userList: [bobPk],
      ));
      
      // Wait for Bob to join
      await Future.delayed(const Duration(seconds: 2));
      
      // Setup Bob's signaling listener on bob's instance
      var bobReceivedGroupInvite = false;
      String? bobGroupInviteID;
      String? bobGroupID;
      
      final bobSignalingListener = V2TimSignalingListener(
        onReceiveNewInvitation: (inviteID, inviter, groupID, inviteeList, data) {
          if (groupID == groupId) {
            bobReceivedGroupInvite = true;
            bobGroupInviteID = inviteID;
            bobGroupID = groupID;
          }
        },
        onInviteeAccepted: (inviteID, invitee, data) {},
        onInviteeRejected: (inviteID, invitee, data) {},
        onInvitationCancelled: (inviteID, inviter, data) {},
        onInvitationTimeout: (inviteID, inviteeList) {},
      );
      
      await bob.runWithInstanceAsync(() async {
        ffi_lib.Tim2ToxFfi.open().setCurrentInstance(bob.testInstanceHandle!);
        await TencentCloudChatSdkPlatform.instance.addSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.addSignalingListener(bobSignalingListener);
      });
      
      // Alice sends group signaling invite (native may expect 64-char for inviteeList)
      final groupInviteResult = await alice.runWithInstanceAsync(() async => TIMSignalingManager.instance.inviteInGroup(
        groupID: groupId,
        inviteeList: [bobPk],
        data: '{"type":"group_video_call"}',
        timeout: 30,
      ));
      
      expect(groupInviteResult.code, equals(0));
      expect(groupInviteResult.data, isNotNull);
      expect(groupInviteResult.data, isNotEmpty);
      
      // Wait for Bob to receive group invite (pump so Bob's Tox can process the packet)
      await waitUntilWithPump(() => bobReceivedGroupInvite, timeout: const Duration(seconds: 30), description: 'bobReceivedGroupInvite');
      
      expect(bobGroupInviteID, isNotNull);
      expect(bobGroupID, equals(groupId));
      
      // Clean up
      await bob.runWithInstanceAsync(() async {
        await TencentCloudChatSdkPlatform.instance.removeSignalingListener(listener: bobSignalingListener);
        TIMSignalingManager.instance.removeSignalingListener(listener: bobSignalingListener);
      });
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
