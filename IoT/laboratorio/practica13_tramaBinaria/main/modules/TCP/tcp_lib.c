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




esp_err_t tcp_cliente_init(){

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
        char conn_msg[64];
        snprintf(conn_msg, sizeof(conn_msg), "\r\nconectado a %s:%d\r\n",
                tcp_client.host_ip, tcp_client.host_port);
        uart_write_bytes(global_uart.NUM_PORT, UART_GREEN, strlen(UART_GREEN));
        uart_write_bytes(global_uart.NUM_PORT, conn_msg, strlen(conn_msg));
        uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        return ESP_OK;

    }

    // se agotaron los 5 intentos
    char conn_msg[64];
    snprintf(conn_msg, sizeof(conn_msg), "\r\nno se pudo hacer la conexion con el servidor:%s:%d\r\n",
            tcp_client.host_ip, tcp_client.host_port);
    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
    uart_write_bytes(global_uart.NUM_PORT, conn_msg, strlen(conn_msg));
    uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
    tcp_client.connected = 0;
    return ESP_FAIL;
}

// ahora que mas sigue despues de ponder coenctarme a TCP 

void keep_alive_task(void *params)
{
    while(1)
    {
        uint8_t payload[8];
        int offset = 0;

        uint16_t id = htons(HEADER);
        uint32_t user_keep = htonl(user);
        uint8_t keep = (keep_alive << 4) | server;

        memcpy(payload+offset, &id, 2); offset += 2;
        payload[offset] = 5; offset++;
        memcpy(payload+offset, &user_keep, 4); offset += 4;
        memcpy(payload+offset, &keep, 1); offset++;

        int err = send(tcp_client.sock, payload, offset, 0);
        if(err < 0)
        {
            char err_msg[80];
            snprintf(err_msg, sizeof(err_msg), "\r\nError send keep alive, errno: %d\r\n", errno);
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT, err_msg, strlen(err_msg));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }
        else
        {
            // Solo imprimir lo enviado, sin esperar ACK
            char hex_buf[128] = {0};
            int pos = 0;
            for(int i = 0; i < offset; i++)
                pos += snprintf(hex_buf + pos, sizeof(hex_buf)-pos, "%02X ", payload[i]);
            uart_write_bytes(global_uart.NUM_PORT, UART_BLUE, strlen(UART_BLUE));
            uart_write_bytes(global_uart.NUM_PORT, "\r\nKEEP ALIVE enviado: ", 22);
            uart_write_bytes(global_uart.NUM_PORT, hex_buf, pos);
            uart_write_bytes(global_uart.NUM_PORT, "\r\n", 2);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}


//esta funcion es la encargada de recibir 
//es que ahora la forma en como recibe los datos son distintos 
void recv_task(void *params){

    //aqui debemos de tener una estrucutra que tenga 
    
    uint8_t rx_buffer[40]; //41 y el /0 en donde termina 
    uint8_t *buffer_tmp;
    
    // tcp_client_t *tcp_client = (tcp_client_t *)params;

    while(1){

        // resetear buffer de impresion en cada iteracion
        char hex_buf[128] = {0};
        int pos = 0;

        int len = recv(tcp_client.sock,rx_buffer, sizeof(rx_buffer)-1,0);

        if(len < 0 ){

            if(errno == EAGAIN || errno == EWOULDBLOCK){
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;  // timeout normal del SO_RCVTIMEO, no es error
            }
            //error fatal de recv (socket caido, red perdida, etc.)
            char err[80];
            snprintf(err, sizeof(err),"\r\nTCP_LIB: recv fallo errno: %d\r\n", errno);
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));

            tcp_client.connected = 0;
            tcp_client.logged_in = 0;
            if(tcp_client.sock >= 0){
                close(tcp_client.sock);
                tcp_client.sock = -1;
            }
            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);
            vTaskDelete(NULL); // esta tarea ya no tiene socket que leer
        }

        else if(len == 0){
            //conexion cerrada por el servidor 
            char err[80];
            snprintf(err, sizeof(err),"\r\nTCP_LIB: el servidor cerro la conexion\r\n");
            uart_write_bytes(global_uart.NUM_PORT, UART_RED, strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT,err,strlen(err));
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
            tcp_client.connected = 0;
            tcp_client.logged_in = 0;
            close(tcp_client.sock);
            tcp_client.sock = -1;
            xEventGroupSetBits(g_tcp_event_group, TCP_DISCONNECTED);
            vTaskDelete(NULL); // esta tarea ya no tiene socket que leer
        }

        //minimo necesitamos 3 bytes, el head y la medida de resto del frame 
        else if(len < 3){

            uart_write_bytes(global_uart.NUM_PORT, UART_YELLOW, strlen(UART_YELLOW));
            uart_write_bytes(global_uart.NUM_PORT, "\r\nframe demasiado corto\r\n", 25);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET,  strlen(UART_RESET));
            continue;
        }

        //+++++++++++++++++++++++++++imprimir lo que recibimos 

        for(int i = 0; i < len; i++){
            pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%02X ", rx_buffer[i]);
        }
        uart_write_bytes(global_uart.NUM_PORT, UART_CYAN,  strlen(UART_CYAN));
        uart_write_bytes(global_uart.NUM_PORT, "\r\nRAW: ", 7);
        uart_write_bytes(global_uart.NUM_PORT, hex_buf, pos);
        uart_write_bytes(global_uart.NUM_PORT, "\r\n",   2);
        uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));

        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


        uint8_t offset=0;


        format_request_t *frame = pvPortMalloc(sizeof(format_request_t));

        if (frame == NULL) {
            uart_write_bytes(global_uart.NUM_PORT, UART_RED,   strlen(UART_RED));
            uart_write_bytes(global_uart.NUM_PORT, "\r\nsin memoria\r\n", 15);
            uart_write_bytes(global_uart.NUM_PORT, UART_RESET, strlen(UART_RESET));
            continue;
        }

        //imprimamos lo que llego 


        //copiamos el 0xCAFE
        memcpy(&frame->id, &rx_buffer[offset], 2); offset +=2;
        frame->id = ntohs(frame->id);

        if(frame->id != 0x3501){
            //llego un ACK y lo mandamos directo porque no teiene mas 

            memcpy(&frame->len,&rx_buffer[offset],1); offset++;

            memcpy(&frame->user, &rx_buffer[offset], 4); offset +=4;
            frame->user = ntohl(frame->user);

            uint8_t action_res   = rx_buffer[offset]; offset++;         
            frame->action   = (action_res >> 4) & 0x0f;
            frame->resourse = (action_res)      & 0x0f;

            uint8_t value_len = (frame->len > 5) ? (frame->len - 5) : 0;
            if(value_len > 32) value_len = 32;
            if(value_len > 0){
                memcpy(frame->value, &rx_buffer[offset], value_len);
            }
        }

        //ahora enviaremos llego un NACK 

        if(frame->id == 0x3501 && rx_buffer[offset] == 0xff){
            //llego un nack 
            memcpy(&frame->len,&rx_buffer[offset], 1); offset++;
        
        }

        if(frame->id == 0x3501 && rx_buffer[offset] != 0xff){
            //llego un nack 
            memcpy(&frame->len,&rx_buffer[offset], 1); offset++;
        }

        //al final si llega un ACK llega solo y enviamos por la cola 


        //encolar 
        if(xQueueSend(tcp_rx_queue, &frame, 0) != pdTRUE){
            vPortFree(frame);
        }
    }
}


