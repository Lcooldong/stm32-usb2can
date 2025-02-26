#include "can_mode.h"

bool canModeInit(void)
{
  return true;
}

void canModeMain(mode_args_t *args)
{
  logPrintf("canMode in\r\n");

#ifndef _USE_HW_RTOS
  uint32_t can_pre_time = millis();
#endif

  while (args->keepLoop())
  {

#ifdef _USE_HW_RTOS
    if(uartAvailable(_DEF_UART1) > 0)
    {
      uartPrintf(_DEF_UART1, "RX : 0x%X\r\n", uartRead(_DEF_UART1));
    }
    else
    {
      delay(1);
    }

#else
    if(millis() - can_pre_time >= 1)
    {
      can_pre_time = millis();
    }
    else if(uartAvailable(_DEF_UART1) > 0)
    {
      uartPrintf(_DEF_UART1, "RX : 0x%X\r\n", uartRead(_DEF_UART1));
    }
#endif

  }
  
  logPrintf("canMode out\r\n");
}