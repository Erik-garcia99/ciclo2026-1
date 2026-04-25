#ifndef GLOBAL_H
#define GLOBAL_H

#include<driver/uart.h>
#include<driver/gpio.h>

#define TRUE 1
#define FALSE 0
#define UART_MAIN UART_NUM_0

//colores 
#define UART_RED     "\033[31m"
#define UART_RESET    "\033[0m"
#define UART_GREEN   "\033[32m"
#define UART_YELLOW  "\033[33m"
#define UART_BLUE    "\033[34m"
#define UART_MAGENTA "\033[35m"
#define UART_CYAN    "\033[36m"
#define UART_WHITE   "\033[37m"

#define MAX_USER_LEN 16
#define MAX_COMMENT_LEN 32


//estrucutra que contendra el puerto de UART esot sera ideal para cunado estemos menjando diferentes puerto de UART 

typedef struct {
    uart_port_t NUM_PORT;
}task_uart_port_t;

extern task_uart_port_t global_uart;




//cola que controlara el flujo de la ifnromacion entre los diferners archivos 
extern QueueHandle_t flow_data_queue; 


#endif


