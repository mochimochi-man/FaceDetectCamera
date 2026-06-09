# FaceDetectCamera for XIAO ESP32S3 Sense

This is a web camera application that performs face detection using MediaPipe with the built-in camera of the XIAO ESP32S3 Sense. 
The ESP32 operates as a camera server and streams video to a web browser. Face detection is processed on the browser side, and the results are sent back to the ESP32's serial output in JSON format.

* The project source file (.ino) is provided only in Japanese. Non-Japanese speakers should translate it themselves as needed.

## Key Features

- **Wi-Fi Setup via Web Browser**
  - On the first boot or when Wi-Fi settings are not saved, the device automatically starts in Access Point (AP) mode.
  - Connect to the SSID `Camera-Setup` to configure Wi-Fi settings through a web browser.
  - After saving the settings, the device automatically reboots and switches to Wi-Fi connection mode.

- **MediaPipe Face Detection**
  - Runs MediaPipe Face Detection on the browser side.
  - Outputs the detected face positions, scores, and thumbnail images via serial communication in JSON format.

- **Serial Output Control**
  - In normal operation mode (when connected to Wi-Fi), system logs are suppressed and not output to the serial console.
  - Information is output to the serial console only during setup mode (AP mode).

## Requirements

- XIAO ESP32S3 Sense
- Arduino IDE (Supports ESP32 Board Package version 3.x)
- A web browser that can access the same network (An internet connection is required only for the initial loading of the MediaPipe model)

## Pin Assignment (XIAO ESP32S3 Sense)

Since this project utilizes the built-in camera, you generally do not need to worry about pin configuration.

| Function | GPIO |
|----------|------|
| XCLK     | 10   |
| SIOD (SDA)| 40   |
| SIOC (SCL)| 39   |
| D7 (Y9)  | 48   |
| D6 (Y8)  | 11   |
| D5 (Y7)  | 12   |
| D4 (Y6)  | 14   |
| D3 (Y5)  | 16   |
| D2 (Y4)  | 18   |
| D1 (Y3)  | 17   |
| D0 (Y2)  | 15   |
| VSYNC    | 38   |
| HREF     | 47   |
| PCLK     | 13   |

## Initial Setup Instructions

1. Flash the sketch to your board.
2. Open the Serial Monitor (Set the baud rate to `115200 bps`).
3. If you see the following output, the device has successfully started in Setup Mode:

```text
Access Point started. Connect to:
SSID: Camera-Setup
Password: setup1234
Config URL: [http://192.168.4.1/](http://192.168.4.1/)
