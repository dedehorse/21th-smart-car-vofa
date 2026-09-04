# 基于VOFA上位机与无线串口的车辆参数控制及发车/停车控制系统实施方案

> 项目：LQ_TC387_Software_Library 智能车工程
> 日期：2026-07-10
> 版本：V2.0（最终版，修正了 FireWater 协议的认知错误）

---

## 〇、V2.0 修订说明（相对 V1.0）

V1.0 存在一个**关键事实性错误**：把 `#P<通道号>=<值>!` 格式说成是"FireWater 协议的下行命令规范"。

经查阅 VOFA+ 官方文档（https://www.vofa.plus/docs/learning/dataengines/firewater/）确认：

1. **FireWater 是一个"数据引擎"**，只规定**上行数据**（单片机→上位机）的格式：`"<any>:ch0,ch1,...,chN\n"`
2. **FireWater 协议本身不规定下行命令格式**！下行命令靠 VOFA+ 的"控件命令绑定"机制，发送内容完全由用户自定义
3. `d:\qq\file\Vofa.c` 中的 `#P1=%f!` 只是该参考文件作者**自定义**的帧格式，不是 FireWater 协议规范

V2.0 据此重新设计了下行命令方案，并提供两种可选格式供决策。

---

## 一、项目概述

基于 **TC387智能车工程**，通过 VOFA+ 上位机与无线串口，实现：
- PID 参数在线调节（速度环/角速度环/方向环）
- 实时波形显示（速度、角速度、PWM、偏航角等）
- 远程发车/停车控制
- 无线串口可靠通信

---

## 二、阶段一：技术调研与环境准备

### 2.1 VOFA+ 软件架构认知（重要）

VOFA+ 不是传统"单向发指令"的上位机，而是**事件驱动型控制平台**。核心机制：

- **数据引擎**：负责解析"下位机→上位机"的数据流（FireWater/JustFloat/RawData 三选一）
- **控件命令绑定**：负责"上位机→下位机"的命令下发，与数据引擎**相互独立**
- **控件事件**：滑块"值改变"、按钮"鼠标按下"等事件触发命令发送

**关键认知**：数据引擎协议（如 FireWater）和下行命令格式是两件事。FireWater 只管上行数据怎么解析成波形，下行命令发什么完全由用户在"命令绑定"里自定义。

### 2.2 FireWater 协议真实规范（仅限上行数据）

**来源**：VOFA+ 官方文档 https://www.vofa.plus/docs/learning/dataengines/firewater/

#### 协议特点
- CSV 风格的字符串流，直观简洁，编程像 printf
- 字符串解析消耗运算资源，**建议仅在通道数量不多、发送频率不高时使用**
- 遇到换行才打印数据

#### 上行数据格式（单片机→上位机）

```
"<any>:ch0,ch1,ch2,...,chN\n"
```

规则：
- `any` 和冒号**可以省略**
- **换行 `\n` 不可省略**（也可为 `\n\r` 或 `\r\n`）
- `any` **不可以是 "image"**（图片前导帧专用前缀）
- 此处 `\n` 是真正的换行字符（0x0A），不是字面的斜杠+n

示例：
```c
// 带 any 前缀
printf("channels: 1.386578,0.977929,-0.628913,-0.942729\n");
// 不带 any 和冒号
printf("1.386578,0.977929,-0.628913,-0.942729\n");
// 任意前缀（如 STATE）
printf("STATE:100.0,98.5,100.0,99.2\n");
```

#### 图片前导帧（本项目暂不用）
```
"image:IMG_ID, IMG_SIZE, IMG_WIDTH, IMG_HEIGHT, IMG_FORMAT\n"
```

#### 重要注意事项
- FireWater 以换行作为帧结束标志，没有换行就不解析
- 如果一直不发换行，缓冲区会爆满，软件卡死
- 图片前导帧的 `any` 必须是 `image`

### 2.3 下行命令机制（控件→下位机）

**FireWater 协议不规定下行命令格式**。下行命令通过 VOFA+ 的"命令绑定"机制实现：

#### 操作流程
1. 添加控件（滑块/按钮/输入框）
2. 左侧第三个绿色图标 → 命令配置 → 添加新命令
3. 编辑命令名称和发送内容（支持 `%f` 占位符，运行时替换为当前值）
4. 将命令与控件事件绑定（如滑块"值改变"、按钮"鼠标按下"）

#### `%f` 占位符说明
- `%f` 是 VOFA+ 的占位符，运行时自动替换为控件当前浮点值
- 例如命令内容填 `motor_speed:%f\n`，滑块拖到 25.5 时实际发送 `motor_speed:25.5\n`
- 按钮没有数值，直接填固定字符串如 `start:1\n`

#### 两种主流下行命令格式

