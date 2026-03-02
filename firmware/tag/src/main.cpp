/*
 * VERTEX UWB - Tag 0 (Piggyback Mode)
 * 
 * Sends BME680 sensor data over UWB using AT+DATA (no WiFi needed)
 * Anchors receive via AT+RDATA and publish to MQTT
 * 
 * Hardware: MakerFabs ESP32-S3 UWB Pro (DW3000 + STM32 AT)
 * Sensor:  BME680 on I2C (SDA=6, SCL=7)
 */

#include <Adafruit_BME680.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// ============== USER CONFIG ==============
#define UWB_INDEX 0          // Tag ID (0-63)
#define UWB_TAG_COUNT 64     // Max tags in system
#define SENSOR_INTERVAL 5000 // Send sensor data every 5 seconds
// =========================================

// BME680 on second I2C bus (pins 6, 7)
TwoWire I2C_BME = TwoWire(1);
Adafruit_BME680 bme(&I2C_BME);
bool bmeReady = false;

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

String response = "";
unsigned long lastSensorSend = 0;

void logoshow() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("VERTEX Tag Piggyback"));
    
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.print("T");
    display.println(UWB_INDEX);
    
    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("BME680 + UWB Data");
    display.println("No WiFi needed");
    
    display.display();
    delay(2000);
}

void updateDisplay(String line1, String line2 = "", String line3 = "") {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0, 0);
    display.print("T");
    display.print(UWB_INDEX);
    display.println(" PIGGYBACK");
    
    display.setCursor(0, 16);
    display.println(line1);
    
    if (line2.length() > 0) {
        display.setCursor(0, 32);
        display.println(line2);
    }
    if (line3.length() > 0) {
        display.setCursor(0, 48);
        display.println(line3);
    }
    
    display.display();
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
    temp += UWB_INDEX;
    temp += ",0";         // Role: 0 = Tag
    temp += ",1";         // Speed: 1 = 6.8M
    temp += ",1";         // Range filter: on
    return temp;
}

String cap_cmd() {
    String temp = "AT+SETCAP=";
    temp += UWB_TAG_COUNT;
    temp += ",10";
    temp += ",1";
    return temp;
}

void setupBME680() {
    I2C_BME.begin(6, 7);
    if (!bme.begin(0x76)) {
        if (!bme.begin(0x77)) {
            SERIAL_LOG.println("BME680 not found!");
            updateDisplay("BME680 FAILED!", "Check wiring");
            return;
        }
    }
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150);
    bmeReady = true;
    SERIAL_LOG.println("BME680 ready!");
}

void sendSensorData() {
    if (!bmeReady) return;
    if (!bme.performReading()) {
        SERIAL_LOG.println("[SENSOR] BME680 read failed!");
        return;
    }
    
    // Format: T:25.3,H:45.2,P:1013.2,G:52.1
    // Keep it compact to fit in UWB frame (~127 byte max)
    String payload = "T:";
    payload += String(bme.temperature, 1);
    payload += ",H:";
    payload += String(bme.humidity, 1);
    payload += ",P:";
    payload += String(bme.pressure / 100.0, 1);
    payload += ",G:";
    payload += String(bme.gas_resistance / 1000.0, 1);
    
    // Send via AT+DATA=<length>,<data>
    String cmd = "AT+DATA=";
    cmd += payload.length();
    cmd += ",";
    cmd += payload;
    
    SERIAL_AT.println(cmd);
    
    SERIAL_LOG.print("[SENSOR] Sent via UWB: ");
    SERIAL_LOG.println(payload);
    
    // Update display
    String tempStr = "T:" + String(bme.temperature, 1) + "C";
    String humStr = "H:" + String(bme.humidity, 1) + "%";
    String gasStr = "G:" + String(bme.gas_resistance / 1000.0, 1) + "kOhm";
    updateDisplay(tempStr, humStr, gasStr);
}

void setup() {
    // Reset pin
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    
    // USB Serial for debugging
    SERIAL_LOG.begin(115200);
    delay(2000);
    
    SERIAL_LOG.println(F("\n\n========================================"));
    SERIAL_LOG.println(F("  VERTEX UWB - Tag 0 (Piggyback Mode)"));
    SERIAL_LOG.println(F("  BME680 data sent over UWB, no WiFi"));
    SERIAL_LOG.println(F("========================================\n"));
    
    // UWB Serial (to STM32)
    SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    SERIAL_AT.println("AT");
    
    // OLED Display
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        SERIAL_LOG.println(F("SSD1306 OLED failed!"));
    }
    display.clearDisplay();
    logoshow();
    
    // Setup BME680
    setupBME680();
    
    // Configure UWB module as Tag
    SERIAL_LOG.println(F("Configuring UWB module as Tag 0..."));
    
    sendData("AT?", 2000, true);
    sendData("AT+RESTORE", 5000, true);
    sendData(config_cmd(), 2000, true);
    sendData(cap_cmd(), 2000, true);
    sendData("AT+SETRPT=1", 2000, true);
    sendData("AT+SAVE", 2000, true);
    sendData("AT+RESTART", 2000, true);
    
    SERIAL_LOG.println(F("\n========================================"));
    SERIAL_LOG.println(F("  Tag 0 Ready - Piggyback Mode"));
    SERIAL_LOG.println(F("  Sensor data every 5 seconds via UWB"));
    SERIAL_LOG.println(F("========================================\n"));
    
    updateDisplay("Ready", "Waiting to range...");
}

void loop() {
    // Send sensor data every SENSOR_INTERVAL ms
    if (millis() - lastSensorSend >= SENSOR_INTERVAL) {
        lastSensorSend = millis();
        sendSensorData();
    }
    
    // Forward commands from USB Serial to UWB module (for debugging)
    while (SERIAL_LOG.available() > 0) {
        SERIAL_AT.write(SERIAL_LOG.read());
        yield();
    }
    
    // Read and display data from UWB module
    while (SERIAL_AT.available() > 0) {
        char c = SERIAL_AT.read();
        
        if (c == '\r') {
            continue;
        } else if (c == '\n') {
            if (response.length() > 0) {
                SERIAL_LOG.println(response);
            }
            response = "";
        } else {
            response += c;
        }
    }
}