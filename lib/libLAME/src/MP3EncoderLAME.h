#pragma once

#include "lame_log.h"
#include "liblame/lame.h"
#include <stdint.h>
#include <unistd.h>

namespace liblame {

typedef void (*MP3CallbackFDK)(uint8_t *mp3_data, size_t len);
/**
 * @brief LAME parameters
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
struct AudioInfo {
  AudioInfo() = default;
  AudioInfo(const AudioInfo &) = default;
  int sample_rate = 16000;
  int channels = 1;
  int bits_per_sample = 16; // we assume int16_t
  int quality = 7;          // 0..9.  0=best (very slow).  9=worst.
  int frame_size = 0;       // determined by decoder
};

/**
 * @brief Encodes PCM data to the MP3 format and writes the result to a stream
 * or provides it via a callback
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class MP3EncoderLAME {

public:
  /// Empty Constructor: call setDataCallback or setStream to define how the
  /// result should be provided.
  MP3EncoderLAME() { LOG_LAME(LAMEDebug, __FUNCTION__); }

  /// Constructor which provides the decoded result in a callback
  MP3EncoderLAME(MP3CallbackFDK cb) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    setDataCallback(cb);
  }

  /// Defines the callback which receives the encded MP3 data
  void setDataCallback(MP3CallbackFDK cb) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    this->MP3Callback = cb;
  }

#ifdef ARDUINO

  /// Constructor which makes sure that the decoded result is written to the
  /// indicatd Stream
  MP3EncoderLAME(Print &out_stream) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    this->out = &out_stream;
  }

  /// Defines the output stream
  void setOutput(Print &out_stream) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    this->out = &out_stream;
  }

#endif

  ~MP3EncoderLAME() {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    end();
    // release buffers
    if (convert_buffer != nullptr)
      delete[] convert_buffer;
    if (mp3_buffer != nullptr)
      delete[] mp3_buffer;
  }

  /**
   * @brief Opens the encoder
   */
  bool begin() {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    active = setup();
    return active;
  }

  /**
   * @brief Opens the encoder
   *
   * @param info
   * @return int
   */
  bool begin(AudioInfo in) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    setAudioInfo(in);
    active = setup();
    return active;
  }

  /**
   * @brief Opens the encoder
   *
   * @param input_channels
   * @param input_sample_rate
   * @param input_bits_per_sample
   * @return int 0 => ok; error with negative number
   */
  void begin(int input_channels, int input_sample_rate,
             int input_bits_per_sample) {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    AudioInfo ai;
    ai.channels = input_channels;
    ai.sample_rate = input_sample_rate;
    ai.bits_per_sample = input_bits_per_sample;
    setAudioInfo(ai);
    active = setup();
  }

  /// Defines the audio information
  void setAudioInfo(AudioInfo in) { this->info = in; }

  /// Provides the audio information
  AudioInfo audioInfo() { return info; }

  /// write PCM data to be converted to MP3 - The size is in bytes
  int32_t write(void *pcm_samples, int bytes) {
    int32_t result = 0;
    if (active) {
      LOG_LAME(LAMEDebug, "write %d bytes", bytes);

      // convert to required input format
      short *buffer = convertToShort(pcm_samples, bytes);
      if (buffer == nullptr) {
        return 0;
      }

      // setup output buffer if necessary
      int nsamples = bytes / (info.bits_per_sample / 8) / info.channels;
      int outbuf_size = 7200 + (1.25 * nsamples);
      if (!setupOutputBuffer(outbuf_size)) {
        return 0;
      }

      // encode based on the number of channels
      int mp3_len = 0;
      if (info.channels == 1) {
        mp3_len =
            lame_encode_buffer(lame, buffer, NULL, nsamples, mp3_buffer, 0);
      } else {
        mp3_len = lame_encode_buffer_interleaved(lame, buffer, nsamples,
                                                 mp3_buffer, 0);
      }

      if (mp3_len > 0) {
        provideResult((uint8_t *)mp3_buffer, mp3_len);
      }
      result = bytes;
    }
    return result;
  }

  /// closes the processing and release resources
  void end() {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    lame_close(lame);
    lame = nullptr;
  }

  operator boolean() { return active; }

