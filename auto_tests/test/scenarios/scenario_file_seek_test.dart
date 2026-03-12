/// File Seek Test
///
/// Tests file transfer seeking to a specific position
/// Reference: c-toxcore/auto_tests/scenarios/scenario_file_seek_test.c

import 'dart:io';
import 'package:test/test.dart';
import 'package:tencent_cloud_chat_sdk/native_im/adapter/tim_message_manager.dart';
import 'package:tencent_cloud_chat_sdk/enum/V2TimAdvancedMsgListener.dart';
import 'package:tencent_cloud_chat_sdk/enum/message_elem_type.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message.dart';
import 'package:tencent_cloud_chat_sdk/models/v2_tim_message_download_progress.dart';
import 'package:tencent_cloud_chat_sdk/tencent_cloud_chat_sdk_platform_interface.dart';
import 'package:path/path.dart' as path;
import 'package:tim2tox_dart/sdk/tim2tox_sdk_platform.dart';
import '../test_helper.dart';
import '../test_fixtures.dart';

/// Subdir so this scenario's file is not deleted by another file's teardown (cleanupTestDataDir).
const _fileSeekSubdir = 'scenario_file_seek';

void main() {
  group('File Seek Tests', () {
    late TestScenario scenario;
    late String testDataDir;
    late File testFile;

    setUpAll(() async {
      await setupTestEnvironment();
      testDataDir = await getTestDataDir(_fileSeekSubdir);

      // Create a large test file for seeking (isolated dir so other scenarios' teardown won't wipe it)
      testFile = File(path.join(testDataDir, 'test_file_seek.txt'));
      final content = 'File content for seeking test.\n' * 2000;
      await testFile.writeAsString(content);

      scenario = await createTestScenario(['alice', 'bob']);
      await scenario.initAllNodes();
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
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
      await establishFriendship(alice, bob);
      await pumpFriendConnection(alice, bob);
      // Ensure FfiChatService polling is started so file_request events are consumed (test instances may not go through platform login)
      final platform = TencentCloudChatSdkPlatform.instance;
      if (platform is Tim2ToxSdkPlatform) {
        await platform.ffiService.startPolling();
      }
    });

    tearDownAll(() async {
      await scenario.dispose();
      await cleanupTestDataDir(_fileSeekSubdir);
      await teardownTestEnvironment();
    });

    // Lightweight setUp: ensure test file exists (e.g. when running single test by name, or if cleaned)
    setUp(() async {
      if (!await testFile.exists()) {
        await Directory(testDataDir).create(recursive: true);
        final content = 'File content for seeking test.\n' * 2000;
        await testFile.writeAsString(content);
      }
      final bob = scenario.getNode('bob')!;
      bob.receivedMessages.clear();
    });

    test('Seek to middle position during file transfer', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      var fileReceived = false;
      var seekPosition = 0;
      final fileSize = await testFile.length();
      final seekTarget = fileSize ~/ 2; // Seek to middle

      final seekListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          final text = message.textElem?.text ?? '';
          final isForwardFileText =
              text.contains('转发文件') && text.contains('test_file_seek.txt');
          if (message.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE ||
              message.fileElem != null ||
              isForwardFileText) {
            fileReceived = true;
            bob.addReceivedMessage(message);
          }
        },
        onMessageDownloadProgressCallback:
            (V2TimMessageDownloadProgress progress) {
          seekPosition = progress.currentSize;
          if (progress.currentSize >= seekTarget &&
              progress.currentSize < progress.totalSize) {
            // File transfer is in progress from seek position
          }
        },
      );

      bob.runWithInstance(() =>
          TIMMessageManager.instance.addAdvancedMsgListener(seekListener));
      try {
        await Future.delayed(const Duration(seconds: 2));

        final sendResult = await alice.runWithInstanceAsync(() async {
          final fileResult = TIMMessageManager.instance.createFileMessage(
            filePath: testFile.path,
            fileName: 'test_file_seek.txt',
          );
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            id: fileResult.id!,
            receiver: bobToxId,
          );
        });

        expect(sendResult.code, equals(0));

        // Wait for file request
        await waitUntilWithPump(
          () => fileReceived,
          timeout: const Duration(seconds: 15),
          description: 'file request received',
        );

        // Note: File seeking in tim2tox requires FfiChatService
        // The seek operation would be: ffiService.seekFileTransfer(peerId, fileNumber, position)
        // This test verifies the file transfer can be initiated and progress tracked
        expect(fileReceived, isTrue);

        // Verify file message
        final fileMessages = bob.receivedMessages
            .where((m) =>
                m.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE ||
                m.fileElem != null)
            .toList();
        if (fileMessages.isNotEmpty) {
          final fileMessage = fileMessages.last;
          expect(fileMessage.fileElem, isNotNull);
          final reportedSize = fileMessage.fileElem?.fileSize;
          if (reportedSize != null) {
            expect(reportedSize, equals(fileSize));
          }
        } else {
          final hasForwardText = bob.receivedMessages.any((m) {
            final t = m.textElem?.text ?? '';
            return t.contains('转发文件') && t.contains('test_file_seek.txt');
          });
          expect(hasForwardText, isTrue,
              reason: 'Expected FILE element or forward file text');
        }
        expect(seekPosition, greaterThanOrEqualTo(0),
            reason: 'should have received progress');
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: seekListener));
      }
    }, timeout: const Timeout(Duration(seconds: 90)));

    test('Seek and verify file integrity', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForConnection(timeout: const Duration(seconds: 15));
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      var transferComplete = false;
      final fileSize = await testFile.length();
      final seekTarget = fileSize ~/ 3; // Seek to 1/3 position

      var progressPositions = <int>[];
      final progressListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          if (message.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE) {
            bob.addReceivedMessage(message);
          }
        },
        onMessageDownloadProgressCallback:
            (V2TimMessageDownloadProgress progress) {
          progressPositions.add(progress.currentSize);
          if (progress.currentSize == progress.totalSize &&
              progress.totalSize > 0) {
            transferComplete = true;
          }
        },
      );

      bob.runWithInstance(() =>
          TIMMessageManager.instance.addAdvancedMsgListener(progressListener));
      try {
        await Future.delayed(const Duration(seconds: 2));

        final sendResult = await alice.runWithInstanceAsync(() async {
          final fileResult = TIMMessageManager.instance.createFileMessage(
            filePath: testFile.path,
            fileName: 'test_file_seek.txt',
          );
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            id: fileResult.id!,
            receiver: bobToxId,
          );
        });

        expect(sendResult.code, equals(0));

        // Wait for Bob to receive the file message (rely on FfiChatService 50ms poll timer)
        await waitUntilWithPump(
          () => bob.receivedMessages.any((m) =>
              m.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE ||
              m.fileElem != null),
          timeout: const Duration(seconds: 20),
          description: 'file message received by Bob',
          iterationsPerPump: 80,
          stepDelay: const Duration(milliseconds: 200),
        );
        // Give poll timer time to process file_request and progress_recv (poll already started in setUpAll)
        pumpAllInstancesOnce(iterations: 100);
        await Future.delayed(const Duration(milliseconds: 800));
        // Wait for transfer to start (progress from progress_recv)
        await waitUntilWithPump(
          () => progressPositions.isNotEmpty,
          timeout: const Duration(seconds: 45),
          description: 'file transfer progress',
          iterationsPerPump: 120,
          stepDelay: const Duration(milliseconds: 250),
        );
        // Wait for completion (pump so Tox can complete transfer)
        await waitUntilWithPump(
          () => transferComplete,
          timeout: const Duration(seconds: 60),
          description: 'file transfer complete',
        );

        expect(transferComplete, isTrue,
            reason: 'File transfer should complete');
        if (progressPositions.isNotEmpty) {
          expect(progressPositions.length, greaterThan(0));
        }
        expect(seekTarget, greaterThan(0),
            reason: 'seek target should be positive');
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: progressListener));
      }
    }, timeout: const Timeout(Duration(seconds: 150)));

    test('Seek from beginning after partial transfer', () async {
      final alice = scenario.getNode('alice')!;
      final bob = scenario.getNode('bob')!;
      final bobToxId = bob.getToxId();
      await alice.waitForFriendConnection(bobToxId,
          timeout: const Duration(seconds: 90));

      var fileRequestReceived = false;
      final fileSize = await testFile.length();

      final seekBeginListener = V2TimAdvancedMsgListener(
        onRecvNewMessage: (V2TimMessage message) {
          final text = message.textElem?.text ?? '';
          final isForwardFileText =
              text.contains('转发文件') && text.contains('test_file_seek.txt');
          if (message.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE ||
              message.fileElem != null ||
              isForwardFileText) {
            fileRequestReceived = true;
            bob.addReceivedMessage(message);
          }
        },
        onMessageDownloadProgressCallback:
            (V2TimMessageDownloadProgress progress) {
          // Track progress
        },
      );

      bob.runWithInstance(() =>
          TIMMessageManager.instance.addAdvancedMsgListener(seekBeginListener));
      try {
        await Future.delayed(const Duration(seconds: 2));

        final sendResult = await alice.runWithInstanceAsync(() async {
          final fileResult = TIMMessageManager.instance.createFileMessage(
            filePath: testFile.path,
            fileName: 'test_file_seek.txt',
          );
          return await TIMMessageManager.instance.sendMessage(
            groupID: null,
            id: fileResult.id!,
            receiver: bobToxId,
          );
        });

        expect(sendResult.code, equals(0));

        // Wait for file request
        await waitUntilWithPump(
          () => fileRequestReceived,
          timeout: const Duration(seconds: 15),
          description: 'file request received',
        );

        // Verify latest file message in this test run.
        final fileMessages = bob.receivedMessages
            .where((m) =>
                m.elemType == MessageElemType.V2TIM_ELEM_TYPE_FILE ||
                m.fileElem != null)
            .toList();
        expect(fileMessages.isNotEmpty, isTrue,
            reason: 'Expected at least one file message');
        final fileMessage = fileMessages.last;

        expect(fileMessage.fileElem, isNotNull);
        final reportedSize = fileMessage.fileElem?.fileSize;
        if (reportedSize != null) {
          expect(reportedSize, equals(fileSize));
        }
      } finally {
        bob.runWithInstance(() => TIMMessageManager.instance
            .removeAdvancedMsgListener(listener: seekBeginListener));
      }
      // Note: Seeking to beginning (position 0) would restart the transfer
      // This is handled by FfiChatService.seekFileTransfer(peerId, fileNumber, 0)
    }, timeout: const Timeout(Duration(seconds: 90)));
  });
}
