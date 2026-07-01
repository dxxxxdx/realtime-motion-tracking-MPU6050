/* ============================== Includes ============================== */

#include "MPU6050.h"
#include "I2C_HW.h"

/* ============================== Config ================================ */

#define MPU6050_DEFAULT_ADDR_7BIT 0x68

/* ============================== Notes ================================= */

//WARNING
//这段强耦合了全局的g_i2cOP，容易血压飙升，虽然也不是很抽象
//如果你需要拓展，建议多开几个载荷防止竞态

/* ============================== Private Prototypes ==================== */

static uint8_t MPU6050_I2C_Read(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes);
static uint8_t MPU6050_I2C_Write(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes);
static void MPU6050_OnBusy(MPU6050 *self);

/* ============================== Weak Hooks ============================ */

__attribute__((weak)) void MPU6050_Callback(MPU6050* self){}

/* ============================== Static State ========================== */

static I2CIO g_mpu6050_i2cIO = {
    .SDAChange = CH572_SDA_Change,
    .SCLChange = CH572_SCL_Change,
    .I2CDelay = CH572_I2C_Delay,
    .SDARead = CH572_SDA_Read,
};

static I2C_Operation g_i2cOP;

uint8_t MPU6050ReadBuffer [32] ;
uint8_t MPU6050WriteBuffer [32] ;

/* ============================== Static Sensor Instance ================ */

MPU6050 g_mpu = {
    .I2Cadr = MPU6050_DEFAULT_ADDR_7BIT,
    .Stage = MPU6050_IDLE,
    .action = MPU6050_NOTHINGTODO,
    .readI2C = MPU6050_I2C_Read,
    .writeI2C = MPU6050_I2C_Write,
    .I2CFinishCallBack = I2CDefaultFinishCallBack,
    .OnBusy = MPU6050_OnBusy,
};

/* ============================== I2C Adapter =========================== */

static uint8_t MPU6050_I2C_Read(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes)
{
    g_i2cOP.TargetAddress7Plus1Bit = (uint8_t)((g_mpu.I2Cadr << 1) | 0x01);
    g_i2cOP.HostRegisterAddress = internalReg;
    g_i2cOP.ReadBufferSize = lenInBytes;
    g_i2cOP.WriteBufferSize = 0;
    g_i2cOP.ReadBuffer = data;
    g_i2cOP.WriteBuffer = MPU6050WriteBuffer;

    return I2C_Process(&g_mpu6050_i2cIO, &g_i2cOP);
}

static uint8_t MPU6050_I2C_Write(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes)
{
    g_i2cOP.TargetAddress7Plus1Bit = (uint8_t)(g_mpu.I2Cadr << 1);
    g_i2cOP.HostRegisterAddress = internalReg;
    g_i2cOP.ReadBufferSize = 0;
    g_i2cOP.WriteBufferSize = lenInBytes;
    g_i2cOP.ReadBuffer = MPU6050ReadBuffer;
    g_i2cOP.WriteBuffer = data;

    return I2C_Process(&g_mpu6050_i2cIO, &g_i2cOP);
}

/* ============================== Busy Hook ============================= */

static void MPU6050_OnBusy(MPU6050 *self)
{
    (void)self;
}

/* ============================== Init ================================== */

MPU6050_RET MPU6050_Init(MPU6050* self)
{
    CH572_I2C_HW_Init();
    return MPU6050_Write_Data(self,0x6b, (uint8_t[]){0x01},1);
}

