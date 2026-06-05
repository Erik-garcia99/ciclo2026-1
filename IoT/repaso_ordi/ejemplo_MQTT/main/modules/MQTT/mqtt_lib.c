

#include<stdlib.h>
#include<string.h>
#include<stdio.h>

#include<esp_log.h>

#include "nvs_flash.h"
#include "esp_event.h"

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
#include<freertos/event_groups.h>


#include<mqtt_client.h>


#include<mqtt_lib.h>

const char *TAG = "MQTT_LIB"


void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){

    esp_mqtt_event_handle_t event = event_data;

    client = event->client;
    

    // int msg_id;

    switch((esp_mqtt_event_id_t)event_id){


        case MQTT_EVENT_CONNECTED:{

            //al conectarse al broker lo que hace seria suscribirse a todos los topicos inicales de un sistema 
        }

        case MQTT_EVENT_DISCONNECTED:{

            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED del broker");
        }

        case MQTT_EVENT_PUBLISHED:{
            ESP_LOGI(TAG,"EVENT_PUBLISHED event_id:%d", event->msg_id);
        }

        case MQTT_EVENT_DATA:{

            //se recibireroion datos de un topico en el cual se esta suscrito 
        }



    }


}

void mqtt_client_start(){

    esp_mqtt_client_config_t mqtt_cfg={
        .broker = BROKER
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}