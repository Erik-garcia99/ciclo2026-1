


#include <string.h>
#include <errno.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>

#include "esp_log.h"

#include<esp_err.h>
#include<tcp_lib.h>


//definicion de variables 
//definimos aqui las varibales de la estrucutra tcp_clinete_t, este nos srvira para saber en que punto del proceso de conexion nos encontramos 


const char *TAG="TCP_CLIENT : ";


static int n_retry;


esp_err_t tcp_cliente_init(void){


    n_retry = 0;

    do{

        ESP_LOGI(TAG, "intento %d de establecer conexion", n_retry);

        //creamos el descriptor del socket 
        tcp_client.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(tcp_client.sock < 0){
            ESP_LOGE(TAG, "error al crear el descriptor del socket(): %d", errno);
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
            continue;
        }

        tcp_client.connected = 1;
        ESP_LOGI(TAG, "conectado a %s:%d", tcp_client.host_ip, tcp_client.host_port);

        n_retry++;

        //esperamos 30s antes de volver a dar otra vuelta puede que el servidor tenga probelmas demosle tiempo en que se pueda levantar 
        vTaskDelay(pdMS_TO_TICKS(30000));
    }while((n_retry < 5 ));

    if(tcp_client.connected == 1){
        return ESP_OK;
    }

    return ESP_FAIL;
}

