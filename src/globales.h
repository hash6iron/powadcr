/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: config.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Fichero de variables, estructuras globales
   
    Version: 1.0

    Historico de versiones


    Derechos de autor y distribución
    --------------------------------
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    To Contact the dev team you can write to hash6iron@gmail.com
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


// ************************************************************
//
// Estructura de datos
//
// ************************************************************

// Estructura de un bloque
struct tBlock 
{
  int index = 0;            // Numero del bloque
  int offset = 0;           // Byte donde empieza
  uint8_t* header;      // Cabecera del bloque
  uint8_t* data;        // Datos del bloque
};


// Estructura para el HMI
struct tFileBuffer
{
    bool isDir = false;
    String path = "";
    String type = "";
};

// ZX Proccesor config
// ********************************************************************
//
bool SILENCEDEBUG = false;
double ACU_ERROR = 0.0;
double INTPART = 0.0;

#ifdef MACHINE_ZX
  // Timming estandar de la ROM
  // Frecuencia de la CPU
  //double DfreqCPU = 3476604.8;
  //double DfreqCPU = 3450000;
  double DfreqCPU = 3500000;
  //double DfreqCPU = 3250000;

  const int DPILOT_HEADER = 8063;
  const int DPILOT_DATA = 3223;
  // Señales de sincronismo
  int DSYNC1 = 667;
  int DSYNC2 = 735;
  // Bits 0 y 1
  int DBIT_0 = 855;
  int DBIT_1 = 1710;
  // Pulsos guia
  int DPULSE_PILOT = 2168;
  int DPILOT_TONE = DPILOT_HEADER;
  
  // Definición del silencio entre bloques en ms
  int DSILENT = 1000;
#endif

// Inicializadores para los char*
String INITCHAR = "";
String INITCHAR2 = "..";
String INITFILEPATH = "/";

char STRTYPE0[] = "PROGRAM - HEADER\0";
char STRTYPE1[] = "BYTE - HEADER\0";
char STRTYPE2[] = "BASIC DATA\0";
char STRTYPE3[] = "SCREEN DATA\0";
char STRTYPE4[] = "BYTE DATA\0";
char STRTYPE7[] = "BYTE <SCREEN> - HEADER\0";
char STRTYPEDEF[] = "Standard data\0";

char LASTYPE0[] = "PROGRAM\0";
char LASTYPE1[] = "BYTE.H\0";
char LASTYPE2[] = "BASIC\0";
char LASTYPE3[] = "SCREEN\0";
char LASTYPE4_1[] = "BYTE\0";
char LASTYPE4_2[] = "SCREEN\0";
char LASTYPE7[] = "SCREEN.H\0";
char LASTYPE5[] = "ARRAY.NUM\0";
char LASTYPE6[] = "ARRAY.CHR\0";

bool ID_NOT_IMPLEMENTED = false;

enum edge
{
  up=1,
  down=0
};

// ZXProcessor
// Inicialmente se define como flanco up para que empiece en down.
// Polarización de la señal.
edge POLARIZATION = down;
edge LAST_EAR_IS = down;
bool INVERSETRAIN = false;
bool ZEROLEVEL = false;

// Limites de la señal
int maxLevelUP = 32767;
int maxLevelDown = -32768;
int LEVELUP = maxLevelUP;
int LEVELDOWN = maxLevelDown;

int DEBUG_AMP_L = 0;
int DEBUG_AMP_R = 0;

// Tamaño del fichero abierto
int FILE_LENGTH = 0;

// Schmitt trigger
int SCHMITT_THR = 18;
int LAST_SCHMITT_THR = 0;
bool EN_SCHMITT_CHANGE = false;

// Pulses width
int MIN_SYNC = 6;
int MAX_SYNC = 19;
int MIN_BIT0 = 1;
int MAX_BIT0 = 31;
int MIN_BIT1 = 32;
int MAX_BIT1 = 48;
int MIN_LEAD = 54;
int MAX_LEAD = 62;
int MAX_PULSES_LEAD = 255;

bool SHOW_DATA_DEBUG = false;
// Seleccion del canal para grabación izquierdo. Por defecto es el derecho
bool SWAP_MIC_CHANNEL = false;
bool SWAP_EAR_CHANNEL = false;
//
String RECORDING_DIR = "/REC";

//
String LAST_COMMAND = "";
bool TURBOMODE = false;
//bool TIMMING_STABLISHED = false;

// Variables para intercambio de información con el HMI
// DEBUG
int BLOCK_SELECTED = 0;
int PARTITION_BLOCK = 0;
int TOTAL_PARTS = 0;

String dataOffset1 = "";
String dataOffset2 = "";
String dataOffset3 = "";
String dataOffset4 = "";
String Offset1 = "";
String Offset2 = "";
String Offset3 = "";
String Offset4 = "";

