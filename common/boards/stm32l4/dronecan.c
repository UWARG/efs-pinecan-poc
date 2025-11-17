/*
 * canard_stm32_driver.c
 *
 *  Created on: 
 *      Author: 
 */

#include <string.h>
#include <stdio.h>
#include "stm32l4_efs_dronecan.h"

static uint32_t canMailbox; //CAN Bus Mail box variable

/*
get a 16 byte unique ID for this node, this should be based on the CPU unique ID or other unique ID
*/
void getUniqueID(uint8_t id[16]){
    uint32_t HALUniqueIDs[4];
    // Make Unique ID out of the 96-bit STM32 UID
    memset(id, 0, 16);
    HALUniqueIDs[0] = HAL_GetUIDw0();
    HALUniqueIDs[1] = HAL_GetUIDw1();
    HALUniqueIDs[2] = HAL_GetUIDw2();
    HALUniqueIDs[3] = HAL_GetUIDw1(); // repeating UIDw1 for this, no specific reason I chose this..
    memcpy(id, HALUniqueIDs, 16);
}

void init(InitTypeDef *initParams) {
CAN_FilterTypeDef canfil;
canfil.FilterBank = 0;
canfil.FilterMode = CAN_FILTERMODE_IDMASK;
canfil.FilterFIFOAssignment = CAN_RX_FIFO0;
canfil.FilterIdHigh = 0;
canfil.FilterIdLow = 0;
canfil.FilterMaskIdHigh = 0;
canfil.FilterMaskIdLow = 0;
canfil.FilterScale = CAN_FILTERSCALE_32BIT;
canfil.FilterActivation = ENABLE;
canfil.SlaveStartFilterBank = 14;
HAL_CAN_ConfigFilter(initParams->hcan, &canfil);

HAL_CAN_Start(initParams->hcan);
HAL_CAN_ActivateNotification(initParams->hcan, CAN_IT_RX_FIFO0_MSG_PENDING);
canardInit(initParams->canard,
                initParams->mem_pool,
                initParams->mem_pool_size,
                initParams->on_recep,
                initParams->should_accept,
                NULL);

if (NODE_ID > 0) {
    canardSetLocalNodeID(initParams->canard, NODE_ID);
} else {
    printf("Node ID is 0, this node is anonymous and can't transmit most messaged. Please update this in node_settings.h\n");
}
}

/**
 * @brief  Process CAN message from RxLocation FIFO into rx_frame
 * @param  hcan pointer to an CAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  RxLocation Location of the received message to be read.
 *         This parameter can be a value of @arg FDCAN_Rx_location.
 * @param  rx_frame pointer to a CanardCANFrame structure where the received CAN message will be
 * 		stored.
 * @retval ret == 1: OK, ret < 0: CANARD_ERROR, ret == 0: Check hcan->ErrorCode
 */
int16_t efsDroneCANRecieve(CAN_HandleTypeDef *hcan, uint32_t RxLocation, CanardCANFrame *const rx_frame) {
    if (rx_frame == NULL) {
        return -CANARD_ERROR_INVALID_ARGUMENT;
    }

    CAN_RxHeaderTypeDef RxHeader;
    uint8_t RxData[8];

    if (HAL_CAN_GetRxMessage(hcan, RxLocation, &RxHeader, RxData) == HAL_OK) {

        //	printf("Received message: ID=%lu, DLC=%lu\n", RxHeader.StdId, RxHeader.DLC);
        //
        //	printf("0x");
        //	for (int i = 0; i < RxHeader.DLC; i++) {
        //		printf("%02x", RxData[i]);
        //	}
        //	printf("\n");

        // Process ID to canard format
        rx_frame->id = RxHeader.ExtId;

        if (RxHeader.IDE == CAN_ID_EXT) { // canard will only process the message if it is extended ID
            rx_frame->id |= CANARD_CAN_FRAME_EFF;
        }

        if (RxHeader.RTR == CAN_RTR_REMOTE) { // canard won't process the message if it is a remote frame
            rx_frame->id |= CANARD_CAN_FRAME_RTR;
        }

        rx_frame->data_len = RxHeader.DLC;
        memcpy(rx_frame->data, RxData, RxHeader.DLC);

        // assume a single interface
        rx_frame->iface_id = 0;

        return 1;
    }

    // Either no CAN msg to be read, or an error that can be read from hcan->ErrorCode
    return 0;
}

