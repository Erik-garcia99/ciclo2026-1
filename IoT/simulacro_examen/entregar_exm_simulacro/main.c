/**
 * 
 * @author erik garcia chavez 
 * @date 2026-06-10
 * ingenieira en computacion
 * UABC 
 * internet de las cosas 
 * 
 * 
*/

/*
 * SIMULULACRO DE EXAMEN
 *
 * establecer el recurso de reset 
 * 
 * 
 *
*/

/**
 * 
 * matricula : 61 31 32 37 35 38 36 33(hex) -> a1275863(ascii)
 * 
 */


//+++++++++++++++++++++++++++++++librerias
#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

//drivers
#include<driver/uart.h>
#include<esp_timer.h>

//logs
#include<esp_log.h>
#include <esp_err.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

//librerias  propias
#include<modules/UART/uart_lib.h>
#include<modules/WIFI/wifi_lib.h>
#include<modules/TCP/tcp_lib.h>
#include"modules/ADC/adc_lib.h"
#include"modules/PWM/pwm_lib.h"
#include<global.h>


//+++++++++++++++++++++++++++++++++++++++++vairbales globales

static const char *TAG = "MAIN";
uint8_t led_state;
//identificacion por la matricula 
uint32_t user;
volatile uint8_t login_pending = 0;

//sera de 64 bits porque usare timer para obtener el tiempo 
static uint64_t reset_state; //este es el que llevara la cuenta desde que se inicio a contar, es la varibale global se deifinira si ya se cumplio o no para el resert 
static uint8_t time_resert; //variable en donde se guarda en tiempo en el cual se planea realizar resert 
//inicia el conteo, comparacion de desde cunato se empezo a contar, apra la referencia 
static uint64_t init_count;
//conviertie lo qeu se recibcio en timepo para comparar 
static uint64_t limite_us;


//handle con el que podremos eliminar la tarea en caso de ser necesario 
TaskHandle_t handle_resert_set;
// TaskHandle_t set_count;


//++++++++++++++++ colas 
//cola para los eventos de UART
QueueHandle_t uart_event;
//cola que lleva la informacoin ingresada por recv
QueueHandle_t tcp_rx_queue;
//cola que manerjara el flujo de datos de UART 
QueueHandle_t flow_data_queue;


//++++++++++++++++ grupos de eventos. 

EventGroupHandle_t g_tcp_event_group;
EventGroupHandle_t s_wifi_event_group;

EventGroupHandle_t g_login_event_group;

EventGroupHandle_t g_rst_event;


//++++++++++++++++++  estrucutra
//contiene valores de UART 
task_uart_port_t global_uart;
//estucuturua para red 
esp_wifi_t esp_wifi;
//estrucutra para parametros de la conexion tcp
tcp_client_t tcp_client ={0};
//enum de operacion CP 
op_type_t op_type;
//enum de trama bianrio
action_t action;
resourse_t resourse;
//estrucutra de la uncion que agrupa los datos
send_info_t send_info ={0};

format_request_t format_request ={0};
//++++++++++++++++++++++++++++funciones
/**
 * @brief funcion que se encargara de serparar los tokens ingresador por le usuario
 * este solo es serparar entre los comandos, mas no extrae los token necesarios 
 * 
 * @param line recibe un apuntador a los datos que ingresaron por UART 
 *
 * @return regresa la lista de tokens  
 * 
*/ 
char **pasrse_input(char *line);
//esto es lo mismo pero utilizando otro separados, porque si uso el mismo va a separar todo lo de comandos 
char **pasrse_input_recv(char *line);



/**
 * @brief funcion encargada de actualizar las distittas credencuales mediante comandos 
 * 
 * 
 * @param key donde va el nombre del nuevo SSID o la nueva IP.
 * @param anchor deberia de ir la contrasenia de la nueva red o el nombre del usuario en caso de ser una red de empresa
 * @param pswd_ent si este tiene datos es que se actualizara una red de empresa o unversidad, va la contrasenia del usuario
 * 
 * @param identificator le dira a la funcion que tiepo de actualizacion sera, puede ser <SSID, HOST_IP, SSID_ENT, MAT>
 * 
 * @return ESP_OK si se pudo actualziar 
 * @return ESP_FAIL si no fue posible 
 * 
 * 
 */

/**
  * la funcion tratara de ser lo mas general, los parametros que no seran requeridos se les asginara un 0 o NULL 
  * 
  */
esp_err_t update_setup_cred(char *key, char *anchor ,char *pswd_ent,  char *identificator);

void setup_tcp(void);

static int parse_number(const char *str);

