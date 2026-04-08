#include <string.h>
#include <errno.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_log.h"


#include<esp_err.h>


//librereia propia
#include"tcp_lib.h"


//macros 
#define TAG "TCP_CLIENT"

//varibales

tcp_client_t tcp_client = {
    .sock      = -1,
    .connected = false,
    .logged_in = false,
};


//funciones 
// esp_err_t tcp_init(char server_ip, uint16_t server_port)
esp_err_t  tcp_client_init(void)
{   



    // 1. Crear socket
    //SOCK_STREAM indica que es de tipo TCP la conexion 
    tcp_client.sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcp_client.sock < 0) {
        ESP_LOGE(TAG, "erro al crear el decriptor socket() %d", errno);
        return ESP_FAIL;
    }

    // 2. Timeout de recv
    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(tcp_client.sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  
    // 3. Dirección del servidor
    
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(DEFAULT_PORT),
    };
    inet_pton(AF_INET, DEFAULT_IP, &server_addr.sin_addr);


    
    // 4. Conectar
    if (connect(tcp_client.sock,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG, "descriptor <connect()> fallo  %d", errno);
        close(tcp_client.sock);
        tcp_client.sock = -1;
        return ESP_FAIL;
    }

    tcp_client.connected = true;

    ESP_LOGI(TAG, "conectado a %s:%d", DEFAULT_IP, DEFAULT_PORT);

    return ESP_OK;
}


//tareas 

// esp_err_t tcp_server_upate(char server_IP, uint16_t server_port){

// }



/**
 * apartir de aqui esto no cuenta para mi programa nomas es para hacer lo que el profe quiere
 * 
 * 
 */

esp_err_t tcp_client_send(const char *data)
{
    if (!tcp_client.connected) {
        ESP_LOGE(TAG, "no hay conexion activa");
        return ESP_FAIL;
    }

    int ret = send(tcp_client.sock, data, strlen(data), 0);
    if (ret < 0) {
        ESP_LOGE(TAG, "send() fallo: errno %d", errno);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "TX: %s", data);
    return ESP_OK;
}
int tcp_client_recv(char *buf, size_t buf_len)
{
    if (!tcp_client.connected) return -1;

    int len = recv(tcp_client.sock, buf, buf_len - 1, 0);
    if (len > 0) {
        buf[len] = '\0';
        ESP_LOGI(TAG, "RX: %s", buf);
    } else if (len == 0) {
        ESP_LOGW(TAG, "servidor cerro la conexion");
        tcp_client.connected = false;
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // timeout normal del SO_RCVTIMEO, no es error
        } else {
            // errno 104 = ECONNRESET, 113 = EHOSTUNREACH, etc.
            ESP_LOGE(TAG, "recv() error real: errno %d (%s)", errno, strerror(errno));
            tcp_client.connected = false;  // ← esto faltaba
        }
    }

    return len;
}


void tcp_client_close(void)
{
    if (tcp_client.sock >= 0) {
        close(tcp_client.sock);
        tcp_client.sock = -1;
    }
    tcp_client.connected = false;
    tcp_client.logged_in = false;
    ESP_LOGI(TAG, "conexion cerrada");
}



 /* ────────────────────────────────────────────
 * LOGIN — envía UABC:...:L:S y espera ACK
 * ──────────────────────────────────────────── */
esp_err_t tcp_login(void)
{
    char cmd[128];
    char resp[BUF_SIZE];
    int  len;

    // Armar comando de login
    snprintf(cmd, sizeof(cmd), "UABC:%s:L:S:Login el server\n", USUARIO);

    // Enviar
    if (tcp_client_send(cmd) != ESP_OK) {
        return ESP_FAIL;
    }

    // Esperar respuesta del servidor
    len = tcp_client_recv(resp, sizeof(resp));
    if (len <= 0) {
        ESP_LOGE(TAG, "sin respuesta del servidor");
        return ESP_FAIL;
    }

    // Verificar ACK
    if (strncmp(resp, "ACK", 3) == 0) {
        tcp_client.logged_in = true;
        ESP_LOGI(TAG, "Login exitoso");
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Login rechazado: %s", resp);
    return ESP_FAIL;
}
/* ── KEEP-ALIVE — solo send, sin recv ── */
void keepalive_task(void *pvParameters)
{
    char cmd[128];

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));

        if (tcp_client.connected && tcp_client.logged_in) {
            snprintf(cmd, sizeof(cmd), "UABC:%s:K:S:Keep-Alive al server\n", USUARIO);

            if (tcp_client_send(cmd) != ESP_OK) {
                ESP_LOGE(TAG, "Keep-Alive send fallo");
                tcp_client.connected = false;
                tcp_client.logged_in = false;
            } else {
                ESP_LOGI(TAG, "Keep-Alive enviado");
            }
        }
    }
}

/* ── RECV TASK — unica dueña del recv ── */
void tcp_recv_task(void *pvParameters)
{
    char buf[BUF_SIZE];
    int  len;

    while (1) {
        len = tcp_client_recv(buf, sizeof(buf));

        if (len > 0) {
            ESP_LOGI(TAG, "SERVER >> %s", buf);

        } else if (len == 0) {
            ESP_LOGE(TAG, "servidor desconectado (cierre limpio)");
            break;

        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                vTaskDelay(pdMS_TO_TICKS(100)); // timeout normal, seguir
            } else {
                // errno 104 u otro error real
                ESP_LOGE(TAG, "error de socket errno %d, cerrando tarea", errno);
                tcp_client.connected = false;
                tcp_client.logged_in = false;
                break; // ← antes nunca salía aquí
            }
        }
    }

    tcp_client_close();
    vTaskDelete(NULL);
}