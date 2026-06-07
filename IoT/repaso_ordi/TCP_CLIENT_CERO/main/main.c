#include <stdio.h>

#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
#include<freertos/event_groups.h>


#include<driver/uart.h>


//WIFI
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>


//librerias propias 
#include"modules/WIFI/wifi_lib.h"
#include"modules/UART/uart_lib.h"
#include"global.h"



//++++++++++++++++++colas 

QueueHandle_t uart_queue;
QueueHandle_t flow_data_queue;

//+++++++++++++++++grupos de eventos 
EventGroupHandle_t g_EVENT_WIFI;

//++++++++++++++++++ estrucutras 
format_request_t format_request;
send_info_t send_info;
esp_wifi_t esp_wifi;

//++++++++++++++++++enums
op_type_t op_type;
action_t action;
resourse_t resourse;
instructions_t instructions;



// ++++++++++++++++++=varibales globales



//+++++++++++++++++++++ funciones 

esp_err_t update_cred();
char **parase_cmd(char *line);

esp_err_t update_cred(int op, char *token_1, char *token_2);



//++++++++++++++++++++++ tareas 

void task_cmd_uart(void *params);



void app_main(void)
{
    flow_data_queue = xQueueCreate(10, sizeof(char *));

    //inicamos grupo de eventos 

    g_EVENT_WIFI = xEventGroupCreate();

    //inicamos UART 
    uart_init();

    //iniciamoa la tarea 

    xTaskCreate(uart_task, "uart_task", 4098, NULL, 9, NULL);
    xTaskCreate(task_cmd_uart,"task_cmd_uart", 4098, NULL, 8, NULL);


    //**conexion a WIFI*/

    //antes de relizar una conexion lo que se debe hacer primero es ingresar las credencuales 
    /**
     * que necesiot,
     * -- necesito la tarea que se encarga de recibir los datos desde la tarea de UART para procesar que es lo que el usuario quiere 
     * -- la funcion que parsea el contenido < la funcion que hice en SO para separar lo tokens > 
     * -- la funcion que actualiza los datos y activa el bit que idniq eu se actualizaron los bits 
    */
    char msg[100];
    int len = snprintf(msg, sizeof(msg), "\rningrese las credenciales WIFI -> SSID:<WIFI_SSIW> PSWD:<WIFI_PSWD>: \r\n");
    uart_write_bytes(UART_MAIN,msg, len);



}



void task_cmd_uart(void *params){


    char *receive;

    while(1){

        if(xQueueReceive(flow_data_queue, &receive, portMAX_DELAY)){

            //recibo los datos pero no se que es lo que recibi sis fue WIFI, UDP, TCP, usuarios??


        }

    }
}



esp_err_t update_cred(int op, char *token_1, char *token_2){


}


char **parase_cmd(char *line){




}