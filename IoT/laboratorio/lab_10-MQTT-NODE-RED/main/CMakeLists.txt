idf_component_register(SRCS "main.c" "modules/WIFI/wifi_lib.c" "modules/MQTT/mqtt_lib.c" "modules/ADC/adc_lib.c"
                    INCLUDE_DIRS "." "modules/WIFI" "modules/mqtt" "modules/ADC")
