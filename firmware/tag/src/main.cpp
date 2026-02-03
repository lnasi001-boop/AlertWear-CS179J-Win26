/*
 * VERTEX UWB - Tag 0 (T0) Simple Test
 * 
 * Based on MakerFabs esp32s3_at_t0 example
 * Configures board as Tag 0 and outputs distance data to Serial
 * 
 * Hardware: MakerFabs ESP32-S3 UWB Pro (DW3000 + STM32 AT)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

// ============== USER CONFIG ==============
#define UWB_INDEX 0          // Tag ID (0-63)
#define UWB_TAG_COUNT 64     // Max tags in system
// =========================================

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

void logoshow() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("VERTEX UWB - DW3000"));
    
    display.setCursor(0, 20);
    display.setTextSize(2);
    
    String temp = "T";
    temp += UWB_INDEX;
    temp += "   6.8M";
    display.println(temp);
    
    display.setCursor(0, 40);
    temp = "Tags: ";
    temp += UWB_TAG_COUNT;
    display.println(temp);
    
    display.display();
    delay(2000);
}

String sendData(String command, const int timeout, boolean debug) {
    String response = "";
    
    SERIAL_LOG.print("[TX] ");
    SERIAL_LOG.println(command);
    SERIAL_AT.println(command);
    
    unsigned long startTime = millis();
    while ((startTime + timeout) > millis()) {
        while (SERIAL_AT.available()) {
            char c = SERIAL_AT.read();
            response += c;
        }
    }
    
    if (debug) {
        SERIAL_LOG.print("[RX] ");
        SERIAL_LOG.println(response);
    }
    
    return response;
}

String config_cmd() {
    String temp = "AT+SETCFG=";
    temp += UWB_INDEX;    // Device ID
    temp += ",0";         // Role: 0 = Tag  <-- KEY DIFFERENCE FROM ANCHOR
    temp += ",1";         // Speed: 1 = 6.8M
    temp += ",1";         // Range filter: 1 = on
    return temp;
}

String cap_cmd() {
    String temp = "AT+SETCAP=";
    temp += UWB_TAG_COUNT;  // Tag capacity
    temp += ",10";          // Time slot: 10ms for 6.8M
    temp += ",1";           // Extended mode: 1 = on
    return temp;
}

void setup() {
    // Reset pin
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, HIGH);
    
    // USB Serial for debugging
    SERIAL_LOG.begin(115200);
    delay(2000);  // Wait for USB CDC
    
    SERIAL_LOG.println(F("\n\n========================================"));
    SERIAL_LOG.println(F("  VERTEX UWB - Tag 0 Test"));
    SERIAL_LOG.println(F("========================================\n"));
    
    // UWB Serial (to STM32)
    SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);
    SERIAL_AT.println("AT");
    
    // OLED Display
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        SERIAL_LOG.println(F("SSD1306 OLED failed!"));
        // Continue anyway, UWB will still work
    }
    display.clearDisplay();
    logoshow();
    
    // Configure UWB module as Tag
    SERIAL_LOG.println(F("Configuring UWB module as Tag 0..."));
    
    sendData("AT?", 2000, true);
    sendData("AT+RESTORE", 5000, true);
    sendData(config_cmd(), 2000, true);
    sendData(cap_cmd(), 2000, true);
    sendData("AT+SETRPT=1", 2000, true);  // Enable auto reporting
    sendData("AT+SAVE", 2000, true);
    sendData("AT+RESTART", 2000, true);
    
    SERIAL_LOG.println(F("\n========================================"));
    SERIAL_LOG.println(F("  Tag 0 Ready - Broadcasting"));
    SERIAL_LOG.println(F("========================================\n"));
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Tag 0 READY");
    display.println("Broadcasting...");
    display.display();
}

void loop() {
    // Forward commands from USB Serial to UWB module
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
                
                // Update display with distance data
                if (response.startsWith("AT+RANGE") || response.indexOf(":") > 0) {
                    display.clearDisplay();
                    display.setTextSize(1);
                    display.setCursor(0, 0);
                    display.println("Tag 0 - RANGING");
                    display.println();
                    
                    // Show raw data (truncated to fit)
                    if (response.length() > 21) {
                        display.println(response.substring(0, 21));
                    } else {
                        display.println(response);
                    }
                    display.display();
                }
            }
            response = "";
        } else {
            response += c;
        }
    }
}