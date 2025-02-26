#include "ap.h"



typedef enum
{
  MODE_IDLE,
  MODE_CLI,
  MODE_CAN,

}ap_mode_t;

static ap_mode_t mode = MODE_IDLE;
static ap_mode_t mode_next = MODE_IDLE;
static mode_args_t mode_args;

static bool startFlag = true;

static bool apLoopIdle(void);
static void apLedUpdate(void);
static void apGetModeNext(ap_mode_t *p_mode_next);

static void ledThread(void const *argument);

void apInit(void)
{
  cliOpen(_DEF_UART1, 57600);

  // canOpen(_DEF_CAN1, CAN_NORMAL, CAN_CLASSIC, CAN_500K, CAN_2M);
  canOpen(_DEF_CAN1, CAN_NORMAL, CAN_FD_NO_BRS, CAN_500K, CAN_2M);
  i2cOpen(_DEF_I2C1, I2C_FREQ_100KHz);    // I2C 속도 조절

  cliModeInit();
  canModeInit();
  mode_args.keepLoop = apLoopIdle;  // Ide을 함수포인터로 받음 ( 호출가능 )

#ifdef _USE_HW_RTOS
  osThreadDef(ledThread, ledThread, _HW_DEF_RTOS_THREAD_PRI_LED, 0, _HW_DEF_RTOS_THREAD_MEM_LED);
  if(osThreadCreate(osThread(ledThread), NULL) == NULL)
  {
    logPrintf("ledThread Fail\r\n");
  }
#endif

}

void apMain(void)
{
  

  while (1)   
  {
    switch (mode)
    {
    case MODE_CLI:
      cliModeMain(&mode_args);  
      break;
      
    case MODE_CAN:
      canModeMain(&mode_args);
      break;

    default:
      apLoopIdle();
      break;
    }

  }
}

bool apLoopIdle(void)
{
  bool ret = true;    

#ifndef _USE_HW_RTOS
  apLedUpdate();
#endif

  apGetModeNext(&mode_next);
  if(mode != mode_next)
  {
    mode = mode_next;
    ret = false;

    ledOff(_DEF_LED1);
    ledOff(_DEF_LED2);
  }
  else if(startFlag)
  { 
    
    //delay(1000);
    //logPrintf("Start : Mode IDLE\r\n");
    startFlag = false;
    
  }

  return ret;   // mode change 여부
}



// 상태 확인용
void apLedUpdate(void)
{

  static int count = 0;
  static uint32_t led_pre_time[2] = {0,};
  // static uint32_t length = sizeof(pre_time)/sizeof(pre_time[0]);
  uint32_t led_blink_time;    

  switch (mode)
  {
  case MODE_CLI:
    led_blink_time = 500;
    break;

  case MODE_CAN:
    led_blink_time = 1000;
    break;
  
  default:
    led_blink_time = 3000; 
    break;
  }

  uint32_t cur_time = millis();
  if(cur_time - led_pre_time[0] >= led_blink_time)
  {
    led_pre_time[0] = cur_time;
    count++;
    ledToggle(_DEF_LED1);
    // logPrintf("[%d]:\r\n", count);
  }

  
  if (cur_time - led_pre_time[1] >= 500)
  {
    led_pre_time[1] = cur_time;
    //if (cdcIsConnect() == true)
    if (usbIsOpen() == true)
    {
      ledToggle(_DEF_LED2);
    }
    else
    {
      ledOff(_DEF_LED2);
    }
    
  }



}

// 현재 모드 확인
void apGetModeNext(ap_mode_t *p_mode_next)
{
  if(uartGetBaud(_DEF_UART1) == 1400)
  {
    *p_mode_next = MODE_CAN;
  }
  else
  {
    *p_mode_next = MODE_CLI;
  }

  if(usbIsOpen() != true)
  {
    *p_mode_next = MODE_IDLE;      
  }
}


void ledThread(void const *argument)
{
  (void)argument;

  while (1)
  {
    apLedUpdate();
    delay(10);
  }
  

}