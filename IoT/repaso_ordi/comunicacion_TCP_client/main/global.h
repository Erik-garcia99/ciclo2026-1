
#ifndef GLOBAL_H
#define GLOBAL_H




#include<driver/uart.h>
#include<driver/gpio.h>


#define TRUE 1
#define FALSE 0

//HEADER del paruqte 

#define HEADER 0xCAFE
#define ACK 0x3501

#define user 1275863
#define MAX_USER_LEN 16
#define MAX_COMMENT_LEN 32

#define OUTPUT_PIN 19
#define ADC_CHANNEL ADC_CHANNEL_4
#define PWM_LED 18

#define PWM_MAX     8191
#define BINARY_MAX  1


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

// //lo dejamos como global o lo pasamos como parame
extern format_request_t format_request;



typedef struct{
    op_type_t op_type; //que operacion vamos a relaizar 
    format_request_t format_request; //trametos la trama a enviar 
}send_info_t;

extern send_info_t send_info;


void gpio_init();
/**
 * @brief tarea encargada de ser el mediador entre lo que se recibe el esp,  parsea los datos, detecta que es lo que 
 * se pretende hacer, recopila los datos y manda de nuevo para poder mandar la infromacion. 
 * 
 * 
*/

void tcp_process_task(void *params);


#endif

