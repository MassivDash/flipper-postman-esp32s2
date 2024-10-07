#include "http_utils.h"
#include "led.h"
#include "uart_utils.h"
#include "wifi_utils.h"

const int MAX_CONTENT_LENGTH = 512 * 1024;

struct HttpCallConfig {
  String method;
  String url;
  std::vector<std::pair<String, String>> headers;
  String payload;
  bool showResponseHeaders = false;

  void reset() {
    method = "";
    url = "";
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
  if (packet) {
    packet->printf("%s", response.c_str());
  } else {
    UART0.println(response);
  }
}

void setShowResponseHeaders(bool show) {
  httpCallConfig.showResponseHeaders = show;
  UART0.println("Show response headers set to: " + String(show));
}

void setHttpMethod(String method) {

  if (method != "GET" && method != "POST" && method != "PATCH" &&
      method != "PUT" && method != "DELETE" && method != "HEAD") {
    UART0.println("Invalid HTTP method. Supported methods: GET, POST, PATCH, "
                  "PUT, DELETE, HEAD");
    return;
  }

  httpCallConfig.method = method;
  UART0.println("HTTP Method set to: " + method);
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
    UART0.println("Added header: " + name + ": " + value);
  } else {
    UART0.println("Invalid header format. Use: HEADER name:value");
  }
}

void removeHttpHeader(String name) {
  httpCallConfig.removeHeader(name);
  UART0.println("Removed header: " + name);
}

void resetHttpConfig() {
  httpCallConfig.reset();
  UART0.println("HTTP configuration reset");
}

void setHttpPayload(String payload) {
  httpCallConfig.payload = payload;
  UART0.println("HTTP Payload set to: " + payload);
}

bool isContentLengthAcceptable(String url) {
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.sendRequest("HEAD");

  if (httpResponseCode > 0) {
    int contentLength = http.getSize();
    UART0.println("Content Length: " + String(contentLength) +
                  "<= " + String(MAX_CONTENT_LENGTH));

    if (contentLength == -1) {
      UART0.println("Warning: Content-Length is unknown. Proceeding with "
                    "caution. On no response restart the board");
      return true; // Proceed with caution
    }
    http.end();
    return contentLength <= MAX_CONTENT_LENGTH;
  } else {
    http.end();
    return false;
  }
}

void executeHttpCall(AsyncUDPPacket *packet) {
  if (httpCallConfig.url.isEmpty() || httpCallConfig.method.isEmpty()) {
    String errorMsg = "HTTP URL or Method not set";
    printResponse(errorMsg, packet);
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.begin(httpCallConfig.url);

    for (const auto &header : httpCallConfig.headers) {
      http.addHeader(header.first, header.second);
    }

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
      httpResponseCode = http.DELETE(httpCallConfig.payload);
    } else if (httpCallConfig.method == "HEAD") {
      httpResponseCode = http.sendRequest("HEAD");
    } else {
      String errorMsg = "Unsupported HTTP method: " + httpCallConfig.method;
      printResponse(errorMsg, packet);
      http.end();
      led_set_blue(0);
      return;
    }

    String response = "HTTP Response code: " + String(httpResponseCode) + "\n";
    printResponse(response, packet);

    if (httpResponseCode > 0) {
      if (httpCallConfig.showResponseHeaders) {
        int headerCount = http.headers();
        for (int i = 0; i < headerCount; i++) {
          String headerName = http.headerName(i);
          String headerValue = http.header(i);
          UART0.println(headerName + ": " + headerValue);
        }
      }

      String payload = http.getString();
      printResponse(payload, packet);
    } else {
      String errorMsg = "HTTP Error: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
      led_error();
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP Error: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

void makeHttpRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!isContentLengthAcceptable(url)) {
      String errorMsg = "Content too large";
      printResponse(errorMsg, packet);
      return;
    }

    HTTPClient http;
    led_set_blue(255);
    http.begin(url);

    String response;
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      response = "HTTP Response code: " + String(httpResponseCode) + "\n";

      if (isJson(payload)) {
        response += parseJson(payload);
        printResponse(response, packet);
      } else {
        printResponse(response, packet);
        printHtml(payload, packet);
      }
    } else {
      String errorMsg = "HTTP Error: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP Error: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

void makeHttpRequestStream(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    led_set_blue(255);
    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      WiFiClient *stream = http.getStreamPtr();
      String response =
          "HTTP Response code: " + String(httpResponseCode) + "\n";
      printResponse(response, packet);

      uint8_t buff[128] = {0};

      while (stream->available()) {
        size_t size = stream->available();
        if (size) {
          int c = stream->readBytes(
              buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
          if (packet) {
            packet->write(buff, c);
          } else {
            UART0.write(buff, c);
          }
        }
        delay(1); // Yield control to the system
      }
    } else {
      String errorMsg =
          "HTTP Error: " + HTTPClient::errorToString(httpResponseCode) +
          " (Code: " + String(httpResponseCode) + ")";
      printResponse(errorMsg, packet);
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "HTTP Error: WiFi Disconnected";
    printResponse(errorMsg, packet);
  }
}

void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!isContentLengthAcceptable(url)) {
      String errorMsg = "HTTP Error: Content too large";
      printResponse(errorMsg, packet);
      led_error();
      return;
    }

    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String response;
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String payload = http.getString();
      response = "HTTP Response code: " + String(httpResponseCode) + "\n";

      if (isJson(payload)) {
        response += parseJson(payload);
        printResponse(response, packet);
      } else {
        response += "Response is HTML. Printing content:\n\n";
        printResponse(response, packet);
        printHtml(payload, packet);
      }
    } else {
      String errorMsg = "HTTP Error: " + getHttpErrorMessage(httpResponseCode);
      printResponse(errorMsg, packet);
      led_error();
    }

    http.end();
  } else {
    String errorMsg = "HTTP Error: WiFi Disconnected";
    printResponse(errorMsg, packet);
    led_error();
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