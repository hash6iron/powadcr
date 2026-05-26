#pragma once
#include "ADPCM.h"
#include "ADPCMCodec.h"
#include "adpcm-ffmpeg/put_bits.h"

#define FREEZE_INTERVAL 128

namespace adpcm_ffmpeg {

/**
 * @brief ADPCM Encoder
 * @author Phil Schatzmann
 * @copyright GPLv3
 */
class ADPCMEncoder : public ADPCMCodec {
 public:
  ADPCMEncoder() : ADPCMCodec() {
    setBlockSize(ADAPCM_DEFAULT_BLOCK_SIZE);
    avctx.priv_data = (uint8_t *)&enc_ctx;
  }

  bool begin(int sampleRate, int channels) {
    avctx.sample_rate = sampleRate;
    avctx.nb_channels = channels;
    avctx.sample_fmt = sample_formats[0];
    bool rc = adpcm_encode_init() == 0;
    printf("frame_size: %d", frameSize());
    return rc;
  }

  void end() { adpcm_encode_close(); }

  AVPacket &encode(int16_t *data, size_t sampleCount) {
    frame.nb_samples = sampleCount / avctx.nb_channels;
    // fill data
    frame.data[0] = (uint8_t *)data;

    // fill extended_data
    frame.extended_data = extended_data;
    if (channels() == 1 || !isPlanar()) {
      extended_data[0] = data;
    } else if (channels() == 2) {
      // if channels() is 2 we need to split up the stereo data
      // into separate  frame_extended_data_vector2 arrays
      frame_extended_data_vectors.resize(channels());
      for (int ch=0;ch<channels();ch++){
        frame_extended_data_vectors[ch].resize(sampleCount/channels());
        extended_data[ch] = &frame_extended_data_vectors[ch][0];
      }

      // fill with data
      for (int j = 0; j < sampleCount / channels(); j++) {
        for (int ch=0;ch<channels();ch++){
          frame_extended_data_vectors[ch][j] = data[(j * channels()) + ch];
        }
      }
    }

    int got_packet_ptr = 0;
    av_packet_data.resize(sampleCount);
    result.data = &av_packet_data[0];

    int rc = adpcm_encode_frame(&result, &frame, &got_packet_ptr);
    if (rc != 0 || !got_packet_ptr) {
      result.size = 0;
    }
    return result;
  }

  virtual bool is_trellis() { return false; }

  int blockAlign() { return avctx.block_align;}

 protected:
  AVPacket result;
  AVFrame frame;
  int16_t *extended_data[2] = {0};
  ADPCMVector<uint8_t> av_packet_data;
  ADPCMVector<ADPCMVector<int16_t>> frame_extended_data_vectors;
  // encoding data
  int st, pkt_size, ret;
  const int16_t *samples;
  const int16_t *const *samples_p;
  uint8_t *dst;
  ADPCMEncodeContext *c;
  ADPCMEncodeContext *s;

  virtual int adpcm_encode_init_impl() = 0;

  virtual int adpcm_encode_init() {
    s = (ADPCMEncodeContext *)avctx.priv_data;

    if (s == NULL) {
      return -1;
    }

    /*
     * AMV's block size has to match that of the corresponding video
     * stream. Relax the POT requirement.
     */
    if (avctx.codec_id != AV_CODEC_ID_ADPCM_IMA_AMV &&
        (s->block_size & (s->block_size - 1))) {
      av_log(avctx, AV_LOG_ERROR, "block size must be power of 2: %d\n",
             s->block_size);
      return AVERROR(AVERROR_INVALID);
    }

    if (avctx.trellis) {
      int frontier, max_paths;

      if ((unsigned)avctx.trellis > 16U) {
        av_log(avctx, AV_LOG_ERROR, "invalid trellis size\n");
        return AVERROR(AVERROR_INVALID);
      }

      if (avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_SSI ||
          avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_APM ||
          avctx.codec_id == AV_CODEC_ID_ADPCM_ARGO ||
          avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_WS) {
        /*
         * The current trellis implementation doesn't work for extended
         * runs of samples without periodic resets. Disallow it.
         */
        av_log(avctx, AV_LOG_ERROR, "trellis not supported\n");
        return AVERROR_PATCHWELCOME;
      }

      frontier = 1 << avctx.trellis;
      max_paths = frontier * FREEZE_INTERVAL;
      if (!FF_ALLOC_TYPED_ARRAY(TrellisPath *, s->paths, max_paths) ||
          !FF_ALLOC_TYPED_ARRAY(TrellisNode *, s->node_buf, 2 * frontier) ||
          !FF_ALLOC_TYPED_ARRAY(TrellisNode **, s->nodep_buf, 2 * frontier) ||
          !FF_ALLOC_TYPED_ARRAY(uint8_t *, s->trellis_hash, 65536))
        return AVERROR(AVERROR_MEMORY);
    }

    avctx.bits_per_coded_sample = av_get_bits_per_sample();

    return adpcm_encode_init_impl();
  }

