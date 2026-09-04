/*******************************************************************************
 *  @file                 本文件是LQ_TC387_Software_Library 软件开源库文件的一部分
 *  @author               chiusir
 *  @email                chiusir@163.com
 *  @version              V2.0.0
 *  @update               2026年3月24日
 *  @copyright            版权所有 (C) 2025-2026 北京龙邱科技有限公司
 *  @website              http://www.lqist.cn
 *  @taobao               http://longqiu.taobao.com
 *
 *  @brief                龙邱科技 LQ_TC387核心板驱动库声明
 *
 * 本软件遵循GPL-3.0开源协议发布，旨在为TC387芯片嵌入式系统设计者提供快速上手、开发基于TC387应用程序的参考
 * 商业用途（包括单位使用）需提前联系 http://www.lqist.cn 获得授权
 *
 * 开发环境配置:
 *   - 开发平台 : AURIX-Studio 版本-1.10.2  (简称ADS 1.10.x)
 *   - 文件编码 : UTF-8  (AURIX-Studio-1.10.x 默认编码)
 *   - 目标芯片 : TC387QP (TriCore™ @ 300 MHz X4 10 Mbyte flash, 1568 KB of RAM)
 *   - 外置晶振 : 20MHz
 *   - 系统PLL : 300MHz + 300MHz + 300MHz + 300MHz
 *
 * GPL-3.0 许可证声明摘要:
 * 1. 允许自由使用、修改、分发本软件
 * 2. 分发修改后的版本时，必须以相同许可证发布
 * 3. 必须保留原始版权声明和许可证信息
 * 4. 不提供任何担保，使用风险自负
 * 5. 完整协议文本请参见项目根目录 LICENSE 文件
 *******************************************************************************/

#include "LQ_PWM_Moto.h"

/* 角度环无线输出开关: 1=启用, 0=禁用 (仿照DisplayTrack中TRACK_OUTPUT_WIRELESS方式) */
#define ANGLE_LOOP_WIRELESS_OUTPUT  1


/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void MotorInit(void)
@功能说明：电机PWM初始化
@参数说明：无
@函数返回：无
@调用方法：MotorInit(100);
@备    注：驱动4个电机
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void MotorInit(void)
{
    ATOM_PWM_InitConfig(MOTOR1_P, 0, MOTOR_FREQUENCY);
#if !BLS_LAUNCH_ENABLE
    /* MOTOR2_P与BLS3共用P21_2; 启用负压风扇时不得配置为10kHz电机PWM。 */
    ATOM_PWM_InitConfig(MOTOR2_P, 0, MOTOR_FREQUENCY);
#endif
    ATOM_PWM_InitConfig(MOTOR3_P, 0, MOTOR_FREQUENCY);
    ATOM_PWM_InitConfig(MOTOR4_P, 0, MOTOR_FREQUENCY);

    PIN_InitConfig(MOTOR1_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR2_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR3_I, PIN_MODE_OUTPUT, 0);
    PIN_InitConfig(MOTOR4_I, PIN_MODE_OUTPUT, 0);
}

#ifdef USE7843or7971
void MotorCtrL(Motor_e Motor, sint32 motor)
{
    switch (Motor)
    {
    case Motor1:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR1_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR1_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 1);
        }
        break;
    case Motor2:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR2_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR2_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 1);
        }
        break;
    case Motor3:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR3_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR3_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 1);
        }
        break;
    case Motor4:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR4_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR4_P, (ATOM_PWM_MAX + motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 1);
        }
        break;
    }
}

#else // USEDRV8701

