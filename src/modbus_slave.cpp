#include <cstring>

#include "libxlog/xlog.h"
#include "modbus_slave.h"

ModbusSlave &ModbusSlave::GetInstance()
{
    static ModbusSlave instance;
    return instance;
}

ModbusSlave::ModbusSlave()
    : modbusContext(nullptr),
      mapping(nullptr),
      listenThread(nullptr),
      recieveThread(nullptr),
      listenStatus(false),
      stopStatus(false),
      listenSocket(-1),
      registersNumber(0),
      maxSocketContextValue(-1)
{
    LoadDefaultAddressPlan();
    FD_ZERO(&socketContextSet);
}

ModbusSlave::~ModbusSlave()
{
    if (modbusContext != nullptr)
    {
        modbus_close(modbusContext);
        modbus_free(modbusContext);
        modbusContext = nullptr;
    }
    if (mapping != nullptr)
    {
        modbus_mapping_free(mapping);
        mapping = nullptr;
    }
    if (listenThread != nullptr)
    {
        delete listenThread;
        listenThread = nullptr;
    }
    if (recieveThread != nullptr)
    {
        delete recieveThread;
        recieveThread = nullptr;
    }
    if (listenSocket <= 0)
    {
        close(listenSocket);
        listenSocket = -1;
    }
}

void ModbusSlave::StartCommunicate(std::string IP, int port)
{
    XLog::Info("Start communication");
    modbusContext = modbus_new_tcp(IP.c_str(), port);
    if (modbusContext == nullptr)
    {
        XLog::Error("MODBUS context creation failed, check the parameters");
        return;
    }
    else
    {
        XLog::Info("IP: %s, Port: %d", IP.c_str(), port);
    }

    modbus_set_slave(modbusContext, 1);
    listenSocket = modbus_tcp_listen(modbusContext, 1);
    listenStatus = (listenSocket != -1);

    XLog::Info("The listening socket is established, Listen for status: %d", listenStatus);
    modbus_set_indication_timeout(modbusContext, 1, 0);
    listenThread = new std::thread(&ModbusSlave::StartListen, this);
    if (listenStatus == true)
        recieveThread = new std::thread(&ModbusSlave::StartReceive, this);
}

void ModbusSlave::StopCommunicate()
{
    XLog::Info("Communication is stopping......");

    stopStatus = true;

    listenThread->join();
    XLog::Info("The listening thread has stopped");
    recieveThread->join();
    XLog::Info("The recieving thread has stopped");

    if (modbusContext != nullptr)
    {
        modbus_close(modbusContext);
        modbus_free(modbusContext);
        modbusContext = nullptr;
    }
    if (mapping != nullptr)
    {
        modbus_mapping_free(mapping);
        mapping = nullptr;
    }
    if (listenThread != nullptr)
    {
        delete listenThread;
        listenThread = nullptr;
    }
    if (recieveThread != nullptr)
    {
        delete recieveThread;
        recieveThread = nullptr;
    }
    if (listenSocket > 0)
    {
        close(listenSocket);
        listenSocket = -1;
    }
    listenStatus = false;
    stopStatus = false;
    maxSocketContextValue = -1;
    XLog::Info("Communication has stopped");
}

bool ModbusSlave::GetData(uint16_t address, int number, uint16_t *data)
{
    if (address < 0 || address + number - 1 > registersNumber)
    {
        XLog::Error("PARAMETER ERROR: The [address] is out of range! The current error address value is %d", address);
        return false;
    }

    *data = mapping->tab_registers[address];

    return true;
}

bool ModbusSlave::SetData(uint16_t address, int number, uint16_t *data)
{
    if (address < 0 || address + number - 1 > registersNumber)
    {
        XLog::Error("PARAMETER ERROR: The [address] is out of range! The current error address value is %d", address);
        return false;
    }

    uint16_t newValue = *data;

    mapping->tab_registers[address] = newValue;
    return true;
}

/* ------------------------------------------- */

