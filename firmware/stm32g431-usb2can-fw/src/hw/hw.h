#ifndef __HW_H__
#define __HW_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#include "led.h"
#include "rtc.h"
#include "reset.h"
#include "cdc.h"
#include "usb.h"
#include "uart.h"
#include "cli.h"
#include "can.h"
#include "i2c.h"
#include "log.h"
#include "spi.h"
#include "button.h"

bool hwInit(void);

#ifdef __cplusplus
}
#endif

#endif
