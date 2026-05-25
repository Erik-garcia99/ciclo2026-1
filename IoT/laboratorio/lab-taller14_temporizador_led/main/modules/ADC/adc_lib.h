/**
 * 
 * @author erik garcia chavez 
 * @date 2026-06-10
 * ingenieira en computacion
 * UABC 
 * internet de las cosas 
 * 
 * 
*/
#ifndef ADC_LIB_H
#define ADC_LIB_H

#include "esp_adc/adc_oneshot.h"
#include<global.h>
// #define ADC_CHANNEL ADC_CHANNEL_4

esp_err_t set_adc(adc_channel_t channel);
uint16_t read_adc(adc_channel_t channel);

#endif