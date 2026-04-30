//falta la modificacion para modificar la matricula, momas puse todo pero aun no lo tengo 

/**
 * arreglos: 
 * 
 * -- debemos de arreglas, el converitr el puerto que viene como un string a un tipo entero, ya que el miembro de la estrucutra espera un int
 *          al igual que la funcion que establece la conexion con el descriptro connected() s
 * 
 * 
 * o es ESP_LOG o es UART, (creo que al final sera UART, pero por ahora solo dejemoslo como vamos )
 * 
 * 
*/


/**
 * debe de haber algo que me indicque que tareas ya han sido creadas, no se que pueda pasar si la funcion pasa cada vez y crea la tarea. 
 * si, debemos de tneer una bandera que nos idnique que las tareas ya estan creadas y corriendo, cunado se haga login. 
 * 
 * 
 * 
*/


#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>

//drivers
#include<driver/uart.h>

//logs
#include<esp_log.h>
#include <esp_err.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

//librerias 
#include<modules/UART/uart_lib.h>
#include<modules/WIFI/wifi_lib.h>
#include<modules/TCP/tcp_lib.h>
#include"modules/ADC/adc_lib.h"
#include"modules/PWM/pwm_lib.h"
#include<global.h>


//vairbales globales
static const char *TAG = "MAIN";

uint8_t led_state;

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


//++++++++++++++++++  estrucutra
//contiene valores de UART 
task_uart_port_t global_uart;
//estucuturua para red 
esp_wifi_t esp_wifi;
//estrucutra para parametros de la conexion tcp
tcp_client_t tcp_client;
//enum de operacion CP 
op_type_t op_type;
//estrucutra de la uncion que agrupa los datos
send_info_t send_info;

//estrucutra con el formato para la peticion del servidor con los servicios del dispostivo
format_request_t format_request;

//+++++++++++++++ variables 

//identificacion por la matricula 
char user[MAX_USER_LEN];



//funciones
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

//tarea encargada de recibir el comando por UART 
void task_cmd_uart(void *params);


static int parse_number(const char *str);


void app_main(void)
{



    //creamos el grupo de eventos para WIFI 
    s_wifi_event_group = xEventGroupCreate();
    g_tcp_event_group= xEventGroupCreate();
    
    flow_data_queue = xQueueCreate(10, sizeof(char*));
    tcp_rx_queue = xQueueCreate(10, sizeof(char *));

    
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


    //definimos la matricual con la que se inicara sesion  
    strcpy(user,"a1275863");

    //inicamos la estrucutra que deberan de dar formato a la estrucutra de format request

    format_request.header = "UABC";
    format_request.resource[0] = 'L'; //LED : R and W
    format_request.resource[1] = 'A'; //ADC : only R
    format_request.resource[2] = 'P'; // PWD : R nad W

    format_request.operation[0] = 'R'; // read
    format_request.operation[1] = 'W'; //wriute

    format_request.user = user;   



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


    //flujo princpal de TCP 
    setup_tcp(); // inicamos todo el proceso del flujo si se conectata o no, y las psoibles flujos. 




}


