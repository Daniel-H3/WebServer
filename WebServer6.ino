/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-esp8266-plot-chart-web-server/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Sensor:
  temperatur  : DS18B20 waterproof, ✅
                #include <OneWire.h>
                #include <DallasTemperature.h> 
  bunyi       : MAX4466, ✅ gaada library khusus lgsg analog read
  cahaya      : BH1750 ✅
                #include<BH1750.h>
                #include<Wire.h>
  gaya        : Load Cell HX711, ✅
                #include "HX711.h"
                alternatif #include <DFRobot_HX711.h>
  tegangan AC : ZMPT101B, ✅
                #include <ZMPT101B.h>
  arus AC/DC  : ACS712, ✅ gaada library butuh rangkaian voltage diveder
  tegangan DC : INA219, ✅
                #include "Wire.h"
                #include "Adafruit_INA219.h"
  distance    : JSN SR04T ✅ sama kaya ultrasnic biasa
*********/
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <ArduinoJson.h>

#include <OneWire.h>
#include <DallasTemperature.h> 
#include <BH1750.h>
#include "HX711.h"
#include <ZMPT101B.h>
#include "Adafruit_INA219.h"

// Untuk menyimpan pengaturan
String settingsJson = "{}";
DynamicJsonDocument settingsDoc(512); // Objek JSON untuk dipakai dalam handler

/*#include <SPI.h>
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5*/


// Replace with your network credentials
const char* ssid = "ESP32";
const char* password = "qwertyuiop";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Sensor Temperatur
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

String readTemperature() {
  // Read temperature as Celsius (the default)
  sensors.requestTemperatures(); 
  /*
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  */
  float t = sensors.getTempCByIndex(0);
  // Convert temperature to Fahrenheit
  //t = 1.8 * t + 32;
  if (isnan(t)) {    
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

// Sensor Bunyi
const int mic = A0;

String readSound() {
  float b = analogRead(mic);
  if (isnan(b)) {
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(b);
    return String(b);
  }
}

// Sensor Cahaya
BH1750 lightMeter;

String readLight() {
  float l = lightMeter.readLightLevel();
  if (isnan(l)) {
    Serial.println("Failed to read from BME280 sensor!");
    return "";
  }
  else {
    Serial.println(l);
    return String(l);
  }
}

void loadSettings() {
  File file = LittleFS.open("/settings.json", "r");
  if (file) {
    settingsJson = file.readString();  // Simpan sebagai string cache
    deserializeJson(settingsDoc, settingsJson);  // Juga buat objek JSON
    file.close();

    // ✅ Tambahkan ini untuk cetak ke Serial Monitor
    Serial.println("== SETTINGS LOADED ==");
    serializeJsonPretty(settingsDoc, Serial); // tampil lebih rapi
    Serial.println();
  } else {
    settingsJson = "{}";  // Default jika belum ada
    deserializeJson(settingsDoc, settingsJson);
  }
}

  float sine_gen() {            //SINE WAVE GEN
    int angle = angle + 1;  if (angle > 360) angle = 0;
    //sine = 50 * sin((angle * PI / 180));
    float sine = 50 * sin(angle * 0.0174532925);
    Serial.print(sine);
    return sine;
    //sine = abs(sine);
  }
  
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Setup Sensor Temperatur
  sensors.begin(); 

  // Setup Sensor Cahaya
  Wire.begin();
  lightMeter.begin();
  
  bool status; 

  // Initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
        return;
  }

  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP address: ");
  Serial.println(IP);

  loadSettings();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html");
  });
  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/settings.html");
  });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");

  server.on("/save-settings", HTTP_POST, [](AsyncWebServerRequest *request){},
  NULL,
  [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
    File file = LittleFS.open("/settings.json", "w");
    if (file) {
      file.write(data, len);
      file.close();

      // ✅ Langsung muat ulang pengaturan dari file
      loadSettings();

      request->send(200, "text/plain", "Settings saved");
    } else {
      request->send(500, "text/plain", "Failed to save settings");
    }
  });

  server.on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("=> /settings.json requested");
    if (LittleFS.exists("/settings.json")) {
      File file = LittleFS.open("/settings.json", "r");
      if (file) {
        Serial.println("== SETTINGS.JSON CONTENT ==");
        while (file.available()) {
          Serial.write(file.read());
        }
        Serial.println("\n============================");
        file.close();
      } 
      request->send(LittleFS, "/settings.json", "application/json");
    } else {
      request->send(200, "application/json", "{}");
      Serial.println("Failed to open /settings.json");
    }
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    // DynamicJsonDocument doc(256);
    // File file = LittleFS.open("/settings.json", "r");
    // if (file) {
    //   deserializeJson(doc, file);
    //   file.close();
    // }

    DynamicJsonDocument response(256);
    if (settingsDoc["bunyi"]) response["Bunyi"] = random(10, 100);
    if (settingsDoc["gaya"]) response["Gaya"] = random(20, 90);
    if (settingsDoc["tegangan"]) response["Tegangan AC/DC"] = random(40, 70);
    if (settingsDoc["temperatur"]) response["Temperatur"] = random(25, 35);
    
    // if (settingsDoc["teganganDC"]) response["Tegangan DC"] = random(25, 35);
    if (settingsDoc["jarak"]) response["Jarak"] = random(25, 35);
    if (settingsDoc["cahaya"]) response["Cahaya"] = random(25, 35);
    if (settingsDoc["arus"]) response["Arus AC/DC"] = random(25, 35);

    String json;
    serializeJson(response, json);
    request->send(200, "application/json", json);
  });

  server.on("/burst", HTTP_GET, [](AsyncWebServerRequest *request){
  const int numSamples = 500; // atau configurable dari client
  const int samplingInterval = 1; // 1ms

  StaticJsonDocument<4096> json;
  JsonArray arusArray = json.createNestedArray("Arus AC/DC");
  JsonArray teganganArray = json.createNestedArray("Tegangan AC/DC");

  for (int i = 0; i < numSamples; i++) {
    float arus = random(2, 10); // fungsi buat ambil arus
    float tegangan = random(10, 27); // fungsi buat ambil tegangan
    arusArray.add(arus);
    teganganArray.add(tegangan);
    delay(samplingInterval);
  }

  String output;
  serializeJson(json, output);
  request->send(200, "application/json", output);
});


    // Start server
  server.begin();
}
 
void loop(){
  
}