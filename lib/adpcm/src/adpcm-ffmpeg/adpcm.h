#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "config-adpcm.h"

// define some qulifiers used by the code base

#define av_assert(X) assert(X)

#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSIGN(a) ((a) > 0 ? 1 : -1)
#define FFMAX(a, b) ((a) > (b) ? (a) : (b))
#define FFMIN(a, b) ((a) > (b) ? (b) : (a))
#define FFSWAP(type, a, b) \
  do {                     \
    type SWAP_tmp = b;     \
    b = a;                 \
    a = SWAP_tmp;          \
  } while (0)
#define FFALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define AVERROR(X) X

// Some define used by logging
#define AV_LOG_ERROR 16
#define AV_LOG_WARNING 24
#define AV_LOG_INFO 32

#define av_log(A, B, ...) printf(__VA_ARGS__);

#define FF_ALLOC_TYPED_ARRAY(T, p, nelem) (p = (T) av_malloc_array(nelem, sizeof(*p)))
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define AV_NUM_DATA_POINTERS 8

/**
 * @brief Supported Codec IDs
 * @enum Supported Codec IDs
 */
enum AVCodecID {
  /* various ADPCM codecs */
  AV_CODEC_ID_ADPCM_IMA_QT = 0x11000,  ///< IMA_QT
  AV_CODEC_ID_ADPCM_IMA_WAV,           ///< IMA_WAV
  AV_CODEC_ID_ADPCM_IMA_DK3,           ///< DK3
  AV_CODEC_ID_ADPCM_IMA_DK4,           ///< DK4
  AV_CODEC_ID_ADPCM_IMA_WS,            ///< WS
  AV_CODEC_ID_ADPCM_IMA_SMJPEG,        ///< SMJPEG
  AV_CODEC_ID_ADPCM_MS,                ///< MS
  AV_CODEC_ID_ADPCM_4XM,               ///< 4XM
  AV_CODEC_ID_ADPCM_XA,                ///< XA
  AV_CODEC_ID_ADPCM_ADX,               ///< ADX
  AV_CODEC_ID_ADPCM_EA,                ///< EA
  AV_CODEC_ID_ADPCM_G726,              ///< G726
  AV_CODEC_ID_ADPCM_CT,                ///< CT
  AV_CODEC_ID_ADPCM_SWF,               ///< SWF
  AV_CODEC_ID_ADPCM_YAMAHA,            ///< YAHAMA
  AV_CODEC_ID_ADPCM_SBPRO_4,           ///< SBRO 4
  AV_CODEC_ID_ADPCM_SBPRO_3,           ///< SBRO 3
  AV_CODEC_ID_ADPCM_SBPRO_2,           ///< SBRO 2
  AV_CODEC_ID_ADPCM_THP,               ///< THP
  AV_CODEC_ID_ADPCM_IMA_AMV,           ///< AMV
  AV_CODEC_ID_ADPCM_EA_R1,             ///< R1
  AV_CODEC_ID_ADPCM_EA_R3,             ///< R3
  AV_CODEC_ID_ADPCM_EA_R2,             ///< R2
  AV_CODEC_ID_ADPCM_IMA_EA_SEAD,       ///< SEAD
  AV_CODEC_ID_ADPCM_IMA_EA_EACS,       ///< EACS
  AV_CODEC_ID_ADPCM_EA_XAS,            ///< XAS
  AV_CODEC_ID_ADPCM_EA_MAXIS_XA,       ///< MAXIS_XA
  AV_CODEC_ID_ADPCM_IMA_ISS,           ///< ISS
  AV_CODEC_ID_ADPCM_G722,              ///< G722
  AV_CODEC_ID_ADPCM_IMA_APC,           ///< APC
  AV_CODEC_ID_ADPCM_VIMA,              ///< VIMA
  AV_CODEC_ID_ADPCM_AFC,               ///< AFC
  AV_CODEC_ID_ADPCM_IMA_OKI,           ///< OKI
  AV_CODEC_ID_ADPCM_DTK,               ///< DTK
  AV_CODEC_ID_ADPCM_IMA_RAD,           ///< IMA_RAD
  AV_CODEC_ID_ADPCM_G726LE,            ///< G726
  AV_CODEC_ID_ADPCM_THP_LE,            ///< THP_LE
  AV_CODEC_ID_ADPCM_PSX,               ///< PSX
  AV_CODEC_ID_ADPCM_AICA,              ///< AICA
  AV_CODEC_ID_ADPCM_IMA_DAT4,          ///< DAT4
  AV_CODEC_ID_ADPCM_MTAF,              ///< MTAF
  AV_CODEC_ID_ADPCM_AGM,               ///< AGM
  AV_CODEC_ID_ADPCM_ARGO,              ///< ARGO
  AV_CODEC_ID_ADPCM_IMA_SSI,           ///< SSI
  AV_CODEC_ID_ADPCM_ZORK,              ///< ZORK
  AV_CODEC_ID_ADPCM_IMA_APM,           ///< APM
  AV_CODEC_ID_ADPCM_IMA_ALP,           ///< ALP
  AV_CODEC_ID_ADPCM_IMA_MTF,           ///< MTF
  AV_CODEC_ID_ADPCM_IMA_CUNNING,       ///< CUNNING
  AV_CODEC_ID_ADPCM_IMA_MOFLEX,        ///< MOFLX
  AV_CODEC_ID_ADPCM_IMA_ACORN,         ///< ACORN
  AV_CODEC_ID_ADPCM_XMD,               ///< XMD
};