  int adpcm_encode_close() {
    ADPCMEncodeContext *s = (ADPCMEncodeContext *)avctx.priv_data;
    av_freep(&s->paths);
    av_freep(&s->node_buf);
    av_freep(&s->nodep_buf);
    av_freep(&s->trellis_hash);

    return 0;
  }

  inline uint8_t adpcm_ima_compress_sample(ADPCMChannelStatus *c,
                                           int16_t sample) {
    int delta = sample - c->prev_sample;
    int nibble = FFMIN(7, abs(delta) * 4 / ff_adpcm_step_table[c->step_index]) +
                 (delta < 0) * 8;
    c->prev_sample += ((ff_adpcm_step_table[c->step_index] *
                        ff_adpcm_yamaha_difflookup[nibble]) /
                       8);
    c->prev_sample = av_clip_int16(c->prev_sample);
    c->step_index =
        av_clip(c->step_index + ff_adpcm_index_table[nibble], 0, 88);
    return nibble;
  }

  inline uint8_t adpcm_ima_qt_compress_sample(ADPCMChannelStatus *c,
                                              int16_t sample) {
    int delta = sample - c->prev_sample;
    int diff, step = ff_adpcm_step_table[c->step_index];
    int nibble = 8 * (delta < 0);

    delta = abs(delta);
    diff = delta + (step >> 3);

    if (delta >= step) {
      nibble |= 4;
      delta -= step;
    }
    step >>= 1;
    if (delta >= step) {
      nibble |= 2;
      delta -= step;
    }
    step >>= 1;
    if (delta >= step) {
      nibble |= 1;
      delta -= step;
    }
    diff -= delta;

    if (nibble & 8)
      c->prev_sample -= diff;
    else
      c->prev_sample += diff;

    c->prev_sample = av_clip_int16(c->prev_sample);
    c->step_index =
        av_clip(c->step_index + ff_adpcm_index_table[nibble], 0, 88);

    return nibble;
  }

  inline uint8_t adpcm_yamaha_compress_sample(ADPCMChannelStatus *c,
                                              int16_t sample) {
    int nibble, delta;

    if (!c->step) {
      c->predictor = 0;
      c->step = 127;
    }

    delta = sample - c->predictor;

    nibble = FFMIN(7, abs(delta) * 4 / c->step) + (delta < 0) * 8;

    c->predictor += ((c->step * ff_adpcm_yamaha_difflookup[nibble]) / 8);
    c->predictor = av_clip_int16(c->predictor);
    c->step = (c->step * ff_adpcm_yamaha_indexscale[nibble]) >> 8;
    c->step = av_clip(c->step, 127, 24576);

    return nibble;
  }

  virtual int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                                      int *got_packet_ptr) = 0;

  virtual int adpcm_encode_frame(AVPacket *avpkt, const AVFrame *frame,
                                 int *got_packet_ptr) {
    c = (ADPCMEncodeContext *)avctx.priv_data;

    samples = (const int16_t *)frame->data[0];
    samples_p = (const int16_t *const *)frame->extended_data;
    assert(samples_p != NULL);
    assert(samples != NULL);
    st = channels() == 2;

    if (avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_SSI ||
        avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_ALP ||
        avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_APM ||
        avctx.codec_id == AV_CODEC_ID_ADPCM_IMA_WS)
      pkt_size = (frame->nb_samples * channels() + 1) / 2;
    else
      pkt_size = avctx.block_align;
    if ((ret = ff_get_encode_buffer(&avctx, avpkt, pkt_size, 0)) < 0)
      return ret;
    dst = avpkt->data;

    int rc = adpcm_encode_frame_impl(avpkt, frame, got_packet_ptr);
    if (rc != AV_OK) return rc;

    *got_packet_ptr = 1;
    return 0;
  }
};

class ADPCMEncoderTrellis : public ADPCMEncoder {
 public:
  bool is_trellis() { return avctx.trellis; }
  void set_trellis(bool flag) { avctx.trellis = flag;}
  bool store_node(int STEP_INDEX) {
    int d;
    uint32_t ssd;
    int pos;
    TrellisNode *u;
    uint8_t *h;
    dec_sample = av_clip_int16(dec_sample);
    d = sample - dec_sample;
    ssd = nodes[j]->ssd +
          d * (unsigned)d; /* Check for wraparound, skip such samples
                            * completely.          \
                            * Note, changing ssd to a 64 bit variable would be \
                            * simpler, avoiding this check, but it's slower on \
                            * x86 32 bit at the moment. */
    if (ssd < nodes[j]->ssd) {
      /* Collapse any two states with the same previous
       * sample value. One could also distinguish states by step and by 2nd
       * to last sample, but the effects of that are negligible.
       * Since nodes in the previous generation are iterated through a heap,
       * they're roughly ordered from better to worse, but not strictly ordered.
       * Therefore, an earlier node with the same sample value is better in most
       * cases (and thus the current is skipped), but not strictly
       * in all cases. Only skipping samples where ssd >= ssd of the earlier
       * node with the same sample gives slightly worse quality, though, for
       * some reason. */
      return true;
    }
    h = &hash[(uint16_t)dec_sample];
    if (*h == generation) return true;
    if (heap_pos < frontier) {
      pos = heap_pos++;
    } else { /* Try to replace one of the leaf nodes with the new          \
              * one, but try a different slot each time. */
      pos = (frontier >> 1) + (heap_pos & ((frontier >> 1) - 1));
      if (ssd > nodes_next[pos]->ssd) return true;
      heap_pos++;
    }
    *h = generation;
    u = nodes_next[pos];
    if (!u) {
      av_assert(pathn < FREEZE_INTERVAL << avctx.trellis);
      u = t++;
      nodes_next[pos] = u;
      u->path = pathn++;
    }
    u->ssd = ssd;
    u->step = STEP_INDEX;
    u->sample2 = nodes[j]->sample1;
    u->sample1 = dec_sample;
    paths[u->path].nibble = nibble;
    paths[u->path].prev =
        nodes[j]->path; /* Sift the newly inserted node up in the heap to \
                         * restore the heap property. */
    while (pos > 0) {
      int parent = (pos - 1) >> 1;
      if (nodes_next[parent]->ssd <= ssd) break;
      FFSWAP(TrellisNode *, nodes_next[parent], nodes_next[pos]);
      pos = parent;
    }
    return false;
  }

