/*******************************************************************************
 *  @file                 LQ_Vofa.c
 *  @brief                VOFA+ 上位机通信模块实现
 *
 *  上行协议：FireWater   "STATE:ch0,ch1,...,ch11\n"
 *  下行协议：name:value\n   (VOFA+ 控件命令绑定)
 *
 *  架构说明：
 *    - CCU6 1ms 中断：仅做 ms 计数 + 置发送标志（不阻塞控制环）
 *    - UART3 RX 中断：逐字节接收 + 解析命令（在中断中直接执行参数修改）
 *    - 主循环：检查发送标志，调用 Vofa_SendState 阻塞发送（不影响控制环）
 *    - STM 1s 中断：通信超时检测
 *
 *  GPL-3.0 开源协议
 *******************************************************************************/

#include "LQ_Vofa.h"

#if VOFA_CONTROL_ENABLE

/* ========= 外部 PID 变量 (LQ_PID.h) ========= */
extern pid_param_t locpid;
extern pid_param_t anglepid;
extern pid_param_t gyropid;
extern pid_param_t incpidL;
extern pid_param_t incpidR;

/* ========= 外部电机控制变量 (LQ_PWM_Moto.h) ========= */
extern uint8_t  g_stop_active;

/* ========= 模块内部变量 ========= */
static char           s_rx_buf[VOFA_RX_BUF_SIZE];
static volatile uint16_t s_rx_idx = 0;
static uint16_t       s_send_cnt = 0;         /* 发送分频计数 */
static volatile uint8_t  s_send_flag = 0;     /* 发送标志：1=需要发送 */

Vofa_CarState_t g_vofa_car_state = VOFA_STATE_STOPPED;

/* ========= 内部辅助函数声明 ========= */
static void Vofa_ParseLine(char *line);
static void Vofa_ApplyParam(const char *name, float value);
static void Vofa_SendState(void);

/*******************************************************************************
 * @brief  VOFA 模块初始化
 * @note   在 core0_main() 中、UART3 初始化之后调用
 ******************************************************************************/
void Vofa_Init(void)
{
    s_rx_idx     = 0;
    s_send_cnt   = 0;
    s_send_flag  = 0;
    g_vofa_car_state = VOFA_STATE_STOPPED;
}

/*******************************************************************************
 * @brief  UART3 中断逐字节回调
 * @param  dat  串口收到的一个字节
 * @note   在 UART3_RX_IRQHandler 中调用，遇到 \n 结帧并解析
 ******************************************************************************/
void Vofa_RX_Handler(uint8_t dat)
{
    if (dat == '\n' || dat == '\r') {
        if (s_rx_idx > 0) {
            s_rx_buf[s_rx_idx] = '\0';
            Vofa_ParseLine(s_rx_buf);
            s_rx_idx = 0;
        }
        return;
    }

    if (s_rx_idx < VOFA_RX_BUF_SIZE - 1) {
        s_rx_buf[s_rx_idx++] = (char)dat;
    } else {
        s_rx_idx = 0;  /* 缓冲区溢出，丢弃重来 */
    }
}

/*******************************************************************************
 * @brief  解析一帧 name:value 格式的命令
 * @param  line  以 \0 结尾的字符串（不含 \n）
 ******************************************************************************/
static void Vofa_ParseLine(char *line)
{
    char *colon = strchr(line, ':');
    if (colon == NULL) return;

    *colon = '\0';
    float value = (float)atof(colon + 1);
    Vofa_ApplyParam(line, value);
}

/*******************************************************************************
 * @brief  根据命令名执行参数修改
 ******************************************************************************/
