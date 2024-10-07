#include "http_utils.h"
#include "json_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <HTTPClient.h>
#include <WiFi.h>

const int MAX_CONTENT_LENGTH = 512 * 1024;

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

void makeHttpRequest(String url, AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!isContentLengthAcceptable(url)) {
      String errorMsg = "Content too large";
      if (packet) {
        packet->printf("%s", errorMsg.c_str());
      } else {
        UART0.println(errorMsg);
      }
      return;
    }

    HTTPClient http;
    led_set_blue(255);
    http.begin(url);

    // Pre-allocate memory for the response String
    String response;

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
      response = "HTTP Response code: " + String(httpResponseCode) + "\n";

      if (isJson(payload)) {
        response += parseJson(payload);
        if (packet) {
          packet->printf("%s", response.c_str());
        } else {
          UART0.println(response);
        }
      } else {
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
    led_set_blue(0);
  } else {
    String errorMsg = "WiFi Disconnected";
    if (packet) {
      packet->printf("%s", errorMsg.c_str());
    } else {
      UART0.println(errorMsg);
    }
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
      if (packet) {
        packet->printf("%s", response.c_str());
      } else {
        UART0.println(response);
      }

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
      if (packet) {
        packet->printf("%s", errorMsg.c_str());
      } else {
        UART0.println(errorMsg);
      }
    }

    http.end();
    led_set_blue(0);
  } else {
    String errorMsg = "WiFi Disconnected";
    if (packet) {
      packet->printf("%s", errorMsg.c_str());
    } else {
      UART0.println(errorMsg);
    }
  }
}

void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!isContentLengthAcceptable(url)) {
      String errorMsg = "Content too large";
      if (packet) {
        packet->printf("%s", errorMsg.c_str());
      } else {
        UART0.println(errorMsg);
      }
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
        if (packet) {
          packet->printf("%s", response.c_str());
        } else {
          UART0.println(response);
        }
      } else {
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