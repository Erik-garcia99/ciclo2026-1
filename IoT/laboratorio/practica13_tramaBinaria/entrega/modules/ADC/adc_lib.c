
#include "adc_lib.h"
#include "esp_log.h"
#include"global.h"

static const char *TAG = "ADC_LIB";
static adc_oneshot_unit_handle_t adc_handle = NULL;

esp_err_t set_adc(adc_channel_t channel)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "error init: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_oneshot_config_channel(adc_handle, channel, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "error config canal: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}

uint16_t read_adc(adc_channel_t channel)
{
    int raw = 0;
    adc_oneshot_read(adc_handle, channel, &raw);
    return raw;
}