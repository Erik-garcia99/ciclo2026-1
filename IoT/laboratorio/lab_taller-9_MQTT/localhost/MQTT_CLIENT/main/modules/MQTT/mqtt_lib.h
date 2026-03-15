#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#define BROKER "mqtt://148.231.130.229:50003"
#define TOPIC "device/led"

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_start(void);


#endif