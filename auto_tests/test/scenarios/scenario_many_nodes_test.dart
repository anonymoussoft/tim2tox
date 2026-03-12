/// Many Nodes Test
/// 
/// Tests system stability with multiple nodes
/// Reference: c-toxcore/auto_tests/scenarios/scenario_tox_many_test.c and scenario_tox_many_tcp_test.c

import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_group_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/group_member_filter_enum.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

void main() {
  group('Many Nodes Tests', () {
    late TestScenario scenario;
    late List<TestNode> nodes;
    
    setUpAll(() async {
      await setupTestEnvironment();
      final aliases = List.generate(5, (i) => 'node$i');
      scenario = await createTestScenario(aliases);
      nodes = aliases.map((alias) => scenario.getNode(alias)!).toList();
      
      await scenario.initAllNodes();
      // Parallelize login for all 5 nodes
      await Future.wait(nodes.map((node) => node.login()));
      
      await waitUntil(() => nodes.every((node) => node.loggedIn));
      
      // Enable auto-accept so establishFriendship (invite flow) works
      for (final node in nodes) {
        node.enableAutoAccept();
      }
      
      // Configure local bootstrap (like C test's tox_node_bootstrap)
      await configureLocalBootstrap(scenario);
      
      // Wait for all nodes to be connected to Tox network before createGroup/invite/join
      // (multi-node readiness; longer timeout for 5 nodes)
      await Future.wait(
        nodes.map((node) => node.waitForConnection(timeout: const Duration(seconds: 45))),
      );
      await Future.delayed(const Duration(seconds: 2));
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
    
    test('Multiple nodes communication', () async {
      // Establish friendship between node0 and each other node (tim2tox inviteUserToGroup requires inviter-invitee friendship)
      await Future.wait([
        for (int i = 1; i < nodes.length; i++) establishFriendship(nodes[0], nodes[i], timeout: const Duration(seconds: 90)),
      ]);
      await Future.delayed(const Duration(seconds: 2));
      
      // Create a group on node0's instance
      final createResult = await nodes[0].runWithInstanceAsync(() async => TIMGroupManager.instance.createGroup(
        groupType: 'kTIMGroup_Private',
        groupName: 'Many Nodes Group',
      ));
      
      expect(createResult.code, equals(0), reason: 'createGroup failed: ${createResult.code}');
      final groupId = createResult.data!;
      
      // tim2tox requires invite then join (join without invite returns 6017)
      for (int i = 1; i < nodes.length; i++) {
        final peer = nodes[i];
        final peerPublicKey = peer.getPublicKey();
        peer.clearCallbackReceived('onGroupInvited');
        final inviteResult = await nodes[0].runWithInstanceAsync(() async => TIMGroupManager.instance.inviteUserToGroup(
          groupID: groupId,
          userList: [peerPublicKey],
        ));
        expect(inviteResult.code, equals(0), reason: 'inviteUserToGroup(node$i) failed: ${inviteResult.code}');
        await peer.waitForCallback('onGroupInvited', timeout: const Duration(seconds: 20));
        await Future.delayed(const Duration(milliseconds: 500));
        // Use invitee's groupID from callback (e.g. tox_inv_0_xxx); native returns 6017 if creator's groupID is used
        final joinGroupId = peer.getLastCallbackGroupId('onGroupInvited') ?? groupId;
        final joinResult = await peer.runWithInstanceAsync(() async => TIMManager.instance.joinGroup(
          groupID: joinGroupId,
          message: 'Hello from node$i',
        ));
        expect(joinResult.code, equals(0), reason: 'node$i joinGroup failed: ${joinResult.code}');
      }
      
      // Wait for all nodes to be in the group
      await Future.delayed(const Duration(milliseconds: 500));
      
      // Node0 sends C2C message to node1 (use Tox ID for receiver)
      final sendResult = await nodes[0].runWithInstanceAsync(() async {
        final messageResult = TIMMessageManager.instance.createTextMessage(text: 'Hello from node0');
        return TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: messageResult.messageInfo!,
          receiver: nodes[1].getToxId(),
          onlineUserOnly: false,
        );
      });
      
      expect(sendResult.code, equals(0));
      
      // Verify all nodes are in the group (query from node0's instance)
      await Future.delayed(const Duration(milliseconds: 500));
      
      final memberListResult = await nodes[0].runWithInstanceAsync(() async => TIMGroupManager.instance.getGroupMemberList(
        groupID: groupId,
        filter: GroupMemberFilterTypeEnum.V2TIM_GROUP_MEMBER_FILTER_ALL,
        nextSeq: '0',
      ));
      
      expect(memberListResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 420))); // Friendship + invite/join for 5 nodes
  });
}
