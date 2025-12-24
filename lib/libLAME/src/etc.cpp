#include "lame.h"

#if defined(ARDUINO) && defined(ESP32)
#include "Arduino.h"

    int getFreeHeap() {
        return ESP.getFreeHeap();
    }
#else
    int getFreeHeap() {
        return 0;
    }

#endif