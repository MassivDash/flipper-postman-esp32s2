#include "http_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <HTTPClient.h>
#include <WiFi.h>

const int MAX_CONTENT_LENGTH = 512 * 1024;

struct HttpCallConfig {
  String method;
  String implementation;
  String url;
  std::vector<std::pair<String, String>> headers;
  String payload;
  bool showResponseHeaders = false;

  void reset() {
    method = "";
    url = "";
    implementation = "CALL";
    headers.clear();
    payload = "";
    showResponseHeaders = false;
  }

  void removeHeader(String name) {
    headers.erase(std::remove_if(headers.begin(), headers.end(),
                                 [&](const std::pair<String, String> &header) {
                                   return header.first == name;
                                 }),
                  headers.end());
  }
};

HttpCallConfig httpCallConfig;

void led_error() {
  led_set_red(255);
  delay(1000);
  led_set_red(0);
}

void printResponse(String response, AsyncUDPPacket *packet) {
  if (response.isEmpty()) {
    response = "No response body";
  }
  if (packet) {
    packet->printf("%s", response.c_str());
  } else {
    UART0.println(response);
  }
}

void setShowResponseHeaders(bool show) {
  httpCallConfig.showResponseHeaders = show;
  UART0.println("Show response headers set to: " +
                String(show ? "true" : "false"));
}

void setHttpMethod(String method) {
  if (method != "GET" && method != "POST" && method != "PATCH" &&
      method != "PUT" && method != "DELETE" && method != "HEAD") {
    UART0.println("Invalid HTTP method. Supported methods: GET, POST, PATCH, "
                  "PUT, DELETE, HEAD");
    return;
  }

  httpCallConfig.method = method;
  UART0.println("HTTP_SET_METHOD: " + method);
}

void setHttpUrl(String url) {
  httpCallConfig.url = ensureHttpsPrefix(url);
  UART0.println("HTTP URL set to: " + httpCallConfig.url);
}

void addHttpHeader(String header) {
  int separatorIndex = header.indexOf(':');
  if (separatorIndex != -1) {
    String name = header.substring(0, separatorIndex);
    String value = header.substring(separatorIndex + 1);
    httpCallConfig.headers.push_back(std::make_pair(name, value));
    UART0.println("HTTP_ADD_HEADER: " + name + ": " + value);
  } else {
    UART0.println("HTTP_ERROR: Invalid header format, use HEADER name:value");
  }
}

void removeHttpHeader(String name) {
  httpCallConfig.removeHeader(name);
  UART0.println("HTTP_REMOVE_HEADER: " + name);
}

void resetHttpConfig() {
  httpCallConfig.reset();
  UART0.println("HTTP_CONFIG_REST: All configurations reset");
}

void setHttpPayload(String payload) {
  httpCallConfig.payload = payload;
  UART0.println("HTTP_SET_PAYLOAD: " + payload);
}

void setHttpImplementation(String implementation) {
  httpCallConfig.implementation = implementation;
  UART0.println("HTTP_SET_IMPLEMENTATION: " + implementation);
}

int getContentLength(String url) {
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.sendRequest("HEAD");

  if (httpResponseCode > 0) {
    int contentLength = http.getSize();
    http.end();
    return contentLength;
  } else {
    http.end();
    return -1; // Indicate an error
  }
}

