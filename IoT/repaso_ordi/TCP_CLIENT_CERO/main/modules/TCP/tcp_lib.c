
#include<stdlib.h>
#include <string.h>
#include <errno.h>


#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>
#include<freertos/queue.h>

#include "esp_log.h"
#include<esp_err.h>

#include<driver/uart.h>
#include<global.h>
#include<tcp_lib.h>


esp_err_t tcp_cliente_init(){


    //lo que haremos al igual que con WIFI sera el intentar cnectarme 5 veces 
    char msg[120];
    int len;
    
    while(tcp_client.host_ip == NULL || tcp_client.host_port == 0){
        //si no se enceuntran incialziadas las credeniclaes para la conexion con el servidor el sistema esperara 
        len = snprintf(msg, sizeof(msg), "establecer credenciales TCP < IP y PORT > \r\n");
        uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
        uart_write_bytes(UART_MAIN, msg, len);
        uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
        
        EventBits_t bits;
        bits = xEventGroupWaitBits(g_tcp_event_group, UPDATE_TCP, pdTRUE, pdTRUE, portMAX_DELAY);

        if(bits & UPDATE_TCP){
            // si ya se actualizao entonces se rompe el ciclo 
            break;
        }
    }
    
    //no avanzara hasta que se haya establecidos al menos al inicio 

    for(int i=0; i< 5; i++){

        

        len = snprintf(msg, sizeof(msg), "intento : %d\r\n", i);
        uart_write_bytes(UART_MAIN, msg, len);

        tcp_client.sockdf = socket(AF_INET, SOCK_STREAM, 0);
        if(tcp_client.sockdf < 0){
            //ocurrio un error al asignar el descriptior 
            len = snprintf(msg, sizeof(msg), "no s pudeo asignar el descripto\r\n");
            uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            continue;
        }

        if(tcp_client.sockdf == 0 ){
            //el servidor cerr la conexion 
            len = snprintf(msg, sizeof(msg), "el servidor cerro la conexion\r\n");
            uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            continue;
        }   

        //la asignacion fue exitosa 
        //se define el timeout, lo que el socket esperara en la funcion recv. 

        struct timeval timeout = {
            .tv_sec = 5,
            .tv_usec = 0
        };

        setsockopt(tcp_client.sockdf,SOL_SOCKET,SO_RCVTIMEO, (const char *)&timeout,sizeof(timeout));

        //asigmaos la direccion del servidor.
        //pero antes de llamar a esta funcion anteriormente se debio de haber establecio la IP y el puerto por lo que debemos de verificar 

        //verificamos que se haya inicados 

        if(tcp_client.host_ip ==NULL || tcp_client.host_port == 0){
            //si alguno de los 2 no se encuentra inicalizado debemos amrcar un error y regresar 
            len = snprintf(msg, sizeof(msg), "no se encuentra inciadado las crendicales del servodr <IP y puerto>\r\n");
            uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
            return ESP_FAIL;
        }

        struct sockaddr_in server_addr ={
            .sin_family = AF_INET,
            .sin_port = tcp_client.host_port
        };

        inet_pton(AF_INET, tcp_client.host_ip, &server_addr.sin_addr);


        //una vez establecigo esto se relaiza la conexion ese famoso handshake entre el TCP cliente y el TCP servidor 

        if(connect(tcp_client.sockdf, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
            //si es menor que 0 quiere decir que no se pudo conectar 
            len = snprintf(msg, sizeof(msg), "no se pude establecer la conexion : %d\r\n", errno);
            uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
        }

        //en otro caso si se puedo 

        tcp_client.connected =1;

        len = snprintf(msg, sizeof(msg), "CONEXION a: IP:%s <-> PORT:%d\r\n", tcp_client.host_ip, tcp_client.host_port);
        uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
        uart_write_bytes(UART_MAIN, msg, len);
        uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

        return ESP_OK;
    }
    return ESP_FAIL;

}


//ahora lo que sigue es la funcion que recibe desde TCP 


void task_recv_tcp(void *params){

    //imrpmirmr por pantalla
    char msg[100];
    int len;

    uint8_t buffer[50];
    uint8_t *msg_recv; //este es para mandar hacia la tarea 

    while(1){

        // resetear buffer de impresion en cada iteracion
        char hex_buf[128] = {0};
        int pos = 0;


        int size_recv = recv(tcp_client.sockdf, buffer, sizeof(buffer)-1, 0);

        if(size_recv < 0 ){
            //si es menor que 0 quiere decir uqe fallo, pero nos improta 2 fallos que indican solamnete que aun no ha recibido nada 

            if(errno == EAGAIN || errno ==EWOULDBLOCK){
                vTaskDelay(pdTICKS_TO_MS(50));
                continue;
            }
            len = snprintf(msg, sizeof(msg),"\r\nTCP_LIB: recv fallo errno: %d\r\n", errno);
            uart_write_bytes(UART_MAIN,UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_MAIN,msg,len);
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            close(tcp_client.sockdf);
            tcp_client.connected = 0;
            tcp_client.host_ip = NULL;
            tcp_client.host_port = 0;
            tcp_client.logged_in =0;

            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);
            //lo elimanosrmos desde la funncion de setup_tcp, cerramos conexion y se vuelve a crear un nuevo sokcet y establecer una conexio  
        }

        else if(size_recv == 0){
            len = snprintf(msg, sizeof(msg),"el servidro cerro la conexion\r\n");
            uart_write_bytes(UART_MAIN,UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_MAIN,msg,len);
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

            close(tcp_client.sockdf);
            tcp_client.connected = 0;
            tcp_client.host_ip = NULL;
            tcp_client.host_port = 0;
            tcp_client.logged_in =0;

            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);
            //el servidor cerro la conexion
            //de igual froma cerramos desde setup_tcp 
        }
        //en este caso se recibio mas de 0 por lo que se aun hay una conexion viva

        //por lo que ahora lo que toca es separar los BITS en sus diferntres componenes 

         //+++++++++++++++++++++++++++imprimir lo que recibimos 

        for(int i = 0; i < len; i++){
            pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%02X ", buffer[i]);
        }
        uart_write_bytes(UART_MAIN, UART_CYAN,  strlen(UART_CYAN));
        uart_write_bytes(UART_MAIN, "\r\nRAW: ", 7);
        uart_write_bytes(UART_MAIN, hex_buf, pos);
        uart_write_bytes(UART_MAIN, "\r\n",   2);
        uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        format_request_t *rx_request = pvPortMalloc(sizeof(format_request_t));
        int offset = 0; //me tendre que recorrer entre 
        
        
        if(size_recv > 40){
            //si supera los 40 bytes qyuere decir qu ehay sobre flujo 
            len = snprintf(msg, sizeof(msg), "sobre flujo de datos recibidiso\r\n");
            uart_write_bytes(UART_MAIN, UART_RED,  strlen(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
            continue; //solo seguira esperando 
        }

        //lo primero es el HEADER, si es una peticion o un ACK o un NACK, 

        memcpy(&rx_request->header, &buffer, 2); offset +=2;

        //ahora comprobamos que trae el header 

        if(rx_request->header != HEADER){
            //diferente a HEADER quiere decir que es entonces es un ACK o un NACK, la forma de saber si es un ACK o un NACK es mediante el contenido de un len y value 
            //pero eso no se define aqui si no en la tares que procesa lo que se sta pdineido  
            
            if(buffer[offset] > 32 || buffer[offset] <= 0){
                //en este caso hubo un error en donde se delcararon ya sea el fromato o se enciaron de mas o no se nevio datos 
                //en la parte de len del frame 
                len = snprintf(msg, sizeof(msg), "ERROR!/r/nla ubicacion de LEN se enceuntra en overflow o daniada\r\n");
                uart_write_bytes(UART_MAIN, UART_RED,  strlen(UART_RED));
                uart_write_bytes(UART_MAIN, msg, len);
                uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
                
                //mandamos un NACK indicamos que no se enteindio 

                continue; //se va a esperar que llegue el procimo frame 
            }

            memcpy(&rx_request->len, &buffer[offset], 1); offset++;
            

        }
        else{
            //quiere decir que es una operacion
        }

        //al final mandmos 

        /**
         * porque es diferne a true, es algo raro no, lo que pasa es cunado el sistem retorna true o pdPASS quiere decir que ya se cnolo entones 
         * la tarea que recibe el dato en la cola se encarga de librerar este espcio de memoria 
         * 
         * pero en el caso que sea diferente, quiere decir que no se recibio por lo que nadie lo libereara, entonces en este caso si llega un pdFALSE quiere decir que no se 
         * reciio por eso necesitoamos librerar el espacio manualmente. 
         * 
         */
        if(xQueueSend(tcp_data_flow, &rx_request, 0) != pdTRUE){
            vPortFree(rx_request);
        }










    }
}

/***
 * 
 * debemos de verificar antes de enviar que el uaurio el esp se enceuntre loegead. para evitar fallos 
 * 
 */

esp_err_t send_massage(){


    //se estableceran en la estrucutra global lo que se quire enviar 
    uint8_t buffer[40];
    int offset =0;

    char msg[100];
    int len;

    char hex_buf[128] = {0};
    int pos = 0;

    switch(send_info.type){

        case OP_ACK :{
            uint16_t id = htons(ACK); 
            memcpy(buffer + offset, &id, 2); offset+=2;

            uint8_t payload_len = format_request.len;
            
            if(payload_len <0 || payload_len > 32){
                //no puede supar los 32 byts 
                len = snprintf(msg, sizeof(msg), "longitud de len < 32 bytes and > 0 ");
                uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
                uart_write_bytes(UART_MAIN, msg, 100);
                uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
                return ESP_FAIL;
            }
            else{
                memcpy(buffer + offset, &payload_len, 1); offset++;
                memcpy(buffer + offset, format_request.value, payload_len);
            }
        }break;

        case OP_LOGIN:{

            //uyuyuy antes de realziar esto, en el setup debemos de inicar que debe de inciar el usuario que estara usando. 
            //es la unca donde se relizara todo el proceso 
            uint16_t id = htons(HEADER);
            memcpy(buffer + offset, &id, 2); offset += 2;

            buffer[offset]= 5; offset++;

            uint32_t user = htonl(current_user);
            memset(buffer + offset, &user, 4); offset += 4;

            uint8_t proccess = (access_esp << 4) | server; 
            buffer[offset] = proccess;;
        }break;

        case OP_NACK:{
            uint16_t id = htons(ACK); //porque el header para ACK y NACK es lo mismo 
            memcpy(buffer + offset, &id, 2); offset += 2;
            
            uint8_t payload_len = 0xFF;
            memcpy(buffer + offset, &payload_len, 1); offset++;

            format_request.value[0] = 0;
            memcpy(buffer + offset, format_request.value, 1);;
        }break;

        default :{
            len = snprintf(msg, sizeof(msg), "ERROR : tipo de operacion no reconocida");
            uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            return ESP_FAIL;
        }break;
    }

    //despues sale de este 

    int send_pack = send(tcp_client.sockdf, buffer, offset, 0); 

    if(send_pack < 0){
        // no se envio 
        len = snprintf(msg, sizeof(msg), "TCP: error al enviar el paquete : %d", errno);
        uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
        uart_write_bytes(UART_MAIN, msg, len);
        uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
        return ESP_FAIL;
    }
    else if(send_pack == 0){
        //cerro conexion
        len = snprintf(msg, sizeof(msg), "TCP: cerro conexion el servidor : %d", errno);
        uart_write_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
        uart_write_bytes(UART_MAIN, msg, len);
        uart_write_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
        return ESP_FAIL; 
    
    }

    //solo debug
    for(int i = 0; i < offset; i++){
        pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%02X ", buffer[i]);
    }
    uart_write_bytes(UART_MAIN, UART_GREEN,      strlen(UART_GREEN));
    uart_write_bytes(UART_MAIN, "\r\nTX: ",      5);
    uart_write_bytes(UART_MAIN, hex_buf,         pos);
    uart_write_bytes(UART_MAIN, "\r\n",          2);
    uart_write_bytes(UART_MAIN, UART_RESET,      strlen(UART_RESET));


    //se envio
    return ESP_OK;

}





