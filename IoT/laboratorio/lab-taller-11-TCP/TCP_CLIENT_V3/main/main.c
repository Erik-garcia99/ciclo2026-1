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

int led_state;

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
    int led_state=0;


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
    ESP_LOGI(TAG, "SETUP TCP SERVER -> HOST_IP:<host_ip> PORT:<pureto>");
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
                //flata cunado se cambia de matricula 
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

void tcp_process_task(void *params){

    char *msg;
    char **tokens;
    

    //esta variable lo que nos va a indicar sera en que token de la peticion de servidor esta
    //esto para verificar que la solicitus este hecha adecuadamente. 
    //mod 
    static int mem_request;
    int modulo = 7; // 0 - 1 - 2 - 3 - 4 - 5 - 6 {aux -> NULL}- 0 
    char buffer[80]; //para mostrar mensajes por UART 


    esp_err_t ret;


    while(1){

        //recibira los datos por cola 
        if(xQueueReceive(tcp_rx_queue,&msg, portMAX_DELAY)){
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


            /**
             * duranete todo el procesos de leer, debeomos de verificar que la solicitud sea correcta. 
             * si en algun punto no es correcto se debe de mandar el NACK indicando que no es correcto  
             * 
            */

            char *aux;
            while(aux != NULL){
                //token toma el sigueinte token necesario para la compracacion
                aux = tokens[mem_request];

                //verificamos "UABC"
                if(strcmp(aux,format_request.header)!=0){
                    //si es diferente a 0 quiere decir que no esta bien por lo que debemos de mandar un NACK 
                    send_info.op = OP_NACK;
                    send_message(&tcp_client, &send_info);

                }
                //el seguno token que debe de ser mi matricula, en si, hasta ahora esta bien, 
                /**
                 * en si no esta mal, pero si estamos en un broadcast, este mensaje no iria para mi, entonces debemos 
                 * de indicar que la peticion no es para mi, salir y esperar a que llegue otro mensaje 
                 * 
                 * en mi caso tendria que entras "a1275863"
                 */
                if(strcmp(aux,format_request.user) != 0 ){
                    int len = snprintf(buffer,sizeof(buffer), "\n\rMAIN : peticion no para usuario: %s\r\n",format_request.user);
                    
                    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                    uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                    uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                    goto cleanup;
                    break; //terminamos, salimos porque no para nosotros el mesanje y esperamos a que vuelva a recibir algo 
                }
                /**
                 * ahora viene lo buneo, es el saber que quiere hacer y recopilar los datos. 
                 * primero antes que nada debemos saber que es lo que quiere hacer el usuario con los recursos que tenemos
                 * si quiere leer o excribir en el. 
                 * 
                 * 
                 * tenemos el apoyo del enum, por lo que esto nos puede facilitar el como va a seguir el codigo
                 * se puede agrega run switch-case, para facilitar la recopilacion de los datos con base a la OP que se quiere realizar 
                 */
                // deberian de ser caracter R o W
                if(*aux == format_request.operation[0]){
                    //la operacion ser de lectrua
                    //debemos de aumentar en 1 elemeto, porque si no, nos estaria dando un error
                    /**
                     * ya que se estaria manetneindo en 'R', entonces no va a reciri 'L' o 'A' 
                     * 
                     */
                    mem_request = (mem_request + 1) % modulo;
                    //ahora debemos de saber que operacion quier leer
                    //se pueden leer todos los recursos, LED, ADC y PWM 
                    //necesitmaos funciones que nos traigan esos valores 

                    if(*aux == format_request.resource[0]){
                        //quiere leer a led 
                        send_info.op = OP_ACK;
                        send_info.value = led_state; //pasamos el valor del led actual.
                        ret = send_message(&tcp_client, &send_info); //mandamos la infromacion 
                    }

                    //ahora quiere leer ADC 
                    if(*aux == format_request.resource[1]){
                        send_info.op = OP_ACK;
                        uint16_t val = read_adc(ADC_CHANNEL);
                        send_info.value = val;
                        ret = send_message(&tcp_client, &send_info);
                        
                    }

                    if(*aux == format_request.resource[2]){
                        //leer PWM 
                        send_info.op = OP_ACK;
                        uint16_t val = pwm_get_duty();
                        send_info.value = val;
                        ret = send_message(&tcp_client, &send_info);
                    }

                    //puede que haya puesto un recurso que no esta 

                    else{
                        //recurso que no esta listado 
                        int len = snprintf(buffer, sizeof(buffer), "MAIN: \r\nrecurso [%c] no listado\r\n",format_request.resource);
                        uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                        uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                        uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                        goto cleanup;
                        break;
                    }

                }
                else if(*aux == format_request.operation[1]){
                    //la operacion sera de escritura 
                    
                    //debemos de brincar en donde me enceuntro actualmente hacia el sigueint token que es 
                    //al recurso que vamos a escribir. 
                    
                    /**
                     * aqui hay 2 cosas, tenemos que verificar que se introdujo un valor numerico valido y 
                     * convertir este valor numerico en un numero de 16 bits.
                     * 
                     */

                    //valor maximo al pwm - 8191
                    //en este punto estamos en el miembro #4 
                    mem_request = (mem_request + 1) % modulo;
                    //para escribir solamente a L y a ADC
                    if(*aux == format_request.resource[0]){
                        //quiere escirbir a led
                        /**
                         * para el led solo pueden ser 0 o 1, prendido o apagodo mi pa. 
                         */

                        //primero antes que nada debemos de verificar que los datos ingresado, el valor es correcto

                        uint8_t *val;
                        if(validate_binary(aux[mem_request], &val) == ESP_OK){
                            //modificamos el estado del led 
                            led_state = val;
                            gpio_set_level(OUTPUT_PIN, led_state);
                            send_info.op = OP_ACK,
                            send_info.value = val;
                            ret = send_message(&tcp_client, &send_info);

                        }
                        else{
                            int len = snprintf(buffer, sizeof(buffer), "\r\nMAIN: valor para LED invalido (0 - 1)\r\n");
                            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            goto cleanup;
                            break;

                        }
                    }

                    if(*aux == format_request.resource[2]){
                        //quiere escribir al PWM
                        uint16_t *val_pwm;
                        if(validate_pwm(aux[mem_request], &val_pwm) ==ESP_OK){
                            //valor OK 
                            pwm_set_duty(val_pwm);
                            send_info.op = OP_ACK;
                            send_info.value = val_pwm;
                            ret = send_message(&tcp_client, &send_info);
                        }
                        else{
                            int len = snprintf(buffer, sizeof(buffer), "\r\nMAIN: valor para PWD invalido (0 - 8191)\r\n");
                            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                            goto cleanup;
                            break;
                        }
                    }
                    else{
                        int len = snprintf(buffer, sizeof(buffer), "\r\nMAIN: recruso invalido R/W ->  (LED, PWM)\r\n");
                        uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                        uart_write_bytes(global_uart.NUM_PORT, buffer, len);
                        uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                        goto cleanup;
                        break;
                    }

                }
                
                //lo otro seri el mensake lo cual ese no importa, y aumenta el moculo 


                //por ahora este aumentara a posiblmente 4 -> que seria o valor o mensaje 
                mem_request = (mem_request + 1) % modulo; //se incrementa o se reinica el modulo

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

    char **tokens = malloc(5 * sizeof(char*));
    char *token; 
    int position=0;


    token = strtok(line, ":");

    while(token != NULL){

        if(position >= 5 ) break;

        tokens[position++] = strdup(token);
        
        token = strtok(NULL, " ");
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


        //se pudo ingresar las credenciales bien por lo que ahora intentara establecer conexion con el servidor. 
        esp_err_t ret = tcp_cliente_init(); 
        
        static int n_login=0;
        int len;
        if(ret == ESP_OK){
            //hubo exito, el servidor esta arriba 
            uart_write_bytes(UART_MAIN, UART_GREEN, strlen(UART_GREEN));
            const char *m = "\r\nconexion con el servidor establecida\r\n";
            uart_write_bytes(UART_MAIN, m, strlen(m));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            //aqui estareiamos creando las tareas de resv y sned y todo lo demas que seria para la comunicion 
            /**
             * entre el server y el ESP, peticiones y rescpuesta 
            */
            /**
             * ---> TAREAS:  <--- 
             * 
             */

            //primero antes que nada debemos de inicar sesion dentro del servidor, si se logra inicar sesion es cuando empezamos a mandar nuestros keep alive 
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

            // ── Crear tarea de keep‑alive (solo una vez) ───────────────
            static bool ka_created = false;
            if (!ka_created) {
                xTaskCreate(keep_alive_task, "keep_alive_task", 2048, NULL, 6, NULL);
                xTaskCreate(recv_task,  "recv_task", 4098, NULL, 8, NULL);
                xTaskCreate(tcp_process_task, "tcp_process_task", 4098,NULL, 8, NULL);
                ka_created = true;
            }

            //ahora lo unico que podria interferir es que se cambie de red o de servidor. 

            EventBits_t btis= xEventGroupWaitBits(g_tcp_event_group, BREAK_UPDATE_WIFI | UPDATE_TCP, pdTRUE, pdFALSE, portMAX_DELAY);
            //si pasa uno de estos 2 eventos debemos de cerrar todas las conexiones. 

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
        //----- FALLO LA CONEXION CON TCP ----

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
        // esperar la decision del usuario (task_cmd_uart activa el bit correspondiente)
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

            // reconectar WIFI con las nuevas credenciales ya guardadas
            wifi_reconnect();
            // despues del while vuelve a intentar TCP
        }

        // RETY_SERVER y UPDATE_TCP: el while simplemente vuelve a llamar tcp_cliente_init()
        // con los valores actuales de tcp_client.host_ip y tcp_client.host_port


    }

}


void gpio_init(){

    gpio_reset_pin(OUTPUT_PIN);
    gpio_reset_pin(PWM_LED);

    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_PIN, 0);

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

