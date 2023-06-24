// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "driver/rmt.h"
#include "esp_log.h"
#include "led_strip.h"
#include "rgb_led.h"

static const char *TAG = "RGB_LEDs";

led_strip_t *rgb_key;
led_strip_t *rgb_notif;

/// @brief Input queue for sending mouse reports
QueueHandle_t keyled_q;

void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i)
    {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void rgb_notification_led_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(NOTIFICATION_RGB_GPIO, RMT_TX_CHANNEL_NOTIFICATION);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(RGB_LED_NOTIFICATION_NUMBER, (led_strip_dev_t)config.channel);
    rgb_notif = led_strip_new_rmt_ws2812(&strip_config);
    if (!rgb_notif)
    {
        ESP_LOGE(TAG, "Install notification LEDs failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(rgb_notif->clear(rgb_notif, 100));
}

void rgb_key_led_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(KEYBOARD_RGB_GPIO, RMT_TX_CHANNEL_KEYPAD);

    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(RGB_LED_KEYBOARD_NUMBER, (led_strip_dev_t)config.channel);
    rgb_key = led_strip_new_rmt_ws2812(&strip_config);
    if (!rgb_key)
    {
        ESP_LOGE(TAG, "Install key LEDs failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(rgb_key->clear(rgb_key, 100));

    // Init rgb_keystatus
    for (uint8_t i = 0; i < RGB_LED_KEYBOARD_NUMBER; i++)
    {
        rgb_key_status[i].h = 180;
        rgb_key_status[i].s = 100;
        rgb_key_status[i].v = 0;
    }

    /* Queue to send the key led modes */
    keyled_q = xQueueCreate(
        /* The number of items the queue can hold. */
        16,
        /* Size of each item is big enough to hold the
        whole structure. */
        sizeof(color_key));
}


void key_led_modes(void)
{

    uint32_t red2 = 0;
    color_key new_color;

    while (true)
    {
        /* Receive a message from the created queue to hold pointers.  Block for 10
    ticks if a message is not immediately available.  The value is read into a
    pointer variable, and as the value received is the address of the xMessage
    variable, after this call pxRxedPointer will point to xMessage. */
        if (xQueueReceive(keyled_q,
                          &(new_color),
                          100))
        {
            ESP_LOGI(TAG, "Received message from keycolor Q");
            rbg_key *rgb = &rgb_key_status[new_color.key_number];
            rgb->h = new_color.h;
            rgb->s = new_color.s;
            rgb->v = new_color.v;
            uint32_t red = 0;
            uint32_t green = 0;
            uint32_t blue = 0;
            hsv2rgb(rgb->h, rgb->s, rgb->v, &red, &green, &blue);
            ESP_ERROR_CHECK(rgb_key->set_pixel(rgb_key, new_color.key_number, red, green, blue));
            ESP_ERROR_CHECK(rgb_key->refresh(rgb_key, 100));
        }
            //hsv2rgb(0, 100, 5, &red, &green, &blue);
            //ESP_ERROR_CHECK(rgb_key->set_pixel(rgb_key, 3, red, green, blue));
            //ESP_ERROR_CHECK(rgb_key->refresh(rgb_key, 100));
            //vTaskDelay(pdMS_TO_TICKS(RGB_LED_REFRESH_SPEED));
    }
}