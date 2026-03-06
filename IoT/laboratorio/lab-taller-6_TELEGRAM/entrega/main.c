#include <stdio.h>
#include<string.h>

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>

#include<esp_log.h>
#include<esp_event.h>

#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

#include<driver/adc.h>

#include<modules/WIFI/wifi_lib.h>
#include<modules/HTTPS/https_lib.h>
#include<modules/ADC/adc_lib.h>

QueueHandle_t queue_ADC;
static const char *TAG = "MAIN";

void app_main(void)
{
    queue_ADC = xQueueCreate(10, sizeof(uint32_t));

    set_adc(ADC_CHANNEL);

    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    xTaskCreate(http_test_task, "http_test_task", 8192, NULL, 9, NULL);

    //esperamos a que el ADC se calibre 
    vTaskDelay(pdMS_TO_TICKS(1000));

    int read = 0;
    int threshold = 0;
    while(1){
        read = read_adc(ADC_CHANNEL);

        // threshold 0 -> debajo del 25%  (< 1000)
        // threshold 1 -> arriba del 50%  (1000 - 3199)
        // threshold 2 -> al 100%         (>= 3200)
        if(read >= 1000){
            threshold = 1;
        }

        ESP_LOGI(TAG, "ADC raw: %d -> threshold: %d", read, threshold);
        xQueueSend(queue_ADC, &threshold, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(180000));
    }
}