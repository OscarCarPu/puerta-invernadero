// filepath: /home/vant/git/puerta-invernadero/prototipos/src/esp-sht-api.cpp
// Copyright 2025 Puerta Invernadero Project
// Combined ESP32 code for reading SHT30 sensor and sending data to Flask API
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SensirionI2cSht3x.h>
#include <WiFi.h>
#include <Wire.h>

// WiFi Configuration
const char *apiEndpoint = "http://192.168.1.132:8000/api/add_lectura";
const int WIFI_TIMEOUT = 20000;
const int WIFI_SCAN_TIMEOUT = 30000;  // Timeout for network scanning
const int NETWORK_CHECK_INTERVAL =
    300000;                      // Check for better networks every 5 minutes
const int SEND_INTERVAL = 1000;  // Send data every 1 second

// WiFi connection variables
String connectedSSID = "";
bool wifiConnected = false;

// Global variables
SensirionI2cSht3x sensor;
uint32_t lastSendTime = 0;
uint32_t lastNetworkCheck = 0;
static char errorMessage[64];
static int16_t error;
bool sensorInitialized = false;  // Add sensor state tracking

// I2C configuration for SHT30
const int SDA_PIN = 21;
const int SCL_PIN = 22;

bool scanAndConnectToPublicWiFi() {
  Serial.println("Scanning for available WiFi networks...");

  // Start WiFi scan
  int networkCount = WiFi.scanNetworks();

  if (networkCount == 0) {
    Serial.println("No networks found!");
    return false;
  }

  Serial.printf("Found %d networks:\n", networkCount);

  // List all networks and look for open ones
  for (int i = 0; i < networkCount; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    wifi_auth_mode_t encryption = WiFi.encryptionType(i);

    Serial.printf("%d: %s (RSSI: %d, Encryption: %s)\n", i, ssid.c_str(), rssi,
                  (encryption == WIFI_AUTH_OPEN) ? "Open" : "Secured");

    // Try to connect to open networks with good signal strength
    if (encryption == WIFI_AUTH_OPEN && rssi > -70 && ssid.length() > 0) {
      Serial.printf("Attempting to connect to open network: %s\n",
                    ssid.c_str());

      WiFi.begin(ssid.c_str());

      uint32_t startTime = millis();
      while (WiFi.status() != WL_CONNECTED &&
             (millis() - startTime) < WIFI_TIMEOUT) {
        delay(500);
        Serial.print(".");
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n✅ Successfully connected to: %s\n", ssid.c_str());
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        connectedSSID = ssid;
        wifiConnected = true;

        // Clean up scan results
        WiFi.scanDelete();
        return true;
      } else {
        Serial.printf("\n❌ Failed to connect to: %s\n", ssid.c_str());
        WiFi.disconnect();
      }
    }
  }

  // Clean up scan results
  WiFi.scanDelete();
  Serial.println(
      "❌ No suitable public WiFi networks found or connection failed");
  return false;
}

bool connectToWiFi() {
  // If already connected, check if connection is still valid
  if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    Serial.printf("Already connected to: %s\n", connectedSSID.c_str());
    return true;
  }

  // Reset connection state
  wifiConnected = false;
  connectedSSID = "";

  // Disconnect from any previous connection
  WiFi.disconnect();
  delay(1000);

  // Set WiFi mode to station
  WiFi.mode(WIFI_STA);

  // Attempt to scan and connect to public WiFi
  return scanAndConnectToPublicWiFi();
}

bool initializeSensor() {
  Wire.begin(SDA_PIN, SCL_PIN);

  // Scan for the sensor
  int deviceCount = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      deviceCount++;
      Serial.print("Found device at I2C address: 0x");
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found!");
    return false;
  }

  sensor.begin(Wire, SHT30_I2C_ADDR_44);

  sensor.stopMeasurement();
  delay(1);
  sensor.softReset();
  delay(100);

  uint16_t statusRegister = 0u;
  error = sensor.readStatusRegister(statusRegister);
  if (error != NO_ERROR) {
    Serial.print("Error reading status register: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return false;
  }

  Serial.print("SHT30 Status Register: ");
  Serial.println(statusRegister);

  error =
      sensor.startPeriodicMeasurement(REPEATABILITY_MEDIUM, MPS_ONE_PER_SECOND);
  if (error != NO_ERROR) {
    Serial.print("Error starting periodic measurement: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return false;
  }

  Serial.println("SHT30 sensor initialized successfully!");
  return true;
}

bool readSensorData(float &temperature, float &humidity) {
  error = sensor.blockingReadMeasurement(temperature, humidity);
  if (error != NO_ERROR) {
    Serial.print("Error reading measurement: ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return false;
  }
  return true;
}

void sendDataToAPI(float temperature, float humidity) {
  if (WiFi.status() != WL_CONNECTED || !wifiConnected) {
    Serial.println("WiFi not connected, attempting reconnection...");
    if (!connectToWiFi()) {
      Serial.println("Unable to connect to any public WiFi network");
      return;
    }
  }

  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);

  // Create JSON payload
  JsonDocument doc;
  doc["temperatura"] = temperature;
  doc["humedad"] = humidity;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.print("Sending data to: ");
  Serial.println(apiEndpoint);
  Serial.print("JSON payload: ");
  Serial.println(jsonString);

  int httpCode = http.POST(jsonString);

  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_CREATED || httpCode == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.println("✅ Data sent successfully!");
      Serial.print("Server response: ");
      Serial.println(response);
    } else {
      String response = http.getString();
      Serial.printf("❌ HTTP error code: %d\n", httpCode);
      Serial.print("Error response: ");
      Serial.println(response);
    }
  } else {
    Serial.printf("❌ Error on HTTP request: %s\n",
                  http.errorToString(httpCode).c_str());
  }

  http.end();
}

void checkForBetterWiFi() {
  // Check if current connection is weak and rescan for better options
  if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    int32_t currentRSSI = WiFi.RSSI();

    // If signal is weak, try to find a better network
    if (currentRSSI < -80) {
      Serial.printf(
          "Current WiFi signal is weak (RSSI: %d), scanning for better "
          "options...\n",
          currentRSSI);

      // Save current connection info
      String currentSSID = connectedSSID;

      // Disconnect and try to find better network
      WiFi.disconnect();
      wifiConnected = false;
      connectedSSID = "";

      delay(2000);

      if (!scanAndConnectToPublicWiFi()) {
        // If no better network found, try to reconnect to the previous one if
        // it was open
        Serial.printf(
            "No better network found, attempting to reconnect to: %s\n",
            currentSSID.c_str());
        WiFi.begin(currentSSID.c_str());

        uint32_t startTime = millis();
        while (WiFi.status() != WL_CONNECTED &&
               (millis() - startTime) < WIFI_TIMEOUT) {
          delay(500);
          Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
          connectedSSID = currentSSID;
          wifiConnected = true;
          Serial.printf("\nReconnected to: %s\n", currentSSID.c_str());
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 SHT30 + Public WiFi API Client Starting...");
  Serial.println(
      "This device will automatically scan and connect to available public "
      "WiFi networks");

  // Initialize sensor
  if (!initializeSensor()) {
    Serial.println("Failed to initialize SHT30 sensor!");
    Serial.println(
        "Device will halt. Please check sensor connections and restart.");
    while (true) {
      delay(1000);  // Halt execution safely instead of crashing
    }
  }

  sensorInitialized = true;

  // Connect to public WiFi
  Serial.println("Searching for public WiFi networks...");
  if (!connectToWiFi()) {
    Serial.println("Failed to connect to any public WiFi. Will retry in loop.");
  }

  Serial.println("Setup complete!");
}

void loop() {
  // Safety check - don't proceed if sensor not initialized
  if (!sensorInitialized) {
    delay(1000);
    return;
  }

  uint32_t currentTime = millis();

  // Non-blocking interval check
  if (currentTime - lastSendTime >= SEND_INTERVAL) {
    float temperature = 0.0;
    float humidity = 0.0;

    // Read sensor data
    if (readSensorData(temperature, humidity)) {
      Serial.println("--- Sensor Reading ---");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.println(" %RH");
      Serial.println("----------------------");

      // Send data to API
      sendDataToAPI(temperature, humidity);
    } else {
      Serial.println("Failed to read sensor data");
    }

    lastSendTime = currentTime;
  }

  // Check for better WiFi networks periodically
  if (currentTime - lastNetworkCheck >= NETWORK_CHECK_INTERVAL) {
    checkForBetterWiFi();
    lastNetworkCheck = currentTime;
  }

  // Handle millis() overflow
  if (currentTime < lastSendTime) {
    lastSendTime = currentTime;
  }
  if (currentTime < lastNetworkCheck) {
    lastNetworkCheck = currentTime;
  }

  // Small delay to prevent watchdog issues
  delay(100);
}
