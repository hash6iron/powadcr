
/**
 * @file output_mp3.ino
 * @author Phil Schatzmann
 * @brief We just encode random pcm data to mp3
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

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

    info.channels = 2;
    info.sample_rate = 16000;
    mp3.begin(info);

    Serial.println("starting...");

}

void loop() {
    for (int j=0;j<512;j++){
        buffer[j] = (rand() % 100) - 50;         
    }
    if (mp3.write(buffer, 512*sizeof(int16_t))){
        Serial.println("512 samples of random data written");
    }
}
