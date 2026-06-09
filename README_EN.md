[README_EN.md](https://github.com/user-attachments/files/28737914/README_EN.md)
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
Config URL: http://192.168.4.1/
```

4. Connect to the Wi-Fi network `Camera-Setup` from your PC or smartphone (Password: `setup1234`).
5. Open `http://192.168.4.1/` in your web browser.
6. Enter your Wi-Fi SSID, Password, and Transmission Interval (in seconds), then click "Save and Reboot".
7. The device will automatically restart, connect to your Wi-Fi network, and begin operating as a camera server.

## Normal Operation (Face Detection Web Camera)

After the device connects to your Wi-Fi network, access the ESP32's IP address using a web browser.

- **Start** button: Begins retrieving the camera stream and performing face detection.
- **Stop** button: Stops the stream and face detection.

## Serial Output (JSON Format)

When a face is detected, the following JSON payload is output to the serial console at the configured interval:

```json
{
"faces": [
 {
   "xMin": 0.3125,
   "yMin": 0.2083,
   "width": 0.2187,
   "height": 0.2916,
   "score": 0.97
 }
],
"count": 1,
"time": {
 "year": 2026,
 "month": 6,
 "day": 8,
 "hour": 10,
 "minute": 35,
 "second": 12,
 "ms": 345,
 "iso": "2026-06-08T01:35:12.345Z"
},
"images": [
 "data:image/jpeg;base64,/9j/4AAQSkZJRgABAQAAAQ..."
]
}
```

### Field Descriptions

| Field | Description |
| --- | --- |
| `faces` | An array of detected faces. |
| `faces[].xMin` | The normalized X-coordinate of the left edge of the face (0.0 to 1.0). |
| `faces[].yMin` | The normalized Y-coordinate of the top edge of the face (0.0 to 1.0). |
| `faces[].width` | The normalized width of the face (0.0 to 1.0). |
| `faces[].height` | The normalized height of the face (0.0 to 1.0). |
| `faces[].score` | The confidence score of the detection (0.0 to 1.0). |
| `count` | The total number of detected faces. |
| `time` | The timestamp of the detection. |
| `images` | 64x64 pixel face images (JPEG encoded in Base64). |

### About Coordinates

The values for `xMin`, `yMin`, `width`, and `height` are ratios relative to the overall image size (ranging from 0.0 to 1.0).
For example, if the image width is 320px, an `xMin: 0.5` represents the position at 160px.

### About Timestamp Format

```python
# Python Usage Example
if 9 <= data["time"]["hour"] <= 18:
    # Process detections only during business hours
    pass

# Aggregate data by date
date_key = f"{data['time']['year']}-{data['time']['month']:02d}-{data['time']['day']:02d}"
```

The `iso` field contains the timestamp in ISO 8601 format, allowing it to be easily parsed by standard date-time libraries.

## Resetting Settings

If you want to erase the Wi-Fi settings and restart the device in setup mode, set **"Erase All Flash Before Sketch Upload"** to **Enabled** in your Arduino IDE settings and re-upload the sketch.

## Notes

* The initial load will download the MediaPipe model (approx. 2MB).
* The Base64 data for each face image is roughly 2 to 4KB. Due to the serial receive buffer limit of 8KB, the recommended maximum number of simultaneous face detections is about 3 to 4 people.
* The transmission interval can be configured between 1 and 300 seconds (Default: 15 seconds).

## Third-Party Services and Libraries

This project utilizes the following external services and libraries:

### jsDelivr CDN

[jsDelivr](https://www.jsdelivr.com/) is used to deliver JavaScript libraries within the web page.
jsDelivr is a free, public CDN service. The license for the distributed files complies with the terms of their respective packages.

* URL Used: `https://cdn.jsdelivr.net/npm/@mediapipe/face_detection@0.4/face_detection.js`
* Website: https://www.jsdelivr.com/

### MediaPipe Face Detection

Google's [MediaPipe](https://ai.google.dev/edge/mediapipe) Face Detection solution is used for browser-side face recognition.

* License: Apache License 2.0
* Official Site: https://ai.google.dev/edge/mediapipe/solutions/vision/face_detector
* GitHub: https://github.com/google-ai-edge/mediapipe

```
Copyright 2023 The MediaPipe Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

## License

This project (FaceDetectCamera) is licensed under the MIT License.

Developers: うっ Uh / もちもちマン MochiMochi-Man (X: @calorie0)

```
MIT License

Copyright (c) 2026 うっ Uh / もちもちマン MochiMochi-Man (X: @calorie0)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
