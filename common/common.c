#include "rx_handlers_config.h"

bool shouldAcceptTransfer(const CanardInstance* ins,
                          uint64_t* crcSignature,
                          uint16_t dataTypeID,
                          CanardTransferType transferType)
{
    // --------- RESPONSE ----------
    if (transferType == CanardTransferTypeResponse)
    {
        switch (dataTypeID)
        {
            // For RESPONSE, we might have signature TYPE_RESPONSE_SIGNATURE (if you use it)
            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_RESPONSE_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE)   /* nothing */
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_RESPONSE
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_BROADCAST

            default:
                return false;
        }
    }

    // --------- REQUEST ----------
    if (transferType == CanardTransferTypeRequest)
    {
        switch (dataTypeID)
        {
            // For REQUEST, we use TYPE_REQUEST_SIGNATURE
            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_REQUEST_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE)  /* nothing */
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_RESPONSE
            #undef DISPATCH_ACCEPT_CASE_BROADCAST

            default:
                return false;
        }
    }

    // --------- BROADCAST ----------
    if (transferType == CanardTransferTypeBroadcast)
    {
        switch (dataTypeID)
        {
            // For BROADCAST we usually just have TYPE_SIGNATURE
            #define DISPATCH_ACCEPT_CASE_BROADCAST(TYPE) \
                case TYPE##_ID: \
                    *crcSignature = TYPE##_SIGNATURE; \
                    return true;

            #define DISPATCH_ACCEPT_CASE_REQUEST(TYPE)  /* nothing */
            #define DISPATCH_ACCEPT_CASE_RESPONSE(TYPE) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_ACCEPT_CASE_##KIND(TYPE)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_ACCEPT_CASE_BROADCAST
            #undef DISPATCH_ACCEPT_CASE_REQUEST
            #undef DISPATCH_ACCEPT_CASE_RESPONSE

            default:
                return false;
        }
    }

    return false;
}

void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
    // --------- RESPONSE ----------
    if (transfer->transfer_type == CanardTransferTypeResponse)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(transfer); \
                    return;

            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_RESPONSE
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_BROADCAST

            default:
                return;
        }
    }

    // --------- REQUEST ----------
    if (transfer->transfer_type == CanardTransferTypeRequest)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(transfer); \
                    return;

            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_RESPONSE
            #undef DISPATCH_HANDLER_BROADCAST

            default:
                return;
        }
    }

    // --------- BROADCAST ----------
    if (transfer->transfer_type == CanardTransferTypeBroadcast)
    {
        switch (transfer->data_type_id)
        {
            #define DISPATCH_HANDLER_BROADCAST(TYPE, HANDLER) \
                case TYPE##_ID: \
                    HANDLER(transfer); \
                    return;

            #define DISPATCH_HANDLER_REQUEST(TYPE, HANDLER)  /* nothing */
            #define DISPATCH_HANDLER_RESPONSE(TYPE, HANDLER) /* nothing */

            #define REGISTER_RX_HANDLER(TYPE, HANDLER, KIND) \
                DISPATCH_HANDLER_##KIND(TYPE, HANDLER)

            RX_HANDLER_LIST

            #undef REGISTER_RX_HANDLER
            #undef DISPATCH_HANDLER_BROADCAST
            #undef DISPATCH_HANDLER_REQUEST
            #undef DISPATCH_HANDLER_RESPONSE

            default:
                return;
        }
    }
}

// === rx dronecan ===
// uavcan.protocol.getnodeinfo
static void handleGetNodeInfo(CanardRxTransfer *transfer)
{
  nodeStatus.uptime_sec = HAL_GetTick() / 1000U;

  struct uavcan_protocol_GetNodeInfoResponse nodeInfoRes = {0};
  nodeInfoRes.status                                            = nodeStatus;
  nodeInfoRes.software_version.major                            = HARDWARE_MAJOR_VERSION;
  nodeInfoRes.software_version.minor                            = HARDWARE_MINOR_VERSION;
  nodeInfoRes.software_version.optional_field_flags             = UAVCAN_PROTOCOL_SOFTWAREVERSION_OPTIONAL_FIELD_FLAG_VCS_COMMIT;
  nodeInfoRes.software_version.vcs_commit                       = COMMIT_HASH;
  nodeInfoRes.hardware_version.major                            = HARDWARE_MAJOR_VERSION;
  nodeInfoRes.hardware_version.minor                            = HARDWARE_MINOR_VERSION;
  nodeInfoRes.hardware_version.certificate_of_authenticity.len  = 0;
  nodeInfoRes.name.len                                          = sizeof(CAN_NODE_NAME);
  memcpy(nodeInfoRes.hardware_version.unique_id, hardwareID, 16);
  strcpy((char*)nodeInfoRes.name.data, CAN_NODE_NAME);

  uint8_t txBuffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE] = {0};
  uint32_t dataLength = uavcan_protocol_GetNodeInfoResponse_encode(&nodeInfoRes, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
    CanardTransferTypeResponse,
    UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_SIGNATURE,
    UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_ID,
    &transferID,
    CANARD_TRANSFER_PRIORITY_LOW,
    txBuffer,
    dataLength
  };
  canardRequestOrRespondObj(&canard, transfer->source_node_id, &txFrame);
}

// uavcan.protocol.nodestatus
static void sendNodeStatus(void)
{
  nodeStatus.uptime_sec = HAL_GetTick() / 1000U;

  uint8_t txBuffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE] = {0};
  uint32_t dataLength = uavcan_protocol_NodeStatus_encode(&nodeStatus, txBuffer);

  static uint8_t transferID = 0;
  CanardTxTransfer txFrame = {
    CanardTransferTypeBroadcast,
    UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE,
    UAVCAN_PROTOCOL_NODESTATUS_ID,
    &transferID,
    CANARD_TRANSFER_PRIORITY_LOW,
    txBuffer,
    dataLength
  };
  canardBroadcastObj(&canard, &txFrame);
}


void periodicCANTasks(void)
{
  static uint32_t nextRunTime = 0;

  if(HAL_GetTick() >= nextRunTime)
  {
    nextRunTime += 1000U;

    canardCleanupStaleTransfers(&canard, HAL_GetTick() * 1000U);
    sendNodeStatus();
  }
}

efscanname1ms(){
    sendCANTx();
    periodicCANTasks();
}
