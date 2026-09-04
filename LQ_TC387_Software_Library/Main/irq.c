/*******************************************************************************
 * @file                本文件是LQ_TC387_Software_Library 软件开源库文件的一部分
 * @author              chiusir
 * @email               chiusir@163.com
 * @version             V2.0.0
 * @update              2026年3月24日
 * @copyright           版权所有 (C) 2025-2026 北京龙邱科技有限公司
 * @website             http://www.lqist.cn
 * @taobao              http://longqiu.taobao.com
 *
 * @brief               龙邱科技 LQ_TC387核心板驱动库声明
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
#include "irq.h"

/***************************定时器中断回调函数**********************************/
IFX_INTERRUPT(CCU60_CH0_IRQHandler, CCU60_VECTABNUM, CCU60_CH0_PRIORITY);
IFX_INTERRUPT(CCU60_CH1_IRQHandler, CCU60_VECTABNUM, CCU60_CH1_PRIORITY);
IFX_INTERRUPT(CCU61_CH0_IRQHandler, CCU61_VECTABNUM, CCU61_CH0_PRIORITY);
IFX_INTERRUPT(CCU61_CH1_IRQHandler, CCU61_VECTABNUM, CCU61_CH1_PRIORITY);

// CCU60_CH0中断服务函数
void CCU60_CH0_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();

    // 清除中断标志
    IfxCcu6_clearInterruptStatusFlag(&MODULE_CCU60, IfxCcu6_InterruptSource_t12PeriodMatch);

    /* 用户代码 */
    static uint8_t cnt = 0;
    cnt++;
    if(cnt % 2 == 0)
    {
        Attitude_get();
    }
    if(cnt % 5 == 0) //速度环2ms （增量式）
    {
        Encoder_Update();

        if(Get_Angle_Mode())
        {
            AngleLoop_Update(Get_Current_Yaw());
            Gyro_Loop_Update(Get_Current_Gyro());
        }
        else if (!g_test_active)
        {
            DirectionLoop_Update(Track_GetDiff());
        }

        if (!g_test_active || Get_Angle_Mode())
        {
            SpeedLoop_Update(Get_Current_SpeedL(),Get_Current_SpeedR());
        }
    }
    if(cnt % 10 == 0)
    {
        cnt = 0;
    }
#if VOFA_CONTROL_ENABLE
    Vofa_CCU6_Tick();              /* VOFA: 1ms 计数 + 分频发送 */
#endif
}
// CCU60_CH1中断服务函数
void CCU60_CH1_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();

    // 清除中断标志
    IfxCcu6_clearInterruptStatusFlag(&MODULE_CCU60, IfxCcu6_InterruptSource_t13PeriodMatch);

    /* 用户代码 */
    LQ_BLDCCtrl();
}

// CCU61_CH0中断服务函数
void CCU61_CH0_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();

    // 清除中断标志
    IfxCcu6_clearInterruptStatusFlag(&MODULE_CCU61, IfxCcu6_InterruptSource_t12PeriodMatch);

    /* 用户代码 */
    LED_Ctrl(LED1, RVS); // LED点亮
}

// CCU61_CH1中断服务函数
void CCU61_CH1_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();

    // 清除中断标志
    IfxCcu6_clearInterruptStatusFlag(&MODULE_CCU61, IfxCcu6_InterruptSource_t13PeriodMatch);

    /* 用户代码 */
    //    if(cnt % 1 == 0)
    //    {
    //        Encoder_Update();                                                        //注意改DELTA_T
    //        SpeedLoop_Update(Get_Current_SpeedL(),Get_Current_SpeedR());             //速度环 输入速度 输出pwm
    //    }
    //    if(cnt % 2 == 0)
    //    {
    //        Yaw_Update();
    //        Gyro_Loop_Update(Get_Current_Gyro());                                    //角速度环 输入角速度 输出速度
    //    }
    //    if(cnt % 4 == 0)
    //    {
    //        cnt = 0;
    //        if(Get_Target_yaw()) AngleLoop_Updata(Get_Current_Yaw());               //角度环 输入角度 输出角速度
    //        else DirectionLoop_Update(Get_error());                                 //方向环 输入偏差 输出角速度
    //    }
}
// /***************************串口中断回调函数**********************************/
/* UART中断 */
IFX_INTERRUPT(UART0_RX_IRQHandler, UART0_VECTABNUM, UART0_RX_PRIORITY);
IFX_INTERRUPT(UART1_RX_IRQHandler, UART1_VECTABNUM, UART1_RX_PRIORITY);
IFX_INTERRUPT(UART2_RX_IRQHandler, UART2_VECTABNUM, UART2_RX_PRIORITY);
IFX_INTERRUPT(UART3_RX_IRQHandler, UART3_VECTABNUM, UART3_RX_PRIORITY);
IFX_INTERRUPT(UART0_TX_IRQHandler, UART0_VECTABNUM, UART0_TX_PRIORITY);
IFX_INTERRUPT(UART1_TX_IRQHandler, UART1_VECTABNUM, UART1_TX_PRIORITY);
IFX_INTERRUPT(UART2_TX_IRQHandler, UART2_VECTABNUM, UART2_TX_PRIORITY);
IFX_INTERRUPT(UART3_TX_IRQHandler, UART3_VECTABNUM, UART3_TX_PRIORITY);
IFX_INTERRUPT(UART0_ER_IRQHandler, UART0_VECTABNUM, UART0_ER_PRIORITY);
IFX_INTERRUPT(UART1_ER_IRQHandler, UART1_VECTABNUM, UART1_ER_PRIORITY);
IFX_INTERRUPT(UART2_ER_IRQHandler, UART2_VECTABNUM, UART2_ER_PRIORITY);
IFX_INTERRUPT(UART3_ER_IRQHandler, UART3_VECTABNUM, UART3_ER_PRIORITY);

