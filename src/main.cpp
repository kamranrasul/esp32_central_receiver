/*********
  
  Kamran R.
  Waqas M.
  Nabil E.

  Robbin Law
  Rui Santos
  
*********/

#include <Arduino.h>       // base library
#include <esp_now.h>       // esp now library
#include <WiFi.h>          // WiFi connectivity
#include <TFT_eSPI.h>      // for tft display
#include "SPIFFS.h"        // for file flash system upload
#include "TaskScheduler.h" // for mimic delay
#include "time.h"          // time update from ntp

// Local WiFi Credentials
const char *ssid = "Hidden_network";
const char *password = "pak.awan.pk";

// time variable setup
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25362;
const int daylightOffset_sec = 3600;

// for updates between the webserver and central esp32
#define RXD2 16
#define TXD2 17

// declaration of functions for setup
void initSPIFFS();
void timeSetup();
void tftSetup();
void espNowSetup();

// declaration of functions for performing
void countDownTimer();
void printLocalTime();
void tftDisplay();
void detectChange();

// Setup tasks for the task scheduler
Task dataDisplayTFT(9800, TASK_FOREVER, &tftDisplay);
Task dataScheduler(1000, TASK_FOREVER, &countDownTimer);

// Create the scheduler
Scheduler runner;

// count down timer
int count = 10;

// storing status of pins
int storedPin[4] = {0, 0, 0, 0};

// tft is global to this file only
TFT_eSPI tft = TFT_eSPI();

// for tft background and font-background color
uint16_t bg = TFT_BLACK;
uint16_t fg = TFT_WHITE;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message
{
  int id; // must be unique for each sender board
  int pinStatus[4];
  float temperature;
  float humidity;
  float pressure;
  float altitude;
} struct_message;

// Create an array with all the structures
struct_message boardsStruct;

void setup()
{
  // initialize Serial Monitor with computer
  Serial.begin(115200);

  // initialize Serial Communication with Webserver
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  // loading Flash File System
  initSPIFFS();

  // get time from internet
  timeSetup();

  // Start the task scheduler
  runner.init();

  // add task to the scheduler
  runner.addTask(dataDisplayTFT);
  runner.addTask(dataScheduler);

  // enabling the schedulers
  dataDisplayTFT.enable();
  dataScheduler.enable();

  // setting up esp NOW
  espNowSetup();
}

void loop()
{
  // Execute the scheduler runner
  runner.execute();

  // detects any change on the inputs and respond accordingly
  detectChange();
}

// time setup function
void timeSetup()
{
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

// time function
void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "\n%A, %B %d, %Y %H:%M:%S");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  // display time
  printLocalTime();

  Serial.print("\nMy MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println();

  // Copies the sender mac address to a string
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&boardsStruct, incomingData, sizeof(boardsStruct));

  Serial.printf("Board ID %u: %u bytes", boardsStruct.id, len);
  Serial.println();
  Serial.println();
  Serial.printf("Temperature: %-6.2f °C", boardsStruct.temperature);
  Serial.println();
  Serial.printf("Humidity:    %-6.2f %%", boardsStruct.humidity);
  Serial.println();
  Serial.printf("Pressure:    %-6.2f hPa", boardsStruct.pressure);
  Serial.println();
  Serial.printf("Altitude:    %-6.2f m", boardsStruct.altitude);
  Serial.println();
  Serial.println();

  Serial.printf("Control 14 is %3s.", boardsStruct.pinStatus[0] ? "ON" : "OFF");
  Serial.println();

  for (int i = 1; i < 4; i++)
  {
    Serial.printf("Control %d is %3s.", i + 24, boardsStruct.pinStatus[i] ? "ON" : "OFF");
    Serial.println();
  }

  // sending to webserver
  Serial2.print(boardsStruct.temperature);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.humidity);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.pressure);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.altitude);
  Serial2.print(" "); // spacer

  for (int i = 0; i < 4; i++)
  {
    Serial2.print(boardsStruct.pinStatus[i]);
    Serial2.print(" "); // spacer
  }

  Serial.println("\n*** Sent to Webserver ***");
}

