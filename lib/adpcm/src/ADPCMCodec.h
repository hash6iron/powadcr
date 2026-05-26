#pragma once
#include "adpcm-ffmpeg/adpcm.h"
#include "adpcm-ffmpeg/bytestream.h"
#include "string.h"
#include "stddef.h"
#include "ADPCMVector.h"

#define ADAPCM_DEFAULT_BLOCK_SIZE 128

#pragma GCC diagnostic ignored "-Wcomment"
#pragma GCC diagnostic ignored "-Wsign-compare"

namespace adpcm_ffmpeg {

/**
 * @brief Common ADPCM Functionality
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

class ADPCMCodec {
 public:
  ADPCMCodec() {
    memset(&avctx, 0, sizeof(avctx));
    memset(&enc_ctx, 0, sizeof(enc_ctx));
  }

  AVCodecContext &ctx() { return avctx; }

  void setCodecID(AVCodecID id) { avctx.codec_id = id; }

  AVCodecID codecID() { return avctx.codec_id; }

  void setFrameSize(int fs) { avctx.frame_size = fs; }

  virtual void setBlockSize(int size) { enc_ctx.block_size = size; }

  int blockSize() { return enc_ctx.block_size; }

  int frameSize() { return avctx.frame_size; }

  int channels() { return avctx.nb_channels; }

  bool isPlanar() { 
    for (AVSampleFormat fmt: sample_formats){
      if (fmt == AV_SAMPLE_FMT_S16P) return true;
    }
    return false;
  }

 protected:
  AVCodecContext avctx;
  ADPCMEncodeContext enc_ctx;
  ADPCMVector<AVSampleFormat> sample_formats{0};

  int av_get_bits_per_sample() {
    switch (avctx.codec_id) {
      case AV_CODEC_ID_ADPCM_SBPRO_2:
        return 2;
      case AV_CODEC_ID_ADPCM_SBPRO_3:
        return 3;
      case AV_CODEC_ID_ADPCM_SBPRO_4:
      case AV_CODEC_ID_ADPCM_IMA_WAV:
      case AV_CODEC_ID_ADPCM_IMA_QT:
      case AV_CODEC_ID_ADPCM_SWF:
      case AV_CODEC_ID_ADPCM_MS:
      case AV_CODEC_ID_ADPCM_ARGO:
      case AV_CODEC_ID_ADPCM_CT:
      case AV_CODEC_ID_ADPCM_IMA_ALP:
      case AV_CODEC_ID_ADPCM_IMA_AMV:
      case AV_CODEC_ID_ADPCM_IMA_APC:
      case AV_CODEC_ID_ADPCM_IMA_APM:
      case AV_CODEC_ID_ADPCM_IMA_EA_SEAD:
      case AV_CODEC_ID_ADPCM_IMA_OKI:
      case AV_CODEC_ID_ADPCM_IMA_WS:
      case AV_CODEC_ID_ADPCM_IMA_SSI:
      case AV_CODEC_ID_ADPCM_G722:
      case AV_CODEC_ID_ADPCM_YAMAHA:
      case AV_CODEC_ID_ADPCM_AICA:
        return 4;
      default:
        return 0;
    }
  }

  int ff_get_encode_buffer(AVCodecContext *avctx, AVPacket *avpkt, int64_t size,
                           int flags) {
    avpkt->size = size;
    return 0;
  }

  int ff_get_buffer(AVCodecContext *avctx, AVFrame *frame, int flags) {
    return 0;
  }

  //  error message about missing feature
  void avpriv_request_sample(void *avc, const char *msg, ...) {
    printf("%s", msg);
  }
};

}  // namespace adpcm_ffmpeg