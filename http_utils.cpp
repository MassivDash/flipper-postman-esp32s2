#include "http_utils.h"
#include "json_utils.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include "uart_utils.h"

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