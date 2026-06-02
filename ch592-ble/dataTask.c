#include "dataTask.h"
#include "CH59xBLE_LIB.h"
#include "CH59x_common.h"
#include "gattprofile.h"
#include "peripheral.h"
#include <string.h>

#define MS_TO_TICKS(ms)         ((ms) * 8 / 5)
#define DATATASK_PERIOD_TICKS   MS_TO_TICKS(20)



// 【OOP 多实例花名册】：支持最多创建 3 个独立的数据任务实例
#define MAX_INSTANCES 3
static dataTask_FSM* s_taskRegistry[MAX_INSTANCES] = {NULL};

static void dataTask_main(dataTask_FSM* self);

/**
 * @brief  构造函数 (防抢占版)
 * @return 1: 成功; 0: 失败/冲突
 */
uint8_t dataTask_init(dataTask_FSM* self, uint16_t TMOS_bitmask, uint8_t gatt_char_id,uint8_t* pdata)
{
    if (self == NULL) return 0; // 防御：野指针直接打死

    // 1. 查重防御：防止同一个对象被多次恶意初始化
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (s_taskRegistry[i] == self) {
            return 1; // 自己人，已经注册过了，直接放行
        }
    }

    // 2. 抢工位防御：在花名册里找个空位
    int freeSlot = -1;
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (s_taskRegistry[i] == NULL) {
            freeSlot = i;
            break;
        }
    }

    // 如果没有空位了（实例化过多），或者被别人抢光了，直接打死！
    if (freeSlot == -1) return 0;

    // 3. 正式初始化属性
    tmos_memset(self, 0, sizeof(dataTask_FSM));
    self->taskEnabled = 0;
    self->packetCount = 0;
    self->TMOS_Bitmask = TMOS_bitmask;
    self->gattCharID = gatt_char_id; // 绑定它专属的发射口
    self->pdata = pdata; // 绑定它专属的数据源
    // 4. 登记入册并领工牌
    s_taskRegistry[freeSlot] = self;
    self->taskID = TMOS_ProcessEventRegister(dataTask_ProcessEvent);

    return 1;
}

void dataTask_start(dataTask_FSM* self)
{
    if (self == NULL) return;
    if (!self->taskEnabled)
    {
        self->taskEnabled = 1;
        tmos_set_event(self->taskID, self->TMOS_Bitmask);
    }
}

void dataTask_stop(dataTask_FSM* self)
{
    if (self == NULL) return;
    self->taskEnabled = 0;
    tmos_clear_event(self->taskID, self->TMOS_Bitmask);
}

/**
 * @brief  TMOS 轮询回调 (负责将 task_id 桥接回真正的 self 对象)
 */
uint16_t dataTask_ProcessEvent(uint8_t task_id, uint16_t events)
{
    dataTask_FSM* self = NULL;

    // 【OOP 还原】：拿着 TMOS 给的工牌号，去花名册里找对应的人
    for (int i = 0; i < MAX_INSTANCES; i++) {
        if (s_taskRegistry[i] != NULL && s_taskRegistry[i]->taskID == task_id) {
            self = s_taskRegistry[i];
            break;
        }
    }

    // 查无此人！可能工牌被注销或传错，直接打死
    if (self == NULL) return 0;

    if (events & SYS_EVENT_MSG)
    {
        return (events ^ SYS_EVENT_MSG);
    }

    // 处理当前实例的专属事件
    if (events & self->TMOS_Bitmask)
    {
        if (self->taskEnabled)
        {
            dataTask_main(self); // 大家都调用这个函数，但传进去的 self 是各自独立的！
            tmos_start_task(self->taskID, self->TMOS_Bitmask, 10*DATATASK_PERIOD_TICKS);
        }
        return (events ^ self->TMOS_Bitmask);
    }

    return 0;
}

/**
 * @brief  实际干活的业务层
 */
static void dataTask_main(dataTask_FSM* self)
{
    // 终极实例判断：哪怕直接调这个函数，没有合法身份也得滚蛋
    if (self == NULL) return;
    self->packetCount++;

    // ==========================================
    // 蓝牙通道：发送 Hello World
    // ==========================================

    SimpleProfile_SetParameter(self->gattCharID, 16, self->pdata);


    //ai写的能跑就别动,其他通道是很简单的




    if (self->gattCharID == SIMPLEPROFILE_CHAR4)
    {
        uint16_t conn = Peripheral_GetConnHandle();
        if(conn != GAP_CONNHANDLE_INIT)
        {
            attHandleValueNoti_t noti;
            uint8_t sendLen = 16;
            if(sendLen > SIMPLEPROFILE_CHAR4_LEN) sendLen = SIMPLEPROFILE_CHAR4_LEN;
            noti.len = sendLen;
            noti.pValue = GATT_bm_alloc(conn, ATT_HANDLE_VALUE_NOTI, noti.len, NULL, 0);
            if(noti.pValue)
            {
                tmos_memcpy(noti.pValue, self->pdata, noti.len);
                if(simpleProfile_Notify(conn, &noti) != SUCCESS)
                {
                    /* free buffer if notify failed */
                    GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
                }
            }
        }
    }

}