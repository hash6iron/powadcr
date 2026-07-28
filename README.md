# POWADCR

**Professional Digital Cassette Recorder for 8-bit Machines**

Multi-format TAP/TZX/TSX/CDT/CSW digital cassette recorder with WAV/MP3 playback and recording capabilities.

<p align="center">
  <img width="400" height="400" src="/doc/powadcr.png" />
</p>

---

## Overview

POWADCR is a professional-grade digital cassette recorder designed for retrocomputing enthusiasts and developers. Built on the ESP32 Audio Kit platform with a dedicated 3.5-inch capacitive touch interface, it provides comprehensive support for playing and recording tape files across multiple 8-bit computer architectures including ZX Spectrum, Amstrad, MSX, Commodore, FPGA systems, and various emulators.

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/c3fa02bd-d66c-483b-bbb5-5842725ea170" />
</p>

<p align="center">
  <img width="100%" alt="powadcr2" src="https://github.com/user-attachments/assets/6c93b7ac-8313-4c6a-a557-cbd183bb3aa7" />
</p>

---

## Evolution & Applications

The POWADCR platform has been adapted to numerous hardware implementations and use cases:

**Standard Implementations**

<img width="100%" src="https://github.com/user-attachments/assets/f7f18624-5184-490f-a2b1-47833b4a70f2" />

<img width="100%" src="https://github.com/user-attachments/assets/6d7ac494-c201-4113-875b-0324e44a8308" />

**Custom Builds and Integrations**

<img width="100%" src="https://jose.gal/assets/+2A/ESPectrum+2A-V3_14.jpeg" />

