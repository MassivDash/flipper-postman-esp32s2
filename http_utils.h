#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <Arduino.h>
#include <AsyncUDP.h>

void makeHttpRequest(String url, AsyncUDPPacket *packet);
void makeHttpPostRequest(String url, String jsonPayload,
                         AsyncUDPPacket *packet);
String getHttpErrorMessage(int httpCode);
void makeHttpRequestStream(String url, AsyncUDPPacket *packet);

#endif // HTTP_UTILS_H