//funcion en donde se estara contando 
// void set_reset_time(void);

void get_time_current(void);

//++++++++++++++++++++++++++++++tareas 
//tarea encargada de recibir el comando por UART 
void task_cmd_uart(void *params);

//la tarea para realizar el resertn

void task_resert_esp(void *params);

//mantendremos el set como unatarea que necesitamos qeu este coriendo en paralelo 
void set_resert_time_task(void *params);



void app_main(void)
{
    //creamos el grupo de eventos para WIFI 
    s_wifi_event_group = xEventGroupCreate();
    g_tcp_event_group= xEventGroupCreate();
    g_login_event_group = xEventGroupCreate();
    g_rst_event = xEventGroupCreate();


    flow_data_queue = xQueueCreate(10, sizeof(char*));
    tcp_rx_queue = xQueueCreate(10, sizeof(format_request_t *));

    
    //inicmaos los GPIO
    gpio_init();
    set_adc(ADC_CHANNEL);
    pwm_init(); 
    //iniciamos con 0
    led_state=0;


    global_uart.NUM_PORT = UART_MAIN;
    //inicamos UART 
    uart_init(UART_MAIN,115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    
    //aqui esta el demon que esta ecuchando a UART 
    xTaskCreate(task_uart, "task_uart", 4096,&global_uart, 9, NULL);

    xTaskCreate(task_cmd_uart, "task_cmd_uart", 4096, NULL, 8, NULL);

    // xTaskCreate(task_resert_esp, "task_resert_esp", 2048, NULL, 5, NULL);
    
    //definimos por defecto pero es varibale por lo que puede ser modificable   
    //dejemos las varibales externs y todo como este. 
    // user = 0x001377d7; //a1275863 -> 4 bytes
    user = 0x001377d7;
    //configuracion de WIFI
    esp_err_t ret;

    //vamos a inicar con parametros por defecto 
    char *ssid_default ="INFINITUMF4AF\0";
    char *pswd_default = "nFukH34MPW\0";  
    ret = update_setup_cred(ssid_default, pswd_default, NULL, "SSID");

    
    if(ret !=ESP_FAIL){
        uart_write_bytes(UART_MAIN,"\r\n", 2);
        const char *mssg = "credenciales por defecto inicalizadas\0";
        uart_write_bytes(UART_MAIN,UART_GREEN, strlen(UART_GREEN));
        uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
        uart_write_bytes(UART_MAIN,UART_RESET, strlen(UART_RESET));
        
    }
    else{
        uart_write_bytes(UART_MAIN,"\r\n", 2);
        const char *mssg = "no se pudieron actualizar las credenciales\n\0";
        uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
    } 


    //indicamos como seran los comandos 
    ESP_LOGI(TAG, "COMANDOS PARA ESTABLECER CARACTERISITRAS");
    ESP_LOGI(TAG, "SETUP WIFI -> red nomal -> SSID:<nombre_ssid> PSWD:<pasword_red>");
    ESP_LOGI(TAG, "SETUP TCP CLIENT -> HOST_IP:<host_ip> PORT:<pureto>");
    //okay, este es la parte de WIFI, por lo primero debemos de poner que va a inicar la conexion 
    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();
    //inicamos con los parametros por defecto

    update_setup_cred(DEFAULT_HOST, DEFAULT_PORT, NULL, "HOST_IP");//establecer las credenciales para TCP 

    // es necesario establecer los apuntadores aaprtir de aqui??mmmm
    //flujo princpal de TCP 
    setup_tcp(); // inicamos todo el proceso del flujo si se conectata o no, y las psoibles flujos. 
}


void task_cmd_uart(void *params){

    char* cmd_receive;
    esp_err_t ret;

    while(1){

        //esta tarea espera a que se reciban los datos de la tarea que se encarga de monitaorear UART
        //esta tarea se encargara de ver que entro y seguir con el flujo 
        if(xQueueReceive(flow_data_queue, &cmd_receive, portMAX_DELAY)){

            //se recibio una cadena con los comandos, por lo que es necesario parsear la cadena en sus diferentes tokens 
            //se hace uso de esta funcion que tokenisa la cadena y devuvle los comandos si son correctos el programa debe de seguir  
            char **tokens = pasrse_input(cmd_receive);

            //si no se recibio nada pasa y espera la proxima ingresion de datos. 
            if(tokens == NULL || tokens[0] == NULL){
                free(cmd_receive);
                continue;
            }
            
            //si hubo comandos
            /**
             * - en este punto no se sabe que comadnos fueron los que entraron por lo que realizo una copio y asigno un nuevo 
             * espacio de memoria para el primer comando
             * 
             * ---NOTA: los comandos siempre vienen en ORDEN, si no viene en orden no se podra ejecutar  
             * 
             * actualmete la variable < tokens >> contiene la sigueinte informacion pro dar un ejemplo con WIFI 
             * 
             * ["SSID:INFINIMUN123", "PSWD:123456789"]
             * 
             * -> entonces < tmp > trata de sacar el primer comando que se introdujo
             * -- para saber que comandao es y como seguir con el flujo 
            */
            char *cmd_case = strdup(tokens[0]);

            char  *tipo = strtok(cmd_case, ":");
            //aseuramos que sea el comando y que venga con la contrasenia 
            if(strcmp(tipo,"SSID") ==0 && tokens[1] != NULL){
                //en este caso ocurrio un error y es necesario introducir otra red, pero el sismte intento conectarse a la red por defecto 

                //por lo que ahora necesito es serparar la parte que me importa del encabezado del comando 
                char *ssid = strchr(tokens[0], ':');
                char *pswd = strchr(tokens[1], ':');

                if(ssid == NULL || pswd == NULL){
                    ESP_LOGE(TAG,"formato incorrecto. Usa: SSID:<nombre> PSWD:<password>");
                    goto cleanup;
                }

                //brincamos ":"
                ssid++;
                pswd++;
                //actualizamos a las nuevas credenciales 
                ret = update_setup_cred(ssid, pswd, NULL, "SSID");
                //preparamos el envio de una senial 1 que indica que ya ha
                if(ret !=ESP_FAIL){
                    //activamos el bit
                    xEventGroupSetBits(s_wifi_event_group, WIFI_UPDATE); //evento para WIFI - reinica la conexion 
                    xEventGroupSetBits(g_tcp_event_group, BREAK_UPDATE_WIFI); //wvento para TCP - cierra todas las conexiones y espera a la conexion de WIFI para estbalcer 
                    //conexion con el servidor. 
                }
                else{
                    // uart_write_bytes(UART_MAIN,"\r\n", 2);
                    // const char *mssg = 
                    // uart_write_bytes(UART_MAIN,mssg, strlen(mssg));

                    ESP_LOGE(TAG, "no se puderon guardar las credenciales WIFI");

                }   
            }
            else if(strcmp(tipo,"HOST_IP") == 0 && tokens[1] !=NULL){
                //comandos HOST_IP:<IP> PORT:<puerto>

                //el procesoe s muy similar 
                char *host_ip = strchr(tokens[0], ':');
                char *port = strchr(tokens[1], ':');
                
                if (host_ip == NULL || port == NULL) {
                    ESP_LOGE(TAG, "formato incorrecto. Usa: HOST_IP:<ip> PORT:<puerto>");
                    goto cleanup;
                }
                //brincamos ":" 
                host_ip++;
                port++;
                //actualizamos las credenicales para TCP. 

                ret = update_setup_cred(host_ip, port, NULL, "HOST_IP");
            
                if(ret != ESP_FAIL){
                    //se puderin asignar las credenciales 
                    xEventGroupSetBits(g_tcp_event_group, UPDATE_TCP);
                }
                else{
                    ESP_LOGE(TAG, "nERROR: no se pudo guardar el nuevo servidor");

                }
            
            }
            //ahora es cuando se procesa los datos ingresados por el usuario "YES" o "NO" indicand que si quiere volver a intentear a establecer una conexion TCP con el servidor
            //con la esperanza que ya este arriab el servidor. 
            else{
                // pude que funcione no estoy muy seguro 
                //tokens nos va a regresar al menos 2 selemtnos en, uno contendra un valor y otro contendra NULL indicando el final del arreglo 

                //cunado ingresara a esta condiconal
                /**
                 * - para la iteracion de la conexion de TCP "YES" o "NO"
                 * -o cunado se modifique la matricula con la que se loggea 
                */

                if(strcmp(tipo, "YES") == 0 ){
                    xEventGroupSetBits(g_tcp_event_group, RETRY_SERVER);//quiere volver a intetnar la conexion con las mismas credenciales 
                }
                else if(strcmp(tipo, "NO") == 0){
                    xEventGroupSetBits(g_tcp_event_group, NO_RETRY_TCP);//haysa aqui llego pa
                }
                else{
                    //se ingreso algo diferente, por lo que sale 
                    ESP_LOGE(TAG,"ingreso una opcion incorrecta, operacion aborto");
                }
            }
            
            //con una red para UABC 
            //con un nuevo usuario

            cleanup:
            //liberamos tokens 
            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
            free(cmd_case);
            free(cmd_receive);

        }
    }
}

//este recibe del servidor y procesa lo que recibio

//este debe de recibir como parametros otros elementos

void tcp_process_task(void *params){
    
    format_request_t *frame;

    char buffer[80]; //para mostrar mensajes por UART 
    esp_err_t ret;

    uint8_t frame_len;
    int len;


    while(1){

        //recibira los datos por cola 
        if(xQueueReceive(tcp_rx_queue,&frame, portMAX_DELAY)){


            // ACK: identificador 0x3501 y len != 0xFF
            if(frame->id == ACK && frame->len != 0xFF){

                    if(login_pending) {
                        xEventGroupSetBits(g_login_event_group, LOGIN_SUCCESS);
                        login_pending = 0;
                    }

                len = snprintf(buffer, sizeof(buffer), "\r\nservidor contesta: ACK\r\n");
                uart_write_bytes(global_uart.NUM_PORT, UART_GREEN, strlen(UART_GREEN));
                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                vPortFree(frame);
                continue;
            }

            // NACK: identificador 0x3501 y len == 0xFF
            else if(frame->id == ACK && frame->len == 0xFF){

                if(login_pending) {
                    xEventGroupSetBits(g_login_event_group, LOGIN_FAIL);
                    login_pending = 0;
                }

                len = snprintf(buffer, sizeof(buffer), "\r\nservidor contesta: NACK\r\n");
                uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                vPortFree(frame);
                continue;
            }

            //en otro caso recibimos un comando del servidor (trama CAFE)
            else if(frame->id == HEADER){
                frame_len = 0;

                //si llego ahora debemos de ver uqe onda 

                //verificamos que sea para nosotros 
                if(frame->user != user){
                    len = snprintf(buffer, sizeof(buffer), "\r\npeticion no para este usuario\r\n");
                    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                    uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                    free(frame);
                    continue;
                }

                //ahora necesitamos ver que servicio es lo necesario 
                if(frame->action == read_esp){
                    switch(frame->resourse){
                        case led :{
                            //el estado esta 1 o 0, que solo abarca 1 byte
                            memcpy(send_info.format_request.value, &led_state, 1);
                            send_info.format_request.len=1;//solo usamos 1 byte para el led 
                            send_info.op_type = OP_ACK; //operacion ACK
                            ret = send_message();
                            if(ret != ESP_OK){
                                len = snprintf(buffer, sizeof(buffer), "\r\nERRO AL SER EL ENVIO DEL FRAME\r\n");
                                uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            }
                            
                        }break;

                        case adc:{
                            //adc puede ser un valor de 16 bits 
                            uint16_t adc_state = read_adc(ADC_CHANNEL);
                            uint16_t adc_net = htons(adc_state);     
                            memcpy(send_info.format_request.value, &adc_net, 2);
                            send_info.format_request.len = 2;
                            send_info.op_type =OP_ACK;
                            ret = send_message();

                            if(ret != ESP_OK){
                                len = snprintf(buffer, sizeof(buffer), "\r\nERRO AL SER EL ENVIO DEL FRAME\r\n");
                                uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            }
                        }break;

                        case pwm : {
                            uint16_t duty = pwm_get_duty();
                            uint8_t pct = (uint8_t)((duty * 100) / PWM_MAX);
                            send_info.format_request.value[0] = pct;
                            send_info.format_request.len = 1;
                            send_info.op_type = OP_ACK;
                            ret = send_message();

                            if(ret != ESP_OK){
                                len = snprintf(buffer, sizeof(buffer), "\r\nERRO AL SER EL ENVIO DEL FRAME\r\n");
                                uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            }
                        }break;

                        case resert_esp:{
                            uint64_t limite_total_us = (uint64_t)time_resert * 1000000ULL;
                            
                            uint64_t restante_us = (limite_total_us > reset_state) ? (limite_total_us - reset_state) : 0;
                            uint8_t segundos = (uint8_t)(restante_us / 1000000ULL);
                            send_info.format_request.value[0] = segundos;
                            send_info.format_request.len = 1;
                            send_info.op_type = OP_ACK;
                            ret = send_message(); //mandamos 
                        }break;

                        default: {
                            send_info.op_type = OP_NACK;
                            ret = send_message();
                        } break;
                    }
                }
                else if(frame->action == write_esp){
                    switch(frame->resourse){

                        case led:{
                            //quiere escribir 
                            led_state = frame->value[0];
                            gpio_set_level(OUTPUT_PIN, led_state);

                            memcpy(send_info.format_request.value, &led_state, 1);
                            send_info.format_request.len = 1;
                            send_info.op_type = OP_ACK;
                            //conestamos a la peticion 
                            ret = send_message();
                        }break;
                        //falta que mande el ACK con el valor estabelcido 
                        case pwm : {
                            uint8_t pct = frame->value[0];
                            if(pct > 100) pct = 100;
                            uint16_t duty = (pct * PWM_MAX) / 100;
                            pwm_set_duty(duty);
                            send_info.format_request.len = 1;
                            send_info.op_type = OP_ACK;
                            ret = send_message();
                        }break;

                        case resert_esp:{
                            //vamos a ecribir el byte que se reibio representan los segundos en los cuales de requieren reinicar la esp 
                            //establecer el tiempo que queremos que se relice el resert 
                            time_resert = frame->value[0];//el valor entara en el primero byte del frame porqeu sera un valor de 1 byte 
                            //ahora lo que sigue es crear la tarea princilapl 
                            xTaskCreate(task_resert_esp, "task_resert_esp", 2048, NULL, 5, NULL);
                            //
                            memcpy(send_info.format_request.value, &time_resert, 1); //sera de 1 byte
                            send_info.format_request.len = 1;
                            send_info.op_type = OP_ACK;
                            ret = send_message();
                            
                        }break;

                        case adc: {
                            len = snprintf(buffer, sizeof(buffer), "\r\noperacion con ADC incorrecta\r\n");
                            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));

                            //enviamos un NACK 
                            send_info.op_type = OP_NACK;
                            ret = send_message();
                        }break;

                        

                        default: {
                            send_info.op_type = OP_NACK;
                            ret = send_message();
                        } break;
                    }
                }

                else if(frame->action == cancel_resert_esp){
                    //quiere canelcar el resert de la esp 
                    //tna solo pasa en que debemos de activar el grupo de eventos que calncela todo 
                    xEventGroupSetBits(g_rst_event, RST_CANCEL);
                    send_info.format_request.value[0] = 0;
                    send_info.format_request.len=1;
                    send_info.op_type = OP_ACK;
                    ret = send_message();
                }

                
                vPortFree(frame);
            }

            else{
                len = snprintf(buffer, sizeof(buffer), "\r\nFORMTAMO INCORRECTO\r\n");
                uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));

                //debemos de enviar un NACK 

                send_info.op_type = OP_NACK;
                ret = send_message();
                if(ret != ESP_OK){
                    len = snprintf(buffer, sizeof(buffer), "\r\nERRO AL SER EL ENVIO DEL FRAME\r\n");
                    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                    uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                }
                free(frame);
                continue;
            }


            
        }
    }

}



