#include"adc_lib.h"
#include<esp_log.h>

static const char* TAG ="ADC_LIB";


esp_err_t set_adc(adc1_channel_t channel){


    esp_err_t ret;

    ret = adc1_config_channel_atten(channel,ADC_ATTEN_DB_12);

    if(ret != ESP_OK){
        ESP_LOGI(TAG, "error: %s",esp_err_to_name(ret));
        return ESP_FAIL;
    }
    //12 bits de resolucion 
    ret = adc1_config_width(ADC_WIDTH_BIT_12);
    
    if(ret != ESP_OK){
        ESP_LOGI(TAG, "error: %s",esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}



int read_adc(adc1_channel_t channel){

    int read = adc1_get_raw(channel);

    return read;

}