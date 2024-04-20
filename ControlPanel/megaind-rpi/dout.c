#include "dout.h"

int openDrainSet(int dev, int ch)
{
    u8 buff[2];

    if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX)) {
        printf("Open-drain Output channel out of range!\n");
        return ERROR;
    }

    if (OK != i2cMem8Write(dev, I2C_MEM_RELAY_SET, buff, 1)) {
        return COMM_ERR;
    }

    return OK;
}
