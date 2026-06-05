#ifndef MQTT_LIB_H
#define MQTT_LIB_H

#include<mqtt_client.h>


#define BROKER "mqtt://192.168.1.66:1883"
#define TOPIC ""

//definamos los topics de inicio 

extern esp_mqtt_client_handle_t client;
extern QueueHandle_t flow_data;


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_client_start();


#endif