| 方案 | 格式示例 | 解析方式 | 优点 | 缺点 |
|------|---------|---------|------|------|
| **A. FireWater 风格** | `motor_speed:25.5\n` | `strchr`找冒号+`atof` | 可读性强、与上行风格一致、官方文档推荐、调试直观 | 需处理缓冲区残留 |
| **B. 自定义帧头帧尾** | `#P1=25.5!` | 状态机解析 | 帧边界明确、容错好、Vofa.c 已有参考逻辑 | 可读性稍差 |

**本项目推荐方案 A**（FireWater 风格 `控件名:数值\n`），理由：
1. 符合 VOFA+ 官方文档和社区主流做法
2. 命令可读性强：`speed_kp:1.5\n` 比 `#P1=1.5!` 直观
3. 调试时串口助手直接发 `speed_kp:1.5\n` 即可测试
4. 下位机解析简单：`strchr(buf, ':')` + `atof`

**兼容性**：方案 B 的 `#P1=%f!` 格式也可在 VOFA+ 命令绑定里直接填写，下位机沿用 Vofa.c 的状态机解析。若用户坚持用 Vofa.c 参考逻辑，可切换到方案 B。

### 2.4 硬件清单
- TC387核心板 + 电机驱动板
- 无线串口模块（HC-05蓝牙 或 ESP8266 WiFi透传 或 工程已有图传模块）
- USB转串口模块（PC端连接）

### 2.5 软件清单
- **VOFA+** 1.3.10+（官网 https://www.vofa.plus/）
- AURIX-Studio 1.10.2（现有开发环境）
- 串口调试助手（备用验证）

### 2.6 环境配置步骤
1. 安装 VOFA+ 软件
2. 串口参数：波特率 115200、8N1、无校验
3. 数据引擎选择 **FireWater**
4. 测试串口回环（TX短接RX）

---

## 三、阶段二：现有资源分析

### 3.1 当前工程结构

```
LQ_TC387_Software_Library/
├── Main/
│   ├── Cpu0_Main.c              主程序入口
│   ├── config.h                 系统配置（PID参数、编译开关）
│   └── lq_include.h             总头文件
├── Src/
│   ├── APP/
│   │   ├── LQ_PWM_Moto.c        电机控制+三级PID环
│   │   ├── LQ_UART_Bluetooth.c  蓝牙串口示例
│   │   └── inc/LQ_PWM_Moto.h    电机控制头文件
│   ├── Driver/
│   │   ├── LQ_UART.c            UART驱动（4路UART+中断）
│   │   ├── LQ_Wireless_Config.c 无线模块AT配置
│   │   └── inc/LQ_UART.h        UART头文件
│   └── User/
│       ├── LQ_PID.c             PID算法实现
│       └── LQ_PID.h             PID参数结构体
```

### 3.2 现有可复用资源

| 资源 | 源文件 | 复用方式 |
|------|--------|---------|
| UART驱动 | LQ_UART.c / LQ_UART.h | 直接使用，支持4路UART+收发中断 |
| PID参数结构体 | LQ_PID.h 中 `pid_param_t` | 直接绑定到命令通道修改 |
| 电机状态结构体 | LQ_PWM_Moto.h 中 `MotorState` | 状态变量用于波形显示 |
| 三级PID环函数 | LQ_PWM_Moto.c | 通过变量接口在线修改参数 |
| 无线模块配置 | LQ_Wireless_Config.c | 参考AT指令配置流程 |
| 发车/停车逻辑 | LQ_PWM_Moto.c 中 `Motor_Stop()` | 绑定到命令通道 |

### 3.3 参考文件 Vofa.h/Vofa.c 分析

文件位置：`d:\qq\file\Vofa.h`、`d:\qq\file\Vofa.c`

#### 核心功能提取
1. **命令接收**：串口中断逐字节解析 `#P<x>=<value>!` 格式（自定义帧格式，非 FireWater 规范）
2. **通道枚举**：`Vofa_PID_Channel` 定义了 7 个参数通道（Kp/Ki/Kd/Speed/Speed2/W/Enable）
3. **状态机解析**：`#` → `P` → 数字 → `=` → 数值 → `!`
4. **数据发送**：通过 `printf` 重定向输出 FireWater 格式 `Pid_Pos:%f,%f,%f\n`

#### 移植要点
- 原平台为 51/STM32 风格（`uart_tim_init`、`uart_query_byte`），需改用 TC387 的 `UART_InitConfig`/`UART_GetChar`
- 通道枚举需扩展（原 7 个 → 本项目 13 个）
- 建议增加帧校验和超时机制，提高无线可靠性
- **可选**：保留 `#Px=value!` 格式（方案B），或改为 `控件名:数值\n` 格式（方案A，推荐）

### 3.4 现有 PID 参数（config.h）

| PID环 | Kp | Ki | Kd | 说明 |
|-------|----|----|----|------|
| 方向环 | 1.5 | 0.0 | 0.3 | 输入赛道偏差px，输出Target_Gyro |
| 角度环 | 1.8 | 0.0 | 1.2 | 输入偏航角误差°，输出Target_Gyro |
| 角速度环 | 2.2 | 0.0 | 2.8 | 输入角速度误差°/s，输出差速分配 |
| 速度环 | 32.0 | 0.10 | 0.0 | 输入速度误差cm/s，输出PWM |

