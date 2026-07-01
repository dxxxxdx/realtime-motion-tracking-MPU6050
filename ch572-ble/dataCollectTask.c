//
// Created by dxxdx on 2026/7/1.
//

#include "dataCollectTask.h"
#include "CH57x_common.h"
#include "CH572BLEPeri_LIB.h"

#define MS_TO_TICKS(ms)    ((ms) * 8 / 5)

uint8_t g_dataCollectPayload[DATACOLLECT_PACKET_LEN];

dataCollectTask g_dataCollectTask = {
    .taskID = 0,
    .enabled = 0,
    .event = DATACOLLECT_TASK_EVT,
    .periodTicks = MS_TO_TICKS(DATACOLLECT_DEFAULT_MS),
    .sampleCount = 0,
    .errorCount = 0,
    .sensor = &g_mpu,
    .payload = g_dataCollectPayload,
    .payloadLen = DATACOLLECT_PACKET_LEN,
};

static dataCollectTask *s_dataCollectTask = &g_dataCollectTask;

static uint8_t dataCollectTask_parity(uint8_t *data, uint8_t len)
{
    uint8_t parity = 0;
    uint8_t i;

    for(i = 0; i < len; i++)
    {
        parity ^= data[i];
    }

    return parity;
}

static void dataCollectTask_collect(dataCollectTask *self)
{
    ACCELERATION_DATA_t data;

    if(self == nullptr || self->sensor == nullptr || self->payload == nullptr)
    {
        return;
    }

    if(MPU6050_Read_Data(self->sensor, &data) == MPU6050_OK)
    {
        tmos_memcpy(self->payload, &data, MPU6050_DATA_LEN);
        self->payload[14] = (uint8_t)++self->sampleCount;
        self->payload[15] = dataCollectTask_parity(self->payload, 15);
        return;
    }
    PRINT("READERR");
    self->errorCount++;
}

uint8_t dataCollectTask_init(dataCollectTask *self )
{
    if(self->sensor == nullptr || self->payload == nullptr)
    {
        return 0;
    }
    self->enabled = 0;
    self->sampleCount = 0;
    self->errorCount = 0;
    self->taskID = TMOS_ProcessEventRegister(dataCollectTask_ProcessEvent);
    s_dataCollectTask = self;
    return 1;
}

void dataCollectTask_start(dataCollectTask *self)
{


    if(self == nullptr)
    {
        return;
    }

    self->enabled = 1;
    tmos_set_event(self->taskID, self->event);
}

void dataCollectTask_stop(dataCollectTask *self )
{


    if(self == nullptr)
    {
        return;
    }

    self->enabled = 0;
    tmos_clear_event(self->taskID, self->event);
}

uint16_t dataCollectTask_ProcessEvent(uint8_t task_id, uint16_t events)
{
    dataCollectTask *self = s_dataCollectTask;

    if(self == nullptr || self->taskID != task_id)
    {
        return 0;
    }

    if(events & SYS_EVENT_MSG)
    {
        uint8_t *msg = tmos_msg_receive(task_id);
        if(msg)
        {
            tmos_msg_deallocate(msg);
        }
        return (events ^ SYS_EVENT_MSG);
    }

    if(events & self->event)
    {
        if(self->enabled)
        {
            dataCollectTask_collect(self);
            tmos_start_task(self->taskID, self->event, self->periodTicks);
        }
        return (events ^ self->event);
    }

    return 0;
}
