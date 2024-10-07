#include "uart_utils.h"
#include "http_utils.h"
#include "led.h"
#include "wifi_utils.h"

String ensureHttpsPrefix(String url) {
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    return "https://" + url;
  }
  return url;
}

// Placeholder function declarations
void placeholderCommand(String argument) {}

// Declare the commands array with placeholder functions
Command commands[] = {
    {"SET_SSID", "SET ssid <ssid>", placeholderCommand},
    {"SET_PASSWORD", "SET password <password>", placeholderCommand},
    {"ACTIVATE_WIFI", "ACTIVATE_WIFI", placeholderCommand},
    {"DISCONNECT_WIFI", "DISCONNECT_WIFI", placeholderCommand},
    {"LIST_WIFI", "LIST_WIFI", placeholderCommand},
    {"GET", "GET <url>", placeholderCommand},
    {"GET_STREAM", "GET_STREAM <url>", placeholderCommand},
    {"POST", "POST <url> <json_payload>", placeholderCommand},
    {"BUILD_HTTP_METHOD", "BUILD_HTTP_METHOD <method>", placeholderCommand},
    {"BUILD_HTTP_URL", "BUILD_HTTP_URL <url>", placeholderCommand},
    {"BUILD_HTTP_HEADER", "BUILD_HTTP_HEADER <header>", placeholderCommand},
    {"BUILD_HTTP_PAYLOAD", "BUILD_HTTP_PAYLOAD <payload>", placeholderCommand},
    {"REMOVE_HTTP_HEADER", "REMOVE_HTTP_HEADER <header>", placeholderCommand},
    {"RESET_HTTP_CONFIG", "RESET_HTTP_CONFIG", placeholderCommand},
    {"BUILD_HTTP_SHOW_RESPONSE_HEADERS",
     "BUILD_HTTP_SHOW_RESPONSE_HEADERS <true/false>", placeholderCommand},
    {"BUILD_HTTP_IMPLEMENTATION", "BUILD_HTTP_IMPLEMENTATION <STREAM/CALL>",
     placeholderCommand},
    {"EXECUTE_HTTP_CALL", "EXECUTE_HTTP_CALL", placeholderCommand},
    {"CONNECT", "CONNECT <SSID> <password>", placeholderCommand},
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

void setSSIDCommand(String argument) {
  UART0.println("setting SSID to: " + argument);
  setSSID(argument);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

void setPasswordCommand(String argument) {
  setPassword(argument);
  UART0.println("Setting SSID password to: " + argument);
  led_set_green(255);
  delay(1000);
  led_set_green(0);
}

void activateWiFiCommand(String argument) { connectToWiFi(); }

void disconnectWiFiCommand(String argument) { disconnectFromWiFi(); }

void listWiFiCommand(String argument) {
  String list = listWiFiNetworks();
  UART0.println("Available WiFi networks: " + list);
}

void getCommand(String argument) {
  argument = ensureHttpsPrefix(argument);
  UART0.println("GET request to: " + argument);
  makeHttpRequest(argument, nullptr);
}

void getStreamCommand(String argument) {
  argument = ensureHttpsPrefix(argument);
  UART0.println("GET_STREAM request to: " + argument);
  makeHttpRequestStream(argument, nullptr);
}

void postCommand(String argument) {
  int jsonStartIndex = argument.indexOf(' ') + 1;
  String url = argument.substring(0, jsonStartIndex - 1);
  String jsonPayload = argument.substring(jsonStartIndex);
  UART0.println("POST request to: " + url);
  UART0.println("Payload: " + jsonPayload);
  makeHttpPostRequest(url, jsonPayload, nullptr);
}

void buildHttpMethodCommand(String argument) { setHttpMethod(argument); }

void buildHttpUrlCommand(String argument) { setHttpUrl(argument); }

void buildHttpHeaderCommand(String argument) { addHttpHeader(argument); }

void buildHttpPayloadCommand(String argument) { setHttpPayload(argument); }

void removeHttpHeaderCommand(String argument) { removeHttpHeader(argument); }

void resetHttpConfigCommand(String argument) { resetHttpConfig(); }

void buildHttpImplementationCommand(String argument) {
  // Check if argument is a valid string of either STREAM or CALL;
  // if not, print an error message
  if (argument != "STREAM" && argument != "CALL") {
    UART0.println("Invalid HTTP implementation. Supported implementations: "
                  "STREAM, CALL");
    return;
  }
  setHttpImplementation(argument);
}

void buildHttpShowResponseHeadersCommand(String argument) {
  setShowResponseHeaders(argument.equalsIgnoreCase("true"));
}

void executeHttpCallCommand(String argument) { executeHttpCall(nullptr); }

void connectCommand(String argument) {
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

void helpCommand(String argument) {
  UART0.println("Available Commands:");
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    UART0.println(commands[i].description);
  }
}

void handleCommand(String command, String argument) {
  for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
    if (commands[i].name == command) {
      commands[i].execute(argument);
      return;
    }
  }
  UART0.println("Unknown command");
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

      handleCommand(command, argument);

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
}

// Assign the actual functions to the commands array
void initializeCommands() {
  commands[0].execute = setSSIDCommand;
  commands[1].execute = setPasswordCommand;
  commands[2].execute = activateWiFiCommand;
  commands[3].execute = disconnectWiFiCommand;
  commands[4].execute = listWiFiCommand;
  commands[5].execute = getCommand;
  commands[6].execute = getStreamCommand;
  commands[7].execute = postCommand;
  commands[8].execute = buildHttpMethodCommand;
  commands[9].execute = buildHttpUrlCommand;
  commands[10].execute = buildHttpHeaderCommand;
  commands[11].execute = buildHttpPayloadCommand;
  commands[12].execute = removeHttpHeaderCommand;
  commands[13].execute = resetHttpConfigCommand;
  commands[14].execute = buildHttpShowResponseHeadersCommand;
  commands[15].execute = buildHttpImplementationCommand;
  commands[16].execute = executeHttpCallCommand;
  commands[17].execute = connectCommand;
  commands[18].execute = helpCommand;
  commands[19].execute = helpCommand;
}

// Call initializeCommands() in your setup function or main function
void init_cmds() {
  uart_buffer_Mutex = xSemaphoreCreateMutex();
  initializeCommands();
  // Other setup code...
}
