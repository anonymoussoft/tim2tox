/// ToxAV Service
///
/// Manages ToxAV lifecycle and audio/video frame processing

import 'dart:async';
import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart' as pkgffi;
import '../ffi/tim2tox_ffi.dart';
import 'call_av_backend.dart';

/// Callback types for audio/video events
typedef CallCallback = void Function(
    int friendNumber, bool audioEnabled, bool videoEnabled);
typedef CallStateCallback = void Function(int friendNumber, int state);
typedef AudioReceiveCallback = void Function(int friendNumber, List<int> pcm,
    int sampleCount, int channels, int samplingRate);
typedef VideoReceiveCallback = void Function(int friendNumber, int width,
    int height, List<int> y, List<int> u, List<int> v);

// Define callback types for FFI (matching tim2tox_ffi.dart)
typedef _AvCallCallbackNative = ffi.Void Function(
  ffi.Uint32, // friend_number
  ffi.Int32, // audio_enabled
  ffi.Int32, // video_enabled
  ffi.Pointer<ffi.Void>, // user_data
);
typedef _AvCallStateCallbackNative = ffi.Void Function(
  ffi.Uint32, // friend_number
  ffi.Uint32, // state
  ffi.Pointer<ffi.Void>, // user_data
);
typedef _AvAudioReceiveCallbackNative = ffi.Void Function(
  ffi.Uint32, // friend_number
  ffi.Pointer<ffi.Int16>, // pcm
  ffi.Size, // sample_count
  ffi.Uint8, // channels
  ffi.Uint32, // sampling_rate
  ffi.Pointer<ffi.Void>, // user_data
);
typedef _AvVideoReceiveCallbackNative = ffi.Void Function(
  ffi.Uint32, // friend_number
  ffi.Uint16, // width
  ffi.Uint16, // height
  ffi.Pointer<ffi.Uint8>, // y
  ffi.Pointer<ffi.Uint8>, // u
  ffi.Pointer<ffi.Uint8>, // v
  ffi.Pointer<ffi.Void>, // user_data
);

/// ToxAV Service for managing audio/video calls
class ToxAVService implements CallAvBackend {
  final Tim2ToxFfi _ffi;
  bool _initialized = false;
  int? _instanceId; // Instance ID for multi-instance routing

  // Global map for instance ID -> ToxAVService routing (for multi-instance support)
  static final Map<int, ToxAVService> _instanceServices = {};
  static final Object _instanceServicesLock =
      Object(); // Simple lock for thread safety

  // Kept for backward compatibility with default instance
  static ToxAVService? _globalService;

  // Callbacks
  CallCallback? _onCall;
  CallStateCallback? _onCallState;
  AudioReceiveCallback? _onAudioReceive;
  VideoReceiveCallback? _onVideoReceive;

  ToxAVService(this._ffi) {
    // Get current instance ID
    try {
      _instanceId = _ffi.getCurrentInstanceId();
      print('[ToxAVService] Constructor: instanceId=$_instanceId');

      // Register this service instance in the global map
      synchronized(_instanceServicesLock, () {
        if (_instanceId != null && _instanceId! != 0) {
          _instanceServices[_instanceId!] = this;
          print('[ToxAVService] Registered service for instance $_instanceId');
        } else {
          // Default instance (backward compatibility)
          _globalService = this;
          print(
              '[ToxAVService] Registered as global service (default instance)');
        }
      });
    } catch (e) {
      print('[ToxAVService] Constructor: Failed to get instance ID: $e');
      // Fallback to global service
      _globalService = this;
    }
  }

  // Helper function for synchronized access
  static T synchronized<T>(Object lock, T Function() action) {
    return action(); // In Dart, we rely on single-threaded execution
  }

  /// Dispatches AV call callback from native (posted via SendCallbackToDart from tox thread).
  static void dispatchAvCall(
      int instanceId, int friendNumber, bool audioEnabled, bool videoEnabled) {
    ToxAVService? targetService;
    synchronized(_instanceServicesLock, () {
      if (instanceId != 0) {
        targetService = _instanceServices[instanceId];
      } else {
        targetService = _globalService;
      }
    });
    if (targetService != null && targetService!._onCall != null) {
      targetService!._onCall!.call(friendNumber, audioEnabled, videoEnabled);
    }
  }

