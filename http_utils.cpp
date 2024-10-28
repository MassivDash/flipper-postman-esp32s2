/**
 * @file http_utils.cpp
 * @brief HTTP utility functions and configurations
 *
 * This file contains utility functions for handling HTTP requests,
 * managing HTTP configurations, and processing HTTP responses.
 */

#include "http_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <HTTPClient.h>
#include <WiFi.h>

/// Maximum content length for HTTP responses
const int MAX_CONTENT_LENGTH = 512 * 1024;

/**
 * @struct HttpCallConfig
 * @brief Configuration for HTTP calls
 */
struct HttpCallConfig {
    String method;                                  ///< HTTP method
    String implementation;                          ///< Implementation type (e.g., "CALL" or "STREAM")
    String url;                                     ///< URL for the HTTP request
    std::vector<std::pair<String, String>> headers; ///< HTTP headers
    String payload;                                 ///< Request payload
    bool showResponseHeaders;                       ///< Flag to show response headers

  void reset() {                                    ///< Reset the configuration
    method = "";
    url = "";
    implementation = "CALL";
    headers.clear();
    payload = "";
    showResponseHeaders = false;
  }

  void removeHeader(String name) {                 ///< Remove a specific header
    headers.erase(std::remove_if(headers.begin(), headers.end(),
                                 [&](const std::pair<String, String> &header) {
                                   return header.first == name;
                                 }),
                  headers.end());
  }
};

/// Global HTTP call configuration
HttpCallConfig httpCallConfig;

/**
 * @brief Print response to UART or UDP packet
 * @param response The response string to print
 * @param packet Pointer to AsyncUDPPacket, can be null for UART output
 */
void printResponse(String response, AsyncUDPPacket *packet) {
  if (response.isEmpty()) {
    response = "empty";
  }
  if (packet) {
    packet->printf("%s", response.c_str());
  } else {
    UART0.println(response);
  }
}

/**
 * @brief Set flag to show response headers
 * @param show Boolean flag to show or hide headers
 * @param packet Pointer to AsyncUDPPacket for response
 */
void setShowResponseHeaders(bool show, AsyncUDPPacket *packet) {
  httpCallConfig.showResponseHeaders = show;
  printResponse("HTTP_BUILDER_SHOW_RESPONSE_HEADERS: " +
                    String(show ? "true" : "false"),
                packet);
}

/**
 * @brief Get and print current HTTP builder configuration
 * @param packet Pointer to AsyncUDPPacket for response
 */
void getHttpBuilderConfig(AsyncUDPPacket *packet) {
  printResponse("HTTP_BUILDER_CONFIG: ", packet);
  printResponse("HTTP_METHOD: " + httpCallConfig.method, packet);
  printResponse("HTTP_URL: " + httpCallConfig.url, packet);
  printResponse("HTTP_PAYLOAD: " + httpCallConfig.payload, packet);
  printResponse("HTTP_IMPLEMENTATION: " + httpCallConfig.implementation,
                packet);
  printResponse("HTTP_HEADERS: ", packet);
  for (const auto &header : httpCallConfig.headers) {
    printResponse(header.first + ": " + header.second, packet);
  }
}

/**
 * @brief Set HTTP method for the request
 * @param method HTTP method string
 * @param packet Pointer to AsyncUDPPacket for response
 */
void setHttpMethod(String method, AsyncUDPPacket *packet) {
  if (method != "GET" && method != "POST" && method != "PATCH" &&
      method != "PUT" && method != "DELETE" && method != "HEAD") {
    printResponse("HTTP_ERROR: Invalid HTTP method. Supported methods: GET, "
                  "POST, PATCH, PUT, DELETE, HEAD",
                  packet);
    return;
  }

  httpCallConfig.method = method;
  printResponse("HTTP_SET_METHOD: " + method, packet);
}

/**
 * @brief Set URL for the HTTP request
 * @param url URL string
 * @param packet Pointer to AsyncUDPPacket for response
 */

void setHttpUrl(String url, AsyncUDPPacket *packet) {
  httpCallConfig.url = ensureHttpsPrefix(url);
  printResponse("HTTP_URL: " + httpCallConfig.url, packet);
}

/**
 * @brief Add an HTTP header to the configuration
 * @param header Header string in "name:value" format
 * @param packet Pointer to AsyncUDPPacket for response
 */
