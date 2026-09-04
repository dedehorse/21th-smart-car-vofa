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
#include "lq_include.h"


IfxCpu_syncEvent g_cpuSyncEvent = 0;
volatile uint16_t g_debugFps = 0;
volatile uint8_t g_lcdReady = 0;

// 定义共享变量，用于Cpu0和Cpu1之间传递速度数据
//volatile float g_encoderSpeed = 0.0f;

extern char Flag_1s;
extern uint8_t g_test_active;

int core0_main(void)
{

    //================================ 系统代码 ================================//
     cpu_init();                        // 等待cpu初始化、
     IfxCpu_emitEvent(&g_cpuSyncEvent); // 等待cpu同步
     IfxCpu_waitEvent(&g_cpuSyncEvent, 1);
     UART_InitConfig(UART0_RX_P14_1, UART0_TX_P14_0, 115200);  // 下载口的串口,默认重定向PRINTF 在config中选择
    UART_InitConfig(UART3_RX_P00_1, UART3_TX_P00_0, 115200); //  调试 UART 初始化
     Display_Init(2);        // LCD初始化
     Display_CLS(U16_BLACK); // 黑色屏幕
     g_lcdReady = 1;
     Tracking_init();
     MotorInit();
     EncInit();
     SPI_Gryo_Init();
     PidInit();
#if VOFA_CONTROL_ENABLE
     Vofa_Init();
#endif
 #if PATH_PLAN_ENABLE
     PathPlan_Init();
     PathPlan_SetRoute(1);   /* 0=414路线, 1=原始23节点路线, 2=无锁路线 */
 #endif
     CameraInt();
     TR_driver_init();
     g_test_active = 0;  //角度环测试开启
 #if MENU_ENABLE
     GPIO_KEY_Init();                  /* 初始化按键 GPIO (KEY1=P33_9, KEY5=P33_5)  */
     Display_Menu();                   /* 阻塞式参数查看菜单, 长按P33_9退出           */
     EncInit();                        /* 恢复 P33_5 编码器引脚配置                  */
 #endif

     /* 控制中断和风扇启动前车辆保持静止, 采集陀螺仪零偏。 */
     IMU_GetOffset(&icm_offset);

 #if BLS_LAUNCH_ENABLE
     /* 风扇直接运行到620，车辆保持停止2秒后再发车。 */
     g_stop_active = 1;
     incpidL.integrator = 0;
     incpidR.integrator = 0;
     BLS3motorInit();
     BlsmotorCtrl(BLS3, BLS_RUN_DUTY);
     Delay_Ms(BLS_LAUNCH_DELAY_MS);
     incpidL.integrator = 0;
     incpidR.integrator = 0;
     g_stop_active = 0;
 #endif

     CCU6_InitConfig(CCU60, CCU6_Channel0, 1000); // CCU6初始化 (控制ISR此时才开始)
     STM_InitConfig(STM0, STM_Channel_0, 1000000);//1s中断

//    ================================ 外设以及模块驱动测试函数 ================================//
//     Test_GPIO_OUT();    // PASS,测试GPIO，P10.6和P10.5闪灯
//     Test_GPIO_KEY();    // PASS,测试外部按键输入，P22.0--2   按下按键   LED亮
//     Test_GPIO_Extern(); // PASS,测试外部第1组中断P15.4，P10.6和P10.5闪灯
//       Test_Display();     // PASS,测试显示屏，需要去Main/config.h中选择屏幕类型和接口类型
//     Test_CCU6_Timer();  // PASS,测试CCU6定时器
//     Test_ADC();         // PASS,测试ADC, 并把值显示在屏幕上
//     Test_IIC_Gyro();    // PASS,测试陀螺仪模块,包括MPU6050 9250或者ICM20602 20689 IIC接线   P13_1接SCL  P13_2接SDA
//    Test_SPI_Gyro();    // PASS,测试陀螺仪模块,SPI接线，可直接插到母板陀螺仪接口
//     Test_LQ6050_DMP();  // PASS,测试6050DMP,IIC接线   P13_1接SCL  P13_2接SDA
//     Test_Quat();        // PASS,测试四元数运算，陀螺仪姿态解算,采用spi接线，需要把解算和读取函数放在一个1ms的定时器中
//     Test_Encoder();     // PASS,测试编码器，兼容正交解码以及带方向龙邱编码器
//     Test_EEPROM();      // PASS,测试内部EEPROM擦写功能  屏幕提示是否写入成功
//     Test_SoftFft();     // PASS,测试ILLD库的软件FFT函数
//     Test_Motor();       // PASS,测试4路电机PWM控制
//     Test_GyroLoop();    // 测试角速度环,用手旋转车身观察电机是否有阻尼力
//     Test_AngleLoop();    //测试角度环,每3秒自动+90度旋转
//     Test_MotorBLDC();   // PASS,测试2134无刷驱动，接线说明在例程测试函数下
//     Test_Servo();       // PASS,测试两路舵机
//       Test_BlsMotor();    // PASS,测试无感无刷电机驱动,新增P21_2 BLS3
//     Test_Bluetooth();   // PASS,测试UART0(P14.0RX/P14.1TX), lq_WLS_config()初始化串口配置无线通信模块，参数修改详见 函数内宏
//     Test_SBUS();        // PASS,测试遥控器接收数据
//     Test_STM_Timer();   // PASS,利用STM模块去完成代码运行时间记录
//     Test_UTM();         // PASS,测试UTM坐标转换函数
//     Test_BD1202();      // PASS,测试GPS模块。 注意，测试的时候一定要把对应串口中断里面加入读取程序，要不然不能正常收到GPS信号, 一定要看一下函数说明
//     Test_CAMERA();      // PASS,测试龙邱神眼摄像头并在屏幕上显示  LQ_CAMERA.h 中选择屏幕
//     Test_CAMERA_TR();   // PASS,测试龙无线通信模块WIFI图像传输
//     Test_Tracking()     // PASS,测试龙16路模拟量灰度循迹模块，默认串口输出并再IPS屏幕上显示
//     Test_ADC_Key();     // PASS,测试ADC按键和旋钮,在LCD上显示按键状态和ADC值
//   先解决图像（上位机颜色 初始化方差阈值 ） 在解决节点（左t 右t 十字 一套逻辑）（左直角 右直角 t 一套逻辑） 节点识别（根据前方路口特征 节点数组标志位+1） 接着调整角速度环
     uint16_t fps = 0;
#if BT_TRACK_OUTPUT
     uint32_t bt_last_send_us = (uint32_t)STM_GetNowUs(STM0);
#endif

     while (1)
     {
#if BLS_LAUNCH_ENABLE
         /* 跑完全部路口 → 风扇回零油门停车 (单次触发) */
         {
             static uint8_t car_stopped = 0;
             if (!car_stopped && g_pp_action == ACT_STOP) {
                 BlsmotorCtrl(BLS3, BLS_STOP_DUTY);
                 car_stopped = 1;
             }
         }
#endif
         if (Camera_Flag == 2)
         {
             Get_Use_Image();
             Camera_Flag = 0;
             fps++;
             Get_Bin_Image();
             Image_Filter(30);
             Track_Process();
#if TRACK_OUTPUT_WIRELESS
             Wireless_SendTrackImage((const unsigned char *)Pixle);
#endif
#if BT_TRACK_OUTPUT
             {
                 uint32_t now_us = (uint32_t)STM_GetNowUs(STM0);
                 if (!g_test_active
                     && (uint32_t)(now_us - bt_last_send_us)
                        >= (uint32_t)BT_TRACK_INTERVAL_MS * 1000U) {
                     bt_last_send_us = now_us;
                     Bluetooth_SendTrackData();
                 }
             }
#endif

         }

         if (Flag_1s && !g_test_active)
         {
             Flag_1s = 0;
             g_debugFps = fps;
             Track_SetFPS(fps);
             fps = 0;
         }
#if VOFA_CONTROL_ENABLE
         Vofa_MainLoop();             /* VOFA: 检查发送标志，发送状态数据 */
#endif
     }
}