void MotorCtrL(Motor_e Motor, sint32 motor)
{
    switch (Motor)
    {
    case Motor1:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR1_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR1_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR1_I, 1);
        }
        break;
    case Motor2:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR2_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR2_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR2_I, 1);
        }
        break;
    case Motor3:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR3_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR3_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR3_I, 1);
        }
        break;
    case Motor4:
        if (motor > 0)
        {
            ATOM_PWM_SetDuty(MOTOR4_P, motor, MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 0);
        }
        else
        {
            ATOM_PWM_SetDuty(MOTOR4_P, (0 - motor), MOTOR_FREQUENCY);
            PIN_Write(MOTOR4_I, 1);
        }
        break;
    }
}
#endif
void Test_Motor(void)
{
    short duty_L = 1500;
    short duty_R = 1500;
    char txt[32];
    GPIO_LED_Init();
    GPIO_KEY_Init();
    MotorInit();
    Display_Init(0);
    Display_CLS(U16_BLACK);
    Display_CLS(U16_BLACK);
    Display_showString(0, 0, "   LQ Motor Test      ", U16_WHITE, U16_BLACK, 16);

    MotorCtrL(Motor4, -duty_L);  /* Motor4 = 左轮, 负 duty 前进 */
    MotorCtrL(Motor3, -duty_R);  /* Motor3 = 右轮, 负 duty 前进 */

    while (1)
    {
        sprintf(txt, "Left : %05d", duty_L);
        Display_showString(0, 3, txt, U16_WHITE, U16_BLACK, 16);

        sprintf(txt, "Right: %05d", duty_R);
        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16);

        printf("Left: %05d  Right: %05d\r\n", duty_L, duty_R);

        LED_Ctrl(LED0, RVS);
        Delay_Ms(100);
    }
}

uint8_t g_test_active = 0;
uint8_t g_stop_active = 0;

void Motor_Stop(void)
{
    if (!g_stop_active) {
        g_stop_active = 1;
        incpidL.integrator = 0;
        incpidR.integrator = 0;
    }
}

static MotorState motorState = {0};

void MotorState_Reset(void)
{
    motorState.Target_Gyro   = 0.0f;
    motorState.Target_SpeedL = 0.0f;
    motorState.Target_SpeedR = 0.0f;
    motorState.Target_Yaw    = 0.0f;
    motorState.Angle_Mode    = 0;
    motorState.pwmL          = 0;
    motorState.pwmR          = 0;
}

// 标准串级PID控制：方向环输出作为速度环设定值
void DirectionLoop_Update(float direction_ref) {
    static uint8_t straight_cnt = 0;  /* 连续直行计数, 用于确认转弯结束 */
    float base_speed = SPEED;

    /* 停车模式: 目标速度归零, 由速度环自然减速直至停止 */
    if (g_stop_active) {
        motorState.Target_SpeedL = 0;
        motorState.Target_SpeedR = 0;
        return;
    }

    float direction_out = PidLocCtrl(&locpid, -direction_ref);

    /* 连续接近直行时清零速度环积分, 防止转弯期间累积的积分导致出弯后电机无力啸叫 */
    if (direction_out > -15.0f && direction_out < 15.0f) {
        straight_cnt++;
        if (straight_cnt >= 3) {  /* 连续3次(15ms@200Hz)确认直行 */
            incpidL.integrator = 0;
            incpidR.integrator = 0;
            straight_cnt = 0;
        }
    } else {
        straight_cnt = 0;
    }

    // 根据转向方向判断外轮和内轮，应用不同系数
    if (direction_out > 0) {
        // 右转：右轮为内轮，左轮为外轮
        motorState.Target_SpeedL = base_speed + TURN_OUTER_RATIO * direction_out;
        motorState.Target_SpeedR = base_speed - TURN_INNER_RATIO * direction_out;
    } else {
        // 左转或直行：左轮为内轮，右轮为外轮
        motorState.Target_SpeedL = base_speed + TURN_INNER_RATIO * direction_out;
        motorState.Target_SpeedR = base_speed - TURN_OUTER_RATIO * direction_out;
    }

    motorState.Target_SpeedL = constrain_float(motorState.Target_SpeedL, 0, MAX_SPEED);
    motorState.Target_SpeedR = constrain_float(motorState.Target_SpeedR, 0, MAX_SPEED);
}

