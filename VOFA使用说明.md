# VOFA 车辆控制系统 — 使用说明

> 适用工程：LQ_TC387_Software_Library
> 模块版本：V1.0
> 日期：2026-07-11

---

## 一、快速开始（3步上手）

### 第1步：确认硬件连接

将无线模块（WiFi/蓝牙二合一）连接到 TC387 的 **UART3**：

| TC387 引脚 | 功能 | 连接模块 |
|-----------|------|---------|
| P00_0 | UART3 TX | 模块 RX |
| P00_1 | UART3 RX | 模块 TX |
| GND | 地 | 模块 GND |

> 注：核心板自带 WiFi+蓝牙模块，如果已焊接好无需额外连线。

### 第2步：编译烧录

1. 打开 AURIX-Studio
2. 确认 [config.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/config.h) 中 `VOFA_CONTROL_ENABLE = 1`
3. 编译工程，烧录到 TC387

### 第3步：连接 VOFA+ 上位机

1. PC 连接 WiFi 模块的 AP 热点（或蓝牙配对）
2. 打开 VOFA+ 软件
3. 串口配置：波特率 `115200`，8N1
4. 数据引擎选择 **FireWater**
5. 点击"打开串口"
6. 波形窗口应出现12通道数据

> 如果波形无数据，检查串口号和波特率，确认模块已正确透传。

---

## 二、命令格式说明

### 2.1 下行命令（上位机→车辆）

格式：`命令名:数值\n`

每条命令以换行符 `\n`（0x0A）结尾。

**示例：**
```
speed_kp:35.0
base_speed:150
car_start:1
car_stop:1
```

### 2.2 上行数据（车辆→上位机）

格式（FireWater）：`STATE:ch0,ch1,ch2,...,ch11\n`

| 通道 | 数据 | 单位 |
|------|------|------|
| ch0 | 左轮目标速度 | cm/s |
| ch1 | 左轮实际速度 | cm/s |
| ch2 | 右轮目标速度 | cm/s |
| ch3 | 右轮实际速度 | cm/s |
| ch4 | 左轮PWM | bit |
| ch5 | 右轮PWM | bit |
| ch6 | 目标角速度 | °/s |
| ch7 | 实际角速度 | °/s |
| ch8 | 目标偏航角 | ° |
| ch9 | 实际偏航角 | ° |
| ch10 | 赛道偏差 | px |
| ch11 | 车辆状态 | 0=停车/1=运行 |

---

## 三、完整命令列表

### 3.1 PID 参数调节

| 命令名 | 类型 | 范围 | 说明 |
|--------|------|------|------|
| `speed_kp` | float | 0~100 | 速度环比例系数 |
| `speed_ki` | float | 0~10 | 速度环积分系数（修改时自动清零积分器） |
| `speed_kd` | float | 0~10 | 速度环微分系数 |
| `gyro_kp` | float | 0~10 | 角速度环比例系数 |
| `gyro_ki` | float | 0~10 | 角速度环积分系数（修改时自动清零积分器） |
| `gyro_kd` | float | 0~10 | 角速度环微分系数 |
| `dir_kp` | float | 0~10 | 方向环比例系数 |
| `dir_ki` | float | 0~5 | 方向环积分系数（修改时自动清零积分器） |
| `dir_kd` | float | 0~5 | 方向环微分系数 |
| `angle_kp` | float | 0~10 | 角度环比例系数 |
| `angle_ki` | float | 0~10 | 角度环积分系数 |
| `angle_kd` | float | 0~10 | 角度环微分系数 |

### 3.2 车速与模式控制

| 命令名 | 类型 | 范围 | 说明 |
|--------|------|------|------|
| `base_speed` | float | 0~230 | 基础巡航速度 (cm/s) |
| `target_yaw` | float | -180~180 | 目标偏航角 (°) |
| `angle_mode` | int | 0/1 | 0=方向环模式, 1=角度环模式 |

### 3.3 发车/停车

| 命令名 | 值 | 说明 |
|--------|-----|------|
| `car_start` | 1 | 发车（清零积分器，释放电机） |
| `car_stop` | 1 | 紧急停车（立即停止电机） |

---

## 四、VOFA+ 控件配置

### 4.1 滑块控件配置

1. 从左侧控件库拖入"滑块"控件
2. 左侧第三个绿色图标 → 添加新命令
3. 命令内容填写格式：`命令名:%f\n`（`%f` 运行时替换为滑块值）
4. 将命令绑定到滑块的"值改变"事件
5. 右键滑块设置范围（from/to）和步长

**滑块配置参考表：**

