//
// Created by dxxdx on 2026/6/17.
//
#ifndef CH572_BLE_PROJECT_I2C_HW_H
#define CH572_BLE_PROJECT_I2C_HW_H
#include "CH57x_common.h"
#include "I2CCtrl.h" // 里面保留你的 I2C_Operation 结构体即可

// 霸道总裁宏：不管开什么优化，少废话，给我直接把代码拍在调用处！
#define FORCE_INLINE __attribute__((always_inline)) static inline

// 定义引脚
#define I2C_SCL_PIN  (1 << 10)  // PA10
#define I2C_SDA_PIN  (1 << 7)   // PA7

/* ================== 1. 寄存器级极限内联抖动函数 ================== */

FORCE_INLINE void CH572_SCL_Change(uint8_t status)
{
    if(status == I2C_HIZ) {
        R32_PA_DIR &= ~I2C_SCL_PIN;
    } else {
        R32_PA_DIR |= I2C_SCL_PIN;
    }
}

FORCE_INLINE void CH572_SDA_Change(uint8_t status)
{
    if(status == I2C_HIZ) {
        R32_PA_DIR &= ~I2C_SDA_PIN;
    } else {
        R32_PA_DIR |= I2C_SDA_PIN;
    }
}

FORCE_INLINE uint8_t CH572_SDA_Read(void)
{
    return (R32_PA_PIN & I2C_SDA_PIN) ? 1 : 0;
}

/* ================== 2. RISC-V 绝对确定性汇编延时 ================== */

FORCE_INLINE void CH572_I2C_Delay(void)
{
    // RISC-V 极简汇编死循环，彻底剥夺编译器自作聪明的权利。
    // 算法：
    // addi 减1 (1个时钟周期)
    // bnez 判断不为0则跳转 (沁恒 QingKe 内核跳转通常需要 2 个周期)
    // 所以每循环一次精确消耗约 3 个 CPU 周期。

    // 如果主频 60MHz -> 1微秒=60周期 -> 1.25us=75周期。 75 / 3 = 25。
    // 如果主频 32MHz -> 1微秒=32周期 -> 1.25us=40周期。 40 / 3 ≈ 13。
    // 请根据你的实际主频修改下面这个值！
    uint32_t volatile count = 25;

    __asm__ __volatile__(
        "1:\n\t"                  // 局部标签 1
        "addi %0, %0, -1\n\t"     // count = count - 1
        "bnez %0, 1b\n\t"         // 如果 count != 0，跳回标签 1
        : "+r" (count)            // 输出和输入约束：让编译器随便挑一个通用寄存器来存 count
    );
}

/* ================== 3. 硬件初始化 ================== */

FORCE_INLINE void CH572_I2C_HW_Init(void)
{
    R32_PA_PU |= (I2C_SCL_PIN | I2C_SDA_PIN);
    R32_PA_CLR |= (I2C_SCL_PIN | I2C_SDA_PIN);
    R32_PA_DIR &= ~(I2C_SCL_PIN | I2C_SDA_PIN);
}

#endif //CH572_BLE_PROJECT_I2C_HW_H