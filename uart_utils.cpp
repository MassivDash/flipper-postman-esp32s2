#include "uart_utils.h"
#include "http_utils.h"
#include "led.h"
#include "wifi_utils.h"

String uart_buffer = "";
SemaphoreHandle_t uart_buffer_Mutex = NULL;
const uint32_t communicationTimeout_ms = 500;

void UART0_RX_CB() {
  if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
    uint32_t now = millis();
    while ((millis() - now) < communicationTimeout_ms) {
      if (UART0.available()) {
        uart_buffer += (char)UART0.read();
        now = millis();
      }
    }
    xSemaphoreGive(uart_buffer_Mutex);
  }
}

// List of Commands
// LIST WIFI
// SET ssid <ssid>
// SET password <password>
// ACTIVATE WIFI
// DISCONNECT WIFI
// GET <url>
// GET_STREAM <url>
// POST <url> <json_payload>

void handleCommand(String command, String argument) {
  switch (command.c_str()[0]) {
  case 'S':
    if (command == "SET_SSID") {
      UART0.println("setting SSID to: " + argument);
      setSSID(argument);
      led_set_green(255);
      delay(1000);
      led_set_green(0);
    } else if (command == "SET_PASSWORD") {
      setPassword(argument);
      UART0.println("Setting SSID password to: " + argument);
      led_set_green(255);
      delay(1000);
      led_set_green(0);
    }
    break;

  case 'A':
    if (command == "ACTIVATE_WIFI") {
      connectToWiFi();
    }
    break;

  case 'D':
    if (command == "DISCONNECT_WIFI") {
      disconnectFromWiFi();
    }
    break;

  case 'G':
    if (command == "GET") {
      UART0.println("GET request to: " + argument);
      makeHttpRequest(argument, nullptr);
    }

    if (command == "GET_STREAM") {
      UART0.println("GET_STREAM request to: " + argument);
      makeHttpRequestStream(argument, nullptr);
    }
    break;

  case 'L':
    if (command == "LIST_WIFI") {
      String list = listWiFiNetworks();
      UART0.println("Available WiFi networks: " + list);
    }

  case 'P':
    if (command == "POST") {
      int jsonStartIndex = argument.indexOf(' ') + 1;
      String url = argument.substring(0, jsonStartIndex - 1);
      String jsonPayload = argument.substring(jsonStartIndex);
      UART0.println("POST request to: " + url);
      UART0.println("Payload: " + jsonPayload);
      Serial.println("POST request to: " + url);
      Serial.println("Payload: " + jsonPayload);
      makeHttpPostRequest(url, jsonPayload, nullptr);
    }
    break;

  case 'C':
    if (command == "CONNECT") {
      int spaceIndex = argument.indexOf(' ');
      if (spaceIndex != -1) {
        String ssid = argument.substring(0, spaceIndex);
        String password = argument.substring(spaceIndex + 1);
        UART0.println("Setting SSID to: " + ssid);
        setSSID(ssid);
        UART0.println("Setting password to: " + password);
        setPassword(password);
        UART0.println("Connecting to WiFi...");
        connectToWiFi();
      } else {
        UART0.println(
            "Invalid CONNECT command format. Use: CONNECT {SSID} {password}");
      }
    }
    break;

  default:
    UART0.println("Unknown command");
    break;
  }
}

void handleSerialInput() {
  if (uart_buffer.length() > 0) {
    if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
      uart_buffer.trim();

      String command;
      String argument;

      if (uart_buffer.startsWith("SET ssid ")) {
        command = "SET_SSID";
        argument = uart_buffer.substring(9);
      } else if (uart_buffer.startsWith("SET password ")) {
        command = "SET_PASSWORD";
        argument = uart_buffer.substring(13);
      } else if (uart_buffer.startsWith("CONNECT ")) {
        command = "CONNECT";
        argument = uart_buffer.substring(8);
      } else if (uart_buffer.equals("ACTIVATE WIFI")) {
        command = "ACTIVATE_WIFI";
      } else if (uart_buffer.equals("DISCONNECT WIFI")) {
        command = "DISCONNECT_WIFI";
      } else if (uart_buffer.equals("LIST WIFI")) {
        command = "LIST_WIFI";
      } else if (uart_buffer.startsWith("GET ")) {
        command = "GET";
        argument = uart_buffer.substring(4);
      } else if (uart_buffer.startsWith("GET_STREAM ")) {
        command = "GET_STREAM";
        argument = uart_buffer.substring(11);
      } else if (uart_buffer.startsWith("POST ")) {
        command = "POST";
        argument = uart_buffer.substring(5);
      } else {
        command = "UNKNOWN";
      }

      handleCommand(command, argument);

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
}