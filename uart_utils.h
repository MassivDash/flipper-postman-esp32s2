#ifndef UART_UTILS_H
#define UART_UTILS_H

#if ARDUINO_USB_CDC_ON_BOOT
#define UART0 Serial0
#else
#define UART0 Serial
#endif

#include <Arduino.h>
#include <AsyncUDP.h>
#include <semphr.h>
// Command structure
struct Command {
  String name;
  String description;
  void (*execute)(String argument, AsyncUDPPacket *packet);
};

extern String uart_buffer;
extern SemaphoreHandle_t uart_buffer_Mutex;
extern const uint32_t communicationTimeout_ms;

void UART0_RX_CB();
void setSSIDCommand(String argument, AsyncUDPPacket *packet);
void setPasswordCommand(String argument, AsyncUDPPacket *packet);
void activateWiFiCommand(String argument, AsyncUDPPacket *packet);
void disconnectWiFiCommand(String argument, AsyncUDPPacket *packet);
void listWiFiCommand(String argument, AsyncUDPPacket *packet);
void getCommand(String argument, AsyncUDPPacket *packet);
void getStreamCommand(String argument, AsyncUDPPacket *packet);
void postCommand(String argument, AsyncUDPPacket *packet);
void buildHttpMethodCommand(String argument, AsyncUDPPacket *packet);
void buildHttpUrlCommand(String argument, AsyncUDPPacket *packet);
void buildHttpHeaderCommand(String argument, AsyncUDPPacket *packet);
void buildHttpPayloadCommand(String argument, AsyncUDPPacket *packet);
void removeHttpHeaderCommand(String argument, AsyncUDPPacket *packet);
void resetHttpConfigCommand(String argument, AsyncUDPPacket *packet);
void buildHttpImplementationCommand(String argument, AsyncUDPPacket *packet);
void buildHttpShowResponseHeadersCommand(String argument,
                                         AsyncUDPPacket *packet);
void executeHttpCallCommand(String argument, AsyncUDPPacket *packet);
void connectCommand(String argument, AsyncUDPPacket *packet);
void helpCommand(String argument, AsyncUDPPacket *packet);
void handleCommand(String command, String argument, AsyncUDPPacket *packet);
void handleSerialInput();
String ensureHttpsPrefix(String url);
void handleCommand(String command, String argument);
void init_cmds();

#endif // UART_UTILS_H