#include "device_tracking_config.h"
#if !DT_UI_ENABLED

/*******************************************************************************************************************//**
 * User interface (LCD) functionality.
 *
 * Note this file is only in effect when the UI is disabled (DT_UI_ENABLED is 0). This file provides minimal
 * function definitions to satisfy building without a user interface. See ui_enabled.c for the full user interface
 * implementation (DT_UI_ENABLED is 1).
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Copyright Amazon.com, Inc. and its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT
 **********************************************************************************************************************/



/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/

#include "device_tracking/ui.h"



/***********************************************************************************************************************
 * Function Definitions
 **********************************************************************************************************************/

void UiBtnEventCallbackSet(UiButton, UiBtnEventCallback) {
}

void UiHdrTxtSet(const char*, const char*, size_t) {
}

void UiOutTxtAdd(const char*, const char*, size_t) {
}

void UiBtnTxtSet(UiButton, const char*) {
}

void UiWifiLabelUpdate(bool) {
}

void UiInit() {
}

#endif