/**
 * @file wifi_utils.cpp
 * @brief Utilities for WiFi connection and UDP communication
 *
 * This file contains functions for managing WiFi connections,
 * including connecting to networks, scanning for available networks,
 * and sending UDP messages. It also provides utilities for setting
 * and retrieving WiFi credentials.
 */


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

/**
 * @brief Connect to the WiFi network
 *
 * This function attempts to connect to the WiFi network using the provided
 * SSID and password. It will retry a few times before giving up.
 */
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
      UART0.println("WIFI_UDP_INCOMING_DATA: " + receivedData);

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


/**
 * @brief Disconnect from the current WiFi network
 */
void disconnectFromWiFi() {
  WiFi.disconnect();
  UART0.println("WIFI_DISCONNECT: Wifi disconnected");
}

/**
 * @brief List available WiFi networks
 *
 * @return String A comma-separated list of available WiFi SSIDs
 */
String listWiFiNetworks() {
  UART0.println("WIFI_LIST: Scanning WiFi networks...");
  led_set_blue(255);
  int n = WiFi.scanNetworks();
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

/**
 * @brief Set the SSID for WiFi connection
 *
 * @param newSSID The new SSID to set
 */
void setSSID(String newSSID) { ssid = newSSID; }


/**
 * @brief Set the password for WiFi connection
 *
 * @param newPassword The new password to set
 */
void setPassword(String newPassword) { password = newPassword; }

/**
 * @brief Get the current SSID
 *
 * @return const char* Pointer to the current SSID
 */
const char *getSSID() { return ssid.c_str(); }

/**
 * @brief Get the current password
 *
 * @return const char* Pointer to the current password
 */
const char *getPassword() { return password.c_str(); }

/**
 * @brief Send a UDP message
 *
 * @param message The message to send
 * @param remoteIP The IP address of the remote device
 * @param remotePort The port number of the remote device
 */
void sendUDPMessage(const char* message, IPAddress remoteIP, uint16_t remotePort) {
  led_set_blue(255);
  udp.writeTo((const uint8_t*)message, strlen(message), remoteIP, remotePort);
  led_set_blue(0);
}

/**
 * @brief Get the local IP address as a string
 *
 * @return String The local IP address
 */
String getLocalIpString() {
  led_set_blue(255);
  led_set_blue(0);
  return WiFi.localIP().toString();
}