  void loop_nodes(int16_t STEP_TABLE, int STEP_INDEX) {
    const int predictor = nodes[j]->sample1;
    const int div = (sample - predictor) * 4 / STEP_TABLE;
    int nmin = av_clip(div - range, -7, 6);
    int nmax = av_clip(div + range, -6, 7);
    if (nmin <= 0) nmin--; /* distinguish -0 from +0 */
    if (nmax < 0) nmax--;
    for (nidx = nmin; nidx <= nmax; nidx++) {
      const int nibble = nidx < 0 ? 7 - nidx : nidx;
      dec_sample =
          predictor + (STEP_TABLE * ff_adpcm_yamaha_difflookup[nibble]) / 8;
      store_node(STEP_INDEX);
    }
  }

  void adpcm_compress_trellis(const int16_t *samples, uint8_t *dst,
                              ADPCMChannelStatus *c, int n, int stride) {
    // FIXME 6% faster if frontier is a compile-time constant
    s = (ADPCMEncodeContext *)avctx.priv_data;
    frontier = 1 << avctx.trellis;
    version = avctx.codec_id;
    paths = s->paths, *p;
    node_buf = s->node_buf;
    nodep_buf = s->nodep_buf;
    nodes = nodep_buf;  // nodes[] is always sorted by .ssd
    nodes_next = nodep_buf + frontier;
    pathn = 0, froze = -1, i, j, k, generation = 0;
    hash = s->trellis_hash;
    memset(hash, 0xff, 65536 * sizeof(*hash));

    memset(nodep_buf, 0, 2 * frontier * sizeof(*nodep_buf));
    nodes[0] = node_buf + frontier;
    nodes[0]->ssd = 0;
    nodes[0]->path = 0;
    nodes[0]->step = c->step_index;
    nodes[0]->sample1 = c->sample1;
    nodes[0]->sample2 = c->sample2;
    if (version == AV_CODEC_ID_ADPCM_IMA_WAV ||
        version == AV_CODEC_ID_ADPCM_IMA_QT ||
        version == AV_CODEC_ID_ADPCM_IMA_AMV ||
        version == AV_CODEC_ID_ADPCM_SWF)
      nodes[0]->sample1 = c->prev_sample;
    if (version == AV_CODEC_ID_ADPCM_MS) nodes[0]->step = c->idelta;
    if (version == AV_CODEC_ID_ADPCM_YAMAHA) {
      if (c->step == 0) {
        nodes[0]->step = 127;
        nodes[0]->sample1 = 0;
      } else {
        nodes[0]->step = c->step;
        nodes[0]->sample1 = c->predictor;
      }
    }

    for (i = 0; i < n; i++) {
      t = node_buf + frontier * (i & 1);
      sample = samples[i * stride];
      heap_pos = 0;
      memset(nodes_next, 0, frontier * sizeof(TrellisNode *));
      for (j = 0; j < frontier && nodes[j]; j++) {
        // higher j have higher ssd already, so they're likely
        // to yield a suboptimal next sample too
        range = (j < frontier / 2) ? 1 : 0;
        const int step = nodes[j]->step;
        if (version == AV_CODEC_ID_ADPCM_MS) {
          const int predictor = ((nodes[j]->sample1 * c->coeff1) +
                                 (nodes[j]->sample2 * c->coeff2)) /
                                64;
          const int div = (sample - predictor) / step;
          const int nmin = av_clip(div - range, -8, 6);
          const int nmax = av_clip(div + range, -7, 7);
          for (nidx = nmin; nidx <= nmax; nidx++) {
            nibble = nidx & 0xf;
            dec_sample = predictor + nidx * step;

            while (store_node(
                FFMAX(16, (ff_adpcm_AdaptationTable[nibble] * step) >> 8)));
          }
        } else if (version == AV_CODEC_ID_ADPCM_IMA_WAV ||
                   version == AV_CODEC_ID_ADPCM_IMA_QT ||
                   version == AV_CODEC_ID_ADPCM_IMA_AMV ||
                   version == AV_CODEC_ID_ADPCM_SWF) {
          loop_nodes(ff_adpcm_step_table[step],
                     av_clip(step + ff_adpcm_index_table[nibble], 0, 88));
        } else {  // AV_CODEC_ID_ADPCM_YAMAHA
          loop_nodes(step,
                     av_clip((step * ff_adpcm_yamaha_indexscale[nibble]) >> 8,
                             127, 24576));
        }
      }

      u = nodes;
      nodes = nodes_next;
      nodes_next = u;

      generation++;
      if (generation == 255) {
        memset(hash, 0xff, 65536 * sizeof(*hash));
        generation = 0;
      }

      // prevent overflow
      if (nodes[0]->ssd > (1 << 28)) {
        for (j = 1; j < frontier && nodes[j]; j++)
          nodes[j]->ssd -= nodes[0]->ssd;
        nodes[0]->ssd = 0;
      }

      // merge old paths to save memory
      if (i == froze + FREEZE_INTERVAL) {
        p = &paths[nodes[0]->path];
        for (k = i; k > froze; k--) {
          dst[k] = p->nibble;
          p = &paths[p->prev];
        }
        froze = i;
        pathn = 0;
        // other nodes might use paths that don't coincide with the frozen one.
        // checking which nodes do so is too slow, so just kill them all.
        // this also slightly improves quality, but I don't know why.
        memset(nodes + 1, 0, (frontier - 1) * sizeof(TrellisNode *));
      }
    }

    p = &paths[nodes[0]->path];
    for (i = n - 1; i > froze; i--) {
      dst[i] = p->nibble;
      p = &paths[p->prev];
    }

    c->predictor = nodes[0]->sample1;
    c->sample1 = nodes[0]->sample1;
    c->sample2 = nodes[0]->sample2;
    c->step_index = nodes[0]->step;
    c->step = nodes[0]->step;
    c->idelta = nodes[0]->step;
  }

