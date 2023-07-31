#include "device_tracking_config.h"
#if DT_UI_ENABLED

/*******************************************************************************************************************//**
 * User interface (LCD) functionality.
 *
 * Note this file is only in effect when the UI is enabled (DT_UI_ENABLED is 1). See ui_disabled.c for the stub
 * implementation (DT_UI_ENABLED is 0).
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
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
//#include "core2forAWS.h"
#include "device_tracking/ui.h"



/***********************************************************************************************************************
 * Globals
 **********************************************************************************************************************/

static char* TAG = "ui";

#define MAX_TEXTAREA_LENGTH 1024

static SemaphoreHandle_t xGuiSemaphore;

static lv_obj_t* obj_screen;
static lv_obj_t* obj_hdr_txt;
static lv_obj_t* obj_out_txt;
static lv_obj_t* obj_wifi_label;

static lv_obj_t* obj_btn[BTN_COUNT];
static lv_obj_t* obj_btn_txt[BTN_COUNT];
static lv_event_cb_t obj_btn_cb[BTN_COUNT];

// Obtained from lv_obj.h's definition of lv_event_t.
static const char* const LV_EVENT_NAMES[] = {
  "PRESSED",
  "PRESSING",
  "PRESS_LOST",
  "SHORT_CLICKED",
  "LONG_PRESSED",
  "LONG_PRESSED_REPEAT",
  "CLICKED",
  "RELEASED",
  "DRAG_BEGIN",
  "DRAG_END",
  "DRAG_THROW_BEGIN",
  "GESTURE",
  "KEY",
  "FOCUSED",
  "DEFOCUSED",
  "LEAVE",
  "VALUE_CHANGED",
  "INSERT",
  "REFRESH",
  "APPLY",
  "CANCEL",
  "DELETE"
};



/***********************************************************************************************************************
 * Function Definitions
 **********************************************************************************************************************/

//
// The LVGL library requires a mutex around all lv_*() function calls for thread-safety, and core2forAWS.h defines
// xGuiSemaphore for this purpose.  However, xGuiSempahore is not a recursive mutex and attempting to take it from
// an LVGL event callback blocks indefinitely.  Since any given single function lacks the broader context needed to
// know these two things, a typical take/give block can result in an early give-back or block indefinitely... Hence
// this gate that conditionally takes the semaphore.
//
// See:
// - https://docs.lvgl.io/latest/en/html/porting/os.html
// - core2forAWS.h
//
// Return Value: 'true' if the semaphore was taken and so needs to be given back; 'false' otherwise.
//

static bool SafeLock() {
  bool result = false;
  bool should_take = false;

  TaskHandle_t holder = xSemaphoreGetMutexHolder(xGuiSemaphore);

  if(NULL == holder) {
    // No task (including the current task) has the semaphore, so *do* need to take it.
    should_take = true;
  }
  else {
    TaskHandle_t me = xTaskGetCurrentTaskHandle();

    if(holder != me) {
      // Some other task (not the current task) has the semaphore, so *do* need to take it.
      should_take = true;
    }
  }

  if(should_take) {
    result = ( pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) );
  }

  return(result);
}



static void SafeUnlock(bool give) {
  if(give) {
    xSemaphoreGive(xGuiSemaphore);
  }
}



static void ui_txt_prune(lv_obj_t* txt, size_t new_text_length) {
  const char* current_text = lv_textarea_get_text(txt);
  size_t current_text_len = strlen(current_text);

  if(current_text_len + new_text_length >= MAX_TEXTAREA_LENGTH) {
    for(int i = 0; i < new_text_length; i++){
      lv_textarea_set_cursor_pos(txt, 0);
      lv_textarea_del_char_forward(txt);
    }

    lv_textarea_set_cursor_pos(txt, LV_TEXTAREA_CURSOR_LAST);
  }
}