void handleStreamResponse(HTTPClient &http, AsyncUDPPacket *packet) {
  WiFiClient *stream = http.getStreamPtr();
  if (stream) {
    uint8_t buff[128] = {0};
    uint16_t packetNumber = 0;
    while (stream->available()) {
      size_t size = stream->available();
      if (size) {
        int c = stream->readBytes(
            buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
        buff[c] = '\0'; // Ensure null-termination
        String chunk = String((char *)buff);
        if (!chunk.toInt()) { // Check if chunk is not a number
          String packetData = String(packetNumber) + ":" + chunk;
          if (packet) {
            packet->write((uint8_t *)packetData.c_str(), packetData.length());
          } else {
            UART0.write((uint8_t *)packetData.c_str(), packetData.length());
          }
          packetNumber++;
        }
      }
      delay(1); // Yield control to the system
    }
    // Clear the buffer
    memset(buff, 0, sizeof(buff));
    // Set stream pointer to nullptr
    stream = nullptr;
  } else {
    String payload = "empty";
    printResponse(payload, packet);
  }
}

void handleGetStringResponse(HTTPClient &http, AsyncUDPPacket *packet) {
  String payload = http.getString();
  if (payload.isEmpty()) {
    payload = "empty";
  }
  printResponse(payload, packet);
}

void makeHttpRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    int contentLength = getContentLength(url);

    if (contentLength > MAX_CONTENT_LENGTH) {
      String warnMsg = "WARNING: Content of " + contentLength +
                       " exceeds maximum length of " + MAX_CONTENT_LENGTH +
                       " bytes for simple calls. Using stream,  if"
                       "the blue light stays on, reset the board.";
      printResponse(warnMsg, packet);
    }

    if (contentLength == -1) {
      String warnMsg =
          "WARNING: Content-Length is unknown (-1). These calls tend to crash "
          "to board. If the blue light stays on, reset the board.";
      printResponse(warnMsg, packet);
    }

    HTTPClient http;
    led_set_blue(255);
    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = "STATUS: " + String(httpResponseCode) + "\n";
      printResponse(response, packet);

      if (contentLength == -1 || contentLength > MAX_CONTENT_LENGTH) {
        UART0.println("STREAM:") handleStreamResponse(http, packet);
      } else {
        UART0.println("RESPONSE:") handleGetStringResponse(http, packet);
      }
    } else {
      String errorMsg = "HTTP_ERROR: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

void makeHttpRequestStream(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.begin(url);

    int contentLength = getContentLength(url);

    if (contentLength > MAX_CONTENT_LENGTH) {
      String warnMsg = "WARNING: Content of " + contentLength +
                       " exceeds maximum length for stream of." +
                       MAX_CONTENT_LENGTH +
                       " bytes. If
                       the blue light stays on,
             reset the board.";
             printResponse(warnMsg, packet);
    }

    if (contentLength == -1) {
      String warnMsg =
          "WARNING: Content-Length is unknown (-1). These calls tend to crash "
          "to board. If the blue light stays on, reset the board.";
      printResponse(warnMsg, packet);
    }

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = "STATUS: " + String(httpResponseCode) + "\n";
      printResponse(response, packet);
      handleStreamResponse(http, packet);
    } else {
      String errorMsg =
          "HTTP_ERROR: " + HTTPClient::errorToString(httpResponseCode) +
          " (Code: " + String(httpResponseCode) + ")";
      printResponse(errorMsg, packet);
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = "STATUS: " + String(httpResponseCode) + "\n";
      printResponse(response, packet);
      handleGetStringResponse(http, packet);
    } else {
      String errorMsg = "HTTP_ERROR: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
      led_error();
    }

    http.end();
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
    led_error();
  }
}

void executeHttpCall(AsyncUDPPacket *packet) {
  if (httpCallConfig.url.isEmpty() || httpCallConfig.method.isEmpty()) {
    String errorMsg = "HTTP URL or Method not set";
    printResponse(errorMsg, packet);
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    led_set_blue(255);
    HTTPClient http;
    http.begin(httpCallConfig.url);

    for (const auto &header : httpCallConfig.headers) {
      http.addHeader(header.first, header.second);
    }

    // Collect common headers
    const char *headerKeys[] = {"Content-Type", "Content-Length", "Connection",
                                "Date", "Server"};
    size_t headerKeysCount = sizeof(headerKeys) / sizeof(headerKeys[0]);
    http.collectHeaders(headerKeys, headerKeysCount);

    String response;
    int httpResponseCode;

    if (httpCallConfig.method == "GET") {
      httpResponseCode = http.GET();
    } else if (httpCallConfig.method == "POST") {
      httpResponseCode = http.POST(httpCallConfig.payload);
    } else if (httpCallConfig.method == "PATCH") {
      httpResponseCode = http.PATCH(httpCallConfig.payload);
    } else if (httpCallConfig.method == "PUT") {
      httpResponseCode = http.PUT(httpCallConfig.payload);
    } else if (httpCallConfig.method == "DELETE") {
      httpResponseCode = http.sendRequest("DELETE", httpCallConfig.payload);
    } else if (httpCallConfig.method == "HEAD") {
      httpResponseCode = http.sendRequest("HEAD");
    } else {
      String errorMsg = "Unsupported HTTP method: " + httpCallConfig.method;
      printResponse(errorMsg, packet);
      http.end();
      led_set_blue(0);
      return;
    }

    if (httpResponseCode > 0) {
      response = "STATUS: " + String(httpResponseCode) + "\n";

      if (httpCallConfig.showResponseHeaders) {
        UART0.println("HEADERS:");
        // Get the header count
        int headerCount = http.headers();

        // Iterate over the collected headers and print them
        for (int i = 0; i < headerCount; i++) {
          String headerName = http.headerName(i);
          String headerValue = http.header(i);
          UART0.println(headerName + ": " + headerValue);
        }
      }

      printResponse(response, packet);
      if (httpCallConfig.implementation == "STREAM") {
        handleStreamResponse(http, packet);
      } else {
        handleGetStringResponse(http, packet);
      }
    } else {
      String errorMsg = "HTTP_ERROR: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
      led_error();
    }

    http.end();
    response = "";
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
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