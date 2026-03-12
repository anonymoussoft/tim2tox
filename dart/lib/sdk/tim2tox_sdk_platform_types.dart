/// Tim2Tox SDK Platform Types
///
/// This file contains type definitions and callback types used across modules.

import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart' as pkgffi;

// Signaling callback types (must be at top level)
typedef SignalingInvitationCallbackNative = ffi.Void Function(
  ffi.Pointer<pkgffi.Utf8>, // invite_id
  ffi.Pointer<pkgffi.Utf8>, // inviter
  ffi.Pointer<pkgffi.Utf8>, // group_id
  ffi.Pointer<pkgffi.Utf8>, // data
  ffi.Pointer<ffi.Void>, // user_data
);
typedef SignalingCancelCallbackNative = ffi.Void Function(
  ffi.Pointer<pkgffi.Utf8>, // invite_id
  ffi.Pointer<pkgffi.Utf8>, // inviter
  ffi.Pointer<pkgffi.Utf8>, // data
  ffi.Pointer<ffi.Void>, // user_data
);
typedef SignalingAcceptCallbackNative = ffi.Void Function(
  ffi.Pointer<pkgffi.Utf8>, // invite_id
  ffi.Pointer<pkgffi.Utf8>, // invitee
  ffi.Pointer<pkgffi.Utf8>, // data
  ffi.Pointer<ffi.Void>, // user_data
);
typedef SignalingRejectCallbackNative = ffi.Void Function(
  ffi.Pointer<pkgffi.Utf8>, // invite_id
  ffi.Pointer<pkgffi.Utf8>, // invitee
  ffi.Pointer<pkgffi.Utf8>, // data
  ffi.Pointer<ffi.Void>, // user_data
);
typedef SignalingTimeoutCallbackNative = ffi.Void Function(
  ffi.Pointer<pkgffi.Utf8>, // invite_id
  ffi.Pointer<pkgffi.Utf8>, // inviter
  ffi.Pointer<ffi.Void>, // user_data
);
