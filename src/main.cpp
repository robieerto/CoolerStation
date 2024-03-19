

/*
  Modul ID ESP32
  MODBUS RTU NA MODBUS ETHERNET
  RS485
 */

#include <Arduino.h>
#include <EthernetENC.h>
#include <EthernetUdp.h>
#include <NTPClient.h>
#include "esp_system.h"
#include "ModbusRtu.h" // komunikacia modbus slave
#include <SPI.h>
#include <SD.h>

#include <virtuabotixRTC.h>

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"

#include <Firebase_ESP_Client.h>
#include <CircularBuffer.hpp>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#ifdef __INTELLISENSE__
#pragma diag_suppress 350
#endif

// Insert your network credentials
#define WIFI_SSID "wifi_ssid"
#define WIFI_PASSWORD "password"

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
String historyPath = "/HistoryData";

// Variable to save USER UID
String uid;

// Device SSID
String ssid;

int timestamp;
FirebaseJson jsonLive;
FirebaseJson jsonDb;
bool firebaseConfigReady = false;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// Define millis timer
unsigned long timerActualData = 0;
unsigned long timerData = 0;
unsigned long TIME_ACTUAL_DATA = 5000;
unsigned long TIME_DATA = 60000 * 15; // 15 minutes

// Real-time data
int32_t energiaAktualna;
float vykonCinny1;
float vykonCinny2;
float teplotaVonkajsia;

// Data
float energiaVyrobena1;
float energiaVyrobena2;
float energiaVyrobenaCelkovo;
int32_t energiaCelkovo;
float teplotaVstupna;
float teplotaVystupna;

// History data
typedef struct HistoryData
{
  float energiaVyrobena1;
  float energiaVyrobena2;
  float energiaVyrobenaCelkovo;
  int32_t energiaCelkovo;
  float teplotaVonkajsia;
  String timestamp;
} HistoryData;
CircularBuffer<HistoryData, 10> historyData;

int historyCounter;

EthernetLinkStatus ethernetLinkStatus;

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PIN_ENA 25 // reset
#define PIN_CLK 33 //
#define PIN_DAT 32 //
// Ds1302 rtc(PIN_ENA, PIN_CLK, PIN_DAT);

virtuabotixRTC myRTC(PIN_CLK, PIN_DAT, PIN_ENA); // clk,io,ce

const int chipSelect = 5; // SD CARD
#define ETH_CS 16         // pozor nastavenie SPI nastavenie pinov v  utility/Enc28J60Network.cpp

#define SCK 18
#define MISO 19
#define MOSI 23
#define SS 16

//----------modbus RTU-----------------------------
modbus_t telegram[6];
Modbus master(0, 0, 4);

#define led 17
#define TL 26
#define reset_eth 13
#define SD_ok 27

// this is master and RS-232 or USB-FTDI
unsigned long u32wait;
int8_t state = 0;
unsigned long tempus;
uint16_t cas485; // casovac preinanie 485
//----------------------------------------------------
// data array for modbus network sharing
uint16_t au16data1[30]; // meranie tepla  CALOR 38
uint16_t au16data2[30]; // meranie vykonu 1
uint16_t au16data3[30]; // meranie vykonu 2
uint16_t au16data4[30]; // meranie vonkajsej teploty
uint32_t dataI[30];
uint8_t u8state; //!< machine state
uint8_t u8query; //!< pointer to message query
unsigned long timer;
int cas;
int casms;
int casETH;
int casSD;
int casOLED;
int casRTC;
bool komunikacia;
int32_t prirastok;

// ##########################################################################

bool connected;
bool cableConnected;
bool firstConnected = true;

/* Ethernet */
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress myDns(192, 168, 1, 1);

byte ip[] = {192, 168, 1, 55};
byte gateway[] = {192, 168, 1, 1};
byte subnet[] = {255, 255, 255, 0};

// Define IP address
IPAddress staticIP(192, 168, 1, 55);  // Change to your desired static IP
IPAddress gatewayIP(192, 168, 1, 1);  // Change to your network gateway
IPAddress subnetIP(255, 255, 255, 0); // Change to your network subnet
Firebase_StaticIP staIP(staticIP, subnetIP, gatewayIP, gatewayIP, true);

// A UDP instance to let us send and receive packets over UDP
EthernetUDP ntpUDP;

NTPClient timeClient(ntpUDP, 3600);

#define LEAP_YEAR(Y) ((Y > 0) && !(Y % 4) && ((Y % 100) || !(Y % 400)))

