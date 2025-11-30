#pragma once

#include <stdint.h>
#include "canard.h"

uint8_t *getUniqueHardwareID(void);
uint32_t getUptimeMs(void);
bool processTxMessage(const CanardCANFrame *txFrame);
