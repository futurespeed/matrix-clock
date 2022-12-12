#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_http_client.h"

#include "cJSON.h"

#include "esp_http_server.h"
#include "index_html.h"

#define AP_SSID "Matrix Clock"
#define AP_PASS "12345678"
#define AP_CHANNEL (0)
#define AP_CONN (5)
#define WIFI_MAXIMUM_RETRY (5)
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define UART_PORT (1)
#define UART_BAUD_RATE ((int)(115200))
#define PIN_UART_TX (18)
#define PIN_UART_RX (19)
#define UART_BUF_SIZE (1024)
#define HTTP_BUF_SIZE (1024)

typedef enum
{
    MODE_WAIT = 0,
    MODE_CLOCK,
    MODE_ANIMATION,
    MODE_MAX
} clock_mode_t;

typedef enum
{
    CMD_MODE = 1,
    CMD_BRIGHTNESS = 2,
    CMD_TIME = 10,
    CMD_LUNAR = 11,
    CMD_TEMPERATURE = 12,
    CMD_WEATHER = 13
} uart_cmd_t;

typedef enum
{
    WEATHER_NONE = 0,
    WEATHER_SUN,
    WEATHER_RAIN,
    WEATHER_CLOUD,
    WEATHER_SNOW
} weather_t;

typedef struct _clockInfo
{
    clock_mode_t mode;
    char wifi_ssid[32];
    char wifi_passwd[64];
    uint8_t wifi_connected;
    char wifi_ip[20];
    int brightness;
    int temperature;
    weather_t weather;
    uint64_t sync_time;
    uint32_t lunar_year;
    uint32_t lunar_month;
    uint32_t lunar_day;
} ClockInfo;

void mc_init_flash();
void mc_init_uart();
void mc_uart_send(const char *data, size_t len);
void mc_init_softap();
void mc_init_sta();
void mc_init_httpd();
void mc_http_get(const char *url, void(http_callback)(char *data, uint16_t size));
esp_err_t mc_http_event_handle(esp_http_client_event_t *evt);

void http_get_time();
void http_get_lunar();
void http_get_weather();
void http_time_callback(char *data, uint16_t size);
void http_lunar_callback(char *data, uint16_t size);
void http_weather_callback(char *data, uint16_t size);

static const char *TAG = "Matrix Clock";
static EventGroupHandle_t s_wifi_event_group;
httpd_handle_t httpd = NULL;

static int s_retry_num = 0;
char http_buf[HTTP_BUF_SIZE];
int http_len;
uint8_t http_lock;

ClockInfo clock_info;

void _url_decode(char *dest, const char *src)
{
    const char *p = src;
    char code[3] = {0};
    unsigned long ascii = 0;
    char *end = NULL;
    while (*p)
    {
        if (*p == '%')
        {
            memcpy(code, ++p, 2);
            ascii = strtoul(code, &end, 16);
            *dest++ = (char)ascii;
            p += 2;
        }
        else
            *dest++ = *p++;
    }
}

