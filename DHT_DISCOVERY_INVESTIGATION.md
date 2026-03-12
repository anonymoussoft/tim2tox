# DHT 发现机制调查报告

## 问题描述

在 `scenario_group_general_test.dart` 测试中，peer1 加入 PUBLIC 群组后，两个实例（founder 和 peer1）都无法看到对方，只能看到自己。`onGroupPeerJoin` 回调从未被触发。

## 调查发现

### 1. DHT 连接状态
- ✅ 两个实例都已连接到 DHT（状态：UDP，值为 2）
- ✅ 群组连接状态正常（is_connected=1）
- ✅ 群组隐私状态为 PUBLIC（privacy_state=0）
- ✅ 两个实例都在持续调用 `tox_iterate`

### 2. 回调状态
- ❌ `onGroupPeerJoin` 回调从未被触发
- ❌ `HandleGroupPeerJoin` 从未被调用
- ❌ 两个实例都只能看到 1 个 peer（自己）

### 3. c-toxcore 实现机制

从 c-toxcore 源码分析：

#### PUBLIC 群组的 DHT 发现机制
1. **DHT 公告（Announcements）**：
   - PUBLIC 群组使用 `gc_add_peers_from_announces` 来添加 peer
   - 每个 peer 需要在 DHT 上公告自己的群组信息（`update_self_announces = true`）
   - 公告通过 `GC_Announce` 结构包含：
     - peer_public_key
     - IP_Port（如果可用）
     - TCP relays（如果 IP_Port 不可用）

2. **公告传播**：
   - 公告通过 DHT 网络传播
   - 其他 peer 通过 DHT 查询发现这些公告
   - 发现后调用 `peer_add` 添加 peer，然后触发 `onGroupPeerJoin` 回调

3. **关键代码路径**：
   ```c
   // group_chats.c:2809
   chat->update_self_announces = true;  // 标记需要更新公告
   
   // group_chats.c:8387
   int gc_add_peers_from_announces(GC_Chat *chat, ...) {
       if (!is_public_chat(chat)) return 0;  // 只对 PUBLIC 群组有效
       // 添加 peer 并触发回调
   }
   ```

### 4. 可能的原因

#### 原因 1: DHT 公告未及时传播
- DHT 公告需要时间在 DHT 网络中传播
- 即使使用本地 bootstrap，公告传播仍需要多次 DHT 查询
- c-toxcore 测试使用 `WAIT_UNTIL` 宏持续迭代直到发现 peer

#### 原因 2: 公告更新频率
- `update_self_announces` 标志需要被处理
- 公告刷新间隔：`GC_SELF_REFRESH_ANNOUNCE_INTERVAL = 60 * 20` 秒（20分钟）
- 初始公告可能需要更长时间才能被其他 peer 发现

#### 原因 3: 本地 bootstrap 的限制
- 虽然两个实例都连接到同一个 bootstrap 节点
- 但 DHT 公告仍然需要通过 DHT 网络传播
- 本地 bootstrap 只是帮助连接到 DHT，不直接加速公告传播

### 5. c-toxcore 测试的处理方式

从 `scenario_group_general_test.c` 和 `scenario_group_topic_test.c` 来看：

```c
// 等待 peer 加入
WAIT_UNTIL(state->peer_ids[i] != UINT32_MAX);

// WAIT_UNTIL 宏会持续调用 tox_iterate 直到条件满足
// 这意味着 DHT 发现可能需要较长时间
```

### 6. 当前实现的优化

已实施的优化：
1. ✅ 增加了 `JoinGroup` 中的迭代次数（从 5 次增加到 20 次）
2. ✅ 添加了 DHT 诊断日志
3. ✅ 检查 peer 连接状态
4. ✅ 优化了等待循环

但仍未解决问题。

### 7. 建议的解决方案

#### 方案 1: 增加等待时间（临时方案）
- 在测试中增加 DHT 同步等待时间
- 但这不符合用户要求（20秒超时限制）

#### 方案 2: 检查公告机制（根本方案）
- 验证 `update_self_announces` 是否被正确处理
- 检查公告是否被发送到 DHT
- 验证公告查询是否正常工作

#### 方案 3: 使用 friend 连接（替代方案）
- 对于本地测试，可以添加 friend 连接
- 但 PUBLIC 群组理论上不需要 friend 连接

#### 方案 4: 检查 tox_iterate 调用频率
- 确保两个实例的 `event_thread_` 都在正常运行
- 验证 `tox_iteration_interval` 是否合理
- 可能需要更频繁的迭代

### 8. 下一步行动

1. **检查公告更新**：
   - 添加日志验证 `update_self_announces` 是否被处理
   - 检查公告是否被发送

2. **检查 DHT 查询**：
   - 验证 DHT 查询是否正常工作
   - 检查是否收到其他 peer 的公告

3. **参考 c-toxcore 测试**：
   - 查看 c-toxcore 测试如何处理这种情况
   - 可能需要更长的等待时间或不同的策略

4. **考虑 friend 连接**：
   - 对于本地测试场景，添加 friend 连接可能更可靠
   - 虽然 PUBLIC 群组理论上不需要，但可以加速发现

## 结论

DHT 发现机制依赖于 DHT 公告的传播，这个过程可能需要较长时间，即使使用本地 bootstrap。`onGroupPeerJoin` 回调只有在 DHT 公告被接收并处理时才会触发。

当前实现已经优化了等待和迭代，但 DHT 公告传播可能需要更多时间。建议进一步调查公告机制的具体实现，或者考虑使用 friend 连接作为本地测试的替代方案。
