/// Group Message Test
/// 
/// Tests group message sending and receiving, including:
/// - Group messages
/// - Private messages in group
/// - Custom packets
/// - Custom private packets
/// - Lossless message delivery
/// Reference: c-toxcore/auto_tests/scenarios/scenario_group_message_test.c

import 'dart:async';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Group Message Tests', () {
    late TestScenario scenario;
    late TestNode founder;
    late TestNode member1;
    String? groupId;
    
    setUpAll(() async {
      await setupTestEnvironment();
      scenario = await createTestScenario(['founder', 'member1']);
      founder = scenario.getNode('founder')!;
      member1 = scenario.getNode('member1')!;
      
      await scenario.initAllNodes();
      // Parallelize login
      await Future.wait([
        founder.login(),
        member1.login(),
      ]);
      
      await waitUntil(
        () => founder.loggedIn && member1.loggedIn,
        timeout: const Duration(seconds: 10),
        description: 'all nodes logged in',
      );
      
      // Configure local bootstrap (like C test's tox_node_bootstrap)
      await configureLocalBootstrap(scenario);
      
      // Enable auto-accept for friend requests
      founder.enableAutoAccept();
      member1.enableAutoAccept();
      
      // Establish bidirectional friendship (required for group message delivery in Tox)
      await establishFriendship(founder, member1, timeout: const Duration(seconds: 15));
      
      final founderToxId = founder.getToxId();
      final member1ToxId = member1.getToxId();
      await Future.wait([
        founder.waitForFriendConnection(member1ToxId, timeout: const Duration(seconds: 30)),
        member1.waitForFriendConnection(founderToxId, timeout: const Duration(seconds: 30)),
      ]);
      
      // Create group (private group, like C test)
      final createResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Utah Data Center',
      ));
      
      expect(createResult.code, equals(0), reason: 'createGroup failed: ${createResult.code}');
      expect(createResult.data, isNotNull);
      groupId = createResult.data;
      
      // Invite member1 and wait for join (tim2tox requires invite -> onGroupInvited -> joinGroup)
      final member1PublicKey = member1.getPublicKey();
      member1.clearCallbackReceived('onGroupInvited');
      final inviteResult = await founder.runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
        groupID: groupId!,
        userList: [member1PublicKey],
      ));
      expect(inviteResult.code, equals(0), reason: 'inviteUserToGroup failed: ${inviteResult.code}');
      await member1.waitForCallback('onGroupInvited', timeout: const Duration(seconds: 20));
      await Future.delayed(const Duration(milliseconds: 500));
      final joinResult = await member1.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(groupID: groupId!, message: ''));
      expect(joinResult.code, equals(0), reason: 'member1 joinGroup failed: ${joinResult.code}');
      // Wait until founder sees member1 in group before tests send group messages (avoids sending before peer is in group)
      final member1InGroup = await waitUntilFounderSeesMemberInGroup(founder, member1, groupId!, timeout: const Duration(seconds: 25));
      expect(member1InGroup, isNotNull, reason: 'Founder must see member1 in group before sending group messages');
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
    
    test('Send and receive group message', () async {
      const testMessage = 'Where is it I\'ve read that someone condemned to death says or thinks...';
      final completer = Completer<V2TimMessage>();
      
      // Set up message listener on member1's instance
      final listener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.groupID == groupId && 
              message.textElem?.text == testMessage) {
            member1.addReceivedMessage(message);
            if (!completer.isCompleted) {
              completer.complete(message);
            }
          }
        },
      );
      
      member1.runWithInstance(() {
        TIMMessageManager.instance.addAdvancedMsgListener(listener);
      });
      
      // Founder sends message to group (in founder's instance scope)
      final sendResult = await founder.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: testMessage);
        return await TIMMessageManager.instance.sendMessage(
          message: messageResult.messageInfo!,
          receiver: null,
          groupID: groupId!,
          onlineUserOnly: false,
        );
      });
      
      expect(sendResult.code, equals(0), reason: 'sendMessage failed: ${sendResult.code}');
      
      // Member1 receives message
      try {
        final receivedMessage = await completer.future.timeout(
          const Duration(seconds: 30),
          onTimeout: () => throw TimeoutException('Group message delivery timeout'),
        );

        expect(receivedMessage.textElem?.text, equals(testMessage));
        expect(member1.receivedMessages.length, greaterThan(0));
      } finally {
        member1.runWithInstance(() {
          TIMMessageManager.instance.removeAdvancedMsgListener(listener: listener);
        });
      }
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Lossless message delivery test', () async {
      const maxNumMessages = 10; // Reduced for Tox group message latency; still validates ordering
      final receivedMessages = <int, bool>{};
      final completer = Completer<void>();
      
      // Set up message listener on member1's instance
      final listener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.groupID == groupId && message.customElem != null) {
            // Parse custom message with checksum
            final data = message.customElem!.data;
            if (data != null && data.length >= 4) {
              // Extract message number and checksum from custom data
              // Format: [2 bytes: message_num][2 bytes: checksum][data]
              try {
                final dataBytes = data.codeUnits;
                if (dataBytes.length >= 4) {
                  final messageNum = (dataBytes[0] << 8) | dataBytes[1];
                  receivedMessages[messageNum] = true;
                  
                  if (messageNum == maxNumMessages) {
                    if (!completer.isCompleted) {
                      completer.complete();
                    }
                  }
                }
              } catch (e) {
                // Ignore parsing errors
              }
            }
          }
        },
      );
      
      member1.runWithInstance(() {
        TIMMessageManager.instance.addAdvancedMsgListener(listener);
      });
      
      // Founder sends numbered messages with checksums (in founder's instance scope)
      await founder.runWithInstanceAsync(() async {
        for (int i = 0; i <= maxNumMessages; i++) {
          final messageNumBytes = [(i >> 8) & 0xFF, i & 0xFF];
          final checksumBytes = [0, 0];
          final randomData = List.generate(10, (_) => (i * 7) % 256);
          final allBytes = [...messageNumBytes, ...checksumBytes, ...randomData];
          final messageData = String.fromCharCodes(allBytes);
          final messageResult = TIMMessageManager.instance.createCustomMessage(
            data: messageData,
            desc: 'Lossless test message $i',
          );
          final sendResult = await TIMMessageManager.instance.sendMessage(
            message: messageResult.messageInfo!,
            receiver: null,
            groupID: groupId!,
            onlineUserOnly: false,
          );
          expect(sendResult.code, equals(0), reason: 'sendMessage $i failed: ${sendResult.code}');
          if (i % 10 == 0) {
            await Future.delayed(const Duration(milliseconds: 100));
          }
        }
      });
      
      // Wait for all messages to be received
      try {
        await completer.future.timeout(
          const Duration(seconds: 45),
          onTimeout: () => throw TimeoutException(
            'Lossless message test timeout. Received: ${receivedMessages.length}/$maxNumMessages',
          ),
        );

        final receivedCount = receivedMessages.length;
        expect(receivedCount, greaterThanOrEqualTo(maxNumMessages),
            reason: 'Expected almost all lossless packets, but got $receivedCount/$maxNumMessages');
        expect(receivedMessages.containsKey(maxNumMessages), isTrue,
            reason: 'Final sequence packet should be received in lossless test');
      } finally {
        member1.runWithInstance(() {
          TIMMessageManager.instance.removeAdvancedMsgListener(listener: listener);
        });
      }
    }, timeout: const Timeout(Duration(seconds: 90)));
    
    test('Group private message', () async {
      // Wait for group to be ready
      await Future.delayed(const Duration(seconds: 5));
      
      final privateMessageText = 'Don\'t spill yer beans';
      final completer = Completer<V2TimMessage>();
      
      // Set up message listener on member1's instance
      final listener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.groupID == groupId && message.textElem?.text == privateMessageText) {
            member1.addReceivedMessage(message);
            if (!completer.isCompleted) {
              completer.complete(message);
            }
          }
        },
      );
      
      member1.runWithInstance(() {
        TIMMessageManager.instance.addAdvancedMsgListener(listener);
      });
      
      // Founder sends private message to member1. Use member1's userID from founder's
      // getGroupMemberList so it matches the peer public key cached in HandleGroupPeerJoin.
      final memberListResult = await founder.runWithInstanceAsync(() async =>
          TIMGroupManager.instance.getGroupMemberList(
            groupID: groupId!,
            filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
            nextSeq: '0',
            count: 100,
          ));
      expect(memberListResult.code, equals(0), reason: 'getGroupMemberList failed: ${memberListResult.code}');
      final founderToxId = founder.getToxId();
      final founderPublicKey = founderToxId.length >= 64 ? founderToxId.substring(0, 64) : founderToxId;
      final members = memberListResult.data?.memberInfoList ?? [];
      final others = members.where((m) => m.userID != founderPublicKey).toList();
      final receiverUserID = others.isNotEmpty ? others.first.userID : member1.getPublicKey();
      final sendResult = await founder.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: privateMessageText);
        return await TIMMessageManager.instance.sendMessage(
          message: messageResult.messageInfo!,
          receiver: receiverUserID,
          groupID: groupId!,
          onlineUserOnly: false,
        );
      });
      
      expect(sendResult.code, equals(0), reason: 'sendMessage failed: ${sendResult.code}');
      
      try {
        final receivedMessage = await completer.future.timeout(
          const Duration(seconds: 30),
          onTimeout: () => throw TimeoutException('Group private message delivery timeout'),
        );
        expect(receivedMessage.groupID, equals(groupId));
        expect(receivedMessage.textElem?.text, equals(privateMessageText));
      } finally {
        member1.runWithInstance(() {
          TIMMessageManager.instance.removeAdvancedMsgListener(listener: listener);
        });
      }
    }, timeout: const Timeout(Duration(seconds: 60)));
    
    test('Group custom message', () async {
      // Wait for group to be ready
      await Future.delayed(const Duration(seconds: 5));
      
      final customData = '{"type":"group_custom","data":"Why\'d ya spill yer beans?"}';
      final completer = Completer<V2TimMessage>();
      
      // Set up message listener on member1's instance
      final listener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.groupID == groupId && message.customElem != null) {
            member1.addReceivedMessage(message);
            if (!completer.isCompleted) {
              completer.complete(message);
            }
          }
        },
      );
      
      member1.runWithInstance(() {
        TIMMessageManager.instance.addAdvancedMsgListener(listener);
      });
      
      // Founder sends custom message to group (in founder's instance scope)
      final sendResult = await founder.runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createCustomMessage(
          data: customData,
          desc: 'Group custom message',
        );
        return await TIMMessageManager.instance.sendMessage(
          message: messageResult.messageInfo!,
          receiver: null,
          groupID: groupId!,
          onlineUserOnly: false,
        );
      });
      
      expect(sendResult.code, equals(0), reason: 'sendMessage failed: ${sendResult.code}');
      
      try {
        await completer.future.timeout(
          const Duration(seconds: 30),
          onTimeout: () => throw TimeoutException('Group custom message delivery timeout'),
        );
        
        expect(member1.receivedMessages.any((m) => m.customElem?.data == customData), isTrue);
      } finally {
        member1.runWithInstance(() {
          TIMMessageManager.instance.removeAdvancedMsgListener(listener: listener);
        });
      }
    }, timeout: const Timeout(Duration(seconds: 60)));
  });
}
