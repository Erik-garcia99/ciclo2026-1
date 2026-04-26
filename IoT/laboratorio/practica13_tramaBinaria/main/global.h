
#ifndef GLOBAL_H
#define GLOBAL_H

/**
 * modificaciones: 
 * eliminaos enums de op_type que no usamos, porque nosotros no mandamos infromacion recibidos, en este proyecto ya que es cliente tcp
 * 
 * el campo del user que tendra para enviar y como nos vamos a identificar 
 * 
 * 
 * 
*/


#include<driver/uart.h>
#include<driver/gpio.h>

#define TRUE 1
#define FALSE 0
#define UART_MAIN UART_NUM_0

//HEADER del paruqte 

#define HEADER 0xCAFE

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

}op_type_t;

extern op_type_t op_type;

//ahora la operacion y este desmadre sera
typedef enum{
    action_none  = 0x00,
	read = 0x01,    
	write = 0x02,
	access = 0x04, //login
	keep_alive = 0x05,
}action_t;

extern action_t action;


typedef enum{
	led = 0x01,
	adc=0x02,
	pwm=0x03,
    server = 0x0f,
}resourse_t;
extern resourse_t resourse;


//modificamos user, porque ahora no sera un string si no un valor de 4 byte
extern uint32_t user;


//esta varibale se actualizara cunado se prenda un led 
extern uint8_t led_state;

/*
 * se requiere una estrucutra la cual pueda crecer conforme las acciones y recursos tambien se agregen a las habilidades del sistema 
 *
 * por lo que lo mas adecuado podria ser un enum,
 *
*/





// //++++++++++++++ posiblemente esta estrucutra se tendra que ir, este funcionaba para verificar en una cadena ASCII o creo que ni tenato sentido tiene con la cadena 

// //esta estructrura su unica funcion es verificar que la peticion este construida de manera adecuada 
// //para establecer la comunicion
typedef struct{
    uint16_t id; //constante porque el identificador simepre sera 0xCA 0xFE -> por lo que lo decaraomso constante de 16 bits -> 2 bytes
    uint8_t len; //se establece la longitud de la trama
    uint32_t user;//uusario, la matricula con la que se identifica en el servidor  
    //valor de 1 bytes en donde accion -> MSB(pero es nibble) y recurso -> LSB
    //accion
    action_t action:4;
    //recurso
    resourse_t resourse:4;
    uint8_t value[32]; //un valor de 0 a 32 bytes para el valor cunado es escritrua 
}format_request_t;


format_request_t format_request;



typedef struct{
    op_type_t op_type; //que operacion vamos a relaizar 
    format_request_t *format_req_send; //trametos la trama a enviar 
}send_info_t;



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

