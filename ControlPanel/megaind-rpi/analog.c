#include "analog.h"

int val16Set(int dev, int baseAdd, int ch, float scale, float val)
{
    u8 buff[2] = {0,0};
    //u16 raw = 0;
    s16 raw = 0;
    u8 add = 0;

    if (ch < CHANNEL_NR_MIN) {
        return ERROR;
    }

    if ( (baseAdd + 2 * (ch - 1)) >= I2C_MEM_SIZE) {
        return ERROR;
    }

    add = baseAdd + 2 * (ch - 1);
    raw = (s16)ceil(val * scale); //transform to milivolts
    memcpy(buff, &raw, 2);
    if (OK != i2cMem8Write(dev, add, buff, 2)) {
        printf("Fail to write!\n");
        return ERROR;
    }

    return OK;
}

int analogOutVoltageWrite(int dev, int ch, float volt)
{
    if ( (ch < CHANNEL_NR_MIN) || (ch > U_OUT_CH_NR_MAX)) {
        printf("0-10V Output channel out of range!\n");
        return ERROR;
    }

    if (volt < -10 || volt > 10) {
        printf("Invalid voltage value, must be -10..10 \n");
        return ERROR;
    }

    if ( OK != val16Set(dev, I2C_MEM_U0_10_OUT_VAL1, ch, VOLT_TO_MILIVOLT, volt)) {
        return ERROR;
    }

    return OK;
}
