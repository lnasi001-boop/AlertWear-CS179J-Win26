/*
 * VERTEX UWB - Anchor 0 with WiFi + MQTT
 * 
 * Publishes UWB distance data to MQTT broker for dashboard display
 * 
 * Hardware: MakerFabs ESP32-S3 UWB Pro (DW3000 + STM32 AT)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// ============== USER CONFIG - EDIT THESE! ==============
// WiFi Settings
const char* WIFI_SSID = "iPhone";
const char* WIFI_PASSWORD = "cooldawg";

// MQTT Settings (your VERTEX server)
const char* MQTT_SERVER = "172.20.10.2";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "";
const char* MQTT_PASSWORD = "";

// Anchor Configuration
#define ANCHOR_ID 0
#define ANCHOR_X 0.0
#define ANCHOR_Y 0.0

// UWB Settings
#define UWB_TAG_COUNT 64
// =======================================================

// Pin definitions for ESP32-S3 UWB board
#define RESET_PIN 16
#define IO_RXD2 18
#define IO_TXD2 17
#define I2C_SDA 39
#define I2C_SCL 38

// Serial aliases
#define SERIAL_LOG Serial
HardwareSerial SERIAL_AT(2);

// OLED Display
Adafruit_SSD1306 display(128, 64, &Wire, -1);

// WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// State variables
String response = "";
bool wifiConnected = false;
bool mqttConnected = false;
unsigned long lastReconnectAttempt = 0;

void logoshow() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("VERTEX Anchor+MQTT"));
    
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.print("A");
    display.println(ANCHOR_ID);
    
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.print("Pos: ");
    display.print(ANCHOR_X);
    display.print(",");
    display.println(ANCHOR_Y);
    
    display.display();
    delay(2000);
}

void updateDisplay(String status, String data = "") {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("A");
    display.print(ANCHOR_ID);
    display.print(" WiFi:");
    display.print(wifiConnected ? "OK" : "--");
    display.print(" MQ:");
    display.println(mqttConnected ? "OK" : "--");
    
    display.setCursor(0, 16);
    display.println(status);
    
    if (data.length() > 0) {
        display.setCursor(0, 32);
        if (data.length() > 21) {
            display.println(data.substring(0, 21));
        } else {
            display.println(data);
        }
    }
    
    display.setCursor(0, 56);
    if (wifiConnected) {
        display.print(WiFi.localIP());
    } else {
        display.print("Connecting...");
    }
    
    display.display();
}

void setupWiFi() {
    SERIAL_LOG.print("Connecting to WiFi: ");
    SERIAL_LOG.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        SERIAL_LOG.print(".");
        attempts++;
        updateDisplay("WiFi connecting...");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        SERIAL_LOG.println();
        SERIAL_LOG.print("WiFi connected! IP: ");
        SERIAL_LOG.println(WiFi.localIP());
    } else {
        SERIAL_LOG.println();
        SERIAL_LOG.println("WiFi connection failed!");
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    SERIAL_LOG.print("MQTT server: ");
    SERIAL_LOG.print(MQTT_SERVER);
    SERIAL_LOG.print(":");
    SERIAL_LOG.println(MQTT_PORT);
}

void reconnectMQTT() {
    if (!wifiConnected) return;
    
    if (millis() - lastReconnectAttempt < 5000) return;
    lastReconnectAttempt = millis();
    
    SERIAL_LOG.print("Connecting to MQTT...");
    
    String clientId = "vertex-anchor-";
    clientId += ANCHOR_ID;
    clientId += WiFi.macAddress();      // Added
    
    bool connected;
    if (strlen(MQTT_USER) > 0) {
        connected = mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD);
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        mqttConnected = true;
        SERIAL_LOG.println("connected!");
        String topic = "uwb/anchor/";
        topic += ANCHOR_ID;
        topic += "/status";
        mqttClient.publish(topic.c_str(), "online", true);

        updateDisplay("Ready", "Waiting for tags...");
    } else {
        mqttConnected = false;
        SERIAL_LOG.print("failed, rc=");
        SERIAL_LOG.println(mqttClient.state());
        updateDisplay("MQTT Failed", "Retrying...");
    }
}

void publishDistance(int tagId, float distance, int rssi) {
    if (!mqttConnected) return;
    
    JsonDocument doc;
    doc["tagId"] = tagId;
    doc["distance"] = distance;
    doc["rssi"] = rssi;
    doc["anchorId"] = ANCHOR_ID;
    doc["anchorX"] = ANCHOR_X;
    doc["anchorY"] = ANCHOR_Y;
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = "uwb/anchor/";
    topic += ANCHOR_ID;
    
    if (mqttClient.publish(topic.c_str(), payload.c_str())) {
        SERIAL_LOG.print("[MQTT] Published: ");
        SERIAL_LOG.println(payload);
    } else {
        SERIAL_LOG.println("[MQTT] Publish failed!");
    }
}

void parseAndPublish(String data) {
    // Format: AT+RANGE=tid:0,mask:01,seq:65,range:(92,0,0,0,0,0,0,0),ancid:(0,-1,-1,-1,-1,-1,-1,-1)
    
    if (!data.startsWith("AT+RANGE=")) return;
    
    // Extract tag ID (tid:X)
    int tidIdx = data.indexOf("tid:");
    if (tidIdx < 0) return;
    int tidEnd = data.indexOf(",", tidIdx);
    int tagId = data.substring(tidIdx + 4, tidEnd).toInt();
    
    // Extract range value for this ANCHOR_ID
    int rangeIdx = data.indexOf("range:(");
    int ancIdx = data.indexOf("ancid:(");
    if (rangeIdx < 0 || ancIdx < 0) return;
    
    int rangeStart = rangeIdx + 7;
    int rangeEnd = data.indexOf(")", rangeStart);
    int ancStart = ancIdx + 7;
    int ancEnd = data.indexOf(")", ancStart);
    if (rangeEnd < 0 || ancEnd < 0) return;
    
    String rangeList = data.substring(rangeStart, rangeEnd);
    String ancList = data.substring(ancStart, ancEnd);
    
    int distanceCm = 0;
    int rPos = 0;
    int aPos = 0;
    for (int i = 0; i < 8; i++) {
        int rComma = rangeList.indexOf(",", rPos);
        int aComma = ancList.indexOf(",", aPos);
        
        String rPart = (rComma == -1) ? rangeList.substring(rPos) : rangeList.substring(rPos, rComma);
        String aPart = (aComma == -1) ? ancList.substring(aPos) : ancList.substring(aPos, aComma);
        
        if (aPart.toInt() == ANCHOR_ID) {
            distanceCm = rPart.toInt();
            break;
        }
        
        if (rComma == -1 || aComma == -1) break;
        rPos = rComma + 1;
        aPos = aComma + 1;
    }
    
    
    
    // Only publish valid readings
    if (distanceCm > 0) {
        float distanceM = (distanceCm / 100.0) - 0.5;    
        if(distanceM < 0) distanceM = 0;
        publishDistance(tagId, distanceM, 0);


        // Update display
        String displayData = "T";
        displayData += tagId;
        displayData += ": ";
        displayData += String(distanceM, 2);
        displayData += "m";
        updateDisplay("Ranging OK", displayData);
    }
}

String sendData(String command, const int timeout, boolean debug) {
    String resp = "";
    
    SERIAL_LOG.print("[TX] ");
    SERIAL_LOG.println(command);
    SERIAL_AT.println(command);
    
    unsigned long startTime = millis();
    while ((startTime + timeout) > millis()) {
        while (SERIAL_AT.available()) {
            char c = SERIAL_AT.read();
            resp += c;
        }
    }
    
    if (debug) {
        SERIAL_LOG.print("[RX] ");
        SERIAL_LOG.println(resp);
    }
    
    return resp;
}

String config_cmd() {
    String temp = "AT+SETCFG=";
    temp += ANCHOR_ID;
    temp += ",1";  // Role: 1 = Anchor
    temp += ",1";  // Speed: 1 = 6.8M
    temp += ",1";  // Range filter: on
    return temp;
}

String cap_cmd() {
    String temp = "AT+SETCAP=";
    temp += UWB_TAG_COUNT;
    temp += ",10";
    temp += ",1";
    return temp;
}

void setup() {
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    
    SERIAL_LOG.begin(115200);
    delay(2000);
    
    SERIAL_LOG.println(F("\n\n================================================"));
    SERIAL_LOG.println(F("  VERTEX UWB - Anchor with WiFi + MQTT"));
    SERIAL_LOG.println(F("================================================\n"));
    
    SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    SERIAL_AT.println("AT");
    
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        SERIAL_LOG.println(F("SSD1306 OLED failed!"));
    }
    display.clearDisplay();
    logoshow();
    
    SERIAL_LOG.println(F("Configuring UWB module..."));
    updateDisplay("UWB Config...");
    
    sendData("AT?", 2000, true);
    sendData("AT+RESTORE", 5000, true);
    sendData(config_cmd(), 2000, true);
    sendData(cap_cmd(), 2000, true);
    sendData("AT+SETRPT=1", 2000, true);
    sendData("AT+SAVE", 2000, true);
    sendData("AT+RESTART", 2000, true);
    
    SERIAL_LOG.println(F("UWB configured as Anchor!"));
    
    setupWiFi();
    setupMQTT();
    
    SERIAL_LOG.println(F("\n================================================"));
    SERIAL_LOG.println(F("  Anchor Ready - Publishing to MQTT"));
    SERIAL_LOG.println(F("================================================\n"));
    
    updateDisplay("Ready", "Waiting for tags...");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        wifiConnected = false;
        mqttConnected = false;
        static unsigned long lastWifiAttempt = 0;
        if (millis() - lastWifiAttempt > 10000) {
            lastWifiAttempt = millis();
            WiFi.reconnect();
        }
    } else {
        wifiConnected = true;
    }
    
    if (wifiConnected && !mqttClient.connected()) {
        mqttConnected = false;
        reconnectMQTT();
    }
    
    if (mqttConnected) {
        mqttClient.loop();
    }
    
    while (SERIAL_LOG.available() > 0) {
        SERIAL_AT.write(SERIAL_LOG.read());
        yield();
    }
    
    while (SERIAL_AT.available() > 0) {
        char c = SERIAL_AT.read();
        
        if (c == '\r') {
            continue;
        } else if (c == '\n') {
            if (response.length() > 0) {
                SERIAL_LOG.print("[UWB] ");
                SERIAL_LOG.println(response);
                
                parseAndPublish(response);
            }
            response = "";
        } else {
            response += c;
        }
    }
}