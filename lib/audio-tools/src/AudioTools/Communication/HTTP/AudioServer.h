#pragma once

#include "AudioServerWiFi.h"

// Only include Ethernet server if enabled (avoids missing Ethernet.h for WiFi-only projects)
#if defined(USE_ETHERNET) && USE_ETHERNET
#  include "AudioServerEthernet.h"
#endif