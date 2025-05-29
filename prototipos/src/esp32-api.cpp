#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "681E";
const char* password = "t7RshYf37hHtD7";
const char* apiEndpoint = "https://jsonplaceholder.typicode.com/posts/1";

void connectToWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.print("\nWiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
}

void fetchData() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected");
        return;
    }
    HTTPClient http;
    Serial.print("Fetching data from: ");
    Serial.println(apiEndpoint);

    http.begin(apiEndpoint);
    int httpCode = http.GET();

    if (httpCode > 0){
        Serial.printf("GET code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println("--- API Response ---");
            Serial.println(payload);
            Serial.println("---------------------");
        }
    } else {
        Serial.printf("Error on HTTP request: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    connectToWiFi();
}

void loop() {
    fetchData();
    delay(5000);
}