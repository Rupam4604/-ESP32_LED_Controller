# 🚀 ESP32 Smart LED Controller

### Wi-Fi-Controlled WS2812B RGB LED Strip Using ESP32

A smart addressable LED control system built using the **ESP32 DevKit V1**, **WS2812B RGB LED strip**, and **FastLED** library. The project features a responsive web dashboard that allows users to control LED colors, adjust brightness, and switch between dynamic lighting effects directly from a smartphone or computer.

The goal of this project is to explore embedded firmware development, Wi-Fi communication, web-based hardware control, and addressable RGB LED programming.

---

## ✨ Features

- 📡 **Wi-Fi Access Point:** The ESP32 creates its own wireless network.
- 🌐 **Web-Based Dashboard:** Control the LED strip from a web browser.
- 🎨 **RGB Color Control:** Select and apply custom colors.
- 💡 **Brightness Adjustment:** Adjust LED brightness using a slider.
- 🌈 **Dynamic Lighting Effects:** Switch between multiple lighting modes.
- 📱 **Mobile-Friendly Interface:** Control the LEDs using a smartphone.
- ⚡ **Real-Time Interaction:** Send control commands to the ESP32 through HTTP requests.

## 🎬 Lighting Effects

| Effect | Description |
|---|---|
| Solid Color | Displays a selected RGB color |
| Rainbow | Produces a continuously changing rainbow |
| Chase | Creates a moving LED chase effect |
| Breathe | Creates a pulsing brightness effect |
| Off | Turns off the LEDs |

## 🛠️ Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main microcontroller and Wi-Fi controller |
| WS2812B RGB LED Strip | Individually addressable RGB lighting |
| 5 V Power Supply | Supplies power to the LED strip |
| Jumper Wires | Electrical connections |
| Breadboard / Prototyping Setup | Circuit prototyping, where suitable |

**LED strip configuration:** 41 addressable LEDs.

## 💻 Software and Technologies

- **Programming language:** C++ (Arduino framework)
- **Microcontroller platform:** ESP32
- **Development environment:** Arduino IDE
- **LED control library:** FastLED
- **Networking:** ESP32 Wi-Fi and WebServer libraries
- **Communication protocol:** HTTP
- **User interface:** HTML, CSS, and JavaScript

## 🏗️ System Architecture

```text
 Smartphone / Computer
          |
          | Wi-Fi
          v
   ESP32 Access Point
          |
          v
    HTTP Web Server
          |
          v
    FastLED Library
          |
          v
   WS2812B LED Strip
       (41 LEDs)
```

The ESP32 hosts a Wi-Fi network and serves a web dashboard. When a user changes the color, brightness, or lighting effect, the browser sends an HTTP request to the ESP32. The firmware processes the request and updates the LED strip.

## 🔌 Getting Started

### 1. Install the Required Software

- Install the [Arduino IDE](https://www.arduino.cc/en/software).
- Install ESP32 board support using the Arduino Boards Manager.
- Install the **FastLED** library through the Arduino Library Manager.

### 2. Clone the Repository

```bash
git clone https://github.com/Rupam4604/-ESP32_LED_Controller.git
cd -ESP32_LED_Controller
```

### 3. Open the Arduino Sketch

Open the following file in Arduino IDE:

```text
led_contrller_esp32_v1.0/
└── led_contrller_esp32_v1.0.ino
```

Select **ESP32 Dev Module** or the appropriate board for your hardware, then select the correct serial port.

### 4. Configure and Upload

Check the LED data pin, LED count, Wi-Fi access-point credentials, and other settings in the sketch. Connect the ESP32 to your computer and upload the firmware.

### 5. Connect to the Controller

1. Power on the ESP32 and LED strip.
2. Connect your phone or computer to the Wi-Fi access point configured in the firmware.
3. Open a browser and navigate to:

   `http://192.168.4.1`

4. Select a color, adjust brightness, and experiment with the lighting effects.

The device's access-point name and password are defined in the firmware.

## 🔋 Power and Wiring Notes

- Use a suitable regulated 5 V supply for the WS2812B strip.
- Connect the ESP32 ground and LED power-supply ground together.
- Connect the LED data input to the configured ESP32 GPIO.
- Ensure the LED strip's data direction follows its arrows from DIN toward the next LED.
- For a more robust installation, use an appropriate 3.3 V-to-5 V logic-level shifter and a series resistor on the data line.
- For larger strips, use appropriate power wiring and power injection as needed.

**Safety:** Never connect 5 V directly to an ESP32 GPIO. Take care when working with mains-powered power supplies.

## 📁 Repository Structure

```text
-ESP32_LED_Controller/
│
├── README.md
│
└── led_contrller_esp32_v1.0/
    ├── README.md
    └── led_contrller_esp32_v1.0.ino
```

The root README provides an overview of the complete repository, while the Arduino sketch folder contains the firmware and its project-specific documentation.

## 🧠 Key Learning Outcomes

Through this project, I am developing practical skills in:

- ESP32 microcontroller programming
- Embedded C++ and firmware development
- Wi-Fi networking and HTTP communication
- Web server implementation on microcontrollers
- WS2812B addressable LED control
- Integrating hardware with a browser-based interface
- Debugging and testing embedded systems

## 🚧 Future Improvements

Planned enhancements for future versions include:

- Additional customizable lighting effects
- Animation speed control and smoother transitions
- Saving settings across power cycles
- Improved firmware structure and error handling
- Integration with other IoT platforms
- PCB design and a dedicated enclosure

These features are planned improvements and are not necessarily part of V1.

## 👨‍💻 Author

**Rupam Ghosh**

B.Tech in Electronics & Communication Engineering | Aspiring Embedded Systems & Firmware Engineer

- GitHub: [@Rupam4604](https://github.com/Rupam4604)
- LinkedIn: [Connect with me](https://www.linkedin.com/in/rupam-ghosh-0406047119326s)

I am passionate about embedded systems, electronics, IoT, firmware development, and robotics. I build hands-on projects to strengthen my engineering skills and turn ideas into working hardware.



⭐ **If you find this project interesting, consider starring the repository!**

*Built with curiosity, electronics, and a passion for embedded systems.*
