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
EventGroupHandle_t g_user_def;

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

uint32_t current_user;

//manejadores para tareas 
TaskHandle_t recv_handle;

int first_instance = 0;


//+++++++++++++++++++++ funciones 

char **pasrse_input(char *line);

esp_err_t update_cred(char *token_1, char *token_2, int op);

void setup_client(void);

void screen_cmd();


//++++++++++++++++++++++ tareas 

void task_cmd_uart(void *params);
//para WIFI al menos que haga una tarea especifica que espera que se actualice o se haga un update y realice el cierre de los sockets 

void task_update_wifi(void *parms);

void task_recv_proccess(void *params);

void app_main(void)
{
    flow_data_queue = xQueueCreate(10, sizeof(char *));
    tcp_data_flow = xQueueCreate(10, sizeof(format_request_t *));

    //inicamos grupo de eventos 

    g_EVENT_WIFI = xEventGroupCreate();
    g_tcp_event_group = xEventGroupCreate();
    g_user_def = xEventGroupCreate();

    //inicamos UART 
    uart_init();

    //iniciamoa la tarea 

    xTaskCreate(uart_task, "uart_task", 4098, NULL, 9, NULL);
    xTaskCreate(task_cmd_uart,"task_cmd_uart", 4098, NULL, 8, NULL);
    xTaskCreate(task_update_wifi,"task_update_wifi", 1024, NULL, 5, NULL);



    /**
     * iniciamos varibales globales 
     * 
     */
    //inciando en cero y nulos los valores para TCP 
    //el caso especula del sokcet, porque un 0 incia que el servidor cerro la conexion, poor lo que un numero menor que cero indica que nose pudo asingar el descriptor al socket 
    tcp_client.sockdf = -1;
    tcp_client.host_ip = NULL;
    tcp_client.host_port = 0;
    tcp_client.logged_in = 0;
    tcp_client.connected = 0;
    //lo mismo en wifi
    esp_wifi.connected =0;
    esp_wifi.esp_pswd=NULL;
    esp_wifi.esp_ssid = NULL;
    esp_wifi.ip = 0;

    current_user = 0;


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

    len = snprintf(msg, sizeof(msg), "credenucaes para TCP -> HOST_TPC_IP:<IP> HOST_TPC_PORT:<PORT>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    len = snprintf(msg, sizeof(msg), "credenucaes para UDP -> HOST_UDP_IP:<IP> HOST_UDP_IP:<PORT>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    len = snprintf(msg, sizeof(msg), "ingrear usuaria -> USER:<user>\r\n");
    uart_write_bytes(UART_MAIN,msg, len);
    memset(msg, 0, sizeof(msg));

    //lo primero que hara la primerita vez que se prenda cunado se inice todo el proceso sera introducir las credenciales WIFI 
    //por lo que lo que haremos es primero verificar si no hay credecniales, si hay credenciles el sistema intentara conectarse con esas credenciales al no poder si ese su caso
    //entonces pedira la actualizacion de las credencuales. 


    //primero debemos inicar esta munda 
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("MAIN", "WIFI_STA");
    wifi_init_sta();
    //inicamos 

    //al igual que el codigo inicar tenemos una funcion que se encrga de orquestar todas la inciaidlizaciones y todo el espapaye entre TCP, pero ahoira le agregamos UDP 
    setup_client();    

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
                    len = snprintf(msg, sizeof(msg), "no se puedieron actualizar las credenclaes\r\n");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(tokens);
                    continue;
                }

                //en otro caos si se asignanos y entonces procedemos a actcar el bit para actulaizar credeniclaes 

                xEventGroupSetBits(g_EVENT_WIFI, WIFI_UPDATE);
                xEventGroupSetBits(g_EVENT_WIFI, DELETE_TCP);
                //en este caso habria otro eventos que indica que se actualizo el wifi por lo que se deberia de reicniar 
                //la conexion entre el servidor y el esp. 

            }
            else if(strcmp(type,"HOST_TCP_IP") == 0 && tokens[1] != NULL){
                //ambos parametros debemos de estar inicalizados 
                char *host_ip = strchr(tokens[0], ':');
                char *host_port = strchr(tokens[1], ':');

                if(host_ip == NULL || host_port == NULL){
                    len = snprintf(msg, sizeof(msg), "se necesitan los 2 parametros, IP y puerto\r\n");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(receive);
                    free(tokens);
                    continue;
                }

                host_ip++;
                host_port++;
                //en este momneto el puerto aun sigue sindo una cadena que viene de UART, por lo que en la actualizacion debemos de converitr ese string en un numero entero 
                //de 16 bits 
                ret = update_cred(host_ip, host_port, HOST_TCP);

                if(ret != ESP_OK){
                    len = snprintf(msg, sizeof(msg), "no se puedieron actualizar las credenclaes\r\n");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(tokens);
                    //va a esperar a que se ingresen de nvo
                    continue;
                }


                //si no es asi entonces debemos inciar auqe las credeniclaes se actualizadon 
                xEventGroupSetBits(g_tcp_event_group, UPDATE_TCP);

            }

            else if(strcmp(type, "USER") == 0){
                char *user = strchr(tokens[0], ':');

                if(user == NULL){
                    len = snprintf(msg, sizeof(msg), "no se ingreso el usuario\r\n");
                    uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(receive);
                    free(tokens);
                    continue;
                }

                ret = update_cred(user, NULL, USER);

                if(ret != ESP_OK){
                    len = snprintf(msg, sizeof(msg), "intente de nuevo: \r\n");
                    uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
                    uart_write_bytes(UART_MAIN, msg, len);
                    uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                    free(receive);
                    free(tokens);
                    //se queda esperandoa  esperar el nuevo usuario
                    continue;
                    
                }

                //EN ESTE CASO SE ACTUALIZO EL USUARIO, puede tener un buen punto, 
                //al final aqui solo actualizamos el usurio mas no se cierra la conexion a internet, el socket le da igual si es un usuario o otro, el socket sigue siendo el mimso
                //por lo que tansolo ahora se manda con el nuevo usuario 

                xEventGroupSetBits(g_user_def, UPDATE_USER);


            }

            else if(strcmp(type, "YES") == 0){
                //en el caso qde reintentar relizar la conexion con las actuales credenciales 
                xEventGroupSetBits(g_tcp_event_group, RETRY_SERVER);
            }
            else if(strcmp(type, "NO") == 0){
                xEventGroupSetBits(g_tcp_event_group, NO_RETRY_SERVER);
            }

            else{
                len = snprintf(msg, sizeof(msg), "ERROR!\r\n Formato de CMD incorrecto");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            }


        }

    }
}



