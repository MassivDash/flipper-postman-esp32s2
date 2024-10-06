#include "uart_utils.h"
#include "http_utils.h"


String uart_buffer = "";
SemaphoreHandle_t uart_buffer_Mutex = NULL;
const uint32_t communicationTimeout_ms = 500;

void UART0_RX_CB() {
  if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
    uint32_t now = millis();
    while ((millis() - now) < communicationTimeout_ms) {
      if (UART0.available()) {
        uart_buffer += (char)UART0.read();
        now = millis();
      }
    }
    xSemaphoreGive(uart_buffer_Mutex);
  }
}

void handleSerialInput() {
  if (uart_buffer.length() > 0) {
    if (xSemaphoreTake(uart_buffer_Mutex, portMAX_DELAY)) {
      uart_buffer.trim();

      if (uart_buffer.startsWith("GET ")) {
        String url = uart_buffer.substring(4);
        UART0.println("Received Serial GET request for URL: " + url);
        makeHttpRequest(url, nullptr);
      } else {
        UART0.println("Received Serial: " + uart_buffer);
      }

      uart_buffer = "";
      xSemaphoreGive(uart_buffer_Mutex);
    }
  }
}