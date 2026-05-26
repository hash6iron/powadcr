#pragma once

#define av_cold
#define av_always_inline inline
#define av_const const
#define av_unused
#define av_alias

#define AV_HAVE_BIGENDIAN 0
#define CACHED_BITSTREAM_READER 0
#define BITSTREAM_READER_LE 1
#define BITSTREAM_WRITER_LE 1

/**
 * @ingroup lavc_decoding
 * Required number of additionally allocated bytes at the end of the input
 * bitstream for decoding. This is mainly needed because some optimized
 * bitstream readers read 32 or 64 bit at once and could read over the end.<br>
 * Note: If the first 23 bits of the additional bytes are not 0, then damaged
 * MPEG bitstreams could cause overread and segfault.
 */
#define AV_INPUT_BUFFER_PADDING_SIZE 64

/// The AV_CODEC_ID_ADPCM_IMA_QT seems to be broken, so we deactivate it
#define ENABLE_BROKEN_CODECS true

/// Automatic name space support
#define ADPCM_ADD_NAMESPACE false


