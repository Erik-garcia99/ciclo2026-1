#ifndef UART_LIB_H
#define UART_LIB_H

//freeRTO
#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>

#include<driver/uart.h>
#include<esp_log.h>
#include<esp_err.h>


#define MAIN_UART UART_NUM_0
/**
 * @brief el tamanio dl buffer en donde ingresaran los datos que entran por UART
 * 
*/
#define BUFFER 1024 


/**
 * @brief cola en donde se ingresaran los datos de UART
 *  
*/
QueueHandle_t uart_event;

//varibales 



//prototipo de funciones 

/**
 * @brief inicar un UART 
 * 
 * @param sel_uart selecciona puerto de uart a usar < UART0,UART1,UART2>
 * @param uart_baudrate establece al baudrate de comunicacion 
 * @param uart_data_length establece el tamanio de la plabra que aceptara UART < 5 , 6, 7 ,8 bits >
 * @param uart_parity establece la paridad del frame enviado o recibido < indica si hubo perdida de datos en la comunicacion >
 * @param stop_bit estableces el bits de parada del frame 
 * @param RX pin de recepcion 
 * @param TX pin de trasmision 
 * 
 * 
 * @return ESP_OK si se asigno correctamente el puerto de uart
 * @return ESP_FAIL si en algun punto fallo 
*/


esp_err_t uart_init(uart_port_t sel_uart, int uart_baudrate, uart_word_length_t uart_data_length, uart_parity_t uart_parity, uart_stop_bits_t stop_bits, int RX, int TX);



void task_uart(void *params);





#endif