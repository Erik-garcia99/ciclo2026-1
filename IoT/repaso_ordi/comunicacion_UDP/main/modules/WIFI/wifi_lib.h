#ifndef WIFI_LIB_H
#define WIFI_LIB_H

#include <freertos/FreeRTOS.h>      
#include <freertos/event_groups.h>  
#include <esp_event.h>



//macros 
/**
 * se pide que se pueda cambiar las SSID, IP's, etc.. en el caso de wifi, 
 * 
 * -> dejaremos estas credenciales
 * 
 * -> pero ahora cunado el ESP no se pueda conectar a la red que tiene guardada en memoeria, entonces dira  que no se puede conectar
 * 
 * -> entonces por medio de UART pedira la nueca SSID y la nueva password necesario para una red disponible 
 *      --> como ya tenemos un evento que nos indica que no se pudo conectar a WIFI, entonces lo que hacemos ahora seria pedir las nuevas credenciales y volver a intentar conectarnos
 * 
 * --> estas macros podriamos dejarlas pero seria necesario que la red de conectate a varibales, porque en el caso de no poder conectarnos la red sera necesario ingresar 
 *      las nuevas credecniales < SSID y pswd > de la nueva red.
 * 
 * 
*/
/**
 * 
 * se modificaron los nombres de las macros
 */
#define ESP_WIFI_SSID "INFINITUMF4AF"
#define ESP_WIFI_PASS "nFukH34MPW"

#define ESP_MAX_RETRY 5 //intentara conectarse 5 veces antes de arrojar un error de conexion

//bits 
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1


//prototipos
void wifi_init_sta(void);



#endif