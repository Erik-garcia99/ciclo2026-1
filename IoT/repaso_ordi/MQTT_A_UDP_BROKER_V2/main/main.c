/**
 * proximos pasos 
 * 
 * --> construir la estrucutra del frame binario --> creo que ya esta 
 * 
 * --> constuir las tarea que reiceb 
 * 
 * --> consturir la funcion que envia 
 * 
 * --> construir la logica de alamcenar y orqueetar los topicos 
 * 
 * --> funcion que funciona para enviar las publicaciones (tendra cola para no perder nignua publicacion )
 * 
 * --> solo 16 topicos posibles 
 * 
 * 
 * --> funcion sub tiene el parametro ticpic y el de pub tiene topic + msg --> esto para el cliente. 
 * 
 * 
 * 
*/



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

#include"lwip/sockets.h"
#include"lwip/netdb.h"

#include<global.h>
#include<modules/WIFI/wifi_lib.h>
#include<modules/UDP/udp_lib.h>
#include<modules/UART/uart_lib.h>



//++++++++++++++++++colas 

QueueHandle_t uart_queue;
QueueHandle_t flow_data_queue;
QueueHandle_t udp_data_flow; 
QueueHandle_t send_msg_queue;


//+++++++++++++++++grupos de eventos 
EventGroupHandle_t g_EVENT_WIFI;
EventGroupHandle_t g_udp_event_group;
EventGroupHandle_t g_user_def;


//++++++++++++++++++ estrucutras 

format_request_t format_request;
// send_info_t send_info;
frame_recv_t frame_recv;


esp_wifi_t esp_wifi;
udp_server_t udp_server;

//lo dejamos por si acaso 




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


//este arreglo es el que va a definir cuales son los topicos actuales, porque los topicos pueden llegar desordenados. 
//si un publicador publica en un nuevo topico se registra 
uint8_t topico[MAX_TOPIC]; 
int count_tipic;


//+++++++++++++++++++++ funciones 

char **pasrse_input(char *line);

esp_err_t update_cred(char *token_1, char *token_2, int op);

void setup_server_udp(void);

void screen_cmd();

//las dejamos igual

//funciones para listas enlazadas 


//el usuario tiene una cola, dentro de su cuerpo en el cual vamos agregando los mensajes, la forma en como vamos a liberar y que el sistem 
//va a comprender que se recibio el usuario correctamnte es con el ACK 

//entonces el usuario conestara el ACK [ACK][NUM][LEN][USER][TOPIC][0]
//donde el NUM es el enumerate, regresara el enumerete que recbio paa nosotros desde aca buscarlo y lberearlo 
//por que es necesario el usuario porque el sistem puede tener un usuario en disfrentes disposiivos, bueno, al final se identifica por la IP 
//creo que el usuario sale sobrando no es necesario

/**
 * @brief se encarga de crear el nuevo nodo ante un nuevo usuario 
 * 
 * @param user_name - MATRICULA DEL USUARIO 
 * @param ip : la ip del disposito en donde llego la peticion de conexion - publicacion - suscripcion 
 * 
 * @return apuntdor -> regresa el apuntador del head de la lista de tipo node_user_t
*/
node_user_t *create_user_node(char *ip, uint16_t port); //creo que no hay de otra mas que crear una funcion de cada estrucuta 


/*
 * @brief funcion que crea el nodo ante el nuevo suscritos de un topico -> se lanza cunado el topico no ha sido creado 
 * 
 * -- se creara el nodo si un usuario publica o se quire suscribir al topico y si este no existe crea el nodo 
 * -- si un publicador publica en un tipoco que no existe se crea este nuevo nodo mas aun no se ha reistrado nadie que quiera suscribirse 
 * -- cunado algueins e quiere susbrir a este nuvo topico entonces se crea o se agrega recorriendo la lista 
 * 
 * @param user_sub - apuntador al nodo donde se encuentra el usurio que se registro ante este nuevo topico  
 * 
 * @return node_subs_t* - regresa un apuntador de la head de la lista s 
 */
node_subs_t *create_subs_node(uint8_t num_topic);


//funcion de creacion del nodo para la cola de los mensajes pendeintes del usuario, de cada usuario 
pending_msg_t *create_node_msg_pendentig(op_type_t type_msg, format_request_t *msg);




