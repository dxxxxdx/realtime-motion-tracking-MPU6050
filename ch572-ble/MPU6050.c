/* ============================== Includes ============================== */

#include "MPU6050.h"
#include "I2C_HW.h"

/* ============================== Config ================================ */

#define MPU6050_DEFAULT_ADDR_7BIT 0x68

/* ============================== Private Prototypes ==================== */

static uint8_t MPU6050_I2C_Read(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes);
static uint8_t MPU6050_I2C_Write(uint8_t internalReg, uint8_t *data, uint8_t lenInBytes);
static void MPU6050_OnBusy(MPU6050 *self);

/* ============================== Weak Hooks ============================ */

__attribute__((weak)) void MPU6050_Callback(MPU6050* self){}

/* ============================== Driver State ========================== */

I2CIO g_mpu6050_i2cIO = {
    .SDAChange = CH572_SDA_Change,
    .SCLChange = CH572_SCL_Change,
    .I2CDelay = CH572_I2C_Delay,
    .SDARead = CH572_SDA_Read,
};

I2C_Operation g_i2cOP;

uint8_t MPU6050ReadBuffer [32] ;
uint8_t MPU6050WriteBuffer [32] ;

/* ============================== Default Sensor Instance =============== */

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
//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING
//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING//WARNING
//这段强耦合了全局的g_i2cOP，容易让人血压飙升，虽然也不是很抽象
//如果你需要拓展，建议多开几个载荷防止竞态
//还有就是这载荷为了兼容性拷贝了一份..........
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

/* ============================== Data Access API ======================= */

MPU6050_RET MPU6050_Read_Data(MPU6050* self, ACCELERATION_DATA_t* data)
{
    uint8_t ret;
    if(self->Stage == MPU6050_I2CBUSY)
    {
        self->OnBusy(self);
        return MPU6050_BUSY;
    }
    self->Stage = MPU6050_I2CBUSY;
    self->action = MPU6050_READDATA;
    ret = self->readI2C(MPU6050_DATA_REG, (uint8_t*)data, MPU6050_DATA_LEN);
    if(ret != 0)
    {
        self->Stage = MPU6050_ERROR_STAGE;
        self->action = MPU6050_NOTHINGTODO;
        return MPU6050_ERROR;
    }
    self->I2CFinishCallBack(self);
    return MPU6050_OK;
}

MPU6050_RET MPU6050_Write_Data(MPU6050* self, uint8_t internalReg, uint8_t* data,uint8_t lenInBytes)
{
    uint8_t ret;
    if(self->Stage == MPU6050_I2CBUSY)
    {
        self->OnBusy(self);
        return MPU6050_BUSY;
    }

    self->Stage = MPU6050_I2CBUSY;
    self->action = MPU6050_WRITEDATA;
    ret = self->writeI2C(internalReg, data, lenInBytes);
    if(ret != 0)
    {
        self->Stage = MPU6050_ERROR_STAGE;
        self->action = MPU6050_NOTHINGTODO;
        return MPU6050_ERROR;
    }
    self->I2CFinishCallBack(self);
    return MPU6050_OK;
}

/* ============================== Finish Callback ======================= */

void I2CDefaultFinishCallBack(MPU6050* self) {
    self->Stage = MPU6050_IDLE;
    self->action = MPU6050_NOTHINGTODO;
    MPU6050_Callback(self);
};

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
