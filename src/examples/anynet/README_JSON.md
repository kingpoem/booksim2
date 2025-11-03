# JSON 配置文件使用说明

## 概述

本目录包含了一个 JSON 格式的配置文件示例 `anynet_config.json`，用于替代传统的文本格式配置文件。

## 配置文件对比

### 原始配置文件 (anynet_config)
- 文本格式，使用 `key = value;` 语法
- 所有路由器使用相同的配置参数

### JSON 配置文件 (anynet_config.json)
- JSON 格式，更易读和编辑
- 支持路由器特定配置

## JSON 配置结构

```json
{
  "全局配置参数": "值",
  "routers": {
    "路由器ID": {
      "参数名": "值"
    }
  }
}
```

## 示例配置说明

在 `anynet_config.json` 中：

### 全局配置
- `topology`: 网络拓扑类型（anynet）
- `routing_function`: 路由算法（min）
- `network_file`: 网络拓扑描述文件（anynet_file）
- `num_vcs`: 默认虚拟通道数（1）
- `vc_buf_size`: 默认 VC 缓冲区大小（3）

### 路由器特定配置
- 路由器 0: `num_vcs=2`, `vc_buf_size=4`
- 路由器 1: `num_vcs=2`, `vc_buf_size=5`
- 路由器 2: `num_vcs=1`, `vc_buf_size=3` (与全局配置相同)

## 运行方法

```bash
cd src/examples/anynet
../../booksim anynet_config.json
```

## 工作原理

1. 系统检测到 `.json` 扩展名，使用 JSON 解析器
2. 全局配置参数被解析到配置映射中
3. 路由器特定配置被存储到独立映射中
4. 当创建路由器时：
   - 首先检查是否有该路由器的特定配置
   - 如果有，使用特定配置
   - 如果没有，回退到全局配置

## 验证测试

运行测试：
```bash
cd src/examples/anynet
../../booksim anynet_config.json
```

预期输出应包含：
- "BEGIN Configuration File (JSON): anynet_config.json"
- 网络解析信息
- 路由器创建信息
- 仿真统计结果

测试状态：✅ 通过
- JSON 配置成功解析
- 网络文件正确加载
- 仿真成功运行
- 路由器特定配置机制正常工作

## 注意事项

1. JSON 文件必须使用 `.json` 扩展名才能被识别
2. 路由器 ID 在 JSON 中必须使用字符串格式（如 "0", "1"）
3. 如果路由器特定配置中缺少某个参数，会自动使用全局配置
4. 保持向后兼容：旧的文本格式配置文件仍然可以正常使用
