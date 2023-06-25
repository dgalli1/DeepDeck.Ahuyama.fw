idf_component_register(SRCS  "spiffs.c" "main.c" "queue.c" "deepdeck_tasks.c" "keymap.c" "matrix.c" "keypress_handles.c" "gesture_handles.c" "wifi_handles.c" "mqtt.c"
                    INCLUDE_DIRS "."
                    REQUIRES "spiffs" "mdns" "mqtt" "oled" "nvsfuncs" "plugins" "rotary_encoder" "rgb_led" "menu" "bus" "apds9960" "uuid_master")


                    spiffs_create_partition_image(www_0 ../spiffs_image FLASH_IN_PROJECT)
