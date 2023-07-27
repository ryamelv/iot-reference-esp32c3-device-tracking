#include "device_tracking_config.h"
#if !DT_IOT_AGENT

/*******************************************************************************************************************//**
 * IoT functionality.
 *
 * Note this file is only in effect when the IoT Agent is disabled (DT_IOT_AGENT is 0), which means the device
 * tracking app must use the standalone core MQTT library. See iot_agent.c for when an externally managed IoT
 * Agent is in effect (DT_UI_AGENT is 1).
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
#include "sdkconfig.h"
#include "device_tracking/iot.h"
#include "device_tracking/ui.h"



/***********************************************************************************************************************
 * Globals
 **********************************************************************************************************************/

// Logging identifier for this module.
static const char* TAG = "iot";

// Use identifiers from aws_iot_config.h, but AWS account/region specific values ultimately come from sdkconfig.
// Need a char buffer only because IoT_Client_Init_Params.pHostURL requires a (non-cost) char*.
char awsIotMqttHostUrl[] = CONFIG_GRI_MQTT_ENDPOINT;
const uint16_t awsIotMqttHostPort = CONFIG_GRI_MQTT_PORT;

// AWS Root Certificate Authority certificate.
extern const char aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");



/***********************************************************************************************************************
 * Function Definitions
 **********************************************************************************************************************/

void IotDisconnectHandler(IotContext* iot_context, void* data) {
  ESP_LOGW(TAG, "AWS IoT Client Disconnect; Auto-reconnecting...");
}



MQTTStatus_t IotInit(IotContext* iot_context) {
  MQTTStatus_t rc = MQTTSuccess;

  IoT_Client_Init_Params initParams = iotClientInitParamsDefault;

  initParams.pHostURL = awsIotMqttHostUrl;
  initParams.port = awsIotMqttHostPort;

  initParams.pRootCALocation = aws_root_ca_pem_start;

  initParams.pDeviceCertLocation = "#";
  initParams.pDevicePrivateKeyLocation = "#0";
  initParams.isSSLHostnameVerify = true;

  initParams.enableAutoReconnect = false;       // Auto-connect is later enabled explicitly after first connect.
  initParams.mqttCommandTimeout_ms = 20000;
  initParams.tlsHandshakeTimeout_ms = 5000;

  initParams.disconnectHandler = IotDisconnectHandler;
  initParams.disconnectHandlerData = NULL;

  // aws_iot_mqtt_init() makes its own copy of the init params.
  IoT_Error_t rc = aws_iot_mqtt_init(iot_context, &initParams);

  if(SUCCESS != rc) {
    ESP_LOGE(TAG, "aws_iot_mqtt_init() error: %d ", rc);
  }

  return(rc);
}



MQTTStatus_t IotConnect(IotContext* iot_context, const char* client_id) {
  MQTTStatus_t rc = MQTTSuccess;

  IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

  connectParams.MQTTVersion = MQTT_3_1_1;

  connectParams.pClientID = cient_id;
  connectParams.clientIDLen = strlen(client_id);

  connectParams.isCleanSession = true;
  connectParams.isWillMsgPresent = false;
  connectParams.keepAliveIntervalInSec = 10;

  ui_out_txt_add("Connecting to AWS IoT Core...\n", NULL, 0);

  IoT_Error_t rc = SUCCESS;

  do {
    ESP_LOGI(TAG, "Connecting to AWS IoT Core at %s:%d...", awsIotMqttHostUrl, awsIotMqttHostPort);

    rc = aws_iot_mqtt_connect(iot_context, &connectParams);

    if(SUCCESS != rc) {
      ESP_LOGE(TAG, "aws_iot_mqtt_connect() error: %d ", rc);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

  } while(SUCCESS != rc);

  ui_out_txt_add("Connected to AWS IoT Core.\n", NULL, 0);
  ESP_LOGI(TAG, "Connected to AWS IoT Core.");

  // Enable auto-reconnect (must be done after first connect).
  rc = aws_iot_mqtt_autoreconnect_set_status(iot_context, true);

  if(SUCCESS != rc) {
    ESP_LOGE(TAG, "aws_iot_mqtt_autoreconnect_set_status() error: %d ", rc);
  }

  // Return value is based on connection, not enabling auto-reconnect.
  return(SUCCESS);
}



MQTTStatus_t IotPublsh(IotContext* iot_context, const char* topic, const char* msg) {
  ESP_LOGI(TAG, "Publishing MQTT Message: [%s] %s", topic, msg);

  MQTTPublishInfo_t pub_info = {0};

  pub_info.qos             = MQTTQoS1;
  pub_info.retain          = false;
  pub_info.dup             = false;
  pub_info.pTopicName      = topic;
  pub_info.topicNameLength = (uint16_t) strlen(pub_info.pTopicName);
  pub_info.pPayload        = msg;
  pub_info.payloadLength   = (uint16_t) strlen(pub_info.pPayload);

  MQTTAgentCommandContext_t cmd_cxt = {0};
  cmd_cxt.xTaskToNotify = xTaskGetCurrentTaskHandle();

  MQTTAgentCommandInfo_t cmd_info = {0};

  cmd_info.blockTimeMs = 500;
  cmd_info.cmdCompleteCallback = prvPublishCommandCallback;
  cmd_info.pCmdCompleteCallbackContext = &cmd_ctx;

  MQTTStatus_t rc = MQTTAgent_Publish(iot_context, &pub_info, &cmd_info );

  if(MQTTSuccess != rc) {
    ESP_LOGW(TAG, "aws_iot_mqtt_publish() error: %d ", rc);
  }

  return(rc);
}



const char* IotGetClientId() {
  return(client_id);
}

#endif