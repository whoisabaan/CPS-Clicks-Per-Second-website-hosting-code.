/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <esp_timer.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"
#include <time.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include "nvs_flash.h"
#include "freertos/timers.h"
#include "unistd.h"

#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN (64)

static const char *TAG = "example server:";
/* An HTTP GET handler */
static float clicks;
static char html[5000];
bool click_state = true;
bool closer = false;
int timer1;
int timer2;
static char web_state[20];
static uint32_t currentMillis;

uint32_t millis()
{
    return esp_timer_get_time() / 1000;
}

void start_timer(void *pvParameter)
{
    ESP_LOGI(TAG, "the loop is in progress");
    while (click_state == true || closer == false)
    {
        uint32_t currentMillis = millis();
        timer2 = (int)currentMillis;
        ESP_LOGI(TAG, "the loop is in progress2");
        if (timer2 - timer1 > 15000)
        {
            clicks = clicks / 15;
            strcpy(web_state, "restart");
            ESP_LOGI(TAG, "time ended!");
            click_state = true;
            timer1 = 0;
            timer2 = 0;
            currentMillis = 0;
            click_state = false;
            vTaskDelete(NULL);
            closer = true;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    return;
}

void store_handler(char *buf, int *ret)
{
    if (click_state == true)
    {
        clicks+= 1;
        ESP_LOGI(TAG, "PASSED ON DATA ->%.*s", *ret, buf);
        if (clicks == 1)
        {
            uint32_t currentMillis = millis();
            timer1 = (int)currentMillis;
            xTaskCreate(
                start_timer, // Function
                "My Task",   // Name
                4096,        // Stack size
                NULL,        // Parameters
                7,           // Priority
                NULL         // Task handle
            );
            ESP_LOGI(TAG, "Started timer");
        }
    }
    if (click_state == false)
    {
        return;
    }
    return;
}

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    snprintf(html, sizeof(html),
             "<!DOCTYPE html>\n"
             "<html lang=\"en\">\n"
             "<head>\n"
             "    <meta charset=\"UTF-8\">\n"
             "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
             "    <title>ESP32 CPS</title>\n"
             "    <style>\n"
             "        body {\n"
             "            font-family: Arial, sans-serif;\n"
             "            background-color: #000000;\n"
             "            display: flex;\n"
             "            justify-content: center;\n"
             "            align-items: center;\n"
             "            height: 100vh;\n"
             "            margin: 0;\n"
             "        }\n"
             "        .card {\n"
             "            background: black;\n"
             "            padding: 30px;\n"
             "            border-radius: 12px;\n"
             "            box-shadow: 0 4px 10px rgb(174, 0, 255);\n"
             "            text-align: center;\n"
             "            max-width: 400px;\n"
             "        }\n"
             "        h1 {\n"
             "            color: #ffffff;\n"
             "        }\n"
             "        p {\n"
             "            color: #4c007c;\n"
             "        }\n"
             "        button {\n"
             "            background: #00ff1a;\n"
             "            color: white;\n"
             "            border: none;\n"
             "            padding: 10px 20px;\n"
             "            border-radius: 6px;\n"
             "            cursor: pointer;\n"
             "            font-size: 16px;\n"
             "        }\n"
             "        button:hover {\n"
             "            background: #2fff00;\n"
             "        }\n"
             "    </style>\n"
             "</head>\n"
             "<body>\n"
             "    <div class=\"card\">\n"
             "        <h1>ESP32 CPS9 counter:%.2f</h1>\n"
             "        <p>%s</p>\n"
             "            <button onclick=\"sendPost(), window.location=\'/hello\'\">Click</button>"
             "            <script>"
             "            function sendPost() {"
             "                fetch('/echo', {"
             "                    method: 'POST',"
             "                    headers: {"
             "                        'Content-Type': 'application/json'"
             "                    },"
             "                    body: JSON.stringify({"
             "                        button: 'clicked'"
             "                    })"
             "                })"
             "            }"
             "            </script>"
             "    </div>\n"
             "</body>\n"
             "</html>\n",
             clicks,
             web_state);
    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    /* Set some custom headers */
    httpd_resp_set_hdr(req, "Content-Type", "text/html");
    httpd_resp_set_hdr(req, "Custom-Header-2", "Custom-Value-2");

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-2") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_hdr_value_str(req, "Test-Header-2", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Test-Header-2: %s", buf);
        }
        free(buf);
    }

    buf_len = httpd_req_get_hdr_value_len(req, "Test-Header-1") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_hdr_value_str(req, "Test-Header-1", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Test-Header-1: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, TAG, "buffer alloc failed");
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN], dec_param[EXAMPLE_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "query1", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query3", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query3=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
            if (httpd_query_key_value(buf, "query2", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query2=%s", param);
                example_uri_decode(dec_param, param, strnlen(param, EXAMPLE_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);
            }
        }
        free(buf);
    }

    /* After sending the HTTP response the old HTTP request
     * headers are lost. Check if HTTP request headers can be read now. */
    if (httpd_req_get_hdr_value_len(req, "Host") == 0)
    {
        ESP_LOGI(TAG, "Request headers lost");
    }
    return ESP_OK;
}

