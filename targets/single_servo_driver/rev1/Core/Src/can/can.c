#include <string.h>
#include "canard.h"
#include "dronecan_msgs.h"
#include "servo.h"
#include "stm32l4xx_hal.h"
#include "pinecan.h"

extern CAN_HandleTypeDef hcan1;

struct uavcan_protocol_NodeStatus nodeStatus;

static CanardInstance canard;
static uint8_t canardMemPool[1024];

// === tx dronecan ===
// uavcan.equipment.actuator.status
// TODO: QOL feature

// uavcan.equipment.power.CircuitStatus
// TODO: QOL feature

// === rx dronecan ===
// uavcan.equipment.actuator.arraycommand
void handleArrayCommand(CanardInstance* ins, CanardRxTransfer *transfer)
{
  UNUSED(ins);

  struct uavcan_equipment_actuator_ArrayCommand cmdArray = {0};
  if(uavcan_equipment_actuator_ArrayCommand_decode(transfer, &cmdArray))
  {
    return;
  }

  for(uint8_t i = 0; i < cmdArray.commands.len; ++i)
  {
    struct uavcan_equipment_actuator_Command cmd = cmdArray.commands.data[i];

    switch(cmd.command_type)
    {
      case UAVCAN_EQUIPMENT_ACTUATOR_COMMAND_COMMAND_TYPE_UNITLESS:
        percentageActuation(cmd.command_value, cmd.actuator_id);
        break;
      case UAVCAN_EQUIPMENT_ACTUATOR_COMMAND_COMMAND_TYPE_PWM:
        dutyCycleActuation(cmd.command_value, cmd.actuator_id);
        break;
    }
  }
}

void initCAN(void)
{
  PinecanInit initParams = {
    .hcan = &hcan1,
    .canard = &canard,
    .canardMemPool = canardMemPool,
    .canardMemPoolSize = sizeof(canardMemPool),
    .nodeStatus = &nodeStatus
  };
  pinecanInit(&initParams);
}