char **pasrse_input(char *line){

    char **tokens = malloc(5 * sizeof(char*));
    char *token; 
    int position=0;


    token = strtok(line, " ");

    while(token != NULL){

        if(position >= 5 ) break;

        tokens[position++] = strdup(token);
        
        token = strtok(NULL, " ");
    }

    tokens[position] = NULL;
    
    
    return tokens;
}



char **pasrse_input_recv(char *line){

    char **tokens = malloc(7 * sizeof(char*));  
    char *token; 
    int position = 0;

    token = strtok(line, ":");

    while(token != NULL){
        if(position >= 6) break;        
        tokens[position++] = strdup(token);
        token = strtok(NULL, ":");
    }

    tokens[position] = NULL;
    return tokens;
}

//actualiza las credencilaes -----> necesita actualicion sobre las varibales de globales --> por referencia 
//ahora esta conexion sera mediante la estrucuutra para lleva run mejor controky que todo este agrupado en una sola varibale 
esp_err_t update_setup_cred(char *key, char *anchor ,char *pswd_ent,  char *identificator){

/**
  * estaba tratando de hacer 2 veces algo que ya hace la tarea. en esta funcion los guiaremos con el cuerto parametro que idnicara que tipo de actualizacion se necesita realizar 
  * 
  * 
*/

    if(strcmp(identificator, "SSID") == 0){

        esp_wifi.ssid =realloc(esp_wifi.ssid, strlen(key)+1);
        esp_wifi.pswd =realloc(esp_wifi.pswd, strlen(anchor)+1);

        if (esp_wifi.ssid != NULL && esp_wifi.pswd != NULL) {
            strcpy(esp_wifi.ssid, key);
            strcpy(esp_wifi.pswd, anchor);
            esp_wifi.type_connected=0; //conexion normal
            esp_wifi.user_name="/0";
            return ESP_OK;
        } else {
            uart_write_bytes(UART_MAIN,"\r\n", 2);
            const char *mssg = "MAIN - no hay memoria para las credenicales\0";
            uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
            return ESP_FAIL;
        }

    }
    //ahora cunado sea cambiar la IP
    

    //aqui tenemos un problema, debemos de cambiar el puerto a entero, ya que es lo que se necesita , pero todo resibira 
    //como caracter pero aqui jacemos la transformacion
    else if(strcmp(identificator, "HOST_IP") == 0){

        tcp_client.host_ip = realloc(tcp_client.host_ip, strlen(key)+1);
        if(tcp_client.host_ip != NULL){
            //la IP esta bien 
            strcpy(tcp_client.host_ip, key);
            // strcpy(tcp_client.host_port, anchor);
            //que que tiene el puerto es anchor por lo que aplicamos atoi
            
            int port_number = atoi(anchor);
            tcp_client.host_port = (uint16_t)port_number;
            return ESP_OK;
        }

        else{
            ESP_LOGE(TAG, "no se pudo asignar memoria para las nuevas llaves");
            return ESP_FAIL;
        }
    }

    
    return ESP_FAIL;

}


