# Flipper Postman Board Software for esp32S2 (Flipper Dev Board)

# ESP32 Flipper Postman Board v0.1

## Project Information

This project is designed to work with the Flipper Postman Board. It provides various functionalities to interact with WiFi networks and make HTTP requests.

## General Features

- Connect to WiFi networks
- Disconnect from WiFi networks
- List available WiFi networks
- Make HTTP GET, POST, and STREAM requests
- Build custom HTTP requests with headers and payloads
- Show or hide HTTP response headers
- Execute HTTP calls
- Stream HTTP responses
- LED indicators for different states

## Available Commands

| Command                                         | Description                                       |
| ----------------------------------------------- | ------------------------------------------------- |
| `SET_SSID <ssid>`                               | Set the SSID for the WiFi connection              |
| `SET_PASSWORD <password>`                       | Set the password for the WiFi connection          |
| `ACTIVATE_WIFI`                                 | Activate the WiFi connection                      |
| `DISCONNECT_WIFI`                               | Disconnect from the WiFi network                  |
| `LIST_WIFI`                                     | List available WiFi networks                      |
| `GET <url>`                                     | Make an HTTP GET request to the specified URL     |
| `GET_STREAM <url>`                              | Make an HTTP GET request and stream the response  |
| `POST <url> <json_payload>`                     | Make an HTTP POST request with a JSON payload     |
| `BUILD_HTTP_METHOD <method>`                    | Set the HTTP method for the custom request        |
| `BUILD_HTTP_URL <url>`                          | Set the URL for the custom HTTP request           |
| `BUILD_HTTP_HEADER <header>`                    | Add a header to the custom HTTP request           |
| `BUILD_HTTP_PAYLOAD <payload>`                  | Set the payload for the custom HTTP request       |
| `REMOVE_HTTP_HEADER <header>`                   | Remove a header from the custom HTTP request      |
| `RESET_HTTP_CONFIG`                             | Reset the custom HTTP request configuration       |
| `BUILD_HTTP_SHOW_RESPONSE_HEADERS <true/false>` | Show or hide HTTP response headers                |
| `BUILD_HTTP_IMPLEMENTATION <STREAM/CALL>`       | Set the HTTP implementation type (STREAM or CALL) |
| `EXECUTE_HTTP_CALL`                             | Execute the custom HTTP request                   |
| `CONNECT <SSID> <password>`                     | Connect to a WiFi network with SSID and password  |
| `?`                                             | Print help information                            |
| `HELP`                                          | Print help information                            |

## Installation (Build from source)

0. Git clone the project
1. Install Arduino IDE
2. Arduino IDE -> Settings (Preferences)

Copy and paste the `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

3. Wait for installation
4. Arduino IDE -> Tools -> Manage Libraries -> find and install ArduinoJson
5. Arduino IDE -> Tools -> Boards -> Board manager -> find and install esp32 by expersif systems
6. Arduino IDE -> Verify build
7. Connect board to usb
8. Boot board into bootloader (hold boot, and while holding the boot press the reset and release, then release boot button)
9. Arduino IDE -> Tools -> port -> select the port from your card (it should say esp32s2)
10. Arduino Ide --> Upload

# Use Flipper flasher app to flash the board

1. Copy the files into the Flipperzero->apps_data->flasher_app
2. Go into the files
3. Select reset
4. Select Boot
5. For Firmware A chose ``

## Usage

1. Set the SSID and password for the WiFi connection using the `SET_SSID` and `SET_PASSWORD` commands.
2. Activate the WiFi connection using the `ACTIVATE_WIFI` command.
3. Use the `GET`, `POST`, or `GET_STREAM` commands to make HTTP requests.
4. Build custom HTTP requests using the `BUILD_HTTP_*` commands.
5. Execute custom HTTP requests using the `EXECUTE_HTTP_CALL` command.
6. Use the `?` or `HELP` commands to print help information.

## Example

```plaintext
SET_SSID MyWiFiNetwork
SET_PASSWORD MyWiFiPassword
ACTIVATE_WIFI
GET https://api.example.com/data
```
