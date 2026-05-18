# POWADCR

<p align="center">
  <img src="doc/images/powadcr_banner.png" width="900">
</p>

<p align="center">
  <strong>POWADCR</strong><br>
  TAP/TZX/TSX/CDT Digital Cassette Recorder for 8-bit Computers
</p>

<p align="center">
  WAV / MP3 Player & Recorder based on ESP32 Audio Kit
</p>

---

## Overview

POWADCR is a digital cassette recorder emulator designed for retro 8-bit computers including:

- ZX Spectrum
- Amstrad CPC
- MSX
- ORIC
- Enterprise
- Apple IIe
- TRS-80
- and others

The project is based on the ESP32 Audio Kit development board and provides:

- TAP/TZX/TSX/CDT playback
- WAV recording and playback
- MP3 playback
- Touchscreen interface
- SD card support
- Portable battery-powered operation

The project combines retro-computing compatibility with modern embedded hardware.

---

## Features

- TAP/TZX/TSX/CDT playback
- TAP/WAV recording
- Touchscreen graphical interface
- ES8388 audio codec
- WiFi and Bluetooth capable hardware
- SD card file browser
- Optional battery support
- Compatible with classic and modern 8-bit systems

---

## Hardware

### Main Board

ESP32 Audio Kit by AI-Thinker Technology.

<p align="center">
  <img src="doc/images/esp32_audio_kit.jpg" width="650">
</p>

### Specifications

| Feature | Details |
|---|---|
| CPU | Dual-core ESP32 @ 240MHz |
| RAM | 512KB + 4MB PSRAM |
| Audio Codec | ES8388 |
| Connectivity | WiFi / Bluetooth |
| Storage | MicroSD |
| Inputs | Buttons / GPIO |
| Audio | IN / OUT |

Official documentation:

https://docs.ai-thinker.com/en/esp32-audio-kit

---

## LCD Display

<p align="center">
  <img src="doc/images/display.jpg" width="500">
</p>

| Feature | Value |
|---|---|
| Brand | TJC |
| Model | TJC4832T035_011 |
| Size | 3.5 inch |
| Resolution | 480x320 |
| Interface | UART |

Note: T0 displays are used in this project, although T1 versions can be adapted easily.

---

## Bill of Materials

| Component | Notes |
|---|---|
| ESP32 Audio Kit | Main board |
| 3.5 inch TJC TFT Display | Resistive or capacitive |
| XH2.5 cables | LCD connection |
| 3.7V Battery | Optional |
| MicroSD Card | FAT32 formatted |
| FT232RL Adapter | Optional LCD flashing |
| Stereo Jack Cables | Computer connections |

---

## Hardware Connections

### LCD Connection

Connect the LCD UART pins directly to the Audio Kit expansion connector.

<p align="center">
  <img src="doc/images/lcd_connection.jpg" width="700">
</p>

### LED Indicator

| Signal | GPIO |
|---|---|
| Power/REC LED | GPIO22 |

### REM Connection

| Signal | GPIO |
|---|---|
| Remote Tape Control | GPIO19 |

---

## DIP Switch Configuration

| Switch | Position |
|---|---|
| 1 | OFF |
| 2 | ON |
| 3 | ON |
| 4 | OFF |
| 5 | OFF |

---

## Firmware Installation

### 1. Flash TFT Firmware

Download:

https://github.com/hash6iron/powadcr/releases/download/Release/powadcr_iface.tft

Copy the `.tft` file to an empty SD card and insert it into the display.

Important: Verify power polarity before powering the display.

---

### 2. Flash ESP32 Firmware

Download:

https://github.com/hash6iron/powadcr/releases/latest

Recommended flashing tool:

- ESP32 Flash Download Tool

<p align="center">
  <img src="doc/images/flashing.jpg" width="700">
</p>

---

## Development Environment

### Requirements

- VSCode
- PlatformIO
- ESP32 toolchain

### Installation

```bash
git clone https://github.com/hash6iron/powadcr.git
```

Open the project with VSCode + PlatformIO and build normally.

---

## Supported Machines

| Computer | Playback | Recording |
|---|---|---|
| ZX Spectrum | TAP/TZX/WAV | TAP/WAV |
| CPC Amstrad | CDT/WAV | WAV |
| MSX | TSX/WAV | WAV |
| ORIC | TZX/WAV | WAV |
| Enterprise | TZX/WAV | WAV |
| Apple IIe | TZX/WAV | WAV |
| TRS-80 COCO | TZX/WAV | WAV |
| TI99 | TZX/WAV | WAV |

---

## Usage Notes

### Classic ZX Spectrum

Classic Spectrum machines require attenuation because POWADCR outputs 0–3.3V levels.

Recommended:
- resistor attenuation network
- adjustable amplifier

---

### Modern Devices

Devices such as:
- ESPectrum
- N-Go
- Spectrum Next

can usually connect directly.

---

## Gallery

<p align="center">
  <img src="doc/images/powadcr_front.jpg" width="420">
</p>

<p align="center">
  <img src="doc/images/powadcr_inside.jpg" width="420">
</p>

<p align="center">
  <img src="doc/images/powadcr_running.jpg" width="700">
</p>

---

## 3D Printed Case

A printable enclosure is available here:

https://www.thingiverse.com/thing:7037367

<p align="center">
  <img src="doc/images/case_render.jpg" width="700">
</p>

---

## Contributing

Pull requests, ideas and hardware improvements are welcome.

If you enjoy this project, consider:
- starring the repository
- sharing builds
- reporting issues
- contributing code

---

## License

GPL-3.0 License.

---

## Acknowledgements

- Phil Schatzmann AudioKit libraries
- AI-Thinker
- Retrocomputing community

---

## Links

- https://github.com/hash6iron/powadcr
- https://lacavernainformatica.com/powadcr-el-reproductor-de-cassettes-digital-para-ordenadores-retro-de-8-bits/
