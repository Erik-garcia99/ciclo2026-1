#include <stdio.h>
#include<stdlib.h>
#include<string.h>


//FreeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/event_groups.h>

//ESP LOGS
#include<esp_log.h>
#include<esp_event.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>



#include"WIFI/wifi_lib.h"
#include"TCP/tcp_lib.h"

static const char *TAG = "TCP";

char *parse_time(char *JSON);

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG,"ESP_MODE_STA");
    wifi_init_sta();

    while (1)
    {
        char *request=get_time_tcp();
        ESP_LOGI(TAG, "time--> %s", request);

        char *current_time = parse_time(request);

        ESP_LOGI(TAG,"\nCURRENT TIME---> %s", current_time);

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
    
}



char *parse_time(char *JSON){

    char *aux = JSON;

    char *inicio = strstr(JSON, "\"currentDateTime\":\"");
    if(inicio == NULL){
        ESP_LOGE(TAG, "No se encontro currentDateTime");
        return NULL;
    }


    inicio += strlen("\"currentDateTime\":\"");



    char *time = malloc(6);
    strncpy(time, inicio + 11, 5);
    time[5] = '\0';


    return time;
}
