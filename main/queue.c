#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>




// HID mouse input report length
#define HID_MOUSE_IN_RPT_LEN        5

// HID consumer control input report length
#define HID_CC_IN_RPT_LEN           2

// HID keyboard input report length$
#define HID_KB_IN_RPT_LEN           55

/// @brief Input queue for sending keyboard reports
QueueHandle_t keyboard_q;
/// @brief Input queue for sending mouse reports
QueueHandle_t mouse_q;
/// @brief Input queue for sending media reports
QueueHandle_t media_q;

//init queues inside of function
esp_err_t init_queues(void)
{
    keyboard_q = xQueueCreate(32, HID_KB_IN_RPT_LEN * sizeof(uint8_t));
    mouse_q = xQueueCreate(32, HID_MOUSE_IN_RPT_LEN * sizeof(uint8_t));
    media_q = xQueueCreate(32, HID_CC_IN_RPT_LEN * sizeof(uint8_t));
    return ESP_OK;
}