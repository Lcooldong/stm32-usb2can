#include "main.h"

static void mainThread(void const *argument);



int main(void)
{
    bspInit();

#ifdef _USE_HW_RTOS
    osThreadDef(mainThread, mainThread, _HW_DEF_RTOS_THREAD_PRI_MAIN, 0, _HW_DEF_RTOS_THREAD_MEM_MAIN);  
    
    // FAILED
    if(osThreadCreate(osThread(mainThread), NULL) == NULL)
    {
        ledInit();
        while (1)
        {
            ledToggle(_DEF_LED1);
            delay(100);
        }
        
    }

    osKernelStart();
#else
    hwInit();
    apInit();
    apMain();
#endif

    return 0;
}


void mainThread(void const *argument)
{
    UNUSED(argument);

    hwInit();
    apInit();
    apMain();

}