 protected:
  int frontier;
  int version;
  TrellisPath *paths, *p;
  TrellisNode *node_buf;
  TrellisNode **nodep_buf;
  TrellisNode **nodes;  // nodes[] is always sorted by .ssd
  TrellisNode **nodes_next;
  TrellisNode **u;
  TrellisNode *t;

  int pathn = 0, froze = -1, i, j, k, generation = 0;
  uint8_t *hash;
  int dec_sample = 0;
  int sample;
  int nibble;
  int nidx;
  int range;
  int heap_pos;
};

class EncoderADPCM_IMA_WAV : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_IMA_WAV() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_WAV);
    sample_formats.push_back(AV_SAMPLE_FMT_S16P);
  }
  int adpcm_encode_init_impl() {
    /* each 16 bits sample gives one nibble
       and we have 4 bytes per channel overhead */
    avctx.frame_size = (s->block_size - 4 * channels()) * 8 / (4 * channels()) + 1;
    /* seems frame_size isn't taken into account...
       have to buffer the samples :-( */
    avctx.block_align = s->block_size;
    avctx.bits_per_coded_sample = 4;
    return AV_OK;
  }
  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    int blocks = (frame->nb_samples - 1) / 8;

    for (int ch = 0; ch < channels(); ch++) {
      ADPCMChannelStatus *status = &c->status[ch];
      status->prev_sample = samples_p[ch][0];
      /* status->step_index = 0;
         XXX: not sure how to init the state machine */
      bytestream_put_le16(&dst, status->prev_sample);
      *dst++ = status->step_index;
      *dst++ = 0; /* unknown */
    }

    /* stereo: 4 bytes (8 samples) for left, 4 bytes for right */
    if (avctx.trellis > 0) {
      uint8_t *buf;
      if (!FF_ALLOC_TYPED_ARRAY(uint8_t *, buf, channels() *blocks * 8))
        return AVERROR(AVERROR_MEMORY);
      for (int ch = 0; ch < channels(); ch++) {
        adpcm_compress_trellis(&samples_p[ch][1], buf + ch * blocks * 8,
                               &c->status[ch], blocks * 8, 1);
      }
      for (int i = 0; i < blocks; i++) {
        for (int ch = 0; ch < channels(); ch++) {
          uint8_t *buf1 = buf + ch * blocks * 8 + i * 8;
          for (int j = 0; j < 8; j += 2) *dst++ = buf1[j] | (buf1[j + 1] << 4);
        }
      }
      av_free(buf);
    } else {
      for (int i = 0; i < blocks; i++) {
        for (int ch = 0; ch < channels(); ch++) {
          ADPCMChannelStatus *status = &c->status[ch];
          const int16_t *smp = &samples_p[ch][1 + i * 8];
          for (int j = 0; j < 8; j += 2) {
            uint8_t v = adpcm_ima_compress_sample(status, smp[j]);
            v |= adpcm_ima_compress_sample(status, smp[j + 1]) << 4;
            *dst++ = v;
          }
        }
      }
    }
    return AV_OK;
  }
};

