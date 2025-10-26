# POWADCR
TAP/TZX/TSX/CDT Digital cassette recorder for 8-bit machines, and WAV/MP3 player recorder.
<p align="center">
  <img width="400" height="400" src="/doc/powadcr.png" />
</p>

-----

![20241120_172544](https://github.com/user-attachments/assets/f7f18624-5184-490f-a2b1-47833b4a70f2)

This project pretend to implement a Digital Cassette Recorder (for TAP/TZX files playing and recording on TAP) for ZX Spectrum machines based on ESP32 Audio kit development board and using HMI over touch 3.5" screen.

![image](https://github.com/user-attachments/assets/6d7ac494-c201-4113-875b-0324e44a8308)



The board
-----
![20250530_020348](https://github.com/user-attachments/assets/07e084b3-bae2-4221-b484-52a52ccb1733)

![20241114_094902](https://github.com/user-attachments/assets/112a1133-2ad4-4a44-b31e-9115065462c2)
The launcher was this board, ESP32 Audio Kit equipped with ESP32 v3 microcontroller and ES8388 Audio proccesor 
made by AI-Thinker Technology.

https://docs.ai-thinker.com/en/esp32-audio-kit

The summary of specifications is.
+ CPU 32 bits at 240MHz
+ 512KB + 4MB SRAM (PSRAM available)
+ 2 CORES
+ ES8388 dedicated audio proccesor
+ Audio IN/OUT
+ Bluetooth
+ WiFi
+ 8 switch buttons
+ I/O connectors
+ SD slot
+ ...

So, it's a beautiful develop board with a big possibilities. 

To begin with is necessary use the Phil Schatzmann's libraries for ESP32 Audio Kit v.0.65 (https://github.com/pschatzmann/arduino-audiokit) where we could take advantage of all resources of this kit, to create a digital player and recorder for ZX Spectrum easilly, or this is the first idea.

## LCD Screen Display

The LCD touch screen display chosen for this project is a TFT HMI LCD Display Module Screen Touch connected with 2 serial pins (TX and RX) to the board. 
+ Brand: TJC
+ Model: TJC4832T035_011
+ Size: 3.5".
+ Resolution:  480x320.

NOTE: Several version of this LCD are availables. The project uses T0 versions but is possible to upgrade easily to T1 version.

About POWADCR Device.
-----
In this section we are going to describe parts to be needed to assemble the PowaDCR device.

**Bill of material**
+ Main board: ESP32 Audiokit by AI-Thinker technology : https://docs.ai-thinker.com/en/esp32-audio-kit (Possible buy site. Alliexpress)
+ Color LCD 3.5" 480x320 pixels. Resistive TouchScreen - TJC4832T035_011 resistive (low priced but possible to discontinued and replaced by TJC4832T135 _ 011C capacitive or TJC4832T135 _ 011R resistive)
+ Cable XH2.5 to dupont to connecto LCD to the extended port of ESP32 Audiokit
+ Battery 2000mAh 3.7v (optional not needed)
+ Micro SD card FAT32 formatted (to contain all ZX Spectrum games in TAP and other formats to be red for PowaDCR in the future)
+ Micro SD card or FT232RL FTDI serial interface to program the TJC LCD (both methods are available)
+ Cable with jacks Stereo-stereo male-male 3.5mm to connect PowaDCR to Spectrum Next or N-Go or clone versions.
+ Cable with XH2.5 and mono jack 3.5mm to connect from amplifier out of PowaDCR to EAR connector on ZX Spectrum classic versions (Rubber keyboard 16K, 48K, Spectrum+ and Spectrum 128K Toastrack)

**Software and drivers (Windows version)**
+ LCD HMI editor:
  UART HMI Chinesse editor. Check the last version [here](http://wiki.tjc1688.com/download/old_usart_hmi/history_download.html)
+ CP2102 driver: [https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads](https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip)

Ports of powadcr
-----
![20250530_014338](https://github.com/user-attachments/assets/3b99fdb7-2cc3-438a-9fb3-e441f770584a)

Before assembly powadcr. Audiokit hacking.
-----
<b>Disassemble mics from audiokit ESP32 board</b>
You need to remove both microphones from the Audiokit board. How do I do this?

- An easier way: with pliers and pull up (recomended)
- Other way, unsoldering both, and conect pinout towards ES8388 to ground (See image below)

  <p align="center">
  <img src="/doc/mics.png" />
  </p>

How PowaDCR parts are connected?
-----
<b>LCD Screen connection</b>
- Is very easy. See image below to connect LCD 4-pin connector to Audiokit
  <p align="center">
  <img src="/doc/GPIO_audiokit.png" />
  </p>

  <p align="center">
  <img src="/doc/GPIO.png" />
  </p>

<b>LED indicator</b>
- The led for power on and recording indications is connected to GPIO pin 22 and GND
  <p align="center">
  <img width="326" height="103" alt="led" src="https://github.com/user-attachments/assets/88c771d5-d1f8-4805-8d02-5bae96531cff" />
  </p>

<b>REM connection</b>
- The REM input for remote tape control is assigned to GPIO pin 19 and GND. See example below for MSX computer.
  <p align="center">
    <img width="540" height="326" alt="REM" src="https://github.com/user-attachments/assets/37a33fe6-69c5-4f08-b839-53896d9daa75" />
  </p>
  

Audiokit DIP switch configuration
-----
This project need set the PCB DIP switch to

|Switch|Value|
|---|---|
|1|Off|
|2|On|
|3|On|
|4|Off|
|5|Off|

How to install firmwares in powadcr? 
-----
<b>At the first time</b>

- First you need to flash the screen firmware with this file : https://github.com/hash6iron/powadcr/releases/download/Release/powadcr_iface.tft 
- The second step is to flash the following file in AudioKit board : https://github.com/hash6iron/powadcr/releases/download/Release/Powadcr_v1.0r2.bin
- Then install later, the Screen and AudioKit firmwares from the latest relase :

  https://github.com/hash6iron/powadcr/releases/latest




<b>How to?</b>

To flash the firmware of the screen the file powadcr shown below must be copied inside an empty SD Card, insert it inside the SD Reader of the screen and connect the screen to the GPIO connector of the Audiokit board as shown below. Take care with polarity of the power, because if the polarity is inverted the screen could damage it. When the power is connected the screen will start with blank background and the file copy will be in process. This only takes less than 2 minutes. Be sure that the power couldn't be disconected during this process.


After this you can flash the binaries directly to the AudioKit board if you do not want to mess with code and compilers. 

1. Download ESP32 Flash Downloading Tool - from [here](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)
2. Unzip file and execute - flash_download_tool_x.x.x.exe file

   See the example image below.

   ![image](https://raw.githubusercontent.com/hash6iron/powadcr/refs/heads/main/doc/flash_download_tool.png)

   
4. Select ESP32 model.
   - ESP32
   - Develop
     And press "OK" button
   
5. Setting and begin the flash proccess.
   - Select <b>complete_firmware.bin</b> file or type the path of it.
   - Select all parameters exactly at the image below.
   - Connect ESP32-A1S Audiokit board from UART microUSB port (not power microUSB PORT) at PC USB port.
   - Select the available COM for this connection in COM: field on ESP32 Flash Downloading Tool.
   - Select BAUD: speed at 921600
   - Press START button in ESP32 Flash Downloading Tool. Then downloading proccess begin, and wait for FINISH message. Enjoy!
  
     NOTES: If the proccess fail.
      - Try to download again.
      - Try to ERASE before START proccess.
   
   Show image below.
   
   ![image](https://github.com/user-attachments/assets/b5c189c6-8945-4a65-9e22-e17a56d3eea6)

<b>Upcoming updates</b>
- Put the <b>firmware.bin</b> and <b>powadcr_iface.tft</b> of the latest release inside the root folder in the AudioKit SD and run again. Wait until process finishs.

How custom firmware is uploaded in ESP32-A1S Audiokit? 
-----
1. Install VSCode
   - https://code.visualstudio.com/download
3. Install PlatformIO for VSCode
   - https://platformio.org/install
5. Open powaDCR project into VSCode
6. Connect the ESP32 Audiokit USART USB port to any USB PC port
7. Press BUILD (arrow icon) from PlatformIO toolbar.


Using powadcr recorder with modern and classic 8-bit machines
-----
**Classic machines. ZX Spectrum.**

For this machine series, ZX Spectrum 16K, 48K, +, +2, +3 take into account that powadcr recorder uses a range between 0 to 3.3v then is needed to atenuate the audio output from ZX Spectrum classic version.
Then:
- You can use a tipical R circuit or audio amplifier (that permit adjust volumen signal from zero)
- The goal is test several volumen settings in the input signal until powadcr begins to recognize wave, but from zero.


**Modern machines. ESPectrum (lilygo) , N-Go, etc.**

Is possible to connect directly to the machine audio output line-out, anyway, if you can check before the output power in order to know wave characteristics, you ensure that input signal is ok for powadcr.
- ESPectrum can connecto directly without adaptation circuit.
- N-Go needs a special cable to get the output channel (+3 output) to stereo jack for powadcr (you can repeat the output channel in both left/right channels of the stereo jack or only get left channel and right to ground)

**Supported machines by powadcr**

|   Microcomputer  |  Play format  | Record format |
| ---------------- | ------------- | ------------- |
| Spectrum         | TAP, TZX, WAV | TAP, WAV      |
| CPC Amstrad      | CDT(TZX), WAV | WAV           |
| MSX              | TSX, WAV      | WAV           |
| ORIC             | TZX (DR), WAV | WAV           |
| Enterprise       | TZX (DR), WAV | WAV           |
| Apple IIe        | TZX (DR), WAV | WAV           |
| Mattel Aquarious | TZX (DR), WAV | WAV           |
| Lynx             | TZX (DR), WAV | WAV           |
| TRS-80 (COCO)    | TZX (DR), WAV | WAV           |
| THOMPSON MO5     | TZX (DR), WAV | WAV           |
| TI99             | TZX (DR), WAV | WAV           |
| JUPITER          | TZX (DR), WAV | WAV           |



Are you enjoying?
-----

<img src="https://github.com/user-attachments/assets/f08a42ab-0c6a-4262-b6ec-63c41263b76b" width="480" height="914">

If you enjoy with this project and you want to colaborate, please.

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BAWGJFZGXE5GE&source=url"><img src="/doc/paypal_boton.png" /></a>

<a href="https://www.buymeacoffee.com/atamairon"><img src="/doc/coffe.jpg" /></a>

thxs.
