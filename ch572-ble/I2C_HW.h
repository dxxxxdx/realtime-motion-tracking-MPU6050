//
// Created by dxxdx on 2026/6/17.
//
#ifndef CH572_BLE_PROJECT_I2C_HW_H
#define CH572_BLE_PROJECT_I2C_HW_H
#include "CH57x_common.h"
#include "I2CCtrl.h"

// 定义引脚，方便后期修改
#define I2C_SCL_PIN  (1 << 10)  // PA10
#define I2C_SDA_PIN  (1 << 7)  // PA7

/* ================== 1. 寄存器级极限抖动函数 ================== */

static void CH572_SCL_Change(uint8_t status)
{
    if(status == I2C_HIZ)
    {
        R32_PA_DIR &= ~I2C_SCL_PIN; // 切为输入（靠上拉变高电平）
    }
    else
    {
        R32_PA_DIR |= I2C_SCL_PIN;  // 切为输出（OUT恒为0，拉低电平）
    }
}

static void CH572_SDA_Change(uint8_t status)
{
    if(status == I2C_HIZ)
    {
        R32_PA_DIR &= ~I2C_SDA_PIN; // 切为输入（靠上拉变高电平，放手）
    }
    else
    {
        R32_PA_DIR |= I2C_SDA_PIN;  // 切为输出（拉低电平）
    }
}

static uint8_t CH572_SDA_Read(void)
{
    // 直接读取 PA 端口的 PIN 寄存器状态
    // 如果 SDA 引脚当前是高电平，返回 1；如果是低电平，返回 0
    return (R32_PA_PIN & I2C_SDA_PIN) ? 1 : 0;
}

static void CH572_I2C_Delay(void)
{
    // 400kHz 半周期约 1.25us
    // CH572 如果主频是 32MHz，大约循环十几下即可。具体需要你上机微调。
    volatile uint8_t i = 12;
    while(i--);
}

/* ================== 2. 实例化你的控制器 ================== */

I2CIO mpu6050_io = {
    .SDAChange = CH572_SDA_Change,
    .SCLChange = CH572_SCL_Change,
    .SDAREAD   = CH572_SDA_Read,
    .I2CDelay  = CH572_I2C_Delay
};

/* ================== 3. 硬件初始化 (极其关键) ================== */

void CH572_I2C_HW_Init(void)
{
    // 1. 开启内部上拉
    R32_PA_PU |= (I2C_SCL_PIN | I2C_SDA_PIN);

    // 2. 将数据输出寄存器 永远 设为 0
    R32_PA_CLR |= (I2C_SCL_PIN | I2C_SDA_PIN); // 或者 R32_PA_OUT &= ~(...)

    // 3. 初始状态为总线空闲：把方向全设为输入，让上拉电阻把总线拉高 (HIZ状态)
    R32_PA_DIR &= ~(I2C_SCL_PIN | I2C_SDA_PIN);
}

#endif