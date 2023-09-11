#include "AudioKitHAL.h"
#include "SdFat.h"



AudioKit kit;
SdFat32 sdf;
File32 dir;
File32 file;

#include "TAPrecorder.h"
TAPrecorder taprec;

void setup() {
  Serial.begin(921600);
  // open in read mode
  auto cfg = kit.defaultConfig(AudioInput);
  //cfg.codec_mode = AUDIO_HAL_CODEC_MODE_LINE_IN;
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // microphone?
  cfg.sample_rate = AUDIO_HAL_44K_SAMPLES;
  kit.begin(cfg);

  Serial.println("");
  Serial.println("Waiting for guide tone");
  Serial.println("");

  taprec.set_kit(kit);
  taprec.initialize();
  
}

void loop() 
{
  taprec.recording();
}