---

## 四、阶段三：系统设计

### 4.1 无线串口通信模块设计

#### 通信参数
| 参数 | 取值 | 说明 |
|------|------|------|
| 波特率 | **115200** | 与现有工程一致，平衡速度与稳定性 |
| 数据位 | 8位 | 标准配置 |
| 停止位 | 1位 | 标准配置 |
| 校验位 | 无 | 简化设计 |
| 流控 | 无 | 简化设计 |

#### 串口选择方案
| 方案 | 串口 | 引脚 | 说明 |
|------|------|------|------|
| 方案A（推荐） | UART3 | P00_1 RX / P00_0 TX | 与printf共用，无线输出调试信息 |
| 方案B | UART0 | P14_1 RX / P14_0 TX | 独立串口，不占用下载口 |
| 方案C | UART2 | P14_3 RX / P14_2 TX | 独立串口 |

**推荐方案A**：复用 UART3，printf 也通过无线输出，减少串口占用。

#### 无线模块选型对比
| 方案 | 模块 | 距离 | 优点 | 缺点 |
|------|------|------|------|------|
| 蓝牙 | HC-05/HC-06 | ~10m | 简单、手机也能连 | 距离近 |
| WiFi透传 | ESP8266 | ~50m | 距离远、可联网 | 配置稍复杂 |
| 2.4G数传 | NRF24L01 | ~100m | 速度快、延迟低 | 需额外驱动代码 |
| 工程已有图传 | 龙邱WiFi图传 | ~30m | 数图同传、无需额外硬件 | 占用图传带宽 |

**建议**：优先使用工程已有的无线图传模块 UART 通道（见 LQ_Wireless_Config.c），实现数图同传。

### 4.2 上行数据协议设计（FireWater，单片机→上位机）

#### 通道设计

| 通道 | 变量名 | 单位 | 数据源 |
|------|--------|------|--------|
| ch0 | Target_Speed_L | cm/s | `Get_Target_SpeedL()` |
| ch1 | Actual_Speed_L | cm/s | `Get_Current_SpeedL()` |
| ch2 | Target_Speed_R | cm/s | `Get_Target_SpeedR()` |
| ch3 | Actual_Speed_R | cm/s | `Get_Current_SpeedR()` |
| ch4 | PWM_L | - | `Get_pwmL()` |
| ch5 | PWM_R | - | `Get_pwmR()` |
| ch6 | Target_Gyro | °/s | motorState.Target_Gyro |
| ch7 | Actual_Gyro | °/s | `Get_Current_Gyro()` |
| ch8 | Target_Yaw | ° | `Get_Target_Yaw()` |
| ch9 | Actual_Yaw | ° | `Get_Current_Yaw()` |
| ch10 | Dir_Error | px | 赛道偏差（Track输出） |
| ch11 | Car_State | - | 0=停车, 1=运行 |

#### 数据帧格式（严格遵循 FireWater 规范）
```
STATE:<ch0>,<ch1>,<ch2>,<ch3>,<ch4>,<ch5>,<ch6>,<ch7>,<ch8>,<ch9>,<ch10>,<ch11>\n
```

示例输出：
```
STATE:100.0,98.5,100.0,99.2,1234,1210,0.0,-1.2,90.0,89.5,0.5,1\n
```

**注意**：
- 末尾必须是真正的换行符 `\n`（0x0A），不能用 `\r\n`（某些 MCU 串口驱动会吞 `\r`）
- 前缀 `STATE:` 可省略，但保留前缀便于文本调试时区分数据类型

#### 发送频率
- **50Hz ~ 100Hz**：在 CCU6 1ms 中断中每 10~20 帧发送一次
- 建议：10ms 发送一次（100Hz），数据量约 100字节 × 100 = 10KB/s，远低于 115200 波特率带宽（约 11.5KB/s 有效载荷）

### 4.3 下行命令协议设计（上位机→单片机）

**采用方案 A：FireWater 风格 `控件名:数值\n`**

#### 命令通道定义

