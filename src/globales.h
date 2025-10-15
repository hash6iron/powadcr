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
 #include "nvs.h"
 #include "nvs_flash.h"
 #include <stdio.h>
 #include <inttypes.h>
 #include <string>

 #define  up   1
 #define  down 0
 
File global_dir;
//File global_file; 

int stackFreeCore0 = 0;
int stackFreeCore1 = 0;
int lst_stackFreeCore0 = 0;
int lst_stackFreeCore1 = 0;
int lst_psram_used = 0;
int lst_stack_used = 0;
int lst_psram_free = 0;
int lst_stack_free = 0;
int SD_SPEED_MHZ = 4;

// Inversion de pulso
// ------------------
uint8_t POLARIZATION = 0;             // 0 = DOWN, 1 = HIGH
uint8_t EDGE_EAR_IS = POLARIZATION;

// ************************************************************
//
// Estructura de datos
//
// ************************************************************

enum ConfigType {
  CONFIG_TYPE_STRING,
  CONFIG_TYPE_BOOL,
  CONFIG_TYPE_UINT8,
  CONFIG_TYPE_UINT16,
  CONFIG_TYPE_INT8,
  CONFIG_TYPE_UINT32,
  CONFIG_TYPE_DOUBLE,
  CONFIG_TYPE_FLOAT
};

struct ConfigEntry {
  const char* key;
  ConfigType type;
  void* value;  // Pointer to the configuration value
};

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
    char typeName[15];
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
  bool signalLvl = false;  //true == polarization UP, false == DOWN
  uint8_t edge = POLARIZATION;       // Edge of the begining of the block. Only for playing
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

struct tPZXBlockDescriptor
{
  
};

struct tAudioList
{
    String path = "";
    String filename = "";
    int index = 0;
    int size = 0;
};

// En little endian. Formato PZX
struct tPZX
{
  char tag[5] = {""};                         // el tag son 4 caracteres + /0
  uint32_t size = 0;
  tPZXBlockDescriptor* descriptor = nullptr;
};

// Estructura tipo TAP
struct tTAP 
{
    char name[11];                                  // Nombre del TAP
    uint32_t size = 0;                                   // Tamaño
    int numBlocks = 0;                              // Numero de bloques
    tTAPBlockDescriptor* descriptor;            // Descriptor
};

// Procesador de TAP
tTAP myTAP;
// Procesador de TZX/TSX/CDT
tTZX myTZX;

// tMediaDescriptor myMediaDescriptor[256]; // Descriptor de ficheros multimedia

// struct tBlock 
// {
//   int index = 0;            // Numero del bloque
//   int offset = 0;           // Byte donde empieza
//   uint8_t* header;      // Cabecera del bloque
//   uint8_t* data;        // Datos del bloque
// };

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

//
//
bool myTAPmemoryReserved = false;
bool myTZXmemoryReserved = false;
// bool bitChStrMemoryReserved = false;
// bool datablockMemoryReserved = false;
// bool bufferRecMemoryReserved = false;

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

// WAV record
bool MODEWAV = false;
bool OUT_TO_WAV = false;
bool PLAY_TO_WAV_FILE = false;
bool WAV_8BIT_MONO = false;
bool disable_auto_media_stop = false;

// Power led
bool POWERLED_ON = true;
bool PWM_POWER_LED = false;
uint8_t POWERLED_DUTY = 30;

uint8_t TAPESTATE = 0;
uint8_t LAST_TAPESTATE = 0;

// --------------------------------------------------------------------------
//
//  ZXProcessor
//
// --------------------------------------------------------------------------
// Inicialmente se define como flanco DOWN para que empiece en UP.
// Polarización de la señal.
// Con esto hacemos que el primer pulso sea UP 
// (porque el ultimo era DOWN supuestamente)


// bool FIRST_BLOCK_INVERTED = false;
//edge SCOPE = down;
bool APPLY_END = false;
int SAMPLING_RATE = 22050; //STANDARD_SR_ZX_SPECTRUM * 2;    // 44100 
int LAST_SAMPLING_RATE = 22050; //44100;
int WAV_SAMPLING_RATE = DEFAULT_WAV_SAMPLING_RATE; // 44100;
int WAV_BITS_PER_SAMPLE = 16;
int WAV_CHAN = 2;
bool WAV_UPDATE_SR = false;
bool WAV_UPDATE_BS = false;
bool WAV_UPDATE_CH = false;

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
int SCHMITT_AMP = 50; //Porcentage
float IN_REC_VOL = 0.5;

