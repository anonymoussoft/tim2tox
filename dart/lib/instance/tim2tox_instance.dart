/// Instance-scoped context for multi-instance support.
///
/// Wraps an instance handle and provides [runWithInstance] / [runWithInstanceAsync]
/// so that TIMManager/TIMFriendshipManager/TIMGroupManager calls run against the
/// correct instance without scattered [setCurrentInstance] calls.
///
/// Usage:
/// ```dart
/// final instance = Tim2ToxInstance(handle);
/// instance.runWithInstance(() => TIMManager.instance.someSyncCall());
/// await instance.runWithInstanceAsync(() => TIMManager.instance.login(...));
/// ```
library;

import 'package:tencent_cloud_chat_sdk/native_im/bindings/native_library_manager.dart';

import '../ffi/tim2tox_ffi.dart';

/// Scope for running logic on a specific tim2tox instance.
///
/// [instanceHandle] is 0 for the default instance, or a handle from
/// [Tim2ToxFfi.createTestInstanceNative] / [Tim2ToxFfi.createTestInstanceExNative].
class Tim2ToxInstance {
  /// Instance handle. 0 = default instance.
  final int instanceHandle;

  final Tim2ToxFfi? _ffi;

  /// Creates a scope for [instanceHandle].
  /// [instanceHandle] 0 means default instance; non-zero is a test instance handle.
  /// Optionally pass [ffi] to avoid repeated [Tim2ToxFfi.open()] calls.
  Tim2ToxInstance(this.instanceHandle, [Tim2ToxFfi? ffi]) : _ffi = ffi;

  /// Creates from nullable handle; null is treated as default instance (0).
  factory Tim2ToxInstance.fromHandle(int? handle) =>
      Tim2ToxInstance(handle ?? 0);

  Tim2ToxFfi get _lib => _ffi ?? Tim2ToxFfi.open();

  /// Runs [action] with this instance as current, then restores the previous
  /// current instance. Use for synchronous calls to TIMManager / Friendship / Group.
  ///
  /// Also stamps [NativeLibraryManager.currentRegistrationInstanceId] for the
  /// duration so that listener registrations made by [action] (e.g.
  /// `TIMMessageManager.instance.addAdvancedMsgListener`) are tagged with this
  /// instance id in the manager's per-instance bucket. The SDK's dispatch
  /// helper (`_listenersForCurrentDispatch()`) then routes callbacks back to
  /// only the listeners that belong to the right instance — without this
  /// stamp, Alice's and Bob's listeners end up in the same flat list and an
  /// event meant for Bob fires Alice's listener.
  R runWithInstance<R>(R Function() action) {
    final lib = _lib;
    final prev = lib.getCurrentInstanceId();
    final prevReg = NativeLibraryManager.currentRegistrationInstanceId;
    try {
      if (instanceHandle != 0) {
        lib.setCurrentInstance(instanceHandle);
        NativeLibraryManager.currentRegistrationInstanceId = instanceHandle;
      }
      return action();
    } finally {
      lib.setCurrentInstance(prev);
      NativeLibraryManager.currentRegistrationInstanceId = prevReg;
    }
  }

  /// Runs [action] with this instance as current, then restores the previous
  /// current instance. Use for async calls (login, initSDK, etc.).
  ///
  /// See [runWithInstance] for the registration-id-stamp rationale.
  Future<R> runWithInstanceAsync<R>(Future<R> Function() action) async {
    final lib = _lib;
    final prev = lib.getCurrentInstanceId();
    final prevReg = NativeLibraryManager.currentRegistrationInstanceId;
    try {
      if (instanceHandle != 0) {
        lib.setCurrentInstance(instanceHandle);
        NativeLibraryManager.currentRegistrationInstanceId = instanceHandle;
      }
      return await action();
    } finally {
      lib.setCurrentInstance(prev);
      NativeLibraryManager.currentRegistrationInstanceId = prevReg;
    }
  }
}
