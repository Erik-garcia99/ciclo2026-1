#ifndef WIFI_LIB_H
#define WIFI_LIB_H

#include <freertos/FreeRTOS.h>      
#include <freertos/event_groups.h>  
#include <esp_event.h>


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define ESP_WIFI_SSID "INFINITUMF4AF"
#define ESP_WIFI_PASS "nFukH34MPW"
#define ESP_MAX_RETRY 5 

void wifi_setup_init();




#endif