void addHttpHeader(String header, AsyncUDPPacket *packet) {
  int separatorIndex = header.indexOf(':');
  if (separatorIndex != -1) {
    String name = header.substring(0, separatorIndex);
    String value = header.substring(separatorIndex + 1);
    httpCallConfig.headers.push_back(std::make_pair(name, value));
    printResponse("HTTP_ADD_HEADER: " + name + ": " + value, packet);
  } else {
    printResponse("HTTP_ERROR: Invalid header format, use HEADER name:value",
                  packet);
  }
}

/**
 * @brief Remove an HTTP header from the configuration
 * @param name Name of the header to remove
 * @param packet Pointer to AsyncUDPPacket for response
 */
void removeHttpHeader(String name, AsyncUDPPacket *packet) {
  httpCallConfig.removeHeader(name);
  printResponse("HTTP_REMOVE_HEADER: " + name, packet);
}

/**
 * @brief Reset the HTTP configuration to default values
 * @param packet Pointer to AsyncUDPPacket for response
 */
void resetHttpConfig(AsyncUDPPacket *packet) {
  httpCallConfig.reset();
  printResponse("HTTP_CONFIG_REST: All configurations reset", packet);
}

/**
 * @brief Set the payload for the HTTP request
 * @param payload Payload string
 * @param packet Pointer to AsyncUDPPacket for response
 */
void setHttpPayload(String payload, AsyncUDPPacket *packet) {
  httpCallConfig.payload = payload;
  printResponse("HTTP_SET_PAYLOAD: " + payload, packet);
}

/**
 * @brief Set the implementation type for HTTP requests
 * @param implementation Implementation type string
 * @param packet Pointer to AsyncUDPPacket for response
 */
void setHttpImplementation(String implementation, AsyncUDPPacket *packet) {
  httpCallConfig.implementation = implementation;
  printResponse("HTTP_SET_IMPLEMENTATION: " + implementation, packet);
}

/**
 * @brief Get the content length of a resource at the given URL
 * @param url URL to check
 * @return int Content length or -1 if error
 */
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

/**
 * @brief Handle streaming HTTP response
 * @param http HTTPClient object
 * @param packet Pointer to AsyncUDPPacket for response
 */
void handleStreamResponse(HTTPClient &http, AsyncUDPPacket *packet) {
  int len = http.getSize();
  const size_t bufferSize = 512; // Buffer size for reading data
  uint8_t buff[bufferSize + 1] = {0}; // Buffer with space for null-terminator

  NetworkClient *stream = http.getStreamPtr();

  printResponse("STREAM: ", packet);
  while (http.connected() && (len > 0 || len  == -1) ) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buff, min(size, bufferSize - 1)); // Adjust for null terminator
      buff[c] = '\0'; // Null-terminate the buffer
      if (packet) {
        packet->write(buff, c);
      } else {
        UART0.write(buff, c);
      }
      if (len > 0) {
        len -= c;
      }
    } 
    delay(1); // Yield control to the system
  }
  printResponse("\nSTREAM_END", packet);
}

/**
 * @brief Handle file streaming HTTP response
 * @param http HTTPClient object
 * @param packet Pointer to AsyncUDPPacket for response
 */
void handleFileStreamResponse(HTTPClient &http, AsyncUDPPacket *packet) {
  int len = http.getSize();
  uint8_t buff[512] = {0};

  NetworkClient *stream = http.getStreamPtr();

  while (http.connected() && (len > 0 || len == -1)) {
    size_t size = stream->available();
    if (size) {
      int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
      UART0.write(buff, c);
      if (len > 0) {
        len -= c;
      }
    }
    delay(1); // Yield control to the system
  }
}

/**
 * @brief Handle HTTP response as a string
 * @param http HTTPClient object
 * @param packet Pointer to AsyncUDPPacket for response
 */
