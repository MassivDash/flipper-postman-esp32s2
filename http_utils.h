#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <Arduino.h>
#include <AsyncUDP.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <vector>

// HTTP utility functions
void makeHttpRequest(String url, AsyncUDPPacket *packet);
void makeHttpRequestStream(String url, AsyncUDPPacket *packet);
void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet);
void setHttpMethod(String method);
void setHttpUrl(String url);
void addHttpHeader(String header);
void setHttpPayload(String payload);
void removeHttpHeader(String name);
void resetHttpConfig();
void setShowResponseHeaders(bool show);
void executeHttpCall(AsyncUDPPacket *packet);
String getHttpErrorMessage(int httpCode);
bool isJson(String payload);
String parseJson(String payload);
void printHtml(String payload, AsyncUDPPacket *packet);

#endif // HTTP_UTILS_H