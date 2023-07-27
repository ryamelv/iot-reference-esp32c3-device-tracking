#pragma once

/*******************************************************************************************************************//**
 * User interface (LCD) functionality
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include <stddef.h>
#include <stdbool.h>
#include "device_tracking/device_tracking_config.h"

// Avoid dependency on LVGL if user interface is disabled.
#if DT_UI_ENABLED
  #include "lvgl.h"
#else
  typedef void lv_obj_t;
  typedef void* lv_event_t;
  #define LV_EVENT_PRESSED 0
#endif



/***********************************************************************************************************************
 * Type Definitions
 **********************************************************************************************************************/

/// There are three buttons on this application-specific user interface.
typedef enum UiButton {
  BTN_LEFT,
  BTN_CENTER,
  BTN_RIGHT,
  BTN_COUNT      // Not a button; indicates number of buttons.
} UiButton;

/// Button activation callback.
typedef void (*UiBtnEventCallback)(UiButton btn, lv_event_t event);



/***********************************************************************************************************************
 * Function Declarations
 **********************************************************************************************************************/

void UiInit();

void UiBtnEventCallbackSet(UiButton btn, UiBtnEventCallback func);

void UiHdrTxtSet(const char* txt, const char* param, size_t param_len);
void UiOutTxtAdd(const char* txt, const char* param, size_t param_len);
void UiBtnTxtSet(UiButton btn, const char* txt);

void UiWifiLabelUpdate(bool state);
