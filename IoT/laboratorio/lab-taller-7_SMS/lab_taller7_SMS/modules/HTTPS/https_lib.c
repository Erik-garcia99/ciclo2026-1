/**
 * la API key no es necesario por lo que debemos de cambar varias cosas porque no necesitamos tanta basura
 * 
 * 
 */



//libereias etandar
#include <string.h>
#include <stdlib.h>

//freeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

//logs errors
#include "esp_log.h"
#include "esp_system.h"
//wifi / HTTP
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_http_client.h"

//certificados
#include "esp_crt_bundle.h"
//libereias 
#include "https_lib.h"

#define MAX_HTTP_RECV_BUFFER    1024
#define MAX_HTTP_OUTPUT_BUFFER  2048


//-------------------------------
static const char *TAG  = "HTTP_CLIENT Handler";
static const char *TAG3 = "Sending sendMessage";
static const char *TAG4 = "HTTPS_LIB";


esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    static char *output_buffer;
    static int output_len;
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (evt->user_data) {
                    memcpy(evt->user_data + output_len, evt->data, evt->data_len);
                } else {
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    memcpy(output_buffer + output_len, evt->data, evt->data_len);
                }
                output_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        default:
            ESP_LOGD(TAG, "otro tipo de case: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}


void https_sendMessage_perform_post() {
    char url[512];
    char info_raw[256];
    
    char *user = "01275863";
    char *tel = "6611366387";
    char *message = "prueba_ESP_E";
    
    
    snprintf(info_raw, sizeof(info_raw), "%s:%s:%s", user, tel, message);

    snprintf(url, sizeof(url), "https://tj-uabc.net/sms/snd.php?info=%s", info_raw);

    ESP_LOGI(TAG, "URL armado https: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .transport_type = HTTP_METHOD_GET,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    ESP_LOGW(TAG4, "Iniciando cliente HTTP");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %d", esp_http_client_get_status_code(client),esp_http_client_get_content_length(client));

    } else {
        ESP_LOGE(TAG3, "HTTP POST failed: %s", esp_err_to_name(err));
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "Heap libre: %d bytes", esp_get_free_heap_size());
}


void http_test_task(void *pvParameters) {

    

    //va a enciar el mensaje y va amorir 
    https_sendMessage_perform_post();

    ESP_LOGI(TAG, "Mensaje enviado");
    
    vTaskDelay(pdMS_TO_TICKS(60000));
    // Una tarea FreeRTOS nunca debe retornar
    vTaskDelete(NULL);
}