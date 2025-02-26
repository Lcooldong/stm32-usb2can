#ifndef __LOG_H__
#define __LOG_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "hw_def.h"


#ifdef _USE_HW_LOG

// RAM 저장 -> 전원 없으면 사라짐
#define LOG_CH            HW_LOG_CH
#define LOG_BOOT_BUF_MAX  HW_LOG_BOOT_BUF_MAX   // 부팅 Log 저장
#define LOG_LIST_BUF_MAX  HW_LOG_LIST_BUF_MAX   // LogPrint 저장


bool logInit(void);
void logEnable(void);
void logDisable(void);
bool logOpen(uint8_t ch, uint32_t baud);
void logBoot(uint8_t enable);
void logPrintf(const char *fmt, ...);

#endif

#ifdef __cplusplus
}
#endif



#endif