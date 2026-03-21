#include<stdio.h>

//librerias estandares
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>

#include<esp_log.h>

#include "nvs_flash.h"
#include "esp_event.h"
//mqtt
#include<mqtt_client.h>

//libreias propoas 
#include "mqtt_lib.h"

static const char *TAG = "MQTT";


extern esp_mqtt_client_handle_t client;

//manerjador de eventos para MQTT 
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id){

        case MQTT_EVENT_CONNECTED:{
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            // msg_id = esp_mqtt_client_publish(client, TOPIC, "prueba_publicador", 0, 1, 0);
            // ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            

            //por medio de un evento indicamos que se conecto 


        }break;

        case MQTT_EVENT_SUBSCRIBED: {
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED msg_id=%d", event->msg_id);
        }

        case MQTT_EVENT_DISCONNECTED:{
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        
        }break;
        
        case MQTT_EVENT_PUBLISHED:{
        
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        }break;
        
        case MQTT_EVENT_DATA:{
            //en este pundo recibi el dato desde el servidor esto para poder actualizar el esado del led 
            char topic[128] = {0};
            char data[256]  = {0};

            snprintf(topic, sizeof(topic), "%.*s", event->topic_len, event->topic);
            snprintf(data,  sizeof(data),  "%.*s", event->data_len,  event->data);

            ESP_LOGI(TAG, "Topic : %s", topic);
            ESP_LOGI(TAG, "Data  : %s", data);

        }break;

        case MQTT_EVENT_ERROR:{
            ESP_LOGE(TAG, "Error MQTT");
        }break;




        default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;

    }

}

void mqtt_start(void){

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri =BROKER,
    };
    
    //inicamos el cliente ESP apra poder subscribirse o publicar. 
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}