String displayString = "WAITING";

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

String secondsStr;
String minutesStr;
String hoursStr;
String dayStr;
String monthStr;
String yearStr;

int obrazovka;
bool tlacitko;

// Initialize WiFi
// void initWiFi()
// {
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   while (WiFi.status() != WL_CONNECTED)
//   {
//     Serial.print(".");
//     delay(300);
//   }
//   displayString = "IP " + WiFi.localIP().toString();
// }

String toStrDate(int date)
{
  return (date < 10 ? "0" : "") + String(date);
}

String getTimestamp()
{
  hoursStr = toStrDate(myRTC.hours);
  minutesStr = toStrDate(myRTC.minutes);
  secondsStr = toStrDate(myRTC.seconds);
  return hoursStr + ":" + minutesStr + ":" + secondsStr;
}

String getDateTimestamp()
{
  yearStr = String(myRTC.year);
  monthStr = toStrDate(myRTC.month);
  dayStr = toStrDate(myRTC.dayofmonth);
  return yearStr + "/" + monthStr + "/" + dayStr + "/" + getTimestamp();
}

String getActualSDPath()
{
  yearStr = String(myRTC.year);
  monthStr = toStrDate(myRTC.month);
  return "/" + yearStr + "_" + monthStr;
}

String getActualSDFilename()
{
  yearStr = String(myRTC.year);
  monthStr = toStrDate(myRTC.month);
  dayStr = toStrDate(myRTC.dayofmonth);
  return yearStr + "_" + monthStr + "_" + dayStr + ".csv";
}

// OLED
void oled1()
{
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.print(displayString);
  display.setTextSize(1);
  if (obrazovka == 0)
  {
    display.setCursor(0, 16);
    display.print("E akt: " + String(energiaAktualna));
    display.setCursor(0, 26);
    display.print("E celk: " + String(energiaCelkovo));
    display.setCursor(0, 36);
    display.print("E1+E2: " + String(energiaVyrobenaCelkovo));
    display.setCursor(0, 46);
    display.print("T vonk: " + String(teplotaVonkajsia));
  }
  else if (obrazovka == 1)
  {
    display.setCursor(0, 16);
    display.print("E 1: " + String(energiaVyrobena1));
    display.setCursor(0, 26);
    display.print("E 2: " + String(energiaVyrobena2));
    display.setCursor(0, 36);
    display.print("V 1: " + String(vykonCinny1));
    display.setCursor(0, 46);
    display.print("V 2: " + String(vykonCinny2));
  }
  else if (obrazovka == 2)
  {
    display.setCursor(0, 16);
    display.print("T vstup: " + String(teplotaVstupna));
    display.setCursor(0, 26);
    display.print("T vystup: " + String(teplotaVystupna));
  }
  display.setCursor(0, 56);
  display.print("----- " + getTimestamp() + " -----");
  display.display();
}

