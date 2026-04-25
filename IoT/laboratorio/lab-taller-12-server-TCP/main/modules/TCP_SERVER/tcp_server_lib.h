
#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include"global.h"



#define DEFAULT_PORT "5000"



#define UPDATE_TCP BIT0
#define EXT_TCP_MEM BIT10
#define FAIL_TCP_MEM BIT11
#define BREAK_UPDATE_WIFI BIT12

typedef struct {
    char *host_ip;
    uint16_t host_port;
    int sock;
    int connected;

}tcp_client_t;

extern tcp_client_t tcp_client;
//mantendre el grupo de eventos pero si despues no cuentro una razon por la que este lo saco 
extern EventGroupHandle_t g_tcp_event_group;



/**
 * @brief tarea encargada de recibir lo que se manda del servidor hacia el ESP
 * 
 * 
 * 
*/
void recv_task(void *params);

/**
 * @brief funcion encarga de crear el socket con el que se va a conectar al servidor. lo intentara 5 veces antes de salir e indicar que no se pudo conectar 
 * 
 * @return ESP_OK cunado se pude establecer la conexion 
 * @return ESP_FAIL cunado no se pudo establecer la conexion 
 * 
 * 
*/
esp_err_t tcp_cliente_init(void);



//funcion que debe de realizar el login
/**
 * @brief funcion que funcionara para enivar los datos hacia el servidor
 * 
 * @param sockfd parametro en donde vendra el descriptro con el sokcer abierto en el momento para enviar la infocmacion 
 * @param msg dentro de esta estrucutra se encontrara la informacion del datos a enviar, en especial <op> indicando que estrucutra es la que se va a enviar 
 * 
 * 
 * 
 * @return ESP_OK si se envio correctamente 
 * @return ESP_FAIL ocurrio un error en el envio de los datos. 
 * 
 * 
*/
esp_err_t send_message(tcp_client_t *sockfd,send_info_t *msg);





#endif