void SpeedLoop_Update(float speedL_feedback, float speedR_feedback) {
    float errorL = motorState.Target_SpeedL - speedL_feedback;
    float errorR = motorState.Target_SpeedR - speedR_feedback;

    /* 条件积分: 大误差时冻结积分器, 只在稳态小误差时才累加 */
    float prev_intL = incpidL.integrator;
    float prev_intR = incpidR.integrator;

    motorState.pwmL = (short)PidLocCtrl(&incpidL, errorL);
    motorState.pwmR = (short)PidLocCtrl(&incpidR, errorR);

    if (errorL > 60.0f || errorL < -60.0f) {
        incpidL.integrator = prev_intL;  /* 大误差→回退积分, 防止转弯/刹车时深度饱和 */
    }
    if (errorR > 60.0f || errorR < -60.0f) {
        incpidR.integrator = prev_intR;
    }

    /* 输出限幅 + 抗积分饱和: 输出碰边界时回退本次积分 */
    short pwmL_clamped = constrain_short(motorState.pwmL, -MAX_PWM, MAX_PWM);
    short pwmR_clamped = constrain_short(motorState.pwmR, -MAX_PWM, MAX_PWM);

    if (motorState.pwmL != pwmL_clamped) {
        incpidL.integrator = prev_intL;
    }
    if (motorState.pwmR != pwmR_clamped) {
        incpidR.integrator = prev_intR;
    }

    /* 最低PWM保护: 目标速度>30cm/s且实际远未达标时, PWM过低则强制抬升 */
    /* 新电机静摩擦大, 低于900(9%占空比)转不动, 会产生啸叫 */
    if (motorState.Target_SpeedL > 30.0f && errorL > 20.0f
        && pwmL_clamped > 0 && pwmL_clamped < 900) {
        pwmL_clamped = 900;
    }
    if (motorState.Target_SpeedR > 30.0f && errorR > 20.0f
        && pwmR_clamped > 0 && pwmR_clamped < 900) {
        pwmR_clamped = 900;
    }

    motorState.pwmL = pwmL_clamped;
    motorState.pwmR = pwmR_clamped;
    MotorCtrL(Motor4, -motorState.pwmL);  /* speedL / ENC1 对应左轮 */
    MotorCtrL(Motor3, -motorState.pwmR);  /* speedR / ENC4 对应右轮 */
}

static float gyro_base_speed = SPEED;

void Set_GyroBaseSpeed(float speed)
{
    gyro_base_speed = speed;
}

float Get_GyroBaseSpeed(void)
{
    return gyro_base_speed;
}

void Gyro_Loop_Update(float gyro_feedback)
{
    float gyro_error = motorState.Target_Gyro - gyro_feedback;
    float gyro_out = PidLocCtrl(&gyropid, gyro_error);

    if (gyro_out > 0) {
        motorState.Target_SpeedL = gyro_base_speed + TURN_OUTER_RATIO * gyro_out;
        motorState.Target_SpeedR = gyro_base_speed - TURN_INNER_RATIO * gyro_out;
    } else {
        motorState.Target_SpeedL = gyro_base_speed + TURN_INNER_RATIO * gyro_out;
        motorState.Target_SpeedR = gyro_base_speed - TURN_OUTER_RATIO * gyro_out;
    }

    motorState.Target_SpeedL = constrain_float(motorState.Target_SpeedL, 0, MAX_SPEED);
    motorState.Target_SpeedR = constrain_float(motorState.Target_SpeedR, 0, MAX_SPEED);
}

void AngleLoop_Update(float yaw_feedback)
{
    float yaw_error = motorState.Target_Yaw - yaw_feedback;

    while (yaw_error > 180.0f)  yaw_error -= 360.0f;
    while (yaw_error < -180.0f) yaw_error += 360.0f;

    motorState.Target_Gyro = -PidLocCtrl(&anglepid, yaw_error);
    if(motorState.Target_Gyro > 80.0f)  motorState.Target_Gyro = 80.0f;
    if(motorState.Target_Gyro < -80.0f) motorState.Target_Gyro = -80.0f;

    if(yaw_error > 1.0f && motorState.Target_Gyro > -1.5f)
        motorState.Target_Gyro = -1.5f;
    else if(yaw_error < -1.0f && motorState.Target_Gyro < 1.5f)
        motorState.Target_Gyro = 1.5f;
}

void Set_Target_Yaw(float yaw)
{
    motorState.Target_Yaw = yaw;
    motorState.Angle_Mode = 1;
}

float Get_Target_Yaw(void)
{
    return motorState.Target_Yaw;
}

uint8_t Get_Angle_Mode(void)
{
    return motorState.Angle_Mode;
}

void Set_Angle_Mode(uint8_t mode)
{
    motorState.Angle_Mode = mode ? 1 : 0;
}

float Get_Target_SpeedL()
{
    return motorState.Target_SpeedL;
}

