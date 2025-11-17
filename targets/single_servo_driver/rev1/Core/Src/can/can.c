#include <string.h>
#include "canard.h"
#include "dronecan_msgs.h"
#include "servo.h"
#include "stm32l4xx_hal.h"

#define CAN_NODE_NAME          "SSD1"
#define CAN_NODE_ID            70U
#define COMMIT_HASH            0U
#define SOFTWARE_MAJOR_VERSION 1U
#define SOFTWARE_MINOR_VERSION 0U
#define HARDWARE_MAJOR_VERSION 2U
#define HARDWARE_MINOR_VERSION 0U

extern CAN_HandleTypeDef hcan1;

struct uavcan_protocol_NodeStatus nodeStatus;

static CanardInstance canard;
static uint8_t canardMemPool[1024];
static uint8_t hardwareID[16];

// === tx dronecan ===
// uavcan.equipment.actuator.status
// TODO: QOL feature

// uavcan.equipment.power.CircuitStatus
// TODO: QOL feature

// === rx dronecan ===
// uavcan.equipment.actuator.arraycommand
static void handleArrayCommand(CanardRxTransfer *transfer)
{
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
  typedef struct dronecanInstance_t {
  // canard specific
    CanardInstance canard;
    void *canardMemPool;
    size_t canardMemPoolSize;
  // dronecan specific
    struct uavcan_protocol_NodeStatus *nodeStatus;
  // hardware specific
    CAN_FilterTypeDef *canfil; // might even skip making this configurable for now until we want it to be
    CAN_HandleTypeDef *hcan;
  } dronecanInstance_t;
  dronecanInit();
}
