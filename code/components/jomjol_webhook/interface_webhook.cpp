#ifdef ENABLE_WEBHOOK
#include "interface_webhook.h"

#include "esp_log.h"
#include <time.h>
#include "ClassLogFile.h"
#include "esp_http_client.h"
#include "time_sntp.h"
#include "../../include/defines.h"
#include <cJSON.h>


static const char *TAG = "WEBHOOK";

std::string _webhookURI;
std::string _webhookApiKey;

static esp_err_t http_event_handler(esp_http_client_event_t *evt);

void WebhookInit(std::string _uri, std::string _apiKey)
{
    _webhookURI = _uri;
    _webhookApiKey = _apiKey;
}

void WebhookPublish(std::string _value,std::string _valueraw,std::string _error,std::string _rate,std::string _timestamp, long int _timeUTC)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "value", _value.c_str());
    cJSON_AddStringToObject(json, "valueraw", _valueraw.c_str());
    cJSON_AddStringToObject(json, "error", _error.c_str());
    cJSON_AddStringToObject(json, "rate", _rate.c_str());
    cJSON_AddStringToObject(json, "timestamp", _timestamp.c_str());
    cJSON_AddNumberToObject(json, "timeUTC", _timeUTC);

    char *jsonString = cJSON_PrintUnformatted(json);

    LogFile.WriteToFile(ESP_LOG_INFO, TAG, "sending webhook");
    LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "sending JSON: " + std::string(jsonString));

    char response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    esp_http_client_config_t http_config = {
        .url = _webhookURI.c_str(),
        .user_agent = "ESP32 Meter reader",
        .method = HTTP_METHOD_POST,
        .event_handler = http_event_handler,
        .buffer_size = MAX_HTTP_OUTPUT_BUFFER,
        .user_data = response_buffer
    };

    esp_http_client_handle_t http_client = esp_http_client_init(&http_config);

    esp_http_client_set_header(http_client, "Content-Type", "application/json");
    esp_http_client_set_header(http_client, "APIKEY", _webhookApiKey.c_str());

    ESP_ERROR_CHECK(esp_http_client_set_post_field(http_client, jsonString, strlen(jsonString)));

    esp_err_t err = ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(http_client));

    if(err == ESP_OK) {
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP request was performed");
        int status_code = esp_http_client_get_status_code(http_client);
        LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP status code: " + std::to_string(status_code));
    } else {
        LogFile.WriteToFile(ESP_LOG_ERROR, TAG, "HTTP request failed");
    }

    esp_http_client_cleanup(http_client);
    cJSON_Delete(json);
    free(jsonString);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        case HTTP_EVENT_ERROR:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client Error encountered");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client connected");
            ESP_LOGI(TAG, "HTTP Client Connected");
            break;
        case HTTP_EVENT_HEADERS_SENT:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client sent all request headers");
            break;
        case HTTP_EVENT_ON_HEADER:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "Header: key=" + std::string(evt->header_key) + ", value="  + std::string(evt->header_value));
            break;
        case HTTP_EVENT_ON_DATA:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client data recevied: len=" + std::to_string(evt->data_len));
            break;
        case HTTP_EVENT_ON_FINISH:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client finished");
            break;
         case HTTP_EVENT_DISCONNECTED:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Client Disconnected");
            break;
        case HTTP_EVENT_REDIRECT:
            LogFile.WriteToFile(ESP_LOG_DEBUG, TAG, "HTTP Redirect");
            break;
    }
    return ESP_OK;
}

#endif //ENABLE_WEBHOOK