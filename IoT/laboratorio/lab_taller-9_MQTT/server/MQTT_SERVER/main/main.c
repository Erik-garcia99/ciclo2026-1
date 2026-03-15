#include <stdio.h>

//enviaremos la publicacion al broker del servidor 
//freeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
//drivers 
#include<driver/gpio.h>

#include<esp_timer.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

#include<esp_log.h>
// #include<esp_err.h>

//librerias propias

#include"modules/WIFI/wifi_lib.h"
#include"modules/MQTT/mqtt_lib.h"

#define BUFFER 1024

static const char *TAG = "MAIN";

esp_mqtt_client_handle_t client= NULL;

int LAST_PRESS;
static int led_state = 0;        
static int last_press_handled = 0; 

QueueHandle_t data_queue;


//funcion inical para gpio

void GPIO_INIT(void);


void sent_task(void *params);
//tarea que estar monitoreando si se presiono el btn 
void task_input(void *params);



void mqtt_publish(const char *data);


void app_main(void)
{
    GPIO_INIT();

    data_queue = xQueueCreate(10, sizeof(char)*40);

    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    mqtt_start();

    xTaskCreate(sent_task, "send_task", 2048, NULL, 8, NULL);
    xTaskCreate(task_input, "task_input", 2048, NULL, 9,NULL);

}


void GPIO_INIT(void){

    gpio_reset_pin(18);
    gpio_set_direction(18, GPIO_MODE_OUTPUT);
    gpio_set_level(18, 0);
    //lo prendemos con un boton 

    gpio_reset_pin(19);
    gpio_set_direction(19, GPIO_MODE_DEF_INPUT);

    gpio_pullup_dis(19);
    gpio_pulldown_en(19);

}

void task_input(void *params){

    while(1){
        int current_level = gpio_get_level(19);

        if(current_level == 1 && last_press_handled == 0){

            vTaskDelay(pdMS_TO_TICKS(50)); 

            
            if(gpio_get_level(19) == 1){

                // toggle del estado
                led_state = !led_state;
                gpio_set_level(18, led_state);

                // publica el estado
                //un arreglo en donde pondremos que es lo que queremos publicar. 
                if(led_state){
                    char dataON[]="LED ON : 01275863";
                    ESP_LOGI(TAG, "LED ON");
                    xQueueSend(data_queue,dataON, portMAX_DELAY);
                } else {
                    char dataOFF[]="LED OFF : 01275863";
                    ESP_LOGI(TAG, "LED OFF");
                    xQueueSend(data_queue,dataOFF, portMAX_DELAY);
                }

                last_press_handled = 1;
            }
        }

        // cuando sueltas el botón reseteamos la bandera
        if(current_level == 0){
            last_press_handled = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



void sent_task(void *params){

    char buffer[40];

    while(1){
        if(xQueueReceive(data_queue, (void*)buffer, portMAX_DELAY)){
            ESP_LOGI(TAG, "recibio cola: %s", buffer);
            esp_mqtt_client_publish(client, TOPIC, buffer, 0, 1, 0);
        }   
    }

}