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
#ifndef PWM_LIB_H
#define PWM_LIB_H

#include "driver/ledc.h"
#include "esp_err.h"


#define PWM_FREQ_HZ     5000        // frecuencia en Hz
#define PWM_RESOLUTION  LEDC_TIMER_13_BIT   // 0–8191
#define PWM_CHANNEL     LEDC_CHANNEL_0
#define PWM_TIMER       LEDC_TIMER_0


/**
 * @brief Inicializa el canal LEDC y arranca el PWM.
 *        Llamar una sola vez al inicio.
 */
esp_err_t pwm_init(void);

/**
 * @brief Cambia el duty cycle.
 * @param duty  Valor entre 0 y (2^resolución - 1).
 *              Con 13 bits: 0 = 0%, 8191 = 100%
 */
esp_err_t pwm_set_duty(uint16_t duty);

/**
 * @brief Lee el duty cycle actual.
 * @return valor raw del duty
 */
uint16_t pwm_get_duty(void);

#endif