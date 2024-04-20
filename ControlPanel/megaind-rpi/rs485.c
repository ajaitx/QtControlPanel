#include "rs485.h"

int rs485Set(int dev, u8 mode, u32 baud, u8 stopB, u8 parity, u8 add)
{
    ModbusSetingsType settings;
    u8 buff[5];

    if (baud > 920600 || baud < 1200) {
        if(mode == 0) {
            baud = 9600;
        } else {
            printf("Invalid RS485 Baudrate [1200, 920600]!\n");
            return ERROR;
        }
    }

    if (mode > 1) {
        printf("Invalid RS485 mode : 0 = disable, 1= Modbus RTU (Slave)!\n");
        return ERROR;
    }

    if (stopB < 1 || stopB > 2) {
        if(mode == 0 ) {
            stopB = 1;
        } else {
            printf("Invalid RS485 stop bits [1, 2]!\n");
            return ERROR;
        }
    }

    if (parity > 2) {
        printf("Invalid RS485 parity 0 = none; 1 = even; 2 = odd! Set to none.\n");
        parity = 0;
    }

    if (add < 1) {
        printf("Invalid MODBUS device address: [1, 255]! Set to 1.\n");
        add = 1;
    }

    settings.mbBaud = baud;
    settings.mbType = mode;
    settings.mbParity = parity;
    settings.mbStopB = stopB;
    settings.add = add;

    memcpy(buff, &settings, sizeof(ModbusSetingsType));
    if (OK != i2cMem8Write(dev, I2C_MODBUS_SETINGS_ADD, buff, 5)) {
        printf("Fail to write RS485 settings!\n");
        return ERROR;
    }

    return OK;
}


