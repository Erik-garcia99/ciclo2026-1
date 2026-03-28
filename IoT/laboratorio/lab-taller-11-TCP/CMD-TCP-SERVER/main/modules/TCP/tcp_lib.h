#ifndef TCP_LIB_H
#define TCP_LIB_H

#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>   


/**
 * 
 * 
 * 
 * 
*/

//macros

#define DEFAULT_IP "148.231.130.229"
#define DEFAULT_PORT 50005
//bits para el grupo de evetos 
#define new_cred_tcp BIT0
#define SOCK_CONN_BIT BIT1

#define BUF_SIZE      256
#define USUARIO      "a1275863"


//varibales globales

typedef struct {
    int      sock;
    bool     connected;
    bool     logged_in;
}tcp_client_t;

//
// EventGroupHandle_t s_tcp_client, update_ready;



extern tcp_client_t tcp_client;


//funciones

//la funcion que inicia las caracterisiticas del cliente tcp, 
/**
 * la funcion deberia de recibir el host < direccion IP a cual se va a comnicar > 
 * asi como el puerto en donde se va a conectar.  
 * 
 * 
 */


/**
 * @brief inicar los aspectos basicos de una conexion cliente TCP 
 * 
 * @param server_ip la ip del servidor TCP
 * @param server_port puerto al cual se comunicara 
 * 
 * 
 */
// esp_err_t tcp_init(char server_ip, uint16_t server_port);
esp_err_t  tcp_client_init(void);

//funciones para declrar nuevas credenciales, va a ser algo muy parecido con el SSID y pass del wifi 

//esp_err_t tcp_server_upate(char server_IP, uint16_t server_port);



//tareas


//por mientras para hacer el keep alive

esp_err_t tcp_client_send  (const char *data);
int       tcp_client_recv  (char *buf, size_t buf_len);
void      tcp_client_close (void);
esp_err_t tcp_login     (void);
void      keepalive_task(void *pvParameters);
void tcp_recv_task(void *pvParameters);

#endif