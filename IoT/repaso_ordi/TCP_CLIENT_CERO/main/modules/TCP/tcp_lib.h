#ifndef TCP_LIB_H
#define TCP_LIB_H


#include"global.h"

//+++++++++++++++++++++macros


//++++++++++++++++++++colas
//cola por la cual mandare los datos ya acomodadmos que llegan por TCP hacia la tarea que se encarga de verificar 
//que es lo que se recibio
extern QueueHandle_t tcp_data_flow;


//+++++++++++++++++++grupos de eventos 

extern EventGroupHandle_t g_tcp_event_group;


//++++++++++++++++++++uniones 

//+++++++++++++++++++++estrucutras 

//una conexion TCP necesita el socket, si esta logeado, la ip y el puerto a conectarse

typedef struct{

    int sockdf;
    char *host_ip;
    uint16_t host_port;
    int connected;
    int logged_in;

}tcp_client_t;

extern tcp_client_t tcp_client;

//manerjadore de tareas 

extern TaskHandle_t recv_handle;



//+++++++++++++++++++++funciones 

//funcion encargada de establecer la conexion con el servio 

esp_err_t tcp_cliente_init();

esp_err_t send_massage();

//+++++++++++++++++++++tareas 

void task_recv_tcp(void *params);




#endif