// esp32 Portainer Monitor
//
// Copyright Rob Latour, 2024
// License: MIT
//
// https://github.com/roblatour/esp32PortainerMonitor
//
// Compile and upload using Arduino IDE (2.3.4 or greater)
//
// Physical board:                        ESP32 with built in 2.8 inch TFT display ( ESP32-2432S028R ) - https://www.aliexpress.com/item/1005004502250619.html
// Board in Arduino board manager:        ESP32 Dev Module
//
// Arduino Tools settings:
//
// USB CDC On Boot:                       "Enabled"
// USB DFU On Boot:                       "Disabled"

// CPU Frequency:                         "240MHz (WiFi)"
// Core Debug Level:                      "None"
// Erase All Flash Before Sketch Upload:  "Disabled"
// Events Run On:                         "Core 1"
// Flash Frequency                        "80MHz"
// Flash Mode:                            "QI0"
// Flash Size                             "4MB (32Mb)"               
// JTAG Adapter                           "Disabled"
// Arudino Runs On                        "Core 1"
// USB Firmware MSCOn Boot:               "Disabled"
// Partition Scheme:                      "Default 4BB with spiffs (1.285MB APP/1.5MB SPIFFS)"
// PSRAM:                                 "Disabled"
// Upload Speed:                          "921600"
//
// ----------------------------------------------------------------
// Programmer                             ESPTool
// ----------------------------------------------------------------

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>              // download from: http://pan.jczn1688.com/directlink/1/ESP32%20module/2.8inch_ESP32-2432S028R.rar

#include "pin_config.h"         
#include "virtual_window.h"

#include <bb_spi_lcd.h>            // download from: https://github.com/bitbank2/bb_spi_lcd
BB_SPI_LCD touchPanel;

#include "user_secrets.h"
#include "user_settings.h"

const char* WiFi_SSID = SECRET_WIFI_SSID;
const char* WiFi_Password = SECRET_WIFI_PASSWORD;

String serverBaseURLString = "http://" + String(SETTINGS_PORTAINER_SERVER) + ":" + String(SETTINGS_PORTAINER_SERVER_PORT);
const char* serverBaseURL = serverBaseURLString.c_str();

const char* portainerAccessToken = SECRET_PORTAINER_ACCESS_TOKEN;

String endpointID = "";

const int16_t TFT_Width = SETTINGS_TFT_WIDTH;     
const int16_t TFT_Height = SETTINGS_TFT_HEIGHT;   

TFT_eSPI showPanel = TFT_eSPI(TFT_Width, TFT_Height);

#define LCD_BACKLIGHT PIN_POWER_ON

VirtualWindow* virtualWindow;

void holdHereForeverAndEver() {
  while (true)
    delay(60000);
}

void clearTFTDisplay() {
  virtualWindow->clear();
}

void sendOutput(String msg, bool addNewLine, bool toSerial, bool toTFTDispaly) {

  if (addNewLine)
    msg.concat("\r\n");

  if (toSerial)
    Serial.print(msg);

  if (toTFTDispaly)
    virtualWindow->print(msg);
}

void connectToWiFi() {

  sendOutput("Connecting to " + String(WiFi_SSID) + " ", false, true, true);

  WiFi.begin(WiFi_SSID, WiFi_Password);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 30000;  // 30 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    sendOutput(".", false, true, true);
  }

  if (WiFi.status() == WL_CONNECTED) {

    sendOutput("", true, true, true);
    sendOutput("Connected", true, true, true);
    sendOutput("IP address: " + WiFi.localIP().toString(), true, true, true);
    sendOutput("", true, true, true);

  } else {

    sendOutput("\nFailed to connect to WiFi. Retrying...", true, true, true);
    delay(5000);          // Wait for 5 seconds before retrying
    clearTFTDisplay();
    connectToWiFi();      // Retry connection
  }
}

