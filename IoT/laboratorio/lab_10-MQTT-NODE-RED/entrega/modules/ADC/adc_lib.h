#ifndef ADC_LIB_H
#define ADC_LIB_H

#include<esp_log.h>
#include<driver/adc.h>


#define ADC_CHANNEL ADC1_CHANNEL_4



/**
 * 
 * @brief configuracion del canal de ADC1 a leer
 * 
 * @param canal de ADC1 pueden ser 0 - 3 - 6 - 7 - 4 - 5
 * 
 * @return ESP_OK si se confura de manrea correcta 
 * @return ESP_FAIL indicando que fallo en alguna configuracion 
 * 
 * 
 */
esp_err_t set_adc(adc1_channel_t channel);

int read_adc(adc1_channel_t channel);

#endif