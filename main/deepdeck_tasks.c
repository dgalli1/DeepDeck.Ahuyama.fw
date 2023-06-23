/**
 * @file deepdeck_tasks.c
 * @author ElectroNick (nick@dsd.dev)
 * @brief source file of the deepdeck tasks
 * @version 0.1
 * @date 2022-12-08
 * 
 * @copyright Copyright (c) 2022 
 * 
 */

#include "deepdeck_tasks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"

//MK32 functions
#include "keypress_handles.c"
#include "nvs_funcs.h"

#include "esp_err.h"

#include "rotary_encoder.h"
#include "rgb_led.h"
#include "menu.h"


#include "keycode_conv.h"
#include "gesture_handles.h"
#include "wifi_handles.h"

static const char *TAG = "KeyReport";

#define KEY_REPORT_TAG "KEY_REPORT"
#define SYSTEM_REPORT_TAG "KEY_REPORT"
// #define TRUNC_SIZE 20
#define USEC_TO_SEC 1000000
#define SEC_TO_MIN 60

#ifdef OLED_ENABLE
TaskHandle_t xOledTask;
#endif

TaskHandle_t xKeyreportTask;

//extern SemaphoreHandle_t xSemaphore;

/**
 * @todo look a better way to handle the deepsleep flag.
 * 
 */
bool DEEP_SLEEP = true; // flag to check if we need to go to deep sleep

void oled_task(void *pvParameters) {
	deepdeck_status = S_NORMAL;
	bool CON_LOG_FLAG = false; // Just because I don't want it to keep logging the same thing a billion times
	while (1) {
		switch (deepdeck_status) {
		case S_NORMAL:
			if (!wifiIsConnected()) { //@todo this was a check for ble should be mqtt
				if (CON_LOG_FLAG == false) {
					ESP_LOGI(TAG, "Not connected, waiting for connection ");
				}
				waiting_oled();
				DEEP_SLEEP = false;
				CON_LOG_FLAG = true;
			} else {
				update_oled();
				CON_LOG_FLAG = false;
			}
			break;
		case S_SETTINGS:
			vTaskDelay(pdMS_TO_TICKS(200));

			menu_screen();

			deepdeck_status = S_NORMAL;
			//release gesture sensor again
			apds9960_free();
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(250));
	}

}

void gesture_task(void *pvParameters) {

	while (true) {

		if ( xSemaphoreTake( xSemaphore, 10 ) == pdTRUE) {

			//Do not send anything if queues are uninitialized
			if (keyboard_q == NULL) {
				ESP_LOGE(TAG, "queues not initialized");
				continue;
			}

//			ESP_LOGI("Gesture", "xSemaphore Take");
			do{
				vTaskDelay(pdMS_TO_TICKS(10));
			}while(deepdeck_status == S_SETTINGS);
			read_gesture();

		} else {
			vTaskDelay(pdMS_TO_TICKS(10));
		}

		vTaskDelay(pdMS_TO_TICKS(250));

	}

}


