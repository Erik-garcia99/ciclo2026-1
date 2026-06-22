

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>


#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
#include<freertos/eventgroups.h>

#include"lwip/sockets.h"
#include"lwip/netdb.h"

#include<esp_log.h>
#include<esp_err.h>


#include<drivers/uart.h>

#include"udp_lib.h"
#include"global.h"
#include"modules/WIFI/wifi_lib.h"

/**
 * 
 * como el broker somo el UDP-SERVER 
 * 
 */


char msg[100];
int len;

esp_err_t init_udp(){

    //creamos el socket 

    /**
     * como la comunicacion UDP que es connection-less, no se estabece una conexiocn, lo que hare es intentar crear el sokce y establecer 
     * los parametros porque pueden salir errores  
     * 
    */

    if(udp_server.sockdf > 0){
        //hay un socket creado 
        //lo cerramos 
        close(udp_server.sockdf);
        udp_server.sockdf = -1;
        udp_server.logged_in = 0;
        udp_server.server_ip = 0;
        udp_server.server_port = NULL;
        ESP_LOGI("UDP-SERVER", "socket cerrado");
    }

    for(int i = 0; i< 5; i++){
        
        
        udp_server.sockdf = socket(AF_INET, SOCK_DGRAM, 0);
    
        if(udp_server.sockdf < 0){
            
            len = snprintf(msg, sizeof(msg),"ERROR al crear el descriptor: %d", errno);
            uart_write_bytes(UART_0, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_0, msg, len);
            uart_write_bytes(UART_0M UART_RESET, strlen(UART_RESET));
            continue;
        }

        //un ciente establece la direccion a la cual desea hablar 
        /**
         * pero ahora aqui en el servidor creamos la confguacion para que esuche en el puerto desde cualquier IP
         * 
         * hay 2 direcciones, 2 estrucutras para 1 para el envio y una para el recibir, 
         * 
         * --> en el servidor, como este estara recibeindo datos de varios puntos no guardamos la configuacion de direcion 
         * 
         * esa coniguracion que si debe de guardar es el UDP CLIENTE, ya que ese si conce el puerto y la direccion IP del servidor 
         * y solo lo manda a el o puede que a mas para los conce el SERVER no conoce de donde vienen por lo que lo hace es 
         * configurar tengo esta IP y recibo mensajes en este puerto  
         * 
         * 
         */

        // struct timeval timeout ={
        //     .tv_sec = 10,
        //     .tv_usec = 0
        // };
        // setsockopt(udp_server.sockdf, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


        int flags = fcntl(udp_server.sockdf, F_GETFL,0);
        if(fcntl(udp_server.sockdf, F_SETFL, flags | O_NONBLOCK) < 0){
            ESP_LOGE("UDP-SERVER", "no se pudo poner el socket en modo no bloqueante: %d", errno);
            close(udp_server.sockdf);
            udp_server.sockdf = -1;
            continue;
        }

        struct sockaddr_in addrsock ={
            .sin_family = AF_INET, //IPv4
            .sin_addr.s_addr = INADDR_ANY, //cuqlquier interfaz de red 
            .sin_port = htons(PORT) //un valor de 16 bits 
        };
        inet_pton(udp_server.server_ip, (const char*)&esp_wifi.ip, &addrsock.sin_addr);

        //ahora el bind, asociamos el puerto con la IP 

        if(bind(udp_server.sockdf, (struct sockaddr*)&addrsock, sizeof(addrsock)) < 0){
            len = snprintf(msg, sizeof(msg),"ERROR no se pudo establecer el puerto : %d", errno);
            uart_write_bytes(UART_0, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_0, msg, len);
            uart_write_bytes(UART_0M UART_RESET, strlen(UART_RESET));
            continue;
        }


        return ESP_OK; //se pudo establecer los datos para tener activo el UDP server
    }
    

    return ESP_FAIL; //no logro crear el socket 
}