protected:
  bool active;
  MP3CallbackFDK MP3Callback = nullptr;
  AudioInfo info;
  // lame
  lame_t lame = nullptr;
  // mp3 result buffer
  uint8_t *mp3_buffer = nullptr;
  int mp3_buffer_size = 0;
  // conversion to short
  short *convert_buffer = nullptr;
  int convert_buffer_size = 0;

#ifdef ARDUINO
  Print *out;
#endif

  bool setupOutputBuffer(int size) {
    if (size > mp3_buffer_size) {
      LOG_LAME(LAMEDebug, __FUNCTION__);
      if (mp3_buffer != nullptr)
        delete[] mp3_buffer;
      mp3_buffer = new uint8_t[size];
      mp3_buffer_size = size;
    }
    return mp3_buffer != nullptr;
  }

  bool setup() {
    LOG_LAME(LAMEDebug, __FUNCTION__);
    if (lame==nullptr){
      lame = lame_init();
    }
    if (lame == nullptr) {
      LOG_LAME(LAMEError, "lame_init failed");
      return false;
    }

    lame_set_VBR(lame, vbr_default);
    lame_set_num_channels(lame, info.channels);
    if (info.channels == 1) {
      lame_set_mode(lame, MONO);
    }
    lame_set_in_samplerate(lame, info.sample_rate);
    //lame_set_out_samplerate(lame, info.sample_rate);
    lame_set_quality(lame, info.quality);

    int initRet = lame_init_params(lame);
    if (initRet < 0) {
      LOG_LAME(LAMEError, "lame_init_params\n");
      return false;
    }

    info.frame_size = lame_get_framesize(lame);
    LOG_LAME(LAMEInfo, "Framesize = %d\n", info.frame_size);
    return true;
  }

  short *convertToShort(void *pcm_samples, int bytes) {
    int bytes_per_sample = info.bits_per_sample / 8;
    // input is already in correct format
    if (sizeof(short) == bytes_per_sample) {
      return (short *)pcm_samples;
    }

    // if we got here conversion is required
    LOG_LAME(LAMEDebug, __FUNCTION__);

    // allocate conversion buffer
    int samples = bytes / bytes_per_sample;
    if (convert_buffer == nullptr || samples > convert_buffer_size) {
      if (convert_buffer != nullptr) {
        delete[] convert_buffer;
      }
      convert_buffer = new short[samples];
      convert_buffer_size = samples;
    }

    if (convert_buffer == nullptr) {
      LOG_LAME(LAMEError, "not enough memory to allocate conversion buffer");
      lame_abort();
      return nullptr;
    }

    // convert input to short
    switch (info.bits_per_sample) {
    case 8: {
      int8_t *ptr = (int8_t *)pcm_samples;
      for (int j = 0; j < samples; j++) {
        convert_buffer[j] = ptr[j];
      }
      return convert_buffer;
    }
    case 16: {
      int16_t *ptr = (int16_t *)pcm_samples;
      for (int j = 0; j < samples; j++) {
        convert_buffer[j] = ptr[j];
      }
      return convert_buffer;
    }
    case 32: {
      int32_t *ptr = (int32_t *)pcm_samples;
      for (int j = 0; j < samples; j++) {
        convert_buffer[j] = ptr[j];
      }
      return convert_buffer;
    }
    default:
      LOG_LAME(LAMEError, "Unsupported bits_per_sample: %d", info.bits_per_sample);
      return nullptr;
    }
  }

  /// return the result PWM data
  void provideResult(uint8_t *data, size_t bytes) {
    if (bytes > 0) {
      LOG_LAME(LAMEDebug, "provideResult: %zu samples", bytes);
      // provide result
      if (MP3Callback != nullptr) {
        // output via callback
        MP3Callback(data, bytes);
      }
#ifdef ARDUINO
      if (out != nullptr) {
        out->write((uint8_t *)data, bytes);
      }
#endif
    }
  }
};

} // namespace liblame