/**
 * @brief  Process tx_frame CAN message into Tx FIFO/Queue and transmit it
 * @param  hcan pointer to an CAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  tx_frame pointer to a CanardCANFrame structure that contains the CAN message to
 * 		transmit.
 * @retval ret == 1: OK, ret < 0: CANARD_ERROR, ret == 0: Check hcan->ErrorCode
 */
int16_t efsDroneCANTransmit(CAN_HandleTypeDef *hcan, const CanardCANFrame* const tx_frame) {
    if (tx_frame == NULL) {
        return -CANARD_ERROR_INVALID_ARGUMENT;
    }

    if (tx_frame->id & CANARD_CAN_FRAME_ERR) {
        return -CANARD_ERROR_INVALID_ARGUMENT; // unsupported frame format
    }

    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];

    // Process canard id to STM FDCAN header format
    if (tx_frame->id & CANARD_CAN_FRAME_EFF) {
        TxHeader.IDE = CAN_ID_EXT;
        TxHeader.ExtId = tx_frame->id & CANARD_CAN_EXT_ID_MASK;
    } else {
        TxHeader.IDE = CAN_ID_STD;
        TxHeader.StdId = tx_frame->id & CANARD_CAN_STD_ID_MASK;
    }

    TxHeader.DLC = tx_frame->data_len;

    if (tx_frame->id & CANARD_CAN_FRAME_RTR) {
        TxHeader.RTR = CAN_RTR_REMOTE;
    } else {
        TxHeader.RTR = CAN_RTR_DATA;
    }

    TxHeader.TransmitGlobalTime = DISABLE;
    memcpy(TxData, tx_frame->data, TxHeader.DLC);

    if (HAL_CAN_AddTxMessage(hcan, &TxHeader, TxData, &canMailbox) == HAL_OK) {
//		printf("Successfully sent message with id: %lu \n", TxHeader.StdId);
        return 1;
    }

//	printf("Failed at adding message with id: %lu to Tx Fifo", TxHeader.StdId);
    // This might be for many reasons including the Tx Fifo being full, the error can be read from hcan->ErrorCode
    return 0;
}

void processCanardRxQueue(CanardInstance *canard) {
    struct dequeueRxReturnItem dequeueRxReturnItem = dequeueRxFrame();
    const uint64_t timestamp = HAL_GetTick() * 1000ULL;

    if (dequeueRxReturnItem.isSuccess) {
        canardHandleRxFrame(canard, &(dequeueRxReturnItem.frame), timestamp);
    }
}

void processCanardTxQueue(CAN_HandleTypeDef *hcan, CanardInstance *canard) {
    // Transmitting

    for (const CanardCANFrame *tx_frame ; (tx_frame = canardPeekTxQueue(canard)) != NULL;) {
        const int16_t tx_res = efsDroneCANTransmit(hcan, tx_frame);

        if (tx_res <= 0) {
            printf("Transmit error %d\n", tx_res);
        } else if (tx_res > 0) {
            printf("Successfully transmitted message\n");
        }

        // Pop canardTxQueue either way
        canardPopTxQueue(canard);
    }
}

