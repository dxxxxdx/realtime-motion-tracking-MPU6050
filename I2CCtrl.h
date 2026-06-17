//
// Created by dxxdx on 2026/6/17.
//

#ifndef CH572_BLE_PROJECT_I2CCTRL_H
#define CH572_BLE_PROJECT_I2CCTRL_H
#include <stdint.h>

typedef enum
{
    I2C_HIZ,
    I2C_PD
}GPIO_STATE;

typedef struct I2C_Operation
{
    uint8_t TargetAddress7Bit;
    uint8_t HostRegisterAddress;
    uint8_t ReadBufferSize;
    uint8_t WriteBufferSize;
    uint8_t* ReadBuffer;
    uint8_t* WriteBuffer;

}I2C_Operation;


typedef struct I2CCtrl
{
    void(*SDAChange)(uint8_t status);
    void(*SCLChange)(uint8_t status);
    uint8_t(*SDAREAD)(void);
    I2C_Operation* I2C_operation;

}I2CCtrl;

















#endif //CH572_BLE_PROJECT_I2CCTRL_H
