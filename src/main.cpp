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
#include "clock.h"         // for time display on LCD

// Local WiFi Credentials
const char *WIFI_SSID = "YOUR WIFI SSID";
const char *WIFI_PASS = "YOUR WIFI PASS";

// time variable setup
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25362;
const int daylightOffset_sec = 3600;

// for updates between the IO and central esp32
#define RXD2 16
#define TXD2 17

// countdown initializer on display
#define countInit 21

// scheduler initializer
#define tftRefreshTime 10000
#define countDownTImer 1000

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
void clock_update();

// Setup tasks for the task scheduler
Task dataDisplayTFT(tftRefreshTime, TASK_FOREVER, &tftDisplay);
Task dataScheduler(countDownTImer, TASK_FOREVER, &countDownTimer);
Task clockUpdate(countDownTImer, TASK_FOREVER, &clock_update);

// Create the scheduler
Scheduler runner;

// count down timer
int count = countInit;

// for switching display
bool switchDisp = true;

// storing last status of pins
int storedPin[4] = {0, 0, 0, 0};

// tft is global to this file only
TFT_eSPI tft = TFT_eSPI();

// time struct for time update
struct tm timeinfo;

// for tft background and font-background color
uint16_t bg = TFT_BLACK;
uint16_t fg = TFT_WHITE;

// Must match the sender receiver structure
typedef struct struct_message
{
  int id;            // must be unique for each sender board

  // controller pin state
  int pinStatus[4];  // for peripheral status

  // for BME Chip
  float temperature; // for storing temperature
  float humidity;    // for storing himmidity
  float pressure;    // for storing pressure
  float altitude;    // for storing altitude

  // for MPU Chip
  float temp6050;    // for storing onboard temperature
  float A_values[3]; // for storing accelrometer values x, y, z
  float G_values[3]; // for storing gyroscope values x, y, z
} struct_message;

// Create an array with all the structures
struct_message boardsStruct;

// setup for esp32
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

  // setting up tft
  tftSetup();

  // Start the task scheduler
  runner.init();

  // add task to the scheduler
  runner.addTask(dataDisplayTFT);
  runner.addTask(dataScheduler);
  runner.addTask(clockUpdate);

  // enabling the schedulers
  dataDisplayTFT.enable();
  dataScheduler.enable();
  clockUpdate.enable();
  //runner.enableAll();

  // setting up esp NOW
  espNowSetup();
}

// looping  forever
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
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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

  // displaying the controller pin status
  Serial.println();
  Serial.println("*** Controller Values ***");

  Serial.printf("Control 14 is %3s.", boardsStruct.pinStatus[0] ? "ON" : "OFF");
  Serial.println();

  for (int i = 1; i < 4; i++)
  {
    Serial.printf("Control %d is %3s.", i + 24, boardsStruct.pinStatus[i] ? "ON" : "OFF");
    Serial.println();
  }

  // displaying BME280 values on the serial output
  Serial.println();
  Serial.println("*** BME280 Values ***");
  Serial.printf("Temperature:   %-6.2f °C", boardsStruct.temperature);
  Serial.println();
  Serial.printf("Humidity:      %-6.2f %%", boardsStruct.humidity);
  Serial.println();
  Serial.printf("Pressure:      %-6.2f hPa", boardsStruct.pressure);
  Serial.println();
  Serial.printf("Altitude:      %-6.2f m", boardsStruct.altitude);
  Serial.println();

  // displaying MPU6050 values on the serial output
  Serial.println();
  Serial.println("*** MPU6050 Values ***");
  Serial.printf("Temperature:   %-6.2f °C", boardsStruct.temp6050);
  Serial.println();
  Serial.printf("Acceleration   X: %5.2f, Y: %5.2f, Z: %5.2f   m/s^2", boardsStruct.A_values[0], boardsStruct.A_values[1], boardsStruct.A_values[2]);
  Serial.println();
  Serial.printf("Rotation       X: %5.2f, Y: %5.2f, Z: %5.2f   rad/s", boardsStruct.G_values[0], boardsStruct.G_values[1], boardsStruct.G_values[2]);
  Serial.println();

  // sending BME values to IO server
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

  // sending MPU6050 values to IO server
  Serial2.print(boardsStruct.temp6050);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.A_values[0]);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.A_values[1]);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.A_values[2]);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.G_values[0]);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.G_values[1]);
  Serial2.print(" "); // spacer

  Serial2.print(boardsStruct.G_values[2]);
  Serial2.print(" "); // spacer

  Serial.println("\n*** Sent to IO IC ***");

  delay(500);

  // checking the status of the receiver
  if(Serial2.available())
  {
    Serial.println();
    Serial.print("Last Packet Send Status to IO IC:        ");
    if(Serial2.parseInt() == 200)
    {
      Serial.println("Delivery Success");
    }
    else
    {
      Serial.println("Delivery Failure");
    }
    Serial.println();
  }
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

// time function
void printLocalTime()
{
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "\n%A, %B %d, %Y %H:%M:%S");
}

// count down timer on the tft
void countDownTimer()
{
  count = count == 0 ? countInit : count;
  switchDisp = (count == 1 || count == 11) ? !switchDisp : switchDisp;

  // debugging on Serial Port
  //Serial.printf("Count: %d\n\r", count);
  //Serial.printf("Switch Disp: %d\n\r", int(switchDisp));

  tft.fillRect(279, 4, 26, 18, bg);
  tft.setTextColor(fg, bg);
  tft.setCursor(280, 5);
  tft.print(--count == -1 ? countInit : count);
}

// displaying on tft
void tftDisplay()
{
  // displaying on the TFT
  tft.fillScreen(bg);
  tft.setCursor(5, 5);
  tft.setTextColor(fg, bg);

  tft.loadFont("NotoSansBold20");
  tft.print("Right now, next update in: ");
  tft.fillRect(5, 100, 30, 30, bg);

  tft.setTextColor(TFT_YELLOW, bg);

  // Create TTF fonts using instructions at https://pages.uoregon.edu/park/Processing/process5.html
  if (switchDisp)
  {
    Serial.printf("Displaying BME280 on the screen, @ %lu...", millis());
    Serial.println();
    
    // Temperature
    tft.fillRect(10, 30, 250, 30, bg);
    tft.setCursor(10, 30);
    tft.printf("Temperature:");
    tft.setCursor(160, 30);
    tft.print(boardsStruct.temperature);
    tft.println("  °C");

    // Humidity
    tft.fillRect(10, 50, 250, 30, bg);
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

    // INPUT 25
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
  else // for MPU6050
  {
    Serial.printf("Displaying MPU6050 on the screen, @ %lu...", millis());
    Serial.println();

    // Temperature
    tft.fillRect(10, 30, 250, 30, bg);
    tft.setCursor(10, 30);
    tft.print("Temperature: ");
    tft.print(boardsStruct.temp6050);
    tft.println(" °C");

    // Accelerometer and Gyroscope
    tft.fillRect(10, 60, 250, 30, bg);
    tft.setCursor(10, 60);
    tft.println("Accelerometer Values: (m/s^2)");
    tft.fillRect(10, 90, 250, 30, bg);
    tft.print("   AcX: ");
    tft.println(boardsStruct.A_values[0]);

    tft.fillRect(10, 120, 250, 30, bg);
    tft.print("   AcY: ");
    tft.println(boardsStruct.A_values[1]);

    tft.fillRect(10, 150, 250, 30, bg);
    tft.print("   AcZ: ");
    tft.println(boardsStruct.A_values[2]);

    tft.fillRect(10, 180, 250, 30, bg);
    tft.println(" Gyroscope Values: (rad/s)");
    tft.print("   GyX: ");
    tft.println(boardsStruct.G_values[0]);

    tft.fillRect(10, 210, 250, 30, bg);
    tft.print("   GyY: ");
    tft.println(boardsStruct.G_values[1]);

    tft.fillRect(10, 240, 250, 30, bg);
    tft.print("   GyZ: ");
    tft.println(boardsStruct.G_values[2]);
  }
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
      count = countInit;

      // assigning the previous values
      for (int j = 0; j < 4; j++)
      {
        storedPin[j] = boardsStruct.pinStatus[j];
      }
    }
  }
}

// for clock updates
void clock_update()
{
  refresh_clock(&tft, &timeinfo);
}
