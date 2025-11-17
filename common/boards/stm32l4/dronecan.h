
/*
 * canard_stm32_driver.h
 *
 *  Created on: Jul 9, 2024
 *      Author: ronik
 */
#pragma once

#include "efs_dronecan_common.h"
#include "stm32l4xx_hal.h"

typedef struct {
 CAN_HandleTypeDef *hcan;
 CanardInstance *canard;
 void* mem_pool;
 size_t mem_pool_size;
 CanardOnTransferReception on_recep;
 CanardShouldAcceptTransfer should_accept;
} InitTypeDef;

void init(InitTypeDef *initParams);

void processTasks(CAN_HandleTypeDef *hcan, CanardInstance *canard, uint64_t *next_1hz_service);