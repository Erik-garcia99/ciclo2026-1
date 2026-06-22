
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#include"global.h"
#include"udp_lib.h"


const char *TAG ="UDP";

esp_err_t udp_client_init(){


    //en esto pues no relaizamos ninguna conexion o se intenta lo que se hace es crear el socket y establecer la dirrecion y el puerto 

    //como el 3er parametro indica el protocolo a usar, podemos poner explicitament IPPROTO_UDP o poner 0, al poner 0 el sistema asigna el 
    //protocolo adeucado al seleccionado en este caso UDP 
    udp_client.sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(udp_client.sockfd < 0){
        ESP_LOGE(TAG, "error al crear el socket: %d", errno);
        return ESP_FAIL;
    }
    
    struct timeval timeout ={
        .tv_sec = 10,
        .tv_usec =0 
    };

    setsockopt(udp_client.sockfd,SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    //establecemos la direccion 

    struct sockaddr_in server_addr ={
        .sin_family = AF_INET,
        .sin_port = htons(udp_client.host_port)
    };

    inet_pton(udp_client.sockfd,udp_client.host_ip, &server_addr.sin_addr);

    udp_client.server_addr = server_addr; //esto para podrlo usar en las funciones para enviar y recibir 
}


void udp_recv_task(void *params){

    uint8_t rx_buffer[128];

    while(1){
        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(udp_client.sockfd,rx_buffer,sizeof(rx_buffer)-1, 0, (struct addrinfo*)&source_addr,&socklen);
        if(len < 0){
            ESP_LOGE(TAG, "erro al recbir mensaje -> errno: %d", errno);
            vTaskDelay(pdTICKS_TO_MS(50));
        }
        else{
            //para UDP solo hay que no recibio nada o recibio, no esta el 0 que indiquca que se cerro la conexion porque no hay en UDP 

            //en este caso llego mas que 1, llego algo por recv 
            rx_buffer[len] = '\0'; //termino el pensaje con un caracter nulo 

            int offset = 0; 
            char payload[128];

            memcpy(udp_client.rx_buffer, &rx_buffer, len);  

            // int len = snprintf(payload, sizeof(payload), "msg: %s", rx_buffer);
            ESP_LOGI(TAG, "msg: %s", rx_buffer);
        } 
        
        vTaskDelay(pdTICKS_TO_MS(500));
    }


}


esp_err_t send_message(uint64_t *messange){

    //envamos un mensaje no mas de 128 bytes, porque?, ps nomas 

    //en este si vamos a ocupar la estruuctra mimebro en donde definimos 

    int len = sendto(udp_client.sockfd, messange, sizeof(messange), 0, (struct addrinfo *)&udp_client.server_addr, sizeof(udp_client.server_addr));

    if(len < 0){
        ESP_LOGE(TAG, "error al mandar los datos -> errno: %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "datos enviados");
    return ESP_OK;
}