namespace adpcm_ffmpeg {

enum av_errors {
  AV_OK = 0,
  AVERROR_INVALIDDATA,
  AVERROR_PATCHWELCOME,
  AVERROR_INVALID,
  AVERROR_MEMORY,
};


enum AVChannelLayout {
  AV_CHANNEL_LAYOUT_MONO,    ///< mono: 1 channel
  AV_CHANNEL_LAYOUT_STEREO,  ///< stereo: 2 channels
};

enum AVSampleFormat {
  AV_SAMPLE_FMT_NONE = -1,
  AV_SAMPLE_FMT_U8,   ///< unsigned 8 bits
  AV_SAMPLE_FMT_S16,  ///< signed 16 bits
  AV_SAMPLE_FMT_S32,  ///< signed 32 bits
  AV_SAMPLE_FMT_FLT,  ///< float
  AV_SAMPLE_FMT_DBL,  ///< double

  AV_SAMPLE_FMT_U8P,   ///< unsigned 8 bits, planar
  AV_SAMPLE_FMT_S16P,  ///< signed 16 bits, planar
  AV_SAMPLE_FMT_S32P,  ///< signed 32 bits, planar
  AV_SAMPLE_FMT_FLTP,  ///< float, planar
  AV_SAMPLE_FMT_DBLP,  ///< double, planar
  AV_SAMPLE_FMT_S64,   ///< signed 64 bits
  AV_SAMPLE_FMT_S64P,  ///< signed 64 bits, planar

  AV_SAMPLE_FMT_NB  ///< Number of sample formats. DO NOT USE if linking
                    ///< dynamically
};

/**
 * @brief This structure stores compressed data. It is typically exported by
 * demuxers and then passed as input to decoders, or received as output from
 * encoders and then passed to muxers.
 *
 */
struct AVPacket {
  uint8_t *data;
  int size;
};

/**
 * @brief This structure provides the uncompressed PCM data
 */
struct AVFrame {
  /**
   * pointer to the picture/channel planes.
   * This might be different from the first allocated byte
   *
   * Some decoders access areas outside 0,0 - width,height, please
   * see avcodec_align_dimensions2(). Some filters and swscale can read
   * up to 16 bytes beyond the planes, if these filters are to be used,
   * then 16 extra bytes must be allocated.
   *
   * NOTE: Except for hwaccel formats, pointers not needed by the format
   * MUST be set to NULL.
   */
  uint8_t *data[AV_NUM_DATA_POINTERS];
  /**
   * number of audio samples (per channel) described by this frame
   */

  int nb_samples;
  /**
   * pointers to the data planes/channels.
   *
   * For video, this should simply point to data[].
   *
   * For planar audio, each channel has a separate data pointer, and
   * linesize[0] contains the size of each channel buffer.
   * For packed audio, there is just one data pointer, and linesize[0]
   * contains the total size of the buffer for all channels.
   *
   * Note: Both data and extended_data should always be set in a valid frame,
   * but for planar audio with more channels that can fit in data,
   * extended_data must be used in order to access all channels.
   */