String dbgBlkInfo = "";
String dbgPauseAB = "";
String dbgSync1 = "";
String dbgSync2 = "";
String dbgBit1 = "";
String dbgBit0 = "";
String dbgTState = "";
String dbgRep = "";

// OTRAS
bool TEST_RUNNING = false;
int LOADING_STATE = 0;
int CURRENT_BLOCK_IN_PROGRESS = 0;
int PROGRESS_BAR_REFRESH = 256;
int PROGRESS_BAR_REFRESH_2 = 32;
int PROGRESS_BAR_BLOCK_VALUE = 0;
int PROGRESS_BAR_TOTAL_VALUE = 0;
bool BLOCK_PLAYED = false;
String TYPE_FILE_LOAD = "";
char LAST_NAME[15];
char LAST_TYPE[36];
String LAST_TZX_GROUP = "";
String LAST_MESSAGE = "";
String PROGRAM_NAME = "";
String PROGRAM_NAME_2 = "";
int LAST_SIZE = 0;
int BYTES_LOADED = 0;
int BYTES_TOBE_LOAD = 0;
int BYTES_LAST_BLOCK = 0;
int TOTAL_BLOCKS = 0;
int LOOP_START = 0;
int BL_LOOP_START = 0;
int LOOP_COUNT=0;
int LOOP_END = 0;
int BL_LOOP_END = 0;
// Screen
bool SCREEN_LOADING = 0;
int SCREEN_LINE = 0;
int SCREEN_COL = 0;
int SCREEN_SECTION = 0;

// File system
int FILE_INDEX = 0;           // Índice de la fila seleccionada
int FILE_PAGE = 0;            // Contador de la pagina leida
String FILE_PATH="";         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0;  // Nivel de profundida de directorio
String FILE_LAST_DIR = "/";
String FILE_PREVIOUS_DIR = "/";
String FILE_LAST_DIR_LAST = "../";
int FILE_LAST_INDEX = 0;
int FILE_IDX_SELECTED = -1;
bool FILE_PREPARED = false;
bool PROGRAM_NAME_DETECTED = false;

tFileBuffer FILES_BUFF[MAX_FILES_TO_LOAD];
tFileBuffer FILES_FOUND_BUFF[MAX_FILES_FOUND_BUFF];

String FILE_TO_LOAD = "";
String FILE_TO_DELETE = "";
bool FILE_SELECTED_DELETE = false;
String FILE_DIR_TO_CHANGE = "";
int FILE_PTR_POS = 0;
int FILE_TOTAL_FILES = 0;
int FILE_STATUS = 0;
bool FILE_NOTIFIED = false;
bool FILE_SELECTED = false;
String FILE_PATH_SELECTED = "";
bool FILE_DIR_OPEN_FAILED = false;
bool FILE_BROWSER_OPEN = false;
bool FILE_BROWSER_SEARCHING = false;
bool FILE_CORRUPTED = false;
String FILE_TXT_TO_SEARCH = "";
bool waitingRecMessageShown = false;
bool WasfirstStepInTheRecordingProccess = false;

// Variables de control de la reproducción
bool PLAY = false;
bool PAUSE = true;
bool REC = false;
bool STOP = false;
bool FFWIND = false;
bool RWIND = false;
bool EJECT = false;
bool UP = false;
bool DOWN = false;
bool LEFT = false;
bool RIGHT = false;
bool ENTER = false;
bool LCD_ON = false;
bool ABORT = false;

//Estado de acciones de reproducción
const int PLAY_ST = 0;
const int STOP_ST = 1;
const int PAUSE_ST = 2;
const int REC_ST = 7;
const int READY_ST = 3;
const int END_ST = 4;
const int ACK_LCD = 5;
const int RESET = 6;
//
double MAIN_VOL_FACTOR = 100;

double MAIN_VOL = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_R = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_L = 0.9 * MAIN_VOL_FACTOR;
double LAST_MAIN_VOL = 0.9 * MAIN_VOL_FACTOR;
double LAST_MAIN_VOL_R = 0.9 * MAIN_VOL_FACTOR;
double LAST_MAIN_VOL_L = 0.9 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_R = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_L = 1 * MAIN_VOL_FACTOR;
int EN_STEREO = 0;
bool wasHeadphoneDetected = false;
bool wasHeadphoneAmpDetected = false;
bool preparingTestInOut = false;

// Gestion de menú
bool MENU = false;
bool menuOn = false;
int nMENU = 0;


void log(String txt)
{
  SerialHW.println(txt);
}

void logln(String txt)
{
  SerialHW.println(txt);
  SerialHW.println("");
}

// void logMemory() 
// {
//   log_d("Used PSRAM: %d", ESP.getPsramSize() - ESP.getFreePsram());
// }