void handleGetStringResponse(HTTPClient &http, AsyncUDPPacket *packet) {
  // Check available heap memory
  size_t freeHeap = ESP.getFreeHeap();
  const size_t minHeapThreshold = 1024; // Minimum heap space to avoid overflow

  if (freeHeap < minHeapThreshold) {
    printResponse("WIFI_ERROR: Not enough memory to process the response.",
                  packet);
    return;
  }

  String payload = http.getString();
  if (payload.isEmpty()) {
    payload = "empty";
  }

  // Check again after getting the payload
  freeHeap = ESP.getFreeHeap();
  if (freeHeap < minHeapThreshold) {
    printResponse("WIFI_ERROR: Not enough memory to process the full response.",
                  packet);
    return;
  }

  printResponse("RESPONSE:", packet);
  printResponse(payload, packet);
  printResponse("RESPONSE_END", packet);
}

/**
 * @brief Make an HTTP GET request
 * @param url URL for the request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void makeHttpRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    int contentLength = getContentLength(url);

    if (contentLength > MAX_CONTENT_LENGTH) {
      String warnMsg = "WARNING: Content of " + String(contentLength) +
                       " exceeds maximum length of " +
                       String(MAX_CONTENT_LENGTH) +
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
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = "STATUS: " + String(httpResponseCode) + "\n";
      printResponse(response, packet);

      if (contentLength == -1 || contentLength > MAX_CONTENT_LENGTH) {
        handleStreamResponse(http, packet);
      } else {
        handleGetStringResponse(http, packet);
      }
    } else {
      String errorMsg = "HTTP_ERROR: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
    }

    http.end();
    led_set_blue(0);
  } else {
    led_set_blue(0);
    led_error();
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

/**
 * @brief Make an HTTP GET request for file streaming
 * @param url URL for the request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void makeHttpFileRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);


    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      handleFileStreamResponse(http, packet);
    }

    http.end();
    led_set_blue(0);
  }
}

/**
 * @brief Make an HTTP GET request with streaming response
 * @param url URL for the request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void makeHttpRequestStream(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);

    int contentLength = getContentLength(url);

    if (contentLength > MAX_CONTENT_LENGTH) {
      String warnMsg = "WARNING: Content of " + String(contentLength) +
                       " exceeds maximum length for stream of." +
                       String(MAX_CONTENT_LENGTH) +
                       " bytes. If"
                       "the blue light stays on,"
                       "reset the board.";
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
    led_set_blue(0);
    led_error();
  }
}

/**
 * @brief Make an HTTP POST request
 * @param url URL for the request
 * @param jsonPayload JSON payload for the request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    led_set_blue(255);
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
      led_set_blue(0);
      led_error();
    }
    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    printResponse(errorMsg, packet);
    led_set_blue(0);
    led_error();
  }
}

/**
 * @brief Make an HTTP POST request for file streaming
 * @param url URL for the request
 * @param jsonPayload JSON payload for the request
 * @param packet Pointer to AsyncUDPPacket for response
 */
void makeHttpPostFileRequest(String url, String jsonPayload, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      handleFileStreamResponse(http, packet);
    }

    http.end();
    led_set_blue(0);
  }
}

/**
 * @brief Execute an HTTP call based on the current configuration
 * @param packet Pointer to AsyncUDPPacket for response
 */

void executeHttpCall(AsyncUDPPacket *packet) {
  if (httpCallConfig.url.isEmpty() || httpCallConfig.method.isEmpty()) {
    String errorMsg = "HTTP URL or Method not set";
    printResponse(errorMsg, packet);
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    led_set_blue(255);
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
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
      printResponse(response, packet);

      if (httpCallConfig.showResponseHeaders) {
        printResponse("HEADERS:", packet);
        // Get the header count
        int headerCount = http.headers();

        // Iterate over the collected headers and print them
        for (int i = 0; i < headerCount; i++) {
          String headerName = http.headerName(i);
          String headerValue = http.header(i);
          printResponse(headerName + ": " + headerValue, packet);
        }
      }

      if (httpCallConfig.implementation == "STREAM") {
        handleStreamResponse(http, packet);
      } else {
        handleGetStringResponse(http, packet);
      }
    } else {
      String errorMsg = "HTTP_ERROR: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
      led_set_blue(0);
      led_error();
    }

    http.end();
    response = "";
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP_ERROR: WiFi Disconnected";
    led_set_blue(0);
    led_error();
    printResponse(errorMsg, packet);
  }
}

/**
 * @brief Get a human-readable error message for HTTP error codes
 * @param httpCode HTTP error code
 * @return String Error message
 */
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