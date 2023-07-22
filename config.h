// Machine
//
<<<<<<< Updated upstream
// 0 for ZX Spectrum
// 1 for AMSTRAD
// 2 for MSX
#define MACHINE
#define TEST 0
=======
// Seleccionar solo uno por dispositivo powaDCR
// MACHINE_ZX para ZX Spectrum
// MACHINE_AMSTRAD para AMSTRAD
#define MACHINE_ZX
//#define MACHINE_AMSTRAD

// Descomentar para test de reproducción desde memoria
//#define TEST

>>>>>>> Stashed changes
// Define nivel de log
// 0 - Apagado
// 1 - Essential
// 2 - Error
// 3 - Warning
// 4 - Info     / Full trace info
#define LOG 0

// Inserta codigo de trazabilidad para desarrollo
#define TESTDEV 

// Definición del silencio entre bloques
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

