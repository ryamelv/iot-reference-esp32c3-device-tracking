/*******************************************************************************************************************//**
 * Device tracking app
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "core_mqtt.h"
#include "core_mqtt_agent_manager.h"
#include "core_mqtt_agent_manager_events.h"
#include "core2forAWS.h"
#include "device_tracking/device_tracking_config.h"
#include "device_tracking/iot.h"
#include "device_tracking/ui.h"

#if DT_SNTP_INIT
  #include "esp_netif_sntp.h"
#endif



/***********************************************************************************************************************
 * Globals
 **********************************************************************************************************************/

// Logging identifier for this module.
static const char *TAG = "device_tracking";

// GPS location point.
struct GpsPoint {
  time_t sampleTime;
  double lon;
  double lat;
};

// GPS state.
bool g_gps_mock = DT_GPS_MOCK_DEFAULT;
enum MockScale g_gps_mockScale = DT_GPS_MOCK_SCALE_DEFAULT;

// Local buffer/queue for GPS points to upload to AWS IoT.
QueueHandle_t g_gps_points_queue;

// Optionally pause GPS point production (perhaps while out of WiFi range).
bool g_paused = false;

// AWS IoT MQTT topic ("<client_id>/location"). Only valid after init().
#define MQTT_TOPIC_NAME_LEN (128)
char g_mqtt_topic_name[MQTT_TOPIC_NAME_LEN] = "<UNK>";

static MQTTAgentContext_t* g_mqtt_agent = NULL;
static EventGroupHandle_t mqtt_agent_event_group = NULL;

#define CORE_MQTT_AGENT_CONNECTED_BIT              ( 1 << 0 )
#define CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT    ( 1 << 1 )



/***********************************************************************************************************************
 * Function Definitions
 **********************************************************************************************************************/

static void OnMqttAgentEvent(void* pvHandlerArg, esp_event_base_t xEventBase, int32_t lEventId, void* pvEventData) {
  switch(lEventId) {
    case CORE_MQTT_AGENT_CONNECTED_EVENT:
      ESP_LOGI(TAG, "MQTT Agent Connected Event");
      xEventGroupSetBits(mqtt_agent_event_group, CORE_MQTT_AGENT_CONNECTED_BIT);
      break;

    case CORE_MQTT_AGENT_DISCONNECTED_EVENT:
      ESP_LOGI(TAG, "MQTT Agent Disconnected Event: Pausing device tracking");
      xEventGroupClearBits( mqtt_agent_event_group, CORE_MQTT_AGENT_CONNECTED_BIT );
      break;

    case CORE_MQTT_AGENT_OTA_STARTED_EVENT:
      ESP_LOGI(TAG, "MQTT Agent OTA Started Event: Pausing device tracking");
      xEventGroupClearBits(mqtt_agent_event_group, CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT);
      break;

    case CORE_MQTT_AGENT_OTA_STOPPED_EVENT:
      ESP_LOGI(TAG, "MQTT Agent OTA Stopped Event");
      xEventGroupSetBits(mqtt_agent_event_group, CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT);
      break;

    default:
      ESP_LOGE(TAG, "Unknown MQTT Agent event: %"PRIu32"", lEventId);
      break;
  }
}



static void WaitForMqttAgent() {
  ESP_LOGD(TAG, "Waiting for MQTT Agent to be ready to xmit...");

  xEventGroupWaitBits(
      mqtt_agent_event_group,
      CORE_MQTT_AGENT_CONNECTED_BIT | CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT,
      pdFALSE,            // do not clear bits
      pdTRUE,           // wait for all bits
      portMAX_DELAY);    // wait forever

  ESP_LOGD(TAG, "...MQTT Agent ready.");
}



static void WaitForTimeSync() {
  #if DT_SNTP_INIT
  ESP_LOGI(TAG, "Initializing SNTP service with '%s'", DT_SNTP_SERVERNAME);
  esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(DT_SNTP_SERVERNAME);
  esp_netif_sntp_init(&sntp_config);
  setenv("TZ", "UTC+0", 1);
  tzset();
  #endif

  ESP_LOGI(TAG, "Waiting up to %d seconds for SNTP sync...", DT_SNTP_SYNC_WAIT_IN_SEC);

  if(ESP_OK != esp_netif_sntp_sync_wait(pdMS_TO_TICKS(DT_SNTP_SYNC_WAIT_IN_SEC * 1000))) {
    ESP_LOGW(TAG, "...Timeout waiting for SNTP sync; proceeding anyway (likely with inaccurate time).");
  } else {
    ESP_LOGI(TAG, "...SNTP sync complete.");
  }

  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char now_buf[64];
  strftime(now_buf, sizeof(now_buf), "%a %b %d %X %Y %Z", &timeinfo);
  ESP_LOGI(TAG, "Current date/time: %s", now_buf);
}



