abstract class CallAvBackend {
  bool get isInitialized;

  Future<bool> initialize();

  int getFriendNumberByUserId(String userId);

  Future<bool> startCall(
    int friendNumber, {
    int audioBitRate,
    int videoBitRate,
  });

  Future<bool> answerCall(
    int friendNumber, {
    int audioBitRate,
    int videoBitRate,
  });

  Future<bool> endCall(int friendNumber);

  Future<bool> muteAudio(int friendNumber, bool mute);

  Future<bool> muteVideo(int friendNumber, bool hide);
}
