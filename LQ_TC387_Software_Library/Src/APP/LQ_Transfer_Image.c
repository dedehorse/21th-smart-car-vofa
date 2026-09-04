/*******************************************************************************
 *  @file                 本文件是LQ_TC387_Software_Library 软件开源库文件的一部分
 *  @author               chiusr
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

#include "LQ_Transfer_Image.h"

unsigned char FH[4] = {0xa0, 0xff, 0xff, 0xa0};
unsigned char FE[4] = {0xb0, 0xb0, 0x0a, 0x0d};

void TR_driver_init(void)
{
    QSPI_InitConfig(TR_CLK, TR_MISO, TR_MOSI, TR_CSX, 40000000, 3);
    PIN_InitConfig(TR_CS, PIN_MODE_OUTPUT, 1);
    PIN_InitConfig(IO2, PIN_MODE_INPUT, 1);
}

/* 等待无线模块就绪 (IO2=HIGH), 返回 1=成功 0=超时 */
static uint8_t TR_wait_startSign(uint16_t wait_us)
{
    uint32_t time = 0;
    while (1)
    {
        if (TR_IO2 == 1) return 1;
        Delay_Us(50);
        if ((time += 50) > wait_us) return 0;
    }
}

/* 等待无线模块接收完成 (IO2=LOW), 返回 1=成功 0=超时 */
static uint8_t TR_wait_endSign(uint16_t wait_us)
{
    uint32_t time = 0;
    while (1)
    {
        if (TR_IO2 == 0) return 1;
        Delay_Us(50);
        if ((time += 50) > wait_us) return 0;
    }
}

/* 构建并发送完整图像帧 (FH + data + FE) */
uint8_t TR_Write_Image(unsigned char high, unsigned char wide, unsigned char *dat)
{
    unsigned short i;
    unsigned short temp = high * wide + 8;
    unsigned short frequency = temp / 4000;
    unsigned short remainder = temp % 4000;
    static unsigned char img[TR_IMG_H * TR_IMG_W + 8];
    static unsigned char buff_T[4000];

    memcpy(&img[0], FH, 4);
    memcpy(&img[4], dat, high * wide);
    memcpy(&img[4 + high * wide], FE, 4);

    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &img[4000 * i], 4000);
        if (!IR_Write_byte_4000(buff_T)) return 0;
    }
    memcpy(buff_T, &img[frequency * 4000], remainder);
    return IR_Wirte_byte(buff_T, remainder);
}

void TR_Write_Image_RGB565(unsigned char high, unsigned char wide, unsigned short *dat)
{
    unsigned int pixel_count = (unsigned int)high * wide;
    unsigned int data_bytes = pixel_count * 2;
    unsigned int temp = data_bytes + 8;
    unsigned short frequency = (unsigned short)(temp / 4000);
    unsigned short remainder = (unsigned short)(temp % 4000);
    static unsigned char img[TR_IMG_H * TR_IMG_W * 2 + 8];
    static unsigned char buff_T[4000];
    unsigned short i;

    memcpy(&img[0], FH, 4);
    memcpy(&img[4], dat, data_bytes);
    memcpy(&img[4 + data_bytes], FE, 4);

    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &img[4000 * i], 4000);
        IR_Write_byte_4000(buff_T);
    }
    memcpy(buff_T, &img[frequency * 4000], remainder);
    IR_Wirte_byte(buff_T, remainder);
}

void TR_Write_Image_RGB888(unsigned char high, unsigned char wide, unsigned char *dat)
{
    unsigned int pixel_count = (unsigned int)high * wide;
    unsigned int data_bytes = pixel_count * 3;
    unsigned int temp = data_bytes + 8;
    unsigned short frequency = (unsigned short)(temp / 4000);
    unsigned short remainder = (unsigned short)(temp % 4000);
    static unsigned char img[TR_IMG_H * TR_IMG_W * 3 + 8];
    static unsigned char buff_T[4000];
    unsigned short i;

    memcpy(&img[0], FH, 4);
    memcpy(&img[4], dat, data_bytes);
    memcpy(&img[4 + data_bytes], FE, 4);

    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &img[4000 * i], 4000);
        IR_Write_byte_4000(buff_T);
    }
    memcpy(buff_T, &img[frequency * 4000], remainder);
    IR_Wirte_byte(buff_T, remainder);
}

