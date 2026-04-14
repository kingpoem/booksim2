# Chiplet Mesh 配置参数说明（前端 / 自动化对接）

本文档说明在 BookSim2 中使用 **`topology = chiplet_mesh`** 时，与片芯（chiplet）拓扑、链路与路由相关的**配置项含义与约束**。不涉及页面布局、组件样式等前端实现建议。

仿真器从**文本配置文件**（或等价字段集合）读取这些项；仓库内亦提供 [`tools/chiplet_spec_to_config.py`](../tools/chiplet_spec_to_config.py)，可将 **JSON 规格** 转为同语义的书本配置文件。脚本支持两种根结构：**legacy**（`chiplet` + `sim`）与 **v1 `mesh_chiplet`**（`schema_version` + `mesh_chiplet` + `d2d_cdc` + `sim`）；v1 的 JSON Schema 见 [`tools/mesh_chiplet.schema.json`](../tools/mesh_chiplet.schema.json)。

---

## 1. 拓扑与路由的绑定关系

| 配置项 | 类型 | 含义 |
|--------|------|------|
| `topology` | 字符串 | 必须为 **`chiplet_mesh`** 才会实例化片芯网格拓扑。 |
| `routing_function` | 字符串 | 片芯维序路由应设为 **`dim_order_chiplet_mesh`**。 |

内部注册名称为 **`routing_function` + `_` + `topology`**，即 `dim_order_chiplet_mesh_chiplet_mesh`。前端若拼接校验键名，需与此一致。

---

## 2. 几何与互连（必选 / 常用）

### 2.1 `chiplet_x`、`chiplet_y`

| 项 | 类型 | 默认值（注册表） | 含义 |
|----|------|------------------|------|
| `chiplet_x` | 整数 ≥ 1 | 1 | 片芯阵列在 **X（列）** 方向的 die 个数。 |
| `chiplet_y` | 整数 ≥ 1 | 1 | 片芯阵列在 **Y（行）** 方向的 die 个数。 |

**Die 平面索引**：第 `(cx, cy)` 个 die 的线性下标为 **`d = cy * chiplet_x + cx`**（行优先：cy 为行，cx 为列）。下文凡「长度为 `chiplet_x * chiplet_y` 的列表」均按此顺序排列。

### 2.2 `chiplet_connect`

| 项 | 类型 | 默认值 | 取值 | 含义 |
|----|------|--------|------|------|
| `chiplet_connect` | 字符串 | `x` | **`x`** | 仅在 **相邻 die 的 +X 方向** 建立 die-to-die（D2D）链路（同一行上左右相邻）。 |
| | | | **`xy`** | 在 **+X 与 +Y** 两个方向均建立 D2D（同行相邻与同列相邻）。 |

若取值不是 `x` 或 `xy`，仿真将报错退出。

### 2.3 `chiplet_k`

| 项 | 类型 | 默认值 | 含义 |
|----|------|--------|------|
| `chiplet_k` | 整数 ≥ 1 | 8 | **默认**每个 die 内部为 **`chiplet_k × chiplet_k` 的 2D mesh**，即每 die 上路由器（节点）数为 **`chiplet_k²`**。 |

当通过 `chiplet_die_k` 为每个 die 单独指定边长时，`chiplet_k` 仍作为**未覆盖 die的缺省边长**参与解析；生成工具里也会写入该字段（通常取列表首元素）以满足配置存在性。

### 2.4 `chiplet_die_k`

| 项 | 类型 | 含义 |
|----|------|------|
| `chiplet_die_k` | 整型数组（字符串形式，见 §5） | **按 die 顺序**指定每个 die 的 mesh **边长 k**（每 die **`k × k`** 个路由器）。 |

**允许形态**（解析后整数个数）：

- **空**：所有 die 使用 `chiplet_k`。
- **长度为 1**：该值广播到全部 `chiplet_x * chiplet_y` 个 die。
- **长度等于 `chiplet_x * chiplet_y`**：与 §2.1 的 die 顺序一致，逐项指定。

**约束**：

- 每个元素必须 **≥ 1**。
- 在 **`chiplet_connect = x`** 时：对每个 `cy`，相邻列 `(cx, cx+1)` 的两个 die **k 必须相同**（D2D 边界上行数一致）。
- 在 **`chiplet_connect = xy`** 时：对每个相邻 die 对（+X 与 +Y）**k 也必须一致**。

违反时仿真器会报错。

---

## 3. 链路延迟

### 3.1 `chiplet_intra_latency`

| 项 | 类型 | 默认值 | 含义 |
|----|------|--------|------|
| `chiplet_intra_latency` | 整数 | 0 | **片内 mesh链路**（同一 die 内相邻路由器之间）的延迟。 |