String getEndpointID() {

  String returnValue = "";

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(String(serverBaseURL) + "/api/endpoints");

    http.addHeader("X-API-Key", portainerAccessToken);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {

      String payload = http.getString();
      sendOutput(payload, true, true, false);

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);

      if (doc.is<JsonArray>()) {

        JsonArray endpoints = doc.as<JsonArray>();

        if (endpoints.size() > 0) {

          returnValue = String(endpoints[0]["Id"].as<int>());
          sendOutput("Endpoint ID: " + returnValue, true, true, false);

        } else {

          sendOutput("No endpoints found.", true, true, true);
        }

      } else {

        sendOutput("Failed to parse endpoints response.", true, true, true);
      }
    } else {

      sendOutput("Error on HTTP request: ", false, true, true);
      sendOutput(String(httpResponseCode), true, true, true);
    }

    http.end();
  } else {
    sendOutput("WiFi Disconnected", true, true, true);
  }

  return returnValue;
}

void showStatusOfAllContainers() {

  enum class ContainerState {
    CREATED,
    DEAD,
    EXITED,
    PAUSED,
    REMOVING,
    RESTARTING,
    UNDEFINED,
    RUNNING
  };

  struct ContainerInfo {
    String name;
    ContainerState state;
  };

  static std::vector<ContainerInfo> priorListOfContainers;

  static bool firstPass = true;
  if (firstPass) {
    firstPass = false;
    priorListOfContainers = { ContainerInfo{ "dummy", ContainerState::UNDEFINED } };
  };

  if (WiFi.status() == WL_CONNECTED && endpointID.length() > 0) {

    HTTPClient http;
    String serverName = String(serverBaseURL) + "/api/endpoints/" + endpointID + "/docker/containers/json?all=true";
    http.begin(serverName);
    http.addHeader("X-API-Key", portainerAccessToken);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();

      sendOutput((payload), true, true, false);

      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);

      // Check if the response contains an error message
      if (doc.containsKey("message")) {

        virtualWindow->setColors(TFT_RED, TFT_BLACK);
        sendOutput("Error:", true, true, true);
        sendOutput(String(doc["message"].as<const char*>()), true, true, true);
        sendOutput("", true, true, true);
        sendOutput("Details:", true, true, true);
        sendOutput(String(+doc["details"].as<const char*>()), true, true, true);
        sendOutput("", true, true, true);
        sendOutput("Please check you have the correct value for", true, true, true);
        sendOutput("", true, true, true);
        sendOutput("SETTINGS_PORTAINER_ENDPOINT_ID", true, true, true);
        sendOutput("", true, true, true);
        sendOutput("set in the user_settings.h file.", true, true, true);

        holdHereForeverAndEver();

      } else {

        std::vector<ContainerInfo> currentListOfContainers;

        // Populate the container vector

        for (JsonObject container : doc.as<JsonArray>()) {

          String nameStr = container["Names"][0];
          String nameWithoutSlash = nameStr.substring(1);

          const char* stateStr = container["State"];
          ContainerState state;

          if (strcmp(stateStr, "created") == 0) {
            state = ContainerState::CREATED;
          } else if (strcmp(stateStr, "running") == 0) {
            state = ContainerState::RUNNING;
          } else if (strcmp(stateStr, "restarting") == 0) {
            state = ContainerState::RESTARTING;
          } else if (strcmp(stateStr, "paused") == 0) {
            state = ContainerState::PAUSED;
          } else if (strcmp(stateStr, "exited") == 0) {
            state = ContainerState::EXITED;
          } else if (strcmp(stateStr, "dead") == 0) {
            state = ContainerState::DEAD;
          } else if (strcmp(stateStr, "removing") == 0) {
            state = ContainerState::REMOVING;
          } else
            state = ContainerState::UNDEFINED;

          currentListOfContainers.push_back({ nameWithoutSlash, state });
          //containers.push_back({ "123456789112345678921234567893123456789412345678951234567896123", state });
        };

        // if the list of containers and their statuses have not changed there is no need to refresh the display so we can have an early out

        bool allConatainersAndTheirStatusesRemainUnchanged = std::equal(
          priorListOfContainers.begin(),
          priorListOfContainers.end(),
          currentListOfContainers.begin(),
          [](const ContainerInfo& a, const ContainerInfo& b) {
            return a.name == b.name && a.state == b.state;
          });

        if (allConatainersAndTheirStatusesRemainUnchanged) {
          sendOutput("All containers and thier statuses remained the same - no need to update the display", true, true, false);
          return;
        };

        priorListOfContainers = currentListOfContainers;

        // Sort the containers vector by state and within state by name

        std::sort(currentListOfContainers.begin(), currentListOfContainers.end(), [](const ContainerInfo& a, const ContainerInfo& b) {
          if (a.state == b.state) {
            return a.name < b.name;
          }
          return a.state < b.state;
        });

        clearTFTDisplay();

        virtualWindow->setColors(TFT_LIGHTGREY, TFT_BLACK);

        sendOutput("Portainer containers by state", true, true, true);
        sendOutput("---------------------------------------------------------------", true, true, true);

        // Display the container information

        ContainerState lastContainerState = ContainerState::UNDEFINED;

        for (const auto& container : currentListOfContainers) {

          const char* stateStr;

          if (container.state == lastContainerState)
            stateStr = "";
          else {
            switch (container.state) {
              case ContainerState::CREATED: stateStr = "Created:"; break;
              case ContainerState::RUNNING: stateStr = "Running:"; break;
              case ContainerState::RESTARTING: stateStr = "Restarting:"; break;
              case ContainerState::PAUSED: stateStr = "Paused:"; break;
              case ContainerState::EXITED: stateStr = "Exited:"; break;
              case ContainerState::DEAD: stateStr = "Dead"; break;
              case ContainerState::REMOVING: stateStr = "Removing"; break;
              case ContainerState::UNDEFINED: stateStr = "Undefined"; break;
            };
            lastContainerState = container.state;

            if (container.state == ContainerState::RUNNING)
              virtualWindow->setColors(TFT_GREEN, TFT_BLACK);
            else
              virtualWindow->setColors(TFT_RED, TFT_BLACK);

            sendOutput("", true, true, true);
            sendOutput(stateStr, true, true, true);
          };

          sendOutput(container.name, true, true, true);
        }
      }
    } else {

      sendOutput("Error on HTTP request: " + String(httpResponseCode), true, true, true);

      if (httpResponseCode == -1) {
        sendOutput("Connection refused. Please check the server URL and ensure the server is running.", true, true, true);
      } else if (httpResponseCode == 401) {
        sendOutput("Unauthorized. Please check your API key.", true, true, true);
      } else if (httpResponseCode == 404) {
        sendOutput("Not found. Please check the endpoint URL.", true, true, true);
      };

      holdHereForeverAndEver();
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected or Endpoint ID not available");
  }
}

