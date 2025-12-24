
/**
 * @file speed_test.ino
 * @author Phil Schatzmann
 * @brief We just calulate the number of encoded samples per second
 * @version 0.1
 * @date 2021-08-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "MP3EncoderLAME.h"
#include <stdlib.h>  // for rand

using namespace liblame;

void dataCallback(uint8_t *mp3_data, size_t len) {
}

MP3EncoderLAME mp3(dataCallback);
AudioInfo info;
const int size = 500;
const int time_sec = 10;
int16_t buffer[size];

void speed_test(){
    size_t total = 0;
    // we encode for the indicated number of seconds
    uint64_t end = millis() + (time_sec*1000);
    while(millis()<end){
        // fill buffer with random number
        for (int j=0;j<size;j++){
            buffer[j] = (rand() % 100) - 50;         
        }

        mp3.write(buffer, size*sizeof(int16_t));
        total+=size;
    }

    Serial.print("Samples decoded per second: ");
    Serial.print(total / time_sec / 1000);
    Serial.println(" kHz");
}

void setup() {
    Serial.begin(115200);

    info.channels = 1;
    info.sample_rate = 44100;
    info.quality = 1;
    mp3.begin(info);

    Serial.println("testing...");
}

void loop() {
    speed_test();
}