float Get_Target_SpeedR()
{
    return motorState.Target_SpeedR;
}

float Get_pwmL()
{
    return motorState.pwmL;
}

float Get_pwmR()
{
    return motorState.pwmR;
}

float Get_Target_Gyro()
{
    return motorState.Target_Gyro;
}

void Test_GyroLoop(void) //期望角速度给固定值 给速度环 观察实际角速度 给阶跃响应 越快越好 适当超调 之后要平稳 给一个正弦波（f忽大忽小) 实际角速度大致跟上波形
{
    char txt[32];
    GPIO_LED_Init();
    MotorInit();
    EncInit();
    SPI_Gryo_Init();
    PidInit();
    IMU_GetOffset(&icm_offset);
    Display_Init(0);
    Display_CLS(U16_BLACK);
    Display_CLS(U16_BLACK);
    Display_showString(0, 0, " Gyro Loop Test     ", U16_WHITE, U16_BLACK, 16);

    g_test_active = 1;
    motorState.Angle_Mode = 0;

    while (1)
    {
        float cur_gyro = Get_Current_Gyro();
        float cur_yaw = Get_Current_Yaw();
        short pwmL = motorState.pwmL;
        short pwmR = motorState.pwmR;

        motorState.Target_Gyro = 0.0f;
        motorState.Target_Yaw = cur_yaw;

        sprintf(txt, "TgtGyr: 0.0 deg/s ");
        Display_showString(0, 2, txt, U16_WHITE, U16_BLACK, 16);

        sprintf(txt, "CurGyr:%6.1f      ", cur_gyro);
        Display_showString(0, 3, txt, U16_WHITE, U16_BLACK, 16);

        sprintf(txt, "Yaw:%6.1f deg     ", cur_yaw);
        Display_showString(0, 4, txt, U16_WHITE, U16_BLACK, 16);

        sprintf(txt, "L:%05d            ", pwmL);
        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16);

        sprintf(txt, "R:%05d            ", pwmR);
        Display_showString(0, 6, txt, U16_WHITE, U16_BLACK, 16);

        LED_Ctrl(LED0, RVS);
        Delay_Ms(50);
    }
}

/* 角度环参数无线传输 — UTF-8文本格式, 串口助手可直接阅读
 * 输出示例: AL|err:-90.0|yaw:0.0|tgt:-90.0|gyr:-18.0|S:1|N:1|SL:78|SR:117|PL:1234|PR:2345\r\n */
void TR_Write_AngleLoopData(float yaw_err, float cur_yaw, float target_yaw,
                            float turn_out, uint8_t state, uint8_t step,
                            float speedL_fb, float speedR_fb)
{
#if ANGLE_LOOP_WIRELESS_OUTPUT
    char buf[128];
    int len = sprintf(buf, "AL:%.1f,%.1f,%.1f,%.1f,%.1f,%d,%.1f,%.1f\r\n",
        yaw_err, cur_yaw, target_yaw, turn_out,
        Get_Current_Gyro(), step,
        motorState.Target_SpeedL, motorState.Target_SpeedR);
    IR_Wirte_byte((unsigned char *)buf, (uint16_t)len);
#endif
}