/**
 * 
 * vamos a seguir con la misma estrucutra de enviar informacion 
 * 
*/
esp_err_t send_message(){

    //lo maximo que puede enviar son 40 bytes estos puede variar 
    uint8_t buffer[40];
    int offset= 0;
    int len;

    //quiero ver lo que envio 
    //imrpimri 
    char hex_buf[128] = {0};
    int pos = 0;

    switch(send_info.op_type){

        case OP_LOGIN : {
            // //0xCA 0xFE
            // uint16_t id = htons(HEADER);
            // memcpy(buffer + offset, &id, 2); offset +=2;
            // //este debe de trar el largo de toda la trama 
            // uint8_t payload_len = send_info.format_request.len;
            // memcpy(buffer + offset , &payload_len, 1); offset +=1;
            // //user
            // uint32_t user_htn = htonl(user);
            // memcpy(buffer + offset, &user_htn, 4), offset +=4;
            // //accion de login al servidor 
            // uint8_t login = send_info.format_request.action << 4 | server;
            // memcpy(buffer + offset , &login, 1); offset +=1;

            uint16_t id = htons(HEADER);
            memcpy(buffer + offset, &id, 2); offset += 2;

            // fix 1: longitud hardcodeada = 5 (user 4B + acción/recurso 1B)
            buffer[offset] = 5; offset++;

            uint32_t user_htn = htonl(user);
            memcpy(buffer + offset, &user_htn, 4); offset += 4;

            // fix 2: acción hardcodeada = access_esp (0x4), no leer del struct que llega en 0
            uint8_t login = (access_esp << 4) | server;   // 0x4F
            buffer[offset] = login; offset++;

        }break; 

        //falta esto 
        case OP_ACK: {

            // 0x3501 / 1 byte / < vlaue[32bytes] -> 2 bytes >
            //cunado quier mandar un ACK el envio es 
            uint16_t id = htons(ACK); 
            memcpy(buffer + offset, &id, 2); offset+=2;
            //tramelos el tamanio de la trama 
            uint8_t payload_len = send_info.format_request.len;
            memcpy(buffer + offset , &payload_len, 1); offset +=1;
            //por ahora porque se que no pasaran de los 16 bits lo ponemos de plano asi con la macro de 16 bits 
            if(payload_len > 32){
                //debe de tener algo y no puede ser mas de 32 bytes de infromacion 
                return ESP_FAIL; //hubo un erroe en el len
            }
            //al final con lo que tenemos sabemos que no superaran de los 2 bytes necesarios para poder escribir o leer el maximo del pwm o adc 
            if(payload_len > 0){
                memcpy(buffer + offset, send_info.format_request.value, payload_len);
                offset += payload_len;
            }

        }break;
        case OP_NACK :{

            uint16_t id = htons(ACK);
            memcpy(buffer + offset, &id, 2); offset += 2;
            buffer[offset] = 0xFF; offset++;  // len=0xFF indica NACK, sin value        
        }break;
        

        default: {
            ESP_LOGI(TAG, "debuger");
        }break;
    }

    //no hubo error por lo que pasamos por enviarlo 
    int sent = send(tcp_client.sock, buffer, offset,0);

    //verificamos que se haya enviado 

    if(sent < 0 ){
        char err_str[32];
        snprintf(err_str, sizeof(err_str), "\r\nsend() fallo errno: %d\r\n", errno);
        uart_write_bytes(global_uart.NUM_PORT, UART_RED,    strlen(UART_RED));
        uart_write_bytes(global_uart.NUM_PORT, err_str,     strlen(err_str));
        uart_write_bytes(global_uart.NUM_PORT, UART_RESET,  strlen(UART_RESET));
        return ESP_FAIL;
    }

    
    
    for(int i = 0; i < offset; i++){
        pos += snprintf(hex_buf + pos, sizeof(hex_buf) - pos, "%02X ", buffer[i]);
    }
    uart_write_bytes(global_uart.NUM_PORT, UART_GREEN,      strlen(UART_GREEN));
    uart_write_bytes(global_uart.NUM_PORT, "\r\nTX: ",      5);
    uart_write_bytes(global_uart.NUM_PORT, hex_buf,         pos);
    uart_write_bytes(global_uart.NUM_PORT, "\r\n",          2);
    uart_write_bytes(global_uart.NUM_PORT, UART_RESET,      strlen(UART_RESET));
    return ESP_OK;
}