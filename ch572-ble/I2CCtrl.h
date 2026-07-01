//
// Created by dxxdx on 2026/6/17.
//

#ifndef CH572_BLE_PROJECT_I2CCTRL_H
#define CH572_BLE_PROJECT_I2CCTRL_H
#include <stdint.h>

typedef enum
{
    I2C_HIZ = 1, // 高阻态（线由上拉电阻拉高）
    I2C_PD  = 0  // 强下拉（低电平）
} GPIO_STATE;

typedef struct I2C_Operation
{
    uint8_t TargetAddress7Plus1Bit; // 包含读写位的8位地址（写=0xD0, 读=0xD1）
    uint8_t HostRegisterAddress;    // MPU6050 寄存器地址
    uint8_t ReadBufferSize;
    uint8_t WriteBufferSize;
    uint8_t* ReadBuffer;
    uint8_t* WriteBuffer;
} I2C_Operation;

typedef struct I2CCtrl
{
    void(*SDAChange)(uint8_t status);
    void(*SCLChange)(uint8_t status);
    void(*I2CDelay)(void); // 👈 400kHz 半周期延时 (~1.2us)
    uint8_t(*SDARead)(void);
} I2CIO;

static uint8_t I2C_Process(I2CIO* io, I2C_Operation* operation)
{
    uint8_t ack_errors = 0; // 记录发生 NACK (未应答) 的总次数

    /* ==================== 1. START 信号 ==================== */
    io->SDAChange(I2C_HIZ);
    io->SCLChange(I2C_HIZ);
    io->I2CDelay();
    io->SDAChange(I2C_PD);   // SCL为高，SDA拉低 -> 起始
    io->I2CDelay();
    io->SCLChange(I2C_PD);   // 钳住总线，准备发数据

    /* ==================== 2. 发送设备写地址 (强制最低位为0) ==================== */
    uint8_t write_addr = operation->TargetAddress7Plus1Bit & 0xFE;
    for( int8_t i = 7; i >= 0; i--) // MSB 倒序发送
    {
        io->SDAChange((write_addr >> i) & 0x01);
        io->I2CDelay();
        io->SCLChange(I2C_HIZ);
        io->I2CDelay();
        io->SCLChange(I2C_PD);
    }

    // 读取地址周期的 ACK
    io->SDAChange(I2C_HIZ); // 主机放手，开漏准备读
    io->I2CDelay();
    io->SCLChange(I2C_HIZ);
    io->I2CDelay();
    if(io->SDARead()) ack_errors++; // 如果读到1，说明从机没理你，累加错误
    io->SCLChange(I2C_PD);
    io->I2CDelay();

    /* ==================== 3. 发送 MPU6050 寄存器内部地址 ==================== */
    for( int8_t i = 7; i >= 0; i--)
    {
        io->SDAChange((operation->HostRegisterAddress >> i) & 0x01);
        io->I2CDelay();
        io->SCLChange(I2C_HIZ);
        io->I2CDelay();
        io->SCLChange(I2C_PD);
    }

    // 读取寄存器周期的 ACK
    io->SDAChange(I2C_HIZ);
    io->I2CDelay();
    io->SCLChange(I2C_HIZ);
    io->I2CDelay();
    if(io->SDARead()) ack_errors++;
    io->SCLChange(I2C_PD);
    io->I2CDelay();

    /* ==================== 4. 判断读/写分支 ==================== */
    if(operation->TargetAddress7Plus1Bit & 0x01)
    {
        /* -------------------- 读分支 (READ) -------------------- */
        // 根据 I2C 标准，指定好寄存器地址后，需要插入一个 Re-Start 信号切换到读
        io->SDAChange(I2C_HIZ);
        io->SCLChange(I2C_HIZ);
        io->I2CDelay();
        io->SDAChange(I2C_PD);   // 重复起始条件
        io->I2CDelay();
        io->SCLChange(I2C_PD);

        // 发送设备的真实读地址 (最低位为1)
        for(int8_t i = 7; i >= 0; i--)
        {
            io->SDAChange((operation->TargetAddress7Plus1Bit >> i) & 0x01);
            io->I2CDelay();
            io->SCLChange(I2C_HIZ);
            io->I2CDelay();
            io->SCLChange(I2C_PD);
        }

        // 读取发送读地址后的从机 ACK
        io->SDAChange(I2C_HIZ);
        io->I2CDelay();
        io->SCLChange(I2C_HIZ);
        io->I2CDelay();
        if(io->SDARead()) ack_errors++;
        io->SCLChange(I2C_PD);
        io->I2CDelay();

        // 接收数据包
        for( uint8_t b = 0; b < operation->ReadBufferSize; b++)
        {
            uint8_t received_byte = 0;
            io->SDAChange(I2C_HIZ); // 确保主机完全释放总线，由从机输出

            for(int8_t i = 7; i >= 0; i--)
            {
                io->I2CDelay();
                io->SCLChange(I2C_HIZ);
                io->I2CDelay();
                if(io->SDARead())
                {
                    received_byte |= (1 << i);
                }
                io->SCLChange(I2C_PD);
            }
            operation->ReadBuffer[b] = received_byte;

            // 主机给出应答：如果读到最后一个字节发 NACK(HIZ) 结束接收，否则发 ACK(PD) 催促从机继续发
            uint8_t host_ack = (b == (operation->ReadBufferSize - 1)) ? I2C_HIZ : I2C_PD;
            io->SDAChange(host_ack);
            io->I2CDelay();
            io->SCLChange(I2C_HIZ);
            io->I2CDelay();
            io->SCLChange(I2C_PD);
            io->I2CDelay();
        }
    }
    else
    {
        /* -------------------- 写分支 (WRITE) -------------------- */
        for( uint8_t b = 0; b < operation->WriteBufferSize; b++)
        {
            for( int8_t i = 7; i >= 0; i--)
            {
                io->SDAChange((operation->WriteBuffer[b] >> i) & 0x01);
                io->I2CDelay();
                io->SCLChange(I2C_HIZ);
                io->I2CDelay();
                io->SCLChange(I2C_PD);
            }

            // 每写完一个字节，读取一次从机 ACK
            io->SDAChange(I2C_HIZ);
            io->I2CDelay();
            io->SCLChange(I2C_HIZ);
            io->I2CDelay();
            if(io->SDARead()) ack_errors++;
            io->SCLChange(I2C_PD);
            io->I2CDelay();
        }
    }

    /* ==================== 5. STOP 信号 ==================== */
    io->SDAChange(I2C_PD);
    io->I2CDelay();
    io->SCLChange(I2C_HIZ);
    io->I2CDelay();
    io->SDAChange(I2C_HIZ);  // SCL为高，SDA拉高 -> 停止
    io->I2CDelay();

    return ack_errors; // 👈 返回全流程握手失败的次数。如果是0则表示大获全胜！
}





#endif //CH572_BLE_PROJECT_I2CCTRL_H