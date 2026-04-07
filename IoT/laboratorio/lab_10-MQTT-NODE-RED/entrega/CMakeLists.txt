idf_component_register(SRCS "main.c" "modules/WIFI/wifi_lib.c" "modules/MQTT/mqtt_lib.c" "modules/ADC/adc_lib.c" "modules/DHT11/esp32-dht11.c"
                    INCLUDE_DIRS "." "modules/WIFI" "modules/mqtt" "modules/ADC" "modules/DHT11")
