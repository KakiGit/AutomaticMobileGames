#include "httpserver.h"
#include <math.h>
#include <esp_log.h>
#include <ramdb.h>
#include <sstream>
#include <vector>


/* Our URI handler function to be called during POST /params request */
esp_err_t param_post_handler(httpd_req_t *req)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[100];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = fmin(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    std::string s_content(content, recv_size);
    std::vector<std::string> result = std::vector<std::string>{};
    std::stringstream ss = std::stringstream{s_content};

    for (std::string line; std::getline(ss, line, '\n');) {
        result.push_back(line);
    }
    for (auto it = result.begin(); it != result.end(); it++) {
        uint separator_index = it->find("=");
        std::string key = it->substr(0, separator_index);
        std::string value = it->substr(separator_index + 1);
        RDB.write(key, value);
    }

    /* Send a simple response */
    httpd_resp_send(req, s_content.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t position_post_handler(httpd_req_t *req)
{

    char content[100];

    size_t recv_size = fmin(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    std::string s_content(content, recv_size);
    ESP_LOGI("", "content %s", s_content.c_str());
    std::vector<std::string> result = std::vector<std::string>{};
    std::stringstream ss = std::stringstream{s_content};

    for (std::string line; std::getline(ss, line, '\n');) {
        result.push_back(line);
    }

    for (auto it = result.begin(); it != result.end(); it++) {
        uint command_separator_index = it->find("=");
        std::string command = it->substr(0, command_separator_index);
        uint separator_index = it->find(",");
        std::string x = it->substr(command_separator_index + 1, separator_index);
        std::string y = it->substr(separator_index + 1);
        RDB.send("COMMAND", std::make_pair(command, std::make_pair(x, y)));
    }

    /* Send a simple response */
    httpd_resp_send(req, s_content.c_str(), HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t params_post = {
    .uri      = "/params",
    .method   = HTTP_POST,
    .handler  = param_post_handler,
    .user_ctx = NULL
};


httpd_uri_t command_post = {
    .uri      = "/command",
    .method   = HTTP_POST,
    .handler  = position_post_handler,
    .user_ctx = NULL
};


/* Function for starting the webserver */
httpd_handle_t start_webserver(void)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(server, &params_post);
        httpd_register_uri_handler(server, &command_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}