#pragma once

#include "pinecan.h"

typedef struct {
    CanardInstance *canard;
    void* canardMemPool;
    size_t canardMemPoolSize;
    struct uavcan_protocol_NodeStatus *nodeStatus;
} CommonInitParams;

void init(CommonInitParams *initParams);
void handleRxFrame(CanardCANFrame *rxFrame);