  /// Dispatches AV call state callback from native (posted via SendCallbackToDart from tox thread).
  static void dispatchAvCallState(int instanceId, int friendNumber, int state) {
    ToxAVService? targetService;
    synchronized(_instanceServicesLock, () {
      if (instanceId != 0) {
        targetService = _instanceServices[instanceId];
      } else {
        targetService = _globalService;
      }
    });
    if (targetService != null && targetService!._onCallState != null) {
      targetService!._onCallState!.call(friendNumber, state);
    }
  }

  /// Initialize ToxAV
  Future<bool> initialize() async {
    print('[ToxAVService] initialize() called');
    if (_initialized) {
      print('[ToxAVService] initialize() already initialized, returning true');
      return true;
    }

    try {
      final result = _ffi.avInitialize(_ffi.getCurrentInstanceId());
      print('[ToxAVService] initialize() FFI result: $result');
      if (result == 1) {
        _initialized = true;
        _setupCallbacks();
        print('[ToxAVService] initialize() succeeded');
        return true;
      }
      print('[ToxAVService] initialize() failed: FFI returned $result');
      return false;
    } catch (e, stackTrace) {
      print('[ToxAVService] initialize() exception: $e');
      print('[ToxAVService] Stack trace: $stackTrace');
      return false;
    }
  }

  /// Shutdown ToxAV
  void shutdown() {
    print('[ToxAVService] shutdown() called');
    if (!_initialized) {
      print('[ToxAVService] shutdown() not initialized, skipping');
      return;
    }
    _ffi.avShutdown(_ffi.getCurrentInstanceId());
    _initialized = false;

    // Unregister from instance map
    synchronized(_instanceServicesLock, () {
      if (_instanceId != null && _instanceId! != 0) {
        _instanceServices.remove(_instanceId!);
        print('[ToxAVService] Unregistered service for instance $_instanceId');
      } else {
        if (_globalService == this) {
          _globalService = null;
          print('[ToxAVService] Unregistered global service');
        }
      }
    });

    print('[ToxAVService] shutdown() completed');
  }

  /// Setup callbacks
  void _setupCallbacks() {
    // Get instance ID for user_data
    final instanceId = _instanceId ?? 0;
    print('[ToxAVService] _setupCallbacks() instanceId=$instanceId');

    // Allocate memory for instance ID (will be passed as user_data)
    final instanceIdPtr = pkgffi.malloc<ffi.Int64>();
    instanceIdPtr.value = instanceId;

    // Setup call callback
    final onCallNative = ffi.Pointer.fromFunction<_AvCallCallbackNative>(
      _onCallNativeTrampoline,
    );

    // Setup call state callback
    final onCallStateNative =
        ffi.Pointer.fromFunction<_AvCallStateCallbackNative>(
      _onCallStateNativeTrampoline,
    );

    // Setup audio receive callback
    final onAudioReceiveNative =
        ffi.Pointer.fromFunction<_AvAudioReceiveCallbackNative>(
      _onAudioReceiveNativeTrampoline,
    );

    // Setup video receive callback
    final onVideoReceiveNative =
        ffi.Pointer.fromFunction<_AvVideoReceiveCallbackNative>(
      _onVideoReceiveNativeTrampoline,
    );

    // Pass instance ID as first param and as user_data for routing
    final instanceIdForFfi = _instanceId ?? _ffi.getCurrentInstanceId();
    _ffi.avSetCallCallbackNative(
        instanceIdForFfi, onCallNative.cast(), instanceIdPtr.cast());
    _ffi.avSetCallStateCallbackNative(
        instanceIdForFfi, onCallStateNative.cast(), instanceIdPtr.cast());
    _ffi.avSetAudioReceiveCallbackNative(
        instanceIdForFfi, onAudioReceiveNative.cast(), instanceIdPtr.cast());
    _ffi.avSetVideoReceiveCallbackNative(
        instanceIdForFfi, onVideoReceiveNative.cast(), instanceIdPtr.cast());

    print(
        '[ToxAVService] _setupCallbacks() completed with instanceId=$instanceId');
  }

