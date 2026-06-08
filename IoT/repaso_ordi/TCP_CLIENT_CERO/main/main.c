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
#include"modules/TCP/tcp_lib.h"
#include"global.h"



//++++++++++++++++++colas 

QueueHandle_t uart_queue;
QueueHandle_t flow_data_queue;
QueueHandle_t tcp_data_flow;

//+++++++++++++++++grupos de eventos 
EventGroupHandle_t g_EVENT_WIFI;
EventGroupHandle_t g_tcp_event_group;
//++++++++++++++++++ estrucutras 
format_request_t format_request;
send_info_t send_info;
esp_wifi_t esp_wifi;
tcp_client_t tcp_client;

//++++++++++++++++++enums
op_type_t op_type;
action_t action;
resourse_t resourse;
instructions_t instructions;



// ++++++++++++++++++=varibales globales



//+++++++++++++++++++++ funciones 

char **pasrse_input(char *line);

esp_err_t update_cred(char *token_1, char *token_2, int op);



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
    int len = snprintf(msg, sizeof(msg), "\r\ningrese las credenciales WIFI -> SSID:<WIFI_SSIW> PSWD:<WIFI_PSWD>: \r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    len = snprintf(msg, sizeof(msg), "credenucaes para TCP -> HST_IP:<IP> HST_PORT:<PORT>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    len = snprintf(msg, sizeof(msg), "credenucaes para UDP -> HST_IP:<IP> HST_PORT:<PORT>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    len = snprintf(msg, sizeof(msg), "ingrear usuaria -> USER:<user>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));




}



void task_cmd_uart(void *params){


    char *receive;
    char msg[120];
    int len;
    esp_err_t ret;

    while(1){

        if(xQueueReceive(flow_data_queue, &receive, portMAX_DELAY)){

            //recibo los datos pero no se que es lo que recibi sis fue WIFI, UDP, TCP, usuarios??

            char **tokens = pasrse_input(receive);

            //se reciben os datos ya arseados, estos tiene maximo 2 incides el SSID:<ssid del wufi>
            // .. etc

            if(tokens== NULL || tokens[0] == NULL){
                len = snprintf(msg, sizeof(msg), "\r\nERROR de parseo de comandos\r\n");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));

                free(tokens);
                free(receive);
                continue;
            }


            //en otro caso debemos de separar y los comanods deben de ir ordenados

            char *cmd_case = strdup(tokens[0]);

            char *type = strtok(cmd_case, ":");


            if(strcmp(type, "SSID") == 0 &&  tokens[1] != NULL){
                //si hay ssid debe de haber un paswd

                char *ssid = strchr(tokens[0], ':');
                char *pswd = strchr(tokens[1], ':');

                if(ssid == NULL || pswd == NULL){
                    len = snprintf(msg, sizeof(msg), "se necesita el SSID y el WIFI de la red");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(receive);
                    free(tokens);
                    continue;
                }
                ssid++;
                pswd++;


                ret = update_cred(ssid, pswd, SSID);

                if(ret != ESP_OK){
                    len = snprintf(msg, sizeof(msg), "no se puedieron actualizar las credenclaes ");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(tokens);
                    continue;
                }

                //en otro caos si se asignanos y entonces procedemos a actcar el bit para actulaizar credeniclaes 

                xEventGroupSetBits(g_EVENT_WIFI, WIFI_UPDATE);
                //en este caso habria otro eventos que indica que se actualizo el wifi por lo que se deberia de reicniar 
                //la conexion entre el servidor y el esp. 

            }


        }

    }
}



esp_err_t update_cred(char *token_1, char *token_2, int op){

    char msg[120];
    int len;
    switch(op){

        case SSID :{
            int len_ssid = strlen(token_1);
            int len_pswd = strlen(token_2);

            esp_wifi.esp_ssid = realloc(esp_wifi.esp_ssid,len_ssid+1);
            esp_wifi.esp_pswd= realloc(esp_wifi.esp_pswd, len_pswd+1);

            if(esp_wifi.esp_ssid != NULL && esp_wifi.esp_pswd != NULL){

                strcpy(esp_wifi.esp_ssid, token_1);
                strcpy(esp_wifi.esp_pswd, token_2);
                esp_wifi.connected = 0;
                
                return ESP_OK;
            }


        }break;
        case HOST_TCP_IP:{

        }break;

        // case HOST_UDP_IP:{


        // }break;

        // case USER:{

        // }break;


        default :{
            char msg[80];
            int len = snprintf(msg, sizeof(msg), "la opcion ingresada no es un comando");
            uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
        }break;

    }

    return ESP_FAIL;

}




char **pasrse_input(char *line){

    char **tokens = malloc(5 * sizeof(char*));
    char *token; 
    int position=0;

    token = strtok(line, " ");
    while(token != NULL){
        if(position >= MAX_ARGS ) break;
        tokens[position++] = strdup(token);
        token = strtok(NULL, " ");
    }
    tokens[position] = NULL;
    return tokens;
}