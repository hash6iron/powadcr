/**
 * @file lame_log.h
 * @author Phil Schatzmann
 * @brief Simple Logging
 * @version 0.1
 * @date 2021-08-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "liblame/log.h"

// User Settings: Activate/Deactivate logging
#ifndef LAME_LOGGING_ACTIVE
#define LAME_LOGGING_ACTIVE true
#endif

#ifndef LAME_LOG_LEVEL
#define LAME_LOG_LEVEL LAMEInfo
#endif

// Logging Implementation
#if LAME_LOGGING_ACTIVE == true

enum LogLevelLame {LAMEDebug, LAMEInfo, LAMEWarning, LAMEError};
static int LOGLEVEL_LAME = LAMEWarning;

static const char* levelName(LogLevelLame level) {
    switch(level){
        case LAMEDebug:
            return "D";
        case LAMEInfo:
            return "I";
        case LAMEWarning:
            return "W";
        case LAMEError:
            return "E";
    }
    return "";
}

// We print the log based on the log level
#define LOG_LAME(level,...) { if(level>=LOGLEVEL_LAME) {  snprintf(log_msg, MAX_LOG_LEN, __VA_ARGS__);  print_log(__FILE__,__LINE__, levelName(level)); } }
#else
// Remove all log statments from the code
#define LOG_LAME(level, ...) 
#endif
