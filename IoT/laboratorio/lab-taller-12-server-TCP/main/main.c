/**
 * en este proyecto sera un servidor TCP por ESP32. 
 * 
 * puede que ene l cliente debamos de establecer la direccion IP del server de repuesto, o no, introducimos uno de la direccion ip que nos da en la casa y despues podramos actualizar aqui. 
 * 
 * 
 * --> necesitamos traer todo lo posible para poder actualizar wifi 
 * 
 * 
*/






#include <stdio.h>

//drivers
#include<driver/uart.h>



//librerias propias 
#include"modules/WIFI/wifi_lib.h"
#include"modules/UART/uart_lib.h"




//vairbales globales
static const char *TAG = "MAIN";



//++++++++++++++++ colas 
//cola para los eventos de UART
QueueHandle_t uart_event;
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

//funciones
/**
 * el server solo va a actualizar wifi, porque en si es lo unico que necesita actualizar tal vez el puerto. 
 * 
 * -ya veremos si metemos para actualizar el pruerto pero creo que si 
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


//tarea encargada de recibir el comando por UART 
void task_cmd_uart(void *params);



void app_main(void)
{

    //creamos el grupo de eventos para WIFI 
    s_wifi_event_group = xEventGroupCreate();

    
    flow_data_queue = xQueueCreate(10, sizeof(char*));

    
    global_uart.NUM_PORT = UART_MAIN;
    //inicamos UART 
    uart_init(UART_MAIN,115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    
    //aqui esta el demon que esta ecuchando a UART 
    xTaskCreate(task_uart, "task_uart", 4096,&global_uart, 9, NULL);

    xTaskCreate(task_cmd_uart, "task_cmd_uart", 4096, NULL, 8, NULL);


    
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
    ESP_LOGI(TAG, "SETUP WIFI -> SSID:<nombre_ssid> PSWD:<pasword_red>");
    ESP_LOGI(TAG, "SETUP TCP SERVER -> PORT:<pureto>");
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


/**
 * el ESP32 TCP server lo que podra modificar seria 
 * 
 * >> WIFI
 * 
 * >> puede ser el puerto con el que se pueden conectar clientes, mas de alla pues no tendria porque ser modificado. 
 * 
 * >> para la modificacion del puerto no sera HOST_IP, ahora solo sera (HOST_PORT:<num. puerto>)  
 * 
 * 
 * 
*/

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
            else if(strcmp(tipo,"HOST_PORT") == 0 && tokens[1] ==NULL){
                //comandos HOST_PORT:<puerto>

                //el procesoe s muy similar 
                char *host_port = strchr(tokens[0], ':');
                
                
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


            //ahi otros 2 datos que el servidor puede enviar que son el ACK y en NACK 
            /**
             * en donde estos solo reperesentan si es lo que envio fue recibido correctamnete o no. 
             * 
             */

            
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
