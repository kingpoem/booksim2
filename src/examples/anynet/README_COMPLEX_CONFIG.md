# 复杂 JSON 配置文件说明

## 文件说明

- **anynet_config.json**: 基础 JSON 配置文件，已验证可运行 ✅
- **anynet_config_complex.json**: 复杂 JSON 配置文件，包含所有主要配置参数

## 复杂配置文件统计

- **全局配置项**: 118 项
- **路由器特定配置**:
  - 路由器 0: 26 项配置
  - 路由器 1: 25 项配置
  - 路由器 2: 28 项配置
- **总配置项数**: 196 项

## 包含的主要配置类别

### 1. 网络拓扑配置
- `topology`, `network_file`, `routing_function`
- `use_noc_latency`, `x`, `y`, `xr`, `yr`
- `in_ports`, `out_ports`

### 2. 路由器通用配置
- `router`, `output_delay`, `credit_delay`
- `internal_speedup`, `output_buffer_size`, `noq`

### 3. 虚拟通道配置
- `num_vcs`, `vc_buf_size`, `buf_size`
- `buffer_policy`, `private_bufs`, `private_buf_size`
- `max_held_slots`, `feedback_aging_scale`, `feedback_offset`

### 4. 虚拟通道行为配置
- `wait_for_tail_credit`, `vc_busy_when_full`
- `vc_prioritize_empty`, `vc_priority_donation`
- `vc_shuffle_requests`, `hold_switch_for_packet`

### 5. 推测执行配置
- `speculative`, `spec_check_elig`, `spec_check_cred`
- `spec_mask_by_reqs`, `spec_sw_allocator`

### 6. 延迟配置
- `routing_delay`, `vc_alloc_delay`, `sw_alloc_delay`
- `st_prepare_delay`, `st_final_delay`

### 7. 速度配置
- `input_speedup`, `output_speedup`, `internal_speedup`

### 8. 分配器配置
- `vc_allocator`, `sw_allocator`, `arb_type`, `alloc_iters`

### 9. 流量配置
- `classes`, `traffic`, `injection_rate`, `packet_size`
- `injection_process`, `burst_alpha`, `burst_beta`, `burst_r1`
- `priority`, `batch_size`, `batch_count`

### 10. 读写配置
- `use_read_write`, `write_fraction`
- `read_request_begin_vc`, `read_request_end_vc`
- `write_request_begin_vc`, `write_request_end_vc`
- `read_reply_begin_vc`, `write_reply_begin_vc`
- `read_request_size`, `write_request_size`, etc.

### 11. 仿真参数
- `sim_type`, `warmup_periods`, `sample_period`, `max_samples`
- `measure_stats`, `pair_stats`
- `latency_thres`, `warmup_thres`, `stopping_thres`
- `sim_count`, `include_queuing`, `seed`

### 12. 输出配置
- `print_activity`, `print_csv_results`
- `watch_file`, `watch_flits`, `watch_packets`
- `stats_out`, `watch_out`

### 13. 功耗配置
- `sim_power`, `power_output_file`, `tech_file`
- `channel_width`, `channel_sweep`

## 路由器特定配置示例

配置文件展示了三种不同的路由器配置策略：

### 路由器 0 - 高性能配置
- 更多 VC (16)
- 更大缓冲区 (32)
- 更高速度 (speedup = 2)
- 推测执行启用
- 优化分配器 (islip, 4 次迭代)

### 路由器 1 - 平衡配置
- 中等 VC (8)
- 标准缓冲区 (16)
- 标准速度 (speedup = 1)
- 不同分配器 (pim)
- 保持交换配置 (hold_switch_for_packet)

### 路由器 2 - 简化配置
- 较少 VC (4)
- 小缓冲区 (8)
- 共享缓冲区策略
- 简单分配器 (separable_input_first)

## 注意事项

1. **配置兼容性**: 某些配置组合可能不兼容，需要注意：
   - NOQ 需要 `routing_delay = 0` 和足够的 VC
   - VC 范围配置必须与 `num_vcs` 匹配
   - 缓冲区策略选择影响其他参数

2. **参数依赖关系**:
   - `noq` 要求 `routing_delay = 0`
   - `noq` 要求 `num_vcs >= outputs`
   - 推测执行相关参数需要 `speculative = 1`

3. **测试建议**:
   - 先使用简单配置 (`anynet_config.json`) 验证基本功能
   - 逐步添加复杂配置项
   - 注意错误消息中的配置冲突提示

## 使用示例

```bash
# 运行简单配置（已验证）
cd src/examples/anynet
../../booksim anynet_config.json

# 运行复杂配置（需要根据实际情况调整参数）
../../booksim anynet_config_complex.json
```

## 配置验证

配置文件已通过 JSON 格式验证：
```bash
python3 -m json.tool anynet_config_complex.json
```

配置文件结构正确，但某些参数组合可能需要根据具体网络拓扑和需求进行调整。
