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

// HTTP configuration functions
void setHttpMethod(String method);
void setHttpUrl(String url);
void addHttpHeader(String header);
void setHttpPayload(String payload);
void setHttpImplementation(String implementation);
void removeHttpHeader(String name);
void resetHttpConfig();
void executeHttpCall(AsyncUDPPacket *packet);

// HTTP Helper functions
void setShowResponseHeaders(bool show);
String getHttpErrorMessage(int httpCode);
bool isJson(String payload);
String parseJson(String payload);
void printHtml(String payload, AsyncUDPPacket *packet);

#endif // HTTP_UTILS_H