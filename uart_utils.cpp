#include "uart_utils.h"
#include "http_utils.h"
#include "led.h"
#include "version.h"
#include "wifi_utils.h"
#include <AsyncUDP.h>

String ensureHttpsPrefix(String url) {
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    return "https://" + url;
  }
  return url;
}

// Placeholder function declarations
void placeholderCommand(String argument, AsyncUDPPacket *packet) {}

// Declare the commands array with placeholder functions
Command commands[] = {
    {"VERSION", "VERSION: Get board version", placeholderCommand},
    {"WIFI_CONNECT", "WIFI_CONNECT <SSID> <password>", placeholderCommand},
    {"WIFI_SET_SSID", "WIFI_SET_SSID ssid <ssid>", placeholderCommand},
    {"WIFI_SET_PASSWORD", "WIFI_SET_PASSWORD password <password>", placeholderCommand},
    {"WIFI_ACTIVATE", "WIFI_ACTIVATE", placeholderCommand},
    {"WIFI_DEACTIVATE", "WIFI_DEACTIVATE", placeholderCommand},
    {"WIFI_LIST", "WIFI_LIST", placeholderCommand},
    {"WIFI_STATUS", "WIFI_STATUS: Show wifi status CONNECTED / DISCONNECTED",
     placeholderCommand},
    {"WIFI_GET_ACTIVE_SSID", "WIFI_GET_ACTIVE_SSID: <ssid>",
     placeholderCommand},
    {"GET", "GET <url>", placeholderCommand},
    {"GET_STREAM", "GET_STREAM <url>", placeholderCommand},
    {"POST", "POST <url> <json_payload>", placeholderCommand},
    {"BUILD_HTTP_METHOD", "BUILD_HTTP_METHOD <method>", placeholderCommand},
    {"BUILD_HTTP_URL", "BUILD_HTTP_URL <url>", placeholderCommand},
    {"BUILD_HTTP_HEADER", "BUILD_HTTP_HEADER <key:value>", placeholderCommand},
    {"BUILD_HTTP_PAYLOAD", "BUILD_HTTP_PAYLOAD <payload>", placeholderCommand},
    {"REMOVE_HTTP_HEADER", "REMOVE_HTTP_HEADER <key>", placeholderCommand},
    {"RESET_HTTP_CONFIG", "RESET_HTTP_CONFIG", placeholderCommand},
    {"BUILD_HTTP_SHOW_RESPONSE_HEADERS",
     "BUILD_HTTP_SHOW_RESPONSE_HEADERS <true/false>", placeholderCommand},
    {"BUILD_HTTP_IMPLEMENTATION", "BUILD_HTTP_IMPLEMENTATION <STREAM/CALL>",
     placeholderCommand},
    {"EXECUTE_HTTP_CALL", "EXECUTE_HTTP_CALL", placeholderCommand},
    {"BUILD_HTTP_SHOW_CONFIG",
     "BUILD_HTTP_SHOW_CONFIG: Show current HTTP configuration",
     placeholderCommand},
    {"FILE_STREAM", "FILE_STREAM <url>", placeholderCommand},
    {"?", "type ? to print help", placeholderCommand},
    {"HELP", "HELP", placeholderCommand}};

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

void setSSIDCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("WIFI_SSID: " + argument, packet);
  setSSID(argument);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

void setPasswordCommand(String argument, AsyncUDPPacket *packet) {
  setPassword(argument);
  printResponse("WIFI_PASSWORD: " + argument, packet);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

void activateWiFiCommand(String argument, AsyncUDPPacket *packet) {
  connectToWiFi();
}

void checkWiFiStatusCommand(String argument, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    printResponse("WIFI_STATUS: CONNECTED", packet);
  } else {
    printResponse("WIFI_STATUS: DISCONNECTED", packet);
  }
}

void disconnectWiFiCommand(String argument, AsyncUDPPacket *packet) {
  disconnectFromWiFi();
}

void listWiFiCommand(String argument, AsyncUDPPacket *packet) {
  String list = listWiFiNetworks();
  printResponse(list, packet);
}

void getCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  printResponse("GET request to: " + argument, packet);
  makeHttpRequest(argument, packet);
}

void getFileStreamCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  (argument, packet);
  makeHttpFileRequest(argument, packet);
}

void getStreamCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  printResponse("GET_STREAM: " + argument, packet);
  makeHttpRequestStream(argument, packet);
}

void postCommand(String argument, AsyncUDPPacket *packet) {
  int jsonStartIndex = argument.indexOf(' ') + 1;
  String url = argument.substring(0, jsonStartIndex - 1);
  String jsonPayload = argument.substring(jsonStartIndex);
  printResponse("POST: " + url, packet);
  printResponse("Payload: " + jsonPayload, packet);
  makeHttpPostRequest(url, jsonPayload, packet);
}

