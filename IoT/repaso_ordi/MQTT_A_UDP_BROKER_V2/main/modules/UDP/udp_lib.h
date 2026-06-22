#ifndef UDP_LIB_H
#define UDP_LIB_H


//este ser anuetro broker por lo necesitmos definir el puerto 

#define PORT 2026 

//la ip pues no, esa la traemos en la estrucutra en WIFI donde gurdamos la IP que se nos asgingo aqui, entonces en el bind() que 
/**
 * es la funcon que dice tenga esta IP y esucho por este puerto, traemos ese miebro
 * 
*/


//esrucutra de la infromacion del socket 

typedef struct
{
    int sockdf;
    int server_port;
    char *server_ip;
    int logged_in; //este si puede ir, podemos hacer algo aqui como de primero hace la solicitud de conectarse, no hay una conexion viva 
    //pero si detectara que la infromacion de una ip en concreta esta autorizado, (puede ser creo que esta de mas ademas estos solo funcion en red local )
}udp_server_t;

extern udp_server_t udp_server;






//creamos un grupo de eventos para UDP 

extern EventGroupHandle_t g_udp_event_group;


//+++++++++++ cola

extern QueueHandle_t udp_data_flow;





/**
 * 
 * @brief funcion que construye el socket UDP 
 * 
*/
esp_err_t  init_udp();


//funcion que envia publicaciones o mensajes a los usuarios sea suscritor o publicador
void send_messange();

//tarea encargada de recibir los datos: 

void task_recvfrom_udp(void *params);




#endif
