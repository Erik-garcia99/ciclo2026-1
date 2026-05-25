/**
 * 
 * @author erik garcia chavez 
 * @date 2026-06-10
 * ingenieira en computacion
 * UABC 
 * internet de las cosas 
 * 
 * 
*/
#ifndef WIFI_H
#define WIFI_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_event.h>
#include<esp_err.h>

//macros
#define ESP_MAX_RETRY 5 //intentara conectarse 5 veces antes de arrojar un error de conexion

//bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_UPDATE BIT10


//variables globales para red 
/**
 * 
 * 
 * 
*/
// extern char *ESP_SSID_WIFI;
// extern char *ESP_PSWD_WIFI;


// //para una red de empresa o en este caso para la uni
// extern int ENT_NETWORK;
// //pero este lo dejaremos hasta el ultimo
// extern char *SSID_ENT_WIFI;
// extern char *USER_ENT_WIFI;
// extern char *PSWD_ENT_WIFI;

extern EventGroupHandle_t s_wifi_event_group;

//podemos cambiar estos datos anteriores por esta nuva estrucutra?, podriamos hacer lago parecido con TCP, para que todo este en una estrucutra agrupado y mas limpio?

/**
 * 
 * @brief esta estrucutura guaradaremos infromacion de la conexion o del estado de la conexion 
 * 
 * ssid - nombre de la red a coenctarse @memberof esp_wifi_t
 * pswd - contrasenia de la red o de la cuenta @memberof esp_wifi_t   
 * user_name - usuario de la red para inicar sesion y conectarse a la red @memberof esp_wifi_t  
 * ip - ip otorgada al dispositivo por parte de la red @memberof esp_wifi_t 
 * type_connected - valor de 0 o 1, 0 - conexion en una red normal, 1- conexion en una red de empresa @memberof esp_wifi_t 
 * connected - 1 o 0 indicando que se pudo realizar la conexion y se eucneutra coinectado el ESP @memberof esp_wifi_t
 * 
*/
typedef struct{
    char *ssid;
    char *pswd;
    char *user_name;
    esp_ip4_addr_t *ip;
    int type_connected;
    int connected;
}esp_wifi_t;

extern esp_wifi_t esp_wifi;




//creo que lo ideal seria una cola que traiga

/**
 * ahora la cosa estara en como se ingresan las credencilaes para la conexion de WIFI?
 *  esto solo sera visto por lo que tenga que ver con WIFI. 
 * 
 * por lo que debe de ser una varibale, que al inicio la definiremos como el WIFI de la casa, 
 * 
 * !!!IMPORTANT 
 * 
 * como conectarnos desde la red de UABC, deberiamos de desarrollar la forma de poner conectarnos una red de empresa, se puede pero el ESP primero va atratar de conectarse a la 
 * red que el ya conoce, al no lograrlo tendra la opcion de elegir nueva red o la de empresa < primero vamos a hacer una red nomral con su SSID y pasword > 
 * 
 * 
 * 
 * 
*/



//prototipos
void wifi_init_sta(void);

/**
 * @brief estblecera las credeniclaes de la red, en el cao que se requiera realizar de nuevo  
 * 
 * @param SSID un apuntador al nombre de la red 
 * @param PSDW un apuntador a la contrasenia de la red
 * 
 * @return ESP_OK indica que se pudo realizar el cambio de credenciales 
 * @return ESP_FAIL no se pudo estabelcer la credenciales 
 * 
 * @details lo que hara la funcion es, este va a recibir 2 datos, por lo que estos datos seran copiados en las varibales globales de SSID y PSWD que son los que se estaran usando
 * para conectarse a la red. 
 * 
 * 
*/

/**
 * @brief desconecta y vuelve a conectar con las credenciales actuales en esp_wifi.
 *        llamar despues de actualizar esp_wifi.ssid / pswd.
 */
void wifi_reconnect(void);



#endif