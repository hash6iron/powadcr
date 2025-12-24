#pragma once

// ==> New Config

// use precalculated log table
#ifndef USE_FAST_LOG
#define USE_FAST_LOG 1
#endif

// use precalculated log table as const -> in the ESP32 this will end up in flash memory
#ifndef USE_FAST_LOG_CONST
#define USE_FAST_LOG_CONST 1
#endif

// Avoid big memory allocations in replaygain_data
#ifndef USE_MEMORY_HACK
#define USE_MEMORY_HACK 0
#endif

// The stack on microcontrollers is very limited and we should avoid big objects on the stack in psymodel.c
#ifndef USE_STACK_HACK
#define USE_STACK_HACK 1
#endif

// If you know the encoder will be used in a single threaded environment, you can use this hack to just
// recycle the memory. This will prevent memory fragmentation. Only use this if you are sure that the
// encoder will be called from a single thread.
#ifndef USE_STACK_HACK_RECYCLE_ALLOCATION_SINGLE_THREADED
#define USE_STACK_HACK_RECYCLE_ALLOCATION_SINGLE_THREADED 1
#endif

// If the device is ESP32 and ESP_PSRAM_ENABLE_LIMIT is > 0, then the ESP32 will
// be configured to use allocate any allocation above ESP_PSRAM_ENABLE_LIMIT using
// psram, rather than scarce main memory.
#ifndef ESP_PSRAM_ENABLE_LIMIT
#define ESP_PSRAM_ENABLE_LIMIT 1024
#endif

// Not all microcontroller support vararg methods: alternative impelemtation of logging using the preprocessor
#ifndef USE_LOGGING_HACK
#define USE_LOGGING_HACK 1
#endif

// Print debug messages
#ifndef USE_DEBUG
#define USE_DEBUG 0
#endif

// Print memory allocation
#ifndef USE_DEBUG_ALLOC
#define USE_DEBUG_ALLOC 0
#endif

// ==> Standard Config

// deocode on the fly
#define DECODE_ON_THE_FLY 0

// optimization which only works with -fno-strict-alias
#define USE_HIRO_IEEE754_HACK 0

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 0

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 0

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 0

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

// disallow bcopy in string.h
#define _POSIX_C_SOURCE 200809L

// Includes dmalloc (debug malloc)
#define USE_DMALLOC 0

// removes depreciated methods
#define DEPRECATED_OR_OBSOLETE_CODE_REMOVED 1

// optimizations using NASM (Netwide Assembler (NASM), an asssembler for the x86 CPU)
#define HAVE_NASM 0

// support for MMX instruction set from Intel
#define HAVE_XMMINTRIN_H 0
