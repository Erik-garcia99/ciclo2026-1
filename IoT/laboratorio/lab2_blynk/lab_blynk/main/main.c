#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static const char *TAG = "BLYNK_IDF";

#define WIFI_SSID      "INFINITUMF4AF"
#define WIFI_PASS      "nFukH34MPW"
#define BLYNK_TOKEN    "3oYbZm15m2CpGYwxtCs2a8MCKIqltrK5"


#define LED_GPIO 2


esp_mqtt_client_handle_t client = NULL;
bool mqtt_conectado = false; 

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi desconectado. Reintentando...");
        esp_wifi_connect();
        mqtt_conectado = false; // Bloqueamos envíos
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi Listo! IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "conectando blynk");
        mqtt_conectado = true; 
        

        esp_mqtt_client_subscribe(event->client, "down/V0", 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "desconectado blynk");
        mqtt_conectado = false;
        break;

    case MQTT_EVENT_DATA:
        printf("Mensaje recibido: %.*s\n", event->data_len, event->data);
        

        if (strncmp(event->data, "1", event->data_len) == 0) {
            gpio_set_level(LED_GPIO, 1);
            ESP_LOGI(TAG, "LED ENCENDIDO (Desde App)");
            if(mqtt_conectado) esp_mqtt_client_publish(event->client, "ds/V1", "1", 0, 0, 0);
        } else {
            gpio_set_level(LED_GPIO, 0);
            ESP_LOGI(TAG, "LED APAGADO (Desde App)");
            if(mqtt_conectado) esp_mqtt_client_publish(event->client, "ds/V1", "0", 0, 0, 0);
        }
        break;

    default:
        break;
    }
}

void uptime_task(void *pvParameter)
{
    char uptime_str[16];
    int msg_id; 

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        if (client != NULL && mqtt_conectado) {
            int64_t uptime_sec = esp_timer_get_time() / 1000000;
            sprintf(uptime_str, "%lld", uptime_sec);


            msg_id = esp_mqtt_client_publish(client, "ds/V2", uptime_str, strlen(uptime_str), 1, 0);
            
            if (msg_id != -1) {
                printf("Enviando Uptime a V2: %s s (Msg ID: %d) -> ENVIADO\n", uptime_str, msg_id);
            } else {
                printf("ERROR: No se pudo enviar el mensaje MQTT\n");
            }
        }
    }
}

void app_main(void)
{
  
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    wifi_init_sta();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://blynk.cloud:1883", 
        .credentials.client_id = BLYNK_TOKEN,   
        .credentials.username = "device",       
        .credentials.authentication.password = BLYNK_TOKEN
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

 
    xTaskCreate(uptime_task, "uptime_task", 4096, NULL, 5, NULL);
}