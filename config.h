// Machine
//
// 0 for ZX Spectrum
// 1 for AMSTRAD
// 2 for MSX
#define MACHINE
#define TEST 0
// Define nivel de log
// 0 - Apagado
// 1 - Essential
// 2 - Error
// 3 - Warning
// 4 - Info     / Full trace info
#define LOG 0

// Minimum log level to display.
// DEBUG = 100: Detailed debug information.
// INFO = 200: Interesting events. Examples: User logs in, SQL logs.
// NOTICE = 250: Normal but significant events.
// WARNING = 300: Exceptional occurrences that are not errors. Examples: Use of deprecated APIs, poor use of an API, undesirable things that are not necessarily wrong.
// ERROR = 400: Runtime errors that do not require immediate action but should typically be logged and monitored.
// CRITICAL = 500: Critical conditions. Example: Application component unavailable, unexpected exception.
// ALERT = 550: Action must be taken immediately. Example: Entire website down, database unavailable, etc. This should trigger the SMS alerts and wake you up.
// EMERGENCY = 600: Emergency: system is unusable.
#define MIN_LOG_LEVEL 100 // INFO

// Define silent block in seconds between blocks playing
// each block has two SILENT block
#define SILENT 1.5

// Turbo mode
// 0 - No
// 1 - Yes
#define TURBOMODE 0

// Activa el modo de split de los bloques. 
// si superan el tamaño (en bytes) definido por SIZE_TO_ACTIVATE_SPLIT
#define SPLIT_ENABLED 1
#define SD_FRQ_MHZ_INITIAL 20
#define SIZE_TO_ACTIVATE_SPLIT 20000

