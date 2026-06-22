#ifndef UART_LIB_H 
#define UART_LIB_H

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>

#define BUFFER 1024

//nos vamos a comunicar a traves de UART0 tanto entrada como salida. 1



//colas 
extern QueueHandle_t uart_queue;


//creamos la funcion que inicia UART 
void uart_init(); //usaremos parametros po defecto 

void uart_task(void *params);

/**
 * @brief tarea que recibe los datos desde UART y verifica que el formato de los comandos y los comandos se encuentren registrados 
 * 
 * 
 */
void uart_format_task(void *params); 



#endif