/**
 * Just a simple test program where we encode a stereo sine tone and then
 * decode it again and display the result.
 */

#include <assert.h>
#include <iostream>
#include <string.h>
#include "ADPCM.h"
#include "ADPCMVector.h"
#include "SineGenerator.h"

using namespace adpcm_ffmpeg;

AVCodecID code =
    AV_CODEC_ID_ADPCM_MS;  // AV_CODEC_ID_ADPCM_MS; // AV_CODEC_ID_ADPCM_IMA_WAV
ADPCMVector<int16_t> sample_vector;
SineWaveGenerator<int16_t> genLeft{30000.0};
SineWaveGenerator<int16_t> genRight{30000.0};
int channels = 2;
int sample_rate = 44100;
int loop_count = 100;

int loadSamples(int frame_size) {
  // fill frame with data
  int result = 0;
  for (int j = 0; j < frame_size; j += channels) {
    sample_vector[j] = genLeft.nextSample();
    if (channels == 2) sample_vector[j + 1] = genRight.nextSample();
    result += channels;
  }
  return result;
}

void displayPacket(AVPacket& packet) {
  for (int j = 0; j < packet.size; j++) {
    if (j % 16 == 0) std::cout << std::endl;
    printf("0x%x ", packet.data[j]);
  }
  std::cout << std::endl;
}

void displayResult(AVFrame& frame) {
  // print the result
  int16_t* data = (int16_t*)frame.data[0];
  size_t samples = frame.nb_samples * channels ;
  for (int j = 0; j < samples; j += channels) {
    for (int ch = 0; ch < channels; ch++) {
      std::cout << data[j + ch] << " ";
    }
    std::cout << std::endl;
  }
}

int checkFrameSize(ADPCMDecoder &decoder, ADPCMEncoder &encoder) {
  // determine frame size
  size_t frame_size = encoder.frameSize();
  assert(frame_size == decoder.frameSize());
  assert(frame_size > 0);
  return frame_size;
}

void test(AVCodecID id, const char *title) {
  std::cout << title << "\n";
  ADPCMDecoder& decoder = *ADPCMDecoderFactory::create(id);
  ADPCMEncoder& encoder = *ADPCMEncoderFactory::create(id);

  assert(&decoder != nullptr);
  assert(&encoder != nullptr);

  genLeft.begin(sample_rate, 220);
  genRight.begin(sample_rate, 440);

  // open codec
  if (!encoder.begin(sample_rate, channels)) {
    std::cout << "encoder not supported";
    return;
  }
  if (!decoder.begin(sample_rate, channels)) {
    std::cout << "decoder not supported";
    return;
  }

  int frame_size = checkFrameSize(decoder, encoder);
  int sample_count = frame_size * channels;
  int frame_count = sample_count;

  // setup data for frame
  sample_vector.resize(sample_count);

  for (int n = 0; n < loop_count; n++) {
    size_t samples = loadSamples(sample_count);

    AVPacket& packet = encoder.encode(&sample_vector[0], sample_count);
    displayPacket(packet);

    // decode
    AVFrame& frame = decoder.decode(packet);
    displayResult(frame);
  }
  // close
  encoder.end();
  decoder.end();
  delete &encoder;
  delete &decoder;
}


int main() {
  test(AV_CODEC_ID_ADPCM_IMA_WAV,"IMA_WAV");
  test(AV_CODEC_ID_ADPCM_IMA_SSI,"IMA_SSI");
  test(AV_CODEC_ID_ADPCM_IMA_ALP, "IMA_ALP");
  test(AV_CODEC_ID_ADPCM_MS, "MS");
  test(AV_CODEC_ID_ADPCM_YAMAHA,"YAHMA");
  test(AV_CODEC_ID_ADPCM_IMA_APM,"IMA_APM");
  test(AV_CODEC_ID_ADPCM_ARGO,"ARGO");
  test(AV_CODEC_ID_ADPCM_IMA_WS,"IMA_WS");
  test(AV_CODEC_ID_ADPCM_SWF,"SWF");
  // test(AV_CODEC_ID_ADPCM_IMA_AMV,"IMA_AMV"); // only mono at 22050!
  // test(AV_CODEC_ID_ADPCM_IMA_QT,"IMA_QT"); // broken !

  std::cout << "*** END ***" << "\n";


  return 0;
}