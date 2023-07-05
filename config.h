/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: config.h
    
    Creado por:
      Antonio Tamairón. 2023  
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Fichero de configuración
   
    Version: 1.0

    Historico de versiones

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

// Define silent block in seconds between blocks playing
// each block has two SILENT block
#define SILENT 1.5




// Activa el modo de split de los bloques. 
// si superan el tamaño (en bytes) definido por SIZE_TO_ACTIVATE_SPLIT
#define SPLIT_ENABLED 0
#define SD_FRQ_MHZ_INITIAL 20
#define SIZE_TO_ACTIVATE_SPLIT 20000

