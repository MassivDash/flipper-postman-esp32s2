#include "wifi_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <AsyncUDP.h>
#include <WiFi.h>

// Hold values for pass and ssid
static String ssid;
static String password;

// Create a UDP instance for network communication
extern AsyncUDP udp;

// WIFI Connection settings
const int MAX_RETRY_COUNT = 10; // Maximum number of retries
const int RETRY_DELAY_MS = 1000;

// Connect to the WiFi network
// This function will try to connect to the WiFi network using the provided
// SSID and password. It will retry a few times before giving up.
void connectToWiFi() {

  // Check against empty SSID and password
  if (ssid.isEmpty()) {
    UART0.println("WIFI_ERROR: SSID is missing");
    return;
  }

  if (password.isEmpty()) {
    UART0.println("WIFI_ERROR: Password is missing");
    return;
  }

  // Start up wifi
  led_set_blue(255);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Retry counter
  int retryCount = 0;

  // Try and connect
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

  // After retries still not connected ? Return error
  if (WiFi.status() != WL_CONNECTED) {
    led_error();
    UART0.println("WIFI_ERROR: Failed to connect to WiFi");
    return;
  }

  // Connected to WiFi
  led_set_blue(0);
  led_set_green(255);
  UART0.println("WIFI_CONNECTED: Connected to " + ssid);
  UART0.print("WIFI_INFO: IP Address: ");
  UART0.println(WiFi.localIP());
  led_set_green(0);

  // Start listening for UDP packets
  if (udp.listen(1234)) {
    UART0.println("WIFI_INFO: UDP listening on port 1234");

    udp.onPacket([](AsyncUDPPacket packet) {
      String receivedData = String((char *)packet.data(), packet.length());
      UART0.println("WIFI_INFO: Received UDP: " + receivedData);

      String command;
      String argument;

      // Parse commands and arguments
      int spaceIndex = receivedData.indexOf(' ');
      if (spaceIndex != -1) {
        command = receivedData.substring(0, spaceIndex);
        argument = receivedData.substring(spaceIndex + 1);
      } else {
        command = receivedData;
      }

      // Special UDP Connection method
      // Direct Message stream to uart from UDP
      if (command == "MESSAGE") {
        // Stream the message directly to UART and return
        UART0.println("MESSAGE: " + argument);
        return;
      }

      // Uart file command system
      handleCommand(command, argument, &packet);
    });
  }

  UART0.println("WIFI_SUCCESS: WiFi connected");
}

void disconnectFromWiFi() {
  WiFi.disconnect();
  UART0.println("WIFI_DISCONNECT: Wifi disconnected");
}

// CMD WIFI_LIST: List available WiFi networks
String listWiFiNetworks() {
  UART0.println("WIFI_LIST: Scanning WiFi networks...");
  led_set_blue(255) int n = WiFi.scanNetworks();
  String result = "";
  for (int i = 0; i < n; ++i) {
    result += WiFi.SSID(i);
    if (i < n - 1) {
      result += ", ";
    }
  }
  led_set_blue(0);
  return result;
}

void setSSID(String newSSID) { ssid = newSSID; }

void setPassword(String newPassword) { password = newPassword; }

const char *getSSID() { return ssid.c_str(); }

const char *getPassword() { return password.c_str(); }