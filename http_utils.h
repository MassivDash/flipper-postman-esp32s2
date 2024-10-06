#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <Arduino.h>
#include <AsyncUDP.h>

void makeHttpRequest(String url, AsyncUDPPacket *packet);
String getHttpErrorMessage(int httpCode);

#endif // HTTP_UTILS_H