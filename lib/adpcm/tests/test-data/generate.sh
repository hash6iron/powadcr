#!/usr/bin/env bash

# Generate adpcm wav files for all supported formats: we expect that we can process them successully 
# with this library
codecs=("ima_wav" "ms" "swf" "yamaha" "ima_ws")

CMD=""
for codec in "${codecs[@]}"
do
  CMD="ffmpeg -y -i original.wav -acodec adpcm_$codec $codec.wav"
  echo "$CMD"
  $CMD
done

