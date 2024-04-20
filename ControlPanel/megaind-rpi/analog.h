#ifndef ANALOG_H
#define ANALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "megaind.h"
#include "comm.h"

int analogOutVoltageWrite(int dev, int ch, float volt);

#endif // ANALOG_H
