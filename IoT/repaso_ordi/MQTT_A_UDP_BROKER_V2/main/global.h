#ifndef GLOBAL_H
#define GLOBAL_H

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>


// macros 
#define CLAVE_OFS "clave_IoT"
//los header que son que es lo que quiero hacer con la informacion que se trae en el frame 
#define CONNECT 0xCAFE //indica que el sistema quiere reaizar conexion con el broker    
#define PUBLISHED 0xF1F0
#define SUSCRIBED 0xE4E4
#define CONNACK 0x3501
#define FRAME_SIZE 38 //actulamente es 38 bytes 
#define EOF 0xFF // -> terminacion del contexto del frame 

#define UART_MAIN UART_NUM_0

#define UART_RED     "\033[31m"
#define UART_RESET    "\033[0m"
#define UART_GREEN   "\033[32m"
#define UART_YELLOW  "\033[33m"
#define UART_BLUE    "\033[34m"
#define UART_MAGENTA "\033[35m"
#define UART_CYAN    "\033[36m"
#define UART_WHITE   "\033[37m"

#define MAX_ARGS 5
#define MAX_TOPIC 16 
#define MAX_NODES_CONNECTED 10 //el maximo de disposivotos que un topico soporta 


//aqui pondremos todos los bits para tenerlos organizados y no andar buscando por todos lados 
//inicio de sesion con TCP <UDP no necesita establecer conexion>
#define LOGIN_SUCCESS BIT1
#define LOGIN_FAIL    BIT2
#define WIFI_CONNECTED_BIT BIT3
#define WIFI_FAIL_BIT BIT4
#define WIFI_UPDATE BIT5
#define DELETE_UDP BIT12
//bits para TCP 
#define UDP_DISCONNECTED BIT7
#define UPDATE_UDP BIT8 

//definicion de usuario 
#define UPDATE_USER BIT11
//login al server 
#define LOGIN_OK BIT14
#define NO_LOGIN BIT15



// /++++++++++++++++++++++ colas
extern QueueHandle_t flow_data_queue;
//necesitamos una cola por la cual enviaremos los mensajes, estos por la razon de que se pueden junstar varias envios, peticiones etc.
//entonces la cola es la mejor forma de enviar, donde se envia la mas reciente. 
extern QueueHandle_t send_msg_queue;






//+++++++++++++++++++++++++++ grupo de eventos 

extern EventGroupHandle_t g_user_def;


//+++++++++++++++++++++++++ enums

typedef enum{
    OP_KEEPALIVE,   // K:S
    OP_ACK,         // ACK
    OP_NACK,        // NACK
    OP_PUB,
    OP_SUB,
    OP_NON //no realiazr ninguna operacion 
}op_type_t;

extern op_type_t op_type;

//ahora la operacion y este desmadre sera

typedef enum{
    action_none  = 0x00,
	read_esp = 0x1,    
	write_esp = 0x2,
    cancel_resert_esp=0x03, //cancelar el reinciio del esp
	keep_alive = 0x5,
}action_t;

extern action_t action;

typedef enum{
    resert_esp = 0x00,
	led = 0x1,
	adc=0x2,
	pwm=0x3,
    enable_tmp = 0x8, 
	set_tmp = 0x9, 
	down_tmp = 0xA,  
    server = 0xf,
}resourse_t;

extern resourse_t resourse;


typedef enum{
    SSID=0,
    PSWD,
    HOST_TCP,
    HOST_UDP,
    USER,
}instructions_t;

extern instructions_t instructions;




//++++++++++++++++++++++++++ estrucutras 

//frame 

/**
 * es necesario un header para que dinqiue si quiere < CONNECT(0xCAFE)  - PUBLISHER(0xF0F1) - SUBCRIBED (0xE4E4) - ACK(0x3501) y NACK (0x3501) --> igual > -->
 * 
 * --> tenemos 3 niveles de QoS -> tenemos len indica el tamanio de la trama, debemode de verificar que la trama que llego 
 *  sea del mismo tamanio que se indica, esto es apartir del sigueiten byte despues de EL porque heder no cuenta
 *      --> enumarate: cada paquete dentro de un contexto estara enumerado, deben de ser consecutivos mayores a 0, 
 *          si un paquete se brinca un numero se indicara que te brincaste un paquete. 
 *      --> tenemos el ACK por parte del servidor y cliente, si el servidor no conesta con un ACK ta rapido como llego el usuario 
 *      llegara a la conclusion que el paquete no fue entregado por lo que reenvia el paquete nuevamente 
 * 
*/


