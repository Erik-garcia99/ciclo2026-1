#include <stdio.h>
#include<freertos/FreeRTOS.h>
#include<freertos/task.h>
#include<freertos/event_groups.h>
#include<esp_system.h>
#include<esp_wifi.h>
#include<esp_event.h>
#include<esp_log.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>

/**
 * este programa creara el ESP como estacion, por lo que este podra conectarse a un acces point para transferir datos por ejemplo, quiero conectarmse a mi red local, por lo que como SSD y constrasenia es la del servicio de mi intertet, va a estar canijo para cunaod sea en UABC 
 * 
 * 
*/


//las credenciales de la red a la cual nos vamos a conectar / este puede ser como este caso WIFI comun o unn AP creado desde otra ESP 

#define EXAMPLE_ESP_WIFI_SSID "INFINITUMF4AF"
#define EXAMPLE_ESP_WIF_PASS "nFukH34MPW"
#define EXAMPLE_ESO_MAXIMUM_RETRY 5

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG ="wifi station";
static int s_rety_num=0;

void wifi_init_sta(void);

static void event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data);



void app_main(void)
{
    //guardamos los datos de wifi en la memoria no volarin para que inice sesion mas rapido 

    esp_err_t ret = nvs_flash_init();

    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init(); //vuelve a intentar la conexion 
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();


}

void wifi_init_sta(void){

    //ceramo el grupo de edventos que nos indicaran si se realizo la conexion o no -> esta parte es necesaria 
    s_wifi_event_group = xEventGroupCreate();

    //este proceso se conexion es basicamente la misma para cunado es eatacion o es acces point, la inicalizacion es igual

    ESP_ERROR_CHECK(esp_netif_init()); //funcion encargada de incializar el stack de red, 

    ESP_ERROR_CHECK(esp_event_loop_create_default()); //crea el manejador de eventos por default que lo que sirgan, se puede inicar por el usuario pero se tiene la opcion de que se cree por defecto cunado ocurra eventos sobre wifi en este caso, estos eventos se usan para comunica cambios de estado. 

    esp_netif_create_default_wifi_sta(); //este cambiara de sta -> ap en caso de ser un acces point
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //****************************************8 */
    //esta parte no es tan necesario, pero es informativa, se ejecuta cunado ocurre un evento relacionado con WIFI. 
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id)); //esta esta esperando cualquier evento
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip)); //este esta esperando especificamente que el ESP obtenga su direccion IP 

    /************************************** */
    wifi_config_t wifi_config = {
        .sta ={
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password=EXAMPLE_ESP_WIF_PASS,
            .threshold.authmode=WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi init_sta finashed");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);


    if(bits & WIFI_CONECTED_BIT){
        ESP_LOGI(TAG,"connecterd to ap SSID: %s - password: %s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIF_PASS);
    }
    else if(bits & WIFI_FAIL_BIT){
        ESP_LOGI(TAG,"failed to connect to ap SSID: %s - password: %s", EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIF_PASS);
    }
    else{
        ESP_LOGI(TAG, "evento sin explicacion");
    }


}


static void event_handler(void *args, esp_event_base_t event_base, int32_t event_id, void *event_data){


    //si es evento ocurrido es que el modo estacion comenzo entonces nos conectamos a widi
    if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){
        esp_wifi_connect();
    }
    //en otro caso intentara conectarse a la red, las veces que la macro indique. en el caso de no poder hacerlo indicara que ocurrio un error
    else if( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if(s_rety_num < EXAMPLE_ESO_MAXIMUM_RETRY){
            esp_wifi_connect();
            s_rety_num++;
            ESP_LOGI(TAG, "rety to connectec to te AP");
        }
        else{
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to th AP fail");
        
    }
    //antes de conectarse debe de adquirir una direccion IP este evento indicara que el ESP tiene una direccion IP privada, la cual se asigna en la estrucutra. 
    else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t *event=(ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_rety_num= 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONECTED_BIT);
    }

}