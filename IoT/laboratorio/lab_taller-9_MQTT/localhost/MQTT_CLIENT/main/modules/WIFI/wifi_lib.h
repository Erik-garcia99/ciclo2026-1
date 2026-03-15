#ifndef WIFI_LIB_H
#define WIFI_LIB_H

#include <freertos/FreeRTOS.h>      
#include <freertos/event_groups.h>  
#include <esp_event.h>

//prototipos
void wifi_init_sta(void);

//macros 
#define ESP_WIFI_SSID "INFINITUMF4AF"
#define ESP_WIFI_PASS "nFukH34MPW"
#define ESP_MAX_RETRY 5 //intentara conectarse 5 veces antes de arrojar un error de conexion

//bits 
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1




#endif