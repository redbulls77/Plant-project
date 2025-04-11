 
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <DHT.h>
 
// Pin definitions
#define SOIL_MOISTURE_PIN A0
#define DHT11_PIN 2
#define PUMP_PIN 3
 
// Wi-Fi credentials
const char ssid[] = "";
const char pass[] = "";
 
// MQTT broker details
const char broker[] = "86.50.169.135";
const int port = 1883;
const char clientId[] = "mqtt-test-client";
 
// MQTT topics
const char soilMoistureTopic[] = "testing/soilmoisture";
const char airTemperatureTopic[] = "testing/airtemperature";
const char airHumidityTopic[] = "testing/airhumidity";
 
WiFiClient net;
MqttClient client(net);
DHT dht11(DHT11_PIN, DHT11);
 
unsigned long lastPumpActivationTime = 0; // Saves time log of when pump was last activated
bool pumpActivated = false; // Take a log of pumps condition
const unsigned long pumpWaitTime = 60000; // 10 minutes as milliseconds
 
// Function to publish MQTT messages
void publishToMQTT(const char* topic, const String& payload) {
    client.beginMessage(topic);
    client.print(payload);
    client.endMessage();
}
 
// Function to create JSON payload
String createJsonPayload(const char* key, float value) {
    return String("{\"") + key + "\":" + String(value) + "}";
}
 
// Function to control the pump based on soil moisture
void controlPump(int moisturePercent) {
    if (moisturePercent < 30 && !pumpActivated) {
        Serial.println("⚠ Soil is too dry! Turning on the pump...");
        digitalWrite(PUMP_PIN, LOW); // Turn on the pump
        delay(3000); // Keep the pump on for 5 seconds
        digitalWrite(PUMP_PIN, HIGH); // Turn off the pump
        delay(5000);
        digitalWrite(PUMP_PIN, LOW);
        delay(3000);
        digitalWrite(PUMP_PIN, HIGH);
        pumpActivated = true; // Mark the pump as activated
        lastPumpActivationTime = millis(); // Refresh the latest activation time
    } else {
        Serial.println("🌊 Soil moisture is sufficient.");
    }
}
 
void setup() {
    Serial.begin(9600);
 
    // Initialize DHT sensor
    dht11.begin();
 
    // Initialize pump pin
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, HIGH); // Pump initially off
 
    // Connect to Wi-Fi
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(2000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi!");
 
    // Connect to MQTT broker
    client.setId(clientId);
    Serial.print("Connecting to MQTT broker...");
    if (client.connect(broker, port)) {
        Serial.println("Connected to MQTT broker!");
    } else {
        Serial.println("Failed to connect to MQTT broker!");
    }
}
 
void loop() {
    // Check and maintain MQTT connection
    if (!client.connected()) {
        Serial.println("MQTT connection lost. Attempting to reconnect...");
        if (!client.connect(broker, port)) {
            Serial.println("Failed to reconnect to MQTT broker!");
            delay(5000);
            return;
        }
    }
 
    // Read temperature and humidity from DHT11
    float humidity = dht11.readHumidity();
    float temperature = dht11.readTemperature();
 
    if (!pumpActivated || (millis() - lastPumpActivationTime >= pumpWaitTime)) {
        // Read soil moisture data
        int soilMoistureRaw = analogRead(SOIL_MOISTURE_PIN);
        int moisturePercent = map(soilMoistureRaw, 250, 500, 0, 100);
 
        if (isnan(humidity) || isnan(temperature)) {
            Serial.println("⚠ Failed to read from DHT sensor!");
        } else {
            // Publish soil moisture data
            publishToMQTT(soilMoistureTopic, createJsonPayload("moisture", moisturePercent));
 
            // Publish temperature data
            publishToMQTT(airTemperatureTopic, createJsonPayload("temperature", temperature));
 
            // Publish humidity data
            publishToMQTT(airHumidityTopic, createJsonPayload("humidity", humidity));
 
            // Print readings to serial monitor
            Serial.println("====================================");
            Serial.print("🌱 Soil Moisture (%): ");
            Serial.println(moisturePercent);
            Serial.print("💧 Air Humidity (%): ");
            Serial.println(humidity);
            Serial.print("🌡 Air Temperature (°C): ");
            Serial.println(temperature);
            Serial.println("====================================");
        }
 
        // Control pump based on soil moisture
        controlPump(moisturePercent);
    } else {
        Serial.println("⏳ Waiting 10 minutes before processing new soil moisture reading...");
    }
 
    // Delay before next iteration
    delay(120000); // Print sensor readings to the serial monitor every 2 minutes. 
}
