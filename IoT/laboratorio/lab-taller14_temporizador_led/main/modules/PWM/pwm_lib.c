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

#include "pwm_lib.h"
#include "esp_log.h"
#include"global.h"

static const char *TAG = "PWM_LIB";

esp_err_t pwm_init(void)
{
    // ── 1. Configura el timer ────────────────────────────────────────────────
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num       = PWM_TIMER,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "error timer: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    // ── 2. Configura el canal y el GPIO ─────────────────────────────────────
    ledc_channel_config_t channel_cfg = {
        .gpio_num   = PWM_LED,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = PWM_CHANNEL,
        .timer_sel  = PWM_TIMER,
        .duty       = 0,        // empieza en 0%
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&channel_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "error canal: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    // Desde aquí el PWM corre solo en hardware — no necesita atención del CPU
    ESP_LOGI(TAG, "PWM init OK  |  GPIO %d  |  %d Hz", PWM_LED, PWM_FREQ_HZ);
    return ESP_OK;
}

esp_err_t pwm_set_duty(uint16_t duty)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL, (uint32_t)duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
    return ESP_OK;
}

uint16_t pwm_get_duty(void)
{
    return (uint16_t)ledc_get_duty(LEDC_LOW_SPEED_MODE, PWM_CHANNEL);
}