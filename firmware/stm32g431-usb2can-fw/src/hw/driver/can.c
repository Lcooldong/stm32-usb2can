/*
 * can.c
 *
 *  Created on: 2021. 6. 22.
 *      Author: baram
 */


 #include "can.h"
 #include "qbuffer.h"
 #include "cli.h"
 
 
 #ifdef _USE_HW_CAN
 
 typedef struct
 {
   uint32_t prescaler;
   uint32_t sjw;
   uint32_t tseg1;
   uint32_t tseg2;
 } can_baud_cfg_t;
 
 const can_baud_cfg_t can_baud_cfg_80m_normal[] =
     {
         {50, 1, 13, 2}, // 100K, 87.5%
         {40, 1, 13, 2}, // 125K, 87.5%
         {20, 1, 13, 2}, // 250K, 87.5%
         {10, 1, 13, 2}, // 500K, 87.5%
         {5,  1, 13, 2}, // 1M,   87.5%
     };
 
 const can_baud_cfg_t can_baud_cfg_80m_data[] =
     {
         {40, 8, 11, 8}, // 100K, 60%
         {32, 8, 11, 8}, // 125K, 60%
         {16, 8, 11, 8}, // 250K, 60%
         {8,  8, 11, 8}, // 500K, 60%
         {4,  8, 11, 8}, // 1M,   60%
         {2,  8, 11, 8}, // 2M    60%
         {1,  8, 11, 8}, // 4M    60%
         {1,  8,  9, 6}, // 5M    62.5%
     };
 
 const can_baud_cfg_t *p_baud_normal = can_baud_cfg_80m_normal;
 const can_baud_cfg_t *p_baud_data   = can_baud_cfg_80m_data;
 
 
 const uint32_t dlc_len_tbl[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
 
const uint32_t dlc_tbl[] =
{
    FDCAN_DLC_BYTES_0,
    FDCAN_DLC_BYTES_1,
    FDCAN_DLC_BYTES_2,
    FDCAN_DLC_BYTES_3,
    FDCAN_DLC_BYTES_4,
    FDCAN_DLC_BYTES_5,
    FDCAN_DLC_BYTES_6,
    FDCAN_DLC_BYTES_7,
    FDCAN_DLC_BYTES_8,
    FDCAN_DLC_BYTES_12,
    FDCAN_DLC_BYTES_16,
    FDCAN_DLC_BYTES_20,
    FDCAN_DLC_BYTES_24,
    FDCAN_DLC_BYTES_32,
    FDCAN_DLC_BYTES_48,
    FDCAN_DLC_BYTES_64
};


 
 static const uint32_t frame_tbl[] =
     {
         FDCAN_FRAME_CLASSIC,
         FDCAN_FRAME_FD_NO_BRS,
         FDCAN_FRAME_FD_BRS
     };
 
 static const uint32_t mode_tbl[] =
     {
         FDCAN_MODE_NORMAL,
         FDCAN_MODE_BUS_MONITORING,
         FDCAN_MODE_INTERNAL_LOOPBACK
     };
 
 
 typedef struct
 {
   bool is_init;
   bool is_open;
 
   uint32_t err_code;
   uint8_t  state;
   uint32_t recovery_cnt;
 
   uint32_t q_rx_full_cnt;
   uint32_t q_tx_full_cnt;
   uint32_t fifo_full_cnt;
   uint32_t fifo_lost_cnt;
 
   uint32_t fifo_idx;
   uint32_t enable_int;
   can_mode_t  mode;
   can_frame_t frame;
   can_baud_t  baud;
   can_baud_t  baud_data;
 
   FDCAN_HandleTypeDef  hfdcan;
   bool (*handler)(can_msg_t *arg);
 
   qbuffer_t q_msg;
   can_msg_t can_msg[CAN_MSG_RX_BUF_MAX];
 } can_tbl_t;
 
 static can_tbl_t can_tbl[CAN_MAX_CH];
 
 static volatile uint32_t err_int_cnt = 0;
 
 
 #ifdef _USE_HW_CLI
 static void cliCan(cli_args_t *args);
 #endif
 
 static void canErrUpdate(uint8_t ch);
 
 
 
 
 
 bool canInit(void)
 {
   bool ret = true;
 
   uint8_t i;
 
 
   for(i = 0; i < CAN_MAX_CH; i++)
   {
     can_tbl[i].is_init  = true;
     can_tbl[i].is_open  = false;
     can_tbl[i].err_code = CAN_ERR_NONE;
     can_tbl[i].state    = 0;
     can_tbl[i].recovery_cnt = 0;
 
     can_tbl[i].q_rx_full_cnt = 0;
     can_tbl[i].q_tx_full_cnt = 0;
     can_tbl[i].fifo_full_cnt = 0;
     can_tbl[i].fifo_lost_cnt = 0;
 
     qbufferCreateBySize(&can_tbl[i].q_msg, (uint8_t *)&can_tbl[i].can_msg[0], sizeof(can_msg_t), CAN_MSG_RX_BUF_MAX);
   }
 
 #ifdef _USE_HW_CLI
   cliAdd("can", cliCan);
 #endif
   return ret;
 }
 
 bool canOpen(uint8_t ch, can_mode_t mode, can_frame_t frame, can_baud_t baud, can_baud_t baud_data)
 {
   bool ret = true;
   FDCAN_HandleTypeDef  *p_can;
   uint32_t tdc_offset;
 
 
   if (ch >= CAN_MAX_CH) return false;
 
 
   p_can = &can_tbl[ch].hfdcan;
 
   switch(ch)
   {
     case _DEF_CAN1:
       p_can->Instance                   = FDCAN1;
       p_can->Init.ClockDivider          = FDCAN_CLOCK_DIV1;
       p_can->Init.FrameFormat           = frame_tbl[frame];
       p_can->Init.Mode                  = mode_tbl[mode];
       p_can->Init.AutoRetransmission    = ENABLE;
       p_can->Init.TransmitPause         = ENABLE;
       p_can->Init.ProtocolException     = ENABLE;
       p_can->Init.NominalPrescaler      = p_baud_normal[baud].prescaler;
       p_can->Init.NominalSyncJumpWidth  = p_baud_normal[baud].sjw;
       p_can->Init.NominalTimeSeg1       = p_baud_normal[baud].tseg1;
       p_can->Init.NominalTimeSeg2       = p_baud_normal[baud].tseg2;
       p_can->Init.DataPrescaler         = p_baud_data[baud_data].prescaler;
       p_can->Init.DataSyncJumpWidth     = p_baud_data[baud_data].sjw;
       p_can->Init.DataTimeSeg1          = p_baud_data[baud_data].tseg1;
       p_can->Init.DataTimeSeg2          = p_baud_data[baud_data].tseg2;
       p_can->Init.StdFiltersNbr         = 28;
       p_can->Init.ExtFiltersNbr         = 8;
       p_can->Init.TxFifoQueueMode       = FDCAN_TX_FIFO_OPERATION;
 
       can_tbl[ch].mode                  = mode;
       can_tbl[ch].frame                 = frame;
       can_tbl[ch].baud                  = baud;
       can_tbl[ch].baud_data             = baud_data;
       can_tbl[ch].fifo_idx              = FDCAN_RX_FIFO0;
       can_tbl[ch].enable_int            = FDCAN_IT_LIST_RX_FIFO0 |
                                           FDCAN_IT_BUS_OFF |
                                           FDCAN_IT_ERROR_WARNING |
                                           FDCAN_IT_ERROR_PASSIVE;
       ret = true;
       break;
   }
 
   if (ret != true)
   {
     return false;
   }
 
   if (HAL_FDCAN_Init(p_can) != HAL_OK)
   {
     return false;
   }
 
 
 
   canConfigFilter(ch, 0, CAN_STD, 0x0000, 0x0000); // index -> StdFiltersNbr 
   canConfigFilter(ch, 0, CAN_EXT, 0x0000, 0x0000); // index -> ExtFiltersNbr
 
 
   if (HAL_FDCAN_ConfigGlobalFilter(p_can, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
   {
     return false;
   }
   if (HAL_FDCAN_ActivateNotification(p_can, can_tbl[ch].enable_int, 0) != HAL_OK)
   {
     return false;
   }
 
 
   tdc_offset = p_can->Init.DataPrescaler * p_can->Init.DataTimeSeg1;
 
   if (HAL_FDCAN_ConfigTxDelayCompensation(p_can, tdc_offset, 0) != HAL_OK) return false;
   if (HAL_FDCAN_EnableTxDelayCompensation(p_can) != HAL_OK)                return false;
 
 
   if (HAL_FDCAN_Start(p_can) != HAL_OK)
   {
     return false;
   }
 
 
   can_tbl[ch].is_open = true;
 
 
   return ret;
 }
 
 void canClose(uint8_t ch)
 {
 
 }
 
 bool canConfigFilter(uint8_t ch, uint8_t index, can_id_type_t id_type, uint32_t id, uint32_t id_mask)
 {
   bool ret = false;
   FDCAN_FilterTypeDef sFilterConfig;
 
 
   if (ch >= CAN_MAX_CH) return false;
 
 
   if (id_type == CAN_STD)
   {
     sFilterConfig.IdType = FDCAN_STANDARD_ID;
   }
   else
   {
     sFilterConfig.IdType = FDCAN_EXTENDED_ID;
   }
 
   if (can_tbl[ch].fifo_idx == FDCAN_RX_FIFO0)
   {
     sFilterConfig.FilterConfig  = FDCAN_FILTER_TO_RXFIFO0;
   }
   else
   {
     sFilterConfig.FilterConfig  = FDCAN_FILTER_TO_RXFIFO1;
   }
 
   sFilterConfig.FilterIndex   = index;
   sFilterConfig.FilterType    = FDCAN_FILTER_MASK;
   sFilterConfig.FilterID1     = id;
   sFilterConfig.FilterID2     = id_mask;
 
   if (HAL_FDCAN_ConfigFilter(&can_tbl[ch].hfdcan, &sFilterConfig) == HAL_OK)
   {
     ret = true;
   }
 
   return ret;
 }
 
 uint32_t canMsgAvailable(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return 0;
 
   return qbufferAvailable(&can_tbl[ch].q_msg);
 }
 
 bool canMsgInit(can_msg_t *p_msg, can_frame_t frame, can_id_type_t  id_type, can_dlc_t dlc)
 {
   p_msg->frame   = frame;
   p_msg->id_type = id_type;
   p_msg->dlc     = dlc;
   p_msg->length  = dlc_len_tbl[dlc];
   return true;
 }
 
 bool canMsgWrite(uint8_t ch, can_msg_t *p_msg, uint32_t timeout)
 {
   FDCAN_HandleTypeDef  *p_can;
   FDCAN_TxHeaderTypeDef tx_header;
   uint32_t pre_time;
   bool ret = true;
 
 
   if(ch > CAN_MAX_CH) return false;
 
   if (can_tbl[ch].err_code & CAN_ERR_BUS_OFF) return false;
 
 
   p_can = &can_tbl[ch].hfdcan;
 
   switch(p_msg->id_type)
   {
     case CAN_STD :
       tx_header.IdType = FDCAN_STANDARD_ID;
       break;
 
     case CAN_EXT :
       tx_header.IdType = FDCAN_EXTENDED_ID;
       break;
   }
 
   switch(p_msg->frame)
   {
     case CAN_CLASSIC:
       tx_header.FDFormat      = FDCAN_CLASSIC_CAN;
       tx_header.BitRateSwitch = FDCAN_BRS_OFF;
       break;
 
     case CAN_FD_NO_BRS:
       tx_header.FDFormat      = FDCAN_FD_CAN;
       tx_header.BitRateSwitch = FDCAN_BRS_OFF;
       break;
 
     case CAN_FD_BRS:
       tx_header.FDFormat      = FDCAN_FD_CAN;
       tx_header.BitRateSwitch = FDCAN_BRS_ON;
       break;
   }
 
   tx_header.Identifier          = p_msg->id;
   tx_header.MessageMarker       = 0;
   tx_header.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
   tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
   tx_header.TxFrameType         = FDCAN_DATA_FRAME;
   tx_header.DataLength          = dlc_tbl[p_msg->dlc];
 
 
   if (HAL_FDCAN_GetTxFifoFreeLevel(p_can) == 0)
   {
     return false;
   }
 
 
   pre_time = millis();
   if(HAL_FDCAN_AddMessageToTxFifoQ(p_can, &tx_header, p_msg->data) == HAL_OK)
   {
     /* Wait transmission complete */
     while(HAL_FDCAN_GetTxFifoFreeLevel(p_can) == 0)
     {
       if (millis()-pre_time >= timeout)
       {
         ret = false;
         break;
       }
     }
   }
   else
   {
     ret = false;
   }
 
   return ret;
 }
 
 bool canMsgRead(uint8_t ch, can_msg_t *p_msg)
 {
   bool ret = true;
 
 
   if(ch > CAN_MAX_CH) return 0;
    
   ret = qbufferRead(&can_tbl[ch].q_msg, (uint8_t *)p_msg, 1);
 
   return ret;
 }
 
 uint16_t canGetRxErrCount(uint8_t ch)
 {
   uint16_t ret = 0;
   HAL_StatusTypeDef status;
   FDCAN_ErrorCountersTypeDef error_counters;
 
   if(ch > CAN_MAX_CH) return 0;
 
   status = HAL_FDCAN_GetErrorCounters(&can_tbl[ch].hfdcan, &error_counters);
   if (status == HAL_OK)
   {
     ret = error_counters.RxErrorCnt;
   }
 
   return ret;
 }
 
 uint16_t canGetTxErrCount(uint8_t ch)
 {
   uint16_t ret = 0;
   HAL_StatusTypeDef status;
   FDCAN_ErrorCountersTypeDef error_counters;
 
   if(ch > CAN_MAX_CH) return 0;
 
   status = HAL_FDCAN_GetErrorCounters(&can_tbl[ch].hfdcan, &error_counters);
   if (status == HAL_OK)
   {
     ret = error_counters.TxErrorCnt;
   }
 
   return ret;
 }
 
 uint32_t canGetError(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return 0;
 
   return HAL_FDCAN_GetError(&can_tbl[ch].hfdcan);
 }
 
 uint32_t canGetState(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return 0;
 
   return HAL_FDCAN_GetState(&can_tbl[ch].hfdcan);
 }
 
 void canAttachRxInterrupt(uint8_t ch, bool (*handler)(can_msg_t *arg))
 {
   if(ch > CAN_MAX_CH) return;
 
   can_tbl[ch].handler = handler;
 }
 
 void canDetachRxInterrupt(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return;
 
   can_tbl[ch].handler = NULL;
 }
 
 void canRecovery(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return;
   if(can_tbl[ch].is_open != true) return;
 
   can_tbl[ch].err_code = CAN_ERR_NONE;

   HAL_FDCAN_Stop(&can_tbl[ch].hfdcan);
   HAL_FDCAN_Start(&can_tbl[ch].hfdcan);
 
   can_tbl[ch].recovery_cnt++;
 }
 
 bool canUpdate(void)
 {
   enum
   {
    CAN_STATE_IDLE,
    CAN_STATE_WAIT
   };


   bool ret = false;
   can_tbl_t *p_can;
 
 
   for (int i=0; i<CAN_MAX_CH; i++)
   {
     p_can = &can_tbl[i];
 
 
     switch(p_can->state)
     {
       case CAN_STATE_IDLE:
         if (p_can->err_code & CAN_ERR_BUS_OFF)
         {
           canRecovery(i);
           p_can->state = CAN_STATE_WAIT;
           ret = true;
         }
         break;
 
       case CAN_STATE_WAIT:
         if ((p_can->err_code & CAN_ERR_BUS_OFF) == 0)
         {
           p_can->state = CAN_STATE_IDLE;
         }
         break;
     }
   }
 
   return ret;
 }
 
 void canRxFifoCallback(uint8_t ch, FDCAN_HandleTypeDef *hfdcan)
 {
   can_msg_t *rx_buf;
   FDCAN_RxHeaderTypeDef rx_header;
 
 
   rx_buf  = (can_msg_t *)qbufferPeekWrite(&can_tbl[ch].q_msg);
 
   if (HAL_FDCAN_GetRxMessage(hfdcan, can_tbl[ch].fifo_idx, &rx_header, rx_buf->data) == HAL_OK)
   {
     if(rx_header.IdType == FDCAN_STANDARD_ID)
     {
       rx_buf->id      = rx_header.Identifier;
       rx_buf->id_type = CAN_STD;
     }
     else
     {
       rx_buf->id      = rx_header.Identifier;
       rx_buf->id_type = CAN_EXT;
     }
    //  rx_buf->length = dlc_len_tbl[(rx_header.DataLength >> 16) & 0x0F];  // HAL 1.2.0 버전 DLC 차이
     rx_buf->length = dlc_len_tbl[rx_header.DataLength];           // HAL 1.2.5 버전
 
 
     if (rx_header.FDFormat == FDCAN_FD_CAN)
     {
       if (rx_header.BitRateSwitch == FDCAN_BRS_ON)
       {
         rx_buf->frame = CAN_FD_BRS;
       }
       else
       {
         rx_buf->frame = CAN_FD_NO_BRS;
       }
     }
     else
     {
       rx_buf->frame = CAN_CLASSIC;
     }
 
     if (qbufferWrite(&can_tbl[ch].q_msg, NULL, 1) != true)
     {
       can_tbl[ch].q_rx_full_cnt++;
     }
 
     if( can_tbl[ch].handler != NULL )
     {
       if ((*can_tbl[ch].handler)((void *)rx_buf) == true)
       {
         qbufferRead(&can_tbl[ch].q_msg, NULL, 1);
       }
     }
   }
 }
 
 void canErrClear(uint8_t ch)
 {
   if(ch > CAN_MAX_CH) return;
 
   can_tbl[ch].err_code = CAN_ERR_NONE;
 }
 
 void canErrPrint(uint8_t ch)
 {
   uint32_t err_code;
 
 
   if(ch > CAN_MAX_CH) return;
 
   err_code = can_tbl[ch].err_code;
 
   if (err_code & CAN_ERR_PASSIVE) logPrintf("  ERR : CAN_ERR_PASSIVE\r\n");
   if (err_code & CAN_ERR_WARNING) logPrintf("  ERR : CAN_ERR_WARNING\r\n");
   if (err_code & CAN_ERR_BUS_OFF) logPrintf("  ERR : CAN_ERR_BUS_OFF\r\n");
 }
 
 void canErrUpdate(uint8_t ch)
 {
   FDCAN_ProtocolStatusTypeDef protocol_status;
 
   HAL_FDCAN_GetProtocolStatus(&can_tbl[ch].hfdcan, &protocol_status);
 
   if (protocol_status.ErrorPassive)
   {
     can_tbl[ch].err_code |= CAN_ERR_PASSIVE;
   }
   else
   {
     can_tbl[ch].err_code &= ~CAN_ERR_PASSIVE;
   }
 
   if (protocol_status.Warning)
   {
     can_tbl[ch].err_code |= CAN_ERR_WARNING;
   }
   else
   {
     can_tbl[ch].err_code &= ~CAN_ERR_WARNING;
   }
 
   if (protocol_status.BusOff)
   {
     can_tbl[ch].err_code |= CAN_ERR_BUS_OFF;
   }
   else
   {
     can_tbl[ch].err_code &= ~CAN_ERR_BUS_OFF;
   }
 }
 
 
 
 
 
 
 void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
 {
   uint8_t ch = _DEF_CAN1;
 
   canErrUpdate(ch);
 }

 void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
 {
   uint8_t ch = _DEF_CAN1;
 
   err_int_cnt++;
 
   if (hfdcan->ErrorCode > 0)
   {
     can_tbl[ch].err_code |= CAN_ERR_ETC;
   }
 }
 
 void FDCAN1_IT0_IRQHandler(void)
 {
   HAL_FDCAN_IRQHandler(&can_tbl[_DEF_CAN1].hfdcan);
 }
 
 void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
 {
   if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
   {
     canRxFifoCallback(_DEF_CAN1, hfdcan);
   }
 
   if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_FULL) != RESET)
   {
     can_tbl[_DEF_CAN1].fifo_full_cnt++;
   }
 
   if((RxFifo0ITs & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != RESET)
   {
     can_tbl[_DEF_CAN1].fifo_lost_cnt++;
   }
 }
 
 void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef* fdcanHandle)
 {
 
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   if(fdcanHandle->Instance==FDCAN1)
   {
     /* FDCAN1 clock enable */
     __HAL_RCC_FDCAN_CLK_ENABLE();
 
     __HAL_RCC_GPIOB_CLK_ENABLE();
     /**FDCAN1 GPIO Configuration
     PB8-BOOT0   ------> FDCAN1_RX
     PB9         ------> FDCAN1_TX
     */
     GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
     GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
     GPIO_InitStruct.Pull = GPIO_NOPULL;
     GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
     GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
     HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 
     HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 0, 0);
     HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
   }
 }
 
 void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef* fdcanHandle)
 {
 
   if(fdcanHandle->Instance==FDCAN1)
   {
     __HAL_RCC_FDCAN_CLK_DISABLE();
 
     /**FDCAN1 GPIO Configuration
     PB8-BOOT0   ------> FDCAN1_RX
     PB9         ------> FDCAN1_TX
     */
     HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8|GPIO_PIN_9);
 
     HAL_NVIC_DisableIRQ(FDCAN1_IT0_IRQn);
   }
 }
 
 
 #ifdef _USE_HW_CLI
 void cliCan(cli_args_t *args)
 {
   bool ret = false;
 
 
 
   if (args->argc == 1 && args->isStr(0, "info"))
   {
     for (int i=0; i<CAN_MAX_CH; i++)
     {
       cliPrintf("is_open       : %d\r\n", can_tbl[i].is_open);
 
       cliPrintf("q_rx_full_cnt : %d\r\n", can_tbl[i].q_rx_full_cnt);
       cliPrintf("q_tx_full_cnt : %d\r\n", can_tbl[i].q_tx_full_cnt);
       cliPrintf("fifo_full_cnt : %d\r\n", can_tbl[i].fifo_full_cnt);
       cliPrintf("fifo_lost_cnt : %d\r\n", can_tbl[i].fifo_lost_cnt);
       canErrPrint(i);
     }
     ret = true;
   }
 
   if (args->argc == 1 && args->isStr(0, "read"))
   {
     uint32_t index = 0;
 
     while(cliKeepLoop())
     {
       if (canMsgAvailable(_DEF_CAN1))
       {
         can_msg_t msg;
 
         canMsgRead(_DEF_CAN1, &msg);
 
         index %= 1000;
         cliPrintf("%03d(R) <- id ", index++);
         if (msg.id_type == CAN_STD)
         {
           cliPrintf("std ");
         }
         else
         {
           cliPrintf("ext ");
         }
         cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
         for (int i=0; i<msg.length; i++)
         {
           cliPrintf("0x%02X ", msg.data[i]);
         }
         cliPrintf("\r\n");
       }
     }
     ret = true;
   }
 
   if (args->argc == 1 && args->isStr(0, "send"))
   {
     uint32_t pre_time = 0;
     uint32_t index = 0;
     uint32_t err_code;
 
 
     err_code = can_tbl[_DEF_CAN1].err_code;
 
     while(cliKeepLoop())
     {
       can_msg_t msg;
 
       if (millis()-pre_time >= 1000)
       {
         pre_time = millis();

         //  msg.frame   = CAN_CLASSIC;
         msg.frame   = CAN_FD_NO_BRS;
         msg.id_type = CAN_EXT;
         msg.dlc     = CAN_DLC_8;
    
         msg.id      = 0x314;
         msg.length  = 8;
         msg.data[0] = 1;
         msg.data[1] = 2;
         msg.data[2] = 3;
         msg.data[3] = 4;
         msg.data[4] = 5;
         msg.data[5] = 6;
         msg.data[6] = 7;
         msg.data[7] = 8;

         if (canMsgWrite(_DEF_CAN1, &msg, 10) > 0)
         {
           index %= 1000;
           cliPrintf("%03d(T) -> id ", index++);
           if (msg.id_type == CAN_STD)
           {
             cliPrintf("std ");
           }
           else
           {
             cliPrintf("ext ");
           }
           cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
           for (int i=0; i<msg.length; i++)
           {
             cliPrintf("0x%02X ", msg.data[i]);
           }
           cliPrintf("\r\n");
         }
 
         if (canGetRxErrCount(_DEF_CAN1) > 0 || canGetTxErrCount(_DEF_CAN1) > 0)
         {
           cliPrintf("ErrCnt : %d, %d\r\n", canGetRxErrCount(_DEF_CAN1), canGetTxErrCount(_DEF_CAN1));
         }
 
         if (err_int_cnt > 0)
         {
           cliPrintf("Cnt : %d\r\n",err_int_cnt);
           err_int_cnt = 0;
         }
       }
 
       if (can_tbl[_DEF_CAN1].err_code != err_code)
       {
         cliPrintf("ErrCode : 0x%X\r\n", can_tbl[_DEF_CAN1].err_code);
         canErrPrint(_DEF_CAN1);
         err_code = can_tbl[_DEF_CAN1].err_code;
       }
 
       if (canUpdate())
       {
         cliPrintf("BusOff Recovery\r\n");
       }
 
 



       if (canMsgAvailable(_DEF_CAN1))
       {
         canMsgRead(_DEF_CAN1, &msg);
 
         index %= 1000;
         cliPrintf("%03d(R) <- id ", index++);
         if (msg.id_type == CAN_STD)
         {
           cliPrintf("std ");
         }
         else
         {
           cliPrintf("ext ");
         }
         cliPrintf(": 0x%08X, L:%02d, ", msg.id, msg.length);
         for (int i=0; i<msg.length; i++)
         {
           cliPrintf("0x%02X ", msg.data[i]);
         }
         cliPrintf("\r\n");
       }
     }
     ret = true;
   }
 
   if (ret == false)
   {
     cliPrintf("can info\r\n");
     cliPrintf("can read\r\n");
     cliPrintf("can send\r\n");
   }
 }
 #endif
 
 #endif