  int16_t **extended_data;
};

struct TrellisPath {
  int nibble;
  int prev;
};

struct TrellisNode {
  uint32_t ssd;
  int path;
  int sample1;
  int sample2;
  int step;
};

/**
 * @file compat_public.h
 * public data structures
 */

struct ADPCMChannelStatus {
  int predictor;
  int16_t step_index;
  int step;
  /* for encoding */
  int prev_sample;

  /* MS version */
  int sample1;
  int sample2;
  int coeff1;
  int coeff2;
  int idelta;
};

struct ADPCMEncodeContext {
  // AVClass *class;
  int block_size;

  ADPCMChannelStatus status[6];
  TrellisPath *paths;
  TrellisNode *node_buf;
  TrellisNode **nodep_buf;
  uint8_t *trellis_hash;
};

/**
 * @brief C API context for encoder and decoder
 */

struct AVCodecContext {
  int trellis;
  /// @brief Number of samples per channel in an audio frame.
  int frame_size;
  /// @brief number of bytes per packet if constant and known or 0 Used by some
  /// WAV based audio codecs.
  int block_align;
  uint8_t *extradata;
  int extradata_size;
  //  Codec *codec;
  enum AVCodecID codec_id;
  void *priv_data;
  int nb_channels;
  // bits per sample/pixel from the demuxer
  int bits_per_coded_sample;
  int sample_rate;
  int sample_fmt;
};

struct ADPCMDecodeContext {
  ADPCMChannelStatus status[14];
  int vqa_version; /**< VQA version. Used for ADPCM_IMA_WS */
  int has_status;  /**< Status flag. Reset to 0 after a flush. */
};

static const uint8_t ff_adpcm_ima_block_sizes[4] = {4, 12, 4, 20};
static const uint8_t ff_adpcm_ima_block_samples[4] = {16, 32, 8, 32};

/* ff_adpcm_step_table[] and ff_adpcm_index_table[] are from the ADPCM
   reference source */
static const int8_t ff_adpcm_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8,
};

/**
 * This is the step table. Note that many programs use slight deviations from
 * this table, but such deviations are negligible:
 */
static const int16_t ff_adpcm_step_table[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
    19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
    876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

/* These are for MS-ADPCM */
/* ff_adpcm_AdaptationTable[], ff_adpcm_AdaptCoeff1[], and
   ff_adpcm_AdaptCoeff2[] are from libsndfile */
static const int16_t ff_adpcm_AdaptationTable[] = {230, 230, 230, 230, 307, 409,
                                                   512, 614, 768, 614, 512, 409,
                                                   307, 230, 230, 230};

/** Divided by 4 to fit in 8-bit integers */
static const uint8_t ff_adpcm_AdaptCoeff1[] = {64, 128, 0, 48, 60, 115, 98};

/** Divided by 4 to fit in 8-bit integers */
static const int8_t ff_adpcm_AdaptCoeff2[] = {0, -64, 0, 16, 0, -52, -58};

static const int16_t ff_adpcm_yamaha_indexscale[] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    230, 230, 230, 230, 307, 409, 512, 614};

static const int8_t ff_adpcm_yamaha_difflookup[] = {
    1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15};

/* These are for CD-ROM XA ADPCM */
static const int8_t xa_adpcm_table[5][2] = {
    {0, 0}, {60, 0}, {115, -52}, {98, -55}, {122, -60}};

static const int16_t afc_coeffs[2][16] = {
    {0, 2048, 0, 1024, 4096, 3584, 3072, 4608, 4200, 4800, 5120, 2048, 1024,
     -1024, -1024, -2048},
    {0, 0, 2048, 1024, -2048, -1536, -1024, -2560, -2248, -2300, -3072, -2048,
     -1024, 1024, 0, 0}};

static const int16_t ea_adpcm_table[] = {0,    240, 460, 392, 0,  0, -208,
                                         -220, 0,   1,   3,   4,  7, 8,
                                         10,   11,  0,   -1,  -3, -4};

/*
 * Dumped from the binaries:
 * - FantasticJourney.exe - 0x794D2, DGROUP:0x47A4D2
 * - BigRaceUSA.exe       - 0x9B8AA, DGROUP:0x49C4AA
 * - Timeshock!.exe       - 0x8506A, DGROUP:0x485C6A
 */
static const int8_t ima_cunning_index_table[9] = {-1, -1, -1, -1, 1,
                                                  2,  3,  4,  -1};

