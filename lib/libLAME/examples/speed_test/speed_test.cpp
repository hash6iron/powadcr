#ifdef EMULATOR
#include "Arduino.h"
#include "speed_test.ino"

int main(){
    setup();
    while(true){
        loop();
    }
    return -1;
}

#endif