void task_recvfrom_udp(void *params){

    char msg[100];
    int len;


    uint8_t rx_buffer[52];
    uint8_t offset;

    while(1){

        struct sockaddr_storage soruce_addr;
        socklen_t socklen = sizeof(soruce_addr);
        int recv_len = recvfrom(udp_server.sockdf,rx_buffer, sizeof(rx_buffer)-1, 0, (struct sockinfo*)&soruce_addr, socklen);

        if(recv_len < 0){
            if(errno = EAGAIN || errno == EWOULDBLOCK){
                //no han recibido mensajes
                ESP_LOGI("UDP_SERVER", "no se recibieron mensajes");
            }
            ESP_LOGE("UDP_SERVER", "error en recv cerrando y volviendo a crear :%d", errno);
            //en este caso es un progrma mas 
            EventGroupSetBit(g_udp_event_group, UDP_DISCONNECTED); //algo paso, borramos el recvfrom para volver  crearlo 

        }

        rx_buffer[recv_len] = '\0'; //terminamos 

        /**
         * ahora tengo los paquetes lo primero es saber que es conocer el header 
         * 
        */

        
        frame_recv_t *frame = vPortMalloc(sizeof(frame_recv_t));

        uint16_t id;
        memcpy(&id, &rx_buffer[offset], 2); offset +=2;

        frame->format_request.header = ntohs(id);

        // que es lo que el usuario quiere hacer < conectarse - publicar o suscribirse > 
	
        if(frame->format_request.header == CONNECT){
            //el usario busca conectarse al broker 
            
            //entnces el usuario quiere cinectarse, por lo uqe no hay contenido en si dentroe dle FRAME mas que la macro, la ip que viene en el socket y le usurio
            
            memecpy(&frame->format_request.len, &rx_buffer[offset], 1); offset++; //copiamos el tamanio que solo sera el usuaroi 4 bytes 
            
            if(frame->format_request.len > FRAME_SIZE || frame->format_request.len < 0){
                //hay un overflow de datos 
                ESP_LOGE("UDP", "overflow en el frame");
                pvPortFree(frame); //libero la meoria y se espera a que llegue el proximo frame 
                continue; 
            }
            memcpy(&frame->format_request.enumerate, &rx_buffer[offset], 1); offset++;

            uint32_t rx_user_num;
            memcpy(&rx_user_num, &rx_buffer[offset], 4); offset += 4;
            frame->format_request.user = ntohl(rx_user_num);
            //cunado se hace el connect no se se publica o se suscribe al broker antes de la conexion, si el suuario no existe se crea su nodo en otro caso si esta 
            //se verifica si esta se da acceso. 
            frame->format_request.topic = 0;
            memset(frame->format_request.msg, 0, sizeof(frame->format_request.msg));


            //sacmos la IP del usuario 

            char cliente_ip[INET_ADDRSTRLEN];
            
            //obtenemos el IP 
            struct sockaddr_in *s = (struct sockaddr_in*)&soruce_addr;
            //ahora convertimos esta direccion en binario a un formato de caractere 
            inet_ntop(AF_INET,&(s->sin_addr), cliente_ip, sizeof(cliente_ip));
            frame->port = ntohs(s->sin_port);

            frame->ip = strdup(cliente_ip);

        }
        //ahora requiere relizar una publicacion 
        else if(frame->format_request.header == PUBLISHED){
            
            memcpy(&frame->format_request.len, &rx_buffer[offset], 1); offset++;

            if(frame->format_request.len > FRAME_SIZE || frame->format_request.len < 0){
            
                ESP_LOGE("UDP", "overflow en el frame ");
                vPortFree(frame);
                continue;	
            }

            memcpy(&frame->format_request.enumerate, &rx_buffer[offset], 1); offset++

            uint32_t rx_user_name;
            memcpy(&rx_user_name, &rx_buffer[offset], 4); offset += 4; 
            frame->format_request.user = ntohl(rx_user_name);

            memcpy(&frame->format_request.topic, &rx_buffer[offset], 1); offset++;

            
            uint8_t size_value = frame->format_request.len - 6; // quitamos enumerate , usuario , topico -> para quedarnos con solo el valor no mayor a 32 bits. aunque puede
                                // ser hasta 32 bytes pero quedemosnos con esos 32 bits para poder usar el ntohs, ntohl  
            
            if(size_value < 0 ){
            
                ESP_LOGI("UDP", "ERROR!, el frame de publicacion no tiene informacion");
                vPortFree(frame);
                continue;
            }

            else if(size_value == 1){
                // un dato de 1 byte 
                
                frame->format_request.msg[0] = rx_buffer[offset]; offset ++;
            }
            else if(size_value == 2){
                //valor de 16 bits 
                uint16_t aux;
                memcpy(&aux, &rx_buffer[offset], 2); 
                uint16_t aux_net = ntohs(aux);
                memcpy(&frame->format_request.msg, &aux_net, 2); offset +=2;

            }
            else if(size_value == 4){
            
                uint32_t aux;
                memcpy(&aux, &rx_buffer[offset], 4);
                uint32_t aux_net = ntohl(aux);
                memcpy(&frame->format_request.msg, &aux_net, 4); offset+=4;
            }
            else{
                ESP_LOGE("UDP", "tamanioa del dato no soportado mayor a 4 bytes");
                vPortFree(frame);
                continue;
            }

            //un dato importante es la IP 


            char cliente_ip[INET_ADDRSTRLEN];

            //obtenemos el IP 
            struct sockaddr_in *s = (struct sockaddr_in*)&soruce_addr;
            //ahora convertimos esta direccion en binario a un formato de caractere 
            inet_ntop(AF_INET,&(s->sin_addr), cliente_ip, sizeof(cliente_ip));
            frame->port = ntohs(s->sin_port);

            frame->ip = strdup(cliente_ip);

        }

        if(frame->format_request.header = SUSCRIBED){

            memcpy(&frame->format_request.len, &rx_buffer[offset], 1); offset++;
            if(frame->format_request.len > FRAME_SIZE || frame->format_request.len < 0){
            
                ESP_LOGE("UDP", "overflow en el frame ");
                vPortFree(frame);
                continue;	
            }

            memcpy(&frame->format_request.enumerate, rx_buffer[offset], 1); offset++;

            uint32_t rx_user_name;
            memcpy(&rx_user_name, rx_buffer[offset], 4); 
            frame->format_request.user = rx_user_name; offset+=4;

            memcpy(&frame->format_request.topic, rx_buffer[offset], 1); offset++;

            char cliente_ip[INET_ADDRSTRLEN];

            //obtenemos el IP 
            struct sockaddr_in *s = (struct sockaddr_in*)&soruce_addr;
            //ahora convertimos esta direccion en binario a un formato de caractere 
            inet_ntop(AF_INET,&(s->sin_addr), cliente_ip, sizeof(cliente_ip));
            frame->port = ntohs(s->sin_port);

            frame->ip = strdup(cliente_ip);

        }


        else if(frame->format_request.header == CONNACK){
            //en este caso es neceiro llamar a la funcion indicando si se reciboio un NACK o un ACK 

            //la forma de identificar el NACK era con len en 0xFF y 0 en sus datos 

            memcpy(&frame->format_request.len, &buffer[offset], 1); offset++;
            
            if(frame->format_request.len > FRAME_SIZE || frame->format_request.len < 0){
            
                ESP_LOGE("UDP", "overflow en el frame ");
                vPortFree(frame);
                continue;	
            }

            uint32_t rx_user_name;
            memcpy(&rx_user_name, rx_buffer[offset], 4); 
            frame->format_request.user = rx_user_name; offset+=4;

            uint8_t size_value = frame->format_request.len - 4;

            else if(size_value == 0 || size_value == 1){
                // un dato de 1 byte 
                
                frame->format_request.msg[0] = rx_buffer[offset]; offset ++;
            }
            else if(size_value == 2){
                //valor de 16 bits 
                uint16_t aux;
                memcpy(&aux, &rx_buffer[offset], 2); 
                uint16_t aux_net = ntohs(aux);
                memcpy(&frame->format_request.msg, &aux_net, 2); offset +=2;

            }
            else if(size_value == 4){
            
                uint32_t aux;
                memcpy(&aux, &rx_buffer[offset], 4);
                uint32_t aux_net = ntohl(aux);
                memcpy(&frame->format_request.msg, &aux_net, 4); offset+=4;
            }
            else{
                ESP_LOGE("UDP", "tamanioa del dato no soportado mayor a 4 bytes");
                vPortFree(frame);
                continue;
            }


            char cliente_ip[INET_ADDRSTRLEN];

            //obtenemos el IP 
            struct sockaddr_in *s = (struct sockaddr_in*)&soruce_addr;
            //ahora convertimos esta direccion en binario a un formato de caractere 
            inet_ntop(AF_INET,&(s->sin_addr), cliente_ip, sizeof(cliente_ip));
            frame->port = ntohs(s->sin_port);
            frame->ip = strdup(cliente_ip);

        }

        // por lo que esto no entra en todos los casos. 
        //enivmoas por la cola 
        if(xQueueSend(udp_data_flow, &frame, 0) != pdTRUE){
            vPortFree(frame);
        }



    }


}


void send_messange(){

    pending_msg_t *msg;
    

    if(xQueueReceive(send_msg_queue, &msg, portMAX_DELAY)){


        if(msg->pending_msg->type == OP_ACK){

        }
        else if(msg->pending_msg->type == OP_NACK){

        }
        else if(msg->pending_msg->type = OP_PUB){
            //el broker manda las publicaciones hacia los suscriptores. 
        }


            

    }

}



