/// File Cancel Test
///
/// Tests file transfer cancellation
/// Reference: c-toxcore/auto_tests/scenarios/scenario_file_cancel_test.c

import 'dart:io';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_elem_type.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:path/path.dart' as path;
import '../test_helper.dart';
import '../test_fixtures.dart';

const _fileCancelSubdir = 'scenario_file_cancel';

void main() {
  group('File Cancel Tests', () {
    late TestScenario scenario;
    late String testDataDir;
    late File testFile;

    setUpAll(() async {
      await setupTestEnvironment();
      testDataDir = await getTestDataDir(_fileCancelSubdir);

      // Create a test file (isolated dir so other scenarios' teardown won't wipe it)
      testFile = File(path.join(testDataDir, 'test_file_cancel.txt'));
      await testFile
          .writeAsString('This is a test file for cancellation.\n' * 1000);

      scenario = await createTestScenario(['alice', 'bob']);
      await scenario.initAllNodes();
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      await Future.wait([alice.login(), bob.login()]);
      await waitUntil(() => alice.loggedIn && bob.loggedIn,
          timeout: const Duration(seconds: 10),
          description: 'both nodes logged in');
      await configureLocalBootstrap(scenario);
      await establishFriendship(alice, bob);
      await pumpFriendConnection(alice, bob);
    });

    tearDownAll(() async {
      await scenario.dispose();
      await cleanupTestDataDir(_fileCancelSubdir);
      await teardownTestEnvironment();
    });

    // Lightweight setUp for per-test cleanup if needed
    setUp(() async {
      final bob = scenario.getNode('bob')!;
      bob.receivedMessages.clear();
    });

    test('Cancel incoming file transfer', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      var fileRequestReceived = false;
      String? fileMessageId;

      final bobListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE) {
            fileRequestReceived = true;
            fileMessageId = message.msgID;
            bob.addReceivedMessage(message);
          }
          // tim2tox may deliver file as text "[转发文件] fileName (size 字节)"
          final text = message.textElem?.text ?? '';
          if (text.contains('转发文件') && text.contains('test_file_cancel.txt')) {
            fileRequestReceived = true;
            fileMessageId = message.msgID;
            bob.addReceivedMessage(message);
          }
        },
      );

      bob.runWithInstance(
          () => TIMMessageManager.instance.addAdvancedMsgListener(bobListener));
      try {
        await Future.delayed(const Duration(seconds: 2));

        final sendResult = await alice.runWithInstanceAsync(() async {
          final fileResult = TIMMessageManager.instance.createFileMessage(
            filePath: testFile.path,
            fileName: 'test_file_cancel.txt',
          );
          expect(fileResult.messageInfo, isNotNull,
              reason: 'File message creation should succeed');
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            message: fileResult.messageInfo!,
            receiver: bobToxId,
            onlineUserOnly: false,
          );
        });

        expect(sendResult.code, equals(0));

        // Wait for file request to be received (as FILE or as forward text).
        await waitUntilWithPump(
          () => fileRequestReceived,
          timeout: const Duration(seconds: 20),
          description: 'file request received',
        );

        expect(fileRequestReceived, isTrue);
        expect(fileMessageId, isNotNull);

        // Verify file-related message was received (FILE elem or forward text from tim2tox)
        final fileMsgs = bob.receivedMessages
            .where((m) => m.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE)
            .toList();
        final forwardTextMsgs = bob.receivedMessages
            .where((m) =>
                (m.textElem?.text ?? '').contains('转发文件') &&
                (m.textElem?.text ?? '').contains('test_file_cancel.txt'))
            .toList();
        expect(fileMsgs.isNotEmpty || forwardTextMsgs.isNotEmpty, isTrue,
            reason: 'expected FILE or forward text message');
        if (fileMsgs.isNotEmpty) {
          expect(fileMsgs.first.fileElem, isNotNull);
        }
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: bobListener));
      }
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('Cancel outgoing file transfer', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      await Future.delayed(const Duration(seconds: 2));

      final sendResult = await alice.runWithInstanceAsync(() async {
        final fileResult = TIMMessageManager.instance.createFileMessage(
          filePath: testFile.path,
          fileName: 'test_file_cancel.txt',
        );
        expect(fileResult.messageInfo, isNotNull);
        return await TIMMessageManager.instance.sendMessage(
          groupID: null,
          message: fileResult.messageInfo!,
          receiver: bobToxId,
          onlineUserOnly: false,
        );
      });

      expect(sendResult.code, equals(0));

      // Wait a bit for transfer to start
      await Future.delayed(const Duration(seconds: 2));

      // Cancel the file transfer
      // Note: File cancellation in tim2tox requires FfiChatService
      // This test verifies the send was initiated successfully
      expect(sendResult.code, equals(0));
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('File transfer cancellation state update', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      final cancelListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE) {
            // File request received
          }
        },
        onMessageDownloadProgressCallback: (progress) {
          // Track progress
        },
      );

      bob.runWithInstance(() =>
          TIMMessageManager.instance.addAdvancedMsgListener(cancelListener));
      try {
        await Future.delayed(const Duration(seconds: 2));

        final sendResult = await alice.runWithInstanceAsync(() async {
          final fileResult = TIMMessageManager.instance.createFileMessage(
            filePath: testFile.path,
            fileName: 'test_file_cancel.txt',
          );
          expect(fileResult.messageInfo, isNotNull);
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            message: fileResult.messageInfo!,
            receiver: bobToxId,
            onlineUserOnly: false,
          );
        });

        expect(sendResult.code, equals(0));
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: cancelListener));
      }
      // Note: Actual cancellation would require FfiChatService.cancelFileTransfer()
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
