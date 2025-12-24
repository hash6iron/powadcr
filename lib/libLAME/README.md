# MP3 Encoding with LAME

[LAME](https://lame.sourceforge.io/about.php) is a open source implementation of a MP3 encoder.
This project just reorganized the code to follow Arduino Library conventions, so that you can use it in Arduino and PlatformIO. 

I have also added a simple Arduino style C++ API which allows to output the resulting MP3 via to a Arduino Stream or to receive it via a callback. 

### API
Here is an example Arduino sketch for encoding PCM data into MP3:

```C++
#include "MP3EncoderLAME.h"
#include <stdlib.h>  // for rand

using namespace liblame;

void dataCallback(uint8_t *mp3_data, size_t len) {
    Serial.print("mp3 generated with ");
    Serial.print(len);
    Serial.println(" bytes");
}

MP3EncoderLAME mp3(dataCallback);
AudioInfo info;
int16_t buffer[512];

void setup() {
    Serial.begin(115200);

    info.channels = 1;
    info.sample_rate = 44100;
    mp3.begin(info);

    Serial.println("writing...");

}

void loop() {
    Serial.println("writing 512 samples of random data");
    for (int j=0;j<512;j++){
        buffer[j] = (rand() % 100) - 50;         
    }
    mp3.write(buffer, 512*sizeof(int16_t));
}

```

### Installation

In Arduino, you can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```bash
cd  ~/Documents/Arduino/libraries
git clone pschatzmann/arduino-liblame.git

```

This project can also be built and executed on your desktop with cmake:

```bash
cd arduino-liblame
mkdir build
cd build
cmake ..
make
```

### Documentation

- The [Class Documentation can be found here](https://pschatzmann.github.io/arduino-liblame/html/annotated.html)
- You can also find some [more information in my dedicated Blog](https://www.pschatzmann.ch/home/2021/08/10/an-mp3-encoder-for-arduino/).
- I also suggest that you have a look at [my overview Blog](https://www.pschatzmann.ch/home/2021/08/13/audio-decoders-for-microcontrollers/)

I recommend to use this library together with my [Arduino Audio Tools](https://github.com/pschatzmann/arduino-audio-tools). 
This is just one of many __codecs__ that I have collected so far: Further details can be found in the [Encoding and Decoding Wiki](https://github.com/pschatzmann/arduino-audio-tools/wiki/Encoding-and-Decoding-of-Audio) of the Audio Tools.

### Memory/RAM

This library requires quite a lot of RAM and it should work if you just use this decoder w/o any additional functionality that requires additional RAM. If you want to use e.g. WIFI together with this library you might not have enough RAM: Fortunately this library supports PSRAM so if you activate PSRAM you should have more then enough headroom...  

### Changes to the Original Code
The initial restructured code was working prefectly with my [Arduino Simulator](https://github.com/pschatzmann/Arduino-Emulator) on the desktop, but as soon I as I deployed it on an ESP32 it was crashing because of different reasons:

- The memory structures were partly too big 
- Long arrays were allocated on the stack

So I needed to adjust the original code quite a bit. You can activate some micro processor specific functionaitly in config.h. 

```C++
// use dynamically precalculated log table
#define USE_FAST_LOG 0

// use precalculated log table as const -> in the ESP32 this will end up in flash memory
#define USE_FAST_LOG_CONST 1

// Avoid big memory allocations in replaygain_data: with PSRAM this can be 0
#define USE_MEMORY_HACK 1

// If you know the encoder will be used in a single threaded environment, you can use this hack to just
// recycle the memory. This will prevent memory fragmentation. Only use this if you are sure that the
// encoder will be called from a single thread.
#define USE_STACK_HACK_RECYCLE_ALLOCATION_SINGLE_THREADED 1

// If the device is ESP32 and ESP_PSRAM_ENABLE_LIMIT is > 0, then the ESP32 will
// be configured to use allocate any allocation above ESP_PSRAM_ENABLE_LIMIT using
// psram, rather than scarce main memory.
#define ESP_PSRAM_ENABLE_LIMIT 1024

// Not all microcontroller support vararg methods: alternative impelemtation of logging using the preprocessor
#define USE_LOGGING_HACK 1

// Print debug and trace messages
#define USE_DEBUG 0

// Print memory allocations and frees
#define USE_DEBUG_ALLOC 0

// The stack on microcontrollers is very limited - use the heap for big arrays instead of the stack! 
#define USE_STACK_HACK 1
```

ESP32 Arduino 2.0.x Releases are using a lot of static memory and you will need to set USE_MEMORY_HACK to 0 to be able to compile this library. 