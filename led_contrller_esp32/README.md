# ESP32 WS2812B LED Controller — V2

A wireless addressable RGB LED controller built using the **ESP32 DevKit V1**, a 41-LED WS2812B strip, and the FastLED library. V2 adds **Bluetooth Low Energy (BLE) control** and a dedicated mobile-friendly web interface alongside the existing Wi-Fi control functionality.

## Features

- ESP32-based LED control
- Wi-Fi connectivity and web-based control
- Bluetooth Low Energy (BLE) control
- Mobile-friendly web controller hosted on GitHub Pages
- RGB color selection
- Adjustable LED brightness
- Multiple lighting effects:
  - Solid
  - Rainbow
  - Chase
  - Breathe
  - Off
- BLE text commands for color, brightness, effects, and status
- HTTP-based LED control through the ESP32 web server

## Hardware

- ESP32 DevKit V1
- WS2812B addressable RGB LED strip — 41 LEDs
- Suitable regulated power supply for the LED strip
- Common ground between the ESP32 and LED power supply
- USB data cable for firmware upload

**Power note:** Use a suitable supply for the LED strip and connect its ground to ESP32 ground. Do not power all 41 LEDs from the ESP32's 3.3 V pin.

## Software

- Arduino IDE
- ESP32 Arduino Core
- FastLED library
- Wi-Fi and WebServer libraries
- BLE support provided by the firmware's configured ESP32 BLE library

## Project Links

- **Live mobile controller:** [Open Web Interface](https://rupam4604.github.io/-ESP32_LED_Controller/controller_mobile_web/)
- **GitHub repository:** [ESP32 LED Controller](https://github.com/Rupam4604/-ESP32_LED_Controller)

## How to Use

### 1. Upload the firmware

1. Install ESP32 board support in Arduino IDE.
2. Install the FastLED library.
3. Open the V2 firmware sketch:

   `led_contrller_esp32_/led_contrller_esp32/led_contrller_esp32.ino`

4. Configure Wi-Fi credentials using the firmware's existing configuration method.
5. Select the appropriate ESP32 board and COM port.
6. Upload the firmware and check Serial Monitor for startup information.

Keep private Wi-Fi credentials and `secrets.h` out of public commits.

### 2. Control using Wi-Fi

1. Follow the firmware's startup information to identify its Wi-Fi connection mode and address.
2. Connect your phone or computer to the appropriate network.
3. Open the dashboard address reported by the firmware.
4. Use the available controls to change color, brightness, and lighting effects.

### 3. Control using BLE

1. Enable Bluetooth on your phone.
2. Open the [mobile web controller](https://rupam4604.github.io/-ESP32_LED_Controller/controller_mobile_web/) in a compatible browser.
3. Allow Bluetooth permissions when prompted.
4. Connect to the device named `Glow-Controller`.
5. Use the interface to control the LEDs.

The web interface is hosted on GitHub Pages, while the ESP32 must be powered on and within wireless range to control the physical LED strip.

## BLE Command Reference

The firmware accepts text commands through its BLE command characteristic.

| Command | Description | Example |
|---|---|---|
| `COLOR=RRGGBB` | Set RGB color | `COLOR=FF0000` |
| `BRIGHTNESS=value` | Set brightness | `BRIGHTNESS=40` |
| `EFFECT=name` | Select a lighting effect | `EFFECT=rainbow` |
| `STATUS` | Request controller status | `STATUS` |

### Supported effects

`solid`, `rainbow`, `chase`, `breathe`, `off`

Example commands:

```text
COLOR=00FF00
BRIGHTNESS=40
EFFECT=chase
STATUS
```

### BLE UUIDs

**Service UUID**
```text
7a1e0001-8f2b-4d3c-9a6e-1b2c3d4e5f60
```

**Command characteristic**
```text
7a1e0002-8f2b-4d3c-9a6e-1b2c3d4e5f60
```

**Status characteristic**
```text
7a1e0003-8f2b-4d3c-9a6e-1b2c3d4e5f60
```

## Version History

### V2 — Wi-Fi + BLE + Mobile Web Interface

- Retains Wi-Fi-based LED control.
- Adds BLE-based wireless control.
- Adds a dedicated mobile-friendly web interface.
- Supports color, brightness, lighting effects, and status commands.
- Publishes the mobile interface through GitHub Pages.

### V1 — Initial Working Release

- ESP32-hosted Wi-Fi access point.
- Mobile-friendly web dashboard.
- RGB color selection and brightness adjustment.
- Solid, Rainbow, Chase, Breathe, and Off effects.
- HTTP endpoints for LED control.

## Future Improvements

- Save settings across restarts.
- Add more lighting effects and effect-speed control.
- Improve firmware structure and error handling.
- Add hardware photographs and a wiring diagram.
- Expand status reporting and connection feedback.

## Author

**Rupam Ghosh**

B.Tech in Electronics & Communication Engineering

- GitHub: [Rupam4604](https://github.com/Rupam4604)
- LinkedIn: [Rupam Ghosh](https://linkedin.com/in/rupam-ghosh-0406047119326s)

---

*This project explores embedded firmware development, wireless communication, addressable LED control, and mobile-based hardware interaction.*
