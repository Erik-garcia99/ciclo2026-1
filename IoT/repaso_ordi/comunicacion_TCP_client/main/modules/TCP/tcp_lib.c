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

        tcp_client.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if( tcp_client.sock < 0 ){
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
        setsockopt(tcp_client.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof(timeout));

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
        if(connect(tcp_client.sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ){
            ESP_LOGE(TAG, "error al relizar la conexion con el servidor: %d", errno);
            //antes de dale al sigueitne intento debemos de cerrar y limpiar el socket y la direccion 
            close(tcp_client.sock);
            tcp_client.sock = -1;
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





void task_recv(void *params){

    uint8_t rx_buffer[40] ={0}; //iniciamos todo el arreglo en 0 

    ESP_LOGI(TAG, "inicando tarea recv");

    while(1){

        //esta es la funcion que es la que se encarga de revisar y capturar los datos que llegaran desde fuea, del servidor mas bien dicho 
        /**
         * parametres
         * 
         * descriptor del socket - 
         * -buffer en donde se guardara la infromacion 
         * - el tamanio de este buffer, pero reservamos un espacio para terminal la trama que se recivio con '\0'
         * -flags , es una bandera que idnica como va a tratar los datos entrantes.
         * 
         */
        int size_bytes = recv(tcp_client.sock,rx_buffer, sizeof(rx_buffer)-1, 0); 

        if( size_bytes < 0 ){
            //si el tamanio de los datos recibidos es  mor que 0 quiere decir que hubo un error en la trasmision 

            //pero cunado da error pueden haber una seria de banderas que indicaran porque no se tiene datos, pero la que nos
            /**
             * importa solo seria los errores EAGAINA y EWOULDBLOCK que indicaran que no hya datos entrando solamente, otro tipo de error
             * seria con problema con el socket, con el servidor, etc. por lo que cerramos la conexion para poder volvernos a contectar 
             * 
             * de otro caso en caso que sean los errores dichos solo dame con continue y va a seguir esperando una recepcion de datos.  
             *
             * i
             */

            if(errno == EAGAIN || errno == EWOULDBLOCK){
                //en este caso simplmente no se recibiron datos por lo que esperamos un tiempo y volvemos a esperar que se reciba algo 
                vTaskDelay(pdTICKS_TO_MS(50));
                continue;
            }

            //en otro caso cerramos conexion 
            close(tcp_client.sock);
            tcp_client.sock =-1;
            
            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED); //hubo un error por lo que debemos de reinicar la tarea 



        }

        else if(tcp_client.sock == 0){
            //en este caso del lado del servidor cerror cerro la conexion por lo que volvemos a intentar a reconectar 

            tcp_client.connected = 0;
            close(tcp_client.sock);
            tcp_client.sock = -1;
            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);
        }
        else{
            //en otro caso llego un flujo de bytes adecuado 

            uint8_t offset = 0; 

            //ahora separamos los difernes unidades de la estrucutra 

        }





    }

}


//despues ponermos la 