static void ui_txt_set(lv_obj_t* txt, const char* base_txt, const char* param, size_t param_len) {
  if(base_txt != NULL) {
    bool give = SafeLock();

    if(param != NULL && param_len != 0) {
      size_t base_txtLen = strlen(base_txt);
      size_t bufLen = base_txtLen + param_len;
      char buf[(int) bufLen];
      sprintf(buf, base_txt, param);
      lv_textarea_set_text(txt, buf);
    }
    else {
      lv_textarea_set_text(txt, base_txt);
    }

    SafeUnlock(give);
  }
  else {
    ESP_LOGW(TAG, "Ignoring NULL ui text request.");
  }
}



static void ui_txt_add(lv_obj_t* txt, const char* base_txt, const char* param, size_t param_len) {
  if(base_txt != NULL) {
    bool give = SafeLock();

    if(param != NULL && param_len != 0) {
      size_t base_txtLen = strlen(base_txt);
      size_t bufLen = base_txtLen + param_len;
      char buf[(int) bufLen];
      sprintf(buf, base_txt, param);
      ui_txt_prune(txt, strlen(buf));
      lv_textarea_add_text(txt, buf);
    }
    else {
      ui_txt_prune(txt, strlen(base_txt));
      lv_textarea_add_text(txt, base_txt);
    }

    SafeUnlock(give);
  }
  else{
    ESP_LOGW(TAG, "Ignoring NULL ui text request.");
  }
}



void UiHdrTxtSet(const char* txt, const char* param, size_t param_len) {
  ui_txt_set(obj_hdr_txt, txt, param, param_len);
}



void UiOutTxtAdd(const char* txt, const char* param, size_t param_len) {
  ui_txt_add(obj_out_txt, txt, param, param_len);
}



void UiBtnTxtSet(UiButton btn, const char* txt) {
  lv_obj_t* obj = obj_btn_txt[btn];

  bool give = SafeLock();

  if(txt) {
    lv_btn_set_state(obj, LV_BTN_STATE_RELEASED);
    lv_label_set_text(obj, txt);
  }
  else {
    lv_label_set_text(obj, "");
    lv_btn_set_state(obj, LV_BTN_STATE_DISABLED);
  }

  SafeUnlock(give);
}


void UiWifiLabelUpdate(bool state) {
  bool give = SafeLock();

  if(state) {
    char buffer[25];
    sprintf(buffer, "#0000ff %s #", LV_SYMBOL_WIFI);
    lv_label_set_text(obj_wifi_label, buffer);
  }
  else{
    lv_label_set_text(obj_wifi_label, LV_SYMBOL_WIFI);
  }

  SafeUnlock(give);
}



void UiBtnEventCallbackSet(UiButton btn, UiBtnEventCallback func) {
  obj_btn_cb[btn] = func;
}


static void btn_cb(lv_obj_t* obj, lv_event_t event) {
  ESP_LOGD(TAG, "Button Event: %s", LV_EVENT_NAMES[event]);

  for(int i = 0; i < BTN_COUNT; ++i) {
     if(obj_btn[i] == obj) {
         if(obj_btn_cb[i] != NULL) {
             obj_btn_cb[i](i, event);
         }
     }
  }
}



