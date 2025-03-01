/*
 * rtos.c
 *
 *  Created on: 2021. 7. 18.
 *      Author: baram
 */

 #include "bsp.h"
 #include "rtos.h"
 
 
 
 
 
 bool rtosInit(void)
 {
   return true;
 }
 
 
 
 
 
 TIM_HandleTypeDef        htim6;
 
 
 /**
   * @brief  This function configures the TIM6 as a time base source.
   *         The time source is configured  to have 1ms time base with a dedicated
   *         Tick interrupt priority.
   * @note   This function is called  automatically at the beginning of program after
   *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
   * @param  TickPriority: Tick interrupt priority.
   * @retval HAL status
   */
 HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
 {
   RCC_ClkInitTypeDef    clkconfig;
   uint32_t              uwTimclock = 0;
   uint32_t              uwPrescalerValue = 0;
   uint32_t              pFLatency;
   HAL_StatusTypeDef     status = HAL_OK;
   /* Enable TIM6 clock */
   __HAL_RCC_TIM6_CLK_ENABLE();
 
   /* Get clock configuration */
   HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
 
   /* Compute TIM6 clock */
   uwTimclock = HAL_RCC_GetPCLK1Freq();
 
   /* Compute the prescaler value to have TIM6 counter clock equal to 1MHz */
   uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000) - 1);
 
   /* Initialize TIM6 */
   htim6.Instance = TIM6;
 
   /* Initialize TIMx peripheral as follow:
   + Period = [(TIM6CLK/1000) - 1]. to have a (1/1000) s time base.
   + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
   + ClockDivision = 0
   + Counter direction = Up
   */
   htim6.Init.Period = (1000000 / 1000) - 1;
   htim6.Init.Prescaler = uwPrescalerValue;
   htim6.Init.ClockDivision = 0;
   htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
   status = HAL_TIM_Base_Init(&htim6);
   if (status == HAL_OK)
   {
     /* Start the TIM time Base generation in interrupt mode */
     status = HAL_TIM_Base_Start_IT(&htim6);
     if (status == HAL_OK)
     {
     /* Enable the TIM6 global Interrupt */
         HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
       /* Configure the SysTick IRQ priority */
       if (TickPriority < (1UL << __NVIC_PRIO_BITS))
       {
         /* Configure the TIM IRQ priority */
         HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0U);
         uwTickPrio = TickPriority;
       }
       else
       {
         status = HAL_ERROR;
       }
     }
   }
  /* Return function status */
   return status;
 }
 
 /**
   * @brief  Suspend Tick increment.
   * @note   Disable the tick increment by disabling TIM6 update interrupt.
   * @param  None
   * @retval None
   */
 void HAL_SuspendTick(void)
 {
   /* Disable TIM6 update Interrupt */
   __HAL_TIM_DISABLE_IT(&htim6, TIM_IT_UPDATE);
 }
 
 /**
   * @brief  Resume Tick increment.
   * @note   Enable the tick increment by Enabling TIM6 update interrupt.
   * @param  None
   * @retval None
   */
 void HAL_ResumeTick(void)
 {
   /* Enable TIM6 Update interrupt */
   __HAL_TIM_ENABLE_IT(&htim6, TIM_IT_UPDATE);
 }
 
 void TIM6_DAC_IRQHandler(void)
 {
   HAL_TIM_IRQHandler(&htim6);
 }
 
 
 void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
 {
   if (htim->Instance == TIM6)
   {
     HAL_IncTick();
   }
 }