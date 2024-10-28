/**
 * @file uart_commands.cpp
 * @brief UART command handling and WiFi utilities
 *
 * This file contains functions for handling UART commands, including WiFi operations,
 * HTTP requests, and UDP messaging. It also defines the command structure and initializes
 * the available commands for the system.
 */

#include "uart_utils.h"
#include "http_utils.h"
#include "led.h"
#include "version.h"
#include "wifi_utils.h"
#include <AsyncUDP.h>


/**
 * @brief Buffer to store incoming UART data
 */
String uart_buffer = "";

/**
 * @brief Mutex to protect access to the UART buffer
 */
SemaphoreHandle_t uart_buffer_Mutex = NULL;

/**
 * @brief Timeout for UART communication in milliseconds
 */
const uint32_t communicationTimeout_ms = 500;



/**
 * @brief Ensure the URL has an HTTPS prefix
 *
 * @param url The URL to check and modify if necessary
 * @return String The URL with HTTPS prefix
 */
String ensureHttpsPrefix(String url) {
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    return "https://" + url;
  }
  return url;
}

/**
 * @brief Placeholder function for command execution
 * @param argument The argument passed to the command
 * @param packet Pointer to the UDP packet (can be null)
 */
void placeholderCommand(String argument, AsyncUDPPacket *packet) {}


/**
 * @brief Array of available commands
 */
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
    {"WIFI_GET_LOCAL_IP", "WIFI_GET_LOCAL_IP", placeholderCommand},
    {"GET", "GET <url>", placeholderCommand},
    {"GET_STREAM", "GET_STREAM <url>", placeholderCommand},
    {"FILE_STREAM", "FILE_STREAM <url>", placeholderCommand},
    {"POST", "POST <url> <json_payload>", placeholderCommand},
    {"POST_STREAM", "POST_STREAM <url> <json>", placeholderCommand},
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
    {"MESSAGE_UDP", "MESSAGE_UDP <message> <remoteIP> <remotePort>", placeholderCommand},
    {"?", "type ? to print help", placeholderCommand},
    {"HELP", "HELP", placeholderCommand}};

/**
 * @brief UART0 receive callback function
 *
 * This function is called when data is available on UART0.
 * It reads the data into the uart_buffer with a timeout.
 */
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

/**
 * @brief Set the WiFi SSID
 *
 * @param argument The SSID to set
 * @param packet Pointer to the UDP packet (can be null)
 */
void setSSIDCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("WIFI_SSID: " + argument, packet);
  setSSID(argument);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

/**
 * @brief Handle the incoming serial input
 *
 * This function processes the data in uart_buffer,
 * extracts the command and argument, and calls the appropriate handler.
 */