void UiInit() {
  bool give = SafeLock();

  obj_screen = lv_scr_act();

  obj_hdr_txt = lv_textarea_create(obj_screen, NULL);
  lv_obj_set_size(obj_hdr_txt, 225, 30);
  lv_obj_align(obj_hdr_txt, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 5);
  lv_textarea_set_max_length(obj_hdr_txt, 24);
  lv_textarea_set_text_sel(obj_hdr_txt, false);
  lv_textarea_set_cursor_hidden(obj_hdr_txt, true);
  lv_textarea_set_one_line(obj_hdr_txt, true);
  lv_textarea_set_text(obj_hdr_txt, "");

  obj_out_txt = lv_textarea_create(obj_screen, NULL);
  lv_obj_set_size(obj_out_txt, 310, 150);
  lv_obj_align(obj_out_txt, NULL, LV_ALIGN_IN_TOP_LEFT, 5, 40);
  lv_textarea_set_max_length(obj_out_txt, MAX_TEXTAREA_LENGTH);
  lv_textarea_set_text_sel(obj_out_txt, false);
  lv_textarea_set_cursor_hidden(obj_out_txt, true);
  lv_textarea_set_text(obj_out_txt, "");

  obj_wifi_label = lv_label_create(obj_screen, NULL);
  lv_obj_align(obj_wifi_label, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 10);
  lv_label_set_text(obj_wifi_label, LV_SYMBOL_WIFI);
  lv_label_set_recolor(obj_wifi_label, true);

  // Shared button style.
  static lv_style_t btn_style;
  lv_style_init(&btn_style);
  lv_style_set_border_color(&btn_style, LV_STATE_DEFAULT, LV_COLOR_BLUE);

  // BTN_LEFT
  obj_btn[BTN_LEFT] = lv_btn_create(obj_screen, NULL);
  lv_obj_set_size(obj_btn[BTN_LEFT], 100, 0);
  lv_obj_align(obj_btn[BTN_LEFT], NULL, LV_ALIGN_IN_BOTTOM_LEFT, 5, -25);
  lv_btn_set_checkable(obj_btn[BTN_LEFT], false);
  lv_btn_set_fit2(obj_btn[BTN_LEFT], LV_FIT_NONE, LV_FIT_TIGHT);

  lv_obj_set_event_cb(obj_btn[BTN_LEFT], btn_cb);

  lv_obj_add_style(obj_btn[BTN_LEFT], LV_OBJ_PART_MAIN, &btn_style);
  lv_obj_refresh_style(obj_btn[BTN_LEFT], LV_OBJ_PART_MAIN, LV_STYLE_PROP_ALL);

  obj_btn_txt[BTN_LEFT] = lv_label_create(obj_btn[BTN_LEFT], NULL);
  lv_label_set_text(obj_btn_txt[BTN_LEFT], "");

  // BTN_CENTER
  obj_btn[BTN_CENTER] = lv_btn_create(obj_screen, NULL);
  lv_obj_set_size(obj_btn[BTN_CENTER], 100, 0);
  lv_obj_align(obj_btn[BTN_CENTER], NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -25);
  lv_btn_set_checkable(obj_btn[BTN_CENTER], false);
  lv_btn_set_fit2(obj_btn[BTN_CENTER], LV_FIT_NONE, LV_FIT_TIGHT);

  lv_obj_set_event_cb(obj_btn[BTN_CENTER], btn_cb);

  lv_obj_add_style(obj_btn[BTN_CENTER], LV_OBJ_PART_MAIN, &btn_style);
  lv_obj_refresh_style(obj_btn[BTN_CENTER], LV_OBJ_PART_MAIN, LV_STYLE_PROP_ALL);

  obj_btn_txt[BTN_CENTER] = lv_label_create(obj_btn[BTN_CENTER], NULL);
  lv_label_set_text(obj_btn_txt[BTN_CENTER], "");

  // BTN_RIGHT
  obj_btn[BTN_RIGHT] = lv_btn_create(obj_screen, NULL);
  lv_obj_set_size(obj_btn[BTN_RIGHT], 100, 0);
  lv_obj_align(obj_btn[BTN_RIGHT], NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -5, -25);
  lv_btn_set_checkable(obj_btn[BTN_RIGHT], false);
  lv_btn_set_fit2(obj_btn[BTN_RIGHT], LV_FIT_NONE, LV_FIT_TIGHT);

  lv_obj_set_event_cb(obj_btn[BTN_RIGHT], btn_cb);

  lv_obj_add_style(obj_btn[BTN_RIGHT], LV_OBJ_PART_MAIN, &btn_style);
  lv_obj_refresh_style(obj_btn[BTN_RIGHT], LV_OBJ_PART_MAIN, LV_STYLE_PROP_ALL);

  obj_btn_txt[BTN_RIGHT] = lv_label_create(obj_btn[BTN_RIGHT], NULL);
  lv_label_set_text(obj_btn_txt[BTN_RIGHT], "");

  SafeUnlock(give);
}

#endif