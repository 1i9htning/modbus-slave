⚡程序生成命令
g++ -o bin/ModbusSlave src/main.cpp src/modbus_slave.cpp -Iinc/libmodbus -Iinc/libxlog -Iinc -Llib -Llib/libmodbus -Llib/libxlog -lmodbus -lxlog -pthread