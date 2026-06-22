#ifndef WIFI_LIB_H
#define WIFI_LIB_H


#include<esp_wifi.h>


#define ESP_MAX_RETRY 5 

//creamos un grupo de eventos para comunicar que un proceso se completo en este caso si se conecto o no al wifi o si hay un update
//en las credeniclaes 
extern EventGroupHandle_t g_EVENT_WIFI;


/**
 * agrupamos la infromacion sobre la conexion WIFI en esta estrucutra para poder tener mas orden sobre el flujo y no tener todo regado 
 * la razon porque SSID y PSWD es porque son un string y mejor usamos un apuntador. porque no sabemos realmente de que tamanio seria el 
 * SSID y el PSWD. 
 * 
 */
typedef struct{
    char *esp_ssid;
    char *esp_pswd;
    esp_ip4_addr_t *ip;
    //este connected servirea para verificar las conexones al inciio 
    int connected;
}esp_wifi_t;

extern esp_wifi_t esp_wifi;


void wifi_init_sta();



#endif