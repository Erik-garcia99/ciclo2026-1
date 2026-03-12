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

#include<driver/gpio.h>

#include"modules/WIFI/wifi_lib.h"
#include"modules/HTTPS/https_lib.h"

#define BUTTON 18
#define BUTTON_PRESSED  0 



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


    gpio_reset_pin(BUTTON);
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    // con pull-down, siempre estarea leyedno 0, cunado presione el btn entonces leera 1
    gpio_pullup_dis(BUTTON);
    gpio_pulldown_en(BUTTON);
    //en este punto el GPIO 18 esta configurado como entrada, habilidanto pull-down, cunado se presione, leera 1, entonces respondera que se ha presionado


    ESP_LOGI(TAG, "presiona el boton para enviar el mensaje...", BUTTON);
    
    while (gpio_get_level(BUTTON) != BUTTON_PRESSED) {
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }


    vTaskDelay(pdMS_TO_TICKS(50));
    if (gpio_get_level(BUTTON) == BUTTON_PRESSED) {
        ESP_LOGI(TAG, "enviando mensaje...");
        xTaskCreate(http_test_task, "http_test_task", 4098, NULL, 9, NULL);
    }
}
