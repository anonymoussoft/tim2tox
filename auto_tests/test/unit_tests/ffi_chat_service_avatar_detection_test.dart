import 'package:test/test.dart';
import 'package:tim2tox_dart/service/ffi_chat_service.dart';

void main() {
  group('FfiChatService avatar sync file detection', () {
    test('treats avatar-style image names as avatar sync files', () {
      expect(
        FfiChatService.isAvatarSyncFilePath(
            '/tmp/file_recv/peer_0_42_avatar_ABCDEF1234.jpg'),
        isTrue,
      );
      expect(
        FfiChatService.isAvatarSyncFilePath('avatar_user.jpeg'),
        isTrue,
      );
    });

    test('does not treat regular image names as avatar sync files', () {
      expect(
        FfiChatService.isAvatarSyncFilePath('/tmp/file_recv/photo_2026.png'),
        isFalse,
      );
      expect(
        FfiChatService.isAvatarSyncFilePath('holiday.webp'),
        isFalse,
      );
    });
  });
}
