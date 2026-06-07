#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>

#include<esp_log.h>
#include<errno.h>
#include<esp_err.h>

#include<driver/uart.h>


#include"uart_lib.h"


void uart_init(){

    //veriifcarmos que el uarto no haya sido ya inicalazdo si es asi lo borramos, borramos el driver para vovlver a asignar 

    if(uart_is_driver_installed(UART_NUM_0)){
        uart_driver_delete(UART_NUM_0);
    }

    uart_config_t uart_cgf ={
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity= UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
        
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_cgf));

    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE,UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, BUFFER *2, BUFFER*2, 20, &uart_queue,0));

    char msg[80];
    int len = snprintf(msg, sizeof(msg), "UART 0 inicado");
    uart_write_bytes(UART_NUM_0, msg, len);
}


//trea que recibe los datos por medio de UART 

void uart_task(void *params){

    uart_event_t event;
    uint8_t *buffer = (uint8_t *)malloc(BUFFER);
    uint8_t input[BUFFER];
    uint8_t index_input = 0; 


    while(1){
        if(xQueueReceive(uart_queue, (void *)&event, portMAX_DELAY)){


            switch(event.type){

                case UART_DATA : {
                    int len = uart_read_bytes(UART_NUM_0, buffer, event.size, portMAX_DELAY);

                    // flag para salir del for cuando se procesa un fin de linea

                    int done = 0;

                    for(int i = 0; i < len && !done; i++){

                        char c = buffer[i];

                        // ignorar \n siempre: las terminales mandan \r\n al presionar enter,

                        // usamos solo \r como fin de linea para no disparar dos veces.

                        if(c == '\n'){

                            continue;

                        }

                        if((c >= 'a' && c<='z') || (c >= 'A' && c<='Z') || (c>=32 && c<=63) || (c=='_') || (c == " ")){

                            uart_write_bytes(UART_NUM_0, (const char*)&c, sizeof(c));

                            if(index_input < BUFFER-1){

                                input[index_input++] = c;

                            }

                        }

                        else if(c == '\r'){
                            input[index_input] = '\0';
                            uart_write_bytes(UART_NUM_0, "\r\n", 2);

                            if(index_input == 0){

                                continue;

                            }

                            char *data_flow = (char*)malloc(strlen((char*)input) + 1);
                            strcpy(data_flow, (char*)input);

                            xQueueSend(flow_data_queue, &data_flow, portMAX_DELAY);

                            index_input = 0;
                            memset(input, 0, BUFFER);
                            done = 1;

                        }
                        else if(c == '\b'){

                            if(index_input > 0){

                                index_input--;

                                uart_write_bytes(UART_NUM_0, "\b \b", 3);

                            }



                        }



                    }







                }break;



                case UART_BUFFER_FULL :{



                    xQueueReset(uart_event);

                    uart_flush(current_port->NUM_PORT);

                }break;



                case UART_FIFO_OVF:{

                    xQueueReset(uart_event);

                    uart_flush(current_port->NUM_PORT);

                }break;





                default :{
                    ESP_LOGE("UART_LIB", "tipo de evento : %s", event.type);
                }break;

            }

        }
    }
}
