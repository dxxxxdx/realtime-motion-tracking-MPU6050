#include "dataTxTask.h"
#include "CH57x_common.h"
#include "CH572BLEPeri_LIB.h"
#include "gattprofile.h"
#include "peripheral.h"

#define MS_TO_TICKS(ms)    ((ms) * 8 / 5)

static dataTxTask *s_dataTxTask = nullptr;

static void dataTxTask_notifyChar4(uint8_t *payload, uint8_t payloadLen)
{
    uint16_t conn;
    attHandleValueNoti_t noti;
    uint8_t sendLen;


    if(payload == NULL || payloadLen == 0)
    {
        return;
    }

    sendLen = payloadLen;
    if(sendLen > SIMPLEPROFILE_CHAR4_LEN)
    {
        sendLen = SIMPLEPROFILE_CHAR4_LEN;
    }

    conn = Peripheral_GetConnHandle();
    if(conn == GAP_CONNHANDLE_INIT)
    {
        return;
    }

    noti.len = sendLen;
    noti.pValue = GATT_bm_alloc(conn, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
    if(noti.pValue == NULL)
    {
        return;
    }

    tmos_memcpy(noti.pValue, payload, noti.len);
    if(simpleProfile_Notify(conn, &noti) != SUCCESS)
    {
        GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
    }
}

static void dataTxTask_send(dataTxTask *self)
{
    uint8_t sendLen;

    if(self == NULL || self->payload == NULL || self->payloadLen == 0)
    {
        return;
    }

    sendLen = self->payloadLen;

    if(self->gattCharID == SIMPLEPROFILE_CHAR4)
    {
        dataTxTask_notifyChar4(self->payload, sendLen);
        return;
    }

    SimpleProfile_SetParameter(self->gattCharID, sendLen, self->payload);
}

uint8_t dataTxTask_init(dataTxTask *self, uint16_t event, uint8_t *payload,
    uint8_t payloadLen, uint16_t periodMs,uint8_t channel)
{
    if(self == NULL || payload == NULL || payloadLen == 0)
    {
        return 0;
    }

    if(s_dataTxTask != NULL && s_dataTxTask != self)
    {
        return 0;
    }

    tmos_memset(self, 0, sizeof(dataTxTask));
    self->event = event;
    self->periodTicks = MS_TO_TICKS(periodMs ? periodMs : DATATX_DEFAULT_MS);
    self->gattCharID = channel;
    self->payload = payload;
    self->payloadLen = payloadLen;
    self->taskID = TMOS_ProcessEventRegister(dataTxTask_ProcessEvent);

    s_dataTxTask = self;
    return 1;
}

void dataTxTask_start(dataTxTask *self)
{
    if(self == NULL)
    {
        return;
    }

    self->enabled = 1;
    tmos_set_event(self->taskID, self->event);
}

void dataTxTask_stop(dataTxTask *self)
{
    if(self == NULL)
    {
        return;
    }

    self->enabled = 0;
    tmos_clear_event(self->taskID, self->event);
}

uint16_t dataTxTask_ProcessEvent(uint8_t task_id, uint16_t events)
{
    dataTxTask *self = s_dataTxTask;

    if(self == NULL || self->taskID != task_id)
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
            self->packetCount++;
            dataTxTask_send(self);
            tmos_start_task(self->taskID, self->event, self->periodTicks);
        }
        return (events ^ self->event);
    }

    return 0;
}