**有效取值逻辑**（实现层面）：若配置值 **> 0**，则片内延迟取该值；若 **≤ 0**（含默认 0），则回退为 **1**（周期数语义与仿真器其他链路一致）。

### 3.2 `chiplet_die_intra_latency`

| 项 | 类型 | 含义 |
|----|------|------|
| `chiplet_die_intra_latency` | 整型数组（字符串形式） | **按 die 顺序**覆盖该片 **片内 mesh 延迟**；**元素为 0** 表示该 die **回退使用**由 `chiplet_intra_latency` 解析得到的默认片内延迟。 |

**允许形态**：

- **空**：不启用 per-die 覆盖，全部 die 使用 §3.1 的全局逻辑。
- **长度等于 `chiplet_x * chiplet_y`**：与 die 顺序一致。

长度不符时仿真器报错。

### 3.3 `chiplet_d2d_latency`

| 项 | 类型 | 默认值 | 含义 |
|----|------|--------|------|
| `chiplet_d2d_latency` | 整数 | 2 | **D2D 链路**上每条**单向** flit 通道及其 credit 通道的延迟。 |

实现中会取 **`max(1, chiplet_d2d_latency)`**，故最小有效值为 **1**。

**D2D 拓扑**：相邻 die 边界上，每个对齐的路由器对之间有一对 D2D 接口；每个接口由 **两条反向的单向 flit 通道 + 对应 credit 通道** 组成。

**两种实现**（由 `chiplet_cdc_enable` 选择）：

- **`chiplet_cdc_enable = 0`（默认）**：D2D 为普通 `FlitChannel` / `CreditChannel`，延迟由 `chiplet_d2d_latency` 统一设置（见上表）。
- **`chiplet_cdc_enable = 1`**：D2D 使用 **跨时钟域模型**（`CdcFlitChannel` / `CdcCreditChannel`）：路由器仍以仿真器**最细时间步**推进；链路在 **writer / reader 两侧各自的时钟沿**上采样，中间为 **异步 FIFO + 同步延迟（全局 tick 计）+ 线延迟**。详见 §3.4。

### 3.4 跨时钟域 D2D（`chiplet_cdc_enable = 1`）

仿真使用**单一最细时间轴**（`GetSimTime()`）；「独立时间基」体现在 **D2D 链路模块**上：每条 D2D 的 flit/credit 路径按 die 的 `chiplet_die_clock_period` / `chiplet_die_clock_phase` 决定 **在哪些全局 tick 上**完成 writer 侧入队与 reader 侧出队。路由器 `Send` 可在任意周期发生；CDC 入口用 **ingress 队列**吸收，在 **writer 时钟沿**上每次最多向异步域提交 **1 个 flit（或 1 个 credit）**。

| 配置项 | 类型 | 默认值 | 含义 |
|--------|------|--------|------|
| `chiplet_cdc_enable` | 0/1 | 0 | 为 **1** 时，所有 D2D 链路使用 CDC 通道；为 **0** 时使用直连通道。 |
| `chiplet_die_clock_period` | 整型数组 | 空（等价全1） | 每个 die 的**名义时钟周期**，以**最细仿真 tick** 为单位（≥1）。空或单元素广播。 |
| `chiplet_die_clock_phase` | 整型数组 | 空（等价全 0） | 与 `period` 对齐的相位；满足 `((t + phase) % period == 0)` 的时刻为该域在链路上的**有效沿**（概念上）。 |
| `chiplet_cdc_sync_cycles` | 整数 ≥ 0 | 2 | flit 路径：自 writer 沿锁入异步域后，再经过多少**全局 tick** 才允许进入「待 reader 交付」状态（同步器/打拍抽象）。 |
| `chiplet_cdc_credit_sync_cycles` | 整数 ≥ 0 | 2 | credit 路径：同上，可与 flit 路径分开配置。 |
| `chiplet_cdc_fifo_depth` | 整数 ≥ 1 | 64 | CDC 内部异步路径 + ingress 等合并后的**最大占用**；溢出会报错退出。 |
| `chiplet_cdc_gray_fifo` | 0/1 | 0 | 为 **1** 时，在 `chiplet_cdc_sync_cycles` / `chiplet_cdc_credit_sync_cycles` 之上，再按 Gray 指针同步器抽象追加延迟：`gray_stages * max(P_writer, P_reader)`（全局 tick）。 |
| `chiplet_cdc_gray_stages` | 整数 ≥ 0 | 2 | 与 `chiplet_cdc_gray_fifo` 联用；为 **0** 时不追加额外延迟。 |

**约束**：若任一 die 的 `period ≠ 1` 或 `phase ≠ 0`，必须设置 **`chiplet_cdc_enable = 1`**，否则配置解析报错。

