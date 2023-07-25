/*
 * ESP32-C3 Featured FreeRTOS IoT Integration V202204.00
 * Copyright (C) 2023 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

#ifndef DEVICE_TRACKING_DEMO_CONFIG_H
#define DEVICE_TRACKING_DEMO_CONFIG_H

/* ESP-IDF sdkconfig include. */
#include <sdkconfig.h>

#if CONFIG_GRI_RUN_QUALIFICATION_TEST
    #include "qualification_wrapper_config.h"
#endif /* CONFIG_GRI_RUN_QUALIFICATION_TEST */

/**
 * @brief The QoS level of MQTT messages sent by this demo. This must be 0 or 1
 * if using AWS as AWS only supports levels 0 or 1. If using another MQTT broker
 * that supports QoS level 2, this can be set to 2.
 */
#define devicetrackingconfigQOS_LEVEL                               ( ( unsigned long ) ( CONFIG_GRI_DEVICE_TRACKING_DEMO_QOS_LEVEL ) )

/**
 * @brief The task priority of each of the SubPubUnsub tasks.
 */
#define devicetrackingconfigTASK_PRIORITY                           ( ( unsigned int ) ( CONFIG_GRI_DEVICE_TRACKING_DEMO_TASK_PRIORITY ) )

/**
 * @brief The task stack size for each of the SubPubUnsub tasks.
 */
#define devicetrackingconfigTASK_STACK_SIZE                         ( ( unsigned int ) ( CONFIG_GRI_DEVICE_TRACKING_DEMO_TASK_STACK_SIZE ) )

#endif /* DEVICE_TRACKING_DEMO_CONFIG_H */
