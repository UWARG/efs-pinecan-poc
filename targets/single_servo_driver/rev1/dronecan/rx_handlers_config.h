// rx_handlers_config.h

// TRANSFER_KIND is one of: REQUEST, RESPONSE, BROADCAST
// TYPE is the base name of the DSDL type:
//   expects TYPE_ID, TYPE_REQUEST_SIGNATURE, TYPE_RESPONSE_SIGNATURE, or TYPE_SIGNATURE to exist
// HANDLER is the function that handles the received transfer

#define RX_HANDLER_LIST \
    REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_GETNODEINFO,           handleGetNodeInfo,           REQUEST) \
    REGISTER_RX_HANDLER(UAVCAN_EQUIPMENT_ACTUATOR_ARRAYCOMMAND, handleArrayCommand,        BROADCAST) \
    REGISTER_RX_HANDLER(ARDUPILOT_INDICATION_NOTIFYSTATE,      handleNotifyState,          BROADCAST) \
    REGISTER_RX_HANDLER(UAVCAN_PROTOCOL_NODESTATUS,            handleNodeStatus,           BROADCAST)