| 命令名（控件名） | 类型 | 取值范围 | 绑定变量/函数 | VOFA+命令内容 |
|-----------------|------|---------|--------------|--------------|
| `speed_kp` | float | 0 ~ 100 | incpidL.kp / incpidR.kp | `speed_kp:%f\n` |
| `speed_ki` | float | 0 ~ 10 | incpidL.ki / incpidR.ki | `speed_ki:%f\n` |
| `speed_kd` | float | 0 ~ 10 | incpidL.kd / incpidR.kd | `speed_kd:%f\n` |
| `gyro_kp` | float | 0 ~ 10 | gyropid.kp | `gyro_kp:%f\n` |
| `gyro_ki` | float | 0 ~ 10 | gyropid.ki | `gyro_ki:%f\n` |
| `gyro_kd` | float | 0 ~ 10 | gyropid.kd | `gyro_kd:%f\n` |
| `dir_kp` | float | 0 ~ 10 | locpid.kp | `dir_kp:%f\n` |
| `dir_ki` | float | 0 ~ 5 | locpid.ki | `dir_ki:%f\n` |
| `dir_kd` | float | 0 ~ 5 | locpid.kd | `dir_kd:%f\n` |
| `base_speed` | float | 0 ~ 300 cm/s | `Set_GyroBaseSpeed()` | `base_speed:%f\n` |
| `car_start` | int | 1 | 发车（按钮） | `car_start:1\n` |
| `car_stop` | int | 1 | 停车（按钮） | `car_stop:1\n` |
| `angle_mode` | int | 0 / 1 | 切换方向环/角度环 | `angle_mode:%f\n` |
| `target_yaw` | float | -180 ~ 180° | `Set_Target_Yaw()` | `target_yaw:%f\n` |

#### 下位机解析逻辑（方案A）

```c
/* 收到一帧后（以 \n 结尾）的解析 */
void Vofa_ParseLine(char *line)
{
    char *colon = strchr(line, ':');
    if (colon == NULL) return;

    *colon = '\0';              /* 把冒号变成结束符，分割出命令名 */
    char *name = line;
    char *value_str = colon + 1;
    float value = atof(value_str);

    if      (strcmp(name, "speed_kp")    == 0) { incpidL.kp = incpidR.kp = constrain_float(value, 0, 100); }
    else if (strcmp(name, "speed_ki")    == 0) { incpidL.ki = incpidR.ki = constrain_float(value, 0, 10); incpidL.integrator = incpidR.integrator = 0; }
    else if (strcmp(name, "speed_kd")    == 0) { incpidL.kd = incpidR.kd = constrain_float(value, 0, 10); }
    else if (strcmp(name, "gyro_kp")     == 0) { gyropid.kp = constrain_float(value, 0, 10); }
    else if (strcmp(name, "gyro_ki")     == 0) { gyropid.ki = constrain_float(value, 0, 10); gyropid.integrator = 0; }
    else if (strcmp(name, "gyro_kd")     == 0) { gyropid.kd = constrain_float(value, 0, 10); }
    else if (strcmp(name, "dir_kp")      == 0) { locpid.kp = constrain_float(value, 0, 10); }
    else if (strcmp(name, "dir_ki")      == 0) { locpid.ki = constrain_float(value, 0, 5); locpid.integrator = 0; }
    else if (strcmp(name, "dir_kd")      == 0) { locpid.kd = constrain_float(value, 0, 5); }
    else if (strcmp(name, "base_speed")  == 0) { Set_GyroBaseSpeed(constrain_float(value, 0, MAX_SPEED)); }
    else if (strcmp(name, "car_start")   == 0) { g_stop_active = 0; incpidL.integrator = incpidR.integrator = 0; }
    else if (strcmp(name, "car_stop")    == 0) { Motor_Stop(); }
    else if (strcmp(name, "angle_mode")  == 0) { motorState.Angle_Mode = (value >= 0.5f) ? 1 : 0; }
    else if (strcmp(name, "target_yaw")  == 0) { Set_Target_Yaw(constrain_float(value, -180, 180)); }
}
```

#### 备选方案 B（自定义帧头帧尾，兼容 Vofa.c）

若用户希望沿用 `d:\qq\file\Vofa.c` 的 `#Px=value!` 格式，下位机保留状态机解析：

| 通道号 | 名称 | VOFA+命令内容 |
|--------|------|--------------|
| P1 | speed_kp | `#P1=%f!` |
| P2 | speed_ki | `#P2=%f!` |
| ... | ... | ... |
| P11 | car_start | `#P11=1!` |
| P12 | car_stop | `#P12=1!` |

**决策点**：方案 A 与方案 B 二选一，需用户确认。默认推荐方案 A。

### 4.4 安全机制设计

| 安全措施 | 实现方式 | 参数 |
|---------|---------|------|
| 通信超时保护 | 500ms 未收到任何命令 → 自动停车 | 超时阈值：500ms |
| 参数限幅 | 所有写入参数经 `constrain_float()` 限幅 | 见命令通道取值范围表 |
| 停车最高优先级 | `car_stop` 命令立即执行，忽略其他命令 | - |
| 帧校验 | 检查冒号 + 换行符，不完整帧丢弃 | - |
| 参数增量保护 | Kp 单次变化不超过当前值 50%，防止震荡 | 变化率 ≤ 50% |
| 积分器清零 | 修改 Ki / 切换模式时，清零积分器防止突变 | - |
| 发车前置条件 | 发车前若速度为 0，警告提示（可选） | - |

---

## 五、阶段四：软件开发实现

### 5.1 新增文件结构