/*
 * Dumped from the binaries:
 * - FantasticJourney.exe - 0x79458, DGROUP:0x47A458
 * - BigRaceUSA.exe       - 0x9B830, DGROUP:0x49C430
 * - Timeshock!.exe       - 0x84FF0, DGROUP:0x485BF0
 */
static const int16_t ima_cunning_step_table[61] = {
    1,     1,     1,     1,     2,     2,    3,    3,    4,    5,     6,
    7,     8,     10,    12,    14,    16,   20,   24,   28,   32,    40,
    48,    56,    64,    80,    96,    112,  128,  160,  192,  224,   256,
    320,   384,   448,   512,   640,   768,  896,  1024, 1280, 1536,  1792,
    2048,  2560,  3072,  3584,  4096,  5120, 6144, 7168, 8192, 10240, 12288,
    14336, 16384, 20480, 24576, 28672, 0};

static const int8_t adpcm_index_table2[4] = {
    -1,
    2,
    -1,
    2,
};

static const int8_t adpcm_index_table3[8] = {
    -1, -1, 1, 2, -1, -1, 1, 2,
};

static const int8_t adpcm_index_table5[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16,
    -1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16,
};

static const int8_t *const adpcm_index_tables[4] = {
    &adpcm_index_table2[0],
    &adpcm_index_table3[0],
    &ff_adpcm_index_table[0],
    &adpcm_index_table5[0],
};

