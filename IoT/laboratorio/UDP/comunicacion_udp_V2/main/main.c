#include <stdio.h>

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
#include<global.h>






//vairbales globales
static const char *TAG = "MAIN";

int ENT_NETWORK;

//colas 
QueueHandle_t uart_event;

//estrucutra 
task_uart_port_t global_uart;

//cola que manerjara el flujo de datos de UART 
QueueHandle_t flow_data_queue;


//necesitaremos otra cola que este encargada de notifiar  que ya hay nuevas credenciales para que este vuelva a intentar.
QueueHandle_t wifi_credential_queue;


//crendiales para WIFI
char *ESP_SSID_WIFI;
char *ESP_PSWD_WIFI;

/**
 * necesitare una funcion que se ecncargue de capturar estos tipos de datos que ocurran, por ejemplo que no se pudo conectar a la red 
 * por lo que pedira las nuevas credecniales y si es una red empresarial entonces la cuenta y psdw de la cuenta < para UABC> 
 * 
 * en el UDP si recibe un btnazo por ejemplo cambiar de IP y puerto si es necesario, por lo que utilizaremos un enum tal vez < no creo que mejor un grupo de eventos>  
 * 
*/

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
/**
 * para dar salto de linea  
 * 
 * 
*/
static inline void uart_jump(void);

//tarea encargada de recibir el comando por UART 
void task_cmd_uart(void *params);


void app_main(void)
{


    flow_data_queue = xQueueCreate(10, sizeof(char*));
    //este solo enviata 1 o 0 indicando una bandera activa 
    wifi_credential_queue = xQueueCreate(2, sizeof(uint8_t));
    global_uart.NUM_PORT = UART_MAIN;
    //inicamos UART 
    uart_init(UART_MAIN,115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    
    //aqui esta el demon que esta ecuchando a UART 
    xTaskCreate(task_uart, "task_uart", 4096,&global_uart, 9, NULL);

    xTaskCreate(task_cmd_uart, "task_cmd_uart", 4096, NULL, 8, NULL);


    //parte de wifi, inicamos las credenciales para wifi con las de mi chante, para poder modificarlas si es necesatio 
    //inicamos valores pode fectos 
    char default_ssid[]="INFINITUMF4AF\0";
    char default_psdw[]="nFukH34MPW\0";




    esp_err_t ret =  new_setup_wifi(default_ssid, default_psdw);

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
    ESP_LOGI(TAG, "SETUP WIFI ENTERPRISE -> ENT_SSID:<>");
    //okay, este es la parte de WIFI, por lo primero debemos de poner que va a inicar la conexion 
    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    

    
    


}


void task_cmd_uart(void *params){



    char* cmd_receive;

    while(1){


        if(xQueueReceive(flow_data_queue, &cmd_receive, portMAX_DELAY)){
            //en este punto se recibio por ejemplo 

            //en este punto tokens contendra los tokens introducidos 
            //es decir los datos que se han serpado por un espacio, mas no trae los datos necesarios
            //para poder actualizar el WIFI 
            char **tokens = pasrse_input(cmd_receive);

            if(tokens == NULL || tokens[0] == NULL){
                free(cmd_receive);
                continue;
            }
            
            /**
             * por lo que ahora tengo es que deberia de tener un orden los comandos, para que sea mas facil identificar a cual se hace refercnia 
             * 
             * por ejemplo para WIFI nomral este debe de inicar con SSID:, 
             * 
             * para WIFI enterprese debe de icnar con ENT_SSID:
             * 
             * para socket si se cambia de IP debe de inicar con IP:
             * 
             * 
             * para esto 
             * 
            */
            //estraemos 
            // char *cmd_case = strdup(tokens[0]);
            
            // cmd_case = strtok(cmd_case, ":");


            // uart_write_bytes(UART_MAIN, cmd_case, strlen(cmd_case));

            // 2. ¿Cómo quedaron los tokens?
            // for(int i = 0; tokens[i] != NULL; i++){
            //     uart_write_bytes(UART_MAIN, "\r\n", 2);
            //     uart_write_bytes(UART_MAIN, tokens[i], strlen(tokens[i]));
            //     // ESP_LOGI("CMD", "token[%d] = [%s]", i, tokens[i]);
            // }

            char *cmd_case = strdup(tokens[0]);
            char  *tmp = strtok(cmd_case, ":");
            
            if(strcmp(tmp,"SSID") ==0 ){


                //si entra aqui es que no es una red de empresa 
                ENT_NETWORK = FALSE; //false
                //en este caso ocurrio un error y es necesario introducir otra red, pero el sismte intento conectarse a la red por defecto 

                //por lo que ahora necesito es serparar la parte que me importa del encabezado del comando 
                char *ssid = strchr(tokens[0], ':');
                char *pwsd = strchr(tokens[1], ':');;
                //brincamos ":"
                ssid++;
                pwsd++;
                //mando las nuevas credenciales a la funcion que se encargara de actualizar las credenciales y volver a correr la coenxion 
                esp_err_t ret =  new_setup_wifi(ssid, pwsd);
                
                //preparamos el envio de una senial 1 que indica que ya ha
                if(ret !=ESP_FAIL){
                    uint8_t signal = 1; 
                    xQueueSend(wifi_credential_queue, &signal, portMAX_DELAY);
                }
                else{
                    uart_write_bytes(UART_MAIN,"\r\n", 2);
                    const char *mssg = "no se pudieron actualizar las credenciales\0";
                    uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
                }   



            }
            // else if(cmd_case == "ENT_SSID"){

            // }else if(cmd_case == "IP"){

            // }

            




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


static inline void uart_jump(void) {
    uart_write_bytes(UART_MAIN, "\r\n", 2);
}

