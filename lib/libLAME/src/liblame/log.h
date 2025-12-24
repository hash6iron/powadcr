#pragma once
#include "config.h"
#define MAX_LOG_LEN 512
#ifdef __cplusplus
extern "C" 
#endif
void print_log(const char* file, int line, const char* current_level);

extern char log_msg[MAX_LOG_LEN];


#if USE_LOGGING_HACK

    #define DEBUGF  lame_debugf 
    #define ERRORF  lame_errorf
    #define MSGF    lame_msgf

    #define lame_errorf(gfc, ...) {  snprintf(log_msg, 512, __VA_ARGS__);  print_log(__FILE__,__LINE__, "Error"); }
    #define lame_msgf(gfc,  ...)   { snprintf(log_msg, 512, __VA_ARGS__);  print_log(__FILE__,__LINE__, "Info"); }
    #if USE_DEBUG
        #define lame_debugf(gfc, ...) {  snprintf(log_msg, 512, __VA_ARGS__);  print_log(__FILE__,__LINE__, "Debug"); }
    #else
        #define lame_debugf(gfc, ...) 
    #endif
#endif