esp_err_t update_cred(char *token_1, char *token_2, int op){

    char msg[120];
    int len;
    switch(op){

        case SSID :{
            free(esp_wifi.esp_ssid);
            free(esp_wifi.esp_pswd);

            esp_wifi.esp_ssid = strdup(token_1);
            esp_wifi.esp_pswd= strdup(token_2);

            if(esp_wifi.esp_ssid == NULL && esp_wifi.esp_pswd == NULL){
                len = snprintf(msg, sizeof(msg), "Error al asignar memoria!\r\n");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                return ESP_FAIL;
            }
            return ESP_OK;


        }break;
        
        case HOST_TCP:{
            free(tcp_client.host_ip);
            tcp_client.host_port =0;

            tcp_client.host_ip = strdup(token_1);
            //ahora convierto ese string es un valor de 16 bits 
            uint16_t port = (uint16_t)atoi(token_2);
            tcp_client.host_port = port;


            if(tcp_client.host_ip == NULL || tcp_client.host_port == 0){

                len = snprintf(msg, sizeof(msg), "Error al asignar memoria!\r\n");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                return ESP_FAIL;
            }


            return ESP_OK;
        }break;

        // case HOST_UDP:{


        // }break;

        case USER:{
            current_user = 0;

            uint32_t current_user = (uint32_t)aoit(token_1); //asignamos el dato 

            if(current_user != 0){
                len = snprintf(msg, sizeof(msg), "usuario declarado: %u\r\n", current_user);
                uart_write_bytes(UART_MAIN, UART_GREEN, sizeof(UART_GREEN));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                return ESP_OK;
            }
            //fallo al establecer el usuerio 
            return ESP_FAIL;
        }break;


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



void task_recv_proccess(void *params){

    format_request_t *rx_request; 

    char msg[100];
    int len;

    while(1){

        if(xQueueReceive(tcp_data_flow, &rx_request, portMAX_DELAY)){


            //verificamos si se inciio sesion, 

            if(rx_request->header == ACK && rx_request->len == 0xFF){
                //este es un NACK 
                len = snprintf(msg, sizeof(msg), "no se pudo relizar login al servidor\r\n");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                xEventGroupSetBits(g_tcp_event_group, LOGIN_FAIL);
            }
            else if(rx_request->header == ACK && rx_request->len < 0xFF){
                //es un ACK 

            }

            else{
                //se pidop un recurso, header es 0xCAFE
                //primero debemos de verificar que se haya inciado sesion, si no, el servidor no puede recibir nada 

            }

            


        }


    }

}


void setup_client(void){

    //ahora debemos debemos de ingresar el servidor TCP e IP a la cual nos debemos de conectar 
    //entonces en este paso se intentara relizar hasta que conecte, 
    
    //perimo lo que hace es establecer la conexion 
    esp_err_t ret;
    char msg[100];
    int len;
    EventBits_t bits;

    static int retry_login =0; 


    //despues debemos de esperar a que se ingrese el usuarios --> antes de inciar 
    //lo ponemos aca arriba, porque si lo podmeos dentro del while, entonces el sistema estara esperando a que se actualice, esta actualizacion no es relevancia, solo en esta
    //parte porque es necesario definir a un usuario antes de enivar, pero depues se actualiza, pero no se cierra el sokcet ni se desactivia el WIFI, porque no afecta a esas partes 
    while(current_user == 0){
        //este se quedara en este ciclo  hasta que se acutlaice que hay un usuario ya establecido 
        bits = xEventGroupWaitBits(g_user_def, UPDATE_USER, pdTRUE, pdTRUE, portMAX_DELAY);

        if(bits & UPDATE_USER){
            break; //ya se deinfiio un uusaior, a la proxima si se raliza una llamada recursiva no entrara aqui porque ya se tendra en varibale global esto. 
        }
    }



    pseudo_recursion:
    while(1){

        ret = tcp_cliente_init(); //esto es importante que sea nomas 1 vez porque es el mismo socket que se estara usando a lo largo de la operacion con TCP, y pues lo mismo para UDP
            
        if(ret != ESP_OK){
            //em caso que no se pueda conectar entonces estara la opcion de volver a intentar reliazar la conexion con las mismas credenciales 
            len = snprintf(msg, sizeof(msg), "ERROR!\r\nquiere intentar conetsar con las mismas credeniclaes\r\n");
            uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));

            len = snprintf(msg, sizeof(msg), "YES - NO \r\n");
            uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            // xEventGroupSetBits(g_tcp_event_group, RETRY_SERVER);
            goto wait;
        }

        if(first_instance != 1){
            
            //conn el objetivo de no volver a crear la instancia de la tarea y se esten creando y creando 
            xTaskCreate(task_recv_tcp, "task_recv_tcp", 2096, NULL, 8,&recv_handle);
            xTaskCreate(task_recv_proccess, "task_recv_proccess", 4098, NULL, 8, NULL);
            first_instance =1;
        }
        
        //ahora lo que se intea hacer es relaizar el login al servidor 
        login:
        if(tcp_client.logged_in != 1){
            //que intente conectarse al menos 5 veces al servidor. si en esas 5 veces no se puede conectar entonces pasamos 

            //no se ha inciado 
            send_info.type = OP_LOGIN; //quiero hace login al servidor 
            send_info.format_request.user = current_user;
            ret = send_massage();

            //mas bien esperamos 2 bits, que inican que pudo inciar y que no pudo inciar, pero este se vera en la constantacion 
            //porque en este momento mi ESP esta conectado al servidor, esta tratando de estbalecer una conexion 
        
            //esperamos 
            bits = xEventGroupWaitBits(g_tcp_event_group, LOGIN_FAIL| LOGIN_OK, pdTRUE, pdFALSE, portMAX_DELAY);

            if(bits & LOGIN_FAIL){
                
                if(retry_login < 5){
                    //quiere decir que aun no pasan las 5 veces 
                    goto login;
                }

                //en otro caso ya se completaron las 5 veces 
                retry_login = 0;

                //entonces mencionamos que no pudo inicar inciar sesion por lo que puede nuevamente intentar inicar sesion o puede actualzar credenciales,
                //credeicnlaes que puede actulaiar 
                //WIFI - TCP - <UDP?> - user
                //para WIFI -TCP se deberia de cerrar conexiones, en caso de actualair WIFI se cierr el socket, en caso de solo el TCP no cerramos WIFI 

                len = snprintf(msg, sizeof(msg), "ERROR!\r\nquiere intentar conetsar con las mismas credeniclaes\r\n");
                uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));

                len = snprintf(msg, sizeof(msg), "YES - NO \r\n");
                uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            }
            else{
                //si se pudo conetar 
                //entonces aqui creamos

                tcp_client.logged_in =1;
                //inidcamos que ya hizo login 

                //cremaos tareas que queremos para enivar como el keeep, 

            }
        }


        wait:
        //aui solo se espera actualizacion hacerca de TCP
        EventBits_t bits = xEventGroupWaitBits(g_tcp_event_group, TCP_DISCONNECTED | UPDATE_TCP | RETRY_SERVER | NO_RETRY_SERVER, pdTRUE, pdFALSE, portMAX_DELAY);
        if(bits & RETRY_SERVER){
            //aqui relaizamos una llamada recursiva 
            //en este caso esas tareas aun no han sido creadas 
            continue;
        }
        else if( bits & NO_RETRY_SERVER){
            // si no, entonces cerramos todo lo relaciado con socket, y reinciamos 
            if(tcp_client.sockdf > 0){
                close(tcp_client.sockdf);
            }
            tcp_client.connected = 0;
            tcp_client.host_ip = NULL;
            tcp_client.host_port =0;
            tcp_client.logged_in = 0;
            //entonces en este momento lo espera es que se actualucen las credenuclaes de TCP 
            len = snprintf(msg, sizeof(msg), "esperando nuevas credenuclaes\r\n");
            uart_write_bytes(UART_MAIN, UART_CYAN, sizeof(UART_CYAN));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            goto wait;
        }
        else if(bits & UPDATE_TCP){
            //preimo verificamos si tenemos tareas de TCP como recv y keep alive activos

            if(first_instance != 0){
                //si es 1 entonces quiere decir que anteiromente ya se estaba trabajdno por lo que las cerramos 
                vTaskDelete(recv_handle);
                first_instance = 0; //lo reinciamos 
            }
            //volvemos a relziar el proceso para relziar la conexion con el servidor 
            continue;

        }

        else if(bits & TCP_DISCONNECTED){

        }
    }



}



void task_update_wifi(void *parms){


    while(1){

        EventBits_t bits = xEventGroupWaitBits(g_EVENT_WIFI, DELETE_TCP, pdFALSE, pdTRUE, portMAX_DELAY);

        if(tcp_client.connected !=0){

            //al actualizar la red WIFI, el socket debe ser cerrado 

            close(tcp_client.sockdf);
            tcp_client.connected =0;
            tcp_client.logged_in = 0;
            tcp_client.sockdf = 1;
            tcp_client.host_ip = NULL;
            tcp_client.host_port =0;
        }

        //este tiene que hacer el proceso de volver a conectar 
        wifi_init_sta();

        //volvemos a llamar para relizar el proceso de nuevo  
        if(first_instance != 0){
            vTaskDelete(recv_handle);
            first_instance =0;
        }
        setup_client();

    }
}