void Test_AngleLoop(void)
{
    float yaw_err, cur_yaw, abs_err;
    short pwmL, pwmR;
    uint8_t sec_cnt = 0, step = 0;
    uint8_t turning = 0, was_turning = 0;
    uint8_t turn_sec = 0;
    extern char Flag_1s;

    g_test_active = 1;
    MotorState_Reset();
    incpidL.integrator = 0;
    incpidR.integrator = 0;
    anglepid.integrator = 0;
    anglepid.last_error = 0;
    gyropid.integrator = 0;
    gyropid.last_error = 0;
    locpid.integrator = 0;
    locpid.last_error = 0;

    GPIO_LED_Init();
    MotorInit();
    EncInit();
    SPI_Gryo_Init();
    Delay_Ms(500);
    IMU_GetOffset(&icm_offset);
    PidInit();
    Set_GyroBaseSpeed(SPEED);

    Display_Init(0);
    Display_CLS(U16_BLACK);
    Display_CLS(U16_BLACK);
    Display_showString(0, 0, " Angle Loop Test   ", U16_WHITE, U16_BLACK, 16);

    { int i; for (i = 0; i < 300; i++) { Attitude_get(); Delay_Ms(2); } }

    motorState.Target_Yaw = Get_Current_Yaw();
    motorState.Angle_Mode = 1;
    incpidL.integrator = 0;
    incpidR.integrator = 0;
    anglepid.integrator = 0;
    anglepid.last_error = 0;
    gyropid.integrator = 0;
    gyropid.last_error = 0;

    printf("=== AngleLoop Test Start ===\r\n");
    printf("Init Yaw=%.1f KP=%.2f\r\n", motorState.Target_Yaw, AngleKP);

    while (1)
    {
        cur_yaw = Get_Current_Yaw();
        yaw_err = motorState.Target_Yaw - cur_yaw;
        while (yaw_err > 180.0f)  yaw_err -= 360.0f;
        while (yaw_err < -180.0f) yaw_err += 360.0f;
        abs_err = (yaw_err >= 0) ? yaw_err : -yaw_err;

        was_turning = turning;
        if (turning)
            turning = (abs_err > 5.0f);
        else
            turning = (abs_err > 10.0f);

        if (was_turning && !turning)
        {
            sec_cnt = 0;
            turn_sec = 0;
            printf("[OK] Step%d done err=%.1f Yaw=%.1f\r\n", step, yaw_err, cur_yaw);
        }

        if (Flag_1s)
        {
            Flag_1s = 0;
            if (turning)
            {
                turn_sec++;
                printf("[TRN] Step%d %ds err=%.1f gyro=%.1f\r\n", step, turn_sec, yaw_err, Get_Current_Gyro());
                if (turn_sec >= 8)
                {
                    turning = 0;
                    turn_sec = 0;
                    step++;
                    motorState.Target_Yaw += 90.0f;
                    sec_cnt = 0;
                    anglepid.integrator = 0;
                    anglepid.last_error = 0;
                    gyropid.integrator = 0;
                    gyropid.last_error = 0;
                    printf("[TO] Step%d TIMEOUT 8s, skip to Step%d Tgt=%.1f\r\n",
                           step - 1, step, motorState.Target_Yaw);
                }
            }
            else
            {
                turn_sec = 0;
                sec_cnt++;
                printf("[STR] %d/5s err=%.1f\r\n", sec_cnt, yaw_err);
            }

            if (sec_cnt >= 5 && !turning)
            {
                step++;
                motorState.Target_Yaw += 90.0f;
                sec_cnt = 0;
                anglepid.integrator = 0;
                anglepid.last_error = 0;
                gyropid.integrator = 0;
                gyropid.last_error = 0;
                printf("[TRG] Step%d Tgt=%.1f (+90,L)\r\n", step, motorState.Target_Yaw);
            }
        }

        pwmL = motorState.pwmL;
        pwmR = motorState.pwmR;

//        sprintf(txt, "Tgt:%03.0f Err:%04.1f ", motorState.Target_Yaw, yaw_err);
//        Display_showString(0, 1, txt, U16_WHITE, U16_BLACK, 16);
//        sprintf(txt, "Yaw:%.1f Gyr:%.1f  ", cur_yaw, Get_Current_Gyro());
//        Display_showString(0, 2, txt, U16_WHITE, U16_BLACK, 16);
//        sprintf(txt, "%s %ds          ", turning ? "TRN" : "STR", turning ? turn_sec : 5 - sec_cnt);
//        Display_showString(0, 3, txt, U16_WHITE, U16_BLACK, 16);
//        sprintf(txt, "SpdL:%.0f SpdR:%.0f ", motorState.Target_SpeedL, motorState.Target_SpeedR);
//        Display_showString(0, 4, txt, U16_WHITE, U16_BLACK, 16);
//        sprintf(txt, "L:%05d R:%05d Stp%d", pwmL, pwmR, step);
//        Display_showString(0, 5, txt, U16_WHITE, U16_BLACK, 16);

        LED_Ctrl(LED0, turning ? ON : OFF);

//        TR_Write_AngleLoopData(yaw_err, cur_yaw, motorState.Target_Yaw,
//                               motorState.Target_Gyro, turning, step,
//                               Get_Current_SpeedL(), Get_Current_SpeedR());
//
//        Delay_Ms(20);
    }
}
