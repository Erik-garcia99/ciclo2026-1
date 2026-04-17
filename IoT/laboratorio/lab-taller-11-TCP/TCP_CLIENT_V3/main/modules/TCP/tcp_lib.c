


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

    while(1){

        //debemos de construir lo que vamos a mandar 

        char playload[50];  
        
        char *inicio = "UABC";
        char *final = "K:S:Keep-Alive al server\n";

        snprintf(playload, sizeof(playload),"%s:%s:%s",inicio,user,final);

        int err = send(tcp_client.sock, playload,strlen(playload), 0);

        if(err < 0 ){

            const char *err="\r\nocurrio un error al enviar la informacion\r\n";
            //ocurrio un error, no pudo enviar la ifnromacion la servidor 
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, (const char*)errno, 8);
            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 3);
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

            //hubon un error al recibir del servidor 
            char err[80];
            snprintf(err, sizeof(err),"TCP_LIB: recv fallo errno: %d", errno);

            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, (const char*)errno, 8);
            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 3);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }else if(len > 0){

            //hubo exito al recbir del servidor. 
            // mostramos un debbug de la informacion que recibimos del  cliente
            rx_buffer[len]='\0';

            for(int i=0; i<= len ;i++){

                char c = rx_buffer[i];

                if(c == '\n'){
                    //linea completa 
                    if(line_buffer !=NULL && line_len >0 ){
                        //eliminamos \r
                        if(line_buffer[line_len-1] == '\r'){
                            line_buffer[line_len-1] = '\0';
                        }
                        else{
                            line_buffer[line_len-1] = '\0';
                        }
                    }

                    //creamos la copia para ser enviada 
                    char *msg_copy = strdup(line_buffer);
                    if(msg_copy){
                        //encolar el puntero, metodo no bloqueante 
                        if(xQueueSend(tcp_rx_queue, &msg_copy, 0) != pdTRUE){
                            free(msg_copy);
                            char err[80];
                            snprintf(err, sizeof(err),"TCP_LIB: cola llena, mensaje destarcado");

                            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
                            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
                            uart_write_bytes(global_uart.NUM_PORT, (const char*)errno, 8);
                            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 3);
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
            snprintf(err, sizeof(err),"TCP_LIB: el servidor cerro la conexion");
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, (const char*)errno, 8);
            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 3);
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
 * la idea de es tener agrupado todos los datos que se enciaran, para no tenerlo todos regados por todas partes 
 * 
 * 
*/
esp_err_t send_message(tcp_client_t *sockfd,send_info_t *msg){

    char buffer[128];
    int len=0;


    switch(msg->op){

        case OP_LOGIN : {

            len = snprintf(buffer, sizeof(buffer), "UABC:%s:L:S:Login server\n", user);
        }break; 

        case OP_ACK: {
            len = snprintf(buffer, sizeof(buffer), "ACK:%d",msg->value);
        }break;
        case OP_NACK :{
            len = snprintf(buffer, sizeof(buffer), "NACK");
        }break;
        

        default: {
            ESP_LOGI(TAG, "debuger");
        }break;
    }
    
    //verificamos que el snprinf no se haya psado 

    if(len < 0 || len>=(int)sizeof(buffer)){
        const char *err= "\r\nbuffer overflow al construir mensaje\r\n";
        uart_write_bytes(global_uart.NUM_PORT,UART_RED, strlen(UART_RED));
        uart_write_bytes(global_uart.NUM_PORT,err, strlen(err));
        uart_write_bytes(global_uart.NUM_PORT,UART_RESET, strlen(UART_RESET));

        return ESP_FAIL;
    }

    //no hubo error por lo que pasamos por enviarlo 
    int sent = send(sockfd->sock, buffer, len,0);

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

