#include <stdio.h>

//freertos

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>


//logs - errors
#include<esp_log.h>


//drivers 
#include<driver/adc.h>


#include"modules/ADC/adc_lib.h"


//varibales - constantes globales 
static const char *TAG = "MAIN";


//tareas


void task_main(void *params);


void app_main(void)
{
    set_adc(ADC_CHANNEL);

    
    xTaskCreate(task_main, "task_main", 4096, NULL, 9, NULL);

    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}


void task_main(void *params){


    while(1){


        int read = read_adc(ADC_CHANNEL);

        ESP_LOGI(TAG, "lectura ADC :%d ", read);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}