int LAST_SCHMITT_THR = 0;
bool EN_SCHMITT_CHANGE = false;
bool EN_EAR_INVERSION = false;

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
bool STOP_OR_PAUSE_REQUEST = false;
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
String LAST_PROGRAM_NAME = "";
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
int PRG_BAR_OFFSET_INI = 0;
int PRG_BAR_OFFSET_END = 0;
int BYTES_BASE = 0;
int BYTES_TOBE_LOAD = 0;
//int BYTES_IN_THIS_BLOCK = 0;
int BYTES_LAST_BLOCK = 0;
//bool TSX_PARTITIONED = false;
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
// String FILE_PATH="";         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0;  // Nivel de profundida de directorio
String FILE_LAST_DIR = "/";
String FILE_PREVIOUS_DIR = "/";
String FILE_LAST_DIR_LAST = "../";
String REC_FILENAME = "";
String SOURCE_FILE_TO_MANAGE = "_files.lst";
String SOURCE_FILE_INF_TO_MANAGE = "_files.inf";
String ROTATE_FILENAME = "";
bool ENABLE_ROTATE_FILEBROWSER = false;
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
String LASTFNAME = "";
bool WAVFILE_PRELOAD = false;

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
bool START_FFWD_ANIMATION = false;
//bool FILE_BROWSER_SEARCHING = false;
bool FB_READING_FILES = false;
bool FB_CANCEL_READING_FILES = false;
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
bool UPDATE_HMI = false;
bool BLOCK_BROWSER_OPEN = false;
int BB_PAGE_SELECTED = 1;
int BB_BROWSER_STEP = 0; // 0 = info general, 1..MAX_BLOCKS_IN_BROWSER = items
int BB_BROWSER_MAX = 0;

// Variables de control de la reproducción
bool PLAY = false;
bool BTN_PLAY_PRESSED = false;

bool PAUSE = true;
bool REC = false;
bool STOP = false;
bool AUTO_STOP = false;
bool AUTO_PAUSE = false;
bool FFWIND = false;
bool RWIND = false;
bool KEEP_FFWIND = false;
bool KEEP_RWIND = false;
bool AUDIO_FORMART_IS_VALID = false;
// bool RECORDING_IN_PROGRESS = false;

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
bool SAMPLINGTEST = false;
//


double MAIN_VOL = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_R = 0.9 * MAIN_VOL_FACTOR;
double MAIN_VOL_L = 0.05 * MAIN_VOL_FACTOR;
float EQ_HIGH = 0.9;
float EQ_MID = 0.5;
float EQ_LOW = 0.7;
bool EQ_CHANGE = false;
bool AMP_CHANGE = false;
bool SPK_CHANGE = false;
bool VOL_CHANGE = false;
bool VOL_LIMIT_HEADPHONE = false;
double TONE_ADJUST = 0.0;
bool AUTO_FADE = false;
bool AUTO_NEXT = false;
int MEDIA_CURRENT_POINTER = 0;
bool CHANGE_TRACK_FILTER = false;

double MAX_MAIN_VOL = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_R = 1 * MAIN_VOL_FACTOR;
double MAX_MAIN_VOL_L = 1 * MAIN_VOL_FACTOR;
int EN_STEREO = 0;
bool ACTIVE_AMP = false;
bool EN_SPEAKER = false;
bool wasHeadphoneDetected = false;
bool wasHeadphoneAmpDetected = false;
bool preparingTestInOut = false;

// bool SCREEN_IS_LOADING = false;
bool BLOCK_REC_COMPLETED = false;

// Gestion de menú
bool MENU = false;
bool menuOn = false;
int nMENU = 0;

// Indica cuando el WEBFILE esta ocupado
bool WF_UPLOAD_TO_SD = 0;

bool SORT_FILES_FIRST_DIR = true;
bool REGENERATE_IDX = false;

#ifdef BLUETOOTH_ENABLE
    bool BLUETOOTH_ACTIVE = true;
    String BLUETOOTH_DEVICE_PAIRED = "JBL T450BT";
