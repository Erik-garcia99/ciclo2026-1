#ifndef HTTPS_LIB_H
#define HTTPS_LIB_H


#include<freertos/FreeRTOS.h>
#include<freertos/queue.h>

extern QueueHandle_t queue_ADC;


void http_test_task(void *pvParameters);





#endif