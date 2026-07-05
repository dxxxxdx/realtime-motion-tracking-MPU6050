//
// Created by my on 2026/5/28.
//

#ifndef REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H
#define REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H
#include <stdint.h>

#define MPU6050_DATA_REG 0x3b
#define MPU6050_DATA_LEN 14

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
    void(*I2CFinishCallBack)(struct MPU6050* self);//给dma用的
    void(*OnBusy)(struct MPU6050* self);
}MPU6050;

typedef enum {
    MPU6050_OK = 0,
    MPU6050_ERROR,
    MPU6050_BUSY,

}MPU6050_RET;

__attribute__((weak)) void MPU6050_Callback(MPU6050* self);
static void I2CDefaultFinishCallBack(MPU6050* self) ;

MPU6050_RET MPU6050_Read_Data(MPU6050* self, ACCELERATION_DATA_t* data);

MPU6050_RET MPU6050_Write_Data(MPU6050* self, uint8_t internalReg, uint8_t* data,uint8_t lenInBytes);

MPU6050_RET MPU6050_Init(MPU6050* self);
extern MPU6050 g_mpu;








#endif //REALTIME_MOTION_TRACKING_MPU6050_MPU6050_H