void setup_tcp(void)
{
    while (1)
    {
        esp_err_t ret = tcp_cliente_init();
        static int n_login = 0;
        int len;

        if (ret == ESP_OK)
        {
            uart_write_bytes(UART_MAIN, UART_GREEN, strlen(UART_GREEN));
            const char *m = "\r\nconexion con el servidor establecida\r\n";
            uart_write_bytes(UART_MAIN, m, strlen(m));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            // Lanzar tareas de recepción y procesamiento
            xTaskCreate(recv_task, "recv_task", 4098, NULL, 8, NULL);
            xTaskCreate(tcp_process_task, "tcp_process_task", 4098, NULL, 8, NULL);

            // ── Autenticación (login) ────────────────────────────────────
            do_login:
            if (tcp_client.logged_in != 1)
            {
                char n_rety_log[64];
                len = snprintf(n_rety_log, sizeof(n_rety_log), "intento #%d de login...\r\n", n_login);
                uart_write_bytes(global_uart.NUM_PORT, n_rety_log, len);

                login_pending = 1;
                send_info.op_type = OP_LOGIN;
                send_info.format_request.len = 5;   // user (4) + acción/recurso (1)
                ret = send_message();

                if (ret != ESP_OK)
                {
                    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                    uart_write_bytes(global_uart.NUM_PORT, "\r\nError al enviar login\r\n", 24);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                    login_pending = 0;
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    continue;
                }

                // Esperar respuesta del servidor (ACK o NACK) máximo 5 segundos
                EventBits_t bits = xEventGroupWaitBits(g_login_event_group,
                                                        LOGIN_SUCCESS | LOGIN_FAIL,
                                                        pdTRUE, pdFALSE,
                                                        pdMS_TO_TICKS(5000));

                if (bits & LOGIN_SUCCESS)
                {
                    tcp_client.logged_in = 1;
                    uart_write_bytes(global_uart.NUM_PORT, UART_GREEN, strlen(UART_GREEN));
                    uart_write_bytes(global_uart.NUM_PORT, "\r\nLogin exitoso (usuario autenticado)\r\n", 41);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                }
                else if (bits & LOGIN_FAIL)
                {
                    tcp_client.logged_in = 0;
                    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                    uart_write_bytes(global_uart.NUM_PORT, "\r\nLogin fallido (credenciales inválidas o servidor rechazó)\r\n", 58);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    goto do_login;   // reintentar login
                }
                else
                {
                    // Timeout
                    login_pending = 0;
                    uart_write_bytes(global_uart.NUM_PORT, UART_YELLOW, strlen(UART_YELLOW));
                    uart_write_bytes(global_uart.NUM_PORT, "\r\nSin respuesta del servidor (login timeout)\r\n", 45);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                    vTaskDelay(pdMS_TO_TICKS(3000));
                    continue;
                }
            }
            // ── Fin login ───────────────────────────────────────────────

            // Crear tarea de keep-alive una sola vez
            static bool ka_created = false;
            if (!ka_created)
            {
                xTaskCreate(keep_alive_task, "keep_alive_task", 2048, NULL, 6, NULL);
                ka_created = true;
            }

            // Esperar eventos que pueden romper la conexión
            EventBits_t btis = xEventGroupWaitBits(g_tcp_event_group,
                                                    BREAK_UPDATE_WIFI | UPDATE_TCP | TCP_DISCONNECTED,
                                                    pdTRUE, pdFALSE, portMAX_DELAY);

            // Cerrar socket y marcar como desconectado
            if (tcp_client.sock >= 0)
            {
                close(tcp_client.sock);
                tcp_client.sock = -1;
            }
            tcp_client.connected = 0;
            tcp_client.logged_in = 0;

            if (btis & BREAK_UPDATE_WIFI)
            {
                wifi_reconnect();
                continue;
            }

            if (btis & TCP_DISCONNECTED)
            {
                uart_write_bytes(UART_MAIN, UART_YELLOW, strlen(UART_YELLOW));
                const char *disc = "\r\nconexion perdida. intentando reconectar...\r\n";
                uart_write_bytes(UART_MAIN, disc, strlen(disc));
                uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

                int reconectado = 0;
                for (int i = 1; i <= 5; i++)
                {
                    char intento[48];
                    int tlen = snprintf(intento, sizeof(intento), "reconexion intento %d/5...\r\n", i);
                    uart_write_bytes(UART_MAIN, intento, tlen);
                    vTaskDelay(pdMS_TO_TICKS(2000));

                    if (tcp_cliente_init() == ESP_OK)
                    {
                        reconectado = 1;
                        break;
                    }
                }

                if (reconectado)
                {
                    uart_write_bytes(UART_MAIN, UART_GREEN, strlen(UART_GREEN));
                    const char *ok = "\r\nreconexion exitosa\r\n";
                    uart_write_bytes(UART_MAIN, ok, strlen(ok));
                    uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
                    tcp_client.logged_in = 0;
                    xTaskCreate(recv_task, "recv_task", 4098, NULL, 8, NULL);
                    xTaskCreate(tcp_process_task, "tcp_process_task", 4098, NULL, 8, NULL);
                    ret = ESP_OK;
                    goto do_login;
                }

                // Fallaron los 5 intentos: preguntar al usuario
                uart_write_bytes(UART_MAIN, UART_YELLOW, strlen(UART_YELLOW));
                const char *fail_msg =
                    "\r\nno se pudo reconectar tras 5 intentos.\r\n"
                    "opciones:\r\n"
                    "  YES ---> reintentar con las mismas credenciales\r\n"
                    "  NO  ---> reiniciar ESP\r\n"
                    "  HOST_IP:<ip> PORT:<puerto> ---> cambiar servidor\r\n"
                    "  SSID:<ssid> PSWD:<pswd>    ---> cambiar red WIFI\r\n";
                uart_write_bytes(UART_MAIN, fail_msg, strlen(fail_msg));
                uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

                xEventGroupClearBits(g_tcp_event_group, RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP);
                EventBits_t user_bits = xEventGroupWaitBits(g_tcp_event_group,
                                                            RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP,
                                                            pdTRUE, pdFALSE, portMAX_DELAY);

                if (user_bits & NO_RETRY_TCP)
                {
                    uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
                    const char *bye = "\r\nterminando... reiniciando ESP\r\n";
                    uart_write_bytes(UART_MAIN, bye, strlen(bye));
                    uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    esp_restart();
                }

                if (user_bits & BREAK_UPDATE_WIFI)
                {
                    if (tcp_client.sock >= 0)
                    {
                        close(tcp_client.sock);
                        tcp_client.sock = -1;
                    }
                    tcp_client.connected = 0;
                    wifi_reconnect();
                }
                // Para RETRY_SERVER o UPDATE_TCP se continúa el bucle while
                continue;
            }
            // Para UPDATE_TCP simplemente se vuelve a llamar tcp_cliente_init() al inicio del while
        }
        else
        {
            // Falló la conexión TCP inicial
            uart_write_bytes(UART_MAIN, UART_YELLOW, strlen(UART_YELLOW));
            const char *m = "\r\nno se pudo conectar al servidor.\r\n"
                            "opciones:\r\n"
                            "  YES ---> reintentar\r\n"
                            "  NO  ---> reiniciar ESP\r\n"
                            "  HOST_IP:<ip> PORT:<puerto> ---> cambiar servidor\r\n"
                            "  SSID:<ssid> PSWD:<pswd>    ---> cambiar red WIFI\r\n";
            uart_write_bytes(UART_MAIN, m, strlen(m));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            xEventGroupClearBits(g_tcp_event_group, RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP);
            EventBits_t bits = xEventGroupWaitBits(g_tcp_event_group,
                                                    RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP,
                                                    pdTRUE, pdFALSE, portMAX_DELAY);

            if (bits & NO_RETRY_TCP)
            {
                uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
                const char *bye = "\r\nterminando... reiniciando ESP\r\n";
                uart_write_bytes(UART_MAIN, bye, strlen(bye));
                uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
                vTaskDelay(pdMS_TO_TICKS(1000));
                esp_restart();
            }

            if (bits & BREAK_UPDATE_WIFI)
            {
                if (tcp_client.sock >= 0)
                {
                    close(tcp_client.sock);
                    tcp_client.sock = -1;
                }
                tcp_client.connected = 0;
                wifi_reconnect();
            }
            // Para RETRY_SERVER o UPDATE_TCP se continúa el bucle (reintentará conexión)
        }
    }
}



