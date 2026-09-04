/*******************************************************************************
 *  @file                 LQ_Vofa.h
 *  @brief                VOFA+ 上位机通信模块 — FireWater 协议
 *
 *  功能：
 *    - 上行：FireWater 格式发送车辆状态（速度/PWM/角速度/偏航角等）
 *    - 下行：name:value\n 格式解析命令，在线修改 PID 参数 & 发车/停车
 *    - 通信超时自动停车保护
 *
 *  依赖：
 *    - UART3 (P00_1 RX / P00_0 TX)，与 printf 共用
 *    - config.h 中 VOFA_CONTROL_ENABLE = 1 时生效
 *
 *  GPL-3.0 开源协议
 *******************************************************************************/
#ifndef __LQ_VOFA_H__
#define __LQ_VOFA_H__

#include "lq_include.h"

/* ========= 串口配置 ========= */
#define VOFA_UART_PORT       UART3       /* 与 PRINTF_UART 一致 */
#define VOFA_RX_BUF_SIZE     128         /* 单帧最大长度 */

/* ========= 上行发送频率 ========= */
#define VOFA_SEND_INTERVAL   10          /* 每 N 次 CCU6 中断触发一次 (N*1ms) */
                                          /* 10 → 100Hz, 20 → 50Hz */

/* ========= 车辆状态枚举 ========= */
typedef enum {
    VOFA_STATE_STOPPED = 0,
    VOFA_STATE_RUNNING = 1
} Vofa_CarState_t;

/* ========= 外部变量 ========= */
extern Vofa_CarState_t g_vofa_car_state;

/* ========= 函数接口 ========= */
void Vofa_Init(void);                    /* 初始化（主循环前调用一次） */
void Vofa_RX_Handler(uint8_t dat);       /* UART3 中断逐字节回调 */
void Vofa_CCU6_Tick(void);              /* CCU6 1ms 中断中调用（计数+置标志） */
void Vofa_MainLoop(void);               /* 主循环中调用（检查标志+发送数据） */

#endif /* __LQ_VOFA_H__ */
