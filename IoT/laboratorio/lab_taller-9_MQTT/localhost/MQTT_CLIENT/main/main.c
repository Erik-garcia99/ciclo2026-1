#include <stdio.h>

//aunque se llama cliente hago referencia al suscriptor, este va a estar leyendo lo que pasa 
//este solo va a recibir lo que se publica, en si nomas tiene que escuchar 

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

    mqtt_start();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
