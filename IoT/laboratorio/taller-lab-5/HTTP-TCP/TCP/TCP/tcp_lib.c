#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <esp_log.h>

#include<freertos/FreeRTOS.h>
#include<freertos/event_groups.h>

#include "esp_netif.h"
#include "esp_log.h"

#include"tcp_lib.h"

static const char *TAG = "TCP";


char *get_time_tcp(void){


    //resolvemos el DNS
    struct hostent *server = gethostbyname(HOST);
    if (server == NULL) {
        ESP_LOGE(TAG, "DNS fallo");
        return NULL;
    }

    //2 creamos el socket
    /**
     * primer parametro indica la familia de direccion, AF_INET para IPv4 y AF_INET6 para IPv6
     * 
     * el segundo parametro indicia que tipo de socket sera, entonces este es un <SOCK_STRAM> 
     * por lo uqe establece una conexion de extremo a extremo, este envia datos sin errores ni duplicados
     * y recibe los datos en el orden de envio. 
     * 
     * 
     * tercer parametros indica el protoclo, por lo genreal se pone 0 para que el sistema eliga el protocolo
     * predeterminado segun la familia. 
     * 
     * @return -> retorna el descriptor del socket, entero no negativo 
     * @return -> -1 en caso de error
     * 
     * 
    */
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0){
        ESP_LOGE(TAG, "ERROR al crear el socket");
        return NULL;
    }



    //paso 3; configruar direccion y conectar
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port=htons(SERVER_PORT);
    memcpy(&dest.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        ESP_LOGE(TAG, "error a conectarlo");
        close(sock);
        return NULL;

    }

    ESP_LOGI(TAG, "conectando a %s", HOST);

    //enviar la peticion HTTP cruda 

    send(sock, REQUEST,strlen(REQUEST), 0);

    //recibnir los datos 

    char response[1024]={0};
    int total =0, bytes=0;
    do {
        bytes = recv(sock, response + total, sizeof(response) - total - 1, 0);
        if (bytes > 0) total += bytes;
    } while (bytes > 0);
    response[total] = '\0';

    close(sock);  // 6. Cerrar socket

    // Separar header del body buscando \r\n\r\n
    char *body = strstr(response, "\r\n\r\n");
    if (body == NULL) {
        ESP_LOGE(TAG, "No se encontro el body");
        return NULL;
    }
    body += 4;  // saltar el \r\n\r\n

    ESP_LOGI(TAG, "Body: %s", body);
    return strdup(body);  // retornar copia del body
}

/**
 * 
 * REF :  
 * 
 * https://www.ibm.com/docs/es/i/7.6.0?topic=characteristics-socket-type
 * 
 * 
 */