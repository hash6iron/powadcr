#pragma once

/**
 * AudioConfigLocal.h - PowaDCR Optimization
 * 
 * Configuración local para AudioTools library optimizada para PowaDCR.
 * NOTA: Este archivo se incluye ANTES que esp32.h en AudioToolsConfig.h
 * Por eso usamos #ifndef para permitir que esp32.h override después.
 * 
 * IMPACTO ESTIMADO: 50-80 KB FLASH savings
 */

// ============================================================================
// LOGGING - DISABLE to save 2-4 KB
// ============================================================================

// Disable AudioTools internal logging
#ifndef USE_AUDIO_LOGGING
#  define USE_AUDIO_LOGGING false
#endif

// Reduce logging buffer
#ifndef LOG_PRINTF_BUFFER_SIZE
#  define LOG_PRINTF_BUFFER_SIZE 128
#endif

// ============================================================================
// DECODERS - Only needed for PowaDCR
// ============================================================================

// We use: MP3, WAV, ADPCM, FLAC
// We DON'T use: OGG, AAC, OPUS

#ifndef USE_CODEC_OGG
#  define USE_CODEC_OGG false
#endif

#ifndef USE_CODEC_AAC  
#  define USE_CODEC_AAC false
#endif

#ifndef USE_CODEC_OPUS
#  define USE_CODEC_OPUS false
#endif

// ============================================================================
// I/O INTERFACES - NOT USED
// ============================================================================

// We don't use: ANALOG, PDM, PWM, TOUCH, A2DP
// Use #undef to prevent esp32.h from redefining these

#undef USE_ANALOG
#define USE_ANALOG false

#undef USE_PDM
#define USE_PDM false

#undef USE_PWM
#define USE_PWM false

#undef USE_TOUCH_READ
#define USE_TOUCH_READ false

#undef USE_A2DP
#define USE_A2DP false

// ============================================================================
// STREAMS - Only Volume needed
// ============================================================================

#ifndef USE_STREAM_EQUALIZER
#  define USE_STREAM_EQUALIZER false
#endif

#ifndef USE_STREAM_MIXER
#  define USE_STREAM_MIXER false
#endif

// ============================================================================
// NETWORKING - WiFi only for OTA, no Ethernet
// ============================================================================

#ifndef USE_ETHERNET
#  define USE_ETHERNET false
#endif

// ============================================================================
// OPTIMIZATION SUMMARY
// ============================================================================
/*
 * ESTIMATED FLASH SAVINGS:
 * 
 * Disabled feature               FLASH saved
 * ================================
 * USE_AUDIO_LOGGING=false        2-4 KB
 * LOG_PRINTF_BUFFER 303→128      0.2 KB
 * USE_CODEC_OGG=false            15-20 KB
 * USE_CODEC_AAC=false            20-30 KB
 * USE_CODEC_OPUS=false           10-15 KB
 * USE_A2DP=false                 5-10 KB
 * USE_ANALOG=false               2-4 KB
 * USE_PWM=false                  2-3 KB
 * USE_PDM=false                  3-5 KB
 * USE_TOUCH_READ=false           1-2 KB
 * USE_ETHERNET=false             5-8 KB
 * USE_STREAM_EQUALIZER=false     3-5 KB
 * USE_STREAM_MIXER=false         2-3 KB
 * ================================
 * TOTAL POTENTIAL: 70-120 KB
 * REALISTIC (with compiler optimization): 50-80 KB
 * 
 * CURRENT STATUS:
 * Before: 96.1% FLASH (1,952,481 / 2,031,616)
 * Expected after optimization: 93-94% FLASH
 */
