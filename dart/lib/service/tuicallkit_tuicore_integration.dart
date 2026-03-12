/// TUICallKit TUICore Integration
///
/// ToxAV service that implements AbstractTUIService for TUICore registration.
/// This is the ONLY integration point needed — no patches or interceptors required.
///
/// Usage:
///   import 'package:tim2tox_dart/service/tuicallkit_tuicore_integration.dart';
///   registerToxAVWithTUICore(adapter);

import 'package:tencent_cloud_uikit_core/tencent_cloud_uikit_core.dart';
import 'tuicallkit_adapter.dart' as callkit;

/// ToxAV service that implements AbstractTUIService for TUICore registration.
class ToxAVCallService extends AbstractTUIService {
  final callkit.TUICallKitAdapter _adapter;
  ToxAVCallService(this._adapter);

  @override
  void onCall(String serviceName, String method, Map<String, dynamic> param) {
    if (method != callkit.METHOD_NAME_CALL) return;

    final type = param[callkit.PARAM_NAME_TYPE] as String?;
    final userIDs = param[callkit.PARAM_NAME_USERIDS];
    final groupId = param[callkit.PARAM_NAME_GROUPID] as String?;

    if (type == null || userIDs == null) return;

    final userids = (userIDs is List) ? userIDs.cast<String>() : <String>[];

    _adapter.handleCall(type: type, userids: userids, groupid: groupId);
  }
}

/// Register ToxAV as the TUICallingService in TUICore.
void registerToxAVWithTUICore(callkit.TUICallKitAdapter adapterInstance) {
  final service = ToxAVCallService(adapterInstance);
  TUICore.instance.registerService(callkit.TUICALLKIT_SERVICE_NAME, service);
}