#endif

// Declaraciones de metodos
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void save();
// void saveHMIcfg(string value);
void logln(String txt);

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Define the array of configuration entries
ConfigEntry configEntries[] = 
{
  {"STEopt", CONFIG_TYPE_BOOL, &EN_STEREO},
  {"MAMopt", CONFIG_TYPE_BOOL, &ACTIVE_AMP},
  {"VLIopt", CONFIG_TYPE_BOOL, &VOL_LIMIT_HEADPHONE},
  {"VOLMopt", CONFIG_TYPE_DOUBLE, &MAIN_VOL},
  {"VOLLopt", CONFIG_TYPE_DOUBLE, &MAIN_VOL_L},
  {"VOLRopt", CONFIG_TYPE_DOUBLE, &MAIN_VOL_R},
  {"EQHopt", CONFIG_TYPE_FLOAT, &EQ_HIGH},
  {"EQMopt", CONFIG_TYPE_FLOAT, &EQ_MID},
  {"EQLopt", CONFIG_TYPE_FLOAT, &EQ_LOW},
  {"PLEopt", CONFIG_TYPE_BOOL, &PWM_POWER_LED},
  {"SFFopt", CONFIG_TYPE_BOOL, &SORT_FILES_FIRST_DIR},
  {"SPKopt", CONFIG_TYPE_BOOL, &EN_SPEAKER},
};

// static inline void erase_cntrl(std::string &s) 
// {
//   s.erase(std::remove_if(s.begin(), s.end(),
//           [&](char ch)
//           { return std::iscntrl(static_cast<unsigned char>(ch));}),
//           s.end());
// }

// Function to load the configuration
bool loadHMICfg() 
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open NVS
  nvs_handle_t handle;
  err = nvs_open("storage", NVS_READONLY, &handle);
  if (err != ESP_OK) {
      printf("Error (%s) opening NVS handle for read!\n", esp_err_to_name(err));
      logln("Error - abriendo NVS");
      return true;
  }

  // Iterate over the configuration entries and load them
  for (auto& entry : configEntries) {
      switch (entry.type) {
          case CONFIG_TYPE_STRING: {
              size_t required_size = 0;
              err = nvs_get_str(handle, entry.key, NULL, &required_size); // Get required size first
              if (err == ESP_OK && required_size > 0) {
                  std::string* str_value = static_cast<std::string*>(entry.value);
                  char* buffer = new char[required_size];
                  err = nvs_get_str(handle, entry.key, buffer, &required_size);
                  if (err == ESP_OK) {
                      // Asignar el contenido del buffer al string
                      *str_value = std::string(buffer, required_size - 1);  // Se resta 1 para no incluir el carácter nulo
                  }
                  delete[] buffer;  // Free memory
              } else if (err == ESP_ERR_NVS_NOT_FOUND) {
                  printf("Key '%s' not found, skipping...\n", entry.key);
              }
              break;
          }
          case CONFIG_TYPE_BOOL: {
              std::string bool_str;
              size_t required_size = 0;
              err = nvs_get_str(handle, entry.key, NULL, &required_size); // Get size of boolean string
              if (err == ESP_OK && required_size > 0) {
                  char* buffer = new char[required_size];
                  err = nvs_get_str(handle, entry.key, buffer, &required_size);
                  if (err == ESP_OK) {
                      bool_str = buffer;
                      *static_cast<bool*>(entry.value) = (bool_str == "true");
                  }
                  delete[] buffer;
              } else if (err == ESP_ERR_NVS_NOT_FOUND) {
                  printf("Key '%s' not found, skipping...\n", entry.key);
              }
              break;
          }
          case CONFIG_TYPE_UINT8: {
              err = nvs_get_u8(handle, entry.key, static_cast<uint8_t*>(entry.value));
              if (err == ESP_ERR_NVS_NOT_FOUND) {
                  printf("Key '%s' not found, skipping...\n", entry.key);
              }
              break;
          }
          case CONFIG_TYPE_UINT16: {
              err = nvs_get_u16(handle, entry.key, static_cast<uint16_t*>(entry.value));
              if (err == ESP_ERR_NVS_NOT_FOUND) {
                  printf("Key '%s' not found, skipping...\n", entry.key);
              }
              break;
          }
          case CONFIG_TYPE_FLOAT: {
            err = nvs_get_i32(handle, entry.key, static_cast<int32_t*>(entry.value));
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                printf("Key '%s' not found, skipping...\n", entry.key);
            }
            break;
          }        
          case CONFIG_TYPE_DOUBLE: {
            err = nvs_get_i64(handle, entry.key, static_cast<int64_t*>(entry.value));
            if (err == ESP_ERR_NVS_NOT_FOUND) {
                printf("Key '%s' not found, skipping...\n", entry.key);
            }
            break;
          }             
          case CONFIG_TYPE_INT8: {
              err = nvs_get_i8(handle, entry.key, static_cast<int8_t*>(entry.value));
              if (err == ESP_ERR_NVS_NOT_FOUND) {
                  printf("Key '%s' not found, skipping...\n", entry.key);
              }
              break;
          }
      }

      // Print error if there's a problem reading any entry
      if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
          printf("Error (%s) reading key '%s'!\n", esp_err_to_name(err), entry.key);
      }
  }

  // Close NVS
  nvs_close(handle);

  return false;
}