```
LQ_TC387_Software_Library/Src/APP/
├── inc/
│   └── LQ_Vofa.h          VOFA模块头文件（新建）
└── LQ_Vofa.c              VOFA模块实现（新建）
```

### 5.2 LQ_Vofa.h 设计（头文件）

```c
#ifndef __LQ_VOFA_H__
#define __LQ_VOFA_H__

#include "lq_include.h"

/* ========= VOFA 串口号配置 ========= */
#define VOFA_UART_PORT       UART3     /* 与 printf 共用串口 */
#define VOFA_BAUDRATE        115200
#define VOFA_RX_BUF_SIZE     128       /* 接收缓冲区大小 */

/* ========= 通信超时配置 ========= */
#define VOFA_TIMEOUT_MS      500       /* 500ms 无通信自动停车 */

/* ========= 命令格式选择 ========= */
/* 1 = 方案A: FireWater风格 "name:value\n" （推荐）*/
/* 0 = 方案B: 自定义帧头帧尾 "#Px=value!" （兼容Vofa.c）*/
#define VOFA_CMD_FORMAT_NAMEVAL  1

/* ========= 车辆状态枚举 ========= */
typedef enum {
    VOFA_STATE_STOPPED = 0,
    VOFA_STATE_RUNNING = 1
} Vofa_CarState_t;

/* ========= 外部变量 ========= */
extern Vofa_CarState_t g_vofa_car_state;

/* ========= 函数接口 ========= */
void Vofa_Init(void);                  /* 初始化 */
void Vofa_RX_Handler(uint8_t dat);     /* 串口中断回调（逐字节） */
void Vofa_ProcessCommand(void);        /* 主循环中处理已接收命令 */
void Vofa_SendState(void);             /* 发送车辆状态（FireWater格式） */
void Vofa_CheckTimeout(void);          /* 超时检测（1s定时器调用） */

#endif /* __LQ_VOFA_H__ */
```

### 5.3 LQ_Vofa.c 核心实现

#### 5.3.1 接收缓冲与逐字节处理

```c
/* 接收缓冲区：遇到 \n 认为一帧结束 */
static char     s_rx_buf[VOFA_RX_BUF_SIZE];
static uint16_t s_rx_idx = 0;
static uint32_t s_last_rx_tick = 0;
Vofa_CarState_t g_vofa_car_state = VOFA_STATE_STOPPED;

void Vofa_RX_Handler(uint8_t dat)
{
    s_last_rx_tick = GetSysTick();   /* 刷新最后接收时间 */

    if (dat == '\n' || dat == '\r') {
        if (s_rx_idx > 0) {
            s_rx_buf[s_rx_idx] = '\0';   /* 截断成字符串 */
            Vofa_ParseLine(s_rx_buf);    /* 立即解析执行 */
            s_rx_idx = 0;
        }
        return;
    }
    if (s_rx_idx < VOFA_RX_BUF_SIZE - 1) {
        s_rx_buf[s_rx_idx++] = dat;
    } else {
        s_rx_idx = 0;  /* 缓冲区溢出，丢弃重来 */
    }
}
```

#### 5.3.2 命令解析（方案 A：name:value）

```c
void Vofa_ParseLine(char *line)
{
    char *colon = strchr(line, ':');
    if (colon == NULL) return;

    *colon = '\0';
    char *name = line;
    float  value = atof(colon + 1);

    if      (strcmp(name, "speed_kp")   == 0) { incpidL.kp = incpidR.kp = constrain_float(value, 0, 100); }
    else if (strcmp(name, "speed_ki")   == 0) { incpidL.ki = incpidR.ki = constrain_float(value, 0, 10);
                                                incpidL.integrator = incpidR.integrator = 0; }
    /* ... 其他通道见 4.3 节 ... */
    else if (strcmp(name, "car_start")  == 0) { g_stop_active = 0;
                                                incpidL.integrator = incpidR.integrator = 0;
                                                g_vofa_car_state = VOFA_STATE_RUNNING; }
    else if (strcmp(name, "car_stop")   == 0) { Motor_Stop();
                                                g_vofa_car_state = VOFA_STATE_STOPPED; }
    else if (strcmp(name, "base_speed") == 0) { Set_GyroBaseSpeed(constrain_float(value, 0, MAX_SPEED)); }
    else if (strcmp(name, "target_yaw") == 0) { Set_Target_Yaw(constrain_float(value, -180, 180)); }
}
```

#### 5.3.3 上行数据发送（FireWater 格式）

```c
void Vofa_SendState(void)
{
    char buf[160];
    int len = sprintf(buf, "STATE:%.1f,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n",
        Get_Target_SpeedL(), Get_Current_SpeedL(),
        Get_Target_SpeedR(), Get_Current_SpeedR(),
        (int)Get_pwmL(), (int)Get_pwmR(),
        motorState.Target_Gyro, Get_Current_Gyro(),
        Get_Target_Yaw(), Get_Current_Yaw(),
        Track_GetError(),
        (g_stop_active ? 0 : 1)
    );
    UART_PutBuff(VOFA_UART_PORT, (unsigned char*)buf, len);
}
```

