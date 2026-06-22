#include<string.h>


#include <freertos/FreeRTOS.h> 
#include<freertos/task.h>
#include<freertos/event_groups.h>


#include<driver/uart.h>

#include<esp_log.h>
#include<esp_event.h>
#include <esp_err.h>

#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

//librerias propias 
#include"wifi_lib.h"
#include"global.h"

static void wifi_event_handler(void *args, esp_event_base_t event_base,int32_t event_id, void *event_data);


//varibales globales 
static int s_retry_num;
// EventGroupHandle_t s_wifi_event_group;
static const char *TAG="wifi_lib";


void wifi_init_sta(void){



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

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start()); 

    wifi_config_t wifi_config;
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_config) == ESP_OK &&
        strlen((char*)wifi_config.sta.ssid) > 0)
    {
        // Liberar memoria previa (por si acaso)
        free(esp_wifi.esp_ssid);
        free(esp_wifi.esp_pswd);
        esp_wifi.esp_ssid = strdup((char*)wifi_config.sta.ssid);
        esp_wifi.esp_pswd = strdup((char*)wifi_config.sta.password);
        ESP_LOGI(TAG, "Credenciales cargadas desde NVS: %s", esp_wifi.esp_ssid);
    }
    else
    {
        ESP_LOGW(TAG, "No hay credenciales WiFi en NVS");
        esp_wifi.esp_ssid = NULL;
        esp_wifi.esp_pswd = NULL;
    }
    

    //ahora viene lo bueno. 
    EventBits_t bits;
    //indicamos que si la conexion es diferente a 1 (diferente a una conexion de emprea) entonces entra aqui
    xEventGroupClearBits(g_EVENT_WIFI, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT | WIFI_UPDATE);
    s_retry_num = 0; 
    
    //esperamos que se hayan ingresado datos a las vairbales de ssid y pswd
    while (esp_wifi.esp_ssid == NULL || esp_wifi.esp_pswd == NULL ||
        strlen(esp_wifi.esp_ssid) == 0 || strlen(esp_wifi.esp_pswd) == 0) {
        ESP_LOGW(TAG, "No hay credenciales WiFi. Esperando comando...");
        xEventGroupWaitBits(g_EVENT_WIFI, WIFI_UPDATE, pdTRUE, pdFALSE, portMAX_DELAY);
    }

    //creo que seria con un DO-while que lo este intentando hasta se logre conectar con exito 
    do{
        wifi_config_t wifi_config = {
            .sta = {
                .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            },
        };

        strncpy((char*)wifi_config.sta.ssid,     esp_wifi.esp_ssid, sizeof(wifi_config.sta.ssid)-1);
        strncpy((char*)wifi_config.sta.password, esp_wifi.esp_pswd, sizeof(wifi_config.sta.password)-1);

            // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

            ////poner esta parte dentro del do-while, para que lo intente al menos 1 vez  hasta que entre correctamnte
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            
        esp_wifi_connect();
            
        ESP_LOGI(TAG, "conectandose a WIFI...");

            //avtivamos el bit correspondiente si es que se conecta o falla 

            /**
             * hasta este punto se supone va a esperar lo que pase si es que se logro conectar o no 
             * 
            */
        bits = xEventGroupWaitBits(g_EVENT_WIFI, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
            

        if(bits & WIFI_FAIL_BIT){
            const char *mssg_error = "fallo al establecer conexion con las credenciales actuales\n\0";
            uart_write_bytes(UART_MAIN, UART_RED, strlen(UART_RED));
            uart_write_bytes(UART_MAIN, mssg_error, strlen(mssg_error));
            uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
            //si fallo la conexion espera a que se introduza las nuevas credencilaes 
            xEventGroupWaitBits(g_EVENT_WIFI, WIFI_UPDATE, pdTRUE,pdTRUE, portMAX_DELAY);
            s_retry_num = 0; 


        }
    }while(!(bits & WIFI_CONNECTED_BIT)); //el programa sigue hasta el el bit que indica que se conecto no este activo 
    esp_wifi.connected =1;
    const char *mssg_error = "conexion exitoso\n\0";
    uart_write_bytes(UART_MAIN, UART_GREEN, strlen(UART_GREEN));
    uart_write_bytes(UART_MAIN, mssg_error, strlen(mssg_error));
    uart_write_bytes(UART_MAIN, UART_RESET, strlen(UART_RESET));
}




static void wifi_event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data){

    //un evento el cual la estacion se inicio 
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        // esp_wifi_connect();
    }
    else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        //intentara conectarse de nuevo
        if(s_retry_num < ESP_MAX_RETRY){
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG,"reitentando conexion de WIFI.. (intento: %d/ de: %d)", s_retry_num, ESP_MAX_RETRY);
        }
        else{
            xEventGroupSetBits(g_EVENT_WIFI, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "No se pudo conectar al WIFI");
        }
    }
    else if(event_base ==IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event=(ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "IP obtenida: "IPSTR, IP2STR(&event->ip_info.ip));
        esp_wifi.ip = &event->ip_info.ip;
        s_retry_num = 0;
        xEventGroupSetBits(g_EVENT_WIFI, WIFI_CONNECTED_BIT);
    }

}   


//desconecta y vuelve a conectar con las credenciales, que ya estane en el ssi y el pswd en el momento 
void wifi_reconnect(void){

    ESP_LOGI(TAG, "reconectando WIFI");
    s_retry_num =0;
    esp_wifi.connected =1;

    //descoenctar la sesion actual, 
    esp_wifi_disconnect();
    //conectar con las nuvas credencuales. 
    wifi_init_sta();
}