void key_reports(void *pvParameters) {
	// Arrays for holding the report at various stages
	uint8_t past_report[REPORT_LEN] = { 0 };
	uint8_t report_state[REPORT_LEN];

	while (1) {
		memcpy(report_state, check_key_state(&key_layouts[current_layout]),
				sizeof report_state);

		//Do not send anything if queues are uninitialized
		if (mouse_q == NULL || keyboard_q == NULL) {
			ESP_LOGE(TAG, "queues not initialized");
			continue;
		}

		//Check if the report was modified, if so send it
		if (memcmp(past_report, report_state, sizeof past_report) != 0) {
			DEEP_SLEEP = false;
			void *pReport;
			memcpy(past_report, report_state, sizeof past_report);

#ifndef NKRO
			uint8_t trunc_report[REPORT_LEN] = { 0 };
			trunc_report[0] = report_state[0];
			trunc_report[1] = report_state[1];

			uint16_t cur_index = 2;
			//Phone's mtu size is usuaully limited to 20 bytes
			for (uint16_t i = 2; i < REPORT_LEN && cur_index < TRUNC_SIZE;
					++i) {
				if (report_state[i] != 0) {
					trunc_report[cur_index] = report_state[i];
					++cur_index;
				}
			}

			pReport = (void*) &trunc_report;
#endif
#ifdef NKRO
			pReport = (void *) &report_state;
#endif

			if (BLE_EN == 1) {
				// @todo send command to mqtt?
				// xQueueSend(keyboard_q, pReport, (TickType_t ) 0);

			}
			if (input_str_q != NULL) {
				xQueueSend(input_str_q, pReport, (TickType_t ) 0);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}

}

void rgb_leds_task(void *pvParameters) {

	rgb_key_led_init();
	rgb_notification_led_init();
	while (1) {
		key_led_modes();
	}
}

void encoder_report(void *pvParameters) {
	uint8_t encoder1_status = 0;
	uint8_t encoder2_status = 0;
	uint8_t past_encoder1_state = 0;
	uint8_t past_encoder2_state = 0;

	rotary_encoder_t *encoder_a = NULL;
	rotary_encoder_t *encoder_b = NULL;

	//--------------------Start encoder 1---------------
	// Rotary encoder underlying device is represented by a PCNT unit in this example
	uint32_t pcnt_unit_a = 0;

	// Create rotary encoder instance
	rotary_encoder_config_t config_a =
					ROTARY_ENCODER_DEFAULT_CONFIG((rotary_encoder_dev_t)pcnt_unit_a, ENCODER1_A_PIN, ENCODER1_B_PIN, ENCODER1_S_PIN, ENCODER1_S_ACTIVE_LOW);
	ESP_ERROR_CHECK(rotary_encoder_new_ec11(&config_a, &encoder_a));

	// Filter out glitch (1us)
	ESP_ERROR_CHECK(encoder_a->set_glitch_filter(encoder_a, 1));

	// Start encoder
	ESP_ERROR_CHECK(encoder_a->start(encoder_a));

	//--------------------Start encoder 2---------------
	uint32_t pcnt_unit_b = 1;

	// Create rotary encoder instance
	rotary_encoder_config_t config_b =
					ROTARY_ENCODER_DEFAULT_CONFIG((rotary_encoder_dev_t)pcnt_unit_b, ENCODER2_A_PIN, ENCODER2_B_PIN, ENCODER2_S_PIN, ENCODER2_S_ACTIVE_LOW);
	ESP_ERROR_CHECK(rotary_encoder_new_ec11(&config_b, &encoder_b));

	// Filter out glitch (1us)
	ESP_ERROR_CHECK(encoder_b->set_glitch_filter(encoder_b, 1));

	// Start encoder
	ESP_ERROR_CHECK(encoder_b->start(encoder_b));

	while (1) {
		encoder1_status = encoder_state(encoder_a);

		if (encoder1_status != past_encoder1_state) {
			//EEP_SLEEP = false;
			// Check if both encoder are pushed, to enter settings mode.

			if (deepdeck_status == S_SETTINGS) {

				menu_command((encoder_state_t) encoder1_status);

			} else if (encoder1_status == ENC_BUT_LONG_PRESS
					&& encoder_push_state(encoder_b)) {
				//Enter Setting mode.
				deepdeck_status = S_SETTINGS;
				ESP_LOGI("Encoder 1", "setting mode");
			} else {
				encoder_command(encoder1_status,
						key_layouts[current_layout].left_encoder_map);
			}
			past_encoder1_state = encoder1_status;
		}

		encoder2_status = encoder_state(encoder_b);

		if (encoder2_status != past_encoder2_state) {
			DEEP_SLEEP = false;

			// Check if both encoder are pushed, to enter settings mode.
			if (encoder2_status == ENC_BUT_LONG_PRESS
					&& encoder_push_state(encoder_a)) {
				//Enter Setting mode.
				deepdeck_status = S_SETTINGS;
				ESP_LOGI("Encoder 2", "setting mode");
			} else {
				encoder_command(encoder2_status,
						key_layouts[current_layout].right_encoder_map);
			}

			past_encoder2_state = encoder2_status;
		}
		taskYIELD();
	}
}