void setPasswordCommand(String argument, AsyncUDPPacket *packet) {
  setPassword(argument);
  printResponse("WIFI_PASSWORD: " + argument, packet);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

/**
 * @brief Activate WiFi connection
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket, unused in this function
 */
void activateWiFiCommand(String argument, AsyncUDPPacket *packet) {
  connectToWiFi();
}

/**
 * @brief Check and print WiFi connection status
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void checkWiFiStatusCommand(String argument, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    printResponse("WIFI_STATUS: CONNECTED", packet);
  } else {
    printResponse("WIFI_STATUS: DISCONNECTED", packet);
  }
}

/**
 * @brief Disconnect from WiFi
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket, unused in this function
 */
void disconnectWiFiCommand(String argument, AsyncUDPPacket *packet) {
  disconnectFromWiFi();
}

/**
 * @brief List available WiFi networks
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void listWiFiCommand(String argument, AsyncUDPPacket *packet) {
  String list = listWiFiNetworks();
  printResponse(list, packet);
}

/**
 * @brief Get and print local WiFi IP address
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getWifiLocalIp(String argument, AsyncUDPPacket *packet) {
  String ipString = getLocalIpString();
  printResponse(ipString, packet);
}

/**
 * @brief Perform HTTP GET request
 * @param argument URL for the GET request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  printResponse("GET request to: " + argument, packet);
  makeHttpRequest(argument, packet);
}

/**
 * @brief Perform HTTP GET request for file streaming
 * @param argument URL for the GET request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getFileStreamCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  (argument, packet);
  makeHttpFileRequest(argument, packet);
}

/**
 * @brief Perform streaming HTTP GET request
 * @param argument URL for the GET request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getStreamCommand(String argument, AsyncUDPPacket *packet) {
  argument = ensureHttpsPrefix(argument);
  printResponse("GET_STREAM: " + argument, packet);
  makeHttpRequestStream(argument, packet);
}

/**
 * @brief Perform HTTP POST request
 * @param argument String containing URL and JSON payload
 * @param packet Pointer to AsyncUDPPacket for response
 */
void postCommand(String argument, AsyncUDPPacket *packet) {
  int jsonStartIndex = argument.indexOf(' ') + 1;
  String url = argument.substring(0, jsonStartIndex - 1);
  String jsonPayload = argument.substring(jsonStartIndex);
  printResponse("POST: " + url, packet);
  printResponse("Payload: " + jsonPayload, packet);
  makeHttpPostRequest(url, jsonPayload, packet);
}


/**
 * @brief Perform streaming HTTP POST request
 * @param argument String containing URL and JSON payload
 * @param packet Pointer to AsyncUDPPacket for response
 */
void postStreamCommand(String argument, AsyncUDPPacket *packet) {
  int jsonStartIndex = argument.indexOf(' ') + 1;
  String url = argument.substring(0, jsonStartIndex - 1);
  String jsonPayload = argument.substring(jsonStartIndex);
  makeHttpPostFileRequest(url, jsonPayload, packet);
}


/**
 * @brief Set HTTP method for request builder
 * @param argument HTTP method to set
 * @param packet Pointer to AsyncUDPPacket for response
 */
void buildHttpMethodCommand(String argument, AsyncUDPPacket *packet) {
  setHttpMethod(argument, packet);
}

/**
 * @brief Set URL for request builder
 * @param argument URL to set
 * @param packet Pointer to AsyncUDPPacket for response
 */
void buildHttpUrlCommand(String argument, AsyncUDPPacket *packet) {
  setHttpUrl(argument, packet);
}

/**
 * @brief Add HTTP header for request builder
 * @param argument Header to add (format: "key:value")
 * @param packet Pointer to AsyncUDPPacket for response
 */
void buildHttpHeaderCommand(String argument, AsyncUDPPacket *packet) {
  addHttpHeader(argument, packet);
}

/**
 * @brief Set HTTP payload for request builder
 * @param argument Payload to set
 * @param packet Pointer to AsyncUDPPacket for response
 */
void buildHttpPayloadCommand(String argument, AsyncUDPPacket *packet) {
  setHttpPayload(argument, packet);
}

/**
 * @brief Remove HTTP header from request builder
 * @param argument Header key to remove
 * @param packet Pointer to AsyncUDPPacket for response
 */
void removeHttpHeaderCommand(String argument, AsyncUDPPacket *packet) {
  removeHttpHeader(argument, packet);
}

/**
 * @brief Reset HTTP configuration in request builder
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void resetHttpConfigCommand(String argument, AsyncUDPPacket *packet) {
  resetHttpConfig(packet);
}

/**
 * @brief Get current HTTP builder configuration
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getHttpBuilderConfigCommand(String argument, AsyncUDPPacket *packet) {
  getHttpBuilderConfig(packet);
}

/**
 * @brief Set HTTP implementation (STREAM or CALL)
 * @param argument Implementation type ("STREAM" or "CALL")
 * @param packet Pointer to AsyncUDPPacket for response
 */
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


/**
 * @brief Set whether to show response headers
 * @param argument "true" or "false"
 * @param packet Pointer to AsyncUDPPacket for response
 */
void buildHttpShowResponseHeadersCommand(String argument,
                                         AsyncUDPPacket *packet) {
  setShowResponseHeaders(argument.equalsIgnoreCase("true"), packet);
}

/**
 * @brief Execute the built HTTP call
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void executeHttpCallCommand(String argument, AsyncUDPPacket *packet) {
  executeHttpCall(packet);
}

/**
 * @brief Get active WiFi network SSID
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void wifiNetworkCommand(String argument, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    printResponse("WIFI_GET_ACTIVE_SSID: " + WiFi.SSID(), packet);
  } else {
    printResponse("WIFI_GET_ACTIVE_SSID: Not connected", packet);
  }
}


/**
 * @brief Connect to WiFi network
 * @param argument String containing SSID and password
 * @param packet Pointer to AsyncUDPPacket for response
 */
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


/**
 * @brief Get board version
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getBoardVersionCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("VERSION: " + String(version), packet);
}


/**
 * @brief Display help information for available commands
 * @param argument Unused parameter
 * @param packet Pointer to AsyncUDPPacket for response
 */
void helpCommand(String argument, AsyncUDPPacket *packet) {
  printResponse("Available Commands:", packet);
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    printResponse(commands[i].description, packet);
  }
}

