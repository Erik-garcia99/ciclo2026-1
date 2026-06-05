#include <stdio.h>
#include<esp_log.h>

#include "nvs_flash.h"
#include "esp_event.h"

#include<mqtt_client.h>
#include<modules/MQTT/mqtt_lib.h>



esp_mqtt_client_handle_t client;
QueueHandle_t flow_data;

void app_main(void)
{

    flow_data = xQueueCreate(10, sizeof(char *));

}
