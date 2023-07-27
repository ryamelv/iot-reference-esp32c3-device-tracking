#pragma once

/*******************************************************************************************************************//**
 * Device tracking app configuration.
 *
 * This is the primary location to configure this app at build-time. The values are used by many/all of the device
 * tracking app source files in the same directory.
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "core_mqtt_serializer.h"



/***********************************************************************************************************************
 * Configuration
 **********************************************************************************************************************/

//
// Task Configuration
//

static const uint16_t DT_TASK_PRIORITY_PRODUCER = CONFIG_GRI_DEVICE_TRACKING_DEMO_PRODUCER_TASK_PRIORITY;
static const uint16_t DT_TASK_PRIORITY_UPLOAD = CONFIG_GRI_DEVICE_TRACKING_DEMO_UPLOAD_TASK_PRIORITY;
static const uint16_t DT_TASK_STACK_WORDS = CONFIG_GRI_DEVICE_TRACKING_DEMO_TASK_STACK_SIZE;



//
// Simple Network Time Protocol (SNTP) Configuration
//

// Optionally have this app initialize the system SNTP service.
#define DT_SNTP_INIT 1

static const char* const DT_SNTP_SERVERNAME = "pool.ntp.org";
static const uint16_t DT_SNTP_SYNC_WAIT_IN_SEC = 60;



//
// AWS IoT (MQTT) Configuration
//

// Use an external MQTT Agent supplied to DeviceTrackingLaunch(), or have this app use coreMQTT stand-alond.
#define DT_IOT_AGENT 1

// MQTT QoS used when publishing messages.
static const MQTTQoS_t DT_MQTT_QOS = CONFIG_GRI_DEVICE_TRACKING_DEMO_QOS_LEVEL;

// Postfix of MQTT topic to which GPS points are published (results in "<client_id>/<postfix>").
static const char* const DT_MQTT_PUBLISH_TOPIC_POSTFIX = "/location";



//
// User Interface Configuration
//

#define DT_UI_ENABLED 0



//
// GPS Configuration
//

// Cadence of sampling GPS location points for upload to AWS IoT, in milliseconds.
static const uint32_t DT_GPS_POINT_PERIOD_IN_MS = 10000;

// GPS location point buffer size, in minutes. Used when, for example, network connection is down.
static const uint32_t DT_GPS_POINT_BUFFER_DURATION_IN_MIN = 10;

// Calculated milliseconds version of GPS_POINT_BUFFER_DURATION_IN_MIN.
static const uint32_t DT_GPS_POINT_BUFFER_DURATION_IN_MS = DT_GPS_POINT_BUFFER_DURATION_IN_MIN * 60 * 1000;

// Mocking is smoother if accelerometer is sampled quickly - faster than the desired GPS point upload rate.
// Should be an even divisor of GPS_POINT_PERIOD_IN_MS above for accurate smoothing.
static const uint32_t DT_GPS_MOCK_CALC_PERIOD_IN_MS = 50;

// Mock GPS is not absolute; it must be relative to a given starting point.
//const double DT_GPS_MOCK_START_LAT = 44.98421;     // Amazon MSP12
//const double DT_GPS_MOCK_START_LON = -93.27502;
static const double DT_GPS_MOCK_START_LAT = 44.81584;
static const double DT_GPS_MOCK_START_LON = -93.63705;

// Whether to mock GPS points (i.e., 'drive' based on tilting the device) or use GPS hardware module accessory.
static const bool DT_GPS_MOCK_DEFAULT = true;

// Mock GPS movement scale (multiplier of tilt angle to velocity).
enum MockScale {
  WALKING = 1,      // ~6 MPH max
  DRIVING = 10,     // ~60 MPH max
  FLYING  = 100     // ~600 MPH max
};

static const enum MockScale DT_GPS_MOCK_SCALE_DEFAULT = DRIVING;

// Optionally apply an offset to accelerometer values read from hardware (ex: compensate for uneven work surface).
static const float DT_GPS_MOCK_ACCEL_OFFSET_X = 0.05;
static const float DT_GPS_MOCK_ACCEL_OFFSET_Y = 0.00;
