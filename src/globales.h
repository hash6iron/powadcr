/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: globales.h
    
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

int stackFreeCore0 = 0;
int stackFreeCore1 = 0;
int lst_stackFreeCore0 = 0;
int lst_stackFreeCore1 = 0;
int lst_psram_used = 0;
int lst_stack_used = 0;
int lst_psram_free = 0;
int lst_stack_free = 0;
int SD_SPEED_MHZ = 4;
// ************************************************************
//
// Estructura de datos
//
// ************************************************************

// Estructura de un bloque
struct tTimming
{
  int bit_0 = 855;
  int bit_1 = 1710;
  int pilot_len = 2168;
  int pilot_num_pulses = 0;
  int sync_1 = 667;
  int sync_2 = 735;
  int pure_tone_len = 0;
  int pure_tone_num_pulses = 0;
  int pulse_seq_num_pulses = 0;
  int* pulse_seq_array=nullptr;
  int bitcfg = 0;
  int bytecfg = 0;
};

// Estructura del descriptor de bloques
struct tTAPBlockDescriptor 
{
    bool corrupted = false;
    int offset = 0;
    int size = 0;
    int chk = 0;
    char name[11];
    bool nameDetected = false;
    bool header = false;
    bool screen = false;
    int type = 0;
    char typeName[11];
    bool playeable = true;
}; 

// Estructura de un descriptor de TZX
struct tTZXBlockDescriptor 
{
  int ID = 0;
  int offset = 0;
  int size = 0;
  int chk = 0;
  int pauseAfterThisBlock = 1000;   //ms
  int lengthOfData = 0;
  int offsetData = 0;
  char name[30];
  bool nameDetected = false;
  bool header = false;
  bool screen = false;
  int type = 0;
  bool playeable = false;
  int delay = 1000;
  int silent;
  int maskLastByte = 8;
  bool hasMaskLastByte = false;
  tTimming timming;
  char typeName[36];
  int group = 0;
  int loop_count = 0;
  bool jump_this_ID = false;
  int samplingRate = 79;
};

// Estructura tipo TZX
struct tTZX
{
  char name[11];                               // Nombre del TZX
  uint32_t size = 0;                             // Tamaño
  int numBlocks = 0;                        // Numero de bloques
  bool hasGroupBlocks = false; 
  tTZXBlockDescriptor* descriptor = nullptr;          // Descriptor
};

// Estructura tipo TSX
// struct tTSX
// {
//   char name[11];                               // Nombre del TSX
//   uint32_t size = 0;                             // Tamaño
//   int numBlocks = 0;                        // Numero de bloques
//   tTSXBlockDescriptor* descriptor = nullptr;          // Descriptor
// };

// Estructura tipo TAP
struct tTAP 
{
    char name[11];                                  // Nombre del TAP
    uint32_t size = 0;                                   // Tamaño
    int numBlocks = 0;                              // Numero de bloques
    tTAPBlockDescriptor* descriptor;            // Descriptor
};

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
    uint32_t fileposition = 0;
};

struct tConfig
{
  bool enable = false;
  String cfgLine = "";
};

// ZX Proccesor config
// ********************************************************************
//
bool SILENCEDEBUG = false;
double ACU_ERROR = 0.0;
double INTPART = 0.0;

// Timming estandar de la ROM
// Frecuencia de la CPU
// double DfreqCPU = 3450000;
// double DfreqCPU = 3476604.8;
// double DfreqCPU = 3500000;

// Ajustamos este valor para un exacta frecuencia de oscilacion
// aunque no coincida con el valor exacto real de la CPU
double DfreqCPU = 3500000;

const int DPULSES_HEADER = 8063;
const int DPULSES_DATA = 3223;
// Señales de sincronismo
int DSYNC1 = 667;
int DSYNC2 = 735;
// Bits 0 y 1
int DBIT_0 = 855;
int DBIT_1 = 1710;
// Pulsos guia
int DPILOT_LEN = 2168;  
// Definición del silencio entre bloques en ms
int DSILENT = 1000;
// A 44.1KHz
int DBIT_DR44_0 = 79;
int DBIT_DR44_1 = 79;
// A 22.05KHz
int DBIT_DR22_0 = 158;
int DBIT_DR22_1 = 158;
// A 11KHz
int DBIT_DR11_0 = 316;
int DBIT_DR11_1 = 316;
// A 22.05KHz
int DBIT_DR08_0 = 319;
int DBIT_DR08_1 = 319;
//
// Para direcrecord
int BIT_DR_1 = 0;
int BIT_DR_0 = 0;
//
bool DIRECT_RECORDING = false;

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
bool NOT_CAPTURE_ID = false;