**`chiplet_d2d_latency`** 在 CDC 模式下表示 **reader 侧可见后的线延迟**（仍取 `max(1,·)`），叠加在 CDC 交付之后，语义上对应封装/互连段的固定流水延迟。

示例配置：`src/examples/chiplet_mesh_2x1_k2_cdc`。

---

## 4. 与全局 `use_noc_latency` 的关系

`use_noc_latency` 为仿真器全局整数开关（默认 1）。在 **`chiplet_mesh`** 的 `_BuildNet` 中，仅当 **`chiplet_intra_latency > 0`** 时直接使用其值；**当 `chiplet_intra_latency ≤ 0` 时，当前代码将片内默认延迟固定为 1**，与 `use_noc_latency` 取 0 或 1 的分支结果相同。前端若需严格区分 NoC 延迟语义，应以实际代码为准，避免仅依赖参数名推断。

---

## 5. 数组类参数在文本配置中的写法

`chiplet_die_k` 与 `chiplet_die_intra_latency` 在配置文件中是**字符串字段**，内容为 BookSim 词法可解析的**花括号整型列表**：

- 格式：`{a,b,c,...}`
- **建议**：逗号后**不要加空格**（与生成脚本及历史示例一致，避免与词法/工具链差异导致问题）。
- 示例：`chiplet_die_k = {2,2};`、`chiplet_die_intra_latency = {3,1};`

空字符串表示「未提供列表」，行为见各节说明。

---

## 6. 规模与节点编号（便于前端展示或生成流量）

- **总路由器（节点）数**：  
  `N = Σ_d k_d²`，其中 `k_d` 为第 `d` 个 die 的边长（来自 `chiplet_die_k` 或 `chiplet_k`）。
- **路由器 ID**：构建后为 **0 … N−1** 的连续整数；同一 die 内按 **x 优先、再 y** 的 mesh 坐标映射到 ID（与 `ChipletGlobalCoordsToId` 一致）。
- **全局坐标**：任意 ID 可映射到 `(cx, cy, x, y)`，其中 `(cx,cy)` 为 die 索引，`(x,y)` 为该 die 内 mesh 坐标（**0 ≤ x,y < k**）。

---

## 7. JSON 规格 →文本配置（`tools/chiplet_spec_to_config.py`）

若前端或后端先产出 JSON，再由脚本生成 `.cfg`，脚本按根对象字段**自动选择格式**：存在 **`mesh_chiplet`** 时按 **v1** 解析；否则若存在 **`chiplet`** 则按 **legacy** 解析。

### 7.0 v1：`schema_version` + `mesh_chiplet` + `d2d_cdc` + `sim`

推荐用于前端：**路由写死为** `chiplet_mesh` + `dim_order_chiplet_mesh`（若 JSON 中写其它值，生成器会**覆盖**并可能发出警告）。**`mesh_chiplet.dies`** 长度须为 **`grid.x * grid.y`**，顺序为 **行优先** `d = cy * chiplet_x + cx`（与 §2.1 一致）。每个 die 至少包含 **`k`** 与 **`clock.period` / `clock.phase`**（整数，语义同 `chiplet_die_clock_*`：相对最细仿真 tick）。

| JSON 路径 | 对应 BookSim 键 / 行为 |
|-----------|-------------------------|
| `routing.*` | 强制写出 `topology`、`routing_function` |
| `mesh_chiplet.grid.x` / `grid.y` | `chiplet_x`、`chiplet_y` |
| `mesh_chiplet.connect` | `chiplet_connect` |
| `mesh_chiplet.dies[].k` | `chiplet_die_k`（脚本校验相邻 die 在 `x`/`xy` 缝上 **k 一致**） |
| `mesh_chiplet.intra_latency` | 默认 `chiplet_intra_latency`；若某 die 的片内延迟与默认不同则写出 `chiplet_die_intra_latency` |
| `mesh_chiplet.d2d_wire_latency` | `chiplet_d2d_latency` |
| `mesh_chiplet.dies[].clock` | `chiplet_die_clock_period` / `chiplet_die_clock_phase` 数组 |
| `d2d_cdc.enabled` | `chiplet_cdc_enable`（0/1） |
| `d2d_cdc.fifo_depth` 等 | `chiplet_cdc_fifo_depth`、`chiplet_cdc_sync_cycles`、`chiplet_cdc_credit_sync_cycles`；`gray.enabled` / `gray.stages` → `chiplet_cdc_gray_fifo`、`chiplet_cdc_gray_stages` |

**生成器约束**：若任一 die **`period ≠ 1` 或 `phase ≠ 0`**，则必须 **`d2d_cdc.enabled: true`**，否则脚本**报错退出**（与 §3.4 仿真器规则一致）。

