# Tim2Tox Bootstrap and Polling
> Language: [Chinese](BOOTSTRAP_AND_POLLING.md) | [English](BOOTSTRAP_AND_POLLING.en.md)


This document describes Tim2Tox’s current Bootstrap node loading, connection establishment, and polling mechanisms, and supplements the Flutter Echo Client’s access methods in startup, settings pages, and LAN Bootstrap scenarios.

## 1. Scope

This link spans two layers:

- `flutter_echo_client`: Responsible for selecting and saving the current Bootstrap node, determining the startup sequence, and providing the LAN Bootstrap UI.
- `tim2tox/dart`: Responsible for applying the saved `host/port/publicKey` to the Tox instance, and driving connection status, messages, files and AV events through `startPolling()`.

It should be noted that there is no separate "LAN Bootstrap mode" branch within Tim2Tox. What the framework really consumes is always just a triple of the current Bootstrap node.

## 2. Bootstrap node source

Three sources currently exist:

1. Manual input
2. Automatically pull from `https://nodes.tox.chat/json`
3. Local Bootstrap service within LAN

Flutter Echo Client side related code:

- `lib/util/bootstrap_nodes.dart`: Pull nodes from the public node list, and fall back to the hardcoded fallback list on failure.
- `lib/ui/settings/bootstrap_settings_section.dart`: Manage three modes of `auto` / `manual` / `lan`.
- `lib/ui/settings/bootstrap_nodes_page.dart`: List online nodes, perform detection, and switch current nodes.
- `lib/util/lan_bootstrap_service.dart`: Start or detect the local Bootstrap service.

Related code on Tim2Tox side:

- `dart/lib/interfaces/bootstrap_service.dart`: Only expose `host/port/publicKey` reading and writing of the current node.
- `dart/lib/service/ffi_chat_service.dart`: Call `_loadAndApplySavedBootstrapNode()` during `init()`.

## 3. Node selection and saving

### Automatic mode

Flutter Echo Client will first check Bootstrap mode in `_StartupGate._decide()`:

- If `lan` is detected on a non-desktop platform, it will be forced to fall back to `auto`.
- In `auto` mode, if the node has not been saved yet, the public node list will be pulled first and the first online node will be written to `Prefs.setCurrentBootstrapNode(...)`

The purpose of this is to ensure that when starting for the first time, there is already a usable node configuration before `FfiChatService.init()`.

### Manual mode

After manually entering the node on the settings page, `Prefs.setCurrentBootstrapNode(host, port, pubkey)` will be called directly. `FfiChatService.init()` will then read and apply it in `_loadAndApplySavedBootstrapNode()`.

### LAN Bootstrap mode

The LAN Bootstrap mode is the configuration layer concept of Flutter Echo Client, not the internal protocol branch of Tim2Tox. In the current implementation:

- The desktop can start a local Tox instance as a Bootstrap service through `LanBootstrapServiceManager.startLocalBootstrapService(port)`
- This service will generate an independent profile, log in as `BootstrapService`, take out `udpPort` and `dhtId`, and then start its own `startPolling()`
- The UI layer will show its `ip/port/pubkey`

But Tim2Tox still only recognizes the "current Bootstrap node" internally. Therefore, if the LAN Bootstrap service wants to actually participate in the connection, it still needs to write the corresponding `host/port/pubkey` back to the current node configuration.

## 4. How Tim2Tox applies Bootstrap nodes

`FfiChatService.init()` will call `_loadAndApplySavedBootstrapNode()` at the end:

- `manual` mode: read `getCurrentBootstrapNode()` and call `addBootstrapNode(...)`
- Other modes: take `BootstrapService.getBootstrapHost/Port/PublicKey()` first; fall back to `getCurrentBootstrapNode()` when you can’t get it.

`addBootstrapNode(host, port, publicKeyHex)` will:

1. Call FFI `tim2tox_ffi_add_bootstrap_node`
2. When successful, save the node back to the current configuration.

This means that in pages with `service` instances, the "test node" is not a purely read-only operation. In the current implementation, a successful test will also write the node as the current node because it reuses `addBootstrapNode(...)`.

## 5. Login and network status

