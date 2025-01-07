# POWADCR
TAP/TZX/TSX/CDT Digital cassette recorder for 8-bit machines
<p align="center">
  <img width="400" height="400" src="/doc/powadcr.png" />
</p>

-----

![20241120_172544](https://github.com/user-attachments/assets/f7f18624-5184-490f-a2b1-47833b4a70f2)

Initially, this project pretend to implement a Digital Cassette Recorder (for TAP/TZX files playing and recording on TAP) for ZX Spectrum machines based on ESP32 Audio kit development board and using HMI over touch 3.5" screen. Currently, powadcr support TAP, TZX, TSX, CDT, WAV and MP3 files, being compatible with ZX Spectrum, Amstrad, MSX and others 8-bit similar machines (see compatible list)

![image](https://github.com/user-attachments/assets/6d7ac494-c201-4113-875b-0324e44a8308)

![image](https://github.com/user-attachments/assets/5513dc37-750b-46e9-9a02-55b858d5e33a)

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

This project need set PCB switches to

|Switch|Value|
|---|---|
|1|Off|
|2|On|
|3|On|
|4|Off|
|5|Off|


## LCD Screen Display

The LCD touch screen display chosen for this project is a TFT HMI LCD Display Module Screen Touch connected with 2 serial pins (TX and RX) to the board. 
+ Brand: TJC
+ Model: TJC4832T035_011
+ Size: 3.5".
+ Resolution:  480x320.

About the loading process with standard timing in Sinclair ZX Spectrum
-----
I recommend Alessandro Grussu's website with interesting information about the loading process and processor timing for this goal. https://www.alessandrogrussu.it/tapir/tzxform120.html#MACHINFO

Now, I'd like to show you how the signal generated from TAP file that Sinclair ZX Spectrum is able to understand. The mechanism to read the audio signal is based on squarewave peak counting, using the Z80 clock timing, then:

The sequence for ZS Spectrum is always for standard loading: 
+ LEADER TONE + SYNC1 + SYNC2 + DATA SEQUENCE + SILENCE

<br>Where: LEADER TONE (2168 T-States) is two kinds of length. 
+ Large (x 8063 T-States) for typical "PROGRAM" block (BASIC)
+ Short (x 3223 T-States) for typical "BYTE" block, Z80 machine code.</br>

**What means T-State?**

Well, this concept could be difficult to understand, but it's not far from reality, as summarized full pulse (two peaks one to high and one to low) has a period equal to "2 x n T-State" time, where T-State = 1/3.5MHz = 0.28571 us, then for example: LARGE LEADER TONE.
+ LEADER TONE = 2168 x 8063 T-States = 17480584 T-States
+ 1 T-State = 1 / 3.5MHz = 0.28571 us = 0.00000028571 s
+ LEADER TONE duration = 17480584 x 0.00000028571 s = 4.98s

**How many peaks have the LARGE LEADER TONE pulse train?**
+ The pulse train has 2168 peaks in both cases but the short leader tone has a different duration (3223 T-States) versus the large leader tone (8063 T-States)

**What's the signal frequency?**
+ We know that LARGE LEADER TONE pulse train is 4.98s 
+ We know that SHORT LEADER TONE pulse train is 1.99s
+ The frecuency for both leader tones (2168 x 0.00000028571) / 2 = 809.2Hz

About POWADCR Device.
-----
In this section, we are going to describe the parts to be needed to assemble the PowaDCR device.

**Bill of material**
+ Main board: ESP32 Audiokit by AI-Thinker technology : https://docs.ai-thinker.com/en/esp32-audio-kit (Possible buy site. Alliexpress)
+ Color LCD 3.5" 480x320 pixels. Resistive TouchScreen - TJC4832T035_011 resistive (low priced but possible to be discontinued and replaced by TJC4832T135 _ 011C capacitive or TJC4832T135 _ 011R resistive)
+ Cable XH2.5 to DuPont to connect LCD to the extended port of ESP32 Audiokit
+ Battery 2000mAh 3.7v (optional not needed)
+ Micro SD card FAT32 formatted (to contain all ZX Spectrum games in TAP and other formats to be red for PowaDCR in the future)
+ Micro SD card or FT232RL FTDI serial interface to program the TJC LCD (both methods are available)
+ Cable with jacks Stereo-stereo male-male 3.5mm to connect PowaDCR to Spectrum Next or N-Go or clone versions.
+ Cable with XH2.5 and mono jack 3.5mm to connect from amplifier out of PowaDCR to EAR connector on ZX Spectrum classic versions (Rubber keyboard 16K, 48K, Spectrum+ and Spectrum 128K Toastrack)
+ HMI Chinesse editor version https://unofficialnextion.com/t/tjc-usart-hmi-editor-1-64-1/1511
+ Arduino IDE 2.0

Hacking the Audiokit board.
-----
This board is built from the same design from AC101 audio chip version but with ES8388 chip. In this case, both mic and line-in are mixed. Not possible to select by independent way mic or line-in then the environment noise comes in when the ZX Spectrum signal is captured.
So, it shall be removed both integrated microphones.

![image](https://github.com/hash6iron/powadcr/assets/118267564/f47c2810-d573-4a8b-9608-7015e7462f15)


How PowaDCR parts are connected?
-----
(in progress)

How .bin firmware is uploaded in ESP32-A1S Audiokit? 
-----
You can flash the binaries directly to the board if you do not want to mess with code and compilers. 

1. Download ESP32 Flash Downloading Tool - https://www.espressif.com/en/support/download/other-tools?keys=&field_type_tid%5B%5D=13
2. Unzip file and execute - flash_download_tool_x.x.x.exe file

   See the example image below.

   ![image](https://github.com/hash6iron/powadcr_IO/assets/118267564/e7158518-4af8-4e6e-b4ab-eff6b9693307)

   
4. Select ESP32 model.
   - ESP32
   - Develop
     And press "OK" button
   
5. Setting and beginning the flash process.
   - Select the .bin file or type the path of it (from /build)
   - Select all parameters exactly in the image below.
   - Connect ESP32-A1S Audiokit board from UART microUSB port (not power microUSB PORT) at PC USB port.
   - Select the available COM for this connection in COM: field on ESP32 Flash Downloading Tool.
   - Select BAUD: speed at 460800
   - Disconnect the Touch Screen cable in order to release serial port (Audiokit sharing USB and UART communications)
   - Press the START button in ESP32 Flash Downloading Tool. Then the downloading process begins, wait for the FINISH message. Enjoy!
  
     NOTES: If the process fail.
      - Try to download it again.
      - Try to ERASE before the START process.
   
   Show the image below.
   
   ![image](https://github.com/user-attachments/assets/b5c189c6-8945-4a65-9e22-e17a56d3eea6)


How custom firmware is uploaded in ESP32-A1S Audiokit? 
-----
1. Install VSCode
   - https://code.visualstudio.com/download
3. Install PlatformIO for VSCode
   - https://platformio.org/install
5. Open the powaDCR project into VSCode
6. Connect the ESP32 Audiokit USART USB port to any USB PC port
7. Press BUILD (arrow icon) from the PlatformIO toolbar.

How firmware is loaded in TJC LCD?
-----
Upload the file "HMI/../build/powadcr_iface.tft" to the root of the SD and insert this into powaDCR. Power on the powaDCR and Wait until firmware is uploaded.

Compatible 8-bits machine list
-----
Currently, powadcr is compatible with

| Machine           | File type   |
| :---:             |    :----:   |
| ZX Spectrum       | TAP, TZX    |
| Amstrad           | CDT         |
| MSX               | TSX         |
| Enter Enterprise  | TZX*        |
| Oric              | TZX*        |
| Lynx              | TZX*        |

*In general, Powadcr is compatible with all systems using the TZX or TSX block ID15 (Direct Recording) 

Enjoy this device and support me.
-----
If you enjoy this device and you want to collaborate, please.

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=BAWGJFZGXE5GE&source=url"><img src="/doc/paypal_boton.png" /></a>

<a href="https://www.buymeacoffee.com/atamairon"><img src="/doc/coffe.jpg" /></a>

thxs.