// funciones de busqueda 
node_user_t *find_node_user(node_user_t *linked_list, char *ip);


node_subs_t *find_node_subs_topic(node_subs_t *topic_linked_list, uint8_t topic);



/***
 * @brief  funcion que busca un nodo en particular 
 * 
 * @param head la cabeza de la cola 
 * @param topic indica que buscamos un topico 
 * @param pckid indica el enumerate del frame enviad 
 * @param flag una bandea que me indica si el nodo que esoyt buscando es el mas reciente agregado por lo que ira hasta el final,
 * para este se envia la macro EOF  
 * 
 * 
 * @return NULL en caso de no encoentrar nada 
 * @return regresa un apuntador al nodo que se esta buscando 
 */
pending_msg_t *find_node_msg_snd(pending_msg_t *head,uint8_t topic,int pckid, uint8_t flag);

//metodos de agregar nuevos nodos 

/**
 * @brief agrega con push un nuevo nodo a la lista enlazada 
 * 
 * 
*/

node_subs_t *push_node_subs_topic(node_subs_t *heap, uint8_t topic);


/*
 * funcion que se encarga de crear el nuevo nodo, busca si el usuario y la ip ya estan registrados en un mismo nodo
 * si no es asi crea el nuevo
 */ 

node_user_t *push_node_user(node_user_t *head,char *ip,uint16_t port);



//metodos de agregar un nuevo nodo a la cola enlazada 

pending_msg_t *add_new_node_msg(pending_msg_t *head,op_type_t type_msg, format_request_t *new_frame);


//++++++++++++++++++++++ tareas 

void task_cmd_uart(void *params);
//para WIFI al menos que haga una tarea especifica que espera que se actualice o se haga un update y realice el cierre de los sockets 

void task_update_wifi(void *parms);

void task_recv_proccess(void *params);