`FfiChatService.login(...)` is only responsible for:

- Call native `login`
- Read the current `selfId`
- Use `getSelfConnectionStatus()` to push the current connection status immediately

It does not guarantee that networking has been completed at this time. Real connection changes rely on the `conn:success` / `conn:failed` events in the polling queue.

The order when Flutter Echo Client is started is:

1. `_StartupGate` ensures that there are currently available Bootstrap nodes
2. Initialize `FfiChatService`
3.`FakeUIKit.startWithFfi(service)`
4. `_initTIMManagerSDK()`
5.`service.startPolling()`
6. Monitor `connectionStatusStream`
7. Wait until the connection is successful within 20 seconds to preload the friends; otherwise, it will also enter the homepage after timeout.

So `startPolling()` is the real starting point for networking and messaging consumption, not just a "background refresh".

## 6. Polling loop

`FfiChatService.startPolling()` currently does the following:

1. Cancel the old poller and old profile save timer
2. Start `saveToxProfileNow()` every 60 seconds
3. Push the current `_isConnected` first
4. Arrange the next round of poll according to the adaptive interval

Current polling interval policy:

- With file transfer: `50ms`
- Multi-instance or shared instance polling: `50ms`
- Activity in the last 2 seconds: `200ms`
- Idle state: `1000ms`

Each round of poll will:

1. First try to execute `avIterate(instanceId)`
2. Poll the text event queue by instance priority
3. A maximum of 200 events can be processed in batches at a time to avoid queue backlog during file transfer.

In multi-instance scenarios, non-zero instances will be polled first, and the receiver instance will be consumed first so that `file_request` can be processed faster.

## 7. What key events are there in the polling queue?

The events that currently require the most attention from maintainers are:

- `conn:success` / `conn:failed`
- `c2c:` / `gtext:`
-`file_request:`
- `file_done:`
- `typing:`

Among them:

- `conn:success` will set `_isConnected` to `true` and trigger avatar synchronization
- `file_request:` is a key pre-event of the file receiving link; if it is not polled in time, the receiving end cannot `acceptFileTransfer` in time
- `file_done:` will close the temporary reception status to historical messages and UI

This is why the poll interval is pressed to `50ms` during file transfer and a single round of batch drain queue is allowed.

## 8. Maintenance points on the Flutter Echo Client side

### Switch nodes

The actual action of switching nodes in `BootstrapNodesPage` is:

1. `service.addBootstrapNode(...)`
2.`service.login(...)`

That is, "write the new node and log in again", not rebuild the entire `FfiChatService`.

### Test node

On the settings page or node list page, if `service != null`, the test action directly reuses `addBootstrapNode(...)`. This has an implementation-level side effect:

- When the test is successful, the current node configuration will also be updatedIf it is just a login page scenario and there is no ready-made `service`, it will degrade to ordinary TCP detection.

### LAN Bootstrap service

Local services are only supported on the desktop. The current implementation is mainly:

- Start an independent Tox instance
- `ip/port/pubkey` who exposed it
- Allow UI to do liveness detection

It does not replace the `currentBootstrapNode` configuration model.

## 9. Recommended maintenance sequence

If you are changing Bootstrap, networking or polling issues, it is recommended to read the code in this order:

1.`flutter_echo_client/lib/main.dart`
2. `flutter_echo_client/lib/ui/settings/bootstrap_settings_section.dart`
3. `flutter_echo_client/lib/util/bootstrap_nodes.dart`
4. `flutter_echo_client/lib/util/lan_bootstrap_service.dart`
5. `tim2tox/dart/lib/interfaces/bootstrap_service.dart`
6. `tim2tox/dart/lib/service/ffi_chat_service.dart`

## 10. Related documents

- [ARCHITECTURE.md](ARCHITECTURE.en.md)
- [API_REFERENCE.md](API_REFERENCE.en.md)
- [../../flutter_echo_client/doc/ACCOUNT_AND_SESSION.md](../../flutter_echo_client/doc/ACCOUNT_AND_SESSION.en.md)
- [../../flutter_echo_client/doc/IMPLEMENTATION_DETAILS.md](../../flutter_echo_client/doc/IMPLEMENTATION_DETAILS.en.md)