#ifndef TCP_LIB_H
#define TCP_LIB_H

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>

#define HOST_IP "192.168.1.66"
#define HOST_PORT 50007


#define TCP_DISCONNECTED BIT1   // servidor cerro la conexion inesperadamente
#define EXT_TCP_MEM BIT11
#define FAIL_TCP_MEM BIT12
#define NO_RETRY_TCP BIT14




//+++++++++++++++++++++++++variables 

extern int tcp_sck;


//+++++++++++++++++++++++++estrucutras 

typedef struct {
    int sock;
    int connected;
    int logged_in;
}tcp_client_t;

extern tcp_client_t tcp_client;



//++++++++++++++++colas
extern QueueHandle_t queue_recv_send;

//+++++++++++++++++grupo de eventos 
extern EventGroupHandle_t g_tcp_event_group;

//++++++++++++++++++++ funciones 
esp_err_t tcp_client_init();

esp_err_t send_menssage();


//tarea que se encarga de estar recibeidno los datos que llegan por la red. 
void task_recv(void *params);

void task_keep_alive(void *params);



#endif 