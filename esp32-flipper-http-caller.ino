#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "led.h"

// Use appropriate Serial definition
#if ARDUINO_USB_CDC_ON_BOOT
#define UART0 Serial0
#else
#define UART0 Serial
#endif

const char *ssid = "MSV";
const char *password = "Starwars01";

AsyncUDP udp;

String uart_buffer = "";
const uint32_t communicationTimeout_ms = 500;
SemaphoreHandle_t uart_buffer_Mutex = NULL;

// Forward declarations
bool isJson(String str);
String parseJson(String jsonString);
void printHtml(String html, AsyncUDPPacket *packet);
String getHttpErrorMessage(int httpCode);

void makeHttpRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      String response =
          "HTTP Response code: " + String(httpResponseCode) + "\n";

      if (isJson(payload)) {
        response += parseJson(payload);
        if (packet) {
          packet->printf("%s", response.c_str());
        } else {
          UART0.println(response);
        }
      } else {
        // Handle HTML response
        response += "Response is HTML. Printing content:\n\n";
        if (packet) {
          packet->printf("%s", response.c_str());
        } else {
          UART0.println(response);
        }
        printHtml(payload, packet);
      }

    } else {
      String errorMsg = "HTTP Error: " + getHttpErrorMessage(httpResponseCode);
      if (packet) {
        packet->printf("%s", errorMsg.c_str());
      } else {
        UART0.println(errorMsg);
      }
    }

    http.end();
  } else {
    String errorMsg = "WiFi Disconnected";
    if (packet) {
      packet->printf("%s", errorMsg.c_str());
    } else {
      UART0.println(errorMsg);
    }
  }
}

String getHttpErrorMessage(int httpCode) {
  switch (httpCode) {
  case HTTPC_ERROR_CONNECTION_REFUSED:
    return "Connection refused";
  case HTTPC_ERROR_SEND_HEADER_FAILED:
    return "Send header failed";
  case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
    return "Send payload failed";
  case HTTPC_ERROR_NOT_CONNECTED:
    return "Not connected";
  case HTTPC_ERROR_CONNECTION_LOST:
    return "Connection lost";
  case HTTPC_ERROR_NO_STREAM:
    return "No stream";
  case HTTPC_ERROR_NO_HTTP_SERVER:
    return "No HTTP server";
  case HTTPC_ERROR_TOO_LESS_RAM:
    return "Not enough RAM";
  case HTTPC_ERROR_ENCODING:
    return "Transfer encoding error";
  case HTTPC_ERROR_STREAM_WRITE:
    return "Stream write error";
  case HTTPC_ERROR_READ_TIMEOUT:
    return "Connection timeout";
  default:
    return "Unknown error: " + String(httpCode);
  }
}

void printHtml(String html, AsyncUDPPacket *packet) {
  // Simple HTML parsing and printing
  // This is a basic implementation and may not handle all HTML structures
  // perfectly
  html.replace("<",
               "\n<"); // Add newline before each tag for better readability
  if (packet) {
    // For UDP, we'll send the HTML as-is
    packet->printf("%s", html.c_str());
  } else {
    // For Serial, we'll print line by line
    int start = 0;
    int end = html.indexOf('\n');
    while (end > 0) {
      UART0.println(html.substring(start, end));
      start = end + 1;
      end = html.indexOf('\n', start);
    }
    if (start < html.length()) {
      UART0.println(html.substring(start));
    }
  }
}

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

void setup() {
  UART0.begin(115200);

  led_init();
  led_set_blue(255);

  uart_buffer_Mutex = xSemaphoreCreateMutex();
  if (uart_buffer_Mutex == NULL) {
    log_e("Error creating Mutex. Sketch will fail.");
    while (true) {
      UART0.println("Mutex error (NULL). Program halted.");
      delay(2000);
    }
  }

  UART0.onReceive(UART0_RX_CB);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    UART0.println("Connecting to WiFi...");
  }

  led_set_blue(0);

  led_set_green(255);
  UART0.println("Connected to WiFi");
  UART0.print("IP Address: ");
  UART0.println(WiFi.localIP());
  led_set_green(0);

  if (udp.listen(1234)) {
    UART0.println("UDP Listening on Port: 1234");

    udp.onPacket([](AsyncUDPPacket packet) {
      String receivedData = String((char *)packet.data(), packet.length());
      UART0.println("Received UDP: " + receivedData);

      if (receivedData.startsWith("GET ")) {
        String url = receivedData.substring(4);
        makeHttpRequest(url, &packet);
      } else {
        packet.printf("Unknown UDP command: %s", receivedData.c_str());
      }
    });
  }

  UART0.println("Send 'GET url' via Serial or UDP to make an HTTP request");
}

void loop() {
  if (uart_buffer.length() > 0) {
    if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
      uart_buffer.trim();

      if (uart_buffer.startsWith("GET ")) {
        String url = uart_buffer.substring(4);
        UART0.println("Received Serial GET request for URL: " + url);
        makeHttpRequest(url, nullptr);
      } else {
        UART0.println("Received Serial: " + uart_buffer);
      }

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
  // Other loop operations can go here
}

bool isJson(String str) {
  str.trim();
  return (str.startsWith("{") && str.endsWith("}")) ||
         (str.startsWith("[") && str.endsWith("]"));
}

String parseJson(String jsonString) {
  DynamicJsonDocument doc(1024); // Adjust size based on your JSON payload

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    return "JSON parsing failed";
  }

  String parsedJson;
  serializeJsonPretty(doc, parsedJson);
  return "Parsed JSON:\n" + parsedJson;
}