  // Static trampoline functions for FFI (must be static for fromFunction)
  // These route callbacks to the correct service instance based on instance ID in userData
  @pragma('vm:entry-point')
  static void _onCallNativeTrampoline(
    int friendNumber,
    int audioEnabled,
    int videoEnabled,
    ffi.Pointer<ffi.Void> userData,
  ) {
    try {
      // Extract instance ID from userData
      int instanceId = 0;
      if (userData.address != 0) {
        instanceId = userData.cast<ffi.Int64>().value;
      }

      print(
          '[ToxAVService] _onCallNativeTrampoline() callback: instanceId=$instanceId, friendNumber=$friendNumber, audioEnabled=$audioEnabled, videoEnabled=$videoEnabled');

      // Route to the correct service instance
      ToxAVService? targetService;
      synchronized(_instanceServicesLock, () {
        if (instanceId != 0) {
          targetService = _instanceServices[instanceId];
        } else {
          targetService = _globalService;
        }
      });

      if (targetService != null && targetService!._onCall != null) {
        targetService!._onCall!.call(
          friendNumber,
          audioEnabled != 0,
          videoEnabled != 0,
        );
      } else {
        print(
            '[ToxAVService] _onCallNativeTrampoline() warning: No service found for instanceId=$instanceId or callback is null');
      }
    } catch (e) {
      print('[ToxAVService] _onCallNativeTrampoline() exception: $e');
    }
  }

  @pragma('vm:entry-point')
  static void _onCallStateNativeTrampoline(
    int friendNumber,
    int state,
    ffi.Pointer<ffi.Void> userData,
  ) {
    try {
      // Extract instance ID from userData
      int instanceId = 0;
      if (userData.address != 0) {
        instanceId = userData.cast<ffi.Int64>().value;
      }

      print(
          '[ToxAVService] _onCallStateNativeTrampoline() callback: instanceId=$instanceId, friendNumber=$friendNumber, state=$state');

      // Route to the correct service instance
      ToxAVService? targetService;
      synchronized(_instanceServicesLock, () {
        if (instanceId != 0) {
          targetService = _instanceServices[instanceId];
        } else {
          targetService = _globalService;
        }
      });

      if (targetService != null && targetService!._onCallState != null) {
        targetService!._onCallState!.call(
          friendNumber,
          state,
        );
      } else {
        print(
            '[ToxAVService] _onCallStateNativeTrampoline() warning: No service found for instanceId=$instanceId or callback is null');
      }
    } catch (e) {
      print('[ToxAVService] _onCallStateNativeTrampoline() exception: $e');
    }
  }

  @pragma('vm:entry-point')
  static void _onAudioReceiveNativeTrampoline(
    int friendNumber,
    ffi.Pointer<ffi.Int16> pcm,
    int sampleCount,
    int channels,
    int samplingRate,
    ffi.Pointer<ffi.Void> userData,
  ) {
    try {
      // Extract instance ID from userData
      int instanceId = 0;
      if (userData.address != 0) {
        instanceId = userData.cast<ffi.Int64>().value;
      }

      // Route to the correct service instance
      ToxAVService? targetService;
      synchronized(_instanceServicesLock, () {
        if (instanceId != 0) {
          targetService = _instanceServices[instanceId];
        } else {
          targetService = _globalService;
        }
      });

      if (targetService != null && targetService!._onAudioReceive != null) {
        final pcmList = pcm.asTypedList(sampleCount);
        targetService!._onAudioReceive!(
          friendNumber,
          pcmList,
          sampleCount,
          channels,
          samplingRate,
        );
      }
    } catch (e) {
      print('[ToxAVService] _onAudioReceiveNativeTrampoline() exception: $e');
    }
  }

  @pragma('vm:entry-point')
  static void _onVideoReceiveNativeTrampoline(
    int friendNumber,
    int width,
    int height,
    ffi.Pointer<ffi.Uint8> y,
    ffi.Pointer<ffi.Uint8> u,
    ffi.Pointer<ffi.Uint8> v,
    ffi.Pointer<ffi.Void> userData,
  ) {
    try {
      // Extract instance ID from userData
      int instanceId = 0;
      if (userData.address != 0) {
        instanceId = userData.cast<ffi.Int64>().value;
      }

      // Route to the correct service instance
      ToxAVService? targetService;
      synchronized(_instanceServicesLock, () {
        if (instanceId != 0) {
          targetService = _instanceServices[instanceId];
        } else {
          targetService = _globalService;
        }
      });

      if (targetService != null && targetService!._onVideoReceive != null) {
        // Calculate buffer sizes
        final ySize = width * height;
        final uvSize = (width ~/ 2) * (height ~/ 2);

        final yList = y.asTypedList(ySize);
        final uList = u.asTypedList(uvSize);
        final vList = v.asTypedList(uvSize);

        targetService!._onVideoReceive!(
          friendNumber,
          width,
          height,
          yList,
          uList,
          vList,
        );
      }
    } catch (e) {
      print('[ToxAVService] _onVideoReceiveNativeTrampoline() exception: $e');
    }
  }

