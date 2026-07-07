#include "dataTxTask.h"
#include "CH57x_common.h"
#include "CH572BLEPeri_LIB.h"
#include "dataCollectTask.h"
#include "gattprofile.h"
#include "peripheral.h"

#define MS_TO_TICKS(ms)    ((ms) * 8 / 5)

dataTxTask g_dataTxTask = {
    .taskID = 0,
    .enabled = 0,
    .event = DATATX_TASK_EVT,
    .periodTicks = MS_TO_TICKS(DATATX_DEFAULT_MS),
    .packetCount = 0,
    .gattCharID = SIMPLEPROFILE_CHAR4,
    .payload = g_dataCollectPayload,
    .payloadLen = DATACOLLECT_PACKET_LEN,
};

static dataTxTask *s_dataTxTask = &g_dataTxTask;

static void dataTxTask_notifyChar4(uint8_t *payload, uint8_t payloadLen)
{
    uint16_t conn;
    attHandleValueNoti_t noti;
    uint8_t sendLen;

    sendLen = payloadLen;
    if(sendLen > SIMPLEPROFILE_CHAR4_LEN)
    {
        sendLen = SIMPLEPROFILE_CHAR4_LEN;
    }

    conn = Peripheral_GetConnHandle();

    noti.len = sendLen;
    noti.pValue = GATT_bm_alloc(conn, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
    tmos_memcpy(noti.pValue, payload, noti.len);
    if(simpleProfile_Notify(conn, &noti) != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
    }
}

static void dataTxTask_send(dataTxTask *self)
{
    uint8_t sendLen;

    sendLen = self->payloadLen;

    if(self->gattCharID == SIMPLEPROFILE_CHAR4)
    {
        dataTxTask_notifyChar4(self->payload, sendLen);
        return;
    }

    SimpleProfile_SetParameter(self->gattCharID, sendLen, self->payload);
}

uint8_t dataTxTask_init(dataTxTask *self)
{
    self->enabled = 0;
    self->packetCount = 0;
    self->taskID = TMOS_ProcessEventRegister(dataTxTask_ProcessEvent);
    s_dataTxTask = self;
    return 1;
}

void dataTxTask_start(dataTxTask *self)
{

    self->enabled = 1;
    tmos_set_event(self->taskID, self->event);
}

void dataTxTask_stop(dataTxTask *self)
{

    self->enabled = 0;
    tmos_clear_event(self->taskID, self->event);
}

uint16_t dataTxTask_ProcessEvent(uint8_t task_id, uint16_t events)
{
    dataTxTask *self = s_dataTxTask;

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
            self->packetCount++;
            dataTxTask_send(self);
            tmos_start_task(self->taskID, self->event, self->periodTicks);
        }
        return (events ^ self->event);
    }

    return 0;
}