void app_main(void)
{

    flow_data_queue = xQueueCreate(10, sizeof(char *));
    udp_data_flow = xQueueCreate(10, sizeof(format_request_t *));
    send_msg_queue = xQueueCreate(10, sizeof(node_msg_pub_t*));
    //inicamos grupo de eventos 

    g_EVENT_WIFI = xEventGroupCreate();
    g_udp_event_group = xEventGroupCreate();
    g_user_def = xEventGroupCreate();

    //inicamos UART 
    uart_init();

    //iniciamoa la tarea 

    xTaskCreate(uart_task, "uart_task", 4098, NULL, 9, NULL);
    xTaskCreate(task_cmd_uart,"task_cmd_uart", 4098, NULL, 8, NULL);
    xTaskCreate(task_update_wifi,"task_update_wifi", 1024, NULL, 5, NULL);

    //inicar variables 
    udp_server.sockdf =-1;
    udp_server.server_port = 0;
    udp_server.server_ip = NULL;
    udp_server.logged_in = 0;

    //lo mismo en wifi
    esp_wifi.connected =0;
    esp_wifi.esp_pswd=NULL;
    esp_wifi.esp_ssid = NULL;
    esp_wifi.ip = 0;

    current_user = 0;
    //iniciamos los elemntos del arreglo en 0, en donde se guadaran los topicos 
    memset(topico, 0, sizeof(topico));
    int count_tipic = 0;


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

    setup_server_udp();


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


void task_recv_proccess(void *params){


    frame_recv_t *rx_frame;

    char msg[100];
    int len;

    // //liysta enlazada que represeta a los usuarios 
    node_user_t *linked_list_user = NULL;

    // //lista enlazada que represeta a los usuarios registrados en los distintos topicos, lo que identifica es el topico 
    node_subs_t *linked_list_topic_subs = NULL;

    // node_topic_t *linked_list_user_mqtt = NULL;

    while(1){

        if(xQueueReceive(udp_data_flow, &rx_frame, portMAX_DELAY)){

            //primero verifiquemos si es que nos envio un ACK o un NACK 

            if(rx_frame->format_request.header == CONNACK && rx_frame->format_request.len == EOF){
                //quiere decir que llego un NACK 
                //entonces re pedira que vuelva a enviar el frame una vez mas. por si se perdieron los datos durante la trasmicion de datos 
                
            }

            else if(rx_frame->format_request.header == CONNACK && rx_frame->format_request.len < EOF){
               
                
            }

            else if(rx_frame->format_request.header == CONNECT){
                // aquie no hya control sobre que se eniva o que onda, porque por ahora el usuario lo unico que quiere es esabece de handshanke para establecer una conexion con el broker y que pueda recibir daos. 

                //esto funciona por si un uusaior externo intenta publicar o suscibirse anes de establecer la conexion 
                //el uusario intenta conectarse al broker, 

                //verificamos la infromacion que llega 

                //pero primero debemos de verificar que la lista ya haya sido creado si es asi primero debemos de crear el primer nodo 
    
                //primero verificiamos que en efecto el usuario no se encuentre registrado 

                

                //okay, estamos a punto de tener un codigo espagetti por aca, entonces debemos de reformular esta parte mi estimadp 

                /**
                 * entonces neceisot una logic que no haga mi codigo u codigo espagueti 
                 *  
                 * ---primero antes de relizar el CONNECT debemos de verificar que el usuario no se encuentro registrados ya, pudo haber un bug 
                 * por parte del usuario que enivara 1 connect o 2 al mismo tiempo 
                 * 
                 * 
                */
                node_user_t *aux; //se usuara comoa varibale auxiliar, en donde asingamos el untero de la creacion del primer nodo o si ya esta 
                                //registrado del nodo que se encontro, tambine serviara para no perder el puntero del head 
                if(linked_list_user == NULL){

                    //si la lista no esta inicializada quiere decir que no se enceuntra registrado nadie y seria el primero, esto
                    //porque puede da errores a la hora de agregar un nuevo usuario 
                    linked_list_user = create_user_node(rx_frame->ip, rx_frame->port);
                    aux = linked_list_user; 

                }else{
                    //en otro caso la lista esta incializada por lo que podemos buscar
                    aux = find_node_user(linked_list_user,rx_frame->ip);

                    if(aux != NULL){
                        //el usuario ya se euncetra registrado
                        ESP_LOGI("MAIN-REGISTER_USER", "usuario con IP: %s ya se euntra registrado!", rx_frame->ip);
                    }
                    else{
                        // registramos al usuario
                        linked_list_user = push_node_user(linked_list_user, rx_frame->ip, rx_frame->port); //agregamos un nuevo usuario
                        aux = find_node_user(linked_list_user,rx_frame->ip); //despues busco al nuevo usuario
                    }
                }

                //en este momento si agrege en la lista o agrege a ala lista, tengo el nodo del usuario con e cual voy a trabajr que esta en aux

               
                
                //ahora verificamos si la lista de los mensajes esta creada o aun no. q
                if(aux->pending_msg !=NULL){

                    aux->pending_msg = add_new_node_msg(aux->pending_msg, OP_ACK, &(rx_frame->format_request));
                    aux->pending_enumerate = aux->pending_enumerate++; //le sumamos 1 indicando que tenges un mensaje mas de o que habia pendeintes para enviar 
                }
                else{
                    //la creamos
                    aux->pending_msg = create_node_msg_pendentig(OP_ACK, &(rx_frame->format_request));
                    aux->pending_enumerate = aux->pending_enumerate++; //le sumamos 1 indicando que tenges un mensaje mas de o que habia pendeintes para enviar 
                }

                xQueueSend(send_msg_queue, &aux, 0);
            }

            else if(rx_frame->format_request.header == PUBLISHED){

                //un uusario quiere relizara una publicacion  

                //primero es verificar que el usuario se encuentre registrado 

                //igual, primero verificar que este nicado la lists 


                //enonces si existe la lista, existe una lista mas no se sabe si existe el usurio por eso lo buscmos 

                // 1 -> antes de publicar verificar que exista el usuario 

               

                //encontro al usuario dentro de la lista, por lo que aora debdemos de hacer lo mismo con la lista enlzada encargada de los topics 


                //ahora si llego a este punto el usuario esta registrado por lo que puede relizar la publicacion 

                /**
                 * puedesn pasar varios escenarios 
                 * --> la lista enlazada que registra los distintos topics no se eucnetra incializada 
                 * 
                 * --> se encuentra incialiada la lista, pero el topic no se enucetra registrado por lo que creamos el nodo en la lista 
                 * 
                 * --> el topic se encuetra reigstrado por lo que no reigstramos uno nuevo solo pasamos la publicacion, 
                 * 
                 * --> que puede pasar al enviar la publicacion, 
                 * 
                 * --> la publicacion se va con exito 
                 * --> un suscriotr recibe un NACK indicano que hubo un error en la rasmicion de los datos. por lo que debemos de volver enviar el dato
                 *      entonces necesitamos un grupo de eventos que indique que si hubo un error entonces se vuelva a enviar los datos. 
                 * 
                 * -->> por ahora la primera 2 partes 
                 * 
                */

                //creo que lo mejor antes de pasar  todo esto es asegurarnos que haya espacio en el arreglo

                //okay, si no existe una lista entonces quiere decir que no hay topicos registrados por lo que nos pasamos de aqui, pero agregamos
                //el primer topico en la lista global 

                
                //ya se ceuntra incializada la lista pero primeor devemos de verificar que en efecto haya espacio suficiente 
                //entonces la lista existe buscamos si existe e topcico en donde el usuario quiere relizar una publicacion s
               

                    //no encontramos el topico entonces primero antes de agregarlo debemos de verificar que haya espacio para poder pubicar 
                    //porque no se puede publicar en un topico que no se haya registrado, en cuento se haya una publicacion o suscripcion en un 
                    //nuevo topico lo que hace el broker es registrarlo 
                    //si se pblica y no tiene suscriptiores pues no desecha no tiene caso nadie lo va a recibir 
                
                //en otro caso entonces si existe el topico por lo que no lo agregamos a la vairble 

                //entonces debemos de verificar que este tenga suscroptiores si no, no tiene caso el relaiar el envio porque nadie lo va a recibri

               
                
                //neceito llamar la funcion para enviar 
                
                //en otro caso hay al menos 1 suscriptor. 

                //pero la cosa es que debemos de enviar a cada suscriptore que se supone que esta en la lista 

                // entonces si o si debe de ser un ciclo for o while, recorrinedo el arreglo, 
                
                

                //************************************************************* */
                /**
                 * en este caso un usuario relizao una publicacion, por lo que de aqui debemos de mandar hacia la funcion, la cola que se encrga
                 * de mandar a todos los suscriptores la publicacion. 
                 * 
                 * 
                 * -> un publicador primero debe d estar registrado para poder relizar pulicaciones 
                 * 
                 * 
                 * 
                */

                node_user_t *aux;
                pending_msg_t *aux_msg;
		
		node_msg_pub_t *msg_pub_send; // esta vribale trae relamente el mensaje a enviar hacia la funcion que funciona para enivar 

                if(linked_list_user == NULL){
                    // no hay usuarios registrados por lo que de una mandamos hacia atras porque ni el que publico lo esta 
                    //en este caso no me importa si llega o no llega, pero si mostrar aqui en el broker que no esta registrado

                    ESP_LOGE("PUB-ERR", "error!, no hay usuarios registrados, tampoco %s", rx_frame->ip);
                    vPortFree(rx_frame);
                    continue;

                }

                //seguimos hay al menos 1,
                //buscamos al usuario 
                aux = find_node_user(linked_list_user, rx_frame->ip);

                if(aux == NULL){
                    //no lo cnetoro por lo que no esta registrado
                    ESP_LOGE("PUB-ERR", "error!, usuario %s no reigstrado.", rx_frame->ip);
                    vPortFree(rx_frame);
                    continue;
                }

                //entonces si lo enceontro 
                //registramos su topico. si esta o no esta, si no esta se registra pero si esta no se registra y se reliza la publicacion 

                //verificamos que la lista de los topicos esta incializada 

                if(linked_list_topic_subs == NULL){
                    //si no se euncetra ningun topico registrado entonces ningun usuario esta suscritp, por lo que creamos el topico 
                    // y no enviamos nada, solo damos a continue para seguir esperando 

                    linked_list_topic_subs = create_subs_node(rx_frame->format_request.topic);
                    //creo o iniceo la lista enlazada. 
                    topico[count_tipic++] = rx_frame->format_request.topic; // en este caso no hay falla porque es el primero, por lo que no deberia de dar errores 
                    vPortFree(rx_frame);
                    continue;
                }
                
                //en este caso la lista esta inciailizada por lo uqe debemos de asegurarnos i se ecuentra registrado o no nuesto topico 

                node_subs_t *aux_sub = find_node_subs_topic(linked_list_topic_subs, rx_frame->format_request.topic);
                
                if(count_tipic >= MAX_TOPIC){

                    ESP_LOGE("ERROR-MAX_TOPIC", "Error!, numero maximo de temas registrados");
                    //para este punto el uuario esta registrado 
                    //le regresamoa al USUARIO un NACK, que en este caso ya se conoce cual es 

                    
                    msg_pub_snd->pending_msg->type = OP_NACK;
                    xQueueSend(send_msg_queue, &aux,0);
                    continue;
                }
                else{

                    if(aux_sub == NULL){
                        //entonces, no enceontro el topico pero la lista ya esta inicalizada 
                        // hay espacio para agregar un nuevo topico, 
                        linked_list_topic_subs= push_node_subs_topic(linked_list_topic_subs, rx_frame->format_request.topic);
                        
                        //nadie esta suscrito or lo que iual lo desechamos 
                        topico[count_tipic++] = rx_frame->format_request.topic;
                        vPortFree(rx_frame);
                        continue;
                    }
                    else{
                        //encontro el topico que puede tener suscriptores 
                        // int i = aux_sub->count; //le pasamos a I el numero total de suscrtroes que tiene actualemnte el arreglo 

                        pending_msg_t *new_msg_create = NULL; 
                        node_user_t *suscribed= NULL;
                        for( int i =0; i< aux_sub->count; i++){
                            suscribed = aux_sub->suscribed[i];

                            //este contendra el nodo, el nuvo mensje creado listo para encolar 
                            //para el usuario, debemos de hacer otra funcion para crear la lista de los nodos pendientes que se tienen 
                            // verificar, - crear - agregar - buscar  - eliminar 

                            //una funcion que realice estas operaciones y que llame para relizar estas mismas puede ser? 

                            /**
                             * 
                             * esta parte del codigo de que es lo que se encarga?, se encarga de mnadar a la cola el mensje que se tiene que enviar 
                             * pero se debe de mandar el nodo exacto, no puedo mandar toda la lista, por lo que 
                             * 
                             * pero esto pasara a usuario a usuario, por lo que creo que esto podemos transformalo en una funcion que hara esto mas facil 
                             * 
                             * que es lo que hara la funcion 
                             * 
                             * - verifica que la cola del los mensajes pendientes del usuario este inciailizada
                             * - si no lo esta entonces la crea y regresa la cadebza de la cola ya que es el unico nodo en este momento 
                             * - si ya esta inicalizada entonces crea el nodo y la agrega al final, no nos improta si es mensaje repetido o no, este broker solo 
                             * reetrasmite los datos  
                             * - se tiene que regresar el nodo pero tambien se tiene que modificar la cola, porque se tiene que agregar
                             * 
                             * 
                             * bueno al final tendre que relizar una funcion de busqueda para buscar los nodos, el pop para la cola, lo ideal es que la cola es la primera que sale. s
                             * 
                             * 
                             * 
                             * noooo, ijodela verga, no, no puede ser 
                             * 
                             * 
                             * */


                            if(suscribed->pending_msg == NULL){
                                //este susciptor no tiene mensajes pendeintes por lo que debemos de inciarlizar la cola 
                                suscribed->pending_msg = create_node_msg_pendentig(OP_PUB,&(rx_frame->format_request));
                                suscribed->pending_enumerate++;
                                //aqui no hay probelma porque solo hay 1 
                                aux_msg = suscribed->pending_msg;
                            }
                            else{
                                //ya tiene incializado la cola, por lo que debemos de agregarlo 

                                aux_msg = add_new_node_msg(suscribed->pending_msg, OP_PUB, &(rx_frame->format_request));
                                suscribed->pending_enumerate++;

                                new_msg_create = find_node_msg_snd(suscribed->pending_msg, 0, 0, EOF);

                                /***
                                 * !!!!NOOO, porque en el otro archivo necesito la ip y el rueto a enviar, entonces si debe de ser suario 
                                 * 
                                 * 
                                 */

                                if(xQueueSend(send_msg_queue, &new_msg_create, 0) != pdTRUE){
                                    vPortFree(new_msg_create);
                                }
                            }

                           // no el mensaje que debemos de mandar es el mensaje nuevo creado porque pues alla tenre que buscar que mensajes ya han sido enviados.  


                        }

                        


                    }

                }
            
            }



            else if(rx_frame->format_request.header = SUSCRIBED){
                // el rpoceso es un poco similar a publicar 

                // primero verificamos que haya usuario creados 

               

                //entonces esta incializada necesito saber si este usuario se enceuntra registraado en la lista enlazada

                
            }


        }   


    }

}







//++++++++++++++++++++++++++++++ TAREAS
// esta deberia de ser modificada 
void task_update_wifi(void *parms){


    while(1){

        EventBits_t bits = xEventGroupWaitBits(g_EVENT_WIFI, DELETE_TCP, pdFALSE, pdTRUE, portMAX_DELAY);

        if(tcp_client.connected !=0){

            //al actualizar la red WIFI, el socket debe ser cerrado 

            // close(tcp_client.sockdf);
            // tcp_client.connected =0;
            // tcp_client.logged_in = 0;
            // tcp_client.sockdf = 1;
            // tcp_client.host_ip = NULL;
            // tcp_client.host_port =0;
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




//+++++++++++++++++++++++++++funciones aca 


void setup_server_udp(void){

    char msg[100];
    int len;
    esp_err_t ret;

    while(1){

        ret = init_udp();

        //revisamos lo que trajo 

        if(ret != ESP_OK){

            //aqui es en donde se supone que deberia de lanzar que si quere no se que madres que la verga, lo que are es avisar 
            //que no se completo un while 1 y washar que pasho 

            len = snprintf(msg, sizeof(msg),"no se pudo establecer el socket UDP");
            uart_write_bytes(UART_0, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_0, msg, len);
            uart_write_bytes(UART_0, UART_RESET, strlen(UART_RESET));
            continue;

            while(1);
        }

        //pudo establecer el sokcer, ahora sigue crear la estrucutra que seria el formato en como llegaran los datos 

        // la tarea que esta esuchadno lo que se recibe 

        // --> 

        if(first_instance != 1){
            xTaskCreate(task_recvfrom_udp, "task_recvfrom_udp", 4098, NULL, 8,NULL);
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
        
        // case HOST_TCP:{
        //     free(tcp_client.host_ip);
        //     tcp_client.host_port =0;

        //     tcp_client.host_ip = strdup(token_1);
        //     //ahora convierto ese string es un valor de 16 bits 
        //     uint16_t port = (uint16_t)atoi(token_2);
        //     tcp_client.host_port = port;


        //     if(tcp_client.host_ip == NULL || tcp_client.host_port == 0){

        //         len = snprintf(msg, sizeof(msg), "Error al asignar memoria!\r\n");
        //         uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
        //         uart_write_bytes(UART_MAIN, msg, len);
        //         uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
        //         return ESP_FAIL;
        //     }


        //     return ESP_OK;
        // }break;

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


//+++++++++++++++++++++++ listas enlazadas 


//creamos la estruuctra que define al nuevo usuario (esto parece POO)
node_user_t *create_user_node(char *ip, uint16_t port){


    node_user_t *aux = malloc(sizeof(node_user_t));
    aux->ip = strdup(ip);
    aux->pending_enumerate = 0; //este sera un contador indicando cunados mensajes se tiene pendeinte para el envio 
    aux->pending_msg = NULL; // este sera la cola en donde guardaremos los mensajes pendeintes a enviar para asegurarnos que se envio 
    //pero aun no se registran mensajes praea ese usuaio, bueno en el momento de la creacion 
    aux->port = port;
    memset(aux->sub, 0, MAX_NODES_CONNECTED);
    aux->nxt = NULL; //apuntador al sigueinte nodo del usuario 

    return aux;
}




node_user_t *push_node_user(node_user_t *head,char *ip,uint16_t port){

    //cunado esta con este es que ya ha tenido un nodo anteriormente 

    node_user_t *aux =head;
    aux = find_node_subs(aux, ip);

    if(aux!=NULL){

        //encoentro un usario con esas mismas credenciles 
        //por lo que no es necesario crear un nuevo usuario 

        ESP_LOGI("MAIN", "usuario e ip ya registrados");
        free(aux); //si porque no se uso 
    }
    else{
        //no se ceonctro vmoa a tener que crearlo 
        aux = create_node_user(ip, port);
        // aux->count_enum = curren_enum;  
        aux->nxt = head;
        head = aux;
    }
    return head;
}

node_user_t *find_node_user(node_user_t *linked_list, char *ip){


    node_user_t *aux = linked_list;

    while(aux!=NULL){
        if(strcmp(ip, aux->ip) == 0){
            //es el usuario que estamos buscando 

            return aux;
        }
        
        aux = aux->nxt;
    }
    //no se llego 
    return NULL;
}


// funciones de la lista d topicos 

/* 
 * si, este esta bien, porque es cunado creamos el nuevo topic, el edit de cad anodo es en donde se va a modificar las propiedaddes 
 * 
 * 
*/ 

node_subs_t *create_subs_node(uint8_t num_topic){

    node_subs_t *aux = malloc(sizeof(node_subs_t));

    aux->num_topic = num_topic;
    aux->capacity = MAX_NODES_CONNECTED;
    aux->count =0;
    
    //esta parte esta bien pero no deberia de ir en la creacion del nodo, esto porque, porque no se si el nodo que lanza la creacion 
    //de la lista es un publicador o un suscrptor, por lo que lo que debemos de hacer es si, ncializarlo pero asignarle NULL 
    //esto hasya que otra funcion que relamente responda a los nodos suscritos 

    aux->suscribed = malloc(MAX_NODES_CONNECTED * sizeof(node_user_t*));
    aux->suscribed = NULL;

    aux->nxt = NULL;
    return aux;
}


// push a la pila 
node_subs_t *push_node_subs_topic(node_subs_t *head, uint8_t topic){


    node_subs_t *aux = head;
    node_subs_t *new = create_subs_node(topic);

    new->nxt = aux;
    head = new;

    return new;
}


//con esto entonces sacamos el nodo 
node_subs_t *find_node_subs_topic(node_subs_t *topic_linked_list, uint8_t topic){

    node_subs_t *aux = topic_linked_list;

    while(aux!=NULL){

        if( aux->num_topic == topic){

            return aux;
        }
        aux = aux->nxt;
    }
    return NULL;
}


//cola de mensajes pendeinte s
pending_msg_t *create_node_msg_pendentig(op_type_t type_msg, format_request_t *msg){

    pending_msg_t *aux = (pending_msg_t*)malloc(sizeof(pending_msg_t));
    
    aux->num_retry = 0;
    aux->msg_info = (format_request_t*)malloc(sizeof(format_request_t));
    aux->msg_info->enumerate = msg->enumerate;
    aux->msg_info->header = msg->header; //este al final no tiene sentido pero ya viene 
    aux->msg_info->len = msg->len;
    aux->msg_info->topic = msg->topic;

    int len_payload = aux->msg_info->len - 2;
    memcpy(&aux->msg_info->msg, &msg->msg, len_payload);


    aux->type = type_msg;
    aux->nxt = NULL;    

    ESP_LOGI("create pila msg", "pckid: %d, memory: %p", aux->msg_info->enumerate, &(aux));

    return aux;
}


/**
 * 
 * mi idea es que con esta misma funcion retornar 2 valores, al parecer si es posible mediante apuntadores dobels 
 * 
 * 
 * con esto actualizo el puntero, la cola desde donde lo llamo y regreso en nuevo nodo creado. 
 * 
 * 
 * */

pending_msg_t *add_new_node_msg(pending_msg_t *head,op_type_t type_msg, format_request_t *new_frame){

    pending_msg_t *aux = head; 
    pending_msg_t *new = create_node_msg_pendentig(type_msg, new_frame);

    //recorremos la cola hasta el final 
    while(aux->nxt != NULL){
        aux = aux->nxt;
    }
    aux->nxt = new;

    ESP_LOGI("PUSH MSG", "head cola - pckid: %d, memory: %p", new->msg_info->enumerate, &(aux));

    //regresamos el incio de la cola poerque ajora aux esta en e penuntimo nodo de la cola. 
    return head;
}




pending_msg_t *find_node_msg_snd(pending_msg_t *head,uint8_t topic,int pckid, uint8_t flag){

    pending_msg_t *aux = head;


    if(flag == EOF){
        //nos vamos al final 
        //quireor el ultimo nodo 
        while(aux->nxt != NULL){
            aux = aux->nxt;
        }
    }

    else{
        //esta en busqueda de un nodo en especial 
        while(aux != NULL){

            if(aux->msg_info->topic = topic){
                if(aux->msg_info->enumerate = pckid){
                    return aux;
                }
            }

            aux = aux->nxt;
        }

    }

    return NULL;
}


