
#ifndef GLOBAL_H
#define GLOBAL_H


#include<driver/uart.h>

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

/**
 * este archivo .h contendra variables, macros, colas, etc, que seran necesario o que compartiran varios archivos .c  
*/

//estrucutra que contendra el puerto de UART esot sera ideal para cunado estemos menjando diferentes puerto de UART 

typedef struct {
    uart_port_t NUM_PORT;
}task_uart_port_t;

extern task_uart_port_t global_uart; 


//cola que controlara el flujo de la ifnromacion entre los diferners archivos 
extern QueueHandle_t flow_data_queue; 

//necesitaremos otra cola que este encargada de notifiar  que ya hay nuevas credenciales para que este vuelva a intentar.
extern QueueHandle_t wifi_credential_queue;


/**
 * comandos que voy a necesitar para que se puedan configurar en el caso necesario 
 * 
 * WIFI -> primero preguntara sis e trata de una red wifi de empresa -> <debemos de poner por defecto nuestra red> -> pero como saber cunado se trata de una red normal
 * o una red empresarial? 
 * 
 * red normal
 * 
 * SSID:<SSID> ->al final de la cadena terminara con el caracter nulo '\0'
 * PSWD:<pasword> ->
 * //como sera un do-whilw entonces por lo menos lo intentara 1 vez 
 * 
 * para un red de empresa < UNI >
 * 
 * ENT_SSID:<SSID>;
 * ENT_PSWD:<PSWR>;
 * 
 * 
*/




#endif