**注意**：末尾 `\n` 是 FireWater 协议硬性要求，不可省略。

#### 5.3.4 超时检测

```c
void Vofa_CheckTimeout(void)
{
    if (g_vofa_car_state == VOFA_STATE_RUNNING) {
        if (GetSysTick() - s_last_rx_tick > VOFA_TIMEOUT_MS) {
            Motor_Stop();   /* 通信超时，自动停车 */
            g_vofa_car_state = VOFA_STATE_STOPPED;
        }
    }
}
```

### 5.4 与现有代码的集成点

#### 5.4.1 Cpu0_Main.c 修改

在 `core0_main()` 中添加：
```c
/* 初始化阶段（UART初始化之后） */
Vofa_Init();

/* 主循环 while(1) 中 */
Vofa_ProcessCommand();

/* 1s 定时器回调（Flag_1s 处理位置） */
Vofa_CheckTimeout();
```

#### 5.4.2 中断集成（irq.c / LQ_UART.c）

在对应 UART 的接收中断中调用：
```c
void UART3_RX_IRQHandler(void)
{
    uint8_t dat = UART_GetChar(UART3);
    Vofa_RX_Handler(dat);
}
```

#### 5.4.3 config.h 添加编译开关

```c
/*========== VOFA 上位机控制 ==========*/
#define VOFA_CONTROL_ENABLE    1    /* [0/1] VOFA 无线控制总开关 */
```

#### 5.4.4 lq_include.h 添加头文件

```c
#include "LQ_Vofa.h"
```

### 5.5 VOFA+ 上位机界面与命令绑定

#### 界面布局

```
┌─────────────────────────────────────────────────────────────────┐
│  [连接状态] [车辆状态]                    保存  重置  加载默认    │ ← 顶部状态栏
├──────────────┬──────────────────────────────────┬───────────────┤
│  控制面板     │         波形显示区域              │   快捷按钮     │
│ ──────────  │  ┌───────────────────────────┐   │ ┌───────────┐ │
│ speed_kp [■] │  │ 速度响应 (ch0-ch3)        │   │ │  发车     │ │
│ speed_ki [■] │  │ PWM输出  (ch4-ch5)        │   │ ├───────────┤ │
│ speed_kd [■] │  │ 角速度   (ch6-ch7)        │   │ │  停车     │ │
│ gyro_kp  [■] │  │ 偏航角   (ch8-ch9)        │   │ ├───────────┤ │
│ gyro_ki  [■] │  │ 方向误差 (ch10)           │   │ │ 保存参数  │ │
│ gyro_kd  [■] │  └───────────────────────────┘   │ └───────────┘ │
│ dir_kp   [■] │                                  │               │
│ dir_ki   [■] │                                  │   数值显示     │
│ dir_kd   [■] │                                  │ ┌───────────┐ │
│ base_speed[■]│                                  │ │L速度: 98.5│ │
│ target_yaw[输入]│                                │ │R速度: 99.2│ │
│              │                                  │ │状态: 运行 │ │
│              │                                  │ └───────────┘ │
└──────────────┴──────────────────────────────────┴───────────────┘
```

#### 命令绑定配置步骤（VOFA+ 操作）

1. **添加控件**：从左侧控件库拖入滑块/按钮
2. **命名控件**：右键 → 名称设置 → 改为 `speed_kp`、`car_start` 等（与下位机解析的 name 一致）
3. **添加命令**：左侧第三个绿色图标 → 添加新命令
4. **编辑命令内容**：
   - 滑块命令内容填 `speed_kp:%f\n`（`%f` 自动替换为滑块值）
   - 按钮命令内容填 `car_start:1\n`（固定值）
5. **绑定命令到控件**：右键控件 → 绑定命令 → 选择对应命令
6. **设置滑块范围**：右键滑块 → 设置 from/to/step
7. **保存配置**：文件 → 保存工程

#### 命令绑定参考表

| 控件 | 控件名 | 命令内容 | 范围/步长 |
|------|--------|---------|----------|
| 滑块 | speed_kp | `speed_kp:%f\n` | 0~50，步长0.1 |
| 滑块 | speed_ki | `speed_ki:%f\n` | 0~2，步长0.01 |
| 滑块 | speed_kd | `speed_kd:%f\n` | 0~5，步长0.1 |
| 滑块 | gyro_kp | `gyro_kp:%f\n` | 0~5，步长0.1 |
| 滑块 | gyro_ki | `gyro_ki:%f\n` | 0~2，步长0.01 |
| 滑块 | gyro_kd | `gyro_kd:%f\n` | 0~5，步长0.1 |
| 滑块 | dir_kp | `dir_kp:%f\n` | 0~5，步长0.1 |
| 滑块 | dir_ki | `dir_ki:%f\n` | 0~1，步长0.01 |
| 滑块 | dir_kd | `dir_kd:%f\n` | 0~3，步长0.1 |
| 滑块 | base_speed | `base_speed:%f\n` | 0~250，步长1.0 |
| 按钮 | car_start | `car_start:1\n` | - |
| 按钮 | car_stop | `car_stop:1\n` | - |
| 输入框 | target_yaw | `target_yaw:%f\n` | -180~180 |

