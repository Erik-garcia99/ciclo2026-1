/**
 * debemos de modificar, agregar algo por si en algun punto del proyecto el usaurio quiere cambiar de red o de host una vez establecidos para que este proceso se vuelva a ejecutar  
 * 
 * el proceso de poder introducir una IP una vez ya captiurado, es algo un poco mas fascil puesto que ya contamos con una estrucutra que nos indica que si ya estamos coenctados
 * podemos apoyarnos con esto, cosa qaue no tenemos con WIFI 
 * 
 * para poder cambiar la red una vez que ya se encuentre conectado seria algo muy parecido, alguna estrucutra algo que nos diera esa ifnromacion porque el grupo de eventos
 * solo nos dincia cosas que hya pasaron pero no nos dice mas. mas informacion y estos se limpian una vez que se utilizan 
 * 
 * 
*/


/**
 * -- pendientes :
 * 
 * - aun no queda el como establecer las credenciuales TCP de manera que se ejcute hasta que se conecte o el uusario eliga que no 
 * 
 * -- falta todo el tema de la comunicacion TCP, tareas de recv y send 
 * 
 * 
 * 
*/

/**
 * mpdificacion:
 * --cambiaremos el grupo de eventos por una cola, la razon es paa poder tener el control sobre si es que quiere intentar de nuevo conectarse o salir, con el grupo de eventos 
 *      estare limitado a la condicion en donde si se desea reestablecer la conexion, con una cola podre mandar 1 - indicando que si quiere o 0 - indicando que no quiere y realizar un brak y salir 
 * 
 * 
 * 
 */



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
#include<modules/TCP/tcp_lib.h>
#include<global.h>


//vairbales globales
static const char *TAG = "MAIN";

//este valor representa <break for update wifi>
//lo usare para indicar que se actualizaron credenciuales, se usara para el ciclo que controla la conexion de TCP con el servidor. este no tiene otra funcionalidad mas que 
//avisar de la actualziacion para salir del ciclo y poder realizar la conexion wifi
int bfuw=0;


//colas 
QueueHandle_t uart_event;

//estrucutra 
task_uart_port_t global_uart;
//estucuturua para red 
esp_wifi_t esp_wifi;
//estrucutra para parametros de la conexion tcp
tcp_client_t tcp_client;


//cola que manerjara el flujo de datos de UART 
QueueHandle_t flow_data_queue;
//cola para manejar que si se intenta o no la reeconexion con els servidor 
QueueHandle_t q_tcp_client_queue;


//grupos de eventos. 

EventGroupHandle_t g_tcp_event_group;
EventGroupHandle_t s_wifi_event_group;


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


