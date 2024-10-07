#include "wifi_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <AsyncUDP.h>
#include <WiFi.h>

static String ssid;
static String password;
extern AsyncUDP udp;

void connectToWiFi() {
  if (ssid.isEmpty()) {
    UART0.println("Error: SSID is missing");
    return;
  }

  if (password.isEmpty()) {
    UART0.println("Error: Password is missing");
    return;
  }
  led_set_blue(255);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  led_set_blue(0);

  while (WiFi.status() != WL_CONNECTED) {
    led_set_blue(255);
    delay(1000);
    UART0.println("Connecting to WiFi...");
    led_set_blue(0);
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

      String command;
      String argument;

      if (receivedData.startsWith("SET ssid ")) {
        command = "SET_SSID";
        argument = receivedData.substring(9);
      } else if (receivedData.startsWith("SET password ")) {
        command = "SET_PASSWORD";
        argument = receivedData.substring(13);
      } else if (receivedData.equals("ACTIVATE WIFI")) {
        command = "ACTIVATE_WIFI";
      } else if (receivedData.equals("DISCONNECT WIFI")) {
        command = "DISCONNECT_WIFI";
      } else if (receivedData.startsWith("GET ")) {
        command = "GET";
        argument = receivedData.substring(4);
      } else if (receivedData.startsWith("GET_STREAM")) {
        command = "GET_STREAM";
        argument = receivedData.substring(10);
      } else if (receivedData.startsWith("POST ")) {
        command = "POST";
        argument = receivedData.substring(5);
      } else {
        command = "UNKNOWN";
      }

      handleCommand(command, argument);

      if (command == "GET" || command == "POST") {
        packet.printf("Command executed: %s", receivedData.c_str());
      } else {
        packet.printf("Command executed: %s", command.c_str());
      }
    });
  }

  UART0.println("The WiFi Connection is established, you can now send GET, "
                "GET_STREAM and POST commands"
                "to the device");
}

void disconnectFromWiFi() {
  WiFi.disconnect();
  UART0.println("WiFi disconnected");
}

String listWiFiNetworks() {
  int n = WiFi.scanNetworks();
  String result = "";
  for (int i = 0; i < n; ++i) {
    result += WiFi.SSID(i);
    if (i < n - 1) {
      result += ", ";
    }
  }
  return result;
}

void setSSID(String newSSID) { ssid = newSSID; }

void setPassword(String newPassword) { password = newPassword; }

const char *getSSID() { return ssid.c_str(); }

const char *getPassword() { return password.c_str(); }