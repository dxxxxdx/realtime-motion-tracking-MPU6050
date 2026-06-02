#ifndef DATATASK_H
#define DATATASK_H
#include <stdint.h>

// OOP 核心类
//这玩意只会每20ms把pdata扔到服务器的值,一次16字节  SIMPLEPROFILE_CHAR1_LEN  自己搜这个
typedef struct {
    uint8_t         taskID;       // TMOS 工牌号
    uint8_t         taskEnabled;  // 运行状态
    uint16_t        TMOS_Bitmask; // 专属事件掩码
    uint8_t         gattCharID;   // 【OOP拓展】专属蓝牙发送通道 (例如 SIMPLEPROFILE_CHAR1)
    uint8_t         reserved;
    uint16_t        packetCount;
    uint8_t*        pdata;

} dataTask_FSM;

// --- 类方法 ---
// 改为 return uint8_t： 1表示成功入职，0表示抢工牌被打死
uint8_t dataTask_init(dataTask_FSM* self, uint16_t TMOS_bitmask, uint8_t gatt_char_id,uint8_t* pdata);
void dataTask_start(dataTask_FSM* self);
void dataTask_stop(dataTask_FSM* self);
uint16_t dataTask_ProcessEvent(uint8_t task_id, uint16_t events);

#endif // DATATASK_H