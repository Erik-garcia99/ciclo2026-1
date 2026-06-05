//hagamos solo recibir infromacion, no se, algun dato que no pase de los 32 bits, 

#include <stdio.h>



#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/queue.h>

#include<esp_log.h>
#include<esp_event.h>

#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>



#include"modules/WIFI/wifi_lib.h"
#include"modules/TCP/tcp_lib.h"
#include"global.h"

static const char *TAG = "MAIN-TCP_CLIENT";




//+++++++++++++++++++++++++++++colas 
QueueHandle_t queue_recv_send;

//+++++++++++++++++handles 

TaskHandle_t handle_task_recv, hande_task_keep_alive;

//++++++++++++++++++++++++++++grupo de eventos 

//++++++++++++++++++++estrucutras 
format_request_t format_request;
send_info_t send_info;
tcp_client_t tcp_client;

//++++++++++++++++enums 
op_type_t op_type;
action_t action;
resourse_t resourse;


//++++++++++++++++++++++++varibales
uint8_t led_state;


//+++++++++++++++++ funciones 

esp_err_t tcp_setup();





void app_main(void)
{

    //+++ creamos un grupo de eventos 
    g_tcp_event_group = xEventGroupCreate();

    //+++++++++creamos colas 

    queue_recv_send = xQueueCreate(10, sizeof(format_request_t));

    //inciamos las vaibrles de la estrucutr a
    tcp_client.sock = -1;
    tcp_client.connected =0;
    tcp_client.logged_in = 0;



    //------------------------------ inicamos wifi
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_MODE_STA");
    wifi_init_sta();
    //-----------------------------------

    //una vez que se conecto al WIFI entonces lo que hacemos es tratar que establecer conexion con el servidor 

    //creamos una cola en donde se staran eviando los datos que recibimos 

    


    ret = tcp_setup(); //establecemos la conexion e inicamos las tareas necesarias 


    


    while(1); //si se puede inciar solo se mantiene esperando 





}


esp_err_t tcp_setup(){

    
    EventBits_t bits;
    do{
        //en mi programa tengo un while(1), eso pues funciona, pero creo que no deberia al final no es una tarea al final esto se puede romper
        //por lo que al menos se ejecuta 1 vez por lo que eleigmos mejor un cilo do-while

        //inicamos el intneto
        esp_err_t ret = tcp_client_init();

        if(ret == ESP_OK){
            //acibamos
            //creamos la tarea para empezar a recibir 
            xTaskCreate(task_recv, "task_recv",4096, NULL, 10, &handle_task_recv);
            xTaskCreate(task_keep_alive, "task_keep_alive", 2098, NULL, 6, &hande_task_keep_alive); //para la tarea que envia el keepalive. 




            bits = xEventGroupWaitBits(g_tcp_event_group, TCP_DISCONNECTED, pdTRUE, pdFALSE, portMAX(5000));




            if(bits & TCP_DISCONNECTED){

                //intentamos reconectar 
                vTaskDelete(handle_task_recv);
                vTaskDelete(hande_task_keep_alive);
                tcp_setup(); //hacemos una llamada recursiva y se vuelve a intentar a relaizar la conexion 

                
            }




        }
        else{

            ESP_LOGE(TAG, "no se pudo establecer comunicacion\n realizar otro intento");
            vTaskDelay(pdTICKS_TO_MS(100));
            continue;
        }


    }while();

}