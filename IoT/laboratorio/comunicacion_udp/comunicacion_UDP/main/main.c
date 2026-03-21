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
// #include"modules/WIFI/wifi_lib.h" //vamos a poner un WIFI normal o un wifi
#include"modules/UART/uart_lib.h"
#include"global.h"

//varibales globales 
static const char *TAG = "MAIN";


//varibales globales para freertos
//colas
QueueHandle_t flow_data_queue;
QueueHandle_t uart_event;

//estrucutrua para pasar parametros a la tarea 
task_uart_params_t global_uart;

void app_main(void)
{
    //manejaremos punteros
    //una cola de 10 items en donde cada uno tendra el tamanio de BUFFER con la capacidad de guardar a char. 
    flow_data_queue = xQueueCreate(10, sizeof(char*)); 

    uart_init(MAIN_UART, 9600, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //inicando WIFI 
    // esp_err_t ret = nvs_flash_init();
    // if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     ret = nvs_flash_init();
    // }
    // ESP_ERROR_CHECK(ret);

    // ESP_LOGI(TAG, "ESP_MODE_STA");
    // wifi_init_sta();
    
    //seleccionamos uart 0
    global_uart.NUM_UART= MAIN_UART;
    //ESTE SE COMPINE EL UART PARA COMUNICARSE CON PA PC

    //creacion de tarea 
    xTaskCreate(task_uart, "task_uart", 8192, &global_uart,9,NULL); 

}
