#ifndef __BSP_H__
#define __BSP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "def.h"
#include "stm32g4xx_hal.h"
#include "cmsis_os.h"




bool bspInit(void);

void delay(uint32_t time_ms);
uint32_t millis(void);
uint32_t micros(void);

void Error_Handler(void);

void logPrintf(const char *fmt, ...);


#ifdef __cplusplus
}
#endif


#endif