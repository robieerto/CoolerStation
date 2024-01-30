/*
  Rui Santos
  Complete project details at our blog.
    - ESP32: https://RandomNerdTutorials.com/esp32-firebase-realtime-database/
    - ESP8266: https://RandomNerdTutorials.com/esp8266-nodemcu-firebase-realtime-database/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based in the RTDB Basic Example by Firebase-ESP-Client library by mobizt
  https://github.com/mobizt/Firebase-ESP-Client/blob/main/examples/RTDB/Basic/Basic.ino
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Robom"
#define WIFI_PASSWORD "starbucksCOFFEE100"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCIQN38pHQlcLp0_ujse-owqncyLZIHTLk"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "robert.mysza@gmail.com"
#define USER_PASSWORD "frekvencnyMENIC230."

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://coolerstation-32933-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase Data object
FirebaseData fbdo;

// Define Firebase objects
FirebaseAuth auth;
FirebaseConfig config;

// Database paths
String espDataPath = "/ESPData";
String dataPath = "/Data";
String actualPath = "/ActualData";

// Variable to save USER UID
String uid;

// Device SSID
String ssid;

int timestamp;
FirebaseJson json;

const char *ntpServer = "pool.ntp.org";

// Define millis timer
unsigned long timer = 0;
unsigned long timerActualData = 0;
unsigned long timerData = 0;
unsigned long timerDelayActualData = 5;
unsigned long timerDelayData = 15;

// Real-time data
float energiaAktualna = 0;
float vykonCinny1 = 0;
float vykonCinny2 = 0;
float teplotaVonkajsia = 0;

// Data
float energiaVyrobenaCelkovo = 0;
float energiaSpotrebovana1 = 0;
float energiaSpotrebovana2 = 0;
float energiaSpotrebovanaCelkovo = 0;

// Initialize WiFi
void initWiFi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void setup()
{
  Serial.begin(115200);

  initWiFi();

  configTime(0, 0, ntpServer);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  // fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Get device SSID
  char c_ssid[23];
  snprintf(c_ssid, 23, "ESP32-%llX", ESP.getEfuseMac());
  ssid = String(c_ssid);

  // Update database path
  espDataPath += "/" + ssid;
}

void loop()
{
  if (millis() - timer > 1000)
  {
    timer = millis();
    timerActualData++;
    timerData++;
  }

  if (Firebase.ready())
  {
    if (timerActualData > timerDelayActualData)
    {
      timerActualData = 0;

      timestamp = getTime();

      // Read data
      energiaAktualna = random(0, 100);
      vykonCinny1 = random(0, 100);
      vykonCinny2 = random(0, 100);
      teplotaVonkajsia = random(0, 100);

      json.set((actualPath + "/energiaAktualna").c_str(), String(energiaAktualna));
      json.set((actualPath + "/vykonCinny1").c_str(), String(vykonCinny1));
      json.set((actualPath + "/vykonCinny2").c_str(), String(vykonCinny2));
      json.set((actualPath + "/teplotaVonkajsia").c_str(), String(teplotaVonkajsia));
      json.set((actualPath + "/timestamp").c_str(), String(timestamp));

      if (timerData > timerDelayData)
      {
        timerData = 0;

        // Read data
        energiaVyrobenaCelkovo = random(0, 100);
        energiaSpotrebovana1 = random(0, 100);
        energiaSpotrebovana2 = random(0, 100);
        energiaSpotrebovanaCelkovo = random(0, 100);

        String dataTimePath = dataPath + "/" + String(timestamp) + "/";

        json.set((dataTimePath + "/energiaVyrobenaCelkovo").c_str(), String(energiaVyrobenaCelkovo));
        json.set((dataTimePath + "/energiaSpotrebovana1").c_str(), String(energiaSpotrebovana1));
        json.set((dataTimePath + "/energiaSpotrebovana2").c_str(), String(energiaSpotrebovana2));
        json.set((dataTimePath + "/energiaSpotrebovanaCelkovo").c_str(), String(energiaSpotrebovanaCelkovo));
        json.set((dataTimePath + "/teplotaVonkajsia").c_str(), String(teplotaVonkajsia));
        json.set((dataTimePath + "/timestamp").c_str(), String(timestamp));
      }

      Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, espDataPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
    }
  }
}
