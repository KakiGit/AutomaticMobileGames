#include <Arduino.h>
#include <windmouse.h>
#include <WiFi.h>
#include <sstream>
#include <math.h>
#include <httpserver.h>
#include "SPIFFS.h"
#include <esp_log.h>
#include <ramdb.h>

#define DISTANCE 2000

TaskHandle_t Core0Task;
TaskHandle_t Core1Task;

void delay_rand(uint32_t ms) {
  double r_rand = 2 * ((double)rand())/RAND_MAX - 1;
  double t_rand = ms;
  t_rand +=  T_RAND_CLIP * t_rand * r_rand;
  delay(uint32_t(t_rand));
}

void logMemory() {
  ESP_LOGI("", "Total: %d. Used PSRAM: %d", ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
}

void WiFiSetup() {
  std::vector<String> wifi_cred;
  if(!SPIFFS.begin(true)){
    ESP_LOGI("", "An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/wifi");
  if(!file){
    ESP_LOGI("", "Failed to open file for reading");
    return;
  }

  String ssid, password;
  while (file.available()) {
    String line;
    line = file.readStringUntil('\n');
    int separatorIndex = line.indexOf(':');
    if (separatorIndex != -1) {
      ssid = line.substring(0, separatorIndex);
      password = line.substring(separatorIndex + 1);
    }
  }
  file.close();
  const char* ssidc = ssid.c_str();
  const char* passwordc = password.c_str();
  ESP_LOGI("", "Conncting WiFI with %s %s", ssidc, passwordc);
  WiFi.begin(ssidc, passwordc);
  while(WiFi.status() != WL_CONNECTED){
      ESP_LOGI("", "Connecting.");
      delay(1000);
  }
  ESP_LOGI("", "%s", WiFi.localIP().toString());
}

void initParams() {
  RDB.write("G", "9");
  RDB.write("W", "5");
  RDB.write("M", "20");
  RDB.write("D", "12");
}

const static std::vector<std::pair<cooint_t, cooint_t>> CURVE{
  std::pair<cooint_t, cooint_t>(DISTANCE, 0),
  std::pair<cooint_t, cooint_t>(0, DISTANCE),
  std::pair<cooint_t, cooint_t>(-DISTANCE, 0),
  std::pair<cooint_t, cooint_t>(0, -DISTANCE),
};

void mouseControl(void *parameter)
{

  WindMouse windMouse;

  for (;;)
  {
    ESP_LOGI("", "Clicking...");
    delay_rand(2000);
    srand(esp_random());
    windMouse.syncParams();
    windMouse.press();
    for (auto it = CURVE.begin(); it != CURVE.end(); it++) {
      windMouse.move(it->first, it->second);
    }
    if (windMouse.isPressed()) {
      windMouse.release();
    }
  }
}

void httpServerControl(void *parameter)
{
  httpd_handle_t server = start_webserver();
  for (;;)
  {
    delay(5000);
  }
}

void TaskPinning() {
  // Set up Core 0 task handler
  xTaskCreatePinnedToCore(
    mouseControl,
    "Mouse control task",
    10000,
    NULL,
    1,
    &Core0Task,
    0);

  // Set up Core 1 task handler
  xTaskCreatePinnedToCore(
    httpServerControl,
    "HTTP server task task",
    10000,
    NULL,
    1,
    &Core1Task,
    1);
}

void setup() {
  Serial.begin(115200);
  logMemory();
  heap_caps_malloc_extmem_enable(10000);
  delay_rand(1000);
  WiFiSetup();
  initParams();
  TaskPinning();
}

void loop() {}