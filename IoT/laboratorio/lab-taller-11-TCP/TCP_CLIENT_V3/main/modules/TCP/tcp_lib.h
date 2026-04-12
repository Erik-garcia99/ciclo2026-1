#ifndef TCP_LIB_H
#define TCP_LIB_H

//macros
//definamos un punto, en este caso haremos un local host en mi red, por lo que cunado arranque tratara de coenctarse 
//a algun servidor o servicio en mi maquina en mi red, pero al no encontrarla se podra cambiar la direccion de host y el puerto 

#define DEFAULT_HOST "192.168.1.66"
#define DEFAULT_PORT "5000" 
#define RETY_SERVER BIT0
#define READY_CRED BIT1


//variables
//vamos a definir variables 
// extern char *host_ip;
// extern char *host_port;


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
    char *host_port;
    int sock;
    int connected;
    int logged_in;

}tcp_client_t;


extern tcp_client_t tcp_client;
//mantendre el grupo de eventos pero si despues no cuentro una razon por la que este lo saco 
extern EventGroupHandle_t g_tcp_event_group;

//cola para indicar si quiere intentar la coenxion de nuevo o no 
extern QueueHandle_t q_tcp_client_queue;



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
 * @brief funcion encarga de crear el socket con el que se va a conectar al servidor. lo intentara 5 veces antes de salir e indicar que no se pudo conectar 
 * 
 * @return ESP_OK cunado se pude establecer la conexion 
 * @return ESP_FAIL cunado no se pudo establecer la conexion 
 * 
 * 
*/
esp_err_t tcp_cliente_init(void);

 




//tareas


#endif