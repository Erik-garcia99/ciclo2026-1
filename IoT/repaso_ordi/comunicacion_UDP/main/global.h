#ifndef GLOBAL_H
#define GLOBAL_H

//freeRTO
#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>


/**
 * en esta libleria .h contendra varibales, funciones, cosas que todos los archivos .c podrian necesesitar pero no deberian o podria o no estar asociados a un archivo 
 * como declrar una varibale, una cola para transportar infromacion entre los diferentes archivos podria estar rn UART, el wifi, etc. entonces para evitar enlcar archivos 
 * de codigo que solo se necesta 1 varibale, 1 cola, 1 funciones, usaremos este.  
 * 
 * 
*/

/**
 * necesitamos una cola, esta cola sera algo temporal, algo que tan solo servira para enviar datos de una tarea, a otra tarea, funcion, etc..  
 * 
*/

extern QueueHandle_t flow_data_queue;



//estrucutra parametros para tarea de UART

typedef struct{
    int NUM_UART;
}task_uart_params_t;


extern task_uart_params_t global_uart;






#endif