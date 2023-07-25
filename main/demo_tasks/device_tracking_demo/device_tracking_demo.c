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

/*
 * Device tracking demo app.
 */

/* Includes *******************************************************************/

/* Standard includes. */

/* FreeRTOS includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ESP-IDF includes. */
#include "esp_log.h"

/* Public functions include. */
#include "device_tracking_demo.h"

/* Demo task configurations include. */
#include "device_tracking_demo_config.h"

/* Global variables ***********************************************************/

/**
 * @brief Logging tag for ESP-IDF logging functions.
 */
static const char * TAG = "device_tracking_demo";

/* Static function definitions ************************************************/

/**
 * @brief The function that implements the task demonstrated by this file.
 */
static void prvDeviceTrackingTask( void * pvParameters )
{
    while( 1 )
    {
        ESP_LOGI( TAG, "Device Tracking demo app task loop.");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    vTaskDelete( NULL );
}

/* Public function definitions ************************************************/

void vStartDeviceTrackingDemo( void )
{
    xTaskCreate( prvDeviceTrackingTask,
                 "DeviceTracking",
                 devicetrackingconfigTASK_STACK_SIZE,
                 NULL,
                 devicetrackingconfigTASK_PRIORITY,
                 NULL );
}