// esta estrucutura funciona para cunado yo SERVIDOR quiero enivar algo hacia un usuario en concreto, si tiene mas de una sesion activa 
//yo servidor quiero 

//el valor lo vamos a ofuscar, al final es lo mismo tan solo lo pasamos por un XOR 
typedef struct{
    uint16_t header; // es ne 
    uint8_t len; //se establece la longitud de la trama
    uint8_t enumerate;  
    //accion
    uint8_t topic;
    uint8_t msg[32]; //un valor de 0 a 32 bytes para el valor cunado es escritrua 
}format_request_t;

//creo que al final no se manejan la varibale global, no, porque cada nosos manejaara su proprio frame 
// extern format_request_t format_request;


typedef struct
{
    op_type_t type;
    char *ip;
    uint16_t port;
    format_request_t format_request;
}send_info_t;



typedef struct{
    char *ip;
    uint16_t port;
    format_request_t format_request;

}frame_recv_t;




//la sigueinte estrucutra su proposito es que el proceso de buscar que nodos estas sucrutrs a que topicos sea mucho mas rapido sin 
//tener que estar buscando uno a uno, cada nodo por cada item del arreglo, si no guardar su direccion de memoria y ya se que este nodos
//no se que usuario es pero se que este nodo esta suscruto por lo que enivale la publicacion. 

/*
 * el detalle quee sta en cunatos usuarios puede tener, usaremos un tmanio limitado, unos 10 por ejemplo en caso que sea necesario se 
 * necesita implementar la logica para reasiganr la infromacion de un nuevo bloque otros 10 con realloc puede ser 
 * 
*/


//este se activa cunado el emsanje nidca que quiere suscribirse 

//ahorita esta estrucuutra no tiene nad que la identifique, que diga este mensaje es de esto. 

typedef struct pending_msg{
    uint8_t num_retry;
    format_request_t *msg_info;
    op_type_t type;
    struct pending_msg *nxt;
}pending_msg_t;

/*
 * estrucutras en donde estaremos organizando como es el proceso de guardar y conocer que usuario esta sscrutoa a que 
 * si el usuario se quiere conectar se lanza este nodo, aun no se ha sscrito o ha publicado pero primero necesita reistrarse en el broker 
*/ 


//esta estrucutra guarda la infroacion de los usuarios 
typedef struct node_user{
    char *ip;
    uint16_t port;
    uint8_t pending_enumerate; //numero de mensajes pendintes 
    pending_msg_t *pending_msg; 
    // TimerHandle_t retry_timer; --> ??
    //lo topicos en donde este usario esta suscrito. 
    uint8_t sub[MAX_TOPIC]; //en donde esta suscrito 
    struct node_user *nxt;
    
}node_user_t;


typedef struct node_subs{
    uint8_t num_topic;
    //representa la capacidad que cada topico tiene para albergar suscriptiores y lleva el conteo de estos  
    uint8_t capacity;
    uint8_t count;
    //esta varibale representa el conteo del contexto, es uno de los metodos que tenemos 
    node_user_t **suscribed; //--> lo que busco es guardar las direcciones de memoria en donde se guarda la estrucutra que repsenta al usuario.  
    struct node_subs *nxt;
}node_subs_t;







/**
 * 
 * en esta lista se guardan los topics que han entrado, debemos de verificar que en efecto sea uno 
*/
extern uint8_t topico[MAX_TOPIC]; 

extern int count_tipic;



// extern send_info_t send_info;

extern uint32_t current_user;


//la funcion que se encarga de aseugarse que todos los sub reicban el paqute correctamente 
/**
 * esta funcion debde de recibir l que entra sinedo un ACK o un NACK  
 * 
 * menjramos punteors con la lista enazada, porque lo que en la tarea vamos a poder ver lo que pasa aqui, para poder lbrerar el topico
 * 
 * //lo del ciclo form se manera aqui, pero entonces en la tarea nos quedamos con el grupo de eventos, no, 
 * 
 * necesitamos que la tarea siga 
 * 
 * ijole creo que no va a hacer. 
 * 
 * 
 * 
*/


/**
 * 
 * @brief valida que todos los suscriptores 
 * 
*/
void validate_all_nodes(frame_recv_t *recv_frame);





#endif