enum edge
{
  up=1,
  down=0
};

// WAV record
bool MODEWAV = false;

uint8_t TAPESTATE = 0;

// --------------------------------------------------------------------------
//
//  ZXProcessor
//
// --------------------------------------------------------------------------
// Inicialmente se define como flanco DOWN para que empiece en UP.
// Polarización de la señal.
// Con esto hacemos que el primer pulso sea UP 
// (porque el ultimo era DOWN supuestamente)

// Inversion de pulso
// ------------------
// Para que empiece en DOWN tenemos que poner POLARIZATION en UP
// Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en DOWN)

edge POLARIZATION = down;
edge LAST_EAR_IS = POLARIZATION;
//edge SCOPE = down;
bool APPLY_END = false;
int SAMPLING_RATE = 44100;

bool INVERSETRAIN = false;
bool ZEROLEVEL = false;

// Limites de la señal
int maxLevelUp = 32767;
int maxLevelDown = -32767;
int LEVELUP = maxLevelUp;
int LEVELDOWN = maxLevelDown;

int DEBUG_AMP_L = 0;
int DEBUG_AMP_R = 0;

// Tamaño del fichero abierto
int FILE_LENGTH = 0;
bool FILE_IS_OPEN = false;

// Schmitt trigger
int SCHMITT_THR = 10; //Porcentage
int LAST_SCHMITT_THR = 0;
bool EN_SCHMITT_CHANGE = false;

int MIN_SYNC = 10;   //6
int MAX_SYNC = 18;  //19  -- S1: 190.6us + S2: 210us 

int MIN_BIT0 = 19;   //1
int MAX_BIT0 = 22;  //31  -- 2442.8 us * 2

int MIN_BIT1 = 24;  //32
int MAX_BIT1 = 44;  //48  -- 4885.7 us * 2

int MIN_LEAD = 50;  //54
int MAX_LEAD = 55;  //62  -- 619.4us * 2

int MAX_PULSES_LEAD = 256;

bool SHOW_DATA_DEBUG = false;
// Seleccion del canal para grabación izquierdo. Por defecto es el derecho
bool SWAP_MIC_CHANNEL = false;
bool SWAP_EAR_CHANNEL = false;
//
String RECORDING_DIR = "/REC";
int RECORDING_ERROR = 0;
//bool waitingNextBlock = false;
//bool FIRSTBLOCKREAD = false;
 

//int RECORDING_ERROR = 0;
bool REC_AUDIO_LOOP = true;
bool WIFI_ENABLE = true;
//
String LAST_COMMAND = "";
//bool TIMMING_STABLISHED = false;

// Variables para intercambio de información con el HMI
// DEBUG
int BLOCK_SELECTED = 0;
int POS_ROTATE_CASSETTE = 4;
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

// Configuracion
// Configuracion
tConfig* CFGHMI;
tConfig* CFGWIFI;

//
String POWAIP = "";

// OTRAS
bool TEST_RUNNING = false;
int LOADING_STATE = 0;              // 0 - Standby, 1 - Play, 2 - Stop, 3 - Pause, 4 - Recording
int CURRENT_BLOCK_IN_PROGRESS = 0;
int PROGRESS_BAR_REFRESH = 256;
int PROGRESS_BAR_REFRESH_2 = 32;
int PROGRESS_BAR_BLOCK_VALUE = 0;
int PROGRESS_BAR_TOTAL_VALUE = 0;
int PARTITION_SIZE = 0;
bool BLOCK_PLAYED = false;
String TYPE_FILE_LOAD = "";
char LAST_NAME[15];
char LAST_TYPE[36];
String lastType = "";
// Para TZX / TSX
String LAST_GROUP = "";
bool LAST_BLOCK_WAS_GROUP_START = false;
bool LAST_BLOCK_WAS_GROUP_END = false;
String lastGrp = "";
String LAST_MESSAGE = "";
String HMI_FNAME = "";
String lastMsn = "";
String lastfname = "";
int lastPr1 = 0;
int lastPr2 = 0;
int lastBl1 = 0;
int lastBl2 = 0;
String PROGRAM_NAME = "";
String PROGRAM_NAME_2 = "";
String lastPrgName = "";
String lastPrgName2 = "";
bool PROGRAM_NAME_ESTABLISHED = false;
int LAST_SIZE = 0;
int lstLastSize = 0;