static void CheckTaskStackUsage() {
  const char* const taskName = pcTaskGetName(NULL);

  const UBaseType_t taskStackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  const UBaseType_t inBytes = (taskStackHighWaterMark * 4);                        // Assuming 32-bit CPU

  ESP_LOGI(TAG, "Task '%s' min unused stack: %d bytes", taskName, inBytes);

  if(1024 > inBytes) {
    ESP_LOGW(TAG, "Task '%s' stack may be undersized.", taskName);
  }
}



static void GetMockGpsPoint(struct GpsPoint* gps_point) {
  // Read current accelerometer hardware values.
  float xA = 0, yA = 0, zA = 0;

  MPU6886_GetAccelData(&xA, &yA, &zA);

  gps_point->sampleTime = time(NULL);

  // Calibrate (see main.h).
  float xAc = xA + DT_GPS_MOCK_ACCEL_OFFSET_X, yAc = yA + DT_GPS_MOCK_ACCEL_OFFSET_Y;

  // Round to tenths to remove jitter.
  float xAr = round(10 * xAc) / 10, yAr = round(10 * yAc) / 10;

  // Normalize per number of samples per second, to make impact independent of sample rate.
  xAr = ( xAr * (DT_GPS_POINT_PERIOD_IN_MS / 1000.0) );
  yAr = ( yAr * (DT_GPS_POINT_PERIOD_IN_MS / 1000.0) );

  // Consider each value (about -1.0 to 1.0) a percentage of jogging speed (6 MPH = 8.8 fps).
  float xIncD = xAr * 8.8;
  float yIncD = yAr * 8.8;

  // Scale by walking/driving/flying (see main.h).
  xIncD *= g_gps_mockScale;
  yIncD *= g_gps_mockScale;

  // Accumulate absolute distance from starting point.
  static double xD = 0, yD = 0;
  xD += xIncD;
  yD += yIncD;

  // Convert feet to GPS.  Conversion factor is a very rough estimate.
  double gpsLat = DT_GPS_MOCK_START_LAT, gpsLon = DT_GPS_MOCK_START_LON;
  const double ftToGpsConv = 0.000002160039;
  gpsLat -= (yD * ftToGpsConv); gpsLon -= (xD * ftToGpsConv);

  ESP_LOGD(TAG, "Raw: %+.2f/%+.2f | Calib: %+.2f/%+.2f | Rounded: %+.2f/%+.2f | Dist: %+.0lf/%+.0lf | GPS: %.6lf/%.6lf",
    xA, yA, xAc, yAc, xAr, yAr, xD, yD, gpsLat, gpsLon);

  gps_point->lat = gpsLat;
  gps_point->lon = gpsLon;
}



static void GetGpsPoint(struct GpsPoint* gps_point) {
  if(g_gps_mock) {
    GetMockGpsPoint(gps_point);
  }
  else {
    ESP_LOGW(TAG, "Non-Mock GPS points not implemented yet!");
    gps_point->sampleTime = time(NULL);
    gps_point->lat = DT_GPS_MOCK_START_LAT;
    gps_point->lon = DT_GPS_MOCK_START_LON;
  }
}



static int GetProduceLoopsPerGpsPoint() {
  static const uint32_t MOCK_LOOPS_PER_GPS_POINT = DT_GPS_POINT_PERIOD_IN_MS / DT_GPS_MOCK_CALC_PERIOD_IN_MS;
  return( g_gps_mock ? MOCK_LOOPS_PER_GPS_POINT : 1 );
}




