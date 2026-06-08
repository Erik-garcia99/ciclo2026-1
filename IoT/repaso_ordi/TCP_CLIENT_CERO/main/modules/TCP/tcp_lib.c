
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
    for(int i=0; i< 5; i++){

        tcp_client.sockdf = socket(AF_INET, SOCK_STREAM, 0);

        if(tcp_client.sockdf < 0){
            //ocurrio un error al asignar el descriptior 
            len = snprintf(msg, sizeof(msg), "no s pudeo asignar el descripto\r\n");
            uart_writes_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_writes_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            continue;
        }

        if(tcp_client.sockdf ==0 ){
            //el servidor cerr la conexion 
            len = snprintf(msg, sizeof(msg), "el servidor cerro la conexion\r\n");
            uart_writes_bytes(UART_MAIN, UART_RED, sizeof(UART_RED));
            uart_write_bytes(UART_MAIN, msg, len);
            uart_writes_bytes(UART_MAIN, UART_RESET, sizeof(UART_RESET));
            continue;
        }   


        //seguimos
        



    }


    return ESP_FAIL;

}