int LAST_BIT_WIDTH = 0;
int MULTIGROUP_COUNT = 1;

int BYTES_LOADED = 0;
int BYTES_INI = 0;
int BYTES_TOBE_LOAD = 0;
int BYTES_IN_THIS_BLOCK = 0;
int BYTES_LAST_BLOCK = 0;
bool TSX_PARTITIONED = false;
int TOTAL_BLOCKS = 0;
int LOOP_START = 0;
int BL_LOOP_START = 0;
int LOOP_COUNT=0;
int LOOP_PLAYED=0;
int LOOP_END = 0;
int BL_LOOP_END = 0;
bool WAITING_FOR_USER_ACTION = false;
int LAST_SILENCE_DURATION = 0;

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
String SOURCE_FILE_TO_MANAGE = "_files.lst";
String SOURCE_FILE_INF_TO_MANAGE = "_files.inf";
bool LST_FILE_IS_OPEN = false;
int FILE_LAST_INDEX = 0;
int FILE_IDX_SELECTED = -1;
bool FILE_SELECTED = false;
bool FILE_PREPARED = false;
bool FILE_INSIDE_TAPE = false;
bool PROGRAM_NAME_DETECTED = false;

tFileBuffer FILES_BUFF[MAX_FILES_TO_LOAD];
tFileBuffer FILES_FOUND_BUFF[MAX_FILES_FOUND_BUFF];

int DIRCURRENTPOS = 0;
int DIRPREVPOS = 0;
String PATH_FILE_TO_LOAD = "";
String FILE_LOAD = "";
String FILE_TO_DELETE = "";
bool FILE_SELECTED_DELETE = false;
String FILE_DIR_TO_CHANGE = "";
int FILE_PTR_POS = 1;
int FILE_TOTAL_FILES = 0;
int FILE_TOTAL_FILES_SEARCH = 0;
int FILE_STATUS = 0;

String FILE_PATH_SELECTED = "";
bool FILE_DIR_OPEN_FAILED = false;
bool FILE_BROWSER_OPEN = false;
bool UPDATE = false;
//bool FILE_BROWSER_SEARCHING = false;
bool FILE_CORRUPTED = false;

bool IN_THE_SAME_DIR = false;

String FILE_TXT_TO_SEARCH = "";
//bool waitingRecMessageShown = false;
int CURRENT_PAGE = 0;

// Block browser
bool BB_OPEN = false;
bool BB_UPDATE = false;
int BB_PAGE = 0;
int BB_PTR_ITEM = 0;

// Variables de control de la reproducción
bool PLAY = false;
bool PAUSE = true;
bool REC = false;
bool STOP = false;
bool AUTO_STOP = false;
bool AUTO_PAUSE = false;
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
bool DISABLE_SD = false;

//Estado de acciones de reproducción
const int PLAY_ST = 0;
const int STOP_ST = 1;
const int PAUSE_ST = 2;
const int REC_ST = 7;
const int READY_ST = 3;
const int END_ST = 4;
const int ACK_LCD = 5;
const int RESET = 6;
const int SAMPLINGTEST = 8;
//
double MAIN_VOL_FACTOR = 100;

double MAIN_VOL = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_R = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_L = 0.9 * MAIN_VOL_FACTOR;
// double LAST_MAIN_VOL = 0.9 * MAIN_VOL_FACTOR;
// double LAST_MAIN_VOL_R = 0.9 * MAIN_VOL_FACTOR;
// double LAST_MAIN_VOL_L = 0.9 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_R = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_L = 1 * MAIN_VOL_FACTOR;
int EN_STEREO = 0;
bool wasHeadphoneDetected = false;
bool wasHeadphoneAmpDetected = false;
bool preparingTestInOut = false;

// bool SCREEN_IS_LOADING = false;
bool BLOCK_REC_COMPLETED = false;

// Gestion de menú
bool MENU = false;
bool menuOn = false;
int nMENU = 0;

void logHEX(int n)
{
    Serial.print(" 0x");
    Serial.print(n,HEX);
}

void log(String txt)
{
    Serial.print(txt);
}

void logln(String txt)
{
    Serial.println("");
    Serial.print(txt);
}

String lastAlertTxt = "";

void logAlert(String txt)
{
    // Solo muestra una vez el mismo mensaje.
    // esta rutina es perfecta para no llenar el buffer de salida serie con mensajes ciclicos
    if (lastAlertTxt != txt)
    {
      Serial.println("");
      Serial.print(txt);
      Serial.println("");
    }

    lastAlertTxt = txt;
}
