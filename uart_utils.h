#ifndef UART_UTILS_H
#define UART_UTILS_H

#if ARDUINO_USB_CDC_ON_BOOT
#define UART0 Serial0
#else
#define UART0 Serial
#endif


#include <Arduino.h>
#include <semphr.h>

extern String uart_buffer;
extern SemaphoreHandle_t uart_buffer_Mutex;
extern const uint32_t communicationTimeout_ms;

void UART0_RX_CB();
void handleSerialInput();

#endif // UART_UTILS_H