// 串口0 RX中断函数
void UART0_RX_IRQHandler(void)
{
    IfxAsclin_Asc_isrReceive(&g_UartConfig[0]);

    /* 用户代码 */
}
// 串口0 TX中断函数
void UART0_TX_IRQHandler(void)
{
    IfxAsclin_Asc_isrTransmit(&g_UartConfig[0]);

    /* 用户代码 */
}
// 串口0 ER中断函数
void UART0_ER_IRQHandler(void)
{
    IfxAsclin_Asc_isrError(&g_UartConfig[0]);

    /* 用户代码 */
}

unsigned char ReadBuff1[512];

// 串口1 RX中断函数
void UART1_RX_IRQHandler(void)
{
    IfxAsclin_Asc_isrReceive(&g_UartConfig[1]);

    /* 用户代码 */
//    R9DS_Read();
}
// 串口1 TX中断函数
void UART1_TX_IRQHandler(void)
{
    IfxAsclin_Asc_isrTransmit(&g_UartConfig[1]);

    /* 用户代码 */
}
// 串口1 ER中断函数
void UART1_ER_IRQHandler(void)
{
    IfxAsclin_Asc_isrError(&g_UartConfig[1]);

    /* 用户代码 */
}
// 串口2 RX中断函数
void UART2_RX_IRQHandler(void)
{
    IfxAsclin_Asc_isrReceive(&g_UartConfig[2]);

    /* 用户代码 */
}
// 串口2 TX中断函数
void UART2_TX_IRQHandler(void)
{
    IfxAsclin_Asc_isrTransmit(&g_UartConfig[2]);

    /* 用户代码 */
}
// 串口2 ER中断函数
void UART2_ER_IRQHandler(void)
{
    IfxAsclin_Asc_isrError(&g_UartConfig[2]);

    /* 用户代码 */
}
// 串口3 RX中断函数
void UART3_RX_IRQHandler(void)
{

    IfxAsclin_Asc_isrReceive(&g_UartConfig[3]);

#if VOFA_CONTROL_ENABLE
    /* VOFA 命令接收：逐字节解析 name:value\n 格式 */
    {
        char buf = UART_GetChar(UART3);
        Vofa_RX_Handler((uint8_t)buf);
    }
#else
    /*获取1202反馈的定位信息*/
    static int num1;
    char buf = UART_GetChar(UART3);
    if (buf == '\n' || buf == '\r')
    {
        strcpy((char *)Save_Data.GPS_Buffer, (const char *)ReadBuff1);
        Save_Data.isGetData = 1;
        num1 = 0;
        LED_Ctrl(LED1, RVS); // LED翻转闪烁
    }
    else
    {
        LED_Ctrl(LED0, RVS); // LED翻转闪烁
        ReadBuff1[num1++] = buf;
    }
#endif
}
// 串口3 TX中断函数
void UART3_TX_IRQHandler(void)
{

    IfxAsclin_Asc_isrTransmit(&g_UartConfig[3]);

    /* 用户代码 */
}
// 串口3 ER中断函数
void UART3_ER_IRQHandler(void)
{
    IfxAsclin_Asc_isrError(&g_UartConfig[3]);
    /* 用户代码 */
}

