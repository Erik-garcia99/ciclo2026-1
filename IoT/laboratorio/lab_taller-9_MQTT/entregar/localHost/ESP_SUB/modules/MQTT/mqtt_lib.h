#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#define BROKER "mqtt://192.168.1.66:1883"
#define TOPIC "device/led"

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_start(void);


#endif