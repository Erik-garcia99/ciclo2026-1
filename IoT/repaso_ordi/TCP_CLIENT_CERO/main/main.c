#include <stdio.h>

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
#include<freertos/event_groups.h>




//librerias propias 
#include"modules/WIFI/wifi_lib.h"
#include"modules/UART/uart_lib.h"
#include"global.h"



//++++++++++++++++++colas 

QueueHandle_t uart_queue;
QueueHandle_t flow_data_queue;

//+++++++++++++++++grupos de eventos 


//++++++++++++++++++ estrucutras 


//++++++++++++++++++uniones 



// ++++++++++++++++++=varibales globales



//+++++++++++++++++++++ funciones 

esp_err_t update_cred();


//++++++++++++++++++++++ tareas 




void app_main(void)
{
    flow_data_queue = xQueueCreate(10, sizeof(char *));

    //iniciamoa la tarea 

    xTaskCreate(uart_task, "uart_task", 4098, NULL, 9, NULL);

}