void processTasks(CAN_HandleTypeDef *hcan, CanardInstance *canard) {
    processCanardRxQueue(canard);
    processCanardTxQueue(hcan, canard);

    // 1 Hz tasks
    static uint32_t next_run_time = 0
    if (HAL_GetTick() >= next_run_time) {
        next_run_time += 1000U;
        //Purge transfers that are no longer transmitted. This can free up some memory
        canardCleanupStaleTransfers(canard, HAL_GetTick() * 1000U);
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
// Receiving
    CanardCANFrame rx_frame;

    const int16_t rx_res = canardSTM32Recieve(hcan, CAN_RX_FIFO0, &rx_frame);

    if (rx_res < 0) {
        printf("Receive error %d\n", rx_res);
    }
    else if (rx_res > 0)        // Success - process the frame
    {
        enqueueRxReturnCode enqueueRxReturnCode = enqueueRxFrame(&rx_frame);

        switch (enqueueRxReturnCode) {
            case RX_ENQUEUE_EMPTYITEM:
                printf("Rx frame is empty");
                break;
            case RX_ENQUEUE_MALLOCFAIL:
                printf("CanardRxQueueItem memory allocation failed");
                break;
            case RX_ENQUEUE_OVERFLOW:
                printf("rxQueue is full, oldest frame has been removed");
                break;
            case RX_ENQUEUE_SUCCESS:
                break;
        }
    }
}


// Can think abt support for multiple rx fifos (this would come with some smart stuff with setting up the filters correctly for that as well)
// Also even though we can use our own rx fifo queue here, we can also just use the fact that stm32 already has an rx fifo of length 3 (and its in hardware hooray)
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  // receive HAL CAN packet
  CAN_RxHeaderTypeDef rxHeader = {0};
  uint8_t rxData[8] = {0};

  if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
  {
    if(rxHeader.IDE != CAN_ID_EXT || rxHeader.RTR != CAN_RTR_DATA)
    {
      return;
    }
  }

  // create canard packet
  CanardCANFrame rxFrame = {0};

  rxFrame.id = rxHeader.ExtId | CANARD_CAN_FRAME_EFF;
  rxFrame.data_len = rxHeader.DLC ;
  rxFrame.iface_id = 0;
  memcpy(rxFrame.data, rxData, rxHeader.DLC);

  canardHandleRxFrame(&canard, &rxFrame, HAL_GetTick() * 1000U);
}


void sendCANTx(void)
{
  while(1)
  {
    CanardCANFrame* frame = canardPeekTxQueue(&canard);
    if(frame == NULL)
    {
      return;
    }

    if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0)
    {
      CAN_TxHeaderTypeDef txHeader = {0};
      txHeader.ExtId = frame->id & 0x1FFFFFFF;
      txHeader.IDE = CAN_ID_EXT;
      txHeader.RTR = CAN_RTR_DATA;
      txHeader.DLC = frame->data_len;

      uint32_t txMailboxUsed = 0;
      if(HAL_CAN_AddTxMessage(&hcan1, &txHeader, frame->data, &txMailboxUsed) == HAL_OK)
      {
        canardPopTxQueue(&canard);
      }
    }
    else
    {
      return;
    }
  }
}




droneCANinit() {

// this is just copy pasted from existing, read the l4 HAL docs again on how to actually do it
// also probably call a common version of this function to do the parts that are not board specific?

  // configure HAL CAN
  CAN_FilterTypeDef canfil;
  canfil.FilterBank = 0;
  canfil.FilterMode = CAN_FILTERMODE_IDMASK;
  canfil.FilterFIFOAssignment = CAN_RX_FIFO0;
  canfil.FilterIdHigh = 0;
  canfil.FilterIdLow = 0;
  canfil.FilterMaskIdHigh = 0;
  canfil.FilterMaskIdLow = 0;
  canfil.FilterScale = CAN_FILTERSCALE_32BIT;
  canfil.FilterActivation = ENABLE;
  canfil.SlaveStartFilterBank = 0;
  HAL_CAN_ConfigFilter(&hcan1, &canfil);

  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  // configure Canard
  canardInit(&canard,
    canardMemPool,
    sizeof(canardMemPool),
    onTransferReceived,
    shouldAcceptTransfer,
    NULL
  );

  canardSetLocalNodeID(&canard, CAN_NODE_ID);

  // set hardware id
  uint32_t tmpHID[] = {HAL_GetUIDw2(), HAL_GetUIDw1(), HAL_GetUIDw0()};
  memcpy(hardwareID + 4, tmpHID, 12);

  // start HAL driver
  HAL_CAN_Start(&hcan1);
}

