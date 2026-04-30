
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

#define OUTPUT_PIN 19
#define ADC_CHANNEL ADC_CHANNEL_4
#define PWM_LED 18

#define PWM_MAX     8191
#define BINARY_MAX  1


/**
 * este archivo .h contendra variables, macros, colas, etc, que seran necesario o que compartiran varios archivos .c  
*/

//estrucutra que contendra el puerto de UART esot sera ideal para cunado estemos menjando diferentes puerto de UART 

typedef struct {
    uart_port_t NUM_PORT;
}task_uart_port_t;

extern task_uart_port_t global_uart;


typedef enum{
    OP_LOGIN,       // L:S
    OP_KEEPALIVE,   // K:S
    OP_ACK,         // ACK
    OP_NACK,        // NACK
    OP_WRITE_LED,   // W:L
    OP_READ_LED,    // R:L
    OP_READ_ADC,    // R:A
    OP_READ_PWM,    // R:P
    OP_WRITE_PWM,   // W:P

}op_type_t;

extern op_type_t op_type;
//este es para represnetar "UABC" en todos los comandos primero va este 
extern char user[MAX_USER_LEN];

typedef struct{
    op_type_t op;
    uint16_t value;
    char comment[MAX_COMMENT_LEN]; 
}send_info_t;

extern send_info_t send_info;

//esta varibale se actualizara cunado se prenda un led 
extern uint8_t led_state;

//esta estructrura su unica funcion es verificar que la peticion este construida de manera adecuada 
//para establecer la comunicion
typedef struct{
    char *header; //UABC
    char *user; //user que ya tenemos 
    char resource[3]; //["L","A","P"]
    char operation[2];//['R','W']
    int value; //debemos de verificar que el valor sea un valor adecuadao, tipo numero no caracter 

}format_request_t;

extern format_request_t format_request;





//cola que controlara el flujo de la ifnromacion entre los diferners archivos 
extern QueueHandle_t flow_data_queue; 
extern QueueHandle_t tcp_rx_queue;

void gpio_init();

esp_err_t validate_binary(const char *str, uint8_t *out);
esp_err_t validate_pwm(const char *str, uint16_t *out);


/**
 * @brief tarea encargada de ser el mediador entre lo que se recibe el esp,  parsea los datos, detecta que es lo que 
 * se pretende hacer, recopila los datos y manda de nuevo para poder mandar la infromacion. 
 * 
 * 
*/
void tcp_process_task(void *params);


#endif

