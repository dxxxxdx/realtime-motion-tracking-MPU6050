//
// Created by my on 2026/5/28.
//

#include "MPU6050.h"

#include <stddef.h>


#include "stm32f0xx.h" // 确保包含了你的 HAL 库 I2C 头文件，通常是 i2c.h 或 main.h
extern I2C_HandleTypeDef hi2c1;
/* ------------------ 1. 底层 HAL 库适配函数 ------------------ */

// 注意：STM32 HAL库的从机地址需要左移1位（7位地址变8位写/读地址）
// 这里假设传入的 self->I2Cadr 已经是左移后的，或者在函数内部处理。
// 下面代码在调用 HAL 时将地址左移 1 位：(self_dev_addr << 1)

static MPU6050* g_mpu_handle = NULL; // 用于在HAL非阻塞回调中找回对象指针
static uint8_t g_mpu_i2c_addr = 0x68; // 缓存地址，供底层没有 self 指针时使用

/**
 * @brief 适配 HAL 库的 I2C 读取函数
 */
static uint8_t STM32_HAL_I2C_Read(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes) {
    // 假设使用 hi2c1，如果使用别的硬件 I2C 请在此处修改
    // 这里使用 HAL_I2C_Mem_Read_IT (中断模式) 或 HAL_I2C_Mem_Read_DMA (DMA模式) 才能配合你的 Callback 异步机制
    // 如果你只想用最简单的阻塞模式，改用 HAL_I2C_Mem_Read

    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Read_IT(&hi2c1, (uint16_t)(g_mpu_i2c_addr << 1),
                                 (uint16_t)internalReg, I2C_MEMADD_SIZE_8BIT,
                                 data, lenInBytes);

    return (status == HAL_OK) ? 0 : 1;
}

/**
 * @brief 适配 HAL 库的 I2C 写入函数
 */
static uint8_t STM32_HAL_I2C_Write(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes) {
    HAL_StatusTypeDef status;

    status = HAL_I2C_Mem_Write_IT(&hi2c1, (uint16_t)(g_mpu_i2c_addr << 1),
                                  (uint16_t)internalReg, I2C_MEMADD_SIZE_8BIT,
                                  data, lenInBytes);

    return (status == HAL_OK) ? 0 : 1;
}

/**
 * @brief 默认的忙状态处理函数
 */
static void MPU6050_Default_OnBusy(struct MPU6050* self) {
    // 当驱动处于 BUSY 状态，用户尝试再次读写时触发
    // 可以在这里写点调试信息或者直接留空
    (void)self;
}


/* ------------------ 2. HAL 库中断回调函数绑定 ------------------ */

/**
 * @brief HAL库 I2C 传输完成中断回调函数
 * 当 HAL_I2C_Mem_Read_IT 或 Mem_Write_IT 完成时，HAL库会自动调用这个函数
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == hi2c1.Instance) {
        if (g_mpu_handle != NULL) {
            // 触发你在头文件中定义的默认结束回调
            I2CDefaultFinishCallBack(g_mpu_handle);
        }
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == hi2c1.Instance) {
        if (g_mpu_handle != NULL) {
            // 大小端处理：MPU6050高字节在前，STM32是小端存储
            // 如果你直接把 14 字节的数据 readI2C 到 ACCELERATION_DATA_t 结构体中
            // 必须在完成中断里进行高低字节交换，否则数据是错的。
            // 这里为了不破坏你的结构体设计，你可以选择在 MPU6050_Callback 留给应用层转，或者在这里转

            I2CDefaultFinishCallBack(g_mpu_handle);
        }
    }
}


/* ------------------ 3. 供外部调用的高级初始化函数 ------------------ */

/**
 * @brief 针对 STM32F030 HAL 库封装的 MPU6050 初始化
 * @param self MPU6050 结构体指针
 * @param I2Cadr7Bit MPU6050的7位I2C地址 (通常是 0x68 或 0x69)
 * @return MPU6050_RET
 */
MPU6050_RET STM32_MPU6050_Binding(MPU6050* self, uint8_t I2Cadr7Bit) {
    if (self == NULL) {
        return MPU6050_ERROR;
    }

    // 1. 缓存全局指针和地址，供中断回调及底层无 self 参数的接口使用
    g_mpu_handle = self;
    g_mpu_i2c_addr = I2Cadr7Bit;

    // 2. 显式绑定你头文件里定义的成员属性
    self->I2CFinishCallBack = I2CDefaultFinishCallBack;

    // 3. 调用你在头文件中写好的原厂 MPU6050_Init 进行函数指针绑定和唤醒
    // 这里传入我们写好的 STM32_HAL_I2C_Read/Write 适配函数
    return MPU6050_Init(self,
                        I2Cadr7Bit,
                        STM32_HAL_I2C_Read,
                        STM32_HAL_I2C_Write,
                        MPU6050_Default_OnBusy);
}









