#ifndef TCP_LIB_H
#define TCP_LIB_H

#include"global.h"

//macros
//definamos un punto, en este caso haremos un local host en mi red, por lo que cunado arranque tratara de coenctarse 
//a algun servidor o servicio en mi maquina en mi red, pero al no encontrarla se podra cambiar la direccion de host y el puerto 

// #define DEFAULT_HOST "192.168.1.66" // local

// #define DEFAULT_PORT "5000" //local

// UABC
#define DEFAULT_HOST "148.231.130.229"
#define DEFAULT_PORT "50005"


#define RETRY_SERVER BIT0 
#define UPDATE_TCP BIT10
#define EXT_TCP_MEM BIT11
#define FAIL_TCP_MEM BIT12
#define BREAK_UPDATE_WIFI BIT13
#define NO_RETRY_TCP BIT14




/**
 * @brief estrucutra con  la informacion sobre el proceso/ de la conexion de socker TCP 
 * 
 * host_ip - direccion IP al servidor a conectarse @memberof tcp_client_t
 * host_port - puerto del servidor a conectarse @memberof tcp_client_t
 * sock - descriptor del socket creador @memberof tcp_client_t
 * connected -  bandera indicando que se realziara la conexion @memberof tcp_client_t 
 * @logged_in - bandera que indica que se logro loggear al servidor @memberof tcp_client_t 
 * 
*/

typedef struct {
    char *host_ip;
    uint16_t host_port;
    int sock;
    int connected;
    int logged_in;

}tcp_client_t;

extern tcp_client_t tcp_client;


// extern tcp_client_t tcp_client;
//mantendre el grupo de eventos pero si despues no cuentro una razon por la que este lo saco 
extern EventGroupHandle_t g_tcp_event_group;


//tareas 
/**
 * @brief tarea que envia un keep alive cada 10s al servidor.  
 * 
 * 
*/
void keep_alive_task(void *params);


/**
 * @brief tarea encargada de recibir lo que se manda del servidor hacia el ESP
 * 
 * 
 * 
*/
void recv_task(void *params);

//funciones
//la primera funcion que deberia de poner sera la que crea los sockets para establecer la conexion
/**
 * la funcion no va a recibir parametros, porque usaremos varibales globales para esto. 
 * 
 * funcion para crear y establecer la coenxion con el servidor, pero recordemos que debemos deintentanr al menos 5 veces 
 * 
 *  
*/


/**
 *---- > update
 * ahora tendremos que utilizar una varaible pasada por referencia en vez de una global.
 * 
 */

/**
 * @brief funcion encarga de crear el socket con el que se va a conectar al servidor. lo intentara 5 veces antes de salir e indicar que no se pudo conectar 
 * 
 * @param tcp_client es la estrucutra que tiene la informacion del socket de la conexion con el servidor 
 * 
 * @return ESP_OK cunado se pude establecer la conexion 
 * @return ESP_FAIL cunado no se pudo establecer la conexion 
 * 
 * 
*/
esp_err_t tcp_cliente_init();


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
esp_err_t send_message();











//tareas


#endif