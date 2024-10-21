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
void setHttpMethod(String method, AsyncUDPPacket *packet);
void setHttpUrl(String url, AsyncUDPPacket *packet);
void addHttpHeader(String header, AsyncUDPPacket *packet);
void setHttpPayload(String payload, AsyncUDPPacket *packet);
void setHttpImplementation(String implementation, AsyncUDPPacket *packet);
void removeHttpHeader(String name, AsyncUDPPacket *packet);
void resetHttpConfig(AsyncUDPPacket *packet);
void executeHttpCall(AsyncUDPPacket *packet);
void getHttpBuilderConfig(AsyncUDPPacket *packet);
void makeHttpFileRequest(String url, AsyncUDPPacket *packet);
// HTTP Helper functions
void setShowResponseHeaders(bool show, AsyncUDPPacket *packet);
String getHttpErrorMessage(int httpCode);
void printResponse(String response, AsyncUDPPacket *packet);

#endif // HTTP_UTILS_H