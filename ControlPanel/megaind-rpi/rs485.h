#ifndef RS485_H
#define RS485_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "comm.h"
#include "megaind.h"

int rs485Set(int dev, u8 mode, u32 baud, u8 stopB, u8 parity, u8 add);

#endif // RS485_H