/* 发送4000字节块, 带重试, 返回 1=成功 0=失败 */
uint8_t IR_Write_byte_4000(unsigned char *dat)
{
    unsigned char buff[32];
    unsigned char buff_V[32];
    uint8_t retry;

    for (retry = 0; retry < TR_MAX_RETRIES; retry++)
    {
        if (!TR_wait_startSign(TR_START_TIMEOUT_US)) continue;

        TR_CS_L;
        for (int fre = 0; fre < 125; fre++)
        {
            memcpy(buff, &dat[32 * fre], 32);
            QSPI_ReadWriteNByte(TR_SPIX, buff, buff_V, 32);
        }
        TR_CS_H;

        if (TR_wait_endSign(TR_END_TIMEOUT_US)) return 1;
    }
    return 0;
}

/* 发送指定长度 (<4000), 带重试, 返回 1=成功 0=失败 */
uint8_t IR_Wirte_byte(unsigned char *dat, uint16_t len)
{
    unsigned short i;
    unsigned short fre = len / 32;
    unsigned short rem = len % 32;
    unsigned char buff[32];
    unsigned char buff_V[32];
    uint8_t retry;

    for (retry = 0; retry < TR_MAX_RETRIES; retry++)
    {
        if (!TR_wait_startSign(TR_START_TIMEOUT_US)) continue;

        TR_CS_L;
        for (i = 0; i < fre; i++)
        {
            memcpy(buff, &dat[32 * i], 32);
            QSPI_ReadWriteNByte(TR_SPIX, buff, buff_V, 32);
        }
        if (rem != 0)
        {
            memset(buff, 0x00, 32);
            memcpy(buff, &dat[len - rem], rem);
            QSPI_ReadWriteNByte(TR_SPIX, buff, buff_V, rem);
        }
        TR_CS_H;

        if (TR_wait_endSign(TR_END_TIMEOUT_US)) return 1;
    }
    return 0;
}

void TR_Write_JPEG(unsigned char *dat, unsigned int len)
{
    static unsigned char buff_T[4000];
    unsigned short frequency = (unsigned short)(len / 4000);
    unsigned short remainder = (unsigned short)(len % 4000);
    unsigned short i;

    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &dat[4000 * i], 4000);
        IR_Write_byte_4000(buff_T);
    }
    if (remainder > 0)
    {
        memcpy(buff_T, &dat[frequency * 4000], remainder);
        IR_Wirte_byte(buff_T, remainder);
    }
}

void TR_Write_Image_Pixle(unsigned char height, unsigned char width, unsigned char *Pixle)
{
    uint8_t buff_T[32];
    uint8_t buff_R[32];
    unsigned long i;
    unsigned int pixel_total_bits = height * width;
    unsigned int pixel_total_bytes = pixel_total_bits / 8;
    unsigned int total_bytes = pixel_total_bytes + 8;
    unsigned int frequency = total_bytes / 32;
    unsigned char remainder = total_bytes % 32;
    static uint8_t img[TR_IMG_H * TR_IMG_W / 8 + 8];

    memset(img, 0, sizeof(img));
    memcpy(&img[0], FH, 4);

    for (int idx = 0; idx < pixel_total_bits; idx++)
    {
        int row = idx / width;
        int col = idx % width;
        if (Pixle[row * width + col] > 0)
        {
            int byteIndex = idx / 8;
            int bitOffset = idx % 8;
            img[4 + byteIndex] |= (1 << (7 - bitOffset));
        }
    }

    memcpy(&img[4 + pixel_total_bytes], FE, 4);

    TR_wait_startSign(TR_START_TIMEOUT_US);
    TR_CS_L;

    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &img[32 * i], 32);
        QSPI_ReadWriteNByte(TR_SPIX, buff_T, buff_R, 32);
    }
    if (remainder != 0)
    {
        memset(buff_T, 0x00, 32);
        memcpy(buff_T, &img[total_bytes - remainder], remainder);
        QSPI_ReadWriteNByte(TR_SPIX, buff_T, buff_R, remainder);
    }

    TR_CS_H;
    TR_wait_endSign(TR_END_TIMEOUT_US);
}