static const httpd_uri_t hello = {
    .uri = "/hello",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = html};

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[100];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {
                /* Retry receiving if timeout occurred */
                continue; //___
            } //   |
            return ESP_FAIL; //   |
        } //   V

        /* Send back the same data */
        httpd_resp_send_chunk(req, buf, ret);
        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");
        store_handler(buf, &ret);
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri = "/echo",
    .method = HTTP_POST,
    .handler = echo_post_handler,
    .user_ctx = NULL};

/* An HTTP_ANY handler */
static esp_err_t any_handler(httpd_req_t *req)
{
    /* Send response with body set as the
     * string passed in user context*/
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t any = {
    .uri = "/any",
    .method = HTTP_ANY,
    .handler = any_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = "Hello World!"};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/hello", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        ESP_LOGI(TAG, "404 for URI: %s", req->uri);
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    else if (strcmp("/echo", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/echo URI is not available");
        ESP_LOGI(TAG, "404 for URI: %s", req->uri);
        /* Return ESP_FAIL to close underlying socket */
        return ESP_FAIL;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

/* An HTTP PUT handler. This demonstrates realtime
 * registration and deregistration of URI handlers
 */
static esp_err_t ctrl_put_handler(httpd_req_t *req)
{
    char buf;
    int ret;

    if ((ret = httpd_req_recv(req, &buf, 1)) <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    if (buf == '0')
    {
        /* URI handlers can be unregistered using the uri string */
        ESP_LOGI(TAG, "Unregistering /hello and /echo URIs");
        httpd_unregister_uri(req->handle, "/hello");
        httpd_unregister_uri(req->handle, "/echo");
        /* Register the custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGI(TAG, "Registering /hello and /echo URIs");
        httpd_register_uri_handler(req->handle, &hello);
        httpd_register_uri_handler(req->handle, &echo);
        /* Unregister custom error handler */
        httpd_register_err_handler(req->handle, HTTPD_404_NOT_FOUND, NULL);
    }

    /* Respond with empty body */
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t ctrl = {
    .uri = "/ctrl",
    .method = HTTP_PUT,
    .handler = ctrl_put_handler,
    .user_ctx = NULL};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        httpd_register_uri_handler(server, &echo);
        httpd_register_uri_handler(server, &ctrl);
        httpd_register_uri_handler(server, &any);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server)
    {
        ESP_LOGI(TAG, "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK)
        {
            *server = NULL;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to stop http server");
        }
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    httpd_handle_t *server = (httpd_handle_t *)arg;
    if (*server == NULL)
    {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
     * and re-start it upon connection.
     */

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    /* Start the server for the first time */
    server = start_webserver();

    while (server)
    {
        sleep(5);
    }
}
