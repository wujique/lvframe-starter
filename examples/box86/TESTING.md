# box86 测试指南

本文档说明如何对 box86 进行交互式测试。编译、运行和环境配置请参考 [README.md](README.md)。

---

## 启动验证

程序启动后会输出初始化日志，确认以下关键行均出现：

```
[font] Chinese font loaded: ...
[main] Platform initialized
[main] Business thread started
[main] Home page opened successfully
[main] Entering main loop
```

程序启动时会自动创建两个默认的普通灯设备（ID: 1, 2）。

---

## Shell 命令交互

程序运行后可直接在终端输入命令，业务线程实时读取标准输入并执行。

### 命令总览

| 命令 | 语法 | 描述 |
|------|------|------|
| `list` | `list` | 列出所有设备 |
| `get` | `get system` / `get <id>` | 查询系统或设备状态 |
| `add` | `add <type> <name>` | 添加新设备 |
| `del` | `del <id>` | 删除设备 |
| `move` | `move <id> <position>` | 调整设备页顺序（position 从 0 开始） |
| `set` | `set system <field> <value>` / `set <id> <field> <value>` | 设置系统或设备属性 |

---

### 1. 列出设备

```
list
```

输出示例：
```
ID   类型          名称                  状态
1    light        Living Room Light     off
2    light        bath Room Light       off
```

---

### 2. 查询状态

```bash
get system      # 系统设置
get 1           # 指定设备
```

输出示例：
```
# 系统
brightness=50 volume=30 network=1

# 普通灯
id=1 name=Living Room Light onoff=0

# 色温灯
id=3 name=Kitchen onoff=0 cct=4000

# 窗帘
id=4 name=LivingRoom pos=0
```

---

### 3. 添加设备

```bash
add light Bedroom        # 普通灯
add cct_light Kitchen    # 色温灯
add curtain LivingRoom   # 电动窗帘
```

- 设备类型：`light`、`cct_light`、`curtain`
- 设备名称不含空格
- 最多支持 20 个设备
- 成功响应：`added device id=<N>`
- UI 效果：新设备页自动添加到滑动容器

---

### 4. 控制设备

#### 普通灯

```bash
set 1 onoffsta 1    # 开
set 1 onoffsta 0    # 关
```

#### 色温灯

```bash
set 3 onoffsta 1            # 开关
set 3 color_temp 3000       # 色温（2700~6500K）
```

#### 电动窗帘

```bash
set 4 command OPEN    # 打开
set 4 command CLOSE   # 关闭
set 4 command STOP    # 停止
set 4 position 75     # 开合度（0~100）
```

字段值超出范围时自动限制。

---

### 5. 系统设置

```bash
set system brightness 80    # 屏幕亮度（0~100）
set system volume 60        # 音量（0~100）
set system network 1        # 网络开关（0/1）
```

---

### 6. 删除设备

```bash
del 2
```

执行流程：业务线程 → 通知 UI 销毁页面 → UI 确认 → 业务线程释放数据模型。
输出日志：`[BIZ] device 2 model freed`

---

### 7. 移动设备

```bash
move 1 2    # 将 ID=1 的设备移到位置 2
```

---

## 完整测试流程

```bash
# 启动（无头模式）
SDL_VIDEODRIVER=dummy ./box86 &
sleep 1

# 初始状态
list

# 添加设备
add light Bedroom
add cct_light Kitchen
add curtain LivingRoom
list

# 控制设备
set 1 onoffsta 1
set 3 color_temp 3000
set 4 command OPEN

# 系统设置
set system brightness 90
set system volume 40
set system network 1

# 查询
get system
get 1
get 3
get 4

# 删除和移动
del 3
move 4 1
list
```

---

## 故障排除

**Shell 命令无响应**
- 确认看到 `[main] Entering main loop` 日志
- 确认标准输入未被重定向

**UI 不更新**
- 检查 EventBus 初始化日志
- 确认设备页已订阅 `EVENT_APP_MESSAGE`

**无法打开显示**
```bash
SDL_VIDEODRIVER=dummy ./box86   # 无头模式
xvfb-run -a ./box86             # 虚拟帧缓冲
```

**过滤调试日志**
```bash
./box86 2>&1 | grep -E "\[DevicePage\]|\[HomePage\]|\[AppBusAdapter\]"
```

---

## 架构验证要点

| 特性 | 验证方法 |
|------|---------|
| 线程安全 | 并发发送多条命令，UI 无崩溃 |
| 数据一致性 | set 后立即 get，值一致 |
| 生命周期管理 | del 后 list 不再显示，UI 页面消失 |
| 事件驱动 | Shell 命令后 UI 自动刷新，无需手动触发 |
| 资源管理 | 长时间运行后内存无持续增长 |

---

## 性能参考

- Shell 命令 → UI 更新延迟：正常 < 100ms
- 内存监控：`ps aux | grep box86`
- 压力测试：批量添加 10+ 设备，持续发送控制命令
