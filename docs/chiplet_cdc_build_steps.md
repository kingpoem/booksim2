# Chiplet 跨时钟域（CDC）支持：构建与使用步骤

## 1. 编译

在仓库 `src` 目录执行：

```bash
cd src
make
```

CDC 相关源文件由 Makefile 通配符一并编译，无需单独开关：`cdc_channel.cpp`、`networks/chiplet_d2d_cdc.cpp`、`networks/chiplet_mesh.cpp` 等。

## 2. 启用 CDC（配置最小集）

在 BookSim 配置中：

1. **拓扑与路由**（与无 CDC 时相同）  
   - `topology = chiplet_mesh;`  
   - `routing_function = dim_order_chiplet_mesh;`

2. **打开 D2D CDC**  
   - `chiplet_cdc_enable = 1;`

3. **各 die 名义时钟**（以「最细仿真 tick」为单位的整数周期；全为 1 时可不写数组）  
   - `chiplet_die_clock_period = {2,3};`  （长度1 或 `chiplet_x * chiplet_y`）  
   - 可选：`chiplet_die_clock_phase = {0,0};`

4. **规则**：任一 die 的 `period ≠ 1` 或 `phase ≠ 0` 时，**必须** `chiplet_cdc_enable = 1`，否则解析报错。

5. **常用 CDC 参数**（有默认值，可按需覆盖）  
   - `chiplet_d2d_latency` — CDC 交付后的线延迟（≥1）  
   - `chiplet_cdc_fifo_depth`、`chiplet_cdc_sync_cycles`、`chiplet_cdc_credit_sync_cycles`  
   - 可选 Gray 风格附加同步：`chiplet_cdc_gray_fifo`、`chiplet_cdc_gray_stages`

完整参数说明见 [`chiplet_config_reference.md`](chiplet_config_reference.md) §3.4。

## 3. 运行示例

```bash
cd src
./booksim examples/chiplet_mesh_2x1_k2_cdc
```

其它可直接运行的 **D2D CDC** 完整示例（均在 `src/examples/`）：`chiplet_mesh_2x1_k1_cdc`（最小 k=1、周期 2/4）、`chiplet_mesh_2x2_xy_cdc`（2×2、`xy`、四 die 异周期）、`chiplet_mesh_2x1_k2_cdc_phase`（同周期异相位）。

## 4. 从 JSON 生成配置（可选）

[`tools/chiplet_spec_to_config.py`](../tools/chiplet_spec_to_config.py) 支持两种 JSON 根结构：

- **Legacy**：根键 `chiplet` + `sim`。当 `chiplet.cdc_enable` 为 `true` 时，会写出 `chiplet_cdc_*` 与 `chiplet_die_clock_*`。
- **v1 `mesh_chiplet`**：`schema_version`、`routing`、`mesh_chiplet`、`d2d_cdc`、`sim`（见 [`mesh_chiplet_spec.sample.json`](../tools/mesh_chiplet_spec.sample.json) 与 [`chiplet_config_reference.md`](chiplet_config_reference.md) §7.0）。

```bash
python3 tools/chiplet_spec_to_config.py tools/chiplet_spec.sample.json -o out.cfg
python3 tools/chiplet_spec_to_config.py tools/mesh_chiplet_spec.sample.json -o out_v1.cfg
cd src && ./booksim ../out.cfg
```

字段示例见 [`tools/chiplet_spec.sample.json`](../tools/chiplet_spec.sample.json)。冒烟（同频 + 异频 CDC 各一例）：`tools/run_mesh_chiplet_json_smoke.sh`。

## 5. 实现位置（便于对照代码）

| 内容 | 路径 |
|------|------|
| Flit/Credit CDC 通道 | `src/cdc_channel.hpp`、`src/cdc_channel.cpp` |
| 时钟沿工具 | `src/networks/chiplet_clock.hpp` |
| D2D CDC 分配 | `src/networks/chiplet_d2d_cdc.hpp`、`src/networks/chiplet_d2d_cdc.cpp` |
| 拓扑接入与校验 | `src/networks/chiplet_mesh.hpp`、`src/networks/chiplet_mesh.cpp` |
| 默认值 | `src/booksim_config.cpp` |