uint8_t Wireless_SendTrackImage(const unsigned char *binary_image)
{
    static uint8_t img[TR_IMG_H * TR_IMG_W / 8 + 8];
    uint8_t buff_T[32], buff_R[32];
    unsigned long i;
    unsigned int pixel_total_bits = TR_IMG_H * TR_IMG_W;
    unsigned int pixel_total_bytes = pixel_total_bits / 8;
    unsigned int total_bytes = pixel_total_bytes + 8;
    unsigned int frequency = total_bytes / 32;
    unsigned char remainder = total_bytes % 32;
    int r, c, dr, dc;

    /* 构建 188×120 位图帧 */
    memset(img, 0, sizeof(img));
    memcpy(&img[0], FH, 4);

    /* 处理后的 60×94 二值图 → 120×188 位图: 每个像素放大为 2×2 块 */
    for (r = 0; r < LCDH; r++)
    {
        for (c = 0; c < LCDW; c++)
        {
            if (binary_image[r * LCDW + c] > 0)
            {
                int base_r = r * 2, base_c = c * 2;
                for (dr = 0; dr < 2; dr++)
                {
                    for (dc = 0; dc < 2; dc++)
                    {
                        int idx = (base_r + dr) * TR_IMG_W + (base_c + dc);
                        img[4 + idx / 8] |= (1 << (7 - idx % 8));
                    }
                }
            }
        }
    }

    memcpy(&img[4 + pixel_total_bytes], FE, 4);

    /* 通过 QSPI 发送 */
    if (!TR_wait_startSign(TR_START_TIMEOUT_US)) return 0;
    TR_CS_L;
    for (i = 0; i < frequency; i++)
    {
        memcpy(buff_T, &img[32 * i], 32);
        QSPI_ReadWriteNByte(TR_SPIX, buff_T, buff_R, 32);
    }
    if (remainder != 0)
    {
        memset(buff_T, 0x00, 32);
        memcpy(buff_T, &img[total_bytes - remainder], remainder);
        QSPI_ReadWriteNByte(TR_SPIX, buff_T, buff_R, remainder);
    }
    TR_CS_H;
    TR_wait_endSign(TR_END_TIMEOUT_US);
    return 1;
}

/*LLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLLL
@函数名称：void Test_CAMERA (void)
@功能说明：摄像头图像传输
    测试流程，插上无线模块-》确认好程序初始化配置的端口无误后-》打开母板电源 编译烧录程序-》
    打开龙邱多功能调试上位机软件https://gitee.com/lq-tech/LQ-Tool，切换到图像窗口，打开右边网络配置，确保你的网络连接成功后确定IP地址
    如果连接成功且，配置正确，会在上位机看到实时图像
@参数说明：void
@函数返回：void
@备    注：
QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ*/
void Test_CAMERA_TR(void)
{
    GPIO_LED_Init();
    Display_Init(2);
    Display_CLS(U16_BLACK);
    TR_driver_init();

    CAMERA_Init(100);

    while (1)
    {
        if (Camera_Flag == 2)
        {
            Get_Use_Image();
            Camera_Flag = 0;

            Get_Bin_Image();
            Image_Filter(30);
            Wireless_SendTrackImage((const unsigned char *)Pixle);
            Display_Show(0, 0, LCDH, LCDW, (unsigned char *)Pixle);
            LED_Ctrl(LED0, RVS);
        }
    }
}
