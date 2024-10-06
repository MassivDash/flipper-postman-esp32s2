#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncUDP.h>

bool isJson(String str);
String parseJson(String jsonString);
void printHtml(String html, AsyncUDPPacket *packet);

#endif // JSON_UTILS_H