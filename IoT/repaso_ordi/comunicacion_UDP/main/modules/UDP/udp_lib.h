#ifndef UDP_LIB_H
#define UDP_LIB_H 

#include "lwip/sockets.h"
#include"esp_err.h"

typedef struct{

    int sockfd;
    char  host_ip[16];
    uint16_t host_port;
    struct sockaddr_in server_addr;
    char rx_buffer[128];

}udp_client_t;

extern udp_client_t udp_client; //--> esto no tiene caso, recordemos que UDP crea y cierra la conexion al isntante, por lo que el socket ya no funciona 



esp_err_t udp_client_init();

//para ercbir es una tarea 

void udp_recv_task(void *params);

//para enivar es una funciones que regresa un error si no puedo enviar 

esp_err_t send_message(uint64_t *messange);


#endif