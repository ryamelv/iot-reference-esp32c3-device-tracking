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
#include "lvgl/lvgl.h"
#include "device_tracking/device_tracking_config.h"



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