// setting up esp NOW
void espNowSetup()
{
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
}

// SPIFFS Initialization
void initSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("Cannot mount SPIFFS volume...");
    while (1)
      ; // infinite loop
  }
  else
  {
    Serial.println("SPIFFS volume mounted properly");
  }
}

// setting up tft
void tftSetup()
{
  // Setup the TFT
  tft.begin();
  tft.setRotation(3);
  tft.loadFont("NotoSansBold20");
  tft.setTextColor(fg, bg);
  tft.fillScreen(bg);
  tft.setCursor(0, 0);
  tft.println("Hello!");
  tft.println("Searching for the sensor...");
}

// count down timer on the tft
void countDownTimer()
{
  tft.fillRect(279, 4, 13, 18, bg);
  tft.setCursor(280, 5);
  tft.print(--count == -1 ? 9 : count);
}

// displaying on tft
void tftDisplay()
{
  // If you set this, the TFT will not work!!!
  count = 10;

  uint16_t bg = TFT_BLACK;
  uint16_t fg = TFT_WHITE;

  // displaying on the TFT
  tft.setCursor(5, 5);
  tft.setTextColor(fg, bg);
  // Create TTF fonts using instructions at https://pages.uoregon.edu/park/Processing/process5.html
  tft.loadFont("NotoSansBold20");
  tft.print("Right now, next update in: ");
  tft.fillRect(5, 100, 30, 30, bg);
  //  tft.setCursor(5, 100);
  //  tft.println(count);

  tft.setTextColor(TFT_YELLOW, bg);

  // Temperature
  tft.fillRect(10, 30, 250, 30, bg);
  tft.setCursor(10, 30);
  tft.printf("Temperature:");
  tft.setCursor(160, 30);
  tft.print(boardsStruct.temperature);
  tft.println("  °C");

  // Humidity
  tft.fillRect(10, 45, 250, 30, bg);
  tft.setCursor(10, 55);
  tft.print("Humidity:");
  tft.setCursor(160, 55);
  tft.print(boardsStruct.humidity);
  tft.println("   %");

  // Pressure
  tft.fillRect(10, 80, 250, 30, bg);
  tft.setCursor(10, 80);
  tft.print("Pressure:");
  tft.setCursor(160, 80);
  tft.print(boardsStruct.pressure);
  tft.println(" hPa");

  // Appx altitude
  tft.fillRect(10, 105, 250, 30, bg);
  tft.setCursor(10, 105);
  tft.print("Altitude:");
  tft.setCursor(160, 105);
  tft.print(boardsStruct.altitude);
  tft.println("   m");

  // INPUT 14
  tft.fillRect(10, 130, 250, 30, bg);
  tft.setCursor(10, 130);
  tft.print("Light:");
  tft.setCursor(160, 130);
  tft.println(boardsStruct.pinStatus[0] ? "ON" : "OFF");

  // INPUT 210
  tft.fillRect(10, 155, 250, 30, bg);
  tft.setCursor(10, 155);
  tft.print("Furnace:");
  tft.setCursor(160, 155);
  tft.println(boardsStruct.pinStatus[1] ? "ON" : "OFF");

  // INPUT 26
  tft.fillRect(10, 180, 250, 30, bg);
  tft.setCursor(10, 180);
  tft.print("Exhaust:");
  tft.setCursor(160, 180);
  tft.println(boardsStruct.pinStatus[2] ? "ON" : "OFF");

  // INPUT 27
  tft.fillRect(10, 205, 250, 30, bg);
  tft.setCursor(10, 205);
  tft.print("Humidifier:");
  tft.setCursor(160, 205);
  tft.println(boardsStruct.pinStatus[3] ? "ON" : "OFF");
}

// kind of interrupt function
void detectChange()
{
  // checking the input change
  for (int i = 0; i < 4; i++)
  {
    if (boardsStruct.pinStatus[i] != storedPin[i])
    {
      Serial.println("Change Detected...");
      // Disable the tasks
      dataDisplayTFT.disable();
      dataScheduler.disable();

      // Enable the task
      dataDisplayTFT.enable();
      dataScheduler.enable();
    }
  }
}