void ModbusSlave::StartListen()
{
    while (stopStatus == false)
    {
        if (listenStatus == false)
        {
            close(listenSocket);
            listenSocket = modbus_tcp_listen(modbusContext, 1);
            listenStatus = (listenSocket != -1);
            if (listenStatus == false)
                continue;
            // 如果监听重新开启成功
            if (recieveThread != nullptr)
            {
                recieveThread->join();
                delete recieveThread;
                recieveThread = nullptr;
                FD_CLR(listenSocket, &socketContextSet);
                if (listenSocket == maxSocketContextValue)
                {
                    --maxSocketContextValue;
                }
                recieveThread = new std::thread(&ModbusSlave::StartReceive, this);
            }
            XLog::Info("The listening socket is being re-established, Listen for status: %d", listenStatus);
            continue;
        }
        listenStatus = IsSocketListening();
        if (listenStatus == true)
            continue;
        // 如果检测到并不处在监听状态
        if (recieveThread != nullptr)
        {
            recieveThread->join();
            delete recieveThread;
            recieveThread = nullptr;
            FD_CLR(listenSocket, &socketContextSet);
            if (listenSocket == maxSocketContextValue)
            {
                --maxSocketContextValue;
            }
            recieveThread = new std::thread(&ModbusSlave::StartReceive, this);
        }
        XLog::Info("Listen for interrupts, Listen for status: %d", listenStatus);
    }
}

void ModbusSlave::StartReceive()
{
    uint8_t request[MODBUS_TCP_MAX_ADU_LENGTH];
    fd_set temporarySet;
    FD_SET(listenSocket, &socketContextSet);
    if (listenSocket > maxSocketContextValue)
        maxSocketContextValue = listenSocket;
    timeval selectTimeout;
    selectTimeout.tv_sec = 1;
    selectTimeout.tv_usec = 0;
    while (stopStatus == false && listenStatus == true)
    {
        temporarySet = socketContextSet;
        if (select(maxSocketContextValue + 1, &temporarySet, NULL, NULL, &selectTimeout) == -1)
        {
            XLog::Error("Server select() failure!");
            break;
        }
        AcceptOrReceive(request, temporarySet);
    }
}

bool ModbusSlave::IsSocketListening()
{
    bool opt_value;
    socklen_t opt_length = sizeof(opt_value);
    getsockopt(listenSocket, SOL_SOCKET, SO_ACCEPTCONN, &opt_value, &opt_length);
    return opt_value;
}

inline void ModbusSlave::LoadDefaultAddressPlan()
{
    mapping = modbus_mapping_new(0, 0, 50000, 0);
    registersNumber = 50000;
}

void ModbusSlave::AcceptOrReceive(uint8_t request[MODBUS_TCP_MAX_ADU_LENGTH], fd_set &contextSet)
{
    for (socket socketContext = 0; socketContext <= maxSocketContextValue; ++socketContext)
    {
        if (!FD_ISSET(socketContext, &contextSet))
            continue;

        if (socketContext == listenSocket)
        {
            socklen_t addrlen;
            struct sockaddr_in clientaddr;
            int newfd;

            // 处理新的连接请求
            addrlen = sizeof(clientaddr);
            memset(&clientaddr, 0, sizeof(clientaddr));
            newfd = accept(listenSocket, (struct sockaddr *)&clientaddr, &addrlen);
            if (newfd == -1)
                XLog::Error("Server accept() error!");
            else
            {
                FD_SET(newfd, &socketContextSet);

                if (newfd > maxSocketContextValue)
                    maxSocketContextValue = newfd;
                XLog::Info("New connection from %s:%d on socket %d\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd);
            }
            continue;
        }
        modbus_set_socket(modbusContext, socketContext);
        int receiveResult;
        receiveResult = modbus_receive(modbusContext, request);
        if (receiveResult > 0)
        {
            modbus_reply(modbusContext, request, receiveResult, mapping);
        }
        else if (receiveResult == -1)
        {
            // 当连接关闭时，receiveResult也为-1
            XLog::Info("Connection closed on socket %d\n", socketContext);
            close(socketContext);
            FD_CLR(socketContext, &socketContextSet);
            if (socketContext == maxSocketContextValue)
            {
                --maxSocketContextValue;
            }
        }
    }
}