void app_main(void)
{
    //creamos el grupo de eventos para WIFI 
    s_wifi_event_group = xEventGroupCreate();
    g_tcp_event_group= xEventGroupCreate();


    
    flow_data_queue = xQueueCreate(10, sizeof(char*));
    
    //solo maneraja 0 o 1, por lo que esto es suficiente 
    q_tcp_client_queue = xQueueCreate(2,sizeof(uint16_t));

    global_uart.NUM_PORT = UART_MAIN;
    //inicamos UART 
    uart_init(UART_MAIN,115200, UART_DATA_8_BITS, UART_PARITY_DISABLE, UART_STOP_BITS_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    
    //aqui esta el demon que esta ecuchando a UART 
    xTaskCreate(task_uart, "task_uart", 4096,&global_uart, 9, NULL);

    xTaskCreate(task_cmd_uart, "task_cmd_uart", 4096, NULL, 8, NULL);


    //configuracion de WIFI

    esp_err_t ret;
    //esta funcion va ser modificada
    //inicamos la estrucura para la conexion en la red 


    //vamos a inicar con parametros por defecto 
    char *ssid_default ="INFINITUMF4AF\0";
    char pswd_default = "nFukH34MPW\0";  


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
    ESP_LOGI(TAG, "SETUP TCP SERVER -> HOST_IP:<host_ip> PORT:<# port>");
    //okay, este es la parte de WIFI, por lo primero debemos de poner que va a inicar la conexion 
    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    //configuracion de IP - puerto -> creo que todo este desmadre debe de ir en otra funcin 

    tcp_client_t tcp_client = {
    .sock      = -1,
    .connected = false,
    .logged_in = false,
    };

    ESP_LOGI(TAG, "TCP -> se estabelceran credencuales por defecto");
      
    /**
     * no, esta bien, igual lo haremos medinate un do-while, pero de primero, para establecer las credenciales, la IP y el puerto se hara por fuera, porque al final 
     * la funcion que predente actulizar las credenciales solo hace eso, no verifica la conexion de realizo eso lo hacemos en la liberia de tcp cunado se crea el socket 
     * 
     * -- vamos por partes primero seria establecer las credenciales por defecto 
     * 
    */

    ret = update_setup_cred(DEFAULT_HOST, DEFAULT_PORT, NULL, "HOST_IP");

    if(ret != ESP_FAIL){
        ESP_LOGI(TAG, "credenciales TCP por defecto establecidas correctamente");
    }
    else{
        ESP_LOGE(TAG,"error al establecer las credenciales TCP por defecto.");
    }

    //el cliente tcp va aintentar 5 veces conectarse, cada una con un lapso de 30 s, si en esas 5 veces no logra ralizar la conexion entonces le inidicara al usuario si quiere volver
    //a intentar otras 5 veces, si elige que no, entonces quiere decir que va a modificar algo, por lo que ahora la tarea primero que estara activa sera la que recibe por uart. 
    //entonces la funcion por su parte intenttara realziarlo 5 veces y de deste lado intentaremos esperar si el usuario quiere 

    //se me ocurrio la idea de realizar este proceso medinate la recursividad, pero la cosa de la recursividad en un sistema embebido esta medio cabron por la cantidad de memoria 
    //que esta necesita. 

    //okay, este ciclo va a sguir idealmente hasta que se logre establecer la conexion, pero cuales son las posibles posibilidades que puede optar el usario
    /**
     * -- acepta en seguir intentando y se logra la conexion, pero para que esto pueda ser, al final va a ingresar por UART que desea continuar.
     *      por lo que deberiamos de optar por un grupo de eventos que indique que seguira 
     * 
     * --para que el usuario pueda cambiar de red deberia de salir de este ciclo e indicar que no quiere seguir, eso seria lo mas facil o correcto, interrumpir a mitad creo
     *      que seria algo dificil de hacer y muchas cosas. 
     * 
     * 
     * --- IMPORTANTE - como saber como seguir con el ciclo, 
     *          CONDICIONES --> condiciones seria que el ciclo seguira siempre y cuando no se haya conectado ( tcp_client.connected = 0), 
     *                          el bit para contignuar sea 1, (RETY_SERVER.BIT0 = 1) 
     *           estos 2 deberian de cumplirse para poder continuar --> debe de haber una condicion dentro de la tarea que trata la informacion por UART que indique va a contingar y 
     *           active el bit. 
     * 
     */

    uint16_t bit;

    do{
        
        esp_err_t ret  = tcp_cliente_init();

        if(ret == ESP_OK){
            //entonces se logro establecer la coenxion 
            break;
        }
        else{

            char *mess_error ="MAIN-TCP : no se pudo establecer la conexion, posiblemente el servidor no esta arriba, quiere volver a intentar?\n";
            uart_write_bytes(global_uart.NUM_PORT, mess_error, strlen(mess_error));
            //ahora estara esperando a recibir la entrada por UART 
            char *option = "MAIN-TCP : [YES] or [NO]\n"; // -> deberia de tratar lo que se envia, en este caso podemos poner un lowercase para como se que se introdujo 
            //ahpor sera con minusculas --> era una buena idea pero mejor no, debe de ingresar una de esas 2 para poder continuar 

            //grupo de eventos 
            if(xQueueReceive(q_tcp_client_queue, &bit, portMAX_DELAY)){  
                if(!(bit & 1)){
                    //esta doncidion indica que no quiere seguir 
                    break;
                }
            }
        }
        
    }while( tcp_client.connected!=1 && (bit&1));    
    //si no se ha conectado y se ha elegido volver a intentar 
    


}


void task_cmd_uart(void *params){

    char* cmd_receive;
    esp_err_t ret;

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
            char *cmd_case = strdup(tokens[0]);
            char  *tmp = strtok(cmd_case, ":");
            
            if(strcmp(tmp,"SSID") ==0 ){


                //en este caso ocurrio un error y es necesario introducir otra red, pero el sismte intento conectarse a la red por defecto 

                //por lo que ahora necesito es serparar la parte que me importa del encabezado del comando 
                char *ssid = strchr(tokens[0], ':');
                char *pwsd = strchr(tokens[1], ':');
                //brincamos ":"
                ssid++;
                pwsd++;
                //mando las nuevas credenciales a la funcion que se encargara de actualizar las credenciales y volver a correr la coenxion 
                //en este punto ya se que es wifi normal por lo que mandaremos los datos 

                ret = update_setup_cred(ssid, pwsd, NULL, "SSID");

                
                //preparamos el envio de una senial 1 que indica que ya ha
                if(ret !=ESP_FAIL){

                    //ahora ya no vamos a mandar por la cola, ahora vamos a mandar vamos a activar un bit que indicara que ya se activo el bit 
                    // uint8_t signal = 1; 
                    // xQueueSend(wifi_credential_queue, &signal, portMAX_DELAY);
                    
                    //activamos el bit
                    xEventGroupSetBits(s_wifi_event_group, WIFI_UPDATE);
                    bfuw=1;
                }
                else{
                    uart_write_bytes(UART_MAIN,"\r\n", 2);
                    const char *mssg = "no se pudieron actualizar las credenciales\0";
                    uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
                }   



            }
            else if(strcmp("HOST_IP", tmp) == 0){

                char *host_ip = strchr(tokens[0], ':');
                char *port = strchr(tokens[1], ':');
                host_ip++;
                port++;

                //pues el primero sera algo muy similar, una cola para enviar datos o una varibale global para que esto no sea tan pesado o grupo de bits y talvez podramos
                //cambiar el de wifi porque esa colsa creo que es mucho para lo unico que hace 

                ret = update_setup_cred(host_ip, port, NULL, "HOST_IP");

                //indicamos si hubo un error al actualizar, este error, solo va a apsar por si algun caso no se pudeiron actualizar, lo mas probable seria porbelams con memoria, 

                if(ret != ESP_FAIL){
                    //se pudo actualizar y igual por un grupo de eventos indicamos a la funcion que va a poder intentar
                    xEventGroupSetBits(g_tcp_event_group, READY_CRED); //

                }
                else{
                    ESP_LOGE(TAG, "no se pudo actualizar credenciales de conexion al servidor TCP");
                }



            }
            //ahora es cuando se procesa los datos ingresados por el usuario "YES" o "NO" indicand que si quiere volver a intentear a establecer una conexion TCP con el servidor
            //con la esperanza que ya este arriab el servidor. 
            else if(tmp[1]==NULL){
                // pude que funcione no estoy muy seguro 
                //tokens nos va a regresar al menos 2 selemtnos en, uno contendra un valor y otro contendra NULL indicando el final del arreglo 

                //cunado ingresara a esta condiconal
                /**
                 * - para la iteracion de la conexion de TCP "YES" o "NO"
                 * -o cunado se modifique la matricula con la que se loggea 
                */

                if(strcmp("YES", tmp[0]) == 0 ){
                    uint16_t bit = 1;
                    //en este caso el usuario quiere volver a intentar realizar la operacion de conectar 
                    xQueueSend(q_tcp_client_queue,bit,portMAX_DELAY); //esperar si la cola esta llena
                }
                else if(strcmp("YES", tmp[0]) == 0){
                    uint16_t bit = 0;
                    xQueueSend(q_tcp_client_queue, bit, portMAX_DELAY);
                }
                //flata cunado se cambia de matricula 


            }

            
            //con una red para UABC 
            //con un nuevo usuario


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
            esp_wifi.user_name='/0';
            return ESP_OK;
        } else {
            uart_write_bytes(UART_MAIN,"\r\n", 2);
            const char *mssg = "MAIN - no hay memoria para las credenicales\0";
            uart_write_bytes(UART_MAIN,mssg, strlen(mssg));
            return ESP_FAIL;
        }

    }
    //ahora cunado sea cambiar la IP
    else if(strcmp(identificator, "HOST_IP") == 0){

        tcp_client.host_ip = realloc(tcp_client.host_ip, strlen(key)+1);
        tcp_client.host_port = realloc(tcp_client.host_port, strlen(anchor)+1);
        //verificar que se pudieron asginar espacio en memoria 
        if(tcp_client.host_ip != NULL && tcp_client.host_port){
            strcpy(tcp_client.host_ip, key);
            strcpy(tcp_client.host_port, anchor);
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



}