static void ProduceGpsPointsTask(void* param) {
  bool giveUp = false;
  BaseType_t rc = pdFALSE;
  TickType_t xWakePeriod = pdMS_TO_TICKS(DT_GPS_POINT_PERIOD_IN_MS);
  struct GpsPoint gps_point = {0};

  // Accurate time is needed to timestamp the GPS points.
  WaitForTimeSync();

  // Don't bother producing GPS points until the MQTT Agent is ready for the first time.
  WaitForMqttAgent();

  // vTaskDelayUntil() below requires an initial starting time.
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Some complexity for mock mode due to looping faster than GPS point production period.
  for(int loops = 0; !giveUp; loops = (loops + 1) % GetProduceLoopsPerGpsPoint()) {
    // Pause here to produce GPS points at a given frequency.
    vTaskDelayUntil(&xLastWakeTime, xWakePeriod / GetProduceLoopsPerGpsPoint());

    GetGpsPoint(&gps_point);

    // If mocking GPS points, only produce at the desired upload rate despite calculating (looping) more frequently.
    if(0 == loops && !g_paused) {
      ESP_LOGD(TAG, "Producing GPS Point: %lld [%lf, %lf]", gps_point.sampleTime, gps_point.lon, gps_point.lat);

      // Store to queue.
      rc = xQueueSendToBack(g_gps_points_queue, &gps_point, 0);

      if(pdTRUE != rc){
        ESP_LOGW(TAG, "GPS points queue full; discarding GPS point.");
      }

      CheckTaskStackUsage();
    }
  }

  ESP_LOGE(TAG, "Fatal error in produce_gps_points task.");

  vTaskDelete(NULL);
}



static MQTTStatus_t UploadOneGpsPoint(IotContext* iot_context, struct GpsPoint* gps_point) {
  static const char example[] =
    "{ 'SampleTime': 1652985753, 'Position': [ -93.274963, 44.984379 ] }";

  static char msgBuf[sizeof(example) * 2];

  sprintf(msgBuf, "{ \"SampleTime\": %lld, \"Position\": [ %lf, %lf ] }",
    gps_point->sampleTime, gps_point->lon, gps_point->lat);

  MQTTStatus_t rc = IotPublish(iot_context, g_mqtt_topic_name, msgBuf);

  return(rc);
}



static void UploadGpsPointsTask(void* param) {
  bool giveUp = false;
  BaseType_t rc = pdFALSE;
  MQTTStatus_t iot_rc = MQTTSuccess;
  struct GpsPoint gps_point = {0};

  while(!giveUp) {
    // Read from queue. Must wake periodically to yield (see below).
    const TickType_t xBlockTime = pdMS_TO_TICKS(10000);
    rc = xQueuePeek(g_gps_points_queue, &gps_point, xBlockTime);

    // Message received; upload to AWS IoT.
    if(pdTRUE == rc) {

      // Do not pummel the MQTT Agent with messages if it is not in a state to xmit.
      WaitForMqttAgent();

      iot_rc = UploadOneGpsPoint(g_mqtt_agent, &gps_point);

      if(MQTTSuccess == iot_rc) {
        // Above only peeked; remove sent message from queue.
        rc = xQueueReceive(g_gps_points_queue, &gps_point, 0);

        if(pdTRUE != rc) {
          ESP_LOGW(TAG, "xQueueReceive() failed: %d ", rc);
        }
      }
    }

    // AWS IoT Client requires periodic thread time to manage the AWS IoT connection and receive messages.
    //iot_rc = aws_iot_mqtt_yield(&aws_iot_client, 100);

    if(MQTTSuccess != iot_rc) {
        ESP_LOGW(TAG, "aws_iot_mqtt_yield() returned: %d ", iot_rc);
    }

    CheckTaskStackUsage();
  }

  ESP_LOGE(TAG, "Fatal error in upload_gps_points task.");

  vTaskDelete(NULL);
}



static const char* MockModeText() {
  return( g_gps_mock ? "Mock" : "GPS" );
}



static const char* MockScaleText () {
  return( (WALKING == g_gps_mockScale) ? "Walk" : (DRIVING == g_gps_mockScale) ? "Drive" : "Fly" );
}



static const char* PausedText() {
  return( g_paused ? "Paused" : "Active" );
}



static void OnBtnEvent(UiButton btn, lv_event_t event) {
  if(LV_EVENT_PRESSED == event) {
    if(BTN_LEFT == btn) {
      g_gps_mock = !g_gps_mock;
      UiBtnTxtSet(btn, MockModeText());
    }
    else if(BTN_CENTER == btn) {
      g_gps_mockScale = (WALKING == g_gps_mockScale) ? DRIVING : (DRIVING == g_gps_mockScale) ? FLYING : WALKING;
      UiBtnTxtSet(btn, MockScaleText());
    }
    else if(BTN_RIGHT == btn) {
      g_paused = !g_paused;
      UiBtnTxtSet(btn, PausedText());
    }
  }
}