  /// Set call callback
  void setCallCallback(CallCallback? callback) {
    _onCall = callback;
  }

  /// Set call state callback
  void setCallStateCallback(CallStateCallback? callback) {
    _onCallState = callback;
  }

  /// Set audio receive callback
  void setAudioReceiveCallback(AudioReceiveCallback? callback) {
    _onAudioReceive = callback;
  }

  /// Set video receive callback
  void setVideoReceiveCallback(VideoReceiveCallback? callback) {
    _onVideoReceive = callback;
  }

  /// Start a call
  Future<bool> startCall(int friendNumber,
      {int audioBitRate = 48, int videoBitRate = 5000}) async {
    print(
        '[ToxAVService] startCall() called: friendNumber=$friendNumber, audioBitRate=$audioBitRate, videoBitRate=$videoBitRate');
    if (!_initialized) {
      print('[ToxAVService] startCall() not initialized, initializing...');
      final initResult = await initialize();
      if (!initResult) {
        print('[ToxAVService] startCall() failed: initialization failed');
        return false;
      }
    }

    final result = _ffi.avStartCallNative(
        _ffi.getCurrentInstanceId(), friendNumber, audioBitRate, videoBitRate);
    print('[ToxAVService] startCall() FFI result: $result');
    final success = result == 1;
    if (success) {
      print('[ToxAVService] startCall() succeeded: friendNumber=$friendNumber');
    } else {
      print(
          '[ToxAVService] startCall() failed: friendNumber=$friendNumber, result=$result');
    }
    return success;
  }

  /// Answer an incoming call
  Future<bool> answerCall(int friendNumber,
      {int audioBitRate = 48, int videoBitRate = 5000}) async {
    print(
        '[ToxAVService] answerCall() called: friendNumber=$friendNumber, audioBitRate=$audioBitRate, videoBitRate=$videoBitRate');
    if (!_initialized) {
      print('[ToxAVService] answerCall() not initialized, initializing...');
      final initResult = await initialize();
      if (!initResult) {
        print('[ToxAVService] answerCall() failed: initialization failed');
        return false;
      }
    }

    final result = _ffi.avAnswerCallNative(
        _ffi.getCurrentInstanceId(), friendNumber, audioBitRate, videoBitRate);
    print('[ToxAVService] answerCall() FFI result: $result');
    final success = result == 1;
    if (success) {
      print(
          '[ToxAVService] answerCall() succeeded: friendNumber=$friendNumber');
    } else {
      print(
          '[ToxAVService] answerCall() failed: friendNumber=$friendNumber, result=$result');
    }
    return success;
  }

  /// End a call
  Future<bool> endCall(int friendNumber) async {
    print('[ToxAVService] endCall() called: friendNumber=$friendNumber');
    if (!_initialized) {
      print('[ToxAVService] endCall() failed: not initialized');
      return false;
    }

    final result =
        _ffi.avEndCallNative(_ffi.getCurrentInstanceId(), friendNumber);
    print('[ToxAVService] endCall() FFI result: $result');
    final success = result == 1;
    if (success) {
      print('[ToxAVService] endCall() succeeded: friendNumber=$friendNumber');
    } else {
      print(
          '[ToxAVService] endCall() failed: friendNumber=$friendNumber, result=$result');
    }
    return success;
  }

  /// Get user ID (public key hex string) by friend number.
  /// Returns null if the friend number is invalid or lookup fails.
  String? getUserIdByFriendNumber(int friendNumber) {
    final ptr = _ffi.getUserIdByFriendNumberNative(friendNumber);
    if (ptr == ffi.nullptr) return null;
    try {
      return ptr.toDartString();
    } catch (e) {
      print('[ToxAVService] getUserIdByFriendNumber() error: $e');
      return null;
    }
  }

