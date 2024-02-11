#include <Arduino.h>
#include <windmouse.h>
#include <WiFi.h>
#include <sstream>
#include <math.h>
#include <httpserver.h>
#include "SPIFFS.h"

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
  log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
}

void WiFiSetup() {
  std::vector<String> wifi_cred;
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File file = SPIFFS.open("/wifi");
  if(!file){
    Serial.println("Failed to open file for reading");
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
  Serial.println(ssidc);
  Serial.println(passwordc);
  Serial.println("Conncting WiFI");
  WiFi.begin(ssidc, passwordc);
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }
  Serial.println(WiFi.localIP());
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
    Serial.println("Clicking...");
    delay_rand(1000);
    srand(esp_random());
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
  delay_rand(1000);
  WiFiSetup();
  TaskPinning();
}

void loop() {}