static const int16_t mtaf_stepsize[32][16] = {
    {
        1,
        5,
        9,
        13,
        16,
        20,
        24,
        28,
        -1,
        -5,
        -9,
        -13,
        -16,
        -20,
        -24,
        -28,
    },
    {
        2,
        6,
        11,
        15,
        20,
        24,
        29,
        33,
        -2,
        -6,
        -11,
        -15,
        -20,
        -24,
        -29,
        -33,
    },
    {
        2,
        7,
        13,
        18,
        23,
        28,
        34,
        39,
        -2,
        -7,
        -13,
        -18,
        -23,
        -28,
        -34,
        -39,
    },
    {
        3,
        9,
        15,
        21,
        28,
        34,
        40,
        46,
        -3,
        -9,
        -15,
        -21,
        -28,
        -34,
        -40,
        -46,
    },
    {
        3,
        11,
        18,
        26,
        33,
        41,
        48,
        56,
        -3,
        -11,
        -18,
        -26,
        -33,
        -41,
        -48,
        -56,
    },
    {
        4,
        13,
        22,
        31,
        40,
        49,
        58,
        67,
        -4,
        -13,
        -22,
        -31,
        -40,
        -49,
        -58,
        -67,
    },
    {
        5,
        16,
        26,
        37,
        48,
        59,
        69,
        80,
        -5,
        -16,
        -26,
        -37,
        -48,
        -59,
        -69,
        -80,
    },
    {
        6,
        19,
        31,
        44,
        57,
        70,
        82,
        95,
        -6,
        -19,
        -31,
        -44,
        -57,
        -70,
        -82,
        -95,
    },
    {
        7,
        22,
        38,
        53,
        68,
        83,
        99,
        114,
        -7,
        -22,
        -38,
        -53,
        -68,
        -83,
        -99,
        -114,
    },
    {
        9,
        27,
        45,
        63,
        81,
        99,
        117,
        135,
        -9,
        -27,
        -45,
        -63,
        -81,
        -99,
        -117,
        -135,
    },
    {
        10,
        32,
        53,
        75,
        96,
        118,
        139,
        161,
        -10,
        -32,
        -53,
        -75,
        -96,
        -118,
        -139,
        -161,
    },
    {
        12,
        38,
        64,
        90,
        115,
        141,
        167,
        193,
        -12,
        -38,
        -64,
        -90,
        -115,
        -141,
        -167,
        -193,
    },
    {
        15,
        45,
        76,
        106,
        137,
        167,
        198,
        228,
        -15,
        -45,
        -76,
        -106,
        -137,
        -167,
        -198,
        -228,
    },
    {
        18,
        54,
        91,
        127,
        164,
        200,
        237,
        273,
        -18,
        -54,
        -91,
        -127,
        -164,
        -200,
        -237,
        -273,
    },
    {
        21,
        65,
        108,
        152,
        195,
        239,
        282,
        326,
        -21,
        -65,
        -108,
        -152,
        -195,
        -239,
        -282,
        -326,
    },
    {
        25,
        77,
        129,
        181,
        232,
        284,
        336,
        388,
        -25,
        -77,
        -129,
        -181,
        -232,
        -284,
        -336,
        -388,
    },
    {
        30,
        92,
        153,
        215,
        276,
        338,
        399,
        461,
        -30,
        -92,
        -153,
        -215,
        -276,
        -338,
        -399,
        -461,
    },
    {
        36,
        109,
        183,
        256,
        329,
        402,
        476,
        549,
        -36,
        -109,
        -183,
        -256,
        -329,
        -402,
        -476,
        -549,
    },
    {
        43,
        130,
        218,
        305,
        392,
        479,
        567,
        654,
        -43,
        -130,
        -218,
        -305,
        -392,
        -479,
        -567,
        -654,
    },
    {
        52,
        156,
        260,
        364,
        468,
        572,
        676,
        780,
        -52,
        -156,
        -260,
        -364,
        -468,
        -572,
        -676,
        -780,
    },
    {
        62,
        186,
        310,
        434,
        558,
        682,
        806,
        930,
        -62,
        -186,
        -310,
        -434,
        -558,
        -682,
        -806,
        -930,
    },
    {
        73,
        221,
        368,
        516,
        663,
        811,
        958,
        1106,
        -73,
        -221,
        -368,
        -516,
        -663,
        -811,
        -958,
        -1106,
    },
    {
        87,
        263,
        439,
        615,
        790,
        966,
        1142,
        1318,
        -87,
        -263,
        -439,
        -615,
        -790,
        -966,
        -1142,
        -1318,
    },
    {
        104,
        314,
        523,
        733,
        942,
        1152,
        1361,
        1571,
        -104,
        -314,
        -523,
        -733,
        -942,
        -1152,
        -1361,
        -1571,
    },
    {
        124,
        374,
        623,
        873,
        1122,
        1372,
        1621,
        1871,
        -124,
        -374,
        -623,
        -873,
        -1122,
        -1372,
        -1621,
        -1871,
    },
    {
        148,
        445,
        743,
        1040,
        1337,
        1634,
        1932,
        2229,
        -148,
        -445,
        -743,
        -1040,
        -1337,
        -1634,
        -1932,
        -2229,
    },
    {
        177,
        531,
        885,
        1239,
        1593,
        1947,
        2301,
        2655,
        -177,
        -531,
        -885,
        -1239,
        -1593,
        -1947,
        -2301,
        -2655,
    },
    {
        210,
        632,
        1053,
        1475,
        1896,
        2318,
        2739,
        3161,
        -210,
        -632,
        -1053,
        -1475,
        -1896,
        -2318,
        -2739,
        -3161,
    },
    {
        251,
        753,
        1255,
        1757,
        2260,
        2762,
        3264,
        3766,
        -251,
        -753,
        -1255,
        -1757,
        -2260,
        -2762,
        -3264,
        -3766,
    },
    {
        299,
        897,
        1495,
        2093,
        2692,
        3290,
        3888,
        4486,
        -299,
        -897,
        -1495,
        -2093,
        -2692,
        -3290,
        -3888,
        -4486,
    },
    {
        356,
        1068,
        1781,
        2493,
        3206,
        3918,
        4631,
        5343,
        -356,
        -1068,
        -1781,
        -2493,
        -3206,
        -3918,
        -4631,
        -5343,
    },
    {
        424,
        1273,
        2121,
        2970,
        3819,
        4668,
        5516,
        6365,
        -424,
        -1273,
        -2121,
        -2970,
        -3819,
        -4668,
        -5516,
        -6365,
    },
};

static const int16_t oki_step_table[49] = {
    16,  17,  19,  21,  23,  25,   28,   31,   34,   37,  41,  45,  50,
    55,  60,  66,  73,  80,  88,   97,   107,  118,  130, 143, 157, 173,
    190, 209, 230, 253, 279, 307,  337,  371,  408,  449, 494, 544, 598,
    658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552};

// padded to zero where table size is less then 16
static const int8_t swf_index_tables[4][16] = {
    /*2*/ {-1, 2},
    /*3*/ {-1, -1, 2, 4},
    /*4*/ {-1, -1, -1, -1, 2, 4, 6, 8},
    /*5*/ {-1, -1, -1, -1, -1, -1, -1, -1, 1, 2, 4, 6, 8, 10, 13, 16}};

static const int8_t zork_index_table[8] = {
    -1, -1, -1, 1, 4, 7, 10, 12,
};