// Function to save the configuration
void saveHMIcfg(std::string value) 
{

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open NVS
  nvs_handle_t handle;
  err = nvs_open("storage", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
      printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
      logln("Error opening NVS handle");
      return;
  }

  // Iterate over the configuration entries and save them
  for (const auto& entry : configEntries) {
      if (value == "all" || value == entry.key) {
          switch (entry.type) {
              case CONFIG_TYPE_STRING:
                  nvs_set_str(handle, entry.key, static_cast<std::string*>(entry.value)->c_str());
                  break;
              case CONFIG_TYPE_BOOL:
                  nvs_set_str(handle, entry.key, *static_cast<bool*>(entry.value) ? "true" : "false");
                  break;
              case CONFIG_TYPE_UINT8:
                  nvs_set_u8(handle, entry.key, *static_cast<uint8_t*>(entry.value));
                  break;
              case CONFIG_TYPE_UINT16:
                  nvs_set_u16(handle, entry.key, *static_cast<uint16_t*>(entry.value));
                  break;
              case CONFIG_TYPE_UINT32:
              nvs_set_u32(handle, entry.key, *static_cast<uint32_t*>(entry.value));
                  break;
              case CONFIG_TYPE_INT8:
              nvs_set_i8(handle, entry.key, *static_cast<int8_t*>(entry.value));
                  break;
          }
      }
  }

  // Commit the updates to NVS
  err = nvs_commit(handle);
  if (err != ESP_OK) 
  {
      printf("Error (%s) committing updates to NVS!\n", esp_err_to_name(err));
  }

  // Close NVS
  nvs_close(handle);

}

void saveVolSliders()
{
  saveHMIcfg("VOLMopt");
  saveHMIcfg("VOLLopt");
  saveHMIcfg("VOLRopt");  
}

// Function to load configuration from SD using fopen
bool loadFromSD() {
  FILE *file = fopen("/sd/.powadcr.cfg", "r");
  if (!file) {
      printf("Failed to open .powadcr.cfg for reading\n");
      return true;
  }

  char line[128]; // Buffer para leer cada línea
  while (fgets(line, sizeof(line), file)) {
      char *delimiterPos = strchr(line, '=');
      if (!delimiterPos) continue; // Saltar líneas sin '='

      *delimiterPos = '\0'; // Dividir en campo y valor
      std::string key = line;
      std::string valueStr = delimiterPos + 1;

      // Remover salto de línea final si existe
      valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\n'), valueStr.end());

      // Buscamos el campo y actualizamos su valor
      for (auto& entry : configEntries) {
          if (entry.key == key) {
              switch (entry.type) {
                  case CONFIG_TYPE_STRING:
                      *static_cast<std::string*>(entry.value) = valueStr;
                      break;
                  case CONFIG_TYPE_BOOL:
                      *static_cast<bool*>(entry.value) = (valueStr == "true");
                      break;
                  case CONFIG_TYPE_UINT8:
                      *static_cast<uint8_t*>(entry.value) = static_cast<uint8_t>(std::stoi(valueStr));
                      break;
                  case CONFIG_TYPE_UINT16:
                      *static_cast<uint16_t*>(entry.value) = static_cast<uint16_t>(std::stoi(valueStr));
                      break;
                  case CONFIG_TYPE_UINT32:
                      *static_cast<uint32_t*>(entry.value) = static_cast<uint32_t>(std::stoi(valueStr));
                      break;
                  case CONFIG_TYPE_INT8:
                      *static_cast<int8_t*>(entry.value) = static_cast<int8_t>(std::stoi(valueStr));
                      break;
              }
              break;
          }
      }
  }

  fclose(file);
  return false;
}

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
    Serial.println("");
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

