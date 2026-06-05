
#include <freertos/FreeRTOS.h> 
#include<freertos/task.h>
#include<freertos/event_groups.h>


#include<esp_log.h>
#include<esp_event.h>


#include<esp_wifi.h>
#include<nvs_flash.h>
#include<lwip/err.h>
#include<lwip/sys.h>



#include"wifi_lib.h"

static void wifi_event_handler(void *args, esp_event_base_t event_base,int32_t event_id, void *event_data);

static EventGroupHandle_t g_wifi_event_group;


static void wifi_event_handler(void *args, esp_event_base_t event_base,int32_t event_id, void *event_data){



}


void wifi_setup_init(){

    g_wifi_event_group = xEventGroupCreate();

}





