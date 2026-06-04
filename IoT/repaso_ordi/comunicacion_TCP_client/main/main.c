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



#include"modules/WIFI/wifi_lib.h"
#include"modules/TCP/tcp_lib.h"

int tcp_sck=-1;

static const char *TAG = "MAIN-TCP_CLIENT";


void app_main(void)
{

    //------------------------------ inicamos wifi
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();
    //-----------------------------------

    //una vez que se conecto al WIFI entonces lo que hacemos es tratar que establecer conexion con el servidor 

    ret = tcp_client_init();

    if(ret != ESP_OK){
        //si no se puede conectar reinicamos el sistema 
        esp_restart();
    }

    while(1); //si se puede inciar solo se mantiene esperando 


}