void setupDisplay() {

  // touch must be initialized before display

  touchPanel.begin(DISPLAY_CYD);  // initialize the LCD in landscape mode
  touchPanel.rtInit();            // initialize the resistive touch controller


  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_POWER_ON, OUTPUT);

  showPanel.init();
  showPanel.fillScreen(TFT_BLACK);
  showPanel.setRotation(1);

  virtualWindow = new VirtualWindow(&touchPanel, &showPanel, SETTINGS_MAX_BUFFER_ROWS, SETTINGS_MAX_BUFFER_COLUMNS);

  virtualWindow->setColors(TFT_GREEN, TFT_BLACK);
}


void setup() {

  Serial.begin(115200);

  setupDisplay();

  connectToWiFi();

  if (String(SETTINGS_PORTAINER_ENDPOINT_ID).length() == 0) {

    // look up the EndpointID
    endpointID = getEndpointID();

    if (endpointID.length() > 0) {

      sendOutput("Endpoint ID found, value is: " + endpointID, true, true, true);

    } else {

      sendOutput("Failed to retrieve endpoint ID." + endpointID, true, true, true);
      sendOutput("Program will not proceed." + endpointID, true, true, true);

      holdHereForeverAndEver();
    };

  } else {

    endpointID = String(SETTINGS_PORTAINER_ENDPOINT_ID);
  };
}

void loop() {

  const unsigned long refreshRate = SETTINGS_REFRESH_RATE_IN_SECONDS * 1000;

  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();

  showStatusOfAllContainers();

  unsigned long startMillis = millis();
  while (millis() - startMillis < refreshRate) {
    virtualWindow->handleTouch();
    delay(50);
  }
}
