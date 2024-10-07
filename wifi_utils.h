#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <Arduino.h>

void connectToWiFi();
void setSSID(String ssid);
void setPassword(String password);
void disconnectFromWiFi();
const char *getSSID();
const char *getPassword();
String listWiFiNetworks();

#endif // WIFI_UTILS_H