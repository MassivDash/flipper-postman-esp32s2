#include "wifi_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <AsyncUDP.h>
#include <WiFi.h>

static String ssid;
static String password;
extern AsyncUDP udp;

const int MAX_RETRY_COUNT = 10; // Maximum number of retries
const int RETRY_DELAY_MS = 1000;

void connectToWiFi() {
  if (ssid.isEmpty()) {
    UART0.println("WIFI_ERROR: SSID is missing");
    return;
  }

  if (password.isEmpty()) {
    UART0.println("WIFI_ERROR: Password is missing");
    return;
  }
  led_set_blue(255);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  int retryCount = 0;
  led_set_blue(0);

  while (WiFi.status() != WL_CONNECTED && retryCount < MAX_RETRY_COUNT) {
    led_set_blue(255);
    delay(RETRY_DELAY_MS);
    retryCount++;
    UART0.println("WIFI_CONNECT: Connecting to WiFi... try " +
                  String(retryCount) + "/" + String(MAX_RETRY_COUNT));

    led_set_blue(0);

    // Check for specific WiFi statuses
    if (WiFi.status() == WL_CONNECT_FAILED) {
      UART0.println("WIFI_ERROR: Failed to connect to WiFi: Incorrect password "
                    "or other issue.");
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    led_error();
    UART0.println("WIFI_ERROR: Failed to connect to WiFi");
    return;
  }

  led_set_blue(0);
  led_set_green(255);
  UART0.println("WIFI_CONNECTED: Connected to " + ssid);
  UART0.print("WIFI_INFO: IP Address: ");
  UART0.println(WiFi.localIP());
  led_set_green(0);

  if (udp.listen(1234)) {
    UART0.println("WIFI_INFO: UDP listening on port 1234");

    udp.onPacket([](AsyncUDPPacket packet) {
      String receivedData = String((char *)packet.data(), packet.length());
      UART0.println("WIFI_INFO: Received UDP: " + receivedData);

      String command;
      String argument;

      int spaceIndex = receivedData.indexOf(' ');
      if (spaceIndex != -1) {
        command = receivedData.substring(0, spaceIndex);
        argument = receivedData.substring(spaceIndex + 1);
      } else {
        command = receivedData;
      }

      handleCommand(command, argument, &packet);

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