static bool DeviceTrackingInit(MQTTAgentContext_t* mqtt_agent_context) {
  g_mqtt_agent = mqtt_agent_context;

  Core2ForAWS_Init();
  //Core2ForAWS_Display_SetBrightness(80);
  Core2ForAWS_LED_Enable(1);

  // MQTT topic name
  sprintf(g_mqtt_topic_name, "%s%s", IotGetClientId(), DT_MQTT_PUBLISH_TOPIC_POSTFIX);

  // display
  UiHdrTxtSet("ID: %s", IotGetClientId(), strlen(IotGetClientId()));
  UiOutTxtAdd("Device Tracking\n", NULL, 0);

  // buttons
  UiBtnTxtSet(BTN_LEFT, MockModeText());
  UiBtnTxtSet(BTN_CENTER, MockScaleText());
  UiBtnTxtSet(BTN_RIGHT, PausedText());

  UiBtnEventCallbackSet(BTN_LEFT, OnBtnEvent);
  UiBtnEventCallbackSet(BTN_CENTER, OnBtnEvent);
  UiBtnEventCallbackSet(BTN_RIGHT, OnBtnEvent);

  return(true);
}



BaseType_t DeviceTrackingLaunch(MQTTAgentContext_t* mqtt_agent_context) {
  BaseType_t rc = pdPASS;

  ESP_LOGI(TAG, "Launching Device Tracking app...");
  ESP_LOGI(TAG, "AWS IoT SDK coreMQTT version: %s", MQTT_LIBRARY_VERSION);
  ESP_LOGI(TAG, "AWS IoT Client ID: %s", IotGetClientId());

  if(!DeviceTrackingInit(mqtt_agent_context)) {
    return(pdFAIL);
  }

  // Create event group for MQTT agent events.

  mqtt_agent_event_group = xEventGroupCreate();

  if(NULL == mqtt_agent_event_group) {
    ESP_LOGE(TAG, "Failed to create MQTT Agent event group.");
    return(pdFAIL);
  }

  xEventGroupSetBits(mqtt_agent_event_group, CORE_MQTT_AGENT_OTA_NOT_IN_PROGRESS_BIT);

  if(pdPASS != xCoreMqttAgentManagerRegisterHandler(OnMqttAgentEvent)) {
    ESP_LOGE(TAG, "Failed to register MQTT Agent event handler.");
    return(pdFAIL);
  }

  // Create a local buffer/queue for GPS points to then upload to AWS IoT (the math rounds up).

  UBaseType_t gpsPointQueueLength =
      ( DT_GPS_POINT_BUFFER_DURATION_IN_MS + (DT_GPS_POINT_PERIOD_IN_MS - 1) ) / DT_GPS_POINT_PERIOD_IN_MS;

  ESP_LOGI(TAG,
      "Creating GPS points queue with depth %d items = %lu min...",
      gpsPointQueueLength, DT_GPS_POINT_BUFFER_DURATION_IN_MIN);

  g_gps_points_queue = xQueueCreate(gpsPointQueueLength, sizeof(struct GpsPoint));

  if(NULL == g_gps_points_queue) {
    ESP_LOGE(TAG, "Failed to create GPS points queue.");
    return(pdFAIL);
  }

  // Create the task that produces (acquires from the hardware GPS module or mocks) GPS points into the queue.

  ESP_LOGI(TAG, "Creating task to produce GPS points...");

  rc = xTaskCreate(ProduceGpsPointsTask, "ProduceGpsPoints",
      DT_TASK_STACK_WORDS, NULL, DT_TASK_PRIORITY_PRODUCER, NULL);

  if(pdPASS != rc) {
    ESP_LOGE(TAG, "Failed to create task to produce GPS points: %d", rc);
    return(rc);
  }

  // Create the task that uploads GPS points from the queue to AWS IoT.

  ESP_LOGI(TAG, "Creating task to publish GPS points...");

  rc = xTaskCreate(UploadGpsPointsTask, "UploadGpsPoints",
      DT_TASK_STACK_WORDS, NULL, DT_TASK_PRIORITY_UPLOAD, NULL);

  if(pdPASS != rc) {
    ESP_LOGE(TAG, "Failed to create task to upload GPS points: %d", rc);
    return(rc);
  }

  return(rc);
}
