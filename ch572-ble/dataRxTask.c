#include "dataRxTask.h"
#include "CH57x_common.h"
#include "CH572BLEPeri_LIB.h"
#include "gattprofile.h"

#define MS_TO_TICKS(ms)    ((ms) * 8 / 5)
#include "MPU6050.h"
uint8_t g_dataRxPayload[DATARX_PACKET_LEN];

dataRxTask g_dataRxTask = {
    .taskID = 0,
    .enabled = 0,
    .event = DATARX_TASK_EVT,
    .periodTicks = MS_TO_TICKS(DATARX_DEFAULT_MS),
    .packetCount = 0,
    .payload = g_dataRxPayload,
    .gattCharID = SIMPLEPROFILE_CHAR3,
    .payloadLen = DATARX_PACKET_LEN,
    .onReceive = onReceive,
    .echoBackGattCharID = SIMPLEPROFILE_CHAR2,
};



static dataRxTask *s_dataRxTask = &g_dataRxTask;

static uint8_t dataRxTask_hasData(const uint8_t *payload, uint8_t payloadLen)
{
    for(uint8_t i = 0; i < payloadLen; i++)
    {
        if(payload[i] != 0)
        {
            return 1;
        }
    }

    return 0;
}

static void dataRxTask_clear(dataRxTask *self)
{
    uint8_t zero[SIMPLEPROFILE_CHAR3_LEN] = {0};

    SimpleProfile_SetParameter(self->gattCharID, SIMPLEPROFILE_CHAR3_LEN, zero);
}

static void dataRxTask_receive(dataRxTask *self)
{

    if(SimpleProfile_GetParameter(self->gattCharID, self->payload) != SUCCESS)
    {
        return;
    }

    if(!dataRxTask_hasData(self->payload, self->payloadLen))
    {
        return;
    }

    self->packetCount++;

    self->onReceive(self);

    dataRxTask_clear(self);
}

uint8_t dataRxTask_init(dataRxTask *self)
{
    self->enabled = 0;
    self->packetCount = 0;
    self->onReceive = onReceive;
    self->taskID = TMOS_ProcessEventRegister(dataRxTask_ProcessEvent);
    s_dataRxTask = self;
    return 1;
}

void dataRxTask_start(dataRxTask *self)
{
    self->enabled = 1;
    tmos_set_event(self->taskID, self->event);
}

void dataRxTask_stop(dataRxTask *self)
{
    self->enabled = 0;
    tmos_clear_event(self->taskID, self->event);
}

uint16_t dataRxTask_ProcessEvent(uint8_t task_id, uint16_t events)
{
    dataRxTask *self = s_dataRxTask;

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
            dataRxTask_receive(self);
            tmos_start_task(self->taskID, self->event, self->periodTicks);
        }
        return (events ^ self->event);
    }

    return 0;
}
static uint8_t test[8] = {0};
void onReceive(dataRxTask *self)
{
    uint8_t error = 0;
    if (self->payload[0] == 0xAA){   //写寄存器
         error = MPU6050_Write_Data(&g_mpu,self->payload[1], self->payload+2, self->payload[7]);
        //别写太长，一个一个写
        //记得填满数据否则踩坏其他寄存器

    }else if (self->payload[0] == 0x55){
        //TODO 我觉得没必要读其他寄存器
        (void)self;


    }else{

    }
    test[7] = error;
    test[0] = self->packetCount;
    //TODO 上位机自己提需求往这里加，怎么回复自己写,校验自己写
    SimpleProfile_SetParameter(self->echoBackGattCharID, SIMPLEPROFILE_CHAR2_LEN, &test );















}