class EncoderADPCM_IMA_QT : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_IMA_QT() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_QT);
    sample_formats.push_back(AV_SAMPLE_FMT_S16P);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = 64 ;
    avctx.block_align = 34 * channels(); /* End of CASE */
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    for (int ch = 0; ch < channels(); ch++) {
      ADPCMChannelStatus *status = &c->status[ch];
      put_bits(&pb, 9, (status->prev_sample & 0xFFFF) >> 7);
      put_bits(&pb, 7, status->step_index);
      if (avctx.trellis > 0) {
        uint8_t buf[64];
        adpcm_compress_trellis(&samples_p[ch][0], buf, status, 64, 1);
        for (int i = 0; i < 64; i++) put_bits(&pb, 4, buf[i ^ 1]);
        status->prev_sample = status->predictor;
      } else {
        for (int i = 0; i < 64; i += 2) {
          int t1, t2;
          t1 = adpcm_ima_qt_compress_sample(status, samples_p[ch][i]);
          t2 = adpcm_ima_qt_compress_sample(status, samples_p[ch][i + 1]);
          put_bits(&pb, 4, t2);
          put_bits(&pb, 4, t1);
        }
      }
    }

    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_IMA_SSI : public ADPCMEncoder {
 public:
  EncoderADPCM_IMA_SSI() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_SSI);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = s->block_size * 2 / channels();
    avctx.block_align = s->block_size;
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    av_assert(avctx.trellis == 0);

    for (int i = 0; i < frame->nb_samples; i++) {
      for (int ch = 0; ch < channels(); ch++) {
        put_bits(&pb, 4,
                 adpcm_ima_qt_compress_sample(c->status + ch, *samples++));
      }
    }

    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_IMA_ALP : public ADPCMEncoder {
 public:
  EncoderADPCM_IMA_ALP() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_ALP);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = s->block_size * 2 / channels();
    avctx.block_align = s->block_size;
    return AV_OK;
  }

  inline uint8_t adpcm_ima_alp_compress_sample(ADPCMChannelStatus *c,
                                               int16_t sample) {
    const int delta = sample - c->prev_sample;
    const int step = ff_adpcm_step_table[c->step_index];
    const int sign = (delta < 0) * 8;

    int nibble = FFMIN(abs(delta) * 4 / step, 7);
    int diff = (step * nibble) >> 2;
    if (sign) diff = -diff;

    nibble = sign | nibble;

    c->prev_sample += diff;
    c->prev_sample = av_clip_int16(c->prev_sample);
    c->step_index =
        av_clip(c->step_index + ff_adpcm_index_table[nibble], 0, 88);
    return nibble;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    av_assert(avctx.trellis == 0);

    for (int n = frame->nb_samples / 2; n > 0; n--) {
      for (int ch = 0; ch < channels(); ch++) {
        put_bits(&pb, 4,
                 adpcm_ima_alp_compress_sample(c->status + ch, *samples++));
        put_bits(&pb, 4,
                 adpcm_ima_alp_compress_sample(c->status + ch, samples[st]));
      }
      samples += channels();
    }

    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_MS : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_MS() {
    setCodecID(AV_CODEC_ID_ADPCM_MS);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    uint8_t *extradata;
    /* each 16 bits sample gives one nibble
       and we have 7 bytes per channel overhead */
    avctx.frame_size = (s->block_size - 7 * channels()) * 2 / channels() + 2;
    avctx.bits_per_coded_sample = 4;
    avctx.block_align = s->block_size;
    if (!(avctx.extradata =
              (uint8_t *)av_malloc(32 + AV_INPUT_BUFFER_PADDING_SIZE)))
      return AVERROR(AVERROR_MEMORY);
    avctx.extradata_size = 32;
    extradata = avctx.extradata;
    bytestream_put_le16(&extradata, avctx.frame_size);
    bytestream_put_le16(&extradata, 7); /* wNumCoef */
    for (int i = 0; i < 7; i++) {
      bytestream_put_le16(&extradata, ff_adpcm_AdaptCoeff1[i] * 4);
      bytestream_put_le16(&extradata, ff_adpcm_AdaptCoeff2[i] * 4);
    }
    return AV_OK;
  }

  inline uint8_t adpcm_ms_compress_sample(ADPCMChannelStatus *c,
                                          int16_t sample) {
    int predictor, nibble, bias;

    predictor =
        (((c->sample1) * (c->coeff1)) + ((c->sample2) * (c->coeff2))) / 64;

    nibble = sample - predictor;
    if (nibble >= 0)
      bias = c->idelta / 2;
    else
      bias = -c->idelta / 2;

    nibble = (nibble + bias) / c->idelta;
    nibble = av_clip_intp2(nibble, 3) & 0x0F;

    predictor += ((nibble & 0x08) ? (nibble - 0x10) : nibble) * c->idelta;

    c->sample2 = c->sample1;
    c->sample1 = av_clip_int16(predictor);

    c->idelta = (ff_adpcm_AdaptationTable[nibble] * c->idelta) >> 8;
    if (c->idelta < 16) c->idelta = 16;

    return nibble;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    for (int i = 0; i < channels(); i++) {
      int predictor = 0;
      *dst++ = predictor;
      c->status[i].coeff1 = ff_adpcm_AdaptCoeff1[predictor];
      c->status[i].coeff2 = ff_adpcm_AdaptCoeff2[predictor];
    }
    for (int i = 0; i < channels(); i++) {
      if (c->status[i].idelta < 16) c->status[i].idelta = 16;
      bytestream_put_le16(&dst, c->status[i].idelta);
    }
    for (int i = 0; i < channels(); i++) c->status[i].sample2 = *samples++;
    for (int i = 0; i < channels(); i++) {
      c->status[i].sample1 = *samples++;
      bytestream_put_le16(&dst, c->status[i].sample1);
    }
    for (int i = 0; i < channels(); i++)
      bytestream_put_le16(&dst, c->status[i].sample2);

    if (avctx.trellis > 0) {
      const int n = avctx.block_align - 7 * channels();
      uint8_t *buf = (uint8_t *)av_malloc(2 * n);
      if (!buf) return AVERROR(AVERROR_MEMORY);
      if (channels() == 1) {
        adpcm_compress_trellis(samples, buf, &c->status[0], n, channels());
        for (int i = 0; i < n; i += 2) *dst++ = (buf[i] << 4) | buf[i + 1];
      } else {
        adpcm_compress_trellis(samples, buf, &c->status[0], n, channels());
        adpcm_compress_trellis(samples + 1, buf + n, &c->status[1], n,
                               channels());
        for (int i = 0; i < n; i++) *dst++ = (buf[i] << 4) | buf[n + i];
      }
      av_free(buf);
    } else {
      for (int i = 7 * channels(); i < avctx.block_align; i++) {
        int nibble;
        nibble = adpcm_ms_compress_sample(&c->status[0], *samples++) << 4;
        nibble |= adpcm_ms_compress_sample(&c->status[st], *samples++);
        *dst++ = nibble;
      }
    } /* End of CASE */
    return AV_OK;
  }
};