void task_cmd_uart(void *params){

    char* cmd_receive;
    esp_err_t ret;

    while(1){

        if(xQueueReceive(flow_data_queue, &cmd_receive, portMAX_DELAY)){

            char **tokens = pasrse_input(cmd_receive);
 
            if(tokens == NULL || tokens[0] == NULL){
                free(cmd_receive);
                continue;
            }

            char *cmd_case = strdup(tokens[0]);

            char  *tipo = strtok(cmd_case, ":");

            if(strcmp(tipo,"SSID") ==0 && tokens[1] != NULL){
                 
                char *ssid = strchr(tokens[0], ':');
                char *pswd = strchr(tokens[1], ':');

                if(ssid == NULL || pswd == NULL){
                    ESP_LOGE(TAG,"formato incorrecto. Usa: SSID:<nombre> PSWD:<password>");
                    goto cleanup;
                }

                ssid++;
                pswd++;
                ret = update_setup_cred(ssid, pswd, NULL, "SSID");
                if(ret !=ESP_FAIL){
                    xEventGroupSetBits(s_wifi_event_group, WIFI_UPDATE); //evento para WIFI 
                    xEventGroupSetBits(g_tcp_event_group, BREAK_UPDATE_WIFI); //wvento para TCP - 
                }
                else{

                    ESP_LOGE(TAG, "no se puderon guardar las credenciales WIFI");

                }   
            }
            else if(strcmp(tipo,"HOST_IP") == 0 && tokens[1] !=NULL){

                char *host_ip = strchr(tokens[0], ':');
                char *port = strchr(tokens[1], ':');
                
                if (host_ip == NULL || port == NULL) {
                    ESP_LOGE(TAG, "formato incorrecto. Usa: HOST_IP:<ip> PORT:<puerto>");
                    goto cleanup;
                }

                host_ip++;
                port++;
                
                ret = update_setup_cred(host_ip, port, NULL, "HOST_IP");
            
                if(ret != ESP_FAIL){
                    xEventGroupSetBits(g_tcp_event_group, UPDATE_TCP);
                }
                else{
                    ESP_LOGE(TAG, "nERROR: no se pudo guardar el nuevo servidor");

                }
            
            }
             
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
                //flata cunado se cambia de matricula 
            }

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

void tcp_process_task(void *params){

    char *msg;
    char **tokens;
    static int mem_request;
    char buffer[80]; 

    esp_err_t ret;

    while(1){

        //recibira los datos por cola 
        if(xQueueReceive(tcp_rx_queue,&msg, portMAX_DELAY)){


            //el mas sencicllo, verificamos con ACK o en NACK del servidor. 
            if(strncmp(msg, "ACK", 3) == 0){
            int len = snprintf(buffer, sizeof(buffer), "\r\nMAIN: ACK recibido -> %s\r\n", msg);
            uart_write_bytes(global_uart.NUM_PORT, UART_GREEN, strlen(UART_GREEN));
            uart_write_bytes(global_uart.NUM_PORT, buffer, len);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
            free(msg);
            continue;  // no parsear, esperar siguiente mensaje
        }

        if(strncmp(msg, "NACK", 4) == 0){
            int len = snprintf(buffer, sizeof(buffer), "\r\nMAIN: NACK recibido -> %s\r\n", msg);
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT, buffer, len);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
            free(msg);
            continue;
        }

            /**
             * se supone que en este punto se recibe
             * --> UABC:a1275863:R:L:leer led
             * 
             * algo asi es lo que va a llegar por recv
             * 
             * 
            */
            // inicamos, 
            mem_request=0;
            tokens=pasrse_input_recv(msg);

            //verificamos que se haya parseado correctamente 
            if(tokens == NULL || tokens[0] == NULL){
                free(msg);
                continue;
            }
            char current_op = 0;   // guarda 'R' o 'W' del case 2 para usarlo en case 3
            char *aux;
            for(mem_request = 0; tokens[mem_request] != NULL; mem_request++){
                aux = tokens[mem_request];

                switch(mem_request){

                    case 0:  // "UABC"
                        if(strcmp(aux, format_request.header) != 0){
                            send_info.op = OP_NACK;
                            send_message(&tcp_client, &send_info);
                            goto cleanup;
                        }
                        break;

                    case 1:  // usuario
                        if(strcmp(aux, user) != 0){
                            int len = snprintf(buffer, sizeof(buffer), "\r\nN- MAIN: peticion no para: %s\r\n", format_request.user);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RED,   strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT, buffer,     len);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            goto cleanup;
                        }
                        break;

                    case 2:  // operacion R o W
                        current_op = *aux;
                        if(current_op != 'R' && current_op != 'W'){
                            send_info.op = OP_NACK;
                            send_message(&tcp_client, &send_info);
                            goto cleanup;
                        }
                        break;

                    case 3:  // recurso L, A, P
                        if(current_op == 'R'){
                            if(*aux == 'L'){
                                send_info.op = OP_ACK;
                                send_info.value = led_state;
                                send_message(&tcp_client, &send_info);
                            }
                            else if(*aux == 'A'){
                                send_info.op = OP_ACK;
                                send_info.value = read_adc(ADC_CHANNEL);
                                send_message(&tcp_client, &send_info);
                            }
                            else if(*aux == 'P'){
                                send_info.op = OP_ACK;
                                send_info.value = pwm_get_duty();
                                send_message(&tcp_client, &send_info);
                            }
                            else{
                                send_info.op = OP_NACK;
                                send_message(&tcp_client, &send_info);
                                goto cleanup;
                            }
                        }
                        else{  // W
                            if(*aux == 'A'){  // ADC no se puede escribir
                                send_info.op = OP_NACK;
                                send_message(&tcp_client, &send_info);
                                goto cleanup;
                            }
                            // L y P necesitan el valor del case 4, lo guardamos
                            // solo validamos que el recurso sea válido
                            if(*aux != 'L' && *aux != 'P'){
                                send_info.op = OP_NACK;
                                send_message(&tcp_client, &send_info);
                                goto cleanup;
                            }
                        }
                        break;

                    case 4:  // valor (solo para W)
                        if(current_op == 'W'){
                            char recurso = *tokens[3];
                            if(recurso == 'L'){
                                uint8_t val;
                                if(validate_binary(aux, &val) == ESP_OK){
                                    led_state = val;
                                    gpio_set_level(OUTPUT_PIN, led_state);
                                    send_info.op    = OP_ACK;
                                    send_info.value = val;
                                    send_message(&tcp_client, &send_info);
                                } else {
                                    send_info.op = OP_NACK;
                                    send_message(&tcp_client, &send_info);
                                    goto cleanup;
                                }
                            }
                            else if(recurso == 'P'){
                                uint16_t val;
                                if(validate_pwm(aux, &val) == ESP_OK){
                                    pwm_set_duty(val);
                                    send_info.op    = OP_ACK;
                                    send_info.value = val;
                                    send_message(&tcp_client, &send_info);
                                } else {
                                    send_info.op = OP_NACK;
                                    send_message(&tcp_client, &send_info);
                                    goto cleanup;
                                }
                            }
                        }
                        break;

                    case 5:  // comentario, no importa
                        break;

                    default:
                        break;
                }
            }

            
            //al ultimo libreramos la memots 
            cleanup:
            //liberamos tokens 
            for (int i = 0; tokens[i] != NULL; i++) {
                free(tokens[i]);
            }
            free(tokens);
            free(msg);
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

static inline void uart_jump(void) {
    uart_write_bytes(UART_MAIN, "\r\n", 2);
}

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


void setup_tcp(void){
    
    while(1){

        //creando el socket TCP 
        esp_err_t ret = tcp_cliente_init(); 

        static int n_login=0;
        int len;

        if(ret == ESP_OK){

            uart_write_bytes(UART_MAIN, UART_GREEN, strlen(UART_GREEN));
            const char *m = "\r\nconexion con el servidor establecida\r\n";
            uart_write_bytes(UART_MAIN, m, strlen(m));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            if(tcp_client.logged_in != 1){
                
                char n_rety_log[64];
                len= snprintf(n_rety_log, sizeof(n_rety_log),"intento #%i\r\n",n_login);
                uart_write_bytes(global_uart.NUM_PORT, n_rety_log, len);
                //no ha inicado sesion. 
                send_info.op = OP_LOGIN;

                ret = send_message(&tcp_client, &send_info);
                //verificamos que se vuelva a intentar el login 
                if(ret != ESP_OK){
                    
                    //no se pudo conectar 
                    char err[80];
                    len = snprintf(err, sizeof(err), "\r\nno se pudo hacer login al servidor, revise si esta arriba\r\n ");
                    uart_write_bytes(global_uart.NUM_PORT, err, len);
                    continue;
                }

                //se logro hacer el login 
                tcp_client.logged_in = 1;

            }

            static bool ka_created = false;
            if (!ka_created) {
                xTaskCreate(keep_alive_task, "keep_alive_task", 2048, NULL, 6, NULL);
                xTaskCreate(recv_task,  "recv_task", 4098, NULL, 8, NULL);
                xTaskCreate(tcp_process_task, "tcp_process_task", 4098,NULL, 8, NULL);
                ka_created = true;
            }

            EventBits_t btis= xEventGroupWaitBits(g_tcp_event_group, BREAK_UPDATE_WIFI | UPDATE_TCP, pdTRUE, pdFALSE, portMAX_DELAY);

            if(tcp_client.sock>=0){
                close(tcp_client.sock);
                tcp_client.sock=-1;
            }
            tcp_client.connected=0;

            if(btis & BREAK_UPDATE_WIFI){
                wifi_reconnect(); //reconectamos wifi
            }

            //si fuese una actulizacion de TCP, simpe,emte se vuelve a llamar a la funcion que crea el socket 
            continue;
        }

        uart_write_bytes(UART_MAIN, UART_YELLOW, strlen(UART_YELLOW));
        const char *m = "\r\nno se pudo conectar al servidor.\r\n"
                        "opciones:\r\n"
                        "  YES ---> reintentar\r\n"
                        "  NO  ---> reiniciar ESP\r\n"
                        "  HOST_IP:<ip> PORT:<puerto> ---> cambiar servidor\r\n"
                        "  SSID:<ssid> PSWD:<pswd>    ---> cambiar red WIFI\r\n";
        uart_write_bytes(UART_MAIN, m, strlen(m));
        uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

        // limpiar bits relevantes antes de esperar
        xEventGroupClearBits(g_tcp_event_group,RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP);
        EventBits_t bits = xEventGroupWaitBits(g_tcp_event_group, RETRY_SERVER | UPDATE_TCP | BREAK_UPDATE_WIFI | NO_RETRY_TCP,pdTRUE,pdFALSE,portMAX_DELAY);

        if (bits & NO_RETRY_TCP) {
            uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
            const char *bye = "\r\nterminando... reiniciando ESP\r\n";
            uart_write_bytes(UART_MAIN, bye, strlen(bye));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }

        if (bits & BREAK_UPDATE_WIFI) {
            // cerrar socket si habia uno abierto
            if (tcp_client.sock >= 0) {
                close(tcp_client.sock);
                tcp_client.sock = -1;
            }
            tcp_client.connected = 0;

            wifi_reconnect();

        }


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

