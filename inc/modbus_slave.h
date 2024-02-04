#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>

#include "libmodbus/modbus.h"

class ModbusSlave
{
    using socket = int;

public:
    static ModbusSlave &GetInstance();
    void StartCommunicate(std::string IP, int port);
    void StopCommunicate();
    bool GetData(uint16_t addr, int number, uint16_t *data); // 获取数据
    bool SetData(uint16_t addr, int offset, uint16_t *data); // 设置数据

private:
    ModbusSlave();
    ~ModbusSlave();
    void StartListen();
    void StartReceive();
    bool IsSocketListening();
    void LoadDefaultAddressPlan();
    void AcceptOrReceive(uint8_t request[MODBUS_TCP_MAX_ADU_LENGTH], fd_set &contextSet);

    modbus_t *modbusContext;   // modbus上下文
    modbus_mapping_t *mapping; // modbus映射
    socket listenSocket;       // 监听套接字
    fd_set socketContextSet;   // 套接字上下文集合，数量限制为1024

    std::thread *listenThread;  // 监听线程
    std::thread *recieveThread; // 回复线程（回复TCP连接请求与MODBUS请求）

    int maxSocketContextValue;
    int registersNumber; // modbus最大地址
    bool listenStatus;   // 监听状态
    bool stopStatus;     // 通信状态
};
