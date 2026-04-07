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
#include"modules/DHT11/esp32-dht11.h"

//macros 
/**
 * - vamos a necesitar 2 botones, 1 en donde cunado se presiona se publica 
 * - otro boton en que el que va a prender el led
 * 
 * - PIN_SEND es el pin que va tomar la accion de cunado publicar los estados en los topicos correspondientes
 * - PIN_TOGG_LED es el pin con que vamos a prender o apagar el led, es el 2do metodo le otro metodo es medinate la publicacion en node-red
 * 
 * 
 */
#define PIN_OUT 18
#define PIN_SEND 19
#define PIN_TOGG_LED 21
//DHT11
// PIN DE DATA 22
#define CONFIG_DHT11_PIN GPIO_NUM_22
#define CONFIG_CONNECTION_TIMEOUT 5



//buffer para enviar los datos leidos de los perifericos  
#define BUFFER 200

static const char *TAG = "<MAIN> NODE-RED_MQTT > ";

esp_mqtt_client_handle_t client= NULL;

//varibales globales que indican estados de presion del boton si se esta presionando, si se presiono correctamente y el estado del led
int LAST_PRESS;
int led_state = 0;
static volatile int press_handled_send = 0;   
static volatile int press_handled_led  = 0;

//estrucutra para el sensor 
dht11_t dht11_sensor;

//la cola por donde mandaremos la infromacion, esta cola es local asi que pues no hay comunicacion con otros archivos no hay falla. 
static QueueHandle_t data_queue;
QueueHandle_t flow_data;

//funcion inical para gpio
void GPIO_INIT(void);


//declaracion de tareas
//tarea que estar monitoreando si se presiono el btn

/**
 * @brief tarea dedicada a estar esperando que se presiono el btn para leer los estados de los sensores para mandar a publicar en sus 
 * respectivos topicos 
 * 
 * 
 */
void task_input(void *params);
/**
 * @brief tarea que esta esperando la cola con datos para publicar en los topicos
 * 
 */
void send_task(void *params);

/**
 * @brief tarea dedicada a estar mapenado en GPIO para saber cunado se presiono el BTN
 * 
 */
void gpio_task(void *params);


//tarea que escuhca la cola que trae la accion que prendera el led 
void task_input_topic(void *params);



//funciones 
//separamos los tokens recibidos
char **pasrse_input(char *line);


void app_main(void)
{
    //creamos la cola
    data_queue = xQueueCreate(10,sizeof(char)*BUFFER);
    //esta cola, sera una cola hacia un apuntador de tipo caracter
    flow_data = xQueueCreate(10, sizeof(char*)); //10 epacios para strings de 5 caracteres
    //inicamos GPIOS/drivers 
    
    ESP_ERROR_CHECK(set_adc(ADC_CHANNEL));
    GPIO_INIT();
    dht11_sensor.dht11_pin = CONFIG_DHT11_PIN;

    
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
    //despues de inicar me tendre que suscribir al topic 
    //suscribiendonos en el topico 
    // esp_mqtt_client_subscribe(client,TOPIC_ACT,0 );

    //tarea encargada de estar monitoreando si es que producjo un botonzo que envie la infromacion a los diferentes topicos 
    xTaskCreate(task_input, "task_input", 4096, NULL, 9,NULL);
    xTaskCreate(send_task, "send_task", 4096, NULL, 8, NULL);
    xTaskCreate(gpio_task, "gpio_task", 1024, NULL, 5, NULL);
    xTaskCreate(task_input_topic, "task_input_topic", 4096, NULL, 7, NULL);
}


void GPIO_INIT(void){

    gpio_reset_pin(PIN_OUT);
    gpio_set_direction(PIN_OUT, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_OUT, 0);
    //lo prendemos con un boton

    //pin para publicar 
    gpio_reset_pin(PIN_SEND);
    gpio_set_direction(PIN_SEND, GPIO_MODE_DEF_INPUT);

    gpio_pullup_dis(PIN_SEND);
    gpio_pulldown_en(PIN_SEND);

    //pin para prender o apagar el led
    gpio_reset_pin(PIN_TOGG_LED);
    gpio_set_direction(PIN_TOGG_LED, GPIO_MODE_DEF_INPUT);
    
    gpio_pullup_dis(PIN_TOGG_LED);
    gpio_pulldown_en(PIN_TOGG_LED);




}