static const int8_t mtf_index_table[16] = {
    8, 6, 4, 2, -1, -1, -1, -1, -1, -1, -1, -1, 2, 4, 6, 8,
};

/* end of tables */

av_always_inline av_const int av_clip(int a, int amin, int amax) {
  // #if defined(HAVE_AV_CONFIG_H) && defined(ASSERT_LEVEL) && ASSERT_LEVEL >=
  // 2
  assert(amin <= amax);
  // #endif
  if (a < amin)
    return amin;
  else if (a > amax)
    return amax;
  else
    return a;
}

void *av_malloc(size_t size) { return malloc(size); }

void av_free(void *ptr) {
#if HAVE_ALIGNED_MALLOC
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}

void av_freep(void *arg) {
  void *val;

  memcpy(&val, arg, sizeof(val));
  static void *const temp = NULL;
  memcpy(arg, &temp, sizeof(val));
  av_free(val);
}

void *av_mallocz(size_t size) {
  void *ptr = av_malloc(size);
  if (ptr) memset(ptr, 0, size);
  return ptr;
}

int size_mult(size_t a, size_t b, size_t *r) {
  size_t t;

  t = a * b;
  /* Hack inspired from glibc: don't try the division if nelem and elsize
   * are both less than sqrt(SIZE_MAX). */
  if ((a | b) >= ((size_t)1 << (sizeof(size_t) * 4)) && a && t / a != b)
    return AVERROR(AVERROR_INVALID);
  *r = t;
  return 0;
}

void *av_calloc(size_t nmemb, size_t size) {
  size_t result;
  if (size_mult(nmemb, size, &result) < 0) return NULL;
  return av_mallocz(result);
}

void *av_malloc_array(size_t nmemb, size_t size) {
  size_t result;
  if (size_mult(nmemb, size, &result) < 0) return NULL;
  return av_malloc(result);
}

/**
 * Clip a signed integer value into the 0-65535 range.
 * @param a value to clip
 * @return clipped value
 */
av_always_inline av_const uint16_t av_clip_uint16(int a) {
  if (a & (~0xFFFF))
    return (~a) >> 31;
  else
    return a;
}

/**
 * Clip a signed integer value into the -32768,32767 range.
 * @param a value to clip
 * @return clipped value
 */
av_always_inline av_const int16_t av_clip_int16(int a) {
  if ((a + 0x8000U) & ~0xFFFF)
    return (a >> 31) ^ 0x7FFF;
  else
    return a;
}

/**
 * Clip a signed integer into the -(2^p),(2^p-1) range.
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
av_always_inline av_const int av_clip_intp2(int a, int p) {
  if (((unsigned)a + (1 << p)) & ~((2 << p) - 1))
    return (a >> 31) ^ ((1 << p) - 1);
  else
    return a;
}

/**
 * Clip a signed integer to an unsigned power of two range.
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
av_always_inline av_const unsigned av_clip_uintp2(int a, int p) {
  if (a & ~((1 << p) - 1))
    return (~a) >> 31 & ((1 << p) - 1);
  else
    return a;
}

/**
 * Clear high bits from an unsigned integer starting with specific bit
 * position
 * @param  a value to clip
 * @param  p bit position to clip at
 * @return clipped value
 */
av_always_inline av_const unsigned av_mod_uintp2(unsigned a, unsigned p) {
  return a & ((1U << p) - 1);
}

#ifndef sign_extend
inline av_const int sign_extend(int val, unsigned bits) {
  unsigned shift = 8 * sizeof(int) - bits;
  union {
    unsigned u;
    int s;
  } v = {(unsigned)val << shift};
  return v.s >> shift;
}
#endif

#ifndef sign_extend64
inline av_const int64_t sign_extend64(int64_t val, unsigned bits) {
  unsigned shift = 8 * sizeof(int64_t) - bits;
  union {
    uint64_t u;
    int64_t s;
  } v = {(uint64_t)val << shift};
  return v.s >> shift;
}
#endif

#ifndef zero_extend
inline av_const unsigned zero_extend(unsigned val, unsigned bits) {
  return (val << ((8 * sizeof(int)) - bits)) >> ((8 * sizeof(int)) - bits);
}
#endif

inline uint32_t NEG_USR32(uint32_t a, int8_t s) {
  __asm__("shrl %1, %0\n\t" : "+r"(a) : "ic"((uint8_t)(-s)));
  return a;
}

}