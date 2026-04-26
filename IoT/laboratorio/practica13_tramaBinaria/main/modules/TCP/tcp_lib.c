
/**
 * 
 * necesitamos modificar el keep alive,
 * KEEP ALIEVE: 
 *  ahora el frame que se enivara por keep alive 
 * 
 * FunciónDirecciónCuándo usarlahtons / htonlhost → networkantes de enviarntohs / ntohlnetwork → hostdespués de recibir
 * 
 * 
*/

#include <string.h>
#include <errno.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>

#include "esp_log.h"

#include<esp_err.h>
#include<tcp_lib.h>

#include"global.h"

//definicion de variables 
//definimos aqui las varibales de la estrucutra tcp_clinete_t, este nos srvira para saber en que punto del proceso de conexion nos encontramos 


const char *TAG="TCP_CLIENT : ";


esp_err_t tcp_cliente_init(void){

    //reinicamos todo antes de inciar 
    tcp_client.connected = 0;
    if (tcp_client.sock >= 0) {
        close(tcp_client.sock);
        tcp_client.sock = -1;
    }

    for(int n_retry = 0 ; n_retry < 5 ; n_retry++){

        ESP_LOGI(TAG, "intento %d de establecer conexion", n_retry);
    
        //creamos el descriptor del socket 
        tcp_client.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(tcp_client.sock < 0){
            ESP_LOGE(TAG, "error al crear el descriptor del socket(): %d", errno);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue; //saltamos todo y se vuelve a intentar. 
        }
        //timeout de recv
        struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
        setsockopt(tcp_client.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        //direccion del servidor 
        struct sockaddr_in server_addr={
            .sin_family = AF_INET,
            .sin_port = htons(tcp_client.host_port),
        };

        inet_pton(AF_INET, tcp_client.host_ip,&server_addr.sin_addr);

        //conectar 
        if(connect(tcp_client.sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
            ESP_LOGE(TAG, "descriptor <connect()> fallo  %d", errno);
            close(tcp_client.sock);
            tcp_client.sock = -1;
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        //exito, salir de inmediato
        tcp_client.connected = 1;
        ESP_LOGI(TAG, "conectado a %s:%d", tcp_client.host_ip, tcp_client.host_port);
        return ESP_OK;

    }

    // se agotaron los 5 intentos
    ESP_LOGE(TAG, "no se pudo conectar al servidor en 5 intentos");
    tcp_client.connected = 0;
    return ESP_FAIL;
}

// ahora que mas sigue despues de ponder coenctarme a TCP 


void keep_alive_task(void *params){

    format_request_t *KA_ans = (format_request_t *)params; 

    //algo que se es que este siempre sera keep alive 

    while(1){

        //necetiamos memeoria dinaminca para el keep alive pro el hecho de la matricula pero en si tendremos unaos espacios definidos 
        /***
         * 0xCA 0xFE -> cabecesa 2 bytes 
         *  longitud 1 bye
         * matricula - 4 bytes 
         * 0x50 -> keep alive 1 byte
         * valor 0 bytes 
        */
        //por lo que el payload nos da de 8 bytes 

        uint8_t payload[8]; //varibale en donde se guardara nuestro payload
        //necesito por lo que ahora vamos a realizar el como ingresar el paquete 
        //vairbale para recorre el arreglo en donde ira la ifnromacion en el payload
        int offset=0; 
        
        uint16_t id = htons(HEADER);
        // uint8_t len = 0x05; //longitud de 5 bytes para el keep alive 

        uint32_t user = htonl(KA_ans->user);
        uint8_t keep = (action << 4 ) | 0x00;

        memcpy(payload+offset, &id, 2);   offset += 2;
        memcpy(payload + offset, &KA_ans->len,1); offset += 1;
        memcpy(payload + offset, &KA_ans->user, 4); offset +=4;
        memcpy(payload+offset, &keep, 1); offset +=1;


        int err = send(tcp_client.sock, payload,offset, 0);

        if(err < 0 ){

            char err[80];
            snprintf(err, sizeof(err),"\r\nocurrio un error al enviar la informacion errno: %d\r\n", errno);
            //ocurrio un error, no pudo enviar la ifnromacion la servidor 
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }   
        else{
            const char *success = "keep alive enviado\r\n";
            uart_write_bytes(global_uart.NUM_PORT, UART_GREEN, strlen(UART_GREEN));
            uart_write_bytes(global_uart.NUM_PORT,success,strlen(success));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }
        //esperamos 10 segundos 
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

//falta el echo de lo que se recibio. 
void recv_task(void *params){
    
    char rx_buffer[128];
    static char *line_buffer = NULL;
    static size_t line_len = 0;


    while(1){

        int len = recv(tcp_client.sock,rx_buffer, sizeof(rx_buffer)-1,0);

        if(len < 0 ){

            if(errno == EAGAIN || errno == EWOULDBLOCK){
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;  // timeout normal del SO_RCVTIMEO, no es error
            }
            //hubon un error al recibir del servidor 
            char err[80];
            snprintf(err, sizeof(err),"\r\nTCP_LIB: recv fallo errno: %d\r\n", errno);

            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }else if(len > 0){

            //hubo exito al recbir del servidor. 
            // mostramos un debbug de la informacion que recibimos del  cliente
            rx_buffer[len]='\0';

            for(int i=0; i< len ;i++){

                char c = rx_buffer[i];

                if(c == '\n'){
                    //linea completa 
                    if(line_buffer !=NULL && line_len >0 ){
                        //eliminamos \r
                        if(line_buffer[line_len-1] == '\r'){
                            line_buffer[line_len-1] = '\0';  // quita el \r
                        }
                        else{
                            line_buffer[line_len] = '\0';    // ← solo null-termina, no borra nada
                        }
                    }

                    //creamos la copia para ser enviada 
                    char *msg_copy = strdup(line_buffer);
                    if(msg_copy){
                        //echo de la infromacion que llego
                        uart_write_bytes(global_uart.NUM_PORT, UART_CYAN,    strlen(UART_CYAN));
                            const char *prefix = "\r\nRECV  : ";
                            uart_write_bytes(global_uart.NUM_PORT, prefix, strlen(prefix));
                            uart_write_bytes(global_uart.NUM_PORT, msg_copy, strlen(msg_copy));
                            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 2);
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET,   strlen(UART_RESET));

                        //encolar el puntero, metodo no bloqueante 
                        if(xQueueSend(tcp_rx_queue, &msg_copy, 0) != pdTRUE){
                            free(msg_copy);
                            char err[80];
                            snprintf(err, sizeof(err),"\r\nTCP_LIB: cola llena, mensaje destarcado -> errno: %d\r\n", errno);

                            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
                            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
                        }
                    }
                    //liberear memoria 
                    free(line_buffer);
                    line_buffer=NULL;
                    line_len=0;
                }
                else{
                    //acumular caracter
                    char *new_buf = realloc(line_buffer, line_len +2);
                    if(new_buf){
                        line_buffer = new_buf;
                        line_buffer[line_len++]=c;
                    }
                    else{
                        //error de memoria 
                        free(line_buffer);
                        line_buffer=NULL;
                        line_len=0;
                    }
                }
            }
        }
        else if(len == 0){
            //conexion cerrada 
            char err[80];
            snprintf(err, sizeof(err),"\r\nTCP_LIB: el servidor cerro la conexion -> errno: %d \r\n", errno);
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
            tcp_client.connected = 0;
            tcp_client.logged_in =0;
            close(tcp_client.sock);
            tcp_client.sock =-1;
            //falta el bit de desconect
            // xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);

            
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


/**
 * 
 * vamos a seguir con la misma estrucutra de enviar informacion 
 * 
*/
esp_err_t send_message(tcp_client_t *sockfd,send_info_t *msg){

    //lo maximo que puede enviar son 40 bytes estos puede variar 
    uint8_t buffer[40];
    int offset= 0;




    switch(msg->op_type){

        case OP_LOGIN : {
            memcpy(buffer + offset, msg->format_req_send->id, 2); offset +=2;
            memcpy(buffer + offset , msg->format_req_send->len, 1); offset +=1;
            memcpy(buffer + offset, msg->format_req_send->user, 4), offset +=4;
            uint8_t login = msg->format_req_send->action << 3 | server;
            memcpy(buffer + offset , &login, 1); offset +=1;
            memcpy(buffer + offset, msg->format_req_send->value, 32); offset += 32;
            
        }break; 

        case OP_ACK: {
            len = snprintf(buffer, sizeof(buffer), "ACK:%d\n",msg->value);
        }break;
        case OP_NACK :{
            
        }break;
        

        default: {
            ESP_LOGI(TAG, "debuger");
        }break;
    }

    //no hubo error por lo que pasamos por enviarlo 
    int sent = send(sockfd->sock, buffer, offset,0);

    //verificamos que se haya enviado 

    if(sent < 0 ){
        char err_str[32];
        snprintf(err_str, sizeof(err_str), "send() fallo errno: %d\r\n", errno);
        uart_write_bytes(global_uart.NUM_PORT, UART_RED,    strlen(UART_RED));
        uart_write_bytes(global_uart.NUM_PORT, err_str,     strlen(err_str));
        uart_write_bytes(global_uart.NUM_PORT, UART_RESET,  strlen(UART_RESET));
        return ESP_FAIL;
    }

    uart_write_bytes(global_uart.NUM_PORT, UART_GREEN,  strlen(UART_GREEN));
    const char *info_send = "dato enviado: ";
    uart_write_bytes(global_uart.NUM_PORT, info_send,strlen(info_send));
    uart_write_bytes(global_uart.NUM_PORT, buffer,      strlen(buffer));
    uart_write_bytes(global_uart.NUM_PORT, "\r\n",      2);
    uart_write_bytes(global_uart.NUM_PORT, UART_RESET,  strlen(UART_RESET));
    
    return ESP_OK;
}