void buildHttpMethodCommand(String argument, AsyncUDPPacket *packet) {
  setHttpMethod(argument, packet);
}

void buildHttpUrlCommand(String argument, AsyncUDPPacket *packet) {
  setHttpUrl(argument, packet);
}

void buildHttpHeaderCommand(String argument, AsyncUDPPacket *packet) {
  addHttpHeader(argument, packet);
}

void buildHttpPayloadCommand(String argument, AsyncUDPPacket *packet) {
  setHttpPayload(argument, packet);
}

void removeHttpHeaderCommand(String argument, AsyncUDPPacket *packet) {
  removeHttpHeader(argument, packet);
}

void resetHttpConfigCommand(String argument, AsyncUDPPacket *packet) {
  resetHttpConfig(packet);
}

void getHttpBuilderConfigCommand(String argument, AsyncUDPPacket *packet) {
  getHttpBuilderConfig(packet);
}

void buildHttpImplementationCommand(String argument, AsyncUDPPacket *packet) {
  // Check if argument is a valid string of either STREAM or CALL;
  // if not, print an error message
  if (argument != "STREAM" && argument != "CALL") {
    printResponse(
        "HTTP_ERROR: Invalid HTTP implementation. Supported implementations: "
        "STREAM, CALL",
        packet);
    return;
  }
  setHttpImplementation(argument, packet);
}

void buildHttpShowResponseHeadersCommand(String argument,
                                         AsyncUDPPacket *packet) {
  setShowResponseHeaders(argument.equalsIgnoreCase("true"), packet);
}

void executeHttpCallCommand(String argument, AsyncUDPPacket *packet) {
  executeHttpCall(packet);
}

void wifiNetworkCommand(String argument, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    printResponse("WIFI_GET_ACTIVE_SSID: " + WiFi.SSID(), packet);
  } else {
    printResponse("WIFI_GET_ACTIVE_SSID: Not connected", packet);
  }
}

void connectCommand(String argument, AsyncUDPPacket *packet) {
  int spaceIndex = argument.indexOf(' ');
  if (spaceIndex != -1) {
    String ssid = argument.substring(0, spaceIndex);
    String password = argument.substring(spaceIndex + 1);
    setSSID(ssid);
    setPassword(password);
    connectToWiFi();
  } else {
    printResponse("WIFI_ERROR: Invalid CONNECT command format. Use: CONNECT "
                  "<SSID> <password>",
                  packet);
  }
}

void getBoardVersionCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("VERSION: " + String(version), packet);
}

void helpCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("Available Commands:", packet);
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    printResponse(commands[i].description, packet);
  }
}

void handleCommand(String command, String argument, AsyncUDPPacket *packet) {
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (commands[i].name == command) {
      commands[i].execute(argument, packet);
      return;
    }
  }
  printResponse("Unknown command", packet);
}

void handleSerialInput() {
  if (uart_buffer.length() > 0) {
    if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
      uart_buffer.trim();

      String command;
      String argument;

      int spaceIndex = uart_buffer.indexOf(' ');
      if (spaceIndex != -1) {
        command = uart_buffer.substring(0, spaceIndex);
        argument = uart_buffer.substring(spaceIndex + 1);
      } else {
        command = uart_buffer;
      }

      handleCommand(command, argument, nullptr);

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
}

// Assign the actual functions to the commands array
void initializeCommands() {
  commands[0].execute = getBoardVersionCommand;
  commands[1].execute = connectCommand;
  commands[2].execute = setSSIDCommand;
  commands[3].execute = setPasswordCommand;
  commands[4].execute = activateWiFiCommand;
  commands[5].execute = disconnectWiFiCommand;
  commands[6].execute = listWiFiCommand;
  commands[7].execute = checkWiFiStatusCommand;
  commands[8].execute = wifiNetworkCommand;
  commands[9].execute = getCommand;
  commands[10].execute = getStreamCommand;
  commands[11].execute = postCommand;
  commands[12].execute = buildHttpMethodCommand;
  commands[13].execute = buildHttpUrlCommand;
  commands[14].execute = buildHttpHeaderCommand;
  commands[15].execute = buildHttpPayloadCommand;
  commands[16].execute = removeHttpHeaderCommand;
  commands[17].execute = resetHttpConfigCommand;
  commands[18].execute = buildHttpShowResponseHeadersCommand;
  commands[19].execute = buildHttpImplementationCommand;
  commands[20].execute = executeHttpCallCommand;
  commands[21].execute = getHttpBuilderConfigCommand;
  commands[22].execute = getFileStreamCommand;
  commands[23].execute = helpCommand;
  commands[24].execute = helpCommand;
}

// Call initializeCommands() in your setup function or main function
void init_cmds() {
  uart_buffer_Mutex = xSemaphoreCreateMutex();
  initializeCommands();
  // Other setup code...
}