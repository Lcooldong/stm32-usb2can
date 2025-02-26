/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "bsp.h"
#include "stm32g4xx_it.h"
#include "hw_def.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */



/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{

   while (1)
  {
  }

}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{

  while (1)
  {

  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{

  while (1)
  {

  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{

  while (1)
  {

  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {

  }
}

/**
  * @brief This function handles Debug monitor.
  */
 void DebugMon_Handler(void)
 {
 
 }

/**
  * @brief This function handles System service call via SWI instruction.
  */
#ifndef _USE_HW_RTOS

void SVC_Handler(void)
{

}


/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{

}


//extern void swtimerISR(void);

/**
  * @brief This function handles System tick timer.
  */
 
void SysTick_Handler(void)
{

  HAL_IncTick();

}
#endif


/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/

// /**
//   * @brief This function handles USB low priority interrupt remap.
//   */
// void USB_LP_IRQHandler(void)
// {
//   /* USER CODE BEGIN USB_LP_IRQn 0 */

//   /* USER CODE END USB_LP_IRQn 0 */
//   HAL_PCD_IRQHandler(&hpcd_USB_FS);
//   /* USER CODE BEGIN USB_LP_IRQn 1 */

//   /* USER CODE END USB_LP_IRQn 1 */
// }

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
