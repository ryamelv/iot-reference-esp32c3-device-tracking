#pragma once

/*******************************************************************************************************************//**
 * IoT functionality
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include "device_tracking/device_tracking_config.h"

#if DT_IOT_AGENT
  #include "core_mqtt_agent.h"
  typedef MQTTAgentContext_t IotContext;
#else
  #include "core_mqtt.h"
  typedef MQTTContext_t IotContext;
#endif



/***********************************************************************************************************************
 * Function Declarations
 **********************************************************************************************************************/

MQTTStatus_t IotInit(IotContext* iot_context);
MQTTStatus_t IotConnect(IotContext* iot_context, const char* client_id);
MQTTStatus_t IotPublish(IotContext* iot_context, const char* topic, const char* msg);
const char* IotGetClientId();