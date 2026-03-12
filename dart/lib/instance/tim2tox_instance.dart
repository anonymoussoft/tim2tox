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
  R runWithInstance<R>(R Function() action) {
    final lib = _lib;
    final prev = lib.getCurrentInstanceId();
    try {
      if (instanceHandle != 0) {
        lib.setCurrentInstance(instanceHandle);
      }
      return action();
    } finally {
      lib.setCurrentInstance(prev);
    }
  }

  /// Runs [action] with this instance as current, then restores the previous
  /// current instance. Use for async calls (login, initSDK, etc.).
  Future<R> runWithInstanceAsync<R>(Future<R> Function() action) async {
    final lib = _lib;
    final prev = lib.getCurrentInstanceId();
    try {
      if (instanceHandle != 0) {
        lib.setCurrentInstance(instanceHandle);
      }
      return await action();
    } finally {
      lib.setCurrentInstance(prev);
    }
  }
}