| 滑块名称 | 命令内容 | 范围 | 步长 |
|---------|---------|------|------|
| 速度Kp | `speed_kp:%f\n` | 0~50 | 0.1 |
| 速度Ki | `speed_ki:%f\n` | 0~2 | 0.01 |
| 速度Kd | `speed_kd:%f\n` | 0~5 | 0.1 |
| 角速度Kp | `gyro_kp:%f\n` | 0~5 | 0.1 |
| 角速度Ki | `gyro_ki:%f\n` | 0~2 | 0.01 |
| 角速度Kd | `gyro_kd:%f\n` | 0~5 | 0.1 |
| 方向Kp | `dir_kp:%f\n` | 0~5 | 0.1 |
| 方向Ki | `dir_ki:%f\n` | 0~1 | 0.01 |
| 方向Kd | `dir_kd:%f\n` | 0~3 | 0.1 |
| 角度Kp | `angle_kp:%f\n` | 0~5 | 0.1 |
| 角度Ki | `angle_ki:%f\n` | 0~2 | 0.01 |
| 角度Kd | `angle_kd:%f\n` | 0~5 | 0.1 |
| 基础速度 | `base_speed:%f\n` | 0~250 | 1.0 |
| 目标偏航 | `target_yaw:%f\n` | -180~180 | 1.0 |

### 4.2 按钮控件配置

1. 拖入"按钮"控件
2. 添加命令（固定字符串，不含 `%f`）
3. 绑定到按钮的"鼠标按下"事件

**按钮配置参考表：**

| 按钮名称 | 命令内容 |
|---------|---------|
| 发车 | `car_start:1\n` |
| 停车 | `car_stop:1\n` |
| 方向环模式 | `angle_mode:0\n` |
| 角度环模式 | `angle_mode:1\n` |

---

## 五、串口助手调试（无需 VOFA+）

用任意串口助手（如 SSCOM、Putty）也能调试：

1. 连接串口，波特率 115200
2. 发送命令（需勾选"发送新行"或手动加 `\n`）

**测试命令示例：**
```
speed_kp:35.0          ← 修改速度环Kp
base_speed:100         ← 设置基础速度100cm/s
car_start:1            ← 发车
gyro_kp:2.5            ← 修改角速度环Kp
car_stop:1             ← 停车
```

---

## 六、典型操作流程

### 6.1 PID 调参流程

```
1. 连接 VOFA+ → 波形显示正常
2. 设置 base_speed:100（低速起步）
3. 点击"发车" → 车辆开始行驶
4. 观察速度波形（ch0~ch3）
5. 逐步调大 speed_kp，观察响应速度
6. 适当加 speed_ki，消除稳态误差
7. 切换观察角速度波形（ch6~ch7）
8. 调整 gyro_kp/gyro_kd
9. 调参满意后点击"停车"
10. 记录最优参数，更新 config.h
```

### 6.2 发车/停车流程

```
1. 确认 VOFA+ 已连接
2. 设置 base_speed 到合适值
3. 点击"发车"按钮 → 车辆启动
4. 运行中可随时调节参数
5. 需要停车时点击"停车"按钮
6. 若通信中断，500ms 后自动停车
```

### 6.3 角度环测试流程

```
1. 发送 angle_mode:1 → 切换到角度环
2. 发送 target_yaw:90 → 目标偏航角设为90°
3. 点击发车 → 车辆转向到90°
4. 观察偏航角波形（ch8~ch9）
5. 调整 angle_kp/angle_kd 优化响应
6. 测试完毕发送 angle_mode:0 → 切回方向环
```

---

## 七、安全机制

| 机制 | 说明 |
|------|------|
| **通信超时保护** | 车辆运行中，500ms 未收到任何命令 → 自动停车 |
| **参数限幅** | 所有 PID 参数修改经 `constrain_float` 限幅，防止异常值 |
| **积分器清零** | 修改 Ki 时自动清零积分器，防止参数切换导致突变 |
| **停车优先级** | `car_stop` 命令立即调用 `Motor_Stop()`，最高优先级 |
| **缓冲区溢出保护** | 接收超长命令时自动重置缓冲区 |

---

## 八、常见问题排查

### Q1：波形无数据

**检查步骤：**
1. VOFA+ 数据引擎是否选了 **FireWater**？
2. 波特率是否为 **115200**？
3. 串口号是否正确？
4. 无线模块是否已配对/连接？
5. 用串口助手直接收数据，看是否有 `STATE:...` 格式输出？

### Q2：参数改不动

**检查步骤：**
1. 命令格式是否正确？应为 `命令名:数值\n`
2. 命令名拼写是否正确？（如 `speed_kp` 不是 `SpeedKp`）
3. 串口助手是否勾选了"发送新行"？
4. `VOFA_CONTROL_ENABLE` 是否为 1？