  /// Mute/unmute audio
  Future<bool> muteAudio(int friendNumber, bool mute) async {
    if (!_initialized) return false;

    final result = _ffi.avMuteAudioNative(
        _ffi.getCurrentInstanceId(), friendNumber, mute ? 1 : 0);
    return result == 1;
  }

  /// Hide/show video
  Future<bool> muteVideo(int friendNumber, bool hide) async {
    if (!_initialized) return false;

    final result = _ffi.avMuteVideoNative(
        _ffi.getCurrentInstanceId(), friendNumber, hide ? 1 : 0);
    return result == 1;
  }

  /// Send audio frame
  Future<bool> sendAudioFrame(int friendNumber, List<int> pcm, int sampleCount,
      int channels, int samplingRate) async {
    if (!_initialized) return false;

    final pcmPtr = pkgffi.malloc<ffi.Int16>(pcm.length);
    try {
      for (int i = 0; i < pcm.length; i++) {
        pcmPtr[i] = pcm[i];
      }

      final result = _ffi.avSendAudioFrameNative(_ffi.getCurrentInstanceId(),
          friendNumber, pcmPtr, sampleCount, channels, samplingRate);
      return result == 1;
    } finally {
      pkgffi.malloc.free(pcmPtr);
    }
  }

  /// Send video frame (YUV420 format)
  Future<bool> sendVideoFrame(int friendNumber, int width, int height,
      List<int> y, List<int> u, List<int> v,
      {int yStride = 0, int uStride = 0, int vStride = 0}) async {
    if (!_initialized) return false;

    final yPtr = pkgffi.malloc<ffi.Uint8>(y.length);
    final uPtr = pkgffi.malloc<ffi.Uint8>(u.length);
    final vPtr = pkgffi.malloc<ffi.Uint8>(v.length);

    try {
      for (int i = 0; i < y.length; i++) {
        yPtr[i] = y[i];
      }
      for (int i = 0; i < u.length; i++) {
        uPtr[i] = u[i];
      }
      for (int i = 0; i < v.length; i++) {
        vPtr[i] = v[i];
      }

      final result = _ffi.avSendVideoFrameNative(
        _ffi.getCurrentInstanceId(),
        friendNumber,
        width,
        height,
        yPtr,
        uPtr,
        vPtr,
        yStride > 0 ? yStride : width,
        uStride > 0 ? uStride : width ~/ 2,
        vStride > 0 ? vStride : width ~/ 2,
      );
      return result == 1;
    } finally {
      pkgffi.malloc.free(yPtr);
      pkgffi.malloc.free(uPtr);
      pkgffi.malloc.free(vPtr);
    }
  }

  /// Set audio bit rate
  Future<bool> setAudioBitRate(int friendNumber, int audioBitRate) async {
    if (!_initialized) return false;

    final result = _ffi.avSetAudioBitRateNative(
        _ffi.getCurrentInstanceId(), friendNumber, audioBitRate);
    return result == 1;
  }

  /// Set video bit rate
  Future<bool> setVideoBitRate(int friendNumber, int videoBitRate) async {
    if (!_initialized) return false;

    final result = _ffi.avSetVideoBitRateNative(
        _ffi.getCurrentInstanceId(), friendNumber, videoBitRate);
    return result == 1;
  }

  /// Get friend number by user ID
  int getFriendNumberByUserId(String userId) {
    print(
        '[ToxAVService] getFriendNumberByUserId() called: userId=$userId (length=${userId.length})');
    final userIdPtr = userId.toNativeUtf8();
    try {
      final result = _ffi.getFriendNumberByUserIdNative(userIdPtr);
      if (result == 0xFFFFFFFF) {
        print(
            '[ToxAVService] getFriendNumberByUserId() failed: friend not found for userId=$userId');
      } else {
        print(
            '[ToxAVService] getFriendNumberByUserId() succeeded: userId=$userId, friendNumber=$result');
      }
      return result;
    } finally {
      pkgffi.malloc.free(userIdPtr);
    }
  }

  bool get isInitialized => _initialized;
}
