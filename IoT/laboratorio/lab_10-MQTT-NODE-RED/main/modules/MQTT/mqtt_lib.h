#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#include<mqtt_client.h>

#define BROKER "mqtt://192.168.1.66:1883"
#define TOPIC_LED "device/led"
#define TOPIC_ADC "device/adc"
#define TOPIC_HUM "device/humi"
#define TOPIC_TEMP "device/temp"
//este es para activar el led el cual lo hace desde el broker
#define TOPIC_ACT "device/action"


extern esp_mqtt_client_handle_t client;




void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_start(void);


#endif