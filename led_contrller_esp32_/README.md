# ESP32 WS2812B LED Controller — V1

A Wi-Fi-controlled addressable RGB LED strip controller built using an ESP32 DevKit V1 and the FastLED library.

## Features

- ESP32-hosted Wi-Fi access point
- Mobile-friendly web dashboard
- RGB color selection
- Adjustable LED brightness
- Solid, Rainbow, Chase, Breathe, and Off effects
- HTTP endpoints for LED control

## Hardware

- ESP32 DevKit V1
- WS2812B addressable LED strip (41 LEDs)
- 5 V power supply
- Common ground between ESP32 and LED power supply

## Software

- Arduino IDE
- ESP32 Arduino Core
- FastLED library
- Wi-Fi and WebServer libraries

## How to Use

1. Install the ESP32 board support package and FastLED library.
2. Open the Arduino sketch in Arduino IDE.
3. Select the appropriate ESP32 board and COM port.
4. Upload the firmware.
5. Connect your phone to the controller's Wi-Fi access point.
6. Open `http://192.168.4.1` in your browser.

## Version

**V1 — Initial working release**

This version provides basic Wi-Fi dashboard control, color selection, brightness adjustment, and LED effects.

## Future Improvements

- Save settings across restarts
- Add more effects and speed control
- Improve firmware structure and error handling