void task_input(void *params){

    while(1){
        int current_level = gpio_get_level(PIN_SEND);

        if(current_level == 1 && press_handled_send == 0){

            //pequenio antirebote para verificar que sea un shortpress
            vTaskDelay(pdMS_TO_TICKS(50));

            //si es asi entonces procede a recpilar los datos para podere vniarlos
            if(gpio_get_level(PIN_SEND) == 1){

                char value_adc[6];
                char current_state_led[2];
                char temp[10];
                char hum[10];

                //traemos un valor de ADC
                int read_ADC = read_adc(ADC_CHANNEL);
                //el led solo va a cambiar cunado se publique en el topico en el que este led esta suscrito 
                //lectura del sentor 

                if(!dht11_read(&dht11_sensor, CONFIG_CONNECTION_TIMEOUT)){  
                
                    snprintf(temp, sizeof(temp), "%.2f", dht11_sensor.temperature);
                    snprintf(hum, sizeof(hum), "%.2f", dht11_sensor.humidity);

                }
                else{
                    // Lectura fallida: poner valores predeterminados
                    snprintf(temp, sizeof(temp), "0.0");
                    snprintf(hum, sizeof(hum), "0.0");
                }
                snprintf(value_adc, sizeof(value_adc), "%d", read_ADC);
                snprintf(current_state_led, sizeof(current_state_led), "%d", led_state);

                ESP_LOGI(TAG, "DEBUG : STATE-LED: %s",current_state_led);
                ESP_LOGI(TAG, "DEBUB: value ADC : %s", value_adc);
                ESP_LOGI(TAG, "DEBUG : Temp: %s",temp);
                ESP_LOGI(TAG, "DEBUB: Humedad: %s", hum);

                char format_values[BUFFER];

                snprintf(format_values, sizeof(format_values), "%s:%s:%s:%s",current_state_led, value_adc,temp,hum);

                xQueueSend(data_queue, &format_values, portMAX_DELAY);
                press_handled_send = 1;
            }
        }
        // cuando sueltas el botón reseteamos la bandera
        if(current_level == 0){
            press_handled_send = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}



void send_task(void *params){

    char buffer[BUFFER];

    while(1){
        if(xQueueReceive(data_queue, (void*)buffer, portMAX_DELAY)){

            char **tokens = pasrse_input(buffer);

            //perimot lo que ingresa lo primero en el string es LED:ADC:HUM:TMP

            esp_mqtt_client_publish(client, TOPIC_LED, tokens[0], 0, 1, 0);
            esp_mqtt_client_publish(client, TOPIC_ADC, tokens[1], 0, 1, 0);
            esp_mqtt_client_publish(client, TOPIC_TEMP, tokens[2], 0, 1, 0);
            esp_mqtt_client_publish(client, TOPIC_HUM, tokens[3], 0, 1, 0);
        }
    }

}

void gpio_task(void *params){

    while(1){
        int current_level = gpio_get_level(PIN_TOGG_LED);

        if(current_level == 1 && press_handled_led == 0){

            //pequenio antirebote para verificar que sea un shortpress
            vTaskDelay(pdMS_TO_TICKS(50));

            //si es asi entonces procede a recpilar los datos para podere vniarlos
            if(gpio_get_level(PIN_TOGG_LED) == 1){
                led_state = !led_state;              
                gpio_set_level(PIN_OUT, led_state);
                press_handled_led = 1;
            }
        }

        // cuando sueltas el botón reseteamos la bandera
        if(current_level == 0){
            press_handled_led = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


//necesitaria una tarea que este esuchando que este esucnahdno cunado se reciba datos del lado del servidor 



void task_input_topic(void *params){

    char *action;

    while(1){
        if(xQueueReceive(flow_data, &action, portMAX_DELAY)){
            char **tokens = pasrse_input(action); //separamos en sus diferentes tokens             

            ESP_LOGI(TAG, "topic: token[0] = [%s]", tokens[0]);
            ESP_LOGI(TAG, "payload: token[1] = [%s]", tokens[1]);

            if(!(strcmp("ON", tokens[1]))){
                gpio_set_level(PIN_OUT, 1);
                led_state = 1;
            }
            else if(!(strcmp("OFF", tokens[1]))){
                gpio_set_level(PIN_OUT, 0);
                led_state=0;
            }   

            //liberandi memoria
             for(int i = 0; tokens[i] != NULL; i++){
                free(tokens[i]);
            }
            free(tokens);
            // Liberar el mensaje recibido
            free(action);

        }

        vTaskDelay(pdMS_TO_TICKS(10));
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

        token = strtok(NULL, ":");
    }

    tokens[position] = NULL;


    return tokens;
}