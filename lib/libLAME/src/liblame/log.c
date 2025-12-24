

#include "log.h"
#include <stdio.h>
#include <string.h>

char log_msg[MAX_LOG_LEN];

int lame_write(char const s[static 1]) {
    for (size_t i = 0; s[i]; ++i)
        if (putchar(s[i]) == EOF) return EOF;
    return 0;
}

void print_log(const char* file, int line, const char* current_level) {
    const char* file_name = strrchr(file, '/') ? strrchr(file, '/') + 1 : file;
    lame_write("[");
    lame_write(current_level);
    lame_write("] ");
    lame_write(file_name);
    lame_write(" : ");
    char line_str[20];
    snprintf(line_str,20,"%d",line);
    lame_write(line_str);
    lame_write(" - ");
    lame_write(log_msg);
    lame_write("\n");
}
