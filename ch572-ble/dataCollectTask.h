//
// Created by dxxdx on 2026/7/1.
//

#ifndef CH572_BLE_PROJECT_DATACOLLECTTASK_H
#define CH572_BLE_PROJECT_DATACOLLECTTASK_H
#include <stdint.h>
#include "MPU6050.h"

#define DATACOLLECT_TASK_EVT        0x0001
#define DATACOLLECT_DEFAULT_MS      20
#define DATACOLLECT_PACKET_LEN      16

typedef struct
{
    uint8_t data[MPU6050_DATA_LEN];
    uint8_t counter;
    uint8_t parity;
} MPU6050Packet;


//懒癌犯了，其他外设改void*指针，要么自己重写


typedef struct
{
    uint8_t  taskID;
    uint8_t  enabled;
    uint16_t event;

    uint16_t periodTicks;
    uint16_t sampleCount;

    uint16_t errorCount;
    uint8_t  payloadLen;

    MPU6050 *sensor;
    uint8_t *payload;


} dataCollectTask;

extern dataCollectTask g_dataCollectTask;
extern uint8_t g_dataCollectPayload[DATACOLLECT_PACKET_LEN];

uint8_t dataCollectTask_init(dataCollectTask *self );
void dataCollectTask_start(dataCollectTask *self);
void dataCollectTask_stop(dataCollectTask *self);
uint16_t dataCollectTask_ProcessEvent(uint8_t task_id, uint16_t events);


#endif //CH572_BLE_PROJECT_DATACOLLECTTASK_H