// Lee un rango de bytes de un archivo FS (SD, SD_MMC, etc.)
// void readFileRange2(File &file, uint8_t* buffer, int offset, int size, bool keepPosition = false) {
//     // Guarda la posición actual si se va a restaurar después
//     uint32_t oldPos = 0;
//     if (keepPosition) {
//         oldPos = file.position();
//     }

//     // Mueve el puntero al offset deseado
//     file.seek(offset);

//     // Lee los bytes al buffer
//     file.read(buffer, size);

//     // Restaura la posición si es necesario
//     if (keepPosition) {
//         file.seek(oldPos);
//     }
// }

// Puedes ponerla en cualquier archivo de tu proyecto

// size_t readFileRangeFS(File &file, uint8_t* buffer, size_t offset, size_t size, bool keepPosition = false) {
//     size_t oldPos = 0;
//     if (keepPosition) {
//         oldPos = file.position();
//     }

//     size_t fileSize = file.size();
//     if (offset >= fileSize) {
//         if (keepPosition) file.seek(oldPos);
//         return 0;
//     }

//     if (offset + size > fileSize) {
//         size = fileSize - offset;
//     }

//     file.seek(offset);
//     size_t bytesRead = file.read(buffer, size);

//     if (keepPosition) {
//         file.seek(oldPos);
//     }
//     return bytesRead;
// }


void readFileRange(File mFile, uint8_t* &bufferFile, uint32_t offset, int size, bool logOn=false)
{         
    if (mFile) 
    {
        // Ponemos a cero el puntero de lectura del fichero
        mFile.seek(0);          

        // Obtenemos el tamano del fichero
        int rlen = mFile.available();
        FILE_LENGTH = rlen;

        // Posicionamos el puntero en la posicion indicada por offset
        mFile.seek(offset); 

        // Si el fichero tiene aun datos entonces capturo
        if (rlen != 0)
        {
            // Leo el bloque y lo meto en bufferFile.
            mFile.read(bufferFile, size);
            
            #ifdef DEBUGMODE
                logln("buffer read: ");
                for (int i=0; i<size; i++)
                {
                    logHEX(bufferFile[i]);
                    log(" ");
                }
            #endif
        }
    } 
    else 
    {
        #ifdef DEBUGMODE
            logln("SD Card: error opening file.");
            logln(mFile.name());
        #endif
    }
}

bool isDirectoryPath(const char* path) 
{
    String spath = String(path);

    int lastSlash = spath.lastIndexOf('/');
    int lastDot = spath.lastIndexOf('.');

    // ¿Hay un punto después del último slash y al menos un carácter después del punto?
    if (lastDot > lastSlash && lastDot < spath.length() - 1) {
        // Asumimos que es un fichero
        return false;
    }

    // Si no, comprobamos si es directorio
    File dir = SD_MMC.open(path, FILE_READ);
    bool isDir = dir && dir.isDirectory();
    dir.close();
    return isDir;
}

int getFreeFileDescriptors() {
    int count = 0;
    // El ESP32 típicamente tiene un máximo de 16 descriptores
    const int MAX_FD = 16;
    
    // Intentamos abrir archivos hasta que falle
    File* temp[MAX_FD];
    
    for(int i = 0; i < MAX_FD; i++) {
        temp[i] = new File(SD_MMC.open("/tmp.txt", FILE_READ));
        if(!temp[i]->available()) {
            count = i;
            break;
        }
    }
    
    // Cerramos todos los archivos temporales
    for(int i = 0; i < count; i++) {
        temp[i]->close();
        delete temp[i];
    }
    
    return MAX_FD - count;
}

