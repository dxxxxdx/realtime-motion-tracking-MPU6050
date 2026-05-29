//
// Created by my on 2026/5/28.
//

#ifndef REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H
#define REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H


#include <stdint.h>

#include "cmsis_gcc.h"

typedef enum {
    MPU6050_IDLE = 0,
    MPU6050_I2CBUSY,
    MPU6050_ERROR_STAGE,
} MPU6050_Stage;

typedef enum {
    MPU6050_NOTHINGTODO = 0,
    MPU6050_READDATA,
    MPU6050_WRITEDATA,
}MPU6050_ACTION;

typedef struct MPU_DATA_t {
    int16_t AccX;
    int16_t AccY;
    int16_t AccZ;
    int16_t T;
    int16_t GyroX;
    int16_t GyroY;
    int16_t GyroZ;
}ACCELERATION_DATA_t;


typedef struct MPU6050 {
    uint8_t I2Cadr ;
    uint8_t Stage ;
    uint8_t action ;
    uint8_t (*readI2C)(uint8_t internalReg,uint8_t *data,uint8_t lenInBytes);
    uint8_t (*writeI2C)(uint8_t internalReg,uint8_t *data,uint8_t lenInBytes);
    void(*I2CFinishCallBack)(struct MPU6050* self);
    void(*OnBusy)(struct MPU6050* self);
}MPU6050;

typedef enum {
    MPU6050_OK = 0,
    MPU6050_ERROR,
    MPU6050_BUSY,

}
MPU6050_RET;
__attribute__((weak)) void MPU6050_Callback(MPU6050* self) {
    __NOP();
    return;
}
static void I2CDefaultFinishCallBack(MPU6050* self) {
    self->Stage = MPU6050_IDLE;
    self->action = MPU6050_NOTHINGTODO;
    MPU6050_Callback(self);
};

static MPU6050_RET MPU6050_Read_Data(MPU6050* self, ACCELERATION_DATA_t* data) {
    if (self->Stage == MPU6050_I2CBUSY) {self->OnBusy(self); return MPU6050_BUSY;}
    self->Stage = MPU6050_I2CBUSY;
    self->readI2C(0x3b, (uint8_t*)data,14);
    return MPU6050_OK;
}

static MPU6050_RET MPU6050_Write_Data(MPU6050* self, uint8_t internalReg, uint8_t* data,uint8_t lenInBytes) {
    if (self->Stage == MPU6050_BUSY) {self->OnBusy(self); return MPU6050_BUSY;}
    self->Stage = MPU6050_I2CBUSY;
    self->writeI2C(internalReg,(uint8_t*)data,lenInBytes);
    return MPU6050_OK;
}


static MPU6050_RET MPU6050_Init(MPU6050* self,uint8_t I2Cadr7Bit,
                uint8_t(*readI2C)(uint8_t internalReg,uint8_t *data,uint8_t lenInBytes),
                uint8_t(*writeI2C)(uint8_t internalReg,uint8_t *data,uint8_t lenInBytes),
                void(*OnBusy)(struct MPU6050* self))
{
    self->I2Cadr = I2Cadr7Bit;
    self->Stage = MPU6050_IDLE;
    self->action = MPU6050_NOTHINGTODO;
    self->readI2C = readI2C;
    self->writeI2C = writeI2C;
    self->OnBusy = OnBusy;
    return MPU6050_Write_Data(self,0x6b, (uint8_t[]){0x01},1);
}



MPU6050_RET STM32_MPU6050_Binding(MPU6050* self, uint8_t I2Cadr7Bit) ;












#endif //REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H