void task_resert_esp(void *params){

    uint8_t close_servicios = 1; //un colchon de 1 segundos para cerrar TCP y WIFI 
    //en esta tarea se mantendra 
    if (time_resert <= close_servicios) {
        esp_restart(); // no hay tiempo suficiente ni para el colchon
    }
    //convertirmos el timepo en segundos 
    limite_us = (uint64_t)(time_resert - close_servicios) * 1000000ULL;
    //creamos la tarea para empeozar a contar 
    xTaskCreate(set_resert_time_task, "set_resert_time_task", 2048, NULL, 5, &handle_resert_set);
    //debriamos de esperar a que cancelamos o se completa el resert
        
    //un grupo de eventos con un bit para eso 
    EventBits_t bits = xEventGroupWaitBits(g_rst_event, RST_CANCEL | RST_SUCCESS, pdTRUE, pdFALSE, portMAX_DELAY);
    //verificamos cual se activo 
    if(bits & RST_CANCEL){
        //qioere decir que se canelo el resert 
        // primero matar la tarea para que no hyana errores 
        vTaskDelete(handle_resert_set);
        limite_us=0;
        time_resert = 0;
        init_count=0;
        reset_state=0;
        vTaskDelete(NULL); //eliminamos esta tarea 
            // break; //salimos del ciclo a elminar esta tarea hasta que se vuelva a necesitar 
    }
    else{
            
            //no se cancelo y llego a la cenleacion por lo que se procese a cerrar todas las coneciones etc.. 
            //eliminamos la tarea 
        char msg[60];
        int len = sniprintf(msg, sizeof(msg), "\n\rreinciando esp\r\n");
        uart_write_bytes(UART_NUM_0, msg, len);
        close(tcp_client.sock);
        vTaskDelete(handle_resert_set);
        esp_restart();
    }
    //creo que nucna se llega 
    vTaskDelete(NULL);
}

