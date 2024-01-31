

/**
  Modul ID ESP32
  MODBUS RTU NA MODBUS ETHERNET
  RS485
  pozor pri ulozeni do ineho adresara prekopirovat aj adresar utility
 */

#include <Arduino.h>
#include "EthernetENC.h"
#include "ModbusTCPSlave.h"
#include "esp_system.h"
#include "ModbusRtu.h" //komunikacia modbus slave
#include <SPI.h>
#include <SD.h>
// #include <Ds1302.h>
// #include <SSD1306.h>
// SSD1306  display(0x3c, 21, 22);    //SDA SCL

#include <virtuabotixRTC.h>

#include <Adafruit_GFX.h>
#include "Adafruit_SSD1306.h"

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

uint32_t vykonI32;
uint16_t vykonI16;
float vykonR;

uint32_t energiaI32;
uint16_t energiaI16;
float energiaR;
int smernik;
uint16_t rok;
uint16_t mesiac;
uint16_t den;
uint16_t hodiny;
uint16_t minuty;
uint16_t sekundy;
uint16_t couter;
// ##########################################################################

// bool connected;
/* Ethernet */
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress myDns(192, 168, 1, 1);

byte ip[] = {192, 168, 1, 55};
byte gateway[] = {192, 168, 1, 1};
byte subnet[] = {255, 255, 255, 0};

//------------- modbus TCP IP ---------------------
ModbusTCPSlave Mb;

String displayString = "WAITING";

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

//--------------------------  O L E D   ----------------------------------------------------------------------------
void oled1()
{
  /*  display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(10, 0, "skuska"+String(cas));
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);              //velkost textu
    display.drawString(0, 20, " millis: "+ String(millis()));
 // display.drawString(0, 31, "Vykon2: "+ String(Vykon2,1)+ " W" + "  r2:" + String(regVYKON2)+"%");
    display.drawString(0, 42, "cas:"+ String(rok)+ "/" + String(mesiac)+"/"+ String(den)+ ":" + String(hodiny)+":"+ String(minuty)+":"+ String(sekundy));
 //   display.drawString(0, 53, "Vykon:  "+ String(VykonP,1)+ " W" + "  r:" +  String(regVYKON)+ "%");
    display.display(); */
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.print(displayString);
  display.setTextSize(2);
  display.setCursor(20, 32);
  display.print(String(hodiny) + ":" + String(minuty) + ":" + String(sekundy));
  display.setTextSize(1);
  display.setCursor(10, 55);
  display.print("couter= " + String(couter));
  display.display();
}
// ###############################  S E T U P   #############################################################

void setup()
{

  pinMode(led, OUTPUT);       // led
  pinMode(reset_eth, OUTPUT); // reset ethernet
  pinMode(TL, INPUT_PULLUP);
  pinMode(SD_ok, INPUT_PULLUP);
  digitalWrite(reset_eth, true);

  // start the Ethernet connection
  digitalWrite(led, true);
  delay(500);
  digitalWrite(led, false);

  myRTC.updateTime();
  Mb.MBHoldingRegister[0] = au16data1[0] = rok = myRTC.year;        //
  Mb.MBHoldingRegister[1] = au16data1[1] = mesiac = myRTC.month;    //
  Mb.MBHoldingRegister[2] = au16data1[2] = den = myRTC.dayofmonth;  // priradenie
  Mb.MBHoldingRegister[3] = au16data1[3] = hodiny = myRTC.hours;    // priradenie
  Mb.MBHoldingRegister[4] = au16data1[4] = minuty = myRTC.minutes;  // priradenie
  Mb.MBHoldingRegister[5] = au16data1[5] = sekundy = myRTC.seconds; // priradenie
  //----- spustenie OLED terminalu ---------------------
  /*  display.init();   //oled
    display.flipScreenVertically(); //orientacia textu
     display.display();
    oled1();*/
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

  Ethernet.begin(mac, ip, myDns);

  // give the Ethernet shield a second to initialize:
  delay(1000);

  if (client.connect("192.168.1.126", 9000))
  {
    displayString = "Connected to " + String(client.remoteIP());
    // Make a HTTP request:
    client.println("GET / HTTP/1.1");
    client.println("Host: 192.168.1.126:9000");
    client.println("Connection: close");
    client.println();
  }
  else
  {
    // if you didn't get a connection to the server:
    displayString = "connection failed";
  }

  Mb.begin();
  //  rtc.init();

  // komunikacia MDBUS RTU Master-------------------------------------------------------
  //    telegram 0: merenie vyrobeneho tepla -------------------------------------------
  telegram[0].u8id = 1;            // slave address   meranie tepla
  telegram[0].u8fct = 16;          // function code
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
  // nastavenie raalneho casu
  /*  Ds1302::DateTime dt = {
             .year = 24,
             .month = 1,
             .day = 23,
             .hour =17,
             .minute = 28,
             .second = 0,
             .dow = 2
         };

         rtc.setDateTime(&dt);
 */
  // seconds, minutes, hours, day of the week, day of the month, month, year
  if (rok == 2000)
  {
    myRTC.setDS1302Time(00, 40, 10, 1, 29, 1, 2024);
    oled1();
  }
  //------------------------------------------------------------------------------------
}
// ###############################  S E T U P   #############################################################

// ########################################################################################################################
//----------------------------------------------------------------------------------------------------------------------------
//--------------------------------------H L A V N A     S L U C K A ----------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

