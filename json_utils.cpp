#include "json_utils.h"
#include "uart_utils.h"

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

void printHtml(String html, AsyncUDPPacket *packet) {
  html.replace("<",
               "\n<"); // Add newline before each tag for better readability
  if (packet) {
    packet->printf("%s", html.c_str());
  } else {
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