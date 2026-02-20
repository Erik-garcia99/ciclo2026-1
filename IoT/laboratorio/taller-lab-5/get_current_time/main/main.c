//librerias estans 
#include <stdio.h>

//FreeRTOS
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/event_groups.h>

//ESP LOGS
#include<esp_log.h>
#include<esp_event.h>

//wifi
#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

//HTTP
#include<esp_http_client.h>


//macros 
#define ESP_WIFI_SSID "INFINITUMF4AF"
#define ESP_WIFI_PASS "nFukH34MPW"
#define ESP_MAX_RETRY 5 //intentara conectarse 5 veces antes de arrojar un error de conexion

//bits 
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1


//tareas

//prototipos
void wifi_init_sta(void);
static void wifi_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data);

void get_time(char *JSON);



// globales
static const char *TAG = "get time / tijuana";
static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;





void app_main(void)
{
    //inicializacno NVS 

    esp_err_t ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG,"ESP_MODE_STA");
    wifi_init_sta();

    while(1){
        char *JSON=NULL;

        get_time(JSON);
        vTaskDelay(pdMS_TO_TICKS(60000));
    }

}


void wifi_init_sta(void){


    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    //configutracion por defecto de WIFI
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //vamos a registrar los eventos que pasen cuando se intente conectar al WIFI y el importantes que el ESP obtenga una IP para poder conextarse a internet
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    //esta es la estrucutura donde esta infromacion basica de nuestro internet
    wifi_config_t wifi_config ={
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS,
            .threshold.authmode=WIFI_AUTH_WPA2_PSK,
        },
    };


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); 

    ESP_LOGI(TAG, "conectandose a WIFI...");

    //avtivamos el bit correspondiente si es que se conecta o falla 

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    if(bits & WIFI_CONNECTED_BIT){
        ESP_LOGI(TAG, "WIFI conectado con exito!");
    }
    else{
        ESP_LOGE(TAG, "Fallo la conexion WIFI!!");
    }

}


static void wifi_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data){

    //un evento el cual la estacion se inicio 
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    }
    else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        //intentara conectarse de nuevo
        if(s_retry_num < ESP_MAX_RETRY){
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG,"reitentando conexion de WIFI.. (intento: %d/ de: %d)", s_retry_num, ESP_MAX_RETRY);
        }
        else{
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "No se pudo conectar al WIFI");
        }
    }
    else if(event_base ==IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event=(ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP obtenida: "IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }



}   


// necesario la certificacion porqeu es un metodo HTTPS por lo que maniana le intentamos a ver que onda. 

void get_time(char *JSON){

    //peticion a la PAPI que me de el JSON con la infromacion de la hora en tijuana 
    esp_http_client_config_t config = {
        .url = "https://time.now/developer/api/timezone/America/Tijuana",
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t cliente = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(cliente);

    if(err == ESP_OK){
        //quiere decir que la solicitud se pudo hacer 
        int status = esp_http_client_get_status_code(cliente);
        int content_length = esp_http_client_get_content_length(cliente);
        ESP_LOGI(TAG, "HTTP status: %d, tamanio de la respuesta: %d", status, content_length);


    }
    else{
        ESP_LOGE(TAG, "error en la solicitud: %d", esp_err_to_name(err));
    }

    esp_http_client_cleanup(cliente);
}