void mc_init_flash()
{
    ESP_LOGI(TAG, "init Flash...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void mc_init_uart()
{
    ESP_LOGI(TAG, "init UART...");
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, PIN_UART_TX, PIN_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // // Configure a temporary buffer for the incoming data
    // uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);

    // while (1) {
    //     // Read data from the UART
    //     int len = uart_read_bytes(UART_PORT, data, UART_BUF_SIZE, 20 / portTICK_RATE_MS);
    //     // Write data back to the UART
    //     uart_write_bytes(UART_PORT, (const char *) data, len);
    // }
}

void mc_uart_send(const char *data, size_t len)
{
    ESP_LOGI(TAG, "UART send: %.*s\r\n", len, data);
    uart_write_bytes(UART_PORT, (const char *)data, len);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        memset(clock_info.wifi_ip, 0, sizeof(clock_info.wifi_ip));
        sprintf(clock_info.wifi_ip, IPSTR, IP2STR(&event->ip_info.ip));
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void mc_init_sta(void)
{
    ESP_LOGI(TAG, "init sta...");
    if (strlen(clock_info.wifi_ssid) <= 0)
    {
        ESP_LOGI(TAG, "wifi ssid not set, init sta fail.");
        return;
    }
    // TODO reconnect

    s_wifi_event_group = xEventGroupCreate();

    // ESP_ERROR_CHECK(esp_netif_init());

    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            // .ssid = wifi_ssid,
            // .password = wifi_passwd,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    memcpy(wifi_config.sta.ssid, clock_info.wifi_ssid, strlen(clock_info.wifi_ssid));
    memcpy(wifi_config.sta.password, clock_info.wifi_passwd, strlen(clock_info.wifi_passwd));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 clock_info.wifi_ssid, clock_info.wifi_passwd);
        clock_info.wifi_connected = 1;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 clock_info.wifi_ssid, clock_info.wifi_passwd);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void mc_init_softap(void)
{
    ESP_LOGI(TAG, "init soft AP...");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASS,
            .max_connection = AP_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(AP_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             AP_SSID, AP_PASS, AP_CHANNEL);
}

static esp_err_t index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    return httpd_resp_send(req, (const char *)index_html_gz, index_html_gz_len);
}

static esp_err_t cmd_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char variable[32] = {
        0,
    };
    char value[32] = {
        0,
    };

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "var", variable, sizeof(variable)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    int val = atoi(value);
    int res = 0;
    ESP_LOGI(TAG, "receive Command: %s(%d)\r\n", variable, val);
    if (!strcmp(variable, "mode"))
    {
        if (val >= 0 && val < MODE_MAX)
        {
            clock_info.mode = val;
            char uart_data[100];
            sprintf(uart_data, "%03d:%ld\r\n", CMD_MODE, clock_info.mode);
            mc_uart_send(uart_data, strlen(uart_data));
        }
    }
    else if (!strcmp(variable, "set-brightness"))
    {
        clock_info.brightness = val;
        char uart_data[100];
        sprintf(uart_data, "%03d:%ld\r\n", CMD_BRIGHTNESS, clock_info.brightness);
        mc_uart_send(uart_data, strlen(uart_data));
    }
    else if (!strcmp(variable, "sync-time"))
    {
        http_get_time();
        http_get_lunar();
    }
    else if (!strcmp(variable, "get-weather"))
    {
        http_get_weather();
    }
    else
    {
        res = -1;
    }

    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t connect_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;
    char ssid[32] = {
        0,
    };
    char passwd[64] = {
        0,
    };

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = (char *)malloc(buf_len);
        if (!buf)
        {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            if (httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
                httpd_query_key_value(buf, "passwd", passwd, sizeof(passwd)) == ESP_OK)
            {
            }
            else
            {
                free(buf);
                httpd_resp_send_404(req);
                return ESP_FAIL;
            }
        }
        else
        {
            free(buf);
            httpd_resp_send_404(req);
            return ESP_FAIL;
        }
        free(buf);
    }
    else
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    char encode_ssid[32];
    char encode_passwd[64];
    strcpy(encode_ssid, ssid);
    strcpy(encode_passwd, passwd);
    memset(ssid, 0, sizeof(ssid));
    memset(passwd, 0, sizeof(passwd));
    _url_decode(ssid, encode_ssid);
    _url_decode(passwd, encode_passwd);
    int res = 0;
    ESP_LOGI(TAG, "receive ssid: %s, passwd: %s\r\n", ssid, passwd);
    if (strlen(ssid) > 0)
    {
        strcpy(clock_info.wifi_ssid, ssid);
        strcpy(clock_info.wifi_passwd, passwd);
        mc_init_sta();
        if (clock_info.wifi_connected)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
            http_get_time();
        }
    }
    else
    {
        res = -1;
    }

    if (res)
    {
        return httpd_resp_send_500(req);
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req)
{
    static char json_response[1024];
    char *p = json_response;
    *p++ = '{';
    p += sprintf(p, "\"mode\":%d,", clock_info.mode);
    p += sprintf(p, "\"sync-time\":%lld,", clock_info.sync_time);
    p += sprintf(p, "\"lunar-year\":%d,", clock_info.lunar_year);
    p += sprintf(p, "\"lunar-month\":%d,", clock_info.lunar_month);
    p += sprintf(p, "\"lunar-day\":%d,", clock_info.lunar_day);
    p += sprintf(p, "\"wifi-state\":%d,", clock_info.wifi_connected);
    p += sprintf(p, "\"wifi-ip\":\"%.*s\",", strlen(clock_info.wifi_ip), clock_info.wifi_ip);
    p += sprintf(p, "\"wifi-ssid\":\"%.*s\",", strlen(clock_info.wifi_ssid), clock_info.wifi_ssid);
    p += sprintf(p, "\"wifi-passwd\":\"%.*s\",", strlen(clock_info.wifi_passwd), clock_info.wifi_passwd);
    p += sprintf(p, "\"brightness\":%d,", clock_info.brightness);
    p += sprintf(p, "\"temperature\":%d,", clock_info.temperature);
    p += sprintf(p, "\"weather\":%d", clock_info.weather);
    *p++ = '}';
    *p++ = 0;
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(req, json_response, strlen(json_response));
}

