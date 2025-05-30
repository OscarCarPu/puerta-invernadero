// Copyright 2025 Puerta Invernadero Project
// filepath: /home/vant/git/puerta-invernadero/prototipos/src/esp32-api.cpp
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char *ssid = "681E";
const char *password = "t7RshYf37hHtD7";
const char *apiEndpoint = "https://jsonplaceholder.typicode.com/posts/";
const int WIFI_TIMEOUT = 20000;
const int FETCH_INTERVAL = 3000;

// Global variables
uint32_t lastFetchTime = 0;

bool connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - startTime) < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
}

void fetchData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, attempting reconnection...");
    if (!connectToWiFi()) {
      return;
    }
  }

  String apiEndpoint_fetch = String(apiEndpoint) + String(random(1, 101));

  HTTPClient http;
  Serial.print("Fetching data from: ");
  Serial.println(apiEndpoint_fetch);

  http.begin(apiEndpoint_fetch);
  http.setTimeout(10000);

  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("GET code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("--- API Response ---");
      Serial.println(payload);
      Serial.println("---------------------");
    } else {
      Serial.printf("HTTP error code: %d\n", httpCode);
    }
  } else {
    Serial.printf("Error on HTTP request: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 API Client Starting...");

  if (!connectToWiFi()) {
    Serial.println("Failed to connect to WiFi. Will retry in loop.");
  }
}

void loop() {
  uint32_t currentTime = millis();

  // Non-blocking interval check
  if (currentTime - lastFetchTime >= FETCH_INTERVAL) {
    fetchData();
    lastFetchTime = currentTime;
  }

  // Handle millis() overflow
  if (currentTime < lastFetchTime) {
    lastFetchTime = currentTime;
  }

  // Small delay to prevent watchdog issues
  delay(100);
}