---

## 六、阶段五：测试与调试

### 6.1 模块级测试清单

| 编号 | 测试项 | 测试方法 | 预期结果 |
|------|--------|---------|---------|
| T1 | UART 回环 | TX 短接 RX，自检收发 | 接收数据=发送数据 |
| T2 | FireWater 上行 | 调用 Vofa_SendState() | 上位机波形正常显示（注意 \n） |
| T3 | 命令解析(方案A) | 串口助手发 `speed_kp:1.5\n` | incpidL.kp=1.5 |
| T4 | 负数支持 | 发 `target_yaw:-90\n` | Target_Yaw=-90 |
| T5 | 不完整帧 | 发 `speed_kp:1.5`（缺\n） | 参数不被修改 |
| T6 | 限幅测试 | 发 `speed_kp:999\n` | Kp 被限制到 100 |
| T7 | 超时保护 | 断开上位机连接 | 500ms 后自动停车 |
| T8 | 发车按钮 | 点击"发车" | g_stop_active=0，电机运转 |
| T9 | 停车按钮 | 点击"停车" | g_stop_active=1，电机停止 |
| T10 | 缓冲区溢出 | 发送超长字符串（>128字节） | 不死机，自动重置 |

### 6.2 系统集成测试清单

| 编号 | 测试项 | 条件 | 验收标准 |
|------|--------|------|---------|
| S1 | 无线通信距离 | 空旷环境 | ≥10米通信无丢包 |
| S2 | 刷新率 | 连续监测 | 波形刷新率 ≥ 50Hz |
| S3 | 调参实时性 | 调 Kp 阶跃 | 响应延迟 < 100ms |
| S4 | 稳定性 | 连续运行 30min | 无死机、无异常停车 |
| S5 | 并发 | 同时发送参数+按钮 | 命令不丢失、顺序正确 |
| S6 | 数图同传 | 图传+VOFA同时工作 | 两者均不卡顿 |

### 6.3 实车功能测试

#### PID 参数调节测试流程
1. 小车静止，VOFA 显示速度环 4 路波形
2. 基础速度设置为 100cm/s，点击发车
3. 逐步增大 speed_kp，观察速度响应波形
4. 调整 speed_ki 消除稳态误差
5. 记录最优参数并更新 config.h

#### 发车/停车测试流程
1. 连接上位机，确认状态为"已连接"
2. 基础速度 = 100cm/s
3. 点击"发车" → 车辆平稳加速
4. 运行中点击"停车" → 车辆平稳停止
5. 拔掉 USB 线 → 500ms 内自动停车（超时保护）

---

## 七、阶段六：文档编写

### 7.1 系统实现文档（开发人员用）

1. 系统架构与模块框图
2. 硬件接线说明（UART引脚、无线模块连接）
3. 通信协议规范：
   - **上行**：FireWater 格式 `STATE:ch0,ch1,...,ch11\n`
   - **下行**：name:value 格式 `speed_kp:1.5\n`
4. 软件设计说明（接收状态机、命令解析、数据流）
5. 关键代码片段注释
6. PID 调参指南（步骤、经验值、常见问题）

### 7.2 用户操作手册（最终用户用）

1. VOFA+ 软件下载安装教程
2. 工程配置与固件烧录说明
3. 串口连接步骤（选择 COM 口、波特率、数据引擎选 FireWater）
4. 命令绑定操作步骤（添加控件→命名→绑定命令→设置范围）
5. 界面功能说明（各区域、各控件用途）
6. 常规操作流程：
   - 连接 → 设置速度 → 发车 → 调参 → 停车 → 保存
7. 常见问题排查：
   - 连不上串口？（检查驱动、COM口、波特率）
   - 波形无数据？（检查 FireWater 引擎、换行符 \n）
   - 参数改不动？（检查命令格式 `name:value\n`、控件名是否一致）
   - 小车无反应？（检查 car_start 是否发送、g_stop_active 状态）
8. 安全注意事项（先测试停车功能、低速起步、远离障碍物）

---

## 八、实施时间表（预估）

| 阶段 | 工作内容 | 预计工时 | 交付物 |
|------|---------|---------|--------|
| 一 | 技术调研+环境搭建 | 0.5 天 | VOFA 安装完成、串口可用 |
| 二 | 现有资源分析 | 0.5 天 | 接口清单、复用方案 |
| 三 | 系统设计 | 1 天 | 设计文档+协议规范 |
| 四 | 软件开发实现 | 2~3 天 | LQ_Vofa.h/c + 集成完成 |
| 五 | 测试调试 | 1~2 天 | 测试报告+调参记录 |
| 六 | 文档编写 | 0.5 天 | 实现文档+操作手册 |
| **合计** | | **5.5~7.5 天** | |