/**
 * @brief Handle incoming command
 * @param command Command string
 * @param argument Command argument string
 * @param packet Pointer to AsyncUDPPacket for response
 */
void handleCommand(String command, String argument, AsyncUDPPacket *packet) {
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (commands[i].name == command) {
      commands[i].execute(argument, packet);
      return;
    }
  }
  printResponse("Unknown command", packet);
}

/**
 * @brief Handle the incoming serial input
 *
 * This function processes the data in uart_buffer,
 * extracts the command and argument, and calls the appropriate handler.
 */
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


/**
 * @brief Handle UDP message command
 *
 * This function parses the argument string to extract the message, remote IP address,
 * and remote port. It then sends a UDP message to the specified address and port.
 *
 * @param argument String containing the message, remote IP, and remote port
 *                 Format: "<message> <remote_ip> <remote_port>"
 * @param packet Pointer to AsyncUDPPacket, unused in this function
 *
 * @note The function expects the argument to be in the format:
 *       "<message> <remote_ip> <remote_port>"
 *       where <message> can contain spaces, <remote_ip> is a valid IP address,
 *       and <remote_port> is a valid port number.
 *
 * @warning If the IP address format is invalid, an error message is printed to UART0,
 *          and the function returns without sending the message.
 *
 * @see sendUDPMessage()
 */
void handleMessageUDPCommand(String argument, AsyncUDPPacket *packet) {
  // Find the position of the last two spaces
  int lastSpaceIndex = argument.lastIndexOf(' ');
  int secondLastSpaceIndex = argument.lastIndexOf(' ', lastSpaceIndex - 1);

  // Extract remotePort
  uint16_t remotePort = argument.substring(lastSpaceIndex + 1).toInt();

  // Extract remoteIP
  String remoteIPString = argument.substring(secondLastSpaceIndex + 1, lastSpaceIndex);
  IPAddress remoteIP;
  if (!remoteIP.fromString(remoteIPString)) {
    UART0.println("ERROR: Invalid IP address format");
    return;
  }

  // Extract message
  String message = argument.substring(0, secondLastSpaceIndex);

  // Send the UDP message
  sendUDPMessage(message.c_str(), remoteIP, remotePort);

  UART0.println("UDP message sent: " + message);
  UART0.println("To IP: " + remoteIPString + ", Port: " + String(remotePort));
}

/**
 * @brief Initialize the commands array with actual functions
 */
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
  commands[9].execute = getWifiLocalIp;
  commands[10].execute = getCommand;
  commands[11].execute = getStreamCommand;
  commands[12].execute = getFileStreamCommand;
  commands[13].execute = postCommand;
  commands[14].execute = postStreamCommand;
  commands[15].execute = buildHttpMethodCommand;
  commands[16].execute = buildHttpUrlCommand;
  commands[17].execute = buildHttpHeaderCommand;
  commands[18].execute = buildHttpPayloadCommand;
  commands[19].execute = removeHttpHeaderCommand;
  commands[20].execute = resetHttpConfigCommand;
  commands[21].execute = buildHttpShowResponseHeadersCommand;
  commands[22].execute = buildHttpImplementationCommand;
  commands[23].execute = executeHttpCallCommand;
  commands[24].execute = getHttpBuilderConfigCommand;
  commands[25].execute = handleMessageUDPCommand;
  commands[26].execute = helpCommand;
  commands[27].execute = helpCommand;
}

/**
 * @brief Initialize UART commands and related resources
 */
void init_cmds() {
  uart_buffer_Mutex = xSemaphoreCreateMutex();
  initializeCommands();
  // Other setup code...
}