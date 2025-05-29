#include <Arduino.h>
#include <SensirionI2cSht3x.h>
#include <Wire.h>

SensirionI2cSht3x sensor;

static char errorMessage[64];
static int16_t error;

void setup() {

    Serial.begin(9600);
    while (!Serial) {
        delay(100);
    }
    Wire.begin();
    sensor.begin(Wire, SHT30_I2C_ADDR_44);

    sensor.stopMeasurement();
    delay(1);
    sensor.softReset();
    delay(100);
    uint16_t aStatusRegister = 0u;
    error = sensor.readStatusRegister(aStatusRegister);
    if (error != NO_ERROR) {
        Serial.print("Error ejecutando readStatusRegister(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("aStatusRegister: ");
    Serial.print(aStatusRegister);
    Serial.println();
    error = sensor.startPeriodicMeasurement(REPEATABILITY_MEDIUM,
                                            MPS_ONE_PER_SECOND);
    if (error != NO_ERROR) {
        Serial.print("Error ejecutando startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
}

void loop() {

    float aTemperature = 0.0;
    float aHumidity = 0.0;
    error = sensor.blockingReadMeasurement(aTemperature, aHumidity);
    if (error != NO_ERROR) {
        Serial.print("Error ejecutando blockingReadMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("aTemperature: ");
    Serial.print(aTemperature);
    Serial.print(" °C");
    Serial.print("\t");
    Serial.print("aHumidity: ");
    Serial.print(aHumidity);
    Serial.print(" %RH");
    Serial.println();
}