/***************************外部中断中断回调函数**********************************/
/* GPIO外部中断 */
IFX_INTERRUPT(PIN_INT0_IRQHandler, PIN_INT0_VECTABNUM, PIN_INT0_PRIORITY);
IFX_INTERRUPT(PIN_INT1_IRQHandler, PIN_INT1_VECTABNUM, PIN_INT1_PRIORITY);
IFX_INTERRUPT(PIN_INT2_IRQHandler, PIN_INT2_VECTABNUM, PIN_INT2_PRIORITY);
IFX_INTERRUPT(PIN_INT3_IRQHandler, PIN_INT3_VECTABNUM, PIN_INT3_PRIORITY);
IFX_INTERRUPT(PIN_INT4_IRQHandler, PIN_INT4_VECTABNUM, PIN_INT4_PRIORITY);
IFX_INTERRUPT(PIN_INT5_IRQHandler, PIN_INT5_VECTABNUM, PIN_INT5_PRIORITY);
IFX_INTERRUPT(PIN_INT6_IRQHandler, PIN_INT6_VECTABNUM, PIN_INT6_PRIORITY);
IFX_INTERRUPT(PIN_INT7_IRQHandler, PIN_INT7_VECTABNUM, PIN_INT7_PRIORITY);

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：PIN_INT0_IRQHandler中断服务函数
@功能说明：
@参数说明：无
@函数返回：无
@备    注：外部中断0组管脚 使用的中断服务函数
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void PIN_INT0_IRQHandler(void)
{

    /* 用户代码 */
    // LED_Ctrl(LED1, RVS); // 电平翻转,LED闪烁
}

// PIN_INT1_IRQHandler中断服务函数
void PIN_INT1_IRQHandler(void)
{
    /* 用户代码 */
    // LED_Ctrl(LED1, RVS); // 电平翻转,LED闪烁
}

// PIN_INT2_IRQHandler中断服务函数
void PIN_INT2_IRQHandler(void)
{
    // 用户代码
//     LED_Ctrl(LED0, RVS); // 电平翻转,LED闪烁
}

// PIN_INT3_IRQHandler中断服务函数
void PIN_INT3_IRQHandler(void)
{
    // 用户代码
    // LED_Ctrl(LED1, RVS); // 电平翻转,LED闪烁
}
// PIN_INT4_IRQHandler中断服务函数
void PIN_INT4_IRQHandler(void)
{
    // 用户代码
}
// PIN_INT5_IRQHandler中断服务函数
void PIN_INT5_IRQHandler(void)
{
    // 用户代码
}
// PIN_INT6_IRQHandler中断服务函数
void PIN_INT6_IRQHandler(void)
{
    // 用户代码
//    LED_Ctrl(LED1, RVS); // 电平翻转,LED闪烁
}
// PIN_INT7_IRQHandler中断服务函数
void PIN_INT7_IRQHandler(void)
{
    // 已被摄像头占用，不可使用
//    LED_Ctrl(LED1, RVS); // 电平翻转,LED闪烁
    DMA_CameraStart(PIN_INT2_PRIORITY);
}

char Flag_1s = 0;

// STM0_CH1中断服务函数
IFX_INTERRUPT(STM0_CH0_IRQHandler, STM0_VECTABNUM, STM0_CH0_PRIORITY);
IFX_INTERRUPT(STM0_CH1_IRQHandler, STM0_VECTABNUM, STM0_CH1_PRIORITY);
IFX_INTERRUPT(STM1_CH0_IRQHandler, STM1_VECTABNUM, STM1_CH0_PRIORITY);
IFX_INTERRUPT(STM1_CH1_IRQHandler, STM1_VECTABNUM, STM1_CH1_PRIORITY);

// STM0_CH0中断服务函数
void STM0_CH0_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();
    // 清除中断标志
    IfxStm_clearCompareFlag(&MODULE_STM0, g_StmCompareConfig[0].comparator);
    // 开启新的中断配置，开始下次中断
    IfxStm_increaseCompare(&MODULE_STM0, g_StmCompareConfig[0].comparator, g_StmCompareConfig[0].ticks);
    /* 用户代码 */
    Flag_1s = 1;
}

void STM0_CH1_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();
    // 清除中断标志
    IfxStm_clearCompareFlag(&MODULE_STM0, g_StmCompareConfig[1].comparator);
    // 开启新的中断配置，开始下次中断
    IfxStm_increaseCompare(&MODULE_STM0, g_StmCompareConfig[1].comparator, g_StmCompareConfig[1].ticks);
    /* 用户代码 */
}

// STM1_CH0中断服务函数
void STM1_CH0_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();
    // 清除中断标志
    IfxStm_clearCompareFlag(&MODULE_STM1, g_StmCompareConfig[2].comparator);
    // 开启新的中断配置，开始下次中断
    IfxStm_increaseCompare(&MODULE_STM1, g_StmCompareConfig[2].comparator, g_StmCompareConfig[2].ticks);
    /* 用户代码 */
}

// STM1_CH1中断服务函数
void STM1_CH1_IRQHandler(void)
{
    /* 开启CPU中断  否则中断不可嵌套 */
    IfxCpu_enableInterrupts();
    // 清除中断标志
    IfxStm_clearCompareFlag(&MODULE_STM1, g_StmCompareConfig[3].comparator);
    // 开启新的中断配置，开始下次中断
    IfxStm_increaseCompare(&MODULE_STM1, g_StmCompareConfig[3].comparator, g_StmCompareConfig[3].ticks);
    /* 用户代码 */
}
