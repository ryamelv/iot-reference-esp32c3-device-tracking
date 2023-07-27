#include "device_tracking_config.h"
#if DT_IOT_AGENT

/*******************************************************************************************************************//**
 * IoT functionality.
 *
 * Note this file is only in effect when use of an externally managed IoT Agent is enabled (DT_IOT_AGENT is 1). See
 * iot_standalone.c for when the device tracking app must use the standalone core MQTT library (DT_IOT_AGENT is 0).
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include <string.h>
#include "esp_log.h"
#include "core_mqtt_agent_manager.h"
#include "device_tracking/iot.h"



/***********************************************************************************************************************
 * Globals
 **********************************************************************************************************************/

// Logging identifier for this module.
static const char* TAG = "iot";

// Used to pass data between publish and publish-callback.
struct MQTTAgentCommandContext
{
  TaskHandle_t xTaskToNotify;           // handle of publish request thread
  uint32_t ulNotificationValue;         // task notification slot value
  MQTTStatus_t xReturnStatus;           // publish result from publish callback
};



/***********************************************************************************************************************
 * Function Definitions
 **********************************************************************************************************************/

static void PublishCallback(MQTTAgentCommandContext_t* context, MQTTAgentReturnInfo_t* cb_info) {
  // store the command result in the callback context for the requester
  context->xReturnStatus = cb_info->returnCode;

  // notify the requesting thread that the command is complete
  if(context->xTaskToNotify != NULL )
  {
    xTaskNotify(context->xTaskToNotify,               // handle of task to notify
                context->ulNotificationValue,         // notification slot value
                eSetValueWithOverwrite );             // overwrite notification slot with new value
  }
}



static BaseType_t WaitForCallback() {
  BaseType_t rc = xTaskNotifyWait(
    0,                        // notification slot bits to clear on entry
    0,                        // notification slot bits to clear on exit
    NULL,                     // notification slot value (not used)
    portMAX_DELAY);           // wait forever

  return(rc);
}



MQTTStatus_t IotInit(IotContext* iot_context) {
  // nothing to do when externally managed IoT agent is in use
  MQTTStatus_t rc = MQTTSuccess;
  return(rc);
}



MQTTStatus_t IotConnect(IotContext* iot_context, const char* client_id) {
  // nothing to do when externally managed IoT agent is in use
  MQTTStatus_t rc = MQTTSuccess;
  return(rc);
}



MQTTStatus_t IotPublish(IotContext* iot_context, const char* topic, const char* msg) {
  ESP_LOGI(TAG, "Publishing MQTT Message: [%s] %s", topic, msg);

  MQTTPublishInfo_t pub_info = {0};

  pub_info.qos             = DT_MQTT_QOS;
  pub_info.retain          = false;
  pub_info.dup             = false;
  pub_info.pTopicName      = topic;
  pub_info.topicNameLength = (uint16_t) strlen(pub_info.pTopicName);
  pub_info.pPayload        = msg;
  pub_info.payloadLength   = (uint16_t) strlen(pub_info.pPayload);

  MQTTAgentCommandContext_t cmd_cxt = {0};

  cmd_cxt.xTaskToNotify = xTaskGetCurrentTaskHandle();

  MQTTAgentCommandInfo_t cmd_info = {0};

  cmd_info.blockTimeMs = 1000;
  cmd_info.cmdCompleteCallback = PublishCallback;
  cmd_info.pCmdCompleteCallbackContext = &cmd_cxt;

  MQTTStatus_t rc = MQTTAgent_Publish(iot_context, &pub_info, &cmd_info );

  if(MQTTSuccess != rc) {
    ESP_LOGW(TAG, "MQTTAgent_Publish failed: %d ", rc);
  }
  else {
    WaitForCallback();
  }

  if(MQTTSuccess != cmd_cxt.xReturnStatus) {
    ESP_LOGW(TAG, "MQTTAgent_Publish failed: %d ", rc);
  }

  return(rc);
}



const char* IotGetClientId() {
  return(xCoreMqttAgentManagerGetClientId());
}

#endif