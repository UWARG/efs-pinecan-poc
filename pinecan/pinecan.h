#pragma once

#include "canard.h"
#include "pinecanBoard.h"
#include <stdint.h>

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

/**
 * @brief  Process PineCAN tasks every 1 ms
 * @retval None
 */
void pinecan1ms(void);