static void Vofa_ApplyParam(const char *name, float value)
{
    /* ---------- 速度环 PID ---------- */
    if (strcmp(name, "speed_kp") == 0) {
        incpidL.kp = incpidR.kp = constrain_float(value, 0, 100);
    }
    else if (strcmp(name, "speed_ki") == 0) {
        incpidL.ki = incpidR.ki = constrain_float(value, 0, 10);
        incpidL.integrator = incpidR.integrator = 0;
    }
    else if (strcmp(name, "speed_kd") == 0) {
        incpidL.kd = incpidR.kd = constrain_float(value, 0, 10);
    }
    /* ---------- 角速度环 PID ---------- */
    else if (strcmp(name, "gyro_kp") == 0) {
        gyropid.kp = constrain_float(value, 0, 10);
    }
    else if (strcmp(name, "gyro_ki") == 0) {
        gyropid.ki = constrain_float(value, 0, 10);
        gyropid.integrator = 0;
    }
    else if (strcmp(name, "gyro_kd") == 0) {
        gyropid.kd = constrain_float(value, 0, 10);
    }
    /* ---------- 方向环 PID ---------- */
    else if (strcmp(name, "dir_kp") == 0) {
        locpid.kp = constrain_float(value, 0, 10);
    }
    else if (strcmp(name, "dir_ki") == 0) {
        locpid.ki = constrain_float(value, 0, 5);
        locpid.integrator = 0;
    }
    else if (strcmp(name, "dir_kd") == 0) {
        locpid.kd = constrain_float(value, 0, 5);
    }
    /* ---------- 角度环 PID ---------- */
    else if (strcmp(name, "angle_kp") == 0) {
        anglepid.kp = constrain_float(value, 0, 10);
    }
    else if (strcmp(name, "angle_ki") == 0) {
        anglepid.ki = constrain_float(value, 0, 10);
        anglepid.integrator = 0;
    }
    else if (strcmp(name, "angle_kd") == 0) {
        anglepid.kd = constrain_float(value, 0, 10);
    }
    /* ---------- 车速 / 角度 ---------- */
    else if (strcmp(name, "base_speed") == 0) {
        Set_GyroBaseSpeed(constrain_float(value, 0, MAX_SPEED));
    }
    else if (strcmp(name, "target_yaw") == 0) {
        Set_Target_Yaw(constrain_float(value, -180, 180));
    }
    else if (strcmp(name, "angle_mode") == 0) {
        Set_Angle_Mode((uint8_t)(value >= 0.5f ? 1 : 0));
    }
    /* ---------- 发车 / 停车 ---------- */
    else if (strcmp(name, "car_start") == 0) {
        g_stop_active = 0;
        incpidL.integrator = 0;
        incpidR.integrator = 0;
        g_vofa_car_state = VOFA_STATE_RUNNING;
    }
    else if (strcmp(name, "car_stop") == 0) {
        Motor_Stop();
        g_vofa_car_state = VOFA_STATE_STOPPED;
    }
}

/*******************************************************************************
 * @brief  CCU6 1ms 中断中调用：ms 计数 + 分频置发送标志
 * @note   不在此处发送数据（阻塞式UART发送约需8.7ms@115200，会阻塞控制环）
 ******************************************************************************/
void Vofa_CCU6_Tick(void)
{
    s_send_cnt++;

    if (s_send_cnt >= VOFA_SEND_INTERVAL) {
        s_send_cnt = 0;
        s_send_flag = 1;  /* 仅置标志，主循环中发送 */
    }
}

/*******************************************************************************
 * @brief  主循环中调用：检查发送标志，执行实际数据发送
 * @note   在 Cpu0_Main.c 的 while(1) 中调用
 ******************************************************************************/
void Vofa_MainLoop(void)
{
    if (s_send_flag) {
        s_send_flag = 0;
        Vofa_SendState();
    }
}

/*******************************************************************************
 * @brief  发送车辆状态数据（FireWater 格式）
 * @note   12 通道: ch0~ch11
 *   ch0  Target_Speed_L    [cm/s]
 *   ch1  Actual_Speed_L    [cm/s]
 *   ch2  Target_Speed_R    [cm/s]
 *   ch3  Actual_Speed_R    [cm/s]
 *   ch4  PWM_L             [bit]
 *   ch5  PWM_R             [bit]
 *   ch6  Target_Gyro       [°/s]
 *   ch7  Actual_Gyro       [°/s]
 *   ch8  Target_Yaw         [°]
 *   ch9  Actual_Yaw         [°]
 *   ch10 Track_Diff         [px]
 *   ch11 Car_State          [0/1]
 ******************************************************************************/
static void Vofa_SendState(void)
{
    char buf[180];
    int len = sprintf(buf,
        "STATE:%.1f,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f,%.1f,%.1f,%d\n",
        Get_Target_SpeedL(),  Get_Current_SpeedL(),
        Get_Target_SpeedR(),  Get_Current_SpeedR(),
        (int)Get_pwmL(),      (int)Get_pwmR(),
        Get_Target_Gyro(),    Get_Current_Gyro(),
        Get_Target_Yaw(),     Get_Current_Yaw(),
        Track_GetDiff(),
        (g_stop_active ? 0 : 1)
    );
    UART_PutBuff(VOFA_UART_PORT, (unsigned char *)buf, (unsigned long)len);
}

#endif /* VOFA_CONTROL_ENABLE */
