
//drivers
#include<driver/uart.h>

//LOG ERROR
#include<esp_log.h>
#include<esp_err.h>

//propias
#include<uart_lib.h>



static const char *TAG = "UART";

/**
 * okay, mi punto es queter hacer una fyuncion base, pero la que si se tiene que hacer por defecto, seria UART 0, por lo que que si ahora quiero usar UART 1 o UART 2, entonces 
 * tan solo lo pasamos,pero en el caso necesario, para no estar recompilando 
 * 
 * 
 * 
*/

esp_err_t uart_init(uart_port_t sel_uart, int uart_baudrate, uart_word_length_t uart_data_length, uart_parity_t uart_parity, uart_stop_bits_t stop_bits, int RX, int TX){
    
    uart_config_t cfg={
        .baud_rate=uart_baudrate,
        .data_bits = uart_data_length,
        .parity = uart_parity,
        .stop_bits = stop_bits,
        .source_clk = UART_SCLK_DEFAULT,
        .flow_ctrl=UART_HW_FLOWCTRL_DISABLE,
    };

    esp_err_t ret;

    ret = uart_param_config(sel_uart, &cfg);

    if( ret != ESP_OK){
        ESP_LOGE(TAG, "estrucutra -> tipo de error: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = uart_set_pin(sel_uart, TX, RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    if(ret!=ESP_OK){
        ESP_LOGE(TAG, "tipo de error: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    /**
     * por ahora lo que entra por UART ingresa a la cola < uart_event > 
     * 
     * 
    */

    ret = uart_driver_install(sel_uart, BUFFER*2, BUFFER*2, 20, &uart_event, 0);

    if(ret!=ESP_OK){
        ESP_LOGE(TAG, "error: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    //si todo salio bien entonces mandamos ESP_OK
    return ESP_OK;
}