**示例与校验**：[`tools/mesh_chiplet_spec.sample.json`](../tools/mesh_chiplet_spec.sample.json)（异频 CDC）、[`tools/mesh_chiplet_spec_sync_d2d.json`](../tools/mesh_chiplet_spec_sync_d2d.json)（同频、关 CDC）、[`tools/mesh_chiplet_spec_cdc_fifo_stress.json`](../tools/mesh_chiplet_spec_cdc_fifo_stress.json)（极小 FIFO + 高注入率，用于**预期**触发 `CdcFlitChannel` 溢出报错）。冒烟：`tools/run_mesh_chiplet_json_smoke.sh`（由仓库根执行，在 `src` 下编译并运行前两例）。

### 7.1 Legacy：`chiplet` 对象

| JSON 字段 | 类型 | 对应 / 含义 |
|-----------|------|-------------|
| `x` | 整数 | → `chiplet_x` |
| `y` | 整数 | → `chiplet_y` |
| `connect` | 字符串 | → `chiplet_connect`，默认 `x` |
| `k` | 整数 | 当未提供 `die_k` 时，所有 die 的 mesh 边长；→同时写入 `chiplet_k` |
| `die_k` | 整数数组，长度 1 或 `x*y` | → `chiplet_die_k`；顺序为 **行优先** `cy*cx+cx`（与脚本注释一致） |
| `die_intra_latency` | 整数或整数数组 | → `chiplet_die_intra_latency`；规则见脚本：全 ≤0 可不输出 |
| `intra_latency` | 整数 | → `chiplet_intra_latency`，默认1 |
| `d2d_latency` | 整数 | → `chiplet_d2d_latency`，默认 2 |
| `cdc_enable` | 布尔 | 为 true 时输出 `chiplet_cdc_enable = 1` 及下列 CDC 行 |
| `cdc_fifo_depth` 等 | 整数 | → `chiplet_cdc_fifo_depth`、`cdc_sync_cycles`、`cdc_credit_sync_cycles`、`cdc_gray_fifo`、`cdc_gray_stages`（见 [`tools/chiplet_spec_to_config.py`](tools/chiplet_spec_to_config.py)） |
| `die_clock_period` / `die_clock_phase` | 整数或数组 | → `chiplet_die_clock_*`（长度1 或 `x*y`） |

### 7.2 `sim` 对象（节选）

脚本会一并写出 VC、allocator、延迟、流量、注入率等与拓扑无关的仿真参数，例如：`num_vcs`、`vc_buf_size`、`credit_delay`、`traffic`、`packet_size`、`sim_type`、`injection_rate`、`latency_thres` 等。含义与 BookSim2 通用配置一致，**不属于 chiplet 拓扑专有语义**；前端若只关心拓扑，可仅使用 §2–§3 与 §7.0–§7.1。

完整示例：legacy 见 **`tools/chiplet_spec.sample.json`**；v1 见 **`tools/mesh_chiplet_spec.sample.json`**。

---

## 8. 校验失败时的典型原因（便于前端预检查）

- `chiplet_x`、`chiplet_y`、`chiplet_k` 任一为 **< 1**。
- `chiplet_connect` 不是 **`x`** / **`xy`**。
- `chiplet_die_k` 长度不是 **0、1 或 x·y**，或元素 **< 1**，或与 `chiplet_connect` 组合下 **相邻 die 的 k 不一致**。
- `chiplet_die_intra_latency` 非空且长度 **≠ x·y**。
- **v1 JSON**：`mesh_chiplet.dies` 长度 **≠ grid.x·y**；**异频/异相** 但 **`d2d_cdc.enabled`** 为 false（生成器与仿真器均不允许）。
- **v1 JSON**：`tools/chiplet_spec_to_config.py` 会校验 **缝上相邻 die 的 `k` 一致**（与 C++ 侧一致）。

---

## 9. 文档版本与代码位置

- 拓扑与通道：`src/networks/chiplet_mesh.cpp`
- D2D CDC 分配：`src/networks/chiplet_d2d_cdc.hpp`、`src/networks/chiplet_d2d_cdc.cpp`
- 时钟沿工具：`src/networks/chiplet_clock.hpp`
- CDC 链路：`src/cdc_channel.hpp`、`src/cdc_channel.cpp`
- 默认注册：`src/booksim_config.cpp`（`chiplet_*` 字段）
- 路由：`src/routefunc.cpp`（`dim_order_chiplet_mesh`）
- 全局布局只读状态：`src/globals.hpp`、`src/main.cpp`（`gChiplet*`）
- v1 JSON Schema：`tools/mesh_chiplet.schema.json`；生成与冒烟：`tools/chiplet_spec_to_config.py`、`tools/run_mesh_chiplet_json_smoke.sh`

实现若有变更，以源码为准；本文档随仓库维护，用于前后端对齐**参数语义**。