void loop()
{

  // Ds1302::DateTime now;
  // rtc.getDateTime(&now);

  if (digitalRead(SD_ok) == true)
  {
    casSD = 0;
  }
  if (casSD > 10000)
  {
    casSD = 0;
    // SD.begin(chipSelect);
    /*  File file3 = SD.open("/" + String(now.minute) +".csv", FILE_APPEND);  //zapisuje do riadkov
     if (!file3) {return;}
        else
        {
         file3.println("20" + String(now.year)+"/"+String(now.month)+ "/"+String(now.day) + ":"+String(now.hour)
          + ":"+String(now.minute)+ ":"+String(now.second)+ ";"+ String(millis()) );
          }
       file3.close();
    }
    */
    File file3 = SD.open("/" + String(myRTC.year) + "_" + String(myRTC.month) + "_" + String(myRTC.dayofmonth) + "_" + String(myRTC.hours) + "_" + String(myRTC.minutes) + ".csv", FILE_APPEND); // zapisuje do riadkov
    if (!file3)
    {
      return;
    }
    else
    {
      file3.println(String(myRTC.year) + "/" + String(myRTC.month) + "/" + String(myRTC.dayofmonth) + ":" + String(myRTC.hours) + ":" + String(myRTC.minutes) + ";" + String(millis()));
    }
    file3.close();
  }

  couter++;
  //--------------------OLED----------------------
  if (casRTC >= 80)
  {
    casRTC = 0;
    myRTC.updateTime();
    Mb.MBHoldingRegister[0] = au16data1[0] = rok = myRTC.year;        //
    Mb.MBHoldingRegister[1] = au16data1[1] = mesiac = myRTC.month;    //
    Mb.MBHoldingRegister[2] = au16data1[2] = den = myRTC.dayofmonth;  // priradenie
    Mb.MBHoldingRegister[3] = au16data1[3] = hodiny = myRTC.hours;    // priradenie
    Mb.MBHoldingRegister[4] = au16data1[4] = minuty = myRTC.minutes;  // priradenie
    Mb.MBHoldingRegister[5] = au16data1[5] = sekundy = myRTC.seconds; // priradenie */
  }

  if (casOLED >= 100)
  {
    casOLED = 0;
    oled1();
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

  //----  modbus tcp ---------------------------------
  //----  modbus tcp ---------------------------------
  Mb.Run();
  //-------  meranie tepla ---------------------------------
  // Mb.MBHoldingRegister[0]=au16data1[2];  //  energia suma H   uinteger long
  // Mb.MBHoldingRegister[1]=au16data1[3];  //  energia suma   L -/-
  // Mb.MBHoldingRegister[2]=au16data1[4];  //  dT min   H       uinteger long
  // Mb.MBHoldingRegister[3]=au16data1[5];  //  dT min   L       -/-
  // Mb.MBHoldingRegister[4]=au16data1[12]; // energia   H    uinteger long
  // Mb.MBHoldingRegister[5]=au16data1[13]; // energia   L       -/-
  Mb.MBHoldingRegister[6] = au16data1[14]; // teplota vstupna H   0.01C   integer long
  Mb.MBHoldingRegister[7] = au16data1[15]; // teplota vstupna L   -/-
  Mb.MBHoldingRegister[8] = au16data1[16]; // teplota vystupna H  0,01C   integer lon
  Mb.MBHoldingRegister[9] = au16data1[17]; // teplota vystupna L  -/-
                                           //-------  meranie vykonu 1 ---------------------------------
  Mb.MBHoldingRegister[10] = au16data2[0]; //  cinny vykon H    float  .001
  Mb.MBHoldingRegister[11] = au16data2[1]; //  cinny vykon L     -/-
  Mb.MBHoldingRegister[12] = au16data2[2]; // cinna vyrobena energia H  float .01
  Mb.MBHoldingRegister[13] = au16data2[3]; // cinna  vyrobena energie L float
                                           //-------  meranie vykonu 2 ---------------------------------
  Mb.MBHoldingRegister[14] = au16data3[0]; //  cinny vykon H    float .001
  Mb.MBHoldingRegister[15] = au16data3[1]; //  cinny vykon L     -/-
  Mb.MBHoldingRegister[16] = au16data3[2]; //  vyrobena energia H  float .01
  Mb.MBHoldingRegister[17] = au16data3[3]; //  vyrobena energie L -/-
  //-------  meranie vonkajsej teploty ---------------------------------
  Mb.MBHoldingRegister[18] = au16data4[0]; //  teplota vonkasia int  0,1C
  Mb.MBHoldingRegister[19] = au16data4[1]; //  teplota vonkasia int  0,1C
  //----------------------------------------------------------------------

  /*
   Mb.MBHoldingRegister[0]=au16data1[0]=rok=now.year;   //
   Mb.MBHoldingRegister[1]=au16data1[1]=mesiac=now.month;  //
   Mb.MBHoldingRegister[2]=au16data1[2]=den=now.day; //priradenie
   Mb.MBHoldingRegister[3]=au16data1[3]=hodiny=now.hour; //priradenie
   Mb.MBHoldingRegister[4]=au16data1[4]=minuty=now.minute; //priradenie
   Mb.MBHoldingRegister[5]=au16data1[5]=sekundy=now.second; //priradenie
   */

  // prepocty
  vykonI32 = (au16data2[0] << 16 | au16data2[1]);
  vykonR = *(float *)&vykonI32 * 1000;
  vykonI16 = vykonR;

  energiaI32 = (au16data2[2] << 16 | au16data2[3]);
  energiaR = *(float *)&energiaI32 * 100;
  energiaI16 = energiaR;

  //--------------------------------  MODBUS RTU KOMUNIKACIA -----------------------------------------------------------------------------
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

  //--------------------------------------------------------------------------------------------------------------------------------------
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
  //--------------------------------------------------------------------------------------------------------------------
}
