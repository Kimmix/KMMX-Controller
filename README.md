<div align="left">

# ✨ KMMX-Fursuit Controller ✨
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![IDE: PlatformIO](https://img.shields.io/badge/IDE-PlatformIO-orange.svg)](https://platformio.org/)
[![Status](https://img.shields.io/badge/Status-In%20Development-orange.svg)](https://github.com/Kimmix/KMMX-Fursuit)

### Controller V2 Preview
<img src="doc\ControllerV2.png" alt="Kimmix Controller V2">

### Controller V4 Preview
<img src="doc\ControllerV4.3.png" alt="Kimmix Controller V4">

</div>

## 📢 Project Status

> **Note:** This is a personal project built for my own fursuit and is currently in active development. While the code is shared under the MIT License and you're welcome to use, modify, and adapt it for your own projects, please be aware that:
>
> - ⚠️ **No support is provided** at this time
> - 🔧 Features and APIs may change without notice
> - 🚧 Documentation may be incomplete or outdated
> - 🐛 Bugs and incomplete features are expected
>
> Feel free to fork, experiment, and build upon this work within the terms of the MIT License. I'm sharing this publicly in the spirit of the maker community, but I'm not able to provide assistance, answer questions, or accept contributions at this stage of development.

## 🌟 Features

- **Animated LED Matrix Displays** for eyes and mouth expressions
- **Real-time facial animations** including blinking, emoting, and mouth movements
- **Audio-reactive visemes** that respond to speech
- **Proximity sensing** for interactive "booping" responses
- **Accelerometer integration** for motion-based animations and responses
- **Bluetooth connectivity** for remote control and configuration
- **Customizable expressions** with easy bitmap conversion tools

## 🦊 Demo & Gallery

<div align="center">
  <h3>Sleeping Idle Animation</h3>
  <a href="https://x.com/kimmix00/status/1792751189395927293/video/1" target="_blank">
    <img src="https://img.shields.io/badge/View%20on%20X-Sleeping%20Animation-1DA1F2" alt="Sleeping Animation Demo">
  </a>

  <h3>Booping Interaction</h3>
  <a href="https://x.com/kimmix00/status/1687878110430339072/video/1" target="_blank">
    <img src="https://img.shields.io/badge/View%20on%20X-Booping%20Demo-1DA1F2" alt="Booping Demo">
  </a>

  <h3>Bluetooth Smartphone Control</h3>
  <a href="https://x.com/kimmix00/status/1704465522497397001/video/1" target="_blank">
    <img src="https://img.shields.io/badge/View%20on%20X-BLE%20Control%20Demo-1DA1F2" alt="BLE Control Demo">
  </a>

  <h3>Viseme</h3>
  <a href="https://x.com/kimmix00/status/1638887564550754306/video/1" target="_blank">
    <img src="https://img.shields.io/badge/View%20on%20X-BLE%20Control%20Demo-1DA1F2" alt="Viseme Demo">
  </a>

  <i>[ Future gallery of additional photos/videos showing the fursuit in action ]</i>
</div>

## 🛠️ Hardware

<details>
<summary>Click to expand hardware details</summary>

This project is designed to run on an ESP32-based custom board (specifically the ESP32-S3) and includes:

- **HUB75 LED Matrix panels** (64x32 pixel resolution)
- **APDS9930** proximity sensor for booping interaction
- **LIS3DH** accelerometer for motion detection
- **I2S Microphone** for audio input (viseme detection)
- **WS2812 RGB LEDs** for cheek panels and status indicators
- **PWM-controlled LEDs** for horn illumination

</details>

## 📋 Project Structure

<details>
<summary>Click to expand project structure</summary>

- **`src/`** - Main source code
  - **`Bitmaps/`** - Bitmap assets for eye and mouth animations
  - **`Devices/`** - Hardware driver implementations
  - **`FacialStates/`** - Facial animation state machines
  - **`KMMXController/`** - Main controller logic
  - **`Network/`** - Bluetooth connectivity
  - **`Renderer/`** - Animation and rendering code
  - **`Utils/`** - Helper functions
- **`bitmapTool/`** - Tools for converting images to bitmaps
- **`include/`** - Header files
- **`lib/`** - External libraries
- **`boards/`** - Custom board definitions

</details>

## 🚀 Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- ESP32 development board (preferably ESP32-S3)
- LED matrix panels and other hardware components as specified in the code

### Setup

<details>
<summary>Click to expand setup instructions</summary>

1. Clone the repository:

   ```
   git clone https://github.com/yourusername/KMMX-Fursuit.git
   ```

2. Open the project in PlatformIO or Arduino IDE

3. Install the required dependencies (listed in platformio.ini):
   - h2zero/NimBLE-Arduino
   - Adafruit GFX Library
   - Adafruit MPR121
   - arduinoFFT
   - Adafruit LIS3DH
   - ESP32 HUB75 LED MATRIX PANEL DMA Display
   - APDS-9930 Ambient Light and Proximity Sensor
   - Adafruit NeoPixel

4. Configure your hardware connections in `src/config.h`

5. Build and upload to your ESP32 board

</details>

## ⚙️ Configuration

The project is highly configurable. Main settings can be adjusted in `src/config.h`, including:

- Pin assignments for all hardware components
- LED matrix resolution and brightness settings
- Animation timing parameters
- Sensor sensitivity thresholds
- BLE connectivity settings

## 🎮 Usage

Once programmed, the controller will:

1. Initialize all hardware components
2. Start displaying default eye and mouth animations
3. Respond to "boops" via the proximity sensor
4. React to speech and sounds with viseme animations
5. Respond to physical movement via the accelerometer
6. Accept Bluetooth commands for changing expressions and settings

## ✨ Animation Creation

Create your own custom animations using the bitmap tool:

1. Design your animations in Adobe Photoshop or After Effects
2. Use the provided converter tool in `bitmapTool/` to convert to the proper format
3. Add the generated header files to your project
4. Configure the new animations in the controller

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

**TL;DR:** You're free to use, modify, and distribute this code for any purpose (commercial or personal), but it comes with no warranty. This project was built for my personal use, so while you're welcome to use it, please understand it's tailored to my specific hardware setup and requirements.

## 🤝 Support & Contributing

**Current Status:** Not accepting contributions or providing support.

As this is a personal project still in active development, I'm not currently:
- ❌ Accepting pull requests or contributions
- ❌ Providing technical support or troubleshooting help
- ❌ Answering questions about setup or usage
- ❌ Maintaining issues or feature requests

**However**, you're absolutely encouraged to:
- ✅ Fork this repository and make it your own
- ✅ Learn from the code and adapt it to your needs
- ✅ Share your own creations with the community
- ✅ Use this as a starting point for your own fursuit projects

## 📱 Contact

<div align="center">
  <p>Find me on Discord: kimmix</p>
  <p>Or my website: <a href="https://kimmix.anthro.asia/" target="_blank">kimmix.anthro.asia/</a></p>

</div>

---
