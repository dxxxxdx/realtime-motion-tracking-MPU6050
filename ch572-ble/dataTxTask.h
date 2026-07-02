#ifndef DATATXTASK_H
#define DATATXTASK_H

#include <stdint.h>

#define DATATX_TASK_EVT        0x0001
#define DATATX_DEFAULT_MS      15

typedef struct
{
    uint8_t  taskID;
    uint8_t  enabled;
    uint16_t event;
    uint16_t periodTicks;
    uint16_t packetCount;
    uint8_t  gattCharID;
    uint8_t *payload;
    uint8_t  payloadLen;
} dataTxTask;

uint8_t dataTxTask_init(dataTxTask *self, uint16_t event, uint8_t *payload, uint8_t payloadLen, uint16_t periodMs,uint8_t channel);
void dataTxTask_start(dataTxTask *self);
void dataTxTask_stop(dataTxTask *self);
uint16_t dataTxTask_ProcessEvent(uint8_t task_id, uint16_t events);

#endif
