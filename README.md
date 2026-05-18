# 🎙️ POWADCR

**Digital Cassette Recorder for 8-bit Machines**

TAP/TZX/TSX/CDT digital cassette recorder with WAV/MP3 playback and recording capabilities.

<p align="center">
  <img width="400" height="400" src="/doc/powadcr.png" />
</p>

---

## 📋 Table of Contents

- [Overview](#overview)
- [Hardware](#hardware)
- [Assembly](#assembly)
- [Installation](#installation)
- [Usage](#usage)
- [Support](#support)

---

## 🎯 Overview

PowaDCR is a digital cassette recorder designed for ZX Spectrum machines and other 8-bit retro computers. It leverages the ESP32 Audio Kit development board with a 3.5" touch screen interface to provide an intuitive platform for playing and recording tape files.

<p align="center">
  <img src="https://github.com/user-attachments/assets/f7f18624-5184-490f-a2b1-47833b4a70f2" width="600" />
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/6d7ac494-c201-4113-875b-0324e44a8308" width="600" />
</p>

---

## 🛠️ Hardware

### ESP32 Audio Kit

The project is built on the **ESP32 Audio Kit** by AI-Thinker Technology—a professional development board with impressive specifications:

| Component | Specification |
|-----------|---------------|
| **Processor** | ESP32 v3 (32-bit @ 240MHz, 2 cores) |
| **Memory** | 512KB + 4MB SRAM (PSRAM available) |
| **Audio** | ES8388 dedicated processor |
| **Connectivity** | Bluetooth, WiFi |
| **Interfaces** | Audio IN/OUT, SD card slot, 8 buttons |
| **Ports** | I/O connectors, UART |

📚 **Documentation**: [AI-Thinker ESP32-Audio-Kit Docs](https://docs.ai-thinker.com/en/esp32-audio-kit)

> This project utilizes **Phil Schatzmann's ESP32 Audio Kit library (v0.65)** to maximize the capabilities of this powerful board.
> 📦 [Repository](https://github.com/pschatzmann/arduino-audiokit)

### LCD Display

**TFT Touch Screen with Serial Interface**

| Property | Value |
|----------|-------|
| **Manufacturer** | TJC |
| **Model** | TJC4832T035_011 |
| **Size** | 3.5 inches |
| **Resolution** | 480×320 pixels |
| **Interface** | Serial (TX/RX, 2 pins) |
| **Variants** | T0 (project default), T1 (upgradeable) |

> **⚙️ Important**: Before using, configure the screen to **3.3V** by connecting jumper **JP2**
> 
> ![20260215_111140](https://github.com/user-attachments/assets/b07eb60f-c534-4497-a2ea-625520ad8a43)

---

## 🔧 Assembly

### Complete Bill of Materials

#### Core Components
- **Main Board**: ESP32 Audio Kit (AI-Thinker)
  - 📍 Available on [AliExpress](https://www.alliexpress.com)
- **Display**: 3.5" TFT Touch Screen (480×320)
  - TJC4832T035_011 (resistive) — standard option
  - TJC4832T135_011C (capacitive) or TJC4832T135_011R (resistive) — alternative
- **Connectors & Cables**:
  - XH2.5 to Dupont cable (LCD connection)
  - Stereo-to-stereo 3.5mm cable (machine connection)
  - XH2.5 + mono 3.5mm jack (classic Spectrum adapter)

#### Optional Components
- Battery: 2000mAh, 3.7V (optional)
- Programming Interface: FT232RL FTDI or microSD card

#### Storage
- Micro SD card (FAT32 formatted) for games and recordings

#### Software & Drivers
| Item | Link |
|------|------|
| **LCD HMI Editor** | [UART HMI Chinese Editor](http://wiki.tjc1688.com/download/old_usart_hmi/history_download.html) |
| **CP2102 Driver** | [Silicon Labs - CP210x](https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip) |

### Hardware Modifications

#### Remove Built-in Microphones

The ESP32 Audio Kit includes microphones that must be removed to prevent interference:

- **Recommended**: Use pliers to gently pull upward
- **Alternative**: Unsolder and connect pins to ground

<p align="center">
  <img src="/doc/mics.png" width="400" />
</p>

### Ports Overview

<p align="center">
  <img src="https://github.com/user-attachments/assets/3b99fdb7-2cc3-438a-9fb3-e441f770584a" width="600" />
</p>

---

## 🔌 Connections

### System Architecture

<p align="center">
  <img src="https://github.com/user-attachments/assets/4c0b7d64-9389-45ff-875b-fcaddc2149b2" width="900" />
</p>

### LCD Screen Connection

Connect the 4-pin LCD connector to the Audiokit extended port:

<p align="center">
  <img src="/doc/GPIO_audiokit.png" width="500" />
  <img src="/doc/GPIO.png" width="500" />
</p>

### LED Indicator

Power and recording status LED connected to **GPIO 22 + GND**:

<p align="center">
  <img src="https://github.com/user-attachments/assets/88c771d5-d1f8-4805-8d02-5bae96531cff" width="400" />
</p>

### REM (Remote Control)

Remote tape control input assigned to **GPIO 19 + GND**

Example MSX integration:

<p align="center">
  <img src="https://github.com/user-attachments/assets/37a33fe6-69c5-4f08-b839-53896d9daa75" width="600" />
</p>

### External Keypad (Optional)

Connect **MCP23017** I/O expander for vintage cassette control simulation:

<p align="center">
  <img src="https://github.com/user-attachments/assets/77e06446-dde5-4437-8531-cd3ba4e0e63d" width="900" />
</p>

> ⚠️ **Note**: When using external keypad, modify HMI connection:
> - **TX** GPIO → Pin IO5
> - **RX** GPIO → Pin IO22
> - **REM** → Remains on IO19

### DIP Switch Configuration

Set the Audiokit PCB DIP switches as follows:

| Switch | Position |
|--------|----------|
| 1 | ⊘ Off |
| 2 | ⊙ On |
| 3 | ⊙ On |
| 4 | ⊘ Off |
| 5 | ⊘ Off |

---

## 📦 Installation

### Initial Firmware Flash

#### Step 1: Flash LCD Screen Firmware

1. Download: [powadcr_iface.tft](https://github.com/hash6iron/powadcr/releases/download/Release/powadcr_iface.tft)
2. Copy to empty SD card
3. Insert SD card into screen reader
4. Connect screen to Audiokit GPIO connector (check polarity!)
5. Wait for completion (~2 minutes)

> ⚠️ **Critical**: Do not disconnect power during flashing!

#### Step 2: Flash AudioKit Firmware

**Option A: Binary Installation (Recommended for users)**

1. Download [ESP32 Flash Tool](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)
2. Unzip and run `flash_download_tool_x.x.x.exe`

   <p align="center">
     <img src="https://raw.githubusercontent.com/hash6iron/powadcr/refs/heads/main/doc/flash_download_tool.png" width="600" />
   </p>

3. Select **ESP32** → **Develop** → **OK**
4. Configure settings:
   - **File**: `complete_firmware.bin`
   - **Address**: (see tool configuration)
   - **Baud Rate**: 921600
5. Connect Audiokit via UART USB port (⚠️ NOT power port)
6. Select COM port and click **START**

   <p align="center">
     <img src="https://github.com/user-attachments/assets/b5c189c6-8945-4a65-9e22-e17a56d3eea6" width="600" />
   </p>

7. Wait for **FINISH** message

**Troubleshooting**:
- If flashing fails, try again
- Use **ERASE** before **START** if issues persist

**Option B: Development Installation (For developers)**

1. Install [VSCode](https://code.visualstudio.com/download)
2. Install [PlatformIO](https://platformio.org/install)
3. Open PowaDCR project in VSCode
4. Connect Audiokit UART USB port
5. Press **BUILD** (arrow icon) in PlatformIO toolbar

#### Step 3: Update to Latest Firmware

Place the following files in the Audiokit SD card root:
- `firmware.bin`
- `powadcr_iface.tft`

Power cycle and wait for update completion.

---

## 🎮 Usage

### Connecting to 8-bit Machines

#### Classic Machines (ZX Spectrum)

For Spectrum 16K, 48K, +, +2, +3:

> PowaDCR outputs 0–3.3V, requiring signal attenuation from classic machines (5V output)

**Solution**:
- Use voltage divider (resistor circuit) OR
- Connect through audio amplifier with volume control
- Test input levels starting from zero and gradually increase until PowaDCR recognizes wave

#### Modern Machines

**ESPectrum (Lilygo), N-Go, etc.**:
- Direct connection recommended
- Verify output signal level first
- ESPectrum: Direct connection supported
- N-Go: Requires special cable for stereo output (repeat or ground right channel)

### Supported Formats

| Computer | Play Formats | Record Formats |
|----------|--------------|----------------|
| **Spectrum** | TAP, TZX, WAV | TAP, WAV |
| **CPC Amstrad** | CDT (TZX), WAV | WAV |
| **MSX** | TSX, WAV | WAV |
| **ORIC** | TZX (DR), WAV | WAV |
| **Enterprise** | TZX (DR), WAV | WAV |
| **Apple IIe** | TZX (DR), WAV | WAV |
| **Mattel Aquarius** | TZX (DR), WAV | WAV |
| **Lynx** | TZX (DR), WAV | WAV |
| **TRS-80 (COCO)** | TZX (DR), WAV | WAV |
| **THOMPSON MO5** | TZX (DR), WAV | WAV |
| **TI-99** | TZX (DR), WAV | WAV |
| **JUPITER** | TZX (DR), WAV | WAV |

---

## ❤️ Support

### Enjoying PowaDCR?

If you find this project valuable and would like to support development:

<p align="center">
  <a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BAWGJFZGXE5GE&source=url">
    <img src="/doc/paypal_boton.png" alt="PayPal" />
  </a>
  &nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://www.buymeacoffee.com/atamairon">
    <img src="/doc/coffe.jpg" alt="Buy Me A Coffee" />
  </a>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/f08a42ab-0c6a-4262-b6ec-63c41263b76b" width="480" />
</p>

**Thanks for your support!** ☕

---

<p align="center">
  <strong>PowaDCR</strong> — Bringing retro computing to life
</p>