void set_resert_time_task(void *params){

    init_count = esp_timer_get_time();

    char msg[60];

    while(1){
        reset_state = esp_timer_get_time() - init_count;

        // int len = snprintf(msg,sizeof(msg),"resert en : %llu...", reset_state);
        // uart_write_bytes(UART_NUM_0, UART_YELLOW, strlen(UART_YELLOW));
        // uart_write_bytes(UART_NUM_0, msg, len);
        // uart_write_bytes(UART_NUM_0, UART_RESET, strlen(UART_RESET));

        ///ahora asi, dedebemos de verificiar tner 1 o 2 segunso menos, de colchon para realizar cierres 

        if(reset_state >= limite_us){
            //activamos el bit que inidiq eu se llego al conteio final 
            xEventGroupSetBits(g_rst_event, RST_SUCCESS);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}






void gpio_init(){

    gpio_reset_pin(OUTPUT_PIN);
    gpio_reset_pin(PWM_LED);

    // led_state = 0;

    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_PIN, led_state);

    gpio_set_direction(PWM_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(PWM_LED, 0);
}


//validacion para modificar valores de los recursos 

static int parse_number(const char *str)
{
    if (str == NULL || strlen(str) == 0) return -1;

    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i])) return -1;
    }

    return atoi(str);
}


// escenario 1: valor debe ser 0 o 1  (LED, flags, etc.)
esp_err_t validate_binary(const char *str, uint8_t *out)
{
    int val = parse_number(str);

    if (val < 0) {
        ESP_LOGE(TAG, "no es un numero: '%s'", str);
        return ESP_FAIL;
    }
    if (val < 0 || val > BINARY_MAX) {
        ESP_LOGE(TAG, "fuera de rango [0-1]: %d", val);
        return ESP_FAIL;
    }

    *out = (uint8_t)val;
    return ESP_OK;
}


esp_err_t validate_pwm(const char *str, uint16_t *out)
{
    int val = parse_number(str);

    if (val < 0) {
        ESP_LOGE(TAG, "no es un numero: '%s'", str);
        return ESP_FAIL;
    }
    if (val > PWM_MAX) {
        ESP_LOGE(TAG, "fuera de rango [0-8191]: %d", val);
        return ESP_FAIL;
    }

    *out = (uint16_t)val;
    return ESP_OK;
}
