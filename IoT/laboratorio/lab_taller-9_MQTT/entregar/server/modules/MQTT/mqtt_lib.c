
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

static const char *TAG = "MQTT_LIB";


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

        }break;

        case MQTT_EVENT_DISCONNECTED:{
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        
        }break;
        
        case MQTT_EVENT_PUBLISHED:{
        
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
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
        .credentials.username ="mqtt",
        .credentials.authentication.password="mqtt",
    };
    
    //inicamos el cliente ESP apra poder subscribirse o publicar. 
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}

