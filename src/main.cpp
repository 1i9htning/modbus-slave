#include "libxlog/xlog.h"
#include "modbus_slave.h"

int main(int argc, char *argv[])
{
    XLog::Initialize("/home/li9htning/ModbusSlave_Log.txt");
    XLog::SetSizeLimit(10485760); // Unit: byte

    if (argc != 3)
    {
        XLog::Error("参数数量不足");
        return -1;
    }

    std::string IP = argv[1];
    int port = std::atoi(argv[2]);

    ModbusSlave::GetInstance().StartCommunicate(IP, port);

    while (true)
    {
        sleep(1);
        uint16_t data = 0;
        ModbusSlave::GetInstance().GetData(40001, 1, &data);
        uint16_t data_ = 0x0B;
        ModbusSlave::GetInstance().SetData(40002, 1, &data_);
        XLog::Info("40001: %h", data);
    }

    ModbusSlave::GetInstance().StopCommunicate();
}