void getDateTime()
{
  unsigned long epoch = timeClient.getEpochTime();
  unsigned long rawTime = epoch / 86400L; // in days
  unsigned long days = 0, year = 1970;
  uint8_t month, hours, minutes, seconds;
  static const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  while ((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days = 0;
  for (month = 0; month < 12; month++)
  {
    uint8_t monthLength;
    if (month == 1)
    { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    }
    else
    {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength)
      break;
    rawTime -= monthLength;
  }
  hours = timeClient.getHours();
  minutes = timeClient.getMinutes();
  seconds = timeClient.getSeconds();

  yearStr = String(year);
  monthStr = (++month < 10 ? "0" : "") + String(month);   // jan is month 1
  dayStr = (++rawTime < 10 ? "0" : "") + String(rawTime); // day of month
  hoursStr = (hours < 10 ? "0" : "") + String(hours);
  minutesStr = (minutes < 10 ? "0" : "") + String(minutes);
  secondsStr = (seconds < 10 ? "0" : "") + String(seconds);

  if (year > 1970)
  {
    myRTC.setDS1302Time(seconds, minutes, hours, 0, rawTime, month, year);
  }
}

void setupFirebase()
{
  if (firebaseConfigReady)
    return;

  displayString = "Firebase setup";
  oled1();

  firebaseConfigReady = true;

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  /* Assign the pointer to global defined Ethernet Client object */
  fbdo.setEthernetClient(&client, mac, ETH_CS, reset_eth, &staIP); // The staIP can be assigned to the fifth param

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  // Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // while ((auth.token.uid) == "")
  // {
  //   displayString = "UID waiting";
  //   oled1();
  //   delay(1000);
  // }
  // uid = auth.token.uid.c_str();

  displayString = "Firebase OK";
  oled1();
}

void sdCardWrite()
{
  String dirPath = getActualSDPath();
  String fileNamePath = dirPath + "/" + getActualSDFilename();
  bool existsDir = SD.exists(dirPath);
  if (!existsDir)
  {
    SD.mkdir(dirPath);
  }
  bool existsFile = SD.exists(fileNamePath);
  File file = SD.open(fileNamePath, FILE_APPEND);
  if (!file)
  {
    return;
  }
  else
  {
    if (!existsFile)
    {
      file.println("cas;energiaVyrobenaCelkovo;energiaVyrobena1;energiaVyrobena2;energiaCelkovo;teplotaVonkajsia");
    }
    file.println(getTimestamp() + ";" + String(energiaVyrobenaCelkovo) + ";" + String(energiaVyrobena1) + ";" + String(energiaVyrobena2) + ";" + String(energiaCelkovo) + ";" + String(teplotaVonkajsia));
  }
  file.close();
}

void sendFirebaseDbData()
{
  String timestamp = getDateTimestamp();

  for (; historyCounter > 0; historyCounter--)
  {
    HistoryData last = historyData.pop();

    String dbDataPath = espDataPath + dataPath + "/" + String(last.timestamp);

    jsonDb.set("/energiaVyrobenaCelkovo", String(last.energiaVyrobenaCelkovo));
    jsonDb.set("/energiaVyrobena1", String(last.energiaVyrobena1));
    jsonDb.set("/energiaVyrobena2", String(last.energiaVyrobena2));
    jsonDb.set("/energiaCelkovo", String(last.energiaCelkovo));
    jsonDb.set("/teplotaVonkajsia", String(last.teplotaVonkajsia));
    jsonDb.set("/timestamp", String(last.timestamp));

    displayString = Firebase.RTDB.setJSON(&fbdo, dbDataPath.c_str(), &jsonDb) ? "OK" : fbdo.errorReason().c_str();
    if (displayString != "OK")
    {
      historyData.push(last);
      break;
    }
  }

  String dbDataPath = espDataPath + dataPath + "/" + String(timestamp);

  jsonDb.set("/energiaVyrobenaCelkovo", String(energiaVyrobenaCelkovo));
  jsonDb.set("/energiaVyrobena1", String(energiaVyrobena1));
  jsonDb.set("/energiaVyrobena2", String(energiaVyrobena2));
  jsonDb.set("/energiaCelkovo", String(energiaCelkovo));
  jsonDb.set("/teplotaVonkajsia", String(teplotaVonkajsia));
  jsonDb.set("/timestamp", String(timestamp));

  displayString = Firebase.RTDB.setJSON(&fbdo, dbDataPath.c_str(), &jsonDb) ? "OK" : fbdo.errorReason().c_str();
  jsonDb.clear();

  if (displayString == "OK")
  {
    historyCounter = 0;
  }
  else if (historyCounter < 10)
  {
    historyCounter++;
    historyData.unshift({energiaVyrobenaCelkovo, energiaVyrobena1, energiaVyrobena2, energiaCelkovo, teplotaVonkajsia, timestamp});
  }
}

void sendFirebaseLiveData()
{
  String timestamp = getDateTimestamp();

  String dbDataPath = espDataPath + actualPath;

  jsonLive.set("/energiaVyrobenaCelkovo", String(energiaVyrobenaCelkovo));
  jsonLive.set("/energiaVyrobena1", String(energiaVyrobena1));
  jsonLive.set("/energiaVyrobena2", String(energiaVyrobena2));
  jsonLive.set("/energiaCelkovo", String(energiaCelkovo));
  jsonLive.set("/energiaAktualna", String(energiaAktualna));
  jsonLive.set("/vykonCinny1", String(vykonCinny1));
  jsonLive.set("/vykonCinny2", String(vykonCinny2));
  jsonLive.set("/teplotaVonkajsia", String(teplotaVonkajsia));
  jsonLive.set("/teplotaVstupna", String(teplotaVstupna));
  jsonLive.set("/teplotaVystupna", String(teplotaVystupna));
  jsonLive.set("/timestamp", timestamp);

  bool firstSend = displayString != "OK";
  displayString = Firebase.RTDB.setJSON(&fbdo, dbDataPath.c_str(), &jsonLive) ? "OK" : fbdo.errorReason().c_str();
  jsonDb.clear();

  connected = displayString == "OK";
  if (!connected && firstSend)
  {
    casETH = 30000;
  }
  if (connected && firstConnected)
  {
    firstConnected = false;
    String dbDataPath = espDataPath + historyPath + "/" + String(timestamp);
    jsonDb.set("/timestamp", String(timestamp));
    Firebase.RTDB.setJSON(&fbdo, dbDataPath.c_str(), &jsonDb);
    jsonDb.clear();
  }
}

void initInternetConnection()
{
  ethernetLinkStatus = Ethernet.linkStatus();
  if (ethernetLinkStatus == LinkON)
  {
    cableConnected = true;
    Ethernet.begin(mac);
    delay(100);
    timeClient.begin();

    if (timeClient.update())
    {
      getDateTime();
      setupFirebase();
      connected = true;
    }
    else
    {
      displayString = "Not connected";
      connected = false;
    }
  }
  else
  {
    displayString = "Not connected";
    connected = false;
    cableConnected = false;
  }
  oled1();
}

// ###############################  S E T U P   #############################################################

void setup()
{

  pinMode(led, OUTPUT);       // led
  pinMode(reset_eth, OUTPUT); // reset ethernet
  pinMode(TL, INPUT_PULLUP);
  pinMode(SD_ok, INPUT_PULLUP);
  digitalWrite(reset_eth, true);
  digitalWrite(led, false);

  myRTC.updateTime();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); // nie 3D
  display.setRotation(0);
  display.display();
  oled1();

  delay(200);
  digitalWrite(led, true);

  if (digitalRead(SD_ok) == false)
  {
    SPI.begin(SCK, MISO, MOSI, SS);
    if (!SD.begin(chipSelect))
    {
      Serial.println("Card Mount Failed");
      return;
    }
  }
  delay(500);
  digitalWrite(led, false);

  Ethernet.init(ETH_CS);

  // give the Ethernet shield a second to initialize:
  delay(1000);

  // initWiFi();
  // if (client.connect("192.168.1.126", 9000))
  // {
  //   displayString = "Connected to " + String(client.remoteIP());
  //   // Make a HTTP request:
  //   client.println("GET / HTTP/1.1");
  //   client.println("Host: 192.168.1.126:9000");
  //   client.println("Connection: close");
  //   client.println();
  // }
  // else
  // {
  //   // if you didn't get a connection to the server:
  //   displayString = "connection failed";
  // }

  // komunikacia MDBUS RTU Master-------------------------------------------------------
  //    telegram 0: merenie vyrobeneho tepla -------------------------------------------
  telegram[0].u8id = 1;            // slave address   meranie tepla
  telegram[0].u8fct = 3;           // function code
  telegram[0].u16RegAdd = 0;       // start address in slave    bateria
  telegram[0].u16CoilsNo = 20;     // number of elements (coils or registers) to read
  telegram[0].au16reg = au16data1; // pointer to a memory array in the Arduino
  // telegram 1 -2 :  wattmeter 1 -----------------------------------------------------
  telegram[1].u8id = 2;            // slave address vykon 1
  telegram[1].u8fct = 3;           // function code
  telegram[1].u16RegAdd = 28;      // start address in slave vykon
  telegram[1].u16CoilsNo = 2;      // number of elements (coils or registers) to read
  telegram[1].au16reg = au16data2; // pointer to a memory array in the Arduino
  //------
  telegram[2].u8id = 2;                // slave address vykon 1
  telegram[2].u8fct = 3;               // function code
  telegram[2].u16RegAdd = 256;         // start address in slave celkova spotreba
  telegram[2].u16CoilsNo = 2;          // number of elements (coils or registers) to read
  telegram[2].au16reg = au16data2 + 2; // pointer to a memory array in the Arduino
  // telegram 3 - 4:  wattmeter 2 -------------------------------------------------
  telegram[3].u8id = 3;            // slave address  vykon 2
  telegram[3].u8fct = 3;           // function code
  telegram[3].u16RegAdd = 28;      // start address in slave vykon
  telegram[3].u16CoilsNo = 2;      // number of elements (coils or registers) to read
  telegram[3].au16reg = au16data3; // pointer to a memory array in the Arduino
  //------
  telegram[4].u8id = 3;                // slave address  vykon 2
  telegram[4].u8fct = 3;               // function code
  telegram[4].u16RegAdd = 256;         // start address in slave celkova spotreba wattmeter 2
  telegram[4].u16CoilsNo = 2;          // number of elements (coils or registers) to read
  telegram[4].au16reg = au16data3 + 2; // pointer to a memory array in the Arduino
  // telegram 5:  vonkajsi teplomer -----------------------------------------------
  telegram[5].u8id = 4;            // slave address vonkajsia teplota
  telegram[5].u8fct = 3;           // function code
  telegram[5].u16RegAdd = 0;       // start address in slave vykon
  telegram[5].u16CoilsNo = 3;      // number of elements (coils or registers) to read
  telegram[5].au16reg = au16data4; // pointer to a memory array in the Arduino

  master.begin(9600, SERIAL_8E1);
  master.setTimeOut(400);
  u32wait = millis() + 300;
  u8state = 0;

  initInternetConnection();

  // Get device SSID
  char c_ssid[23];
  snprintf(c_ssid, 23, "ESP32-%llX", ESP.getEfuseMac());
  ssid = String(c_ssid);

  // Update database path
  espDataPath += "/" + ssid;
}

// ########################################################################################################################
//----------------------------------------------------------------------------------------------------------------------------
//--------------------------------------H L A V N A     S L U C K A ----------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

void loop()
{
  // Ds1302::DateTime now;
  // rtc.getDateTimeTime(&now);

  Ethernet.maintain(); // pri DHCP

  if (ethernetLinkStatus != Ethernet.linkStatus() || (myRTC.dayofmonth == 0))
  {
    displayString = "Restarting" + String(Ethernet.linkStatus());
    delay(3000);
    ESP.restart();
  }

  if (connected == true || (connected == false && cableConnected == true))
  {
    casETH = 0;
  }
  if (casETH > 30000)
  {
    casETH = 0;
    digitalWrite(reset_eth, false);
    delay(100);
    digitalWrite(reset_eth, true);
    delay(100);
    initInternetConnection();
  }

  if (digitalRead(SD_ok) == true)
  {
    casSD = 0;
  }
  if (casSD > 60000)
  {
    casSD = 0;
    if (SD.begin(chipSelect))
    {
      sdCardWrite();
      SD.end();
    }
  }

  //--------------------OLED----------------------
  if (casRTC >= 80)
  {
    casRTC = 0;
    myRTC.updateTime();
  }

  if (casOLED >= 100)
  {
    casOLED = 0;
    oled1();
  }

  if (timerActualData > TIME_ACTUAL_DATA)
  {
    timerActualData = 0;
    if (connected)
    {
      if (!firebaseConfigReady)
      {
        setupFirebase();
      }
      if (Firebase.ready())
      {
        sendFirebaseLiveData();
      }
    }
    else
    {
      if (Ethernet.linkStatus() == LinkON && timeClient.update())
      {
        setupFirebase();
        connected = true;
      }
    }
  }

  if (timerData > TIME_DATA)
  {
    timerData = 0;
    if (connected)
    {
      if (!firebaseConfigReady)
      {
        setupFirebase();
      }
      if (Firebase.ready())
      {
        sendFirebaseDbData();
      }
    }
  }

  //---------------casovace ------------------------------------
  if (millis() - timer >= 1)
  {
    prirastok = (millis() - timer);
    cas = cas + prirastok;
    cas_com = cas_com + prirastok;
    cas485 = cas485 + prirastok;
    casms = casms + prirastok;
    casETH = casETH + prirastok;
    casSD = casSD + prirastok;
    casOLED = casOLED + prirastok;
    casRTC = casRTC + prirastok;
    timerData = timerData + prirastok;
    timerActualData = timerActualData + prirastok;
    timer = millis();
    if (casms >= 1000)
    {
      casms = 0;
    }
  }
  //-----------------------------------------------------------
  // led blikanie
  if (cas > 600 && komunikacia == true)
  {
    digitalWrite(led, !digitalRead(led));
    cas = 0;
  }
  if (cas > 100 && komunikacia == false)
  {
    digitalWrite(led, !digitalRead(led));
    cas = 0;
  }

  //-------  meranie tepla ---------------------------------
  // Mb.MBHoldingRegister[0] = au16data1[2];  //  energia suma H   uinteger long
  // Mb.MBHoldingRegister[1] = au16data1[3];  //  energia suma   L -/-
  // Mb.MBHoldingRegister[2] = au16data1[4];  //  dT min   H       uinteger long
  // Mb.MBHoldingRegister[3] = au16data1[5];  //  dT min   L       -/-
  // Mb.MBHoldingRegister[4] = au16data1[12]; // energia   H    uinteger long
  // Mb.MBHoldingRegister[5] = au16data1[13]; // energia   L       -/-
  // Mb.MBHoldingRegister[6] = au16data1[14]; // teplota vstupna H   0.01C   integer long
  // Mb.MBHoldingRegister[7] = au16data1[15]; // teplota vstupna L   -/-
  // Mb.MBHoldingRegister[8] = au16data1[16]; // teplota vystupna H  0,01C   integer long
  // Mb.MBHoldingRegister[9] = au16data1[17]; // teplota vystupna L  -/-
  // //-------  meranie vykonu 1 ---------------------------------
  // Mb.MBHoldingRegister[10] = au16data2[0]; //  cinny vykon H    float  .001
  // Mb.MBHoldingRegister[11] = au16data2[1]; //  cinny vykon L     -/-
  // Mb.MBHoldingRegister[12] = au16data2[2]; // cinna vyrobena energia H  float .01
  // Mb.MBHoldingRegister[13] = au16data2[3]; // cinna  vyrobena energie L float
  // //-------  meranie vykonu 2 ---------------------------------
  // Mb.MBHoldingRegister[14] = au16data3[0]; //  cinny vykon H    float .001
  // Mb.MBHoldingRegister[15] = au16data3[1]; //  cinny vykon L     -/-
  // Mb.MBHoldingRegister[16] = au16data3[2]; //  vyrobena energia H  float .01
  // Mb.MBHoldingRegister[17] = au16data3[3]; //  vyrobena energie L -/-
  // //-------  meranie vonkajsej teploty ---------------------------------
  // Mb.MBHoldingRegister[18] = au16data4[0]; //  teplota vonkasia int  0,1C
  // Mb.MBHoldingRegister[19] = au16data4[1]; //  teplota vonkasia int  0,1C
  //----------------------------------------------------------------------

  // prepocty
  uint32_t hodnotaI32;
  hodnotaI32 = (au16data2[0] << 16 | au16data2[1]);
  vykonCinny1 = *(float *)&hodnotaI32 * 1000;

  hodnotaI32 = (au16data2[2] << 16 | au16data2[3]);
  energiaVyrobena1 = *(float *)&hodnotaI32 * 100;

  hodnotaI32 = (au16data3[0] << 16 | au16data3[1]);
  vykonCinny2 = *(float *)&hodnotaI32 * 1000;

  hodnotaI32 = (au16data3[2] << 16 | au16data3[3]);
  energiaVyrobena2 = *(float *)&hodnotaI32 * 100;

  energiaCelkovo = (au16data1[2] << 16 | au16data1[3]);
  energiaAktualna = (au16data1[12] << 16 | au16data1[13]);

  teplotaVstupna = (au16data1[14] << 16 | au16data1[15]) * 0.01;
  teplotaVystupna = (au16data1[16] << 16 | au16data1[17]) * 0.01;

  energiaVyrobenaCelkovo = energiaVyrobena1 + energiaVyrobena2;

  teplotaVonkajsia = au16data4[0] * 0.1;

  //--------------------------------  MODBUS RTU KOMUNIKACIA -----------------------------------------------------------------
  switch (u8state)
  {
  case 0:
    if (millis() > u32wait)
      u8state++; // wait state
    break;
  case 1:
    master.query(telegram[u8query]); // send query (only once)
    u8state++;
    u8query++;
    if (u8query > 5)
      u8query = 0;
    break;
  case 2:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE)
    {
      u8state = 0;
      u32wait = millis() + 200;
    }
    break;
  }

  //--------------------------------------------------------------------------------------------------------------------------
  // komunikacia RTU
  if (cas_com > 2000)
  {
    komunikacia = false;
  }
  else
  {
    komunikacia = true;
  } // casovac nulovany v kniznic Modbusrtu.h nz riadku 666 ak prijma data
    //  ak neprijma data casovac rastie

  //--------------------------  modbusRtu.h RS485 riadenie Rx Tx  -----------------
  if (vyp == LOW)
  {
    cas485 = 0;
  }
  if (cas485 >= 1)
  {
    vyp = LOW;
    digitalWrite(4, LOW);
  }

  // tlacitko
  if (digitalRead(TL) == LOW)
  {
    tlacitko = true;
  }
  else
  {
    if (tlacitko)
    {
      tlacitko = false;
      obrazovka = ++obrazovka % 3;
    }
  }
}
