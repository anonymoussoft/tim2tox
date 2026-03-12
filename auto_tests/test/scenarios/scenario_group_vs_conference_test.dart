/// Group vs Conference Comparison Test
///
/// Tests differences between Group (new API) and Conference (old API) types
/// Verifies that both types work correctly and have expected behaviors

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group vs Conference Comparison Tests', () {
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

      await waitUntil(
        () => alice.loggedIn && bob.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'both nodes logged in',
      );

      await configureLocalBootstrap(scenario);
      await Future.wait([
        alice.waitForConnection(timeout: const Duration(seconds: 15)),
        bob.waitForConnection(timeout: const Duration(seconds: 15)),
      ]);
      await establishFriendship(alice, bob,
          timeout: const Duration(seconds: 90));
      await pumpFriendConnection(alice, bob,
          duration: const Duration(seconds: 4));
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

    test('Create Group type (new API)', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Test Group (New API)',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      expect(createResult.data, isNotNull);
      final groupId = createResult.data!;
      await Future.delayed(const Duration(milliseconds: 500));
      final groupsInfoResult = await alice.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.getGroupsInfo(groupIDList: [groupId]));
      expect(groupsInfoResult.code, equals(0),
          reason: 'getGroupsInfo failed: ${groupsInfoResult.code}');
      expect(groupsInfoResult.data, isNotNull);
      expect(groupsInfoResult.data!.length, equals(1));
      expect(groupsInfoResult.data!.first.groupInfo?.groupName,
          equals('Test Group (New API)'));
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Create Conference type (old API)', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'Meeting',
                groupName: 'Test Conference (Old API)',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      expect(createResult.data, isNotNull);
      final conferenceId = createResult.data!;
      await Future.delayed(const Duration(milliseconds: 500));
      final groupsInfoResult = await alice.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.getGroupsInfo(groupIDList: [conferenceId]));
      expect(groupsInfoResult.code, equals(0),
          reason: 'getGroupsInfo failed: ${groupsInfoResult.code}');
      expect(groupsInfoResult.data, isNotNull);
      expect(groupsInfoResult.data!.length, equals(1));
      expect(groupsInfoResult.data!.first.groupInfo?.groupName,
          equals('Test Conference (Old API)'));
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Group type: join and send message', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Group Type Test',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final expectedGroupIds = <String>{groupId};
      const groupMessage = 'Hello from Group type!';
      final alicePublicKey = alice.getPublicKey();
      var bobReceivedMessage = false;
      final bobListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (!expectedGroupIds.contains(message.groupID ?? '')) {
            return;
          }
          final messageUserID = message.userID ?? '';
          final senderPublicKey = messageUserID.length >= 64
              ? messageUserID.substring(0, 64)
              : messageUserID;
          final fromAlice = senderPublicKey == alicePublicKey;
          final isExpectedText = message.textElem?.text == groupMessage;
          if (fromAlice || isExpectedText) {
            bobReceivedMessage = true;
            bob.addReceivedMessage(message);
          }
        },
      );
      bob.runWithInstance(
          () => TIMMessageManager.instance.addAdvancedMsgListener(bobListener));
      try {
        await Future.wait([
          alice.waitForConnection(timeout: const Duration(seconds: 10)),
          bob.waitForConnection(timeout: const Duration(seconds: 10)),
        ]);
        await Future.delayed(const Duration(milliseconds: 500));
        bob.clearCallbackReceived('onGroupInvited');
        final inviteResult = await alice.runWithInstanceAsync(() async =>
            TIMGroupManager.instance.inviteUserToGroup(
                groupID: groupId, userList: [bob.getPublicKey()]));
        expect(inviteResult.code, equals(0),
            reason: 'inviteUserToGroup failed: ${inviteResult.code}');
        await waitUntilWithPump(
          () => bob.callbackReceived['onGroupInvited'] == true,
          timeout: const Duration(seconds: 12),
          description: 'Bob receives onGroupInvited for group type test',
          iterationsPerPump: 100,
          stepDelay: const Duration(milliseconds: 200),
        );
        final joinGroupId =
            bob.getLastCallbackGroupId('onGroupInvited') ?? groupId;
        expectedGroupIds.add(joinGroupId);
        final joinResult = await bob.runWithInstanceAsync(() async =>
            TIMManager.instance.joinGroup(groupID: joinGroupId, message: ''));
        expect(joinResult.code, equals(0),
            reason: 'joinGroup failed: ${joinResult.code}');
        await pumpGroupPeerDiscovery(alice, bob,
            duration: const Duration(seconds: 3));
        final bobInGroup = await waitUntilFounderSeesMemberInGroup(
            alice, bob, groupId,
            timeout: const Duration(seconds: 20));
        expect(bobInGroup, isNotNull,
            reason: 'Alice must see Bob in group before sending');
        final sendResult = await alice.runWithInstanceAsync(() async {
          final textResult =
              TIMMessageManager.instance.createTextMessage(text: groupMessage);
          return TIMMessageManager.instance.sendMessage(
              message: textResult.messageInfo!,
              receiver: null,
              groupID: groupId);
        });
        expect(sendResult.code, equals(0));
        pumpAllInstancesOnce(iterations: 150);
        await Future.delayed(const Duration(milliseconds: 500));
        await waitUntilWithPump(
          () => bobReceivedMessage,
          timeout: const Duration(seconds: 25),
          description: 'Bob receives message',
          iterationsPerPump: 120,
          stepDelay: const Duration(milliseconds: 200),
        );
        expect(bobReceivedMessage, isTrue);
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: bobListener));
      }
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Conference type: join and send message', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'Meeting',
                groupName: 'Conference Type Test',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      final conferenceId = createResult.data!;
      final expectedGroupIds = <String>{conferenceId};
      const conferenceMessage = 'Hello from Conference type!';
      final alicePublicKey = alice.getPublicKey();
      var bobReceivedMessage = false;
      final bobListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (!expectedGroupIds.contains(message.groupID ?? '')) {
            return;
          }
          final messageUserID = message.userID ?? '';
          final senderPublicKey = messageUserID.length >= 64
              ? messageUserID.substring(0, 64)
              : messageUserID;
          final fromAlice = senderPublicKey == alicePublicKey;
          final isExpectedText = message.textElem?.text == conferenceMessage;
          if (fromAlice || isExpectedText) {
            bobReceivedMessage = true;
            bob.addReceivedMessage(message);
          }
        },
      );
      bob.runWithInstance(
          () => TIMMessageManager.instance.addAdvancedMsgListener(bobListener));
      try {
        await Future.wait([
          alice.waitForConnection(timeout: const Duration(seconds: 10)),
          bob.waitForConnection(timeout: const Duration(seconds: 10)),
        ]);
        await Future.delayed(const Duration(milliseconds: 500));
        bob.clearCallbackReceived('onGroupInvited');
        final inviteResult = await alice.runWithInstanceAsync(() async =>
            TIMGroupManager.instance.inviteUserToGroup(
                groupID: conferenceId, userList: [bob.getPublicKey()]));
        expect(inviteResult.code, equals(0),
            reason: 'inviteUserToGroup failed: ${inviteResult.code}');
        await waitUntilWithPump(
          () => bob.callbackReceived['onGroupInvited'] == true,
          timeout: const Duration(seconds: 12),
          description: 'Bob receives onGroupInvited for conference type test',
          iterationsPerPump: 100,
          stepDelay: const Duration(milliseconds: 200),
        );
        final joinConferenceId =
            bob.getLastCallbackGroupId('onGroupInvited') ?? conferenceId;
        expectedGroupIds.add(joinConferenceId);
        final joinResult = await bob.runWithInstanceAsync(() async => TIMManager
            .instance
            .joinGroup(groupID: joinConferenceId, message: ''));
        expect(joinResult.code, equals(0),
            reason: 'joinGroup failed: ${joinResult.code}');
        await pumpGroupPeerDiscovery(alice, bob,
            duration: const Duration(seconds: 3));
        final bobInConference = await waitUntilFounderSeesMemberInGroup(
            alice, bob, conferenceId,
            timeout: const Duration(seconds: 20));
        expect(bobInConference, isNotNull,
            reason: 'Alice must see Bob in conference before sending');
        final sendResult = await alice.runWithInstanceAsync(() async {
          final textResult = TIMMessageManager.instance
              .createTextMessage(text: conferenceMessage);
          return TIMMessageManager.instance.sendMessage(
              message: textResult.messageInfo!,
              receiver: null,
              groupID: conferenceId);
        });
        expect(sendResult.code, equals(0));
        pumpAllInstancesOnce(iterations: 150);
        await Future.delayed(const Duration(milliseconds: 500));
        await waitUntilWithPump(
          () => bobReceivedMessage,
          timeout: const Duration(seconds: 25),
          description: 'Bob receives conference message',
          iterationsPerPump: 120,
          stepDelay: const Duration(milliseconds: 200),
        );
        expect(bobReceivedMessage, isTrue);
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: bobListener));
      }
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Both types: member list synchronization', () async {
      final groupResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Group Member Test',
                groupID: '',
              ));
      expect(groupResult.code, equals(0));
      final groupId = groupResult.data!;
      await bob.runWithInstanceAsync(() async =>
          TIMManager.instance.joinGroup(groupID: groupId, message: ''));
      await Future.delayed(const Duration(milliseconds: 500));
      final groupMemberResult = await bob.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getGroupMemberList(
                groupID: groupId,
                filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
                nextSeq: '0',
              ));
      expect(groupMemberResult.code, equals(0),
          reason: 'getGroupMemberList failed: ${groupMemberResult.code}');
      expect(groupMemberResult.data, isNotNull);
      expect(groupMemberResult.data!.memberInfoList, isNotNull);
      expect(groupMemberResult.data!.memberInfoList!.length, greaterThan(0));
      final conferenceResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'Meeting',
                groupName: 'Conference Member Test',
                groupID: '',
              ));
      expect(conferenceResult.code, equals(0));
      final conferenceId = conferenceResult.data!;
      await bob.runWithInstanceAsync(() async =>
          TIMManager.instance.joinGroup(groupID: conferenceId, message: ''));
      await Future.delayed(const Duration(milliseconds: 500));
      final conferenceMemberResult = await bob.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getGroupMemberList(
                groupID: conferenceId,
                filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
                nextSeq: '0',
              ));
      expect(conferenceMemberResult.code, equals(0),
          reason: 'getGroupMemberList failed: ${conferenceMemberResult.code}');
      expect(conferenceMemberResult.data, isNotNull);
      expect(conferenceMemberResult.data!.memberInfoList, isNotNull);
      expect(
          conferenceMemberResult.data!.memberInfoList!.length, greaterThan(0));
    }, timeout: const Timeout(Duration(seconds: 60)));

    test('Both types: get joined group list', () async {
      final groupResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Group List Test',
                groupID: '',
              ));
      expect(groupResult.code, equals(0));
      final conferenceResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'Meeting',
                groupName: 'Conference List Test',
                groupID: '',
              ));
      expect(conferenceResult.code, equals(0));
      await Future.delayed(const Duration(milliseconds: 500));
      final joinedListResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getJoinedGroupList());
      expect(joinedListResult.code, equals(0),
          reason: 'getJoinedGroupList failed: ${joinedListResult.code}');
      expect(joinedListResult.data, isNotNull);
      final groupIds = joinedListResult.data!.map((g) => g.groupID).toList();
      expect(groupIds, contains(groupResult.data));
      expect(groupIds, contains(conferenceResult.data));
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
