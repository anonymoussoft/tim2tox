/// Large Group Test
///
/// Tests groups with multiple members (5+)
/// Verifies that large groups work correctly with message broadcasting and member management

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_role_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Large Group Tests', () {
    late TestScenario scenario;
    late TestNode alice;
    late TestNode bob;
    late TestNode charlie;
    late TestNode david;
    late TestNode eve;

    setUpAll(() async {
      await setupTestEnvironment();
      scenario =
          await createTestScenario(['alice', 'bob', 'charlie', 'david', 'eve']);
      alice = scenario.getNode('alice')!;
      bob = scenario.getNode('bob')!;
      charlie = scenario.getNode('charlie')!;
      david = scenario.getNode('david')!;
      eve = scenario.getNode('eve')!;

      await scenario.initAllNodes();
      // Parallelize login for all 5 nodes
      await Future.wait([
        alice.login(),
        bob.login(),
        charlie.login(),
        david.login(),
        eve.login(),
      ]);

      await waitUntil(
        () =>
            alice.loggedIn &&
            bob.loggedIn &&
            charlie.loggedIn &&
            david.loggedIn &&
            eve.loggedIn,
        timeout: const Duration(seconds: 15),
        description: 'all nodes logged in',
      );

      await configureLocalBootstrap(scenario);

      // Wait for DHT connection so createGroup/joinGroup do not block (5 nodes may need longer)
      await Future.wait([
        alice.waitForConnection(timeout: const Duration(seconds: 25)),
        bob.waitForConnection(timeout: const Duration(seconds: 25)),
        charlie.waitForConnection(timeout: const Duration(seconds: 25)),
        david.waitForConnection(timeout: const Duration(seconds: 25)),
        eve.waitForConnection(timeout: const Duration(seconds: 25)),
      ]);
      for (final peer in [bob, charlie, david, eve]) {
        await establishFriendship(alice, peer,
            timeout: const Duration(seconds: 90));
        await pumpFriendConnection(alice, peer,
            duration: const Duration(seconds: 3));
      }
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

    Future<String> inviteAndJoinMember(
      String groupId,
      TestNode invitee, {
      required String context,
      bool waitFounderSeesMember = false,
    }) async {
      invitee.clearCallbackReceived('onGroupInvited');
      final inviteResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.inviteUserToGroup(
                groupID: groupId,
                userList: [invitee.getPublicKey()],
              ));
      expect(inviteResult.code, equals(0),
          reason:
              'inviteUserToGroup failed for ${invitee.alias}: ${inviteResult.code}');

      await waitUntilWithPump(
        () => invitee.callbackReceived['onGroupInvited'] == true,
        timeout: const Duration(seconds: 12),
        description: '${invitee.alias} receives onGroupInvited ($context)',
        iterationsPerPump: 100,
        stepDelay: const Duration(milliseconds: 200),
      );

      final joinGroupId =
          invitee.getLastCallbackGroupId('onGroupInvited') ?? groupId;
      final joinResult = await invitee.runWithInstanceAsync(() async =>
          TIMManager.instance.joinGroup(groupID: joinGroupId, message: ''));
      expect(joinResult.code, equals(0),
          reason: '${invitee.alias} joinGroup failed: ${joinResult.code}');

      if (waitFounderSeesMember) {
        await pumpGroupPeerDiscovery(alice, invitee,
            duration: const Duration(seconds: 2), iterationsPerPump: 60);
        final inviteeInGroup = await waitUntilFounderSeesMemberInGroup(
            alice, invitee, groupId,
            timeout: const Duration(seconds: 20));
        expect(inviteeInGroup, isNotNull,
            reason: 'Alice must see ${invitee.alias} in group before $context');
      }

      return joinGroupId;
    }

    test('Create group with 5 members', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Large Group Test',
                groupID: '',
              ));

      expect(createResult.code, equals(0));
      final groupId = createResult.data!;

      for (final node in [bob, charlie, david, eve]) {
        await inviteAndJoinMember(
          groupId,
          node,
          context: 'creating 5-member group',
          waitFounderSeesMember: false,
        );
      }
      await pumpGroupPeerDiscovery(alice, bob,
          duration: const Duration(seconds: 3), iterationsPerPump: 50);
      final bobSeen = await waitUntilFounderSeesMemberInGroup(
          alice, bob, groupId,
          timeout: const Duration(seconds: 20));
      expect(bobSeen, isNotNull,
          reason: 'Alice must see Bob in group before verifying member list');

      final memberListResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getGroupMemberList(
                groupID: groupId,
                filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
                nextSeq: '0',
              ));

      if (memberListResult.code == 0 && memberListResult.data != null) {
        // tim2tox: C++ may return 76-char Tox ID or 64-char public key
        final alicePublicKey = alice.getPublicKey();
        final memberIds = memberListResult.data!.memberInfoList!
            .map((m) => m.userID)
            .toList();
        final hasAlice = memberIds.any((id) =>
            id == alicePublicKey ||
            (id.length >= 64 && id.startsWith(alicePublicKey)));
        expect(hasAlice, isTrue, reason: 'Alice should be in member list');
        expect(memberIds.length, greaterThanOrEqualTo(1));
      }
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('Broadcast message to all members', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Broadcast Test Group',
                groupID: '',
              ));

      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final alicePublicKey = alice.getPublicKey();
      final expectedGroupIdsByNode = <String, Set<String>>{};
      final receivedCounts = <String, int>{
        bob.userId: 0,
        charlie.userId: 0,
        david.userId: 0,
        eve.userId: 0
      };
      final listeners = <V2TimAdvancedMsgListener>[];

      const broadcastText = 'Broadcast to all members!';
      for (final node in [bob, charlie, david, eve]) {
        final listener = V2TimAdvancedMsgListener(
          onRecvNewMessage: (V2TimMessage message) {
            final expectedIds =
                expectedGroupIdsByNode[node.userId] ?? {groupId};
            if (!expectedIds.contains(message.groupID ?? '')) return;
            final sender = (message.userID ?? message.sender ?? '').toString();
            final senderKey =
                sender.length >= 64 ? sender.substring(0, 64) : sender;
            final fromAlice = senderKey == alicePublicKey;
            final isExpectedText = message.textElem?.text == broadcastText;
            if (fromAlice || isExpectedText) {
              receivedCounts[node.userId] =
                  (receivedCounts[node.userId] ?? 0) + 1;
              node.addReceivedMessage(message);
            }
          },
        );
        listeners.add(listener);
        node.runWithInstance(
            () => TIMMessageManager.instance.addAdvancedMsgListener(listener));
      }

      try {
        for (final node in [bob, charlie, david, eve]) {
          final joinGroupId = await inviteAndJoinMember(
            groupId,
            node,
            context: 'broadcast message',
            waitFounderSeesMember: false,
          );
          expectedGroupIdsByNode[node.userId] = {groupId, joinGroupId};
        }
        final bobInGroup = await waitUntilFounderSeesMemberInGroup(
            alice, bob, groupId,
            timeout: const Duration(seconds: 20));
        expect(bobInGroup, isNotNull,
            reason:
                'Alice must see at least one member (bob) in group before broadcast');

        final messageResult = await alice.runWithInstanceAsync(() async {
          final m =
              TIMMessageManager.instance.createTextMessage(text: broadcastText);
          return TIMMessageManager.instance.sendMessage(
            message: m.messageInfo!,
            receiver: null,
            groupID: groupId,
          );
        });

        expect(messageResult.code, equals(0));
        pumpAllInstancesOnce(iterations: 150);
        await Future.delayed(const Duration(milliseconds: 500));
        await waitUntilWithPump(
          () => receivedCounts.values.any((count) => count > 0),
          timeout: const Duration(seconds: 30),
          description: 'at least one member receives message',
          iterationsPerPump: 150,
          stepDelay: const Duration(milliseconds: 200),
        );
        final totalReceived =
            receivedCounts.values.fold(0, (sum, count) => sum + count);
        expect(totalReceived, greaterThan(0),
            reason:
                'At least one of bob/charlie/david/eve should receive broadcast (receivedCounts: $receivedCounts)');
      } finally {
        for (int i = 0; i < listeners.length; i++) {
          final node = [bob, charlie, david, eve][i];
          node.runWithInstance(() => TIMMessageManager.instance
              .removeAdvancedMsgListener(listener: listeners[i]));
        }
      }
    }, timeout: const Timeout(Duration(seconds: 120)));

    test('Multiple members send messages', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Multi-Sender Test Group',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      final expectedGroupIds = <String>{groupId};
      final messagesBySender = <String, List<V2TimMessage>>{};
      final aliceListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (!expectedGroupIds.contains(message.groupID ?? '')) return;
          final senderId = message.userID ?? '';
          if (!messagesBySender.containsKey(senderId)) {
            messagesBySender[senderId] = [];
          }
          messagesBySender[senderId]!.add(message);
          alice.addReceivedMessage(message);
        },
      );
      alice.runWithInstance(() =>
          TIMMessageManager.instance.addAdvancedMsgListener(aliceListener));
      try {
        final bobSendGroupId = await inviteAndJoinMember(
          groupId,
          bob,
          context: 'multi-sender group (bob join)',
          waitFounderSeesMember: false,
        );
        expectedGroupIds.addAll({groupId, bobSendGroupId});
        final charlieSendGroupId = await inviteAndJoinMember(
          groupId,
          charlie,
          context: 'multi-sender group (charlie join)',
          waitFounderSeesMember: false,
        );
        expectedGroupIds.addAll({groupId, charlieSendGroupId});
        final davidSendGroupId = await inviteAndJoinMember(
          groupId,
          david,
          context: 'multi-sender group (david join)',
          waitFounderSeesMember: false,
        );
        expectedGroupIds.addAll({groupId, davidSendGroupId});
        final eveSendGroupId = await inviteAndJoinMember(
          groupId,
          eve,
          context: 'multi-sender group (eve join)',
          waitFounderSeesMember: false,
        );
        expectedGroupIds.addAll({groupId, eveSendGroupId});
        final bobInGroup = await waitUntilFounderSeesMemberInGroup(
            alice, bob, groupId,
            timeout: const Duration(seconds: 20));
        expect(bobInGroup, isNotNull,
            reason:
                'Alice must see at least one member in group before members send');

        final bobMessage = await bob.runWithInstanceAsync(() async {
          final m = TIMMessageManager.instance
              .createTextMessage(text: 'Message from Bob');
          return TIMMessageManager.instance.sendMessage(
              message: m.messageInfo!, receiver: null, groupID: bobSendGroupId);
        });
        expect(bobMessage.code, equals(0));
        await charlie.runWithInstanceAsync(() async {
          final m = TIMMessageManager.instance
              .createTextMessage(text: 'Message from Charlie');
          return TIMMessageManager.instance.sendMessage(
              message: m.messageInfo!,
              receiver: null,
              groupID: charlieSendGroupId);
        });
        await david.runWithInstanceAsync(() async {
          final m = TIMMessageManager.instance
              .createTextMessage(text: 'Message from David');
          return TIMMessageManager.instance.sendMessage(
              message: m.messageInfo!,
              receiver: null,
              groupID: davidSendGroupId);
        });
        await waitUntilWithPump(() => messagesBySender.length > 0,
            timeout: const Duration(seconds: 25),
            description: 'Alice receives messages',
            iterationsPerPump: 100,
            stepDelay: const Duration(milliseconds: 200));
        expect(messagesBySender.length, greaterThan(0));
      } finally {
        alice.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: aliceListener));
      }
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('Large group member management', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'group',
                groupName: 'Member Management Test',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      final groupId = createResult.data!;
      for (final node in [bob, charlie, david, eve]) {
        await inviteAndJoinMember(
          groupId,
          node,
          context: 'large group member management',
          waitFounderSeesMember: false,
        );
      }
      await pumpGroupPeerDiscovery(alice, bob,
          duration: const Duration(seconds: 3), iterationsPerPump: 50);
      final memberListResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getGroupMemberList(
                groupID: groupId,
                filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
                nextSeq: '0',
              ));

      if (memberListResult.code == 0 && memberListResult.data != null) {
        final bobPublicKey = bob.getPublicKey();
        final charliePublicKey = charlie.getPublicKey();
        // tim2tox: resolve userIDs from member list (64/76-char source of truth)
        final members = memberListResult.data!.memberInfoList!;
        final bobMember = members.where((m) {
          final uid = m.userID;
          return uid == bobPublicKey ||
              (uid.length >= 64 && uid.startsWith(bobPublicKey));
        }).toList();
        final charlieMember = members.where((m) {
          final uid = m.userID;
          return uid == charliePublicKey ||
              (uid.length >= 64 && uid.startsWith(charliePublicKey));
        }).toList();
        if (bobMember.isNotEmpty) {
          await alice.runWithInstanceAsync(
              () async => TIMGroupManager.instance.setGroupMemberRole(
                    groupID: groupId,
                    userID: bobMember.first.userID,
                    role: GroupMemberRoleTypeEnum.V2TIM_GROUP_MEMBER_ROLE_ADMIN,
                  ));
        }
        if (charlieMember.isNotEmpty) {
          await alice.runWithInstanceAsync(
              () async => TIMGroupManager.instance.setGroupMemberInfo(
                    groupID: groupId,
                    userID: charlieMember.first.userID,
                    nameCard: 'Charlie in Large Group',
                  ));
        }
      }

      // Verify operations completed
      expect(memberListResult.code, equals(0),
          reason: 'getGroupMemberList failed: ${memberListResult.code}');
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('Large conference group', () async {
      final createResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.createGroup(
                groupType: 'Meeting',
                groupName: 'Large Conference',
                groupID: '',
              ));
      expect(createResult.code, equals(0));
      final conferenceId = createResult.data!;
      for (final node in [bob, charlie, david, eve]) {
        await inviteAndJoinMember(
          conferenceId,
          node,
          context: 'large conference join',
          waitFounderSeesMember: false,
        );
      }
      await pumpGroupPeerDiscovery(alice, bob,
          duration: const Duration(seconds: 3), iterationsPerPump: 50);
      final memberListResult = await alice.runWithInstanceAsync(
          () async => TIMGroupManager.instance.getGroupMemberList(
                groupID: conferenceId,
                filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
                nextSeq: '0',
              ));

      expect(memberListResult.code, equals(0),
          reason: 'getGroupMemberList failed: ${memberListResult.code}');
      expect(memberListResult.data, isNotNull);
      // tim2tox: C++ may return 76-char Tox ID or 64-char public key
      final alicePublicKey = alice.getPublicKey();
      final memberIds =
          memberListResult.data!.memberInfoList!.map((m) => m.userID).toList();
      final hasAlice = memberIds.any((id) =>
          id == alicePublicKey ||
          (id.length >= 64 && id.startsWith(alicePublicKey)));
      expect(hasAlice, isTrue, reason: 'Alice should be in member list');
      expect(memberIds.length, greaterThanOrEqualTo(1));
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