void mc_init_httpd()
{
    ESP_LOGI(TAG, "init HTTP server...");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL};

    httpd_uri_t status_uri = {
        .uri = "/status",
        .method = HTTP_GET,
        .handler = status_handler,
        .user_ctx = NULL};

    httpd_uri_t cmd_uri = {
        .uri = "/control",
        .method = HTTP_GET,
        .handler = cmd_handler,
        .user_ctx = NULL};

    httpd_uri_t connect_uri = {
        .uri = "/connect",
        .method = HTTP_GET,
        .handler = connect_handler,
        .user_ctx = NULL};

    ESP_LOGI(TAG, "Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&httpd, &config) == ESP_OK)
    {
        httpd_register_uri_handler(httpd, &index_uri);
        httpd_register_uri_handler(httpd, &cmd_uri);
        httpd_register_uri_handler(httpd, &status_uri);
        httpd_register_uri_handler(httpd, &connect_uri);
    }
}

void mc_http_get(const char *url, void(http_callback)(char *data, uint16_t size))
{
    if (!clock_info.wifi_connected || http_lock)
    {
        return;
    }
    http_lock = 1;
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = mc_http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    uint8_t http_ok;
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        if (200 == esp_http_client_get_status_code(client))
        {
            http_ok = 1;
        }
    }
    esp_http_client_cleanup(client);
    if (http_ok)
    {
        http_callback(http_buf, http_len);
    }
    http_lock = 0;
}

