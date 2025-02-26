#include "cli_mode.h"

bool cliModeInit(void)
{
  return true;
}

void cliModeMain(mode_args_t *args)
{
  logPrintf("cliMode in\r\n");
#ifndef _USE_HW_RTOS
  uint32_t cli_pre_time = millis();
#endif
  while(args->keepLoop())
  {

#ifdef _USE_HW_RTOS
    cliMain();
    delay(2);
#else
    if(millis() - cli_pre_time >= 2)
    {
      cli_pre_time = millis();
      cliMain();
    }
    else
    {

    }
#endif    
    
  }

  logPrintf("cliMode out\r\n");

}