*ESPectrum +2A integration by Jose Gal - [Visit Portfolio](https://jose.gal/)*

<img width="100%" src="https://jose.gal/assets/computone_v3/Computone_10.jpeg" />

*Computone vintage system integration by Jose Gal - [Visit Portfolio](https://jose.gal/)*

<p align="center">
  <table width="100%">
    <tr>
      <td width="50%"><img src="https://github.com/user-attachments/assets/60777bfe-e488-4064-a1ba-c4b194ef395d" /></td>
      <td width="50%"><img src="https://github.com/user-attachments/assets/8c7e0c85-3d15-45fe-8598-fab8f638951d" /></td>
    </tr>
  </table>
</p>

*Walkman-style POWADCR device by Jose Gal - [View on Mastodon](https://masto.es/@Jose/116532569484667256)*

<img width="100%" src="https://makerworld.bblmw.com/makerworld/model/DSM00000003042829/design/c01194b65d0c2bec.jpg?x-oss-process=image%2Fformat%2Cwebp" />

*Old school radio-cassette version. Beautiful 3D job of Patum - [Visit makeworld](https://makerworld.com/es/models/3042829-powadcr-2026)*

---

## Hardware Architecture

### Primary Controller: ESP32 Audio Kit

The POWADCR system is built upon the professional-grade **ESP32 Audio Kit** by AI-Thinker Technology—a feature-rich development platform optimized for audio applications.

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/07e084b3-bae2-4221-b484-52a52ccb1733" />
</p>

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/112a1133-2ad4-4a44-b31e-9115065462c2" />
</p>

**Technical Specifications**

| Component | Specification |
|-----------|---------------|
| **Processor** | ESP32 v3 (32-bit ARM) |
| **Clock Frequency** | 240 MHz |
| **CPU Cores** | 2 |
| **SRAM** | 512 KB + 4 MB |
| **Extended Memory** | PSRAM (configurable) |
| **Audio Processor** | ES8388 (dedicated) |
| **Audio Interfaces** | Analog IN/OUT |
| **Wireless** | Bluetooth LE, IEEE 802.11b/g/n |
| **Control Inputs** | 8 programmable buttons |
| **Storage** | Integrated SD card slot |
| **Connectivity** | UART, SPI, I2C interfaces |

**Key Features:**
- Purpose-designed for professional audio applications
- Integrated audio codec with dual analog inputs and outputs
- Sufficient memory for comprehensive buffering and processing
- Native support for multiple connectivity protocols

**Documentation:** [AI-Thinker ESP32-Audio-Kit Reference](https://docs.ai-thinker.com/en/esp32-audio-kit)

**Software Framework:** [Phil Schatzmann's Arduino Audio Kit Library v0.65](https://github.com/pschatzmann/arduino-audiokit)

---

### Display: Touch-Screen Interface

**3.5-inch Resistive TFT Display with Serial Interface**

| Parameter | Value |
|-----------|-------|
| **Manufacturer** | TJC |
| **Model** | TJC4832T035_011 |
| **Screen Diagonal** | 3.5 inches |
| **Resolution** | 480 × 320 pixels (HVGA) |
| **Color Depth** | 16-bit |
| **Touch Interface** | Resistive touchscreen |
| **Communication** | Serial UART (2-wire) |
| **Voltage** | 3.3V (factory-configurable to 5V) |

**Alternative Models:** The platform supports variant models including:
- TJC4832T135_011C (capacitive touchscreen)
- TJC4832T135_011R (resistive touchscreen)

**Critical Setup Requirement:**

Prior to initial operation, the display voltage regulator **must** be configured for 3.3V operation:

1. Locate jumper **JP2** on the display PCB
2. Install a jumper bridge or solder bridge wire across JP2
3. Verify 3.3V output before connecting to the ESP32 Audio Kit

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/b07eb60f-c534-4497-a2ea-625520ad8a43" />
</p>

---

## Assembly and Integration

### System Architecture

<p align="center">
  <img width="100%" alt="System Architecture Diagram" src="https://github.com/user-attachments/assets/4c0b7d64-9389-45ff-875b-fcaddc2149b2" />
</p>

### Complete Bill of Materials

**Core Components:**

| Component | Specification | Remarks |
|-----------|---------------|---------|
| **ESP32 Audio Kit** | AI-Thinker mainboard | Primary controller |
| **TFT Display** | TJC4832T035_011 (480×320) | Serial interface variant required |
| **Connector Cable** | XH2.5 to Dupont adapter | LCD to Audiokit extension port |
| **Audio Cable** | Stereo 3.5mm male-to-male | For modern systems (Spectrum Next, N-Go) |
| **Adapter Cable** | XH2.5 + mono 3.5mm jack | For classic Spectrum (16K, 48K, +, 128K) |

**Storage and Programming:**

| Item | Purpose |
|------|---------|
| **Micro SD Card** | Game/program storage (FAT32 formatted) |
| **Programming Interface** | Either FT232RL FTDI module OR secondary SD card |

**Software Components:**

| Software | Vendor | Link |
|----------|--------|------|
| **LCD HMI Editor** | TJC Chinesse Editor | [Official Repository](http://wiki.tjc1688.com/download/old_usart_hmi/history_download.html) |
| **CP2102 USB Driver** | Silicon Labs | [Download](https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip) |

**Optional Accessories:**

- Lithium Battery: 2000 mAh, 3.7V (for portable operation)
- Amplifier Module: For signal conditioning from classic computers

**Sourcing:**
- Primary components available from [AliExpress](https://www.alliexpress.com/)
- Compatible alternative displays listed above may be substituted

---

### Hardware Preparation

#### Remove Audio Microphones from ESP32 Audio Kit

The ESP32 Audio Kit includes integrated microphones that must be physically removed to prevent audio feedback and interference.

**Removal Methods:**

**Method 1: Mechanical Extraction (Recommended)**
- Use needle-nose pliers to carefully grip the microphone body
- Apply steady upward pressure until microphone detaches from PCB
- No desoldering required; minimal risk of trace damage

**Method 2: Desoldering**
- Apply heat-gun or soldering iron to microphone leads
- Carefully lift microphone from PCB
- Connect microphone pads to PCB ground plane (GND)

<p align="center">
  <img width="100%" src="/doc/mics.png" />
</p>

---

### Connector Specifications

**POWADCR Port Layout**

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/3b99fdb7-2cc3-438a-9fb3-e441f770584a" />
</p>

---

## Electrical Integration

### Display Connection

The 4-pin LCD connector integrates with the ESP32 Audio Kit extended GPIO header:

<p align="center">
  <img width="100%" src="/doc/GPIO_audiokit.png" />
</p>

<p align="center">
  <img width="100%" src="/doc/GPIO.png" />
</p>

### LED Status Indicator

A single indicator LED provides power and recording status feedback:

- **GPIO Assignment:** Pin 22
- **Ground Reference:** System GND
- **Status Indication:** Power (steady), Recording (blinking)

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/88c771d5-d1f8-4805-8d02-5bae96531cff" />
</p>

### REM (Remote Tape Control) Connection

The REM input allows external tape control integration—particularly useful for MSX computers and other systems with remote control support:

- **GPIO Assignment:** Pin 19
- **Ground Reference:** System GND

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/37a33fe6-69c5-4f08-b839-53896d9daa75" />
</p>

### MCP23017 I/O Expansion (Optional)

For users desiring authentic vintage cassette player emulation with external control panels:

<p align="center">
  <img width="100%" alt="GPIO Expansion" src="https://github.com/user-attachments/assets/5e8066bb-a76a-4566-91b3-23a98fcd9cde" />
</p>

**Pin Configuration Modification Required:**

When using external keypad expansion, HMI (display) connections must be reassigned:

| Signal | Original Pin | Modified Pin |
|--------|--------------|--------------|
| TX | GPIO default | IO5 |
| RX | GPIO default | IO22 |
| REM | GPIO 19 | GPIO 19 (unchanged) |

> Note: This modification enables external keypad operation while maintaining REM functionality.

---

### DIP Switch Configuration

Set the Audiokit PCB DIP switches according to the following table:

| Switch # | Position | State |
|----------|----------|-------|
| 1 | OFF |  |
| 2 | ON | ✓ |
| 3 | ON | ✓ |
| 4 | OFF |  |
| 5 | OFF |  |

---

## Firmware Installation

### Initial Setup Procedure

The initial firmware deployment consists of two independent flash operations:

#### Phase 1: Display Firmware Installation

**Required Files:**
- `powadcr_iface.tft` - [Download from Latest Release](https://github.com/hash6iron/powadcr/releases)

**Procedure:**

1. Format an empty microSD card (FAT32 filesystem)
2. Copy `powadcr_iface.tft` to the SD card root directory
3. Insert SD card into the display's integrated SD card reader
4. Connect the display to the Audiokit ESP32 board via the 4-pin XH2.5 connector
5. Verify correct polarity: Red wire = +3.3V, Black wire = GND
6. Apply power to the system
7. The display will show a blank screen and begin file transfer (LED activity may be visible)
8. **Do not interrupt power during this process** (typically 1-2 minutes)
9. Transfer complete when display shows normal boot sequence

#### Phase 2: AudioKit Firmware Installation

**Option A: Binary Flash Tool (Recommended for End Users)**

**Requirements:**
- [ESP32 Flash Download Tool](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)
- `complete_firmware.bin` from [Latest Release](https://github.com/hash6iron/powadcr/releases/latest)
- USB cable for UART connection

**Steps:**

1. Download and extract ESP32 Flash Download Tool
2. Execute `flash_download_tool_x.x.x.exe`
3. Select target device:
   - Board: **ESP32**
   - Mode: **Develop**
   - Click **OK**

<p align="center">
  <img width="100%" src="https://raw.githubusercontent.com/hash6iron/powadcr/refs/heads/main/doc/flash_download_tool.png" />
</p>

4. Configure flash parameters:
   - Firmware File: Select `complete_firmware.bin`
   - Flash Address: (see tool preset values)
   - Baud Rate: **921600** (critical)

5. Connect ESP32 Audio Kit:
   - Locate UART microUSB port (marked separately from power port)
   - Connect to PC USB port
   - Select corresponding COM port in tool

6. Initiate flash operation:
   - Click **START** button in flash tool
   - Monitor progress bar until **FINISH** appears
   - System will auto-reboot upon completion

<p align="center">
  <img width="100%" src="https://github.com/user-attachments/assets/b5c189c6-8945-4a65-9e22-e17a56d3eea6" />
</p>

**Troubleshooting:**
- If flash operation fails: Retry the process
- For persistent failures: Click **ERASE** before **START**

---

**Option B: Development Build (For Developers)**

**Prerequisites:**
- Microsoft Visual Studio Code
- PlatformIO extension for VSCode

**Installation Steps:**

1. Install [Visual Studio Code](https://code.visualstudio.com/download)
2. Install [PlatformIO IDE](https://platformio.org/install)
3. Open POWADCR project directory in VSCode
4. Connect ESP32 Audio Kit via UART microUSB port
5. In PlatformIO toolbar, click **BUILD** (arrow icon)
6. After successful build, click **UPLOAD** to flash device

---

### Firmware Updates

**To update to the latest released firmware:**

1. Download latest `firmware.bin` and `powadcr_iface.tft` from [Releases](https://github.com/hash6iron/powadcr/releases/latest)
2. Place both files in the SD card root directory
3. Power cycle the system
4. Wait for update process completion (typically 1-3 minutes)
5. System will auto-reboot when complete

---

## Operating Principles

### Audio Source Integration

#### Classic Computers (ZX Spectrum 16K/48K/+/+2/+3)

**Signal Level Mismatch:**
Classic Spectrum machines output 5V audio signals, while POWADCR accepts 0-3.3V maximum input. Direct connection will cause signal clipping and reduced fidelity.

**Solution: Signal Conditioning Required**

Two approaches are available:

**Approach 1: Resistive Voltage Divider**
- Construct simple RC network using two resistors
- Recommended values: 10kΩ + 15kΩ configuration
- Achieves 3:5 attenuation ratio

**Approach 2: Audio Amplifier with Volume Control**
- Connect Spectrum audio output to line input of powered amplifier
- Adjust amplifier volume control until POWADCR recognizes signal
- Start from minimum volume and increase gradually

**Optimal Configuration:**
- Begin testing from zero input level
- Gradually increase until POWADCR recognizes valid waveform
- Adjust for minimum distortion and maximum clarity
- Note settings for future sessions

---

#### Modern Systems (ESPectrum, N-Go, Retro Clones)

These platforms implement modern audio output standards compatible with POWADCR:

**ESPectrum (Lilygo):**
- Direct connection supported without attenuation
- Line-level output compatible with POWADCR input

**N-Go and Similar Platforms:**
- Requires special audio adapter cable
- Configuration: Mono 3.5mm jack with channel routing
- Recommended: Mono output to both L/R channels OR left channel + ground

---

## Supported File Formats

### Digital Tape Formats

| Format | File Extension | Supported Versions | Machine Target | Notes |
|--------|---|---|---|---|
| **TAP (ZX)** | .tap | Standard | Spectrum | Playback & recording |
| **TZX** | .tzx | v1.20 | Multi-platform | Playback (native features) |
| **PZX** | .pzx | Latest | Spectrum | Playback (advanced format) |
| **TSX** | .tsx | Standard | MSX | Playback |
| **CDT** | .cdt | v1.20 (TZX) | Amstrad | Playback |
| **TAP (C64)** | .tap | v1.0, v1.1 | Commodore C64 | Playback |
| **CSW** | .csw | v1.1, v2.0 | Multi-platform | Playback |

### Audio Formats

| Format | Bit Depth | Sample Rates | Bitrate | Notes |
|--------|---|---|---|---|
| **WAV** | 8-bit, 16-bit | 8 kHz - 96 kHz | PCM | Full playback & recording |
| **MP3** | Compressed | Variable | 64-320 kbps | Playback only |
| **FLAC** | 24-bit max | Up to 44 kHz | Lossless | Playback only |
| **ZIP** | Container | Multiple | N/A | Archive support |

---

## Compatible Computer Platforms

| Computer Platform | Playback Formats | Recording Formats | Notes |
|---|---|---|---|
| **ZX Spectrum** | TAP, TZX, CSW, WAV | TAP, WAV | Full support with 16K+ memory |
| **CPC Amstrad** | CDT (TZX), WAV | WAV | Via TZX compatibility |
| **MSX** | TSX, WAV | WAV | Standard TSX format |
| **ORIC** | TZX (DR), WAV | WAV | TZX DR variant |
| **Enterprise** | TZX (DR), WAV | WAV | TZX DR variant |
| **Apple IIe** | TZX (DR), WAV | WAV | TZX DR variant |
| **Mattel Aquarius** | TZX (DR), WAV | WAV | TZX DR variant |
| **Lynx** | TZX (DR), WAV | WAV | TZX DR variant |
| **TRS-80 (COCO)** | TZX (DR), WAV | WAV | TZX DR variant |
| **THOMPSON MO5** | TZX (DR), WAV | WAV | TZX DR variant |
| **TI-99** | TZX (DR), WAV | WAV | TZX DR variant |
| **JUPITER** | TZX (DR), WAV | WAV | TZX DR variant |
| **Commodore C64** | CSW, TAP, WAV | - | Read-only support |

---

## Project Support

If you find POWADCR valuable and wish to support continued development, contributions are welcome:

<p align="center">
  <img width="480" src="https://github.com/user-attachments/assets/f08a42ab-0c6a-4262-b6ec-63c41263b76b" />
</p>

**Support Options:**

<p align="center">
  <a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BAWGJFZGXE5GE&source=url">
    <img src="/doc/paypal_boton.png" alt="Donate via PayPal" />
  </a>
  &nbsp;&nbsp;&nbsp;&nbsp;
  <a href="https://www.buymeacoffee.com/atamairon">
    <img src="/doc/coffe.jpg" alt="Buy me a coffee" />
  </a>
</p>

---

<p align="center">
  <strong>POWADCR</strong><br>
  Professional Digital Cassette Recorder for Retrocomputing Enthusiasts
</p>
