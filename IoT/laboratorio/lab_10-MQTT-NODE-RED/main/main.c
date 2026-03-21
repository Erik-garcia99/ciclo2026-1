#include <stdio.h>
#include<stdlib.h>


//enviaremos la publicacion al broker del servidor
//freeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>
//drivers
#include<driver/gpio.h>
#include<driver/adc.h>

#include<esp_timer.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

#include<esp_log.h>
// #include<esp_err.h>

//librerias propias

#include"modules/WIFI/wifi_lib.h"
#include"modules/MQTT/mqtt_lib.h"
#include"modules/ADC/adc_lib.h"


//macros 
#define PIN_OUT 18
#define PIN_IN 19

//buffer para enviar los datos leidos de los perifericos  
#define BUFFER 200

static const char *TAG = "NODE-RED_MQTT > ";

esp_mqtt_client_handle_t client= NULL;

int LAST_PRESS;
static int led_state = 0;
static int last_press_handled = 0;

//la cola por donde mandaremos la infromacion, esta cola es local asi que pues no hay comunicacion con otros archivos no hay falla. 
static QueueHandle_t data_queue;


//funcion inical para gpio
void GPIO_INIT(void);


//declaracion de tareas
//tarea que estar monitoreando si se presiono el btn
void task_input(void *params);
void send_task(void *params);
//separamos los tokens recibidos
char **pasrse_input(char *line);


void app_main(void)
{
    //inicamos GPIOS/drivers 
    
    ESP_ERROR_CHECK(set_adc(ADC_CHANNEL));
    GPIO_INIT();


    //conectamos WIFI
     esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();

    //inicamos la conexion MQTT
    mqtt_start();

    //tarea encargada de estar monitoreando si es que producjo un botonzo que envie la infromacion a los diferentes topicos 
    xTaskCreate(task_input, "task_input", 2048, NULL, 9,NULL);
    xTaskCreate(send_task, "send_task", 2048, NULL, 8, NULL);



}


void GPIO_INIT(void){

    gpio_reset_pin(PIN_OUT);
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_OUT, 0);
    //lo prendemos con un boton

    gpio_reset_pin(PIN_IN);
    gpio_set_direction(PIN_IN, GPIO_MODE_DEF_INPUT);

    gpio_pullup_dis(PIN_IN);
    gpio_pulldown_en(PIN_IN);

}



void task_input(void *params){

    while(1){
        int current_level = gpio_get_level(19);

        if(current_level == 1 && last_press_handled == 0){

            //pequenio antirebote para verificar que sea un shortpress
            vTaskDelay(pdMS_TO_TICKS(50));

            //si es asi entonces procede a recpilar los datos para podere vniarlos
            if(gpio_get_level(19) == 1){

                // // toggle del estado
                // led_state = !led_state;
                // gpio_set_level(18, led_state);

                // // publica el estado
                // if(led_state){
                //     char dataON[]="LED ON : 01275863";
                //     ESP_LOGI(TAG, "LED ON");
                //     xQueueSend(data_queue,dataON, portMAX_DELAY);
                // } else {
                //     char dataOFF[]="LED OFF : 01275863";
                //     ESP_LOGI(TAG, "LED OFF");
                //     xQueueSend(data_queue,dataOFF, portMAX_DELAY);
                // }


                //traemos la lectrua del led 
                /**
                 * aqui pudieran haber 2 posibilades solo mandar el led esperando que desde el otro lado lo cambien  
                 * creo que lo vamos a dejar a que del otro lado le diga que quiere prenderlo o apagarlo 
                 * 
                 * 
                */

                //traemos un valor de ADC
                int read_ADC = read_adc(ADC_CHANNEL);
                
                int state_LED = gpio_get_level(PIN_OUT);

                char value_adc[6];
                char current_state_led[2];

                snprintf(value_adc, sizeof(value_adc), "%d", read_ADC);
                snprintf(current_state_led, sizeof(current_state_led), "%d", state_LED);

                //nos falta la parte del otro periferico pero no lo tengo xd, de perdida con este 

                char format_values[BUFFER];


                //estas estaran serpadas por :

                snprintf(format_values, sizeof(format_values), "%s:%s",current_state_led, value_adc );

                xQueueSend(data_queue, &format_values, portMAX_DELAY);

                



                last_press_handled = 1;
            }
        }

        // cuando sueltas el botón reseteamos la bandera
        if(current_level == 0){
            last_press_handled = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



void send_task(void *params){

    char buffer[BUFFER];

    while(1){
        if(xQueueReceive(data_queue, (void*)buffer, portMAX_DELAY)){

            
            char *topics[2]={TOPIC_LED, TOPIC_ADC};
            //ahora lo que tenemos que hacer es separar el dato que se recibio 

            ESP_LOGI(TAG, "recibio cola: %s", buffer);
            // esp_mqtt_client_publish(client, TOPIC, buffer, 0, 1, 0);
        }
    }

}



char **pasrse_input(char *line){

    char **tokens = malloc(5 * sizeof(char*));
    char *token;
    int position=0;


    token = strtok(line, ":");

    while(token != NULL){

        if(position >= 5 ) break;

        tokens[position++] = strdup(token);

        token = strtok(NULL, " ");
    }

    tokens[position] = NULL;


    return tokens;
}