#include "http_utils.h"
#include "json_utils.h"
#include "led.h"
#include "uart_utils.h"
#include <ArduinoJson.h>
#include <AsyncUDP.h>
#include <HTTPClient.h>
#include <WiFi.h>



const char *ssid = "MSV";
const char *password = "Starwars01";

AsyncUDP udp;

void setup() {
  UART0.begin(115200);

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

  UART0.onReceive(UART0_RX_CB);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

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

void loop() {
  handleSerialInput();
  // Other loop operations can go here
}