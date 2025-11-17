#pragma once

/**
  * @brief  Initialize HAL CAN and canard.
  * @retval None
  */
void initCAN(void);

/**
  * @brief  Send all CAN Tx messages queued in canard.
  * @retval None
  */
void sendCANTx(void);

/**
  * @brief  Execute periodic CAN maintenance tasks.
  * @retval None
  */
void periodicCANTasks(void);
