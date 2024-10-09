#include "uart_utils.h"
#include "version.h"
#include <Arduino.h>
// Function to print the splash screen
void printSplashScreen() {
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println("                            @@@@#                           ");
  UART0.println("                          @@@@@@@@@                         ");
  UART0.println("                        @@@@@@@@@@@@@                       ");
  UART0.println("                      @@@@@@@@@@@@@@@@@                     ");
  UART0.println("                    (@@@@          @@@@@*                   ");
  UART0.println("                   @@@@              @@@@@                  ");
  UART0.println("                 *@@@@                @@@@@,                ");
  UART0.println("                @@@@@@                 @@@@@@               ");
  UART0.println("               @@@@@@* *@@        @@@  @@@@@@@              ");
  UART0.println("              @@@@@@@     .            @@@@@@@@             ");
  UART0.println("             @@@@@@@@                  /@@@@@@@@            ");
  UART0.println("            @@@@@@@@@                   @@@@@@@@@           ");
  UART0.println("           @@@@@@@@@&                   @@@@@@@@@@          ");
  UART0.println("          *@@@@@@@@@#                   @@@@@@@@@@%         ");
  UART0.println("          @@@@@@@@@&*                   %&@@@@@@@@@         ");
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println("                                                            ");
  UART0.println();
}

// Function to print the title
void printTitle() {
  UART0.println("===========================================================");
  UART0.println("||                                                       ||");
  UART0.println("||           Flipper Postman Board v" + String(version) +
                "                ||");
  UART0.println("||          by SpaceGhost at spaceout.pl                 ||");
  UART0.println("||                                                       ||");
  UART0.println("===========================================================");
  UART0.println();
  UART0.println("Type '?' to see available commands");
}
