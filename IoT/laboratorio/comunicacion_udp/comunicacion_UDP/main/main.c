#include <stdio.h>

//freerots
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>


//drivers
#include<driver/gpio.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>


//logs
#include<esp_log.h>

//librerias propias
#include"modules/WIFI/wifi_lib.h" //vamos a poner un WIFI normal o un wifi
#include"modules/UART/uart_lib.h"
#include"global.h"

//varibales globales 
static const char *TAG = "MAIN";

QueueHandle_t flow_data_queue;

void app_main(void)
{
    //manejaremos punteros
    //una cola de 10 items en donde cada uno tendra el tamanio de BUFFER con la capacidad de guardar a char. 
    flow_data_queue = xQueueCreate(10, sizeof(char)*BUFFER); 

    //inicando WIFI 
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();


}
