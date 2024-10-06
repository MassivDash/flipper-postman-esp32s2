#include "wifi_utils.h"
#include "http_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <AsyncUDP.h>
#include <WiFi.h>

static String ssid;
static String password;
extern AsyncUDP udp;

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    UART0.println("Connecting to WiFi...");
  }

  led_set_blue(0);
  led_set_green(255);
  UART0.println("Connected to WiFi");
  UART0.print("IP Address: ");
  UART0.println(WiFi.localIP());
  led_set_green(0);

  if (udp.listen(1234)) {
    UART0.println("UDP Listening on Port: 1234");

    udp.onPacket([](AsyncUDPPacket packet) {
      String receivedData = String((char *)packet.data(), packet.length());
      UART0.println("Received UDP: " + receivedData);

      if (receivedData.startsWith("GET ")) {
        String url = receivedData.substring(4);
        makeHttpRequest(url, &packet);
      } else {
        packet.printf("Unknown UDP command: %s", receivedData.c_str());
      }
    });
  }

  UART0.println("Send 'GET url' via Serial or UDP to make an HTTP request");
}

void disconnectFromWiFi() {
  WiFi.disconnect();
  UART0.println("WiFi disconnected");
}

void setSSID(String newSSID) { ssid = newSSID; }

void setPassword(String newPassword) { password = newPassword; }

const char *getSSID() { return ssid.c_str(); }

const char *getPassword() { return password.c_str(); }