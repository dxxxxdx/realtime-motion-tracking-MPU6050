#ifndef CH572_BLE_PROJECT_DATARXTASK_H
#define CH572_BLE_PROJECT_DATARXTASK_H

#include <stdint.h>

#define DATARX_TASK_EVT        0x0002
#define DATARX_DEFAULT_MS      200
#define DATARX_PACKET_LEN      8


typedef struct dataRxTask
{
    uint8_t            taskID;
    uint8_t            enabled;
    uint16_t           event;

    uint16_t           periodTicks;
    uint16_t           packetCount;

    uint8_t           *payload;

    uint8_t            gattCharID;
    uint8_t            echoBackGattCharID;
    uint8_t            payloadLen;
    void   (*onReceive)(struct dataRxTask* self);
} dataRxTask;

extern dataRxTask g_dataRxTask;
extern uint8_t g_dataRxPayload[DATARX_PACKET_LEN];

uint8_t dataRxTask_init(dataRxTask *self);
void dataRxTask_start(dataRxTask *self);
void dataRxTask_stop(dataRxTask *self);
uint16_t dataRxTask_ProcessEvent(uint8_t task_id, uint16_t events);
void onReceive(dataRxTask *self);
#endif //CH572_BLE_PROJECT_DATARXTASK_H