esp_err_t mc_http_event_handle(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        memset(http_buf, 0, HTTP_BUF_SIZE);
        http_len = 0;
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        ESP_LOGI(TAG, "%.*s", evt->data_len, (char *)evt->data);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // if (!esp_http_client_is_chunked_response(evt->client)) {
        //     ESP_LOGI(TAG, "%.*s", evt->data_len, (char*)evt->data);
        // }
        memcpy(http_buf + http_len, evt->data, evt->data_len);
        http_len += evt->data_len;
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        // ESP_LOGI(TAG, "HTTP receive: %.*s\r\n", http_len, http_buf);
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

void http_get_time()
{
    const char *url = "https://f.m.suning.com/api/ct.do";
    mc_http_get(url, &http_time_callback);
}

void http_get_lunar()
{
    const char *url = "https://api.xlongwei.com/service/datetime/info.json";
    mc_http_get(url, &http_lunar_callback);
}

void http_get_weather()
{
    const char *url = "http://autodev.openspeech.cn/csp/api/v2.1/weather?openId=aiuicus&clientType=android&sign=android&city=%E5%B9%BF%E5%B7%9E&pageNo=1&pageSize=1";
    mc_http_get(url, &http_weather_callback);
}

void http_time_callback(char *data, uint16_t size)
{
    ESP_LOGI(TAG, "http_time_callback, content: %.*s", size, data);
    char json_str[size];
    memcpy(json_str, data, size);
    cJSON *json_root = cJSON_Parse(json_str);
    double currentTime = cJSON_GetObjectItem(json_root, "currentTime")->valuedouble;
    clock_info.sync_time = (uint64_t)currentTime;

    char uart_data[100];
    sprintf(uart_data, "%03d:%lld\r\n", CMD_TIME, clock_info.sync_time);
    mc_uart_send(uart_data, strlen(uart_data));
}

void http_lunar_callback(char *data, uint16_t size)
{
    ESP_LOGI(TAG, "http_lunar_callback, content: %.*s", size, data);
    char json_str[size];
    memcpy(json_str, data, size);
    cJSON *json_root = cJSON_Parse(json_str);
    clock_info.lunar_year = cJSON_GetObjectItem(json_root, "lunarYear")->valueint;
    clock_info.lunar_month = cJSON_GetObjectItem(json_root, "lunarMonth")->valueint;
    clock_info.lunar_day = cJSON_GetObjectItem(json_root, "lunarDay")->valueint;

    char uart_data[100];
    sprintf(uart_data, "%03d:%d,%d,%d\r\n", CMD_LUNAR, clock_info.lunar_year, clock_info.lunar_month, clock_info.lunar_day);
    mc_uart_send(uart_data, strlen(uart_data));
}

void http_weather_callback(char *data, uint16_t size)
{
    ESP_LOGI(TAG, "http_weather_callback, content: %.*s", size, data);
    char json_str[size];
    memcpy(json_str, data, size);
    cJSON *json_root = cJSON_Parse(json_str);
    cJSON *json_data = cJSON_GetObjectItem(json_root, "data");
    cJSON *json_list = cJSON_GetObjectItem(json_data, "list");
    cJSON *json_item = cJSON_GetArrayItem(json_list, 0);
    char *city = cJSON_GetObjectItem(json_item, "city")->valuestring;
    char *weather = cJSON_GetObjectItem(json_item, "weather")->valuestring;
    double temperature = cJSON_GetObjectItem(json_item, "temp")->valuedouble;
    ESP_LOGI(TAG, "city: %s, weather: %s, temperature: %d", city, weather, (int)temperature);

    clock_info.temperature = (int)temperature;
    clock_info.weather = WEATHER_NONE;
    if (strstr(weather, "晴") != NULL)
    {
        clock_info.weather = WEATHER_SUN;
    }
    else if (strstr(weather, "雨") != NULL)
    {
        clock_info.weather = WEATHER_RAIN;
    }
    else if (strstr(weather, "阴") != NULL || strstr(weather, "云") != NULL || strstr(weather, "雾") != NULL || strstr(weather, "霾") != NULL || strstr(weather, "沙") != NULL || strstr(weather, "尘") != NULL)
    {
        clock_info.weather = WEATHER_CLOUD;
    }
    else if (strstr(weather, "雪") != NULL)
    {
        clock_info.weather = WEATHER_SNOW;
    }

    char uart_data[100];
    sprintf(uart_data, "%03d:%d\r\n", CMD_TEMPERATURE, clock_info.temperature);
    mc_uart_send(uart_data, strlen(uart_data));

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    memset(uart_data, 0, sizeof(uart_data));
    sprintf(uart_data, "%03d:%d\r\n", CMD_WEATHER, clock_info.weather);
    mc_uart_send(uart_data, strlen(uart_data));
}

void app_main(void)
{
    clock_info.brightness = 10;

    // // FIXME
    // const char *wifi_ssid = "blue cave";
    // const char *wifi_passwd = "futurespeed";
    // strcpy(clock_info.wifi_ssid, wifi_ssid);
    // strcpy(clock_info.wifi_passwd, wifi_passwd);

    mc_init_flash();
    mc_init_uart();
    mc_init_softap();
    mc_init_sta();
    mc_init_httpd();

    if (clock_info.wifi_connected)
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        http_get_time();
    }
}