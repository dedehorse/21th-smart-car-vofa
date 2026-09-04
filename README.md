# TC387 Smart Car · VOFA+

基于 Infineon AURIX TC387QP 和龙邱 LQ_TC387 软件库的智能车最终版工程。集成摄像头感知、赛道识别、串级 PID 控制、电机驱动、姿态解算、无线通信和 VOFA+ 在线调参。

## 项目定位

这是 `workspace527_vofa` 的最终工程快照。通过无线串口连接 VOFA+ 后，可以实时查看车辆状态，并在线修改 PID 参数、基础速度和运行模式。

## 硬件与软件

| 类别 | 配置 |
| --- | --- |
| 主控 | Infineon AURIX TC387QP |
| 开发板 | 龙邱 LQ_TC387 核心板及 V7 母板 |
| 传感器 | MT9V034 摄像头、编码器、MPU6050/陀螺仪、ADC |
| 执行器 | 无刷电机、舵机、PWM 驱动 |
| 开发环境 | AURIX Development Studio 1.10.x |
| 编译器 | Tasking TriCore Compiler |
| 调试 | DAP MiniWiggler / winIDEA |
| 上位机 | VOFA+，FireWater 数据引擎 |

## 主要功能

- 摄像头采集、二值化、边缘和赛道中线提取
- 十字、T 字、环岛、坡道等赛道元素识别
- 方向环、角度环、角速度环和速度环控制
- 编码器速度反馈、陀螺仪姿态解算和电机差速控制
- WiFi/蓝牙无线串口通信及实时状态回传
- VOFA+ 在线修改 PID 参数、基础速度和目标偏航角
- 远程发车、停车和通信超时自动停车

## 目录结构

```text
LQ_TC387_Software_Library/
├── Main/                  多核入口、系统配置和中断
├── Src/APP/               摄像头、电机、编码器、IMU、VOFA 等应用模块
├── Src/Driver/            UART、SPI、QSPI、DMA、PWM 等底层驱动
├── Src/User/              PID、路径规划和用户算法
├── Libraries/             Infineon iLLD 与基础服务库
├── Configurations/        芯片和启动配置
└── *.md                   VOFA 方案与使用说明
```

## VOFA+ 通信

车辆向 VOFA+ 输出 `STATE:<ch0>,<ch1>,...,<ch11>\n` FireWater 数据。通道覆盖左右轮目标/实际速度、PWM、目标/实际角速度、目标/实际偏航角、赛道偏差和车辆状态。

下行命令采用 `命令名:数值\n` 格式：

```text
base_speed:100\n
speed_kp:35.0\n
car_start:1\n
car_stop:1\n+```

完整命令表、控件配置和故障排查请参阅 [VOFA使用说明.md](VOFA使用说明.md) 与 [VOFA车辆控制系统实施方案.md](VOFA车辆控制系统实施方案.md)。

## 编译与烧录

1. 使用 AURIX Development Studio 导入 `LQ_TC387_Software_Library` 工程。
2. 检查芯片、编译器、引脚映射和下载器配置。
3. 在 `Main/config.h` 中检查功能开关和 PID 初始参数。
4. 编译并使用 DAP MiniWiggler 烧录。
5. 上电后先低速测试，并确认停车保护可用。

## 安全提示

在线调参会直接影响真实车辆。首次测试应抬起驱动轮、设置低基础速度，并优先验证 `car_stop`。通信中断保护不能替代人工断电和物理安全措施。

## 相关仓库

- [smart-car-three-cascade-loop](https://github.com/dedehorse/smart-car-three-cascade-loop)：三串级控制环版本
- [smart-car-history](https://github.com/dedehorse/smart-car-history)：历史迭代归档

## 许可与致谢

本工程使用龙邱 LQ_TC387 软件库及 Infineon iLLD。上游库的许可证和版权声明以仓库内原始文件为准。
