#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>

/** @brief Queue for sending keyboard reports
 * @see keyboard_command_t */
extern QueueHandle_t battery_q;

/** @brief Queue for sending mouse reports
 * @see mouse_command_t */
extern QueueHandle_t mouse_q;

/** @brief Queue for sending keyboard reports
 * @see keyboard_command_t */
extern QueueHandle_t keyboard_q;

/** @brief Queue for sending media reports
 * @see media_command_t */
extern QueueHandle_t media_q;


esp_err_t init_queues(void);