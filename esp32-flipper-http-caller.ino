#include "http_utils.h"
#include "json_utils.h"
#include "led.h"
#include "uart_utils.h"
#include "wifi_utils.h"
#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include <HTTPClient.h>
#include <WiFi.h>

const char *ssid;
const char *password;

AsyncUDP udp;

void setup() {
  UART0.begin(115200);
  while (!Serial) {
    ; // Wait for Serial to be ready
  }

  led_init();
  led_set_blue(255);

  uart_buffer_Mutex = xSemaphoreCreateMutex();
  if (uart_buffer_Mutex == NULL) {
    log_e("Error creating Mutex. Sketch will fail.");
    while (true) {
      UART0.println("Mutex error (NULL). Program halted.");
      delay(2000);
    }
  }
  led_set_blue(0);
  UART0.onReceive(UART0_RX_CB);

  // Print welcome message and available commands
  UART0.println("========================================");
  UART0.println("|                                      |");
  UART0.println("|   Flipper Spaceout Dev Board v0.1    |");
  UART0.println("|             Uart port                |");
  UART0.println("|                                      |");
  UART0.println("========================================");
  UART0.println("|          Available Commands          |");
  UART0.println("|--------------------------------------|");
  UART0.println("| 1. SET ssid <ssid>                   |");
  UART0.println("| 2. SET password <password>           |");
  UART0.println("| 3. ACTIVATE WIFI                     |");
  UART0.println("| 4. DISCONNECT WIFI                   |");
  UART0.println("| 5. GET <url>                         |");
  UART0.println("| 6. POST <url> <json_payload>         |");
  UART0.println("========================================");
}

void loop() {
  handleSerialInput();
  // Other loop operations can go here
}