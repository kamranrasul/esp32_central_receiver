/*********
  
  Kamran R.
  Waqas M.
  Nabil E.

  Robbin Law
  Rui Santos
  
*********/

#include <Arduino.h>  // base library
#include <esp_now.h>  // esp now library
#include <WiFi.h>     // WiFi connectivity
#include "time.h"     // time update from ntp

// Local WiFi Credentials
#define WIFI_SSID "YOUR SSID"
#define WIFI_PASS "YOUR PASS"

// time variable setup
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25362;
const int daylightOffset_sec = 3600;

// for updates between the webserver and central esp32
#define RXD2 16
#define TXD2 17

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// delaying function calls
unsigned long webDelay = millis();

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
  Serial2.print(" ");   // spacer

  Serial2.print(boardsStruct.humidity);
  Serial2.print(" ");   // spacer

  Serial2.print(boardsStruct.pressure);
  Serial2.print(" ");   // spacer

  Serial2.print(boardsStruct.altitude);
  Serial2.print(" ");   // spacer

  for (int i = 0; i < 4; i++)
  {
    Serial2.print(boardsStruct.pinStatus[i]);
    Serial2.print(" ");   // spacer
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

void setup()
{
  // initialize Serial Monitor with computer
  Serial.begin(115200);

  // initialize Serial Communication with Webserver
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);

  // get time from internet
  timeSetup();

  // setting up esp NOW
  espNowSetup();
}

void loop()
{
}