class EncoderADPCM_SWF : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_SWF() {
    setCodecID(AV_CODEC_ID_ADPCM_SWF);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    if (avctx.sample_rate != 11025 && avctx.sample_rate != 22050 &&
        avctx.sample_rate != 44100) {
      av_log(avctx, AV_LOG_ERROR,
             "Sample rate must be 11025, "
             "22050 or 44100\n");
      return AVERROR(AVERROR_INVALID);
    }
    avctx.frame_size = 4096; /* Hardcoded according to the SWF spec. */
    avctx.block_align =
        (2 + channels() * (22 + 4 * (avctx.frame_size - 1)) + 7) / 8;
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    const int n = frame->nb_samples - 1;
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    /* NB: This is safe as we don't have AV_CODEC_CAP_SMALL_LAST_FRAME. */
    av_assert(n == 4095);

    // store AdpcmCodeSize
    put_bits(&pb, 2, 2);  // set 4-bit flash adpcm format

    // init the encoder state
    for (int i = 0; i < channels(); i++) {
      // clip step so it fits 6 bits
      c->status[i].step_index = av_clip_uintp2(c->status[i].step_index, 6);
      put_sbits(&pb, 16, samples[i]);
      put_bits(&pb, 6, c->status[i].step_index);
      c->status[i].prev_sample = samples[i];
    }

    if (avctx.trellis > 0) {
      uint8_t buf[8190 /* = 2 * n */];
      adpcm_compress_trellis(samples + channels(), buf, &c->status[0], n,
                             channels());
      if (channels() == 2)
        adpcm_compress_trellis(samples + channels() + 1, buf + n, &c->status[1],
                               n, channels());
      for (int i = 0; i < n; i++) {
        put_bits(&pb, 4, buf[i]);
        if (channels() == 2) put_bits(&pb, 4, buf[n + i]);
      }
    } else {
      for (int i = 1; i < frame->nb_samples; i++) {
        put_bits(
            &pb, 4,
            adpcm_ima_compress_sample(&c->status[0], samples[channels() * i]));
        if (channels() == 2)
          put_bits(
              &pb, 4,
              adpcm_ima_compress_sample(&c->status[1], samples[2 * i + 1]));
      }
    }
    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_YAMAHA : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_YAMAHA() {
    setCodecID(AV_CODEC_ID_ADPCM_YAMAHA);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = s->block_size * 2 / channels();
    avctx.block_align = s->block_size;
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    int n = frame->nb_samples / 2;
    if (avctx.trellis > 0) {
      uint8_t *buf = (uint8_t *)av_malloc(2 * n * 2);
      if (!buf) return AVERROR(AVERROR_MEMORY);
      n *= 2;
      if (channels() == 1) {
        adpcm_compress_trellis(samples, buf, &c->status[0], n, channels());
        for (int i = 0; i < n; i += 2) *dst++ = buf[i] | (buf[i + 1] << 4);
      } else {
        adpcm_compress_trellis(samples, buf, &c->status[0], n, channels());
        adpcm_compress_trellis(samples + 1, buf + n, &c->status[1], n,
                               channels());
        for (int i = 0; i < n; i++) *dst++ = buf[i] | (buf[n + i] << 4);
      }
      av_free(buf);
    } else
      for (n *= channels(); n > 0; n--) {
        int nibble;
        nibble = adpcm_yamaha_compress_sample(&c->status[0], *samples++);
        nibble |= adpcm_yamaha_compress_sample(&c->status[st], *samples++) << 4;
        *dst++ = nibble;
      }
    return AV_OK;
  }
};

class EncoderADPCM_IMA_APM : public ADPCMEncoder {
 public:
  EncoderADPCM_IMA_APM() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_APM);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = s->block_size * 2 / channels();
    avctx.block_align = s->block_size;

    if (!(avctx.extradata =
              (uint8_t *)av_mallocz(28 + AV_INPUT_BUFFER_PADDING_SIZE)))
      return AVERROR(AVERROR_MEMORY);
    avctx.extradata_size = 28;
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    av_assert(avctx.trellis == 0);

    for (int n = frame->nb_samples / 2; n > 0; n--) {
      for (int ch = 0; ch < channels(); ch++) {
        put_bits(&pb, 4,
                 adpcm_ima_qt_compress_sample(c->status + ch, *samples++));
        put_bits(&pb, 4,
                 adpcm_ima_qt_compress_sample(c->status + ch, samples[st]));
      }
      samples += channels();
    }

    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_IMA_AMV : public ADPCMEncoderTrellis {
 public:
  EncoderADPCM_IMA_AMV() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_AMV);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    if (avctx.sample_rate != 22050) {
      av_log(avctx, AV_LOG_ERROR, "Sample rate must be 22050\n");
      return AVERROR(AVERROR_INVALID);
    }

    if (channels() != 1) {
      av_log(avctx, AV_LOG_ERROR, "Only mono is supported\n");
      return AVERROR(AVERROR_INVALID);
    }

    avctx.frame_size = s->block_size;
    avctx.block_align = 8 + (FFALIGN(avctx.frame_size, 2) / 2);
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    av_assert(channels() == 1);

    c->status[0].prev_sample = *samples;
    bytestream_put_le16(&dst, c->status[0].prev_sample);
    bytestream_put_byte(&dst, c->status[0].step_index);
    bytestream_put_byte(&dst, 0);
    bytestream_put_le32(&dst, avctx.frame_size);

    if (avctx.trellis > 0) {
      const int n = frame->nb_samples >> 1;
      uint8_t *buf = (uint8_t *)av_malloc(2 * n);

      if (!buf) return AVERROR(AVERROR_MEMORY);

      adpcm_compress_trellis(samples, buf, &c->status[0], 2 * n, channels());
      for (int i = 0; i < n; i++)
        bytestream_put_byte(&dst, (buf[2 * i] << 4) | buf[2 * i + 1]);

      samples += 2 * n;
      av_free(buf);
    } else
      for (int n = frame->nb_samples >> 1; n > 0; n--) {
        int nibble;
        nibble = adpcm_ima_compress_sample(&c->status[0], *samples++) << 4;
        nibble |= adpcm_ima_compress_sample(&c->status[0], *samples++) & 0x0F;
        bytestream_put_byte(&dst, nibble);
      }

    if (avctx.frame_size & 1) {
      int nibble = adpcm_ima_compress_sample(&c->status[0], *samples++) << 4;
      bytestream_put_byte(&dst, nibble);
    }
    return AV_OK;
  }
};

class EncoderADPCM_ARGO : public ADPCMEncoder {
 public:
  EncoderADPCM_ARGO() {
    setCodecID(AV_CODEC_ID_ADPCM_ARGO);
    sample_formats.push_back(AV_SAMPLE_FMT_S16P);
  }
  int adpcm_encode_init_impl() {
    avctx.frame_size = 32;
    avctx.block_align = 17 * channels();
    return AV_OK;
  }

  int adpcm_argo_compress_nibble(const ADPCMChannelStatus *cs, int16_t s,
                                 int shift, int flag) {
    int nibble;

    if (flag)
      nibble = 4 * s - 8 * cs->sample1 + 4 * cs->sample2;
    else
      nibble = 4 * s - 4 * cs->sample1;

    return (nibble >> shift) & 0x0F;
  }

  int16_t ff_adpcm_argo_expand_nibble(ADPCMChannelStatus *cs, int nibble,
                                      int shift, int flag) {
    int sample = sign_extend(nibble, 4) * (1 << shift);

    if (flag)
      sample += (8 * cs->sample1) - (4 * cs->sample2);
    else
      sample += 4 * cs->sample1;

    sample = av_clip_int16(sample >> 2);

    cs->sample2 = cs->sample1;
    cs->sample1 = sample;

    return sample;
  }

  int64_t adpcm_argo_compress_block(ADPCMChannelStatus *cs, PutBitContext *pb,
                                    const int16_t *samples, int nsamples,
                                    int shift, int flag) {
    int64_t error = 0;

    if (pb) {
      put_bits(pb, 4, shift - 2);
      put_bits(pb, 1, 0);
      put_bits(pb, 1, !!flag);
      put_bits(pb, 2, 0);
    }

    for (int n = 0; n < nsamples; n++) {
      /* Compress the nibble, then expand it to see how much precision we've
       * lost. */
      int nibble = adpcm_argo_compress_nibble(cs, samples[n], shift, flag);
      int16_t sample = ff_adpcm_argo_expand_nibble(cs, nibble, shift, flag);

      error += abs(samples[n] - sample);

      if (pb) put_bits(pb, 4, nibble);
    }

    return error;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    av_assert(frame->nb_samples == 32);

    for (int ch = 0; ch < channels(); ch++) {
      int64_t error = INT64_MAX, tmperr = INT64_MAX;
      int shift = 2, flag = 0;
      int saved1 = c->status[ch].sample1;
      int saved2 = c->status[ch].sample2;

      /* Find the optimal coefficients, bail early if we find a perfect
       * result. */
      for (int s = 2; s < 18 && tmperr != 0; s++) {
        for (int f = 0; f < 2 && tmperr != 0; f++) {
          c->status[ch].sample1 = saved1;
          c->status[ch].sample2 = saved2;
          tmperr = adpcm_argo_compress_block(
              c->status + ch, NULL, samples_p[ch], frame->nb_samples, s, f);
          if (tmperr < error) {
            shift = s;
            flag = f;
            error = tmperr;
          }
        }
      }

      /* Now actually do the encode. */
      c->status[ch].sample1 = saved1;
      c->status[ch].sample2 = saved2;
      adpcm_argo_compress_block(c->status + ch, &pb, samples_p[ch],
                                frame->nb_samples, shift, flag);
    }

    flush_put_bits(&pb);
    return AV_OK;
  }
};

class EncoderADPCM_IMA_WS : public ADPCMEncoder {
 public:
  EncoderADPCM_IMA_WS() {
    setCodecID(AV_CODEC_ID_ADPCM_IMA_WS);
    sample_formats.push_back(AV_SAMPLE_FMT_S16);
  }
  int adpcm_encode_init_impl() {
    /* each 16 bits sample gives one nibble */
    avctx.frame_size = s->block_size * 2 / channels();
    avctx.block_align = s->block_size;
    return AV_OK;
  }

  int adpcm_encode_frame_impl(AVPacket *avpkt, const AVFrame *frame,
                              int *got_packet_ptr) {
    PutBitContext pb;
    init_put_bits(&pb, dst, pkt_size);

    av_assert(avctx.trellis == 0);
    for (int n = frame->nb_samples / 2; n > 0; n--) {
      /* stereo: 1 byte (2 samples) for left, 1 byte for right */
      for (int ch = 0; ch < channels(); ch++) {
        int t1, t2;
        t1 = adpcm_ima_compress_sample(&c->status[ch], *samples++);
        t2 = adpcm_ima_compress_sample(&c->status[ch], samples[st]);
        put_bits(&pb, 4, t2);
        put_bits(&pb, 4, t1);
      }
      samples += channels();
    }
    flush_put_bits(&pb);
    return AV_OK;
  }
};

class ADPCMEncoderFactory {
 public:
  static ADPCMEncoder *create(AVCodecID id) {
    switch (id) {
      case AV_CODEC_ID_ADPCM_IMA_WAV:
        return new EncoderADPCM_IMA_WAV();
#if ENABLE_BROKEN_CODECS
      case AV_CODEC_ID_ADPCM_IMA_QT:
        return new EncoderADPCM_IMA_QT();
#endif
      case AV_CODEC_ID_ADPCM_IMA_SSI:
        return new EncoderADPCM_IMA_SSI();
      case AV_CODEC_ID_ADPCM_IMA_ALP:
        return new EncoderADPCM_IMA_ALP();
      case AV_CODEC_ID_ADPCM_MS:
        return new EncoderADPCM_MS();
      case AV_CODEC_ID_ADPCM_SWF:
        return new EncoderADPCM_SWF();
      case AV_CODEC_ID_ADPCM_YAMAHA:
        return new EncoderADPCM_YAMAHA();
      case AV_CODEC_ID_ADPCM_IMA_APM:
        return new EncoderADPCM_IMA_APM();
      case AV_CODEC_ID_ADPCM_IMA_AMV:
        return new EncoderADPCM_IMA_AMV();
      case AV_CODEC_ID_ADPCM_ARGO:
        return new EncoderADPCM_ARGO();
      case AV_CODEC_ID_ADPCM_IMA_WS:
        return new EncoderADPCM_IMA_WS();

      default:
        av_log(avctx, AV_LOG_ERROR, "ERROR: encoder [%d] not implemented\n", id);
        return nullptr;
    };
  }
};

}  // namespace adpcm_ffmpeg