#include<stdio.h>
#include<stdlib.h>

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>
#include<freertos/event_groups.h>

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"

#include<esp_log.h>


#include<tcp_lib.h>
#include<global.h>

static const char *TAG = "TCP_CLIENT";



//creamos la conexion con el servidor, por lo que creamos el socket, que sera un socket global, para mas facil 
esp_err_t tcp_client_init(){

    

    //intenteamos al menos 5 veces relizar la conexion 

    for(int n= 0 ; n< 5; n++){

        ESP_LOGI(TAG, "intento # %d ", n);

        tcp_sck = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(tcp_sck < 0 ){
            //indica que hubo un error en la creacion del socket, el descriptor debe ser un numero mayor a 0
            //cunado indica 0, indica que la conexion se cerror, en este caos puede que no se de, pero en un futuro. 
            ESP_LOGE(TAG,"no se pudo crear el socker, error: %d", errno);
            //ponemos continue porque no tiene caso verificar lo demas porque el socket que es el descriptor que utilzia las demas funciones no 
            //se pudo crear.
            continue;
        }

        //se pudo crear el descriptor trae un identificador adeciado 

        struct timeval timeout = {
            .tv_sec = 5,
            .tv_usec = 0
        };

        //esta funcion lo que nos permite es personaizar el socket a nivel de kernetl, utilizado para configurar tiempos de espera, reutilziacion de puertos, politias de 
        //envia y recepcion de datos 

        /**
         * parametros:
         * sockfd : el descriptor del socker creado
         * level : el nivel de protocolo donde reside la opcion, en el caso de TCP ponderemos < SOL_SOCKET> -> Indica al sistema operativo que la opción que vas a 
         * configurar o consultar debe aplicarse a nivel general del socket
         * 
         * optname: es el parametro que se va confiurar, estre estos existen 3 tipos 
         *      --> SO_REUSEADDR: Permite reutilizar una dirección IP y un puerto inmediatamente después de cerrar un servidor 
         *      --> sO_RCVTIMEO / SO_SNDTIMEO: Establece tiempos de espera máximos (timeouts) para enviar o recibir datos.
         *      --> SO_KEEPALIVE: Envía paquetes periódicos para verificar que una conexión inactiva sigue viva
         * 
         * opval: es el valor de la opcion 
         * oplen : el tamanio de la estrucutra o de la opcion
         * 
         */
        setsockopt(tcp_sck, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout));

        //estrucutra en donde se deifen la direccion del servidor

        struct sockaddr_in server_addr= {
            .sin_family = AF_INET, // indica que sera un IPv4, esta la otra igual cunado se trata de una IPv6
            .sin_port= htons(HOST_PORT),

        };

        // cono .sin_addr, se trata de una estruxutural lo que se hace es utilizar: inet_pton() que transforma la direrecion IP en su formato binario en orden en bytes de red
        //listo en bit enddian para ser enviados por la red 

        inet_pton(AF_INET, HOST_IP,&server_addr.sin_addr);

        //ya se construyo el socket y se etbalecio la direccion y el puerto al cual se conectara, entonces ahora se reliza la conexion, se trata de relizar la conecion 

        //es el socket, es los parametros d ela conecion y el tamanio de este, connect puede dar estas respuesta: 
        /**
         * 
         * 0  si se pudo relaizar la conexion 
         * -1 si ocurrio un fallo
         */
        if(connect(tcp_sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ){
            ESP_LOGE(TAG, "error al relizar la conexion con el servidor: %d", errno);
            //antes de dale al sigueitne intento debemos de cerrar y limpiar el socket y la direccion 
            close(tcp_sck);
            tcp_sck = -1;
            //agunata un ratito 
            vTaskDelay(pdTICKS_TO_MS(2000));
            continue; //no tiene caso seguir 
            
        }

        //pasa hacia aca quiere decir que la conexion fue exitosa 
        ESP_LOGI(TAG, "conexion exitosa a IP: %s PORT:%s", HOST_IP, HOST_PORT);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "no se pudo establecer la conexion con IP:%s  PORT:%s", HOST_IP, HOST_PORT);

    return ESP_FAIL;

}



//despues ponermos la 

