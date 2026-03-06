#include <stdio.h>

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>

#include<esp_log.h>
#include<esp_event.h>

#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>





//liobereias propoas
#include"modules/WIFI/wifi_lib.h"
#include"modules/HTTPS/https_lib.h"


static const char *TAG = "MAIN";

void app_main(void)
{

    
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    xTaskCreate(http_test_task, "http_test_task", 4098, NULL, 9, NULL);

}
