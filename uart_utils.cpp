#include "uart_utils.h"
#include "http_utils.h"
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
// SET ssid <ssid>
// SET password <password>
// ACTIVATE
// DISCONNECT
// GET <url>
// POST <url> <json_payload>

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
      } else if (uart_buffer.equals("ACTIVATE WIFI")) {
        command = "ACTIVATE_WIFI";
      } else if (uart_buffer.equals("DISCONNECT WIFI")) {
        command = "DISCONNECT_WIFI";
      } else if (uart_buffer.startsWith("GET ")) {
        command = "GET";
        argument = uart_buffer.substring(4);
      } else if (uart_buffer.startsWith("POST ")) {
        command = "POST";
        argument = uart_buffer.substring(5);
      } else {
        command = "UNKNOWN";
      }

      switch (command.c_str()[0]) {
      case 'S':
        if (command == "SET_SSID") {
          setSSID(argument);
          UART0.println("SSID set to: " + argument);
        } else if (command == "SET_PASSWORD") {
          setPassword(argument);
          UART0.println("Password set to: " + argument);
        }
        disconnectFromWiFi();
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
        break;

      case 'P':
        if (command == "POST") {
          int jsonStartIndex = argument.indexOf(' ') + 1;
          String url = argument.substring(0, jsonStartIndex - 1);
          String jsonPayload = argument.substring(jsonStartIndex);
          UART0.println("POST request to: " + url);
          UART0.println("Payload: " + jsonPayload);
          makeHttpPostRequest(url, jsonPayload, nullptr);
        }
        break;

      default:
        UART0.println("Unknown command");
        break;
      }

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
}