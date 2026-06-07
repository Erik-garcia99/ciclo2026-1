#ifndef GLOBAL_H
#define GLOBAL_H

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>


// macros 

#define HEADER 0xCAFE
#define ACK 0x3501

#define UART_MAIN UART_NUM_0

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

//aqui pondremos todos los bits para tenerlos organizados y no andar buscando por todos lados 
//inicio de sesion con TCP <UDP no necesita establecer conexion>
#define LOGIN_SUCCESS BIT1
#define LOGIN_FAIL    BIT2
#define WIFI_CONNECTED_BIT BIT3
#define WIFI_FAIL_BIT BIT4
#define WIFI_UPDATE BIT5



// /++++++++++++++++++++++ colas
extern QueueHandle_t flow_data_queue;


//+++++++++++++++++++++++++ enums

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
	read_esp = 0x1,    
	write_esp = 0x2,
	access_esp = 0x4, //login
	keep_alive = 0x5,
}action_t;

extern action_t action;

typedef enum{
	led = 0x1,
	adc=0x2,
	pwm=0x3,
    server = 0xf,
}resourse_t;

extern resourse_t resourse;


typedef enum{
    SSID=0,
    PSWD,
    HOST_TCP_IP,
    HOST_TPC_PORT,
    HOST_UDP_IP,
    HOST_UDP_PORT,
    USER,
}instructions_t;

extern instructions_t instructions;




//++++++++++++++++++++++++++ estrucutras 

//esta estrucutra de definiran los elementos que tendra a trama que recibira o enviara el esp32 al servidor
//puede usarse tanto para TCP - UDP

//el valor lo vamos a ofuscar, al final es lo mismo tan solo lo pasamos por un XOR 
typedef struct{



    uint8_t value[32]; //un valor de hasta 32 bytes 
}format_request_t;

extern format_request_t format_request;


typedef struct
{
    op_type_t type;
    format_request_t format_request;
}send_info_t;

extern send_info_t send_info;



#endif