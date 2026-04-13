# Chiplet Mesh 配置参数说明（前端 / 自动化对接）

本文档说明在 BookSim2 中使用 **`topology = chiplet_mesh`** 时，与片芯（chiplet）拓扑、链路与路由相关的**配置项含义与约束**。不涉及页面布局、组件样式等前端实现建议。

仿真器从**文本配置文件**（或等价字段集合）读取这些项；仓库内亦提供 `tools/chiplet_spec_to_config.py`，可将 **JSON 规格** 转为同语义的书本配置文件。

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

**D2D 建模方式（与当前实现一致）**：相邻 die 边界上，每个对齐的路由器对之间有一对 D2D 接口；每个接口由 **两条反向的单向 flit 通道 + 对应 credit 通道** 组成，**不再**经过单独的桥接模块。两条通道的延迟均设为上述 D2D 延迟。

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

若前端或后端先产出 JSON，再由脚本生成 `.cfg`，下列字段与上述参数对应。根对象下分 **`chiplet`** 与 **`sim`**（仿真通用项）。

### 7.1 `chiplet` 对象

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

### 7.2 `sim` 对象（节选）

脚本会一并写出 VC、allocator、延迟、流量、注入率等与拓扑无关的仿真参数，例如：`num_vcs`、`vc_buf_size`、`credit_delay`、`traffic`、`packet_size`、`sim_type`、`injection_rate`、`latency_thres` 等。含义与 BookSim2 通用配置一致，**不属于 chiplet 拓扑专有语义**；前端若只关心拓扑，可仅使用 §2–§3与 §7.1。

完整示例结构见仓库 **`tools/chiplet_spec.sample.json`**。

---

## 8. 校验失败时的典型原因（便于前端预检查）

- `chiplet_x`、`chiplet_y`、`chiplet_k` 任一为 **< 1**。
- `chiplet_connect` 不是 **`x`** / **`xy`**。
- `chiplet_die_k` 长度不是 **0、1 或 x·y**，或元素 **< 1**，或与 `chiplet_connect` 组合下 **相邻 die 的 k 不一致**。
- `chiplet_die_intra_latency` 非空且长度 **≠ x·y**。

---

## 9. 文档版本与代码位置

- 拓扑与通道：`src/networks/chiplet_mesh.cpp`
- 默认注册：`src/booksim_config.cpp`（`chiplet_*` 字段）
- 路由：`src/routefunc.cpp`（`dim_order_chiplet_mesh`）
- 全局布局只读状态：`src/globals.hpp`、`src/main.cpp`（`gChiplet*`）

实现若有变更，以源码为准；本文档随仓库维护，用于前后端对齐**参数语义**。