---

## 九、风险与应对措施

| 风险项 | 可能性 | 影响 | 应对措施 |
|--------|--------|------|---------|
| 无线丢包严重 | 中 | 参数修改延迟、命令丢失 | 1. 增加命令重发；2. 心跳包；3. 超时保护兜底 |
| VOFA 控件绑定繁琐 | 中 | 界面搭建耗时 | 先做核心按钮+滑块，其他控件后续迭代 |
| 调参导致震荡/失控 | 高 | 车辆失控 | 1. 参数增量 ≤ 50%；2. 随时可点停车；3. 先低速调参 |
| 中断优先级冲突 | 中 | CCU6 被串口抢占导致控制不稳 | 合理配置：CCU6 > UART接收 > STM |
| FireWater 格式不兼容 | 低 | 波形不显示 | 先用串口助手抓包验证格式；注意末尾 \n（0x0A） |
| 与蓝牙/WiFi 图传冲突 | 中 | 数图同传互相干扰 | 使用独立物理通道，或分时复用降频 |
| 缓冲区残留导致解析错位 | 中 | 命令匹配失败 | 每帧 \n 后清空缓冲区；用 strchr 而非硬编码偏移 |
| 控件名拼写不一致 | 中 | 命令无法匹配 | 下位机与上位机控件名严格一致，建立命名表 |

---

## 十、关键代码索引（实现时快速定位）

| 功能点 | 源文件 | 关键函数/变量 |
|--------|--------|--------------|
| UART 初始化 | LQ_UART.c | `UART_InitConfig()` |
| UART 收发 | LQ_UART.c | `UART_PutStr()` / `UART_GetCount()` / `UART_GetChar()` |
| UART 中断 | LQ_UART.c | 各 `UARTn_RX_IRQHandler` |
| 速度环 PID 参数 | LQ_PID.h / config.h | `incpidL` / `incpidR` / `IncKP` |
| 角速度环 PID | LQ_PID.h | `gyropid` |
| 方向环 PID | LQ_PID.h | `locpid` |
| 电机目标速度 | LQ_PWM_Moto.c | `motorState.Target_SpeedL/R` |
| 电机 PWM 输出 | LQ_PWM_Moto.c | `Get_pwmL()/Get_pwmR()` |
| 停车控制 | LQ_PWM_Moto.c | `Motor_Stop()` / `g_stop_active` |
| 基础速度设置 | LQ_PWM_Moto.c | `Set_GyroBaseSpeed()` |
| 目标偏航角 | LQ_PWM_Moto.c | `Set_Target_Yaw()` / `Get_Current_Yaw()` |
| 陀螺仪数据 | LQ_SPI_Gyro.c | `Get_Current_Gyro()` / `Attitude_get()` |
| 编码器速度 | LQ_Encoder.c | `Get_Current_SpeedL/R()` |
| 赛道偏差 | LQ_Track.c | `Track_Process()` 输出 |
| 主循环入口 | Cpu0_Main.c | `core0_main()` |
| 1s 定时器标志 | Cpu0_Main.c | `Flag_1s` |
| CCU6 周期中断 | irq.c | 控制环调度入口 |
| 编译配置开关 | Main/config.h | 各模块 `*_ENABLE` 宏 |

---

## 十一、待用户确认的决策点

在进入编码阶段前，需用户确认以下决策：

| 编号 | 决策项 | 选项 | 推荐 |
|------|--------|------|------|
| D1 | 下行命令格式 | A: `name:value\n`（FireWater风格） / B: `#Px=value!`（Vofa.c风格） | A |
| D2 | 串口选择 | A: UART3(与printf共用) / B: UART0(独立) / C: UART2(独立) | A |
| D3 | 无线模块 | A: 工程已有图传 / B: HC-05蓝牙 / C: ESP8266 / D: NRF24L01 | A |
| D4 | 上行发送频率 | A: 50Hz / B: 100Hz | B |
| D5 | 上行数据前缀 | A: 带前缀 `STATE:` / B: 不带前缀 | A |
| D6 | 超时停车阈值 | A: 500ms / B: 1000ms / C: 2000ms | A |

---

## 参考资料

1. VOFA+ 官方文档 - FireWater 协议：https://www.vofa.plus/docs/learning/dataengines/firewater/
2. VOFA+ 官方文档 - Slider 控件：https://www.vofa.plus/docs/learning/widgets/slider/
3. VOFA+ 官方文档 - 数据、命令、参数：https://www.vofa.plus/docs/learning/start/data_cmd_parameter
4. 参考代码：`d:\qq\file\Vofa.h`、`d:\qq\file\Vofa.c`（注：其 `#Px=value!` 为自定义格式，非 FireWater 规范）