### Q3：车辆无响应

**检查步骤：**
1. 是否发送了 `car_start:1` 发车命令？
2. `base_speed` 是否设置为大于 0 的值？
3. 超时保护是否触发？（500ms 无通信会自动停车）
4. 试着持续发送命令（如每 200ms 发一次），保持通信不超时

### Q4：通信经常断开/超时停车

**原因：** 超时保护阈值 500ms 太短

**解决：** 修改 [LQ_Vofa.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/inc/LQ_Vofa.h) 中的宏：
```c
#define VOFA_TIMEOUT_MS      2000    /* 改为 2 秒 */
```

### Q5：波形刷新太慢/太快

**原因：** 发送频率不合适

**解决：** 修改 [LQ_Vofa.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/inc/LQ_Vofa.h) 中的宏：
```c
#define VOFA_SEND_INTERVAL   20      /* 20→50Hz, 10→100Hz */
```

### Q6：如何关闭 VOFA 模块恢复 GPS 功能

修改 [config.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/config.h)：
```c
#define VOFA_CONTROL_ENABLE  0
```
重新编译即可。UART3 中断自动恢复为 GPS 接收模式。

---

## 九、文件修改清单

| 文件 | 改动类型 | 说明 |
|------|---------|------|
| [LQ_Vofa.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/inc/LQ_Vofa.h) | 新建 | 模块头文件 |
| [LQ_Vofa.c](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/LQ_Vofa.c) | 新建 | 模块实现 |
| [config.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/config.h) | 修改 | 添加 `VOFA_CONTROL_ENABLE` 宏 |
| [lq_include.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/lq_include.h) | 修改 | 末尾添加 `#include "LQ_Vofa.h"` |
| [Cpu0_Main.c](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/Cpu0_Main.c) | 修改 | 添加 `Vofa_Init()` + `Vofa_MainLoop()` |
| [irq.c](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Main/irq.c) | 修改 | 3处中断集成 |
| [LQ_PWM_Moto.c](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/LQ_PWM_Moto.c) | 修改 | 添加 `Get_Target_Gyro()` + `Set_Angle_Mode()` |
| [LQ_PWM_Moto.h](file:///d:/smart%20car/workspace527_vofa/LQ_TC387_Software_Library/Src/APP/inc/LQ_PWM_Moto.h) | 修改 | 添加对应声明 |

---

## 十、架构说明

```
┌──────────────────────────────────────────────────┐
│                    TC387 MCU                      │
│                                                   │
│  ┌─────────────┐    ┌──────────────────────┐     │
│  │  CCU6 1ms   │    │   UART3 RX 中断       │     │
│  │  控制中断    │    │   (逐字节接收)        │     │
│  │             │    │                       │     │
│  │ Vofa_CCU6_  │    │  Vofa_RX_Handler()   │     │
│  │ Tick()      │    │  → Vofa_ParseLine()  │     │
│  │ → ms计数    │    │  → Vofa_ApplyParam() │     │
│  │ → 置发送标志 │    │    (修改PID参数)      │     │
│  └──────┬──────┘    └──────────┬───────────┘     │
│         │                      │                  │
│  ┌──────┴──────────────────────┴───────────┐     │
│  │              主循环 while(1)             │     │
│  │                                          │     │
│  │  Vofa_MainLoop()                        │     │
│  │  → 检查发送标志                          │     │
│  │  → Vofa_SendState() (阻塞发送~8ms)     │     │
│  │    → UART_PutBuff() → UART3 TX          │     │
│  └──────────────────────────────────────────┘     │
│                                                   │
│  ┌─────────────┐                                 │
│  │  STM 1s     │  Vofa_CheckTimeout()           │
│  │  定时中断   │  → 500ms无通信 → Motor_Stop()  │
│  └─────────────┘                                 │
│                                                   │
└───────────────────┬───────────────────────────────┘
                    │ UART3 (P00_0/P00_1)
                    │ 115200 8N1
            ┌───────┴───────┐
            │  WiFi/蓝牙模块 │
            └───────┬───────┘
                    │ 无线
            ┌───────┴───────┐
            │   PC VOFA+    │
            │  FireWater引擎 │
            └───────────────┘
```

**关键设计决策：**
- 发送放在主循环而非中断中，因为 `UART_PutBuff` 是阻塞式（100字节@115200约需8.7ms）
- 接收解析放在中断中，因为 `strcmp`/`atof` 执行很快（几微秒）
- 超时检测放在 STM 1s 中断中，1秒检查一次足够
