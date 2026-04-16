/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: globales.h

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
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
#pragma once

#include "nvs.h"
#include "nvs_flash.h"
#include <inttypes.h>
#include <stdio.h>
#include <string>
#include <miniz.h>

#define up 1
#define down 0

File global_dir;

extern bool FTP_CONNECTED;

int stackFreeCore0 = 0;
int stackFreeCore1 = 0;
int lst_stackFreeCore0 = 0;
int lst_stackFreeCore1 = 0;
int lst_psram_used = 0;
int lst_stack_used = 0;
int lst_psram_free = 0;
int lst_stack_free = 0;
// Cached heap total sizes (set once at startup, never change)
uint32_t CACHED_PSRAM_TOTAL_KB = 0;
uint32_t CACHED_HEAP_TOTAL_KB = 0;
int SD_SPEED_MHZ = 4;

// Inversion de pulso
// ------------------
// ******************************************************************************
// POLARIZACIÓN POR DEFECTO: HIGH (1) para compatibilidad con TAPIR
// ******************************************************************************
// TAPIR (emulador de referencia) genera señales con polaridad INVERSA por
// defecto Esto significa que el nivel inicial es HIGH, y el primer pulso baja a
// LOW. Para compatibilidad con TZX generados por TAPIR (como BC's Quest for
// Tires), powaDCR debe usar la misma polaridad por defecto.
//
// POLARIZATION: 0 = LOW inicial (normal), 1 = HIGH inicial (inversa/TAPIR)
// ******************************************************************************
// Empezamos así porque ahora lo primero es aplicar el edge antes del pulso a
// generar y no al final para el siguiente. entonces hay que empezar en HIGH
// para que el primer pulso baje a LOW. Si empezamos en LOW, el primer pulso
// sube a HIGH y eso no es compatible con el estandar TZX.
uint8_t POLARIZATION = 1;
uint8_t EDGE_EAR_IS = POLARIZATION;
bool CHANGE_PZX_LEVEL = false;
bool FORZE_LEVEL = false;       // Forzar nivel inicial en bloques PZX
bool KEEP_CURRENT_EDGE = false; // Mantener el nivel actual sin cambios
int LAST_PULSE_WIDTH = 0;

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
  const char *key;
  ConfigType type;
  void *value; // Pointer to the configuration value
};

struct tRlePulse {
  uint32_t pulse_len; // Longitud del pulso en T-States (puede ser hasta 31 bits
                      // en PZX)
  uint16_t repeat;    // Número de repeticiones (será 1 para pulsos normales)
};

// Estructura de un bloque
struct tTimming {
  int bit_0 = 855;
  int bit_1 = 1710;
  int pilot_len = 2168;
  int pilot_num_pulses = 0;
  int sync_1 = 667;
  int sync_2 = 735;
  int pure_tone_len = 0;
  int pure_tone_num_pulses = 0;
  int pulse_seq_num_pulses = 0;
  int *pulse_seq_array = nullptr;
  int bitcfg = 0;
  int bytecfg = 0;
  uint32_t csw_sampling_rate;
  int csw_compression_type;
  int csw_num_pulses;        // ✅ AÑADIR: Número de pulsos en la secuencia RLE
  tRlePulse *csw_pulse_data; // ✅ AÑADIR: Puntero a la secuencia de pulsos RLE

  // PARA PZX
  int pzx_num_pulses;        // Número de pulsos en un bloque PULS
  tRlePulse *pzx_pulse_data; // Puntero a la secuencia de pulsos (reutilizamos
                             // tRlePulse)
};

struct tSymDef {
  // Definición de pulsos
  int symbolFlag = 0;
  int *pulse_array = nullptr;
};

struct tPrle {
  // Tipo para el array de pilot/sync y repetición
  int symbol = 0;
  int repeat = 0;
};

struct tSymbol {
  // Definición de un bloque 0x19
  int TOTP = 0; // Total numero de simbols para pilot/sync
  int NPP = 0;  // Maximo numero de pulsos por pilot/sync simbolo
  int ASP =
      0; // Numero de simbolos para pilot/sync en la Alphabet table (0 = 256)
  int TOTD = 0; // Total numero de simbols en el data stream
  int NPD = 0;  // Maximo numero de pulsos por data simbolo
  int ASD = 0;  // Numero de simbolos para data en la Alphabet table (0 = 256)
  tSymDef *symDefPilot = nullptr; // Pilot and Sync definition table
  tSymDef *symDefData = nullptr;  // Data definition table
  tPrle *pilotStream = nullptr;   // Pilot and sync data stream
  int offsetDataStream = 0;
  int offsetPilotDataStream = 0;
  uint8_t *dataStream = nullptr; // Data stream
};

// Estructura del descriptor de bloques
struct tTAPBlockDescriptor {
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
struct tTZXBlockDescriptor {
  int ID = 0;
  int offset = 0;
  int size = 0;
  int chk = 0;
  double pauseAfterThisBlock = 1000.0; // ms
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
  tSymbol symbol;
  uint16_t call_sequence_count;
  uint16_t *call_sequence_array;
  uint16_t numSelections;
  char typeName[36];
  //
  int compressionType = 0;
  int group = 0;
  int loop_count = 0;
  bool jump_this_ID = false;
  int samplingRate = 79;
  bool signalLvl = false; // true == polarization UP, false == DOWN
  uint8_t edge =
      POLARIZATION; // Edge of the begining of the block. Only for playing
};

// Estructura tipo TZX
struct tTZX {
  char name[11];     // Nombre del TZX
  uint32_t size = 0; // Tamaño
  int numBlocks = 0; // Numero de bloques
  bool hasGroupBlocks = false;
  tTZXBlockDescriptor *descriptor = nullptr; // Descriptor
  bool availableForREM = true;
};

struct tPZXBlockDescriptor {
  char name[11];
  char tag[5]; // "PZXT", "PULS", etc.
  int offset = 0;
  int size = 0;
  bool playeable = false;
  char typeName[36];
  uint8_t initial_level = 0; // 0 para low, 1 para high
  tTimming timming;          // Para bloques PULS y DATA

  // Para bloque DATA
  int data_bit_count = 0;
  int data_tail_pulse = 0;
  int data_p0_count = 0;
  int data_p1_count = 0;
  uint16_t *data_s0_pulses = nullptr;
  uint16_t *data_s1_pulses = nullptr;
  int data_stream_offset = 0;
  // Para bloques CSW
  int csw_num_pulses;
  tRlePulse *csw_pulse_data;
  // Para el bloque PZX STOP
  uint16_t stop_flags;
  // Para bloque PAUS
  int pause_duration = 0;
  uint8_t edge =
      POLARIZATION; // Edge of the begining of the block. Only for playing
};

struct tAudioList {
  String path = "";
  String filename = "";
  int index = 0;
  int size = 0;
};

struct tRadioList {
  String name = "";
  String url = "";
  int index = 0;
};

// En little endian. Formato PZX
struct tPZX {
  char tag[5] = {""};   // el tag son 4 caracteres + /0
  int numBlocks = 0;    // Numero de bloques
  char name[11] = {""}; // Nombre del PZX
  uint32_t size = 0;
  uint32_t csw_sampling_rate;
  tPZXBlockDescriptor *descriptor = nullptr;
  bool availableForREM = false;
};

// Estructura tipo TAP
struct tTAP {
  char name[11];                   // Nombre del TAP
  uint32_t size = 0;               // Tamaño
  int numBlocks = 0;               // Numero de bloques
  tTAPBlockDescriptor *descriptor; // Descriptor
  bool availableForREM = true;
};

// Procesador de TAP
tTAP myTAP;
// Procesador de TZX/TSX/CDT
tTZX myTZX;
// Procesador de PZX
tPZX myPZX;

// tMediaDescriptor myMediaDescriptor[256]; // Descriptor de ficheros multimedia

// struct tBlock
// {
//   int index = 0;            // Numero del bloque
//   int offset = 0;           // Byte donde empieza
//   uint8_t* header;      // Cabecera del bloque
//   uint8_t* data;        // Datos del bloque
// };

// Estructura para el HMI
struct tFileBuffer {
  bool isDir = false;
  String path = "";
  String type = "";
  uint32_t fileposition = 0;
};

struct tConfig {
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
double DfreqCPU = 3500000.0;

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
bool ENABLE_POWER_LED = true;
uint8_t POWERLED_DUTY = POWER_LED_INTENSITY;
bool POWER_LED_MODE = false; // false = normal on/off, true = intensity control with duty cycle
bool HIDE_VIRTUAL_KEY = false;

uint8_t TAPESTATE = 0;
uint8_t LAST_TAPESTATE = 0;
bool ADD_ONE_SAMPLE_COMPENSATION = false;
bool MCP23017_AVAILABLE = false;

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
// edge SCOPE = down;
// bool APPLY_END = false;
double SAMPLING_RATE = STANDARD_SR_8_BIT_MACHINE; // STANDARD_SR_ZX_SPECTRUM               // 44100
int BASE_SR = STANDARD_SR_REC_ZX_SPECTRUM; // STANDARD_SR_ZX_SPECTRUM // 44100
int BASE_SR_TAP = 31250; // STANDARD_SR_8_BIT_MACHINE_TAP         // 44100
int LAST_SAMPLING_RATE = 22050;                    // 44100;
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
int SCHMITT_THR = 10; // Porcentage
int SCHMITT_AMP = 50; // Porcentage
float IN_REC_VOL = 0.9;

int LAST_SCHMITT_THR = 0;
bool EN_SCHMITT_CHANGE = false;
bool EN_EAR_INVERSION = false;

int MIN_SYNC = 10; // 6
int MAX_SYNC = 18; // 19  -- S1: 190.6us + S2: 210us

int MIN_BIT0 = 19; // 1
int MAX_BIT0 = 22; // 31  -- 2442.8 us * 2

int MIN_BIT1 = 24; // 32
int MAX_BIT1 = 44; // 48  -- 4885.7 us * 2

int MIN_LEAD = 50; // 54
int MAX_LEAD = 55; // 62  -- 619.4us * 2

int MAX_PULSES_LEAD = 256;

bool SHOW_DATA_DEBUG = false;
// Seleccion del canal para grabación izquierdo. Por defecto es el derecho
bool SWAP_MIC_CHANNEL = false;
bool SWAP_EAR_CHANNEL = false;
//
String RECORDING_DIR = "/REC";
int RECORDING_ERROR = 0;
// bool waitingNextBlock = false;
// bool FIRSTBLOCKREAD = false;

// int RECORDING_ERROR = 0;
bool REC_AUDIO_LOOP = true;
bool WIFI_ENABLE = true;
bool WIFI_CONNECTED = false;
bool DHCP_ENABLE = false;
//
String LAST_COMMAND = "";
// bool TIMMING_STABLISHED = false;
bool YES = false;
bool NO = false;

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
tConfig *CFGHMI;
//tConfig *CFGWIFI;
tConfig *CFGSYSTEM;

//
String POWAIP = "";

// OTRAS
bool TEST_RUNNING = false;
bool STOP_OR_PAUSE_REQUEST = false;
int LOADING_STATE =
    0; // 0 - Standby, 1 - Play, 2 - Stop, 3 - Pause, 4 - Recording
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
String LAST_LAST_MESSAGE = "";
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
// int BYTES_IN_THIS_BLOCK = 0;
int BYTES_LAST_BLOCK = 0;
// bool TSX_PARTITIONED = false;
int TOTAL_BLOCKS = 0;
int LOOP_START = 0;
int BL_LOOP_START = 0;
int LOOP_COUNT = 0;
int LOOP_PLAYED = 0;
int LOOP_END = 0;
int BL_LOOP_END = 0;
bool WAITING_FOR_USER_ACTION = false;
int LAST_SILENCE_DURATION = 0;
int LAST_RSAMPLES_CALC = 0;

// Screen
bool SCREEN_LOADING = 0;
int SCREEN_LINE = 0;
int SCREEN_COL = 0;
int SCREEN_SECTION = 0;

// File system
int FILE_INDEX = 0; // Índice de la fila seleccionada
int FILE_PAGE = 0;  // Contador de la pagina leida
// String FILE_PATH="";         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0; // Nivel de profundida de directorio
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
bool REQ_LAST_MEDIA = false;

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
// bool FILE_BROWSER_SEARCHING = false;
bool FB_READING_FILES = false;
bool FB_CANCEL_READING_FILES = false;
bool FILE_CORRUPTED = false;

bool IN_THE_SAME_DIR = false;

String FILE_TXT_TO_SEARCH = "";
// bool waitingRecMessageShown = false;
int CURRENT_PAGE = 0;
bool TAPE0_PAGE_SHOWN = false;
bool TAPE_PAGE_SHOWN = false;
bool RADIO_PAGE_SHOWN = false;

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
bool BTNREC_PRESSED = false;
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

// Estado de acciones de reproducción
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

uint8_t MASTER_VOL = 90;
float MAIN_VOL = 90;
float MAIN_VOL_R = 90;
float MAIN_VOL_L = 90;
float EQ_HIGH = 0.9;
float EQ_MID = 0.5;
float EQ_LOW = 0.7;
bool EQ_CHANGE = false;
bool AMP_CHANGE = false;
bool SPK_CHANGE = false;
bool VOL_CHANGE = false;
bool VOL_LIMIT_HEADPHONE = false;
double TONE_ADJUST = 0.0;
int SAMPLES_ADJUST = 0;
int AZIMUT = 5;
bool AUTO_FADE = false;
bool AUTO_NEXT = false;
int MEDIA_CURRENT_POINTER = 0;
bool CHANGE_TRACK_FILTER = false;

float MAX_MAIN_VOL = 100;
float MAX_MAIN_VOL_R = 100;
float MAX_MAIN_VOL_L = 100;
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
// bool WF_UPLOAD_TO_SD = 0;

bool SORT_FILES_FIRST_DIR = true;
bool REGENERATE_IDX = false;

#ifdef BLUETOOTH_ENABLE
bool BLUETOOTH_ACTIVE = true;
String BLUETOOTH_DEVICE_PAIRED = "JBL T450BT";
#endif

// Control remoto de motor (REM)
bool REM_ENABLE = true;
bool REM_DETECTED = false;
bool STATUS_REM_ACTUATED = false;
int firstBytes_REM = 0;

// WiFi
char HOSTNAME[32] = {};
String ssid = "";
char password[64] = {};

// Static IP - 2.4GHz WiFi AP
IPAddress local_IP(0, 0, 0, 0); // Your Desired Static IP Address
IPAddress subnet(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress primaryDNS(0, 0, 0, 0);   // Not Mandatory
IPAddress secondaryDNS(0, 0, 0, 0); // Not Mandatory

// Media player
bool MEDIA_PLAYER_EN = false;
bool WAS_LAUNCHED = false;

// Internet Radio
bool IRADIO_EN = false;
bool URL_RADIO_IS_READY = false;
bool RADIO_BUFFERED = false;

// Remote control
bool UPDATE_FROM_REMOTE_CONTROL = false;
int FILE_POS_REMOTE_CONTROL = 0;
bool CMD_FROM_REMOTE_CONTROL = false;

//
bool IGNORE_DSC = false;
//
bool CONVERT_TO_TZXDR = true;

//
const char* NTPSERVER = "pool.ntp.org";
int8_t TIMEZONE = 1; // GMT+1
bool SUMMERTIME = true;

uint8_t NTPhour = 0;
uint8_t NTPminute = 0;
uint8_t NTPsecond = 0;
uint16_t NTPyear = 0;
uint8_t NTPmonth = 0;
uint8_t NTPday = 0;

struct tm TIMEINFO;
bool RADIO_IS_PLAYING = false;
bool MUSIC_IS_PLAYING = false;
bool FLAC_IS_PLAYING = false;
bool NTP_AVAILABLE = false;

// bool PZX_EJECT_RQT = false;

// Auto-update
String HMI_MODEL = "";

bool SPOTIFY_CONTROL = false;
bool SPOTIFY_EN = false;
String SPOTIFY_CLIENT_ID = "";
String SPOTIFY_CLIENT_SECRET = "";

//
bool QUICK_BOOT = false;
bool DOWNLOADING_ZXDB = false;
bool BEEP = false;

// Declaraciones de metodos
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// void save();
// void saveHMIcfg(string value);
void logln(String txt);

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Define the array of configuration entries
ConfigEntry configEntries[] = {
    {"STEopt", CONFIG_TYPE_BOOL, &EN_STEREO},
    {"MAMopt", CONFIG_TYPE_BOOL, &ACTIVE_AMP},
    {"VLIopt", CONFIG_TYPE_BOOL, &VOL_LIMIT_HEADPHONE},
    {"VOLMopt", CONFIG_TYPE_FLOAT, &MASTER_VOL},
    {"VOLLopt", CONFIG_TYPE_FLOAT, &MAIN_VOL_L},
    {"VOLRopt", CONFIG_TYPE_FLOAT, &MAIN_VOL_R},
    {"EQHopt", CONFIG_TYPE_FLOAT, &EQ_HIGH},
    {"EQMopt", CONFIG_TYPE_FLOAT, &EQ_MID},
    {"EQLopt", CONFIG_TYPE_FLOAT, &EQ_LOW},
    {"PLEopt", CONFIG_TYPE_BOOL, &ENABLE_POWER_LED},
    {"SFFopt", CONFIG_TYPE_BOOL, &SORT_FILES_FIRST_DIR},
    {"PLDopt", CONFIG_TYPE_BOOL, &POWER_LED_MODE},
    {"HVKopt", CONFIG_TYPE_BOOL, &HIDE_VIRTUAL_KEY},
    {"SPKopt", CONFIG_TYPE_BOOL, &EN_SPEAKER},
    {"RBUFopt", CONFIG_TYPE_BOOL, &RADIO_BUFFERED},
    {"DHCPFopt", CONFIG_TYPE_BOOL, &DHCP_ENABLE},
    {"MCPAVAIL", CONFIG_TYPE_BOOL, &MCP23017_AVAILABLE},
    {"WIFIopt", CONFIG_TYPE_BOOL, &WIFI_ENABLE},
};

//           s.end());
// }

// Function to load the configuration
bool loadHMICfg() {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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
  for (auto &entry : configEntries) {
    switch (entry.type) {
    case CONFIG_TYPE_STRING: {
      size_t required_size = 0;
      err = nvs_get_str(handle, entry.key, NULL,
                        &required_size); // Get required size first
      if (err == ESP_OK && required_size > 0) {
        std::string *str_value = static_cast<std::string *>(entry.value);
        char *buffer = new char[required_size];
        err = nvs_get_str(handle, entry.key, buffer, &required_size);
        if (err == ESP_OK) {
          // Asignar el contenido del buffer al string
          *str_value = std::string(
              buffer,
              required_size - 1); // Se resta 1 para no incluir el carácter nulo
        }
        delete[] buffer; // Free memory
      } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_BOOL: {
      std::string bool_str;
      size_t required_size = 0;
      err = nvs_get_str(handle, entry.key, NULL,
                        &required_size); // Get size of boolean string
      if (err == ESP_OK && required_size > 0) {
        char *buffer = new char[required_size];
        err = nvs_get_str(handle, entry.key, buffer, &required_size);
        if (err == ESP_OK) {
          bool_str = buffer;
          *static_cast<bool *>(entry.value) = (bool_str == "true");
        }
        delete[] buffer;
      } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_UINT8: {
      err = nvs_get_u8(handle, entry.key, static_cast<uint8_t *>(entry.value));
      if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_UINT16: {
      err =
          nvs_get_u16(handle, entry.key, static_cast<uint16_t *>(entry.value));
      if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_FLOAT: {
      err = nvs_get_i32(handle, entry.key, static_cast<int32_t *>(entry.value));
      if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_DOUBLE: {
      err = nvs_get_i64(handle, entry.key, static_cast<int64_t *>(entry.value));
      if (err == ESP_ERR_NVS_NOT_FOUND) {
        printf("Key '%s' not found, skipping...\n", entry.key);
      }
      break;
    }
    case CONFIG_TYPE_INT8: {
      err = nvs_get_i8(handle, entry.key, static_cast<int8_t *>(entry.value));
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
void saveHMIcfg(std::string value) {

  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
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
  for (const auto &entry : configEntries) {
    if (value == "all" || value == entry.key) {
      switch (entry.type) {
      case CONFIG_TYPE_STRING:
        nvs_set_str(handle, entry.key,
                    static_cast<std::string *>(entry.value)->c_str());
        break;
      case CONFIG_TYPE_BOOL:
        nvs_set_str(handle, entry.key,
                    *static_cast<bool *>(entry.value) ? "true" : "false");
        break;
      case CONFIG_TYPE_UINT8:
        nvs_set_u8(handle, entry.key, *static_cast<uint8_t *>(entry.value));
        break;
      case CONFIG_TYPE_UINT16:
        nvs_set_u16(handle, entry.key, *static_cast<uint16_t *>(entry.value));
        break;
      case CONFIG_TYPE_UINT32:
        nvs_set_u32(handle, entry.key, *static_cast<uint32_t *>(entry.value));
        break;
      case CONFIG_TYPE_FLOAT:
        nvs_set_blob(handle, entry.key, static_cast<float *>(entry.value),
                     sizeof(float));
        break;
      case CONFIG_TYPE_INT8:
        nvs_set_i8(handle, entry.key, *static_cast<int8_t *>(entry.value));
        break;
      }
    }
  }

  // Commit the updates to NVS
  err = nvs_commit(handle);
  if (err != ESP_OK) {
    printf("Error (%s) committing updates to NVS!\n", esp_err_to_name(err));
  }

  // Close NVS
  nvs_close(handle);
}

void saveVolSliders() {
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
    if (!delimiterPos)
      continue; // Saltar líneas sin '='

    *delimiterPos = '\0'; // Dividir en campo y valor
    std::string key = line;
    std::string valueStr = delimiterPos + 1;

    // Remover salto de línea final si existe
    valueStr.erase(std::remove(valueStr.begin(), valueStr.end(), '\n'),
                   valueStr.end());

    // Buscamos el campo y actualizamos su valor
    for (auto &entry : configEntries) {
      if (entry.key == key) {
        switch (entry.type) {
        case CONFIG_TYPE_STRING:
          *static_cast<std::string *>(entry.value) = valueStr;
          break;
        case CONFIG_TYPE_BOOL:
          *static_cast<bool *>(entry.value) = (valueStr == "true");
          break;
        case CONFIG_TYPE_UINT8:
          *static_cast<uint8_t *>(entry.value) =
              static_cast<uint8_t>(std::stoi(valueStr));
          break;
        case CONFIG_TYPE_UINT16:
          *static_cast<uint16_t *>(entry.value) =
              static_cast<uint16_t>(std::stoi(valueStr));
          break;
        case CONFIG_TYPE_UINT32:
          *static_cast<uint32_t *>(entry.value) =
              static_cast<uint32_t>(std::stoi(valueStr));
          break;
        case CONFIG_TYPE_INT8:
          *static_cast<int8_t *>(entry.value) =
              static_cast<int8_t>(std::stoi(valueStr));
          break;
        }
        break;
      }
    }
  }

  fclose(file);
  return false;
}

void logHEX(int n) {
  Serial.print(" 0x");
  Serial.print(n, HEX);
}

void logBIN(int n) {
  Serial.print(" 0x");
  Serial.print(n, BIN);
}

void log(String txt) { Serial.print(txt); }

void logln(String txt) {
  //Serial.println("");
  Serial.print(txt);
  Serial.println("");
}

void loglnf(const char *format, String txt)
{
  Serial.printf(format,txt);
}

String lastAlertTxt = "";

void logAlert(String txt) {
  // Solo muestra una vez el mismo mensaje.
  // esta rutina es perfecta para no llenar el buffer de salida serie con
  // mensajes ciclicos
  if (lastAlertTxt != txt) {
    Serial.println("");
    Serial.print(txt);
    Serial.println("");
  }

  lastAlertTxt = txt;
}

// void readFileRange(File mFile, uint8_t* &bufferFile, uint32_t offset, int
// size, bool logOn=false)
// {
//     if (mFile)
//     {
//         // Ponemos a cero el puntero de lectura del fichero
//         mFile.seek(0);

//         // Obtenemos el tamano del fichero
//         int rlen = mFile.available();
//         FILE_LENGTH = rlen;

//         // Posicionamos el puntero en la posicion indicada por offset
//         mFile.seek(offset);

//         // Si el fichero tiene aun datos entonces capturo
//         if (rlen != 0)
//         {
//             // Leo el bloque y lo meto en bufferFile.
//             mFile.read(bufferFile, size);

//             #ifdef DEBUGMODE
//                 logln("buffer read: ");
//                 for (int i=0; i<size; i++)
//                 {
//                     logHEX(bufferFile[i]);
//                     log(" ");
//                 }
//             #endif
//         }
//     }
//     else
//     {
//         #ifdef DEBUGMODE
//             logln("SD Card: error opening file.");
//             logln(mFile.name());
//         #endif
//     }
// }

// ✅ VERSIÓN OPTIMIZADA - SIN MALLOC/FREE INNECESARIOS
void readFileRange(File &file, uint8_t *buffer, int offset, int size,
                   bool usePrgBar = false) {
  if (!file || size == 0 || !buffer) {
    return;
  }

  // ✅ SEEK DIRECTO AL OFFSET
  file.seek(offset);

  // ✅ LEER DIRECTAMENTE AL BUFFER
  size_t bytesRead = file.read(buffer, size);

  if (bytesRead != size) {
    logln("Warning: Expected " + String(size) + " bytes, read " +
          String(bytesRead));
  }

  // ✅ ACTUALIZAR BARRA DE PROGRESO SI APLICA
  if (usePrgBar) {
    BYTES_LOADED = offset + bytesRead;
    PROGRESS_BAR_TOTAL_VALUE = (BYTES_LOADED * 100) / BYTES_TOBE_LOAD;
  }
}

bool isDirectoryPath(const char *path) {
  String spath = String(path);

  int lastSlash = spath.lastIndexOf('/');
  int lastDot = spath.lastIndexOf('.');
  String spathWithoutDots = spath;
  spathWithoutDots.replace(".", "");
  int countDots = spath.length() - spathWithoutDots.length();

  // ¿Hay un punto después del último slash y al menos un carácter después del
  // punto?
  if (countDots == 1 && lastDot > lastSlash && lastDot < spath.length() - 1) 
  {
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
  File *temp[MAX_FD];

  for (int i = 0; i < MAX_FD; i++) {
    temp[i] = new File(SD_MMC.open("/tmp.txt", FILE_READ));
    if (!temp[i]->available()) {
      count = i;
      break;
    }
  }

  // Cerramos todos los archivos temporales
  for (int i = 0; i < count; i++) {
    temp[i]->close();
    delete temp[i];
  }

  return MAX_FD - count;
}

// Devuelve la posición del primer bit a 0 en un byte (0 = LSB, 7 = MSB), o -1 si todos están a 1
// int detectKeyPressed(uint8_t byte) {
//   for (int i = 0; i < 8; ++i) {
//     if (((byte >> i) & 1) == 1) {
//       return i;
//     }
//   }
//   return -1;
// }

// Escribe el estado (HIGH/LOW) en un pin del MCP23017 sin afectar los demás
void  MCP23017_writePin(uint8_t pin, uint8_t state, uint8_t i2c_addr = 0x20) {
  // Determina si el pin está en GPIOA (0-7) o GPIOB (8-15)
  uint8_t reg = (pin < 8) ? 0x12 : 0x13; // GPIOA=0x12, GPIOB=0x13
  uint8_t pin_mask = 1 << (pin % 8);

  // Lee el valor actual del registro
  Wire1.beginTransmission(i2c_addr);
  Wire1.write(reg);
  Wire1.endTransmission();
  Wire1.requestFrom(i2c_addr, (uint8_t)1);
  uint8_t current = 0;
  if (Wire1.available())
    current = Wire1.read();

  // Modifica solo el bit correspondiente
  if (state == HIGH)
    current |= pin_mask;
  else
    current &= ~pin_mask;

  // Escribe el nuevo valor
  Wire1.beginTransmission(i2c_addr);
  Wire1.write(reg);
  Wire1.write(current);
  Wire1.endTransmission();
}

// Lee el estado (HIGH/LOW) de un pin del MCP23017 sin afectar los demás
uint8_t MCP23017_readPin(uint8_t pin, uint8_t i2c_addr = 0x20) 
{
  // Determina si el pin está en GPIOA (0-7) o GPIOB (8-15)
  uint8_t reg = (pin < 8) ? 0x12 : 0x13; // GPIOA=0x12, GPIOB=0x13
  uint8_t pin_mask = 1 << (pin % 8);

  // Lee el valor actual del registro
  Wire1.beginTransmission(i2c_addr);
  Wire1.write(reg);
  Wire1.endTransmission();
  Wire1.requestFrom(i2c_addr, (uint8_t)1);
  uint8_t current = 0;
  if (Wire1.available())
    current = Wire1.read();

  // Devuelve HIGH o LOW como digitalRead
  return (current & pin_mask) ? HIGH : LOW;
}

uint8_t MCP23017_readGPIO(uint8_t regGPIO, uint8_t i2c_addr = 0x20) {

  // Determina si el pin está en GPIOA (0-7) o GPIOB (8-15) 
  uint8_t a;

  // read the current GPIO output latches
  Wire1.beginTransmission(i2c_addr);
  Wire1.write(regGPIO);	
  Wire1.endTransmission();
  
  Wire1.requestFrom(i2c_addr, (uint8_t)1);
  a = Wire1.read();
  // ba = Wire1.read();
  // ba <<= 8;
  // ba |= a;
  return a;
}

void remDetection() {
  bool isAvailableForREM = false;

  if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "TSX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TAP" ||
      TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "PZX" || TYPE_FILE_LOAD == "FLAC" || TYPE_FILE_LOAD == "MP3") 
  {
    isAvailableForREM = true;
  }
  else 
  {
    isAvailableForREM = false;
  }

  if (REM_ENABLE && isAvailableForREM) 
  {
    if (digitalRead(GPIO_MSX_REMOTE_PAUSE) == LOW && !REM_DETECTED) 
    {
      // Informamos del REM detectado
      myNex.writeStr("tape.wavind.txt", "REM");
      logln("REM detected");
      // Retardo de arranque de motor 20ms (50Hz)
      delay(MOTOR_DELAY_MS);
      // Flag del REM a true
      REM_DETECTED = true;
    }
    else if (digitalRead(GPIO_MSX_REMOTE_PAUSE) != LOW && REM_DETECTED) 
    {
      // Recuperamos el mensaje original
      if (WAV_8BIT_MONO) {
        myNex.writeStr("tape.wavind.txt", "MONO");
      } else {
        myNex.writeStr("tape.wavind.txt", "");
      }

      logln("REM released");
      // Retardo de parada de motor 20ms (50Hz)
      delay(MOTOR_DELAY_MS);
      // Reseteamos el flag
      REM_DETECTED = false;
    }
  }
}

// Descompone NTPdate ("YYYY-MM-DD") y NTPtime ("HH:MM:SS") y actualiza las variables globales

// Devuelve un String con la fecha y hora en formato 00/00/0000 - 00:00:00
inline String getFormattedDateTime(String amPm, uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t minute, uint8_t second) {
    char buf[30];
    
    //
    if (amPm=="PM")
    {
        hour = hour + 12; // Convertir a formato 12 horas
    }
    else if (amPm=="AM")
    {
        if (hour == 12) hour = 0; // Ajustar medianoche
    }

    if (day==0 || month==0 || year==0)
    {
        // Si no se han proporcionado fecha, usar la actual
        snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hour, minute, second);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%02u/%02u/%04u - %02u:%02u:%02u", day, month, year, hour, minute, second);
    }
    return String(buf);
}



// void generate_zxdb_files_list_flat(File lst, const char* baseurl, int totalPerPage, int offset) 
// {
// // Descarga todas las páginas de ZXDB (output=flat) y genera _files.lst y _files.inf en /ONLINE
//   HTTPClient http;
  
//   // Capturamos la información de ZXDB
//   String url = String(baseurl) + "&size=" + String(totalPerPage) + "&offset=" + String(offset);
//   http.begin(url.c_str());
//   int httpCode = http.GET();
  
//   if (httpCode == 200) 
//   {
//     String payload = http.getString();
//     int count = 0;
//     int hitNum = offset*totalPerPage;

//     // String totalKey = "total.value=";
//     // int totalPos = payload.indexOf(totalKey);
//     // String total = payload.substring(totalPos + totalKey.length(), payload.indexOf('\n', totalPos));
//     // total.trim();
//     // totalFiles = total.toInt();
//     // logln("Total " + String(totalFiles) + " files in ZXDB");
    
//     // bucle de analisis
//     while (true) 
//     {
//       String idKey = "hits." + String(hitNum) + "._id=";
//       String titleKey = "hits." + String(hitNum) + ".title=";
//       int idPos = payload.indexOf(idKey);
//       int titlePos = payload.indexOf(titleKey);

//       if (idPos == -1 || titlePos == -1) break;
      
//       int idValStart = idPos + idKey.length();
//       int idValEnd = payload.indexOf('\n', idValStart);
//       String id = payload.substring(idValStart, idValEnd);
      
//       int titleValStart = titlePos + titleKey.length();
//       int titleValEnd = payload.indexOf('\n', titleValStart);
//       String title = payload.substring(titleValStart, titleValEnd);
      
//       id.trim(); 
//       title.trim();
      
//       if (id.length() == 0 || title.length() == 0) 
//       {
//         hitNum++;
//         continue;
//       }

//       logln("Found: " + title + " (ID: " + id + ")");
      
//       // Escribimos el fichero _files.lst
//       lst.print(String(hitNum));
//       lst.print("|F|0|");
//       lst.print(title);
//       lst.print("|");
//       lst.println(id);
//       count++;
//       hitNum++;
//     }
//   } 
//   else 
//   {
//     Serial.print("Error HTTP: ");
//     Serial.println(httpCode);
//   }
//   http.end();

//   lst.flush();

//   //return totalFiles;
// }
  
// Descarga todas las rutas .zip de un juego ZXDB por ID y las guarda en _down.lst
// void fetch_zxdb_zip_paths_to_file(const char* game_id) {

//   String url = String("https://api.zxinfo.dk/v3/games/") + game_id + "?mode=compact&output=flat";
//   HTTPClient http;
//   http.begin(url);
//   int httpCode = http.GET();
//   if (httpCode == 200) {
//     String payload = http.getString();
//     File downlst = SD_MMC.open("/ONLINE/_down.lst", FILE_WRITE);
//     if (!downlst) {
//       Serial.println("No se pudo abrir _down.lst para escribir");
//       http.end();
//       return;
//     }
//     int pos = 0;
//     while (true) {
//       String key = "releases.0.files.";
//       int pathIdx = payload.indexOf(key, pos);
//       if (pathIdx == -1) break;
//       int pathStart = payload.indexOf(".path=", pathIdx);
//       if (pathStart == -1) { pos = pathIdx + key.length(); continue; }
//       pathStart += 6; // length of ".path="
//       int pathEnd = payload.indexOf('\n', pathStart);
//       String path = payload.substring(pathStart, pathEnd);
//       path.trim();
//       if (path.endsWith(".zip")) {
//         downlst.println(path);
//       }
//       pos = pathEnd;
//     }
//     downlst.close();
//     Serial.println("_down.lst generado correctamente");
//   } else {
//     Serial.print("Error HTTP: ");
//     Serial.println(httpCode);
//   }
//   http.end();
// }

// Descarga el CSV de World of Spectrum y genera _files.lst y _files.inf en /ONLINE
// void generate_online_files_list() 
// {
// // Descarga el resultado plano de ZXDB (output=flat) y genera _files.lst y _files.inf en /ONLINE
// const char* csv_url = "https://worldofspectrum.org/software/software_export?";
//   HTTPClient http;

//   File lst = SD_MMC.open("/ONLINE/_files.lst", FILE_WRITE);
//   if (!lst) {
//     Serial.println("No se pudo abrir _files.lst para escribir");
//     return;
//   }
//   http.begin(csv_url);
//   int httpCode = http.GET();
//   if (httpCode == 200) {
//     String payload = http.getString();
//     int lineStart = 0;
//     int lineEnd = payload.indexOf('\n');
//     std::vector<String> lines;
//     // Saltar cabecera
//     if (lineEnd != -1) lineStart = lineEnd + 1;
//     lineEnd = payload.indexOf('\n', lineStart);
    
//     while (lineEnd != -1) 
//     {
//       String line = payload.substring(lineStart, lineEnd);
//       lines.push_back(line);
//       lineStart = lineEnd + 1;
//       lineEnd = payload.indexOf('\n', lineStart);
//     }
    
//     // Última línea
//     if (lineStart < payload.length()) 
//     {
//       lines.push_back(payload.substring(lineStart));
//     }
    
//     int count = 0;
    
//     for (size_t i = 0; i < lines.size(); ++i) 
//     {
//       String line = lines[i];
//       // Parseo robusto de CSV para dos primeros campos
//       int len = line.length();
//       int pos = 0;
//       String id = "";
//       String title = "";
      
//       // Campo 1 (ID)
//       if (pos < len && line[pos] == '"') 
//       {
//         pos++;
//         while (pos < len) {
//           if (line[pos] == '"' && line[pos+1] == '"') { id += '"'; pos += 2; }
//           else if (line[pos] == '"') { pos++; break; }
//           else { id += line[pos++]; }
//         }
//         if (pos < len && line[pos] == ',') pos++;
//       } 
//       else 
//       {
//         while (pos < len && line[pos] != ',') id += line[pos++];
//         if (pos < len && line[pos] == ',') pos++;
//       }

//       // Campo 2 (title)
//       if (pos < len && line[pos] == '"') 
//       {
//         pos++;
//         while (pos < len) {
//           if (line[pos] == '"' && line[pos+1] == '"') { title += '"'; pos += 2; }
//           else if (line[pos] == '"') { pos++; break; }
//           else { title += line[pos++]; }
//         }
//         if (pos < len && line[pos] == ',') pos++;
//       } 
//       else 
//       {
//         while (pos < len && line[pos] != ',') title += line[pos++];
//         if (pos < len && line[pos] == ',') pos++;
//       }
//       id.trim(); title.trim();
//       if (id.length() == 0 || title.length() == 0) continue;
//       lst.print(String(i));
//       lst.print("|F|0|");
//       lst.print(title);
//       lst.print("|");
//       lst.println(id);
//       count++;
//     }
//     lst.close();
//     // Crear _files.inf
//     File inf = SD_MMC.open("/ONLINE/_files.inf", FILE_WRITE);
//     if (inf) {
//       inf.println("PATH=/ONLINE");
//       inf.print("CFIL=");
//       inf.println(count);
//       inf.println("CDIR=0");
//       inf.close();
//     } else {
//       Serial.println("No se pudo abrir _files.inf para escribir");
//     }
//     Serial.println("_files.lst y _files.inf generados correctamente");
//     myNex.writeStr("tape.g0.txt", "ZXDB catalogue captured");
//     myNex.writeNum("zxdb.progress.val", 100);
//   } else {
//     Serial.print("Error HTTP: ");
//     Serial.println(httpCode);
//     myNex.writeStr("tape.g0.txt", "Error HTTP " + String(httpCode));
//   }
//   http.end();
// }
// //

// Helper: descarga un fichero binario desde URL a ruta local en SD (HTTPS, chunked)
bool _downloadBinaryToSD(const String& url, const String& localPath)
{
  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.begin(secureClient, url);
  http.setTimeout(60000);   // 60s max (evita overflow de uint16_t en algunas versiones)
  http.setConnectTimeout(30000);
  http.addHeader("User-Agent", "PowaDCR/" + String(VERSION));
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    logln("HTTP error " + String(httpCode) + " downloading: " + url);
    http.end();
    return false;
  }

  File file = SD_MMC.open(localPath.c_str(), FILE_WRITE);
  if (!file)
  {
    logln("No se pudo crear fichero: " + localPath);
    http.end();
    return false;
  }

  const size_t bufSize = 4096;
  uint8_t* buf = (uint8_t*)malloc(bufSize);
  if (!buf)
  {
    logln("No se pudo reservar buffer para descarga");
    file.close();
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  int contentLength = http.getSize();
  size_t total = 0;

  while (http.connected() && (contentLength <= 0 || total < (size_t)contentLength))
  {
    size_t avail = stream->available();
    if (avail == 0) { delay(10); continue; }
    size_t toRead = min(avail, bufSize);
    size_t bytesRead = stream->readBytes(buf, toRead);
    if (bytesRead == 0) { delay(10); continue; }
    file.write(buf, bytesRead);
    total += bytesRead;
  }

  file.flush();
  file.close();
  free(buf);
  http.end();

  logln("Descargados " + String(total) + " bytes -> " + localPath);
  return (total > 0);
}

// Extrae ficheros de juego de un ZIP (leído de SD) a destDir usando miniz
// Retorna el número de ficheros extraídos
int _unzipGameFilesToDir(const String& zipPath, const String& destDir)
{
  File zf = SD_MMC.open(zipPath.c_str(), FILE_READ);
  if (!zf) { logln("_unzipGameFilesToDir: no se pudo abrir " + zipPath); return 0; }

  size_t zipSize = zf.size();
  uint8_t* zipBuf = (uint8_t*)ps_malloc(zipSize);
  if (!zipBuf)
  {
    logln("_unzipGameFilesToDir: sin PSRAM para " + String(zipSize) + " bytes");
    zf.close();
    return 0;
  }
  zf.read(zipBuf, zipSize);
  zf.close();

  mz_zip_archive zip;
  memset(&zip, 0, sizeof(zip));
  if (!mz_zip_reader_init_mem(&zip, zipBuf, zipSize, 0))
  {
    logln("_unzipGameFilesToDir: ZIP inválido: " + zipPath);
    free(zipBuf);
    return 0;
  }

  int numEntries = (int)mz_zip_reader_get_num_files(&zip);
  int extracted = 0;

  for (int i = 0; i < numEntries; i++)
  {
    mz_zip_archive_file_stat fs;
    if (!mz_zip_reader_file_stat(&zip, i, &fs)) continue;
    if (mz_zip_reader_is_file_a_directory(&zip, i)) continue;

    String entryName = String(fs.m_filename);
    int slash = entryName.lastIndexOf('/');
    if (slash != -1) entryName = entryName.substring(slash + 1);

    String entryUpper = entryName;
    entryUpper.toUpperCase();
    bool isGameFile = entryUpper.endsWith(".TAP") || entryUpper.endsWith(".TZX") ||
                      entryUpper.endsWith(".TSX") || entryUpper.endsWith(".PZX") ||
                      entryUpper.endsWith(".CDT");
    if (!isGameFile) continue;

    size_t uncompSize = (size_t)fs.m_uncomp_size;
    uint8_t* outBuf = (uint8_t*)ps_malloc(uncompSize);
    if (!outBuf) { logln("Sin PSRAM para extraer: " + entryName); continue; }

    if (mz_zip_reader_extract_to_mem(&zip, i, outBuf, uncompSize, 0))
    {
      String outPath = destDir + "/" + entryName;
      File outFile = SD_MMC.open(outPath.c_str(), FILE_WRITE);
      if (outFile)
      {
        outFile.write(outBuf, uncompSize);
        outFile.flush();
        outFile.close();
        logln("Extraído: " + outPath + " (" + String(uncompSize) + " bytes)");
        extracted++;
      }
      else { logln("Error creando: " + outPath); }
    }
    else { logln("Error extrayendo: " + entryName); }

    free(outBuf);
  }

  mz_zip_reader_end(&zip);
  free(zipBuf);
  return extracted;
}

// Descarga los ficheros de juego de ZXDB por ID y los guarda en /DOWNLOAD/<title>/
// game_id : ID de 7 dígitos almacenado en _files.lst
// title   : nombre del juego (se usa para crear el subdirectorio)
void downloadFromZXDB(String gameId, String title)
{

  DOWNLOADING_ZXDB = true;
  
  TYPE_FILE_LOAD = "ZXDB";

  LAST_MESSAGE = "Downloading: " + title;
  myNex.writeStr("tape.g0.txt", LAST_MESSAGE);

  if (!WIFI_CONNECTED || !WIFI_ENABLE)
  {
    logln("WiFi no disponible para descarga ZXDB");
    myNex.writeStr("tape.g0.txt", "No WiFi");
    return;
  }

  // Sanitizar el título para nombre de directorio FAT32
  String safeTitle = title;
  safeTitle.replace("/",  "-");
  safeTitle.replace("\\", "-");
  safeTitle.replace(":",  "-");
  safeTitle.replace("*",  "-");
  safeTitle.replace("?",  "-");
  safeTitle.replace("\"", "-");
  safeTitle.replace("<",  "-");
  safeTitle.replace(">",  "-");
  safeTitle.replace("|",  "-");

  String destDir = "/DOWNLOAD/" + safeTitle;

  if (!SD_MMC.exists("/DOWNLOAD")) SD_MMC.mkdir("/DOWNLOAD");
  if (!SD_MMC.exists(destDir))
  {
    if (!SD_MMC.mkdir(destDir))
    {
      logln("Error al crear directorio: " + destDir);
      myNex.writeStr("tape.g0.txt", "Error creating dir");
      return;
    }
  }

  // Obtener metadata del juego de la API ZXDB
  String metaUrl = "https://api.zxinfo.dk/v3/games/" + gameId + "?mode=compact&output=flat";
  logln("Obteniendo metadata ZXDB: " + metaUrl);
  myNex.writeStr("tape.g0.txt", "Fetching metadata...");

  // Payload declarado fuera del bloque para usarlo luego al parsear
  String payload;

  // Verificación de conectividad: si RadioPlayer desconectó WiFi para limpiar
  // el heap, necesitamos reconectar aquí. Esperamos hasta 15 s.
  if (WIFI_ENABLE && WIFI_CONNECTED) 
  {
    logln("WiFi conectado para ZXDB");
  }
  else 
  {
    logln("WiFi no disponible para descarga ZXDB");
    myNex.writeStr("tape.g0.txt", "No WiFi");

    return;
  }

  // Diagnóstico: el que importa es el bloque contiguo en SRAM interna
  // (MALLOC_CAP_INTERNAL). mbedTLS usa SRAM interna exclusivamente.
  size_t sramLibre = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  size_t maxBloque = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  logln("[ZXDB] WiFi conectado. Heap SRAM interna libre  : " + String(sramLibre));
  logln("[ZXDB] Max bloque SRAM interna                  : " + String(maxBloque));
  logln("[ZXDB] Heap total (incl PSRAM)                  : " + String(ESP.getFreeHeap()));
  
  if (maxBloque < 45000) {
    logln("[ZXDB] ⚠ ADVERTENCIA: Max bloque < 45KB. SSL probable fail con ECP_ALLOC");
  }

  // Bloque de scope con retry: el WiFiClientSecure se destruye en cada iteración,
  // liberando el contexto SSL antes de reintentar o de iniciar la descarga.
  bool metaOK = false;
  for (int attempt = 0; attempt < 3 && !metaOK; attempt++)
  {
    if (attempt > 0)
    {
      logln("Reintento metadata " + String(attempt) + " (fragmentacion de heap)...");
      myNex.writeStr("tape.g0.txt", "Retry " + String(attempt) + "...");
      vTaskDelay(pdMS_TO_TICKS(1500));
    }

    WiFiClientSecure secureClient;
    secureClient.setInsecure();

    HTTPClient http;
    http.begin(secureClient, metaUrl);
    http.setTimeout(30000);
    http.addHeader("User-Agent", "PowaDCR/" + String(VERSION));

    int httpCode = http.GET();
    if (httpCode == 200)
    {
      payload = http.getString();
      http.end();
      metaOK = true;
    }
    else
    {
      logln("Error HTTP metadata [intento " + String(attempt + 1) + "]: " + String(httpCode));
      myNex.writeStr("tape.g0.txt", "Error " + String(httpCode) + " (" + String(attempt + 1) + "/3)");
      http.end();
    }
    // secureClient destruido aquí al salir del scope del for, liberando SSL context
  }

  if (!metaOK)
  {
    logln("Fallo al obtener metadata tras 3 intentos.");
    myNex.writeStr("tape.g0.txt", "Metadata error after retries");
    //
    LAST_MESSAGE = "Memory allocation error. Reboot ESP32";
    myNex.writeStr("tape.g0.txt", LAST_MESSAGE);
    return;
  }
  logln("Metadata recibida: " + String(payload.length()) + " bytes");

  // Parsear releases.X.files.Y.path= buscando ficheros .zip
  // Los ficheros en ZXDB son siempre .zip (ej: 1LineCaveAdventure.tzx.zip)
  int filesDownloaded = 0;
  int releaseIdx = 0;
  const String tmpZip = "/tmp_zxdb.zip";

  while (true)
  {
    String releasePrefix = "releases." + String(releaseIdx) + ".files.";
    if (payload.indexOf(releasePrefix) == -1) break;

    int fileIdx = 0;
    while (true)
    {
      String pathKey = "releases." + String(releaseIdx) + ".files." + String(fileIdx) + ".path=";
      int pathPos = payload.indexOf(pathKey);
      if (pathPos == -1) break;

      int pathStart = pathPos + pathKey.length();
      int pathEnd   = payload.indexOf('\n', pathStart);
      String filePath = payload.substring(pathStart, pathEnd);
      filePath.trim();

      // Solo ficheros .zip que contengan juego (tzx.zip, tap.zip, etc.)
      String filePathUpper = filePath;
      filePathUpper.toUpperCase();
      if (filePathUpper.endsWith(".ZIP") && filePath.length() > 0)
      {
        int lastSlash = filePath.lastIndexOf('/');
        String zipName = (lastSlash != -1) ? filePath.substring(lastSlash + 1) : filePath;
        String downloadUrl = "https://spectrumcomputing.co.uk" + filePath;

        logln("Descargando ZIP: " + downloadUrl);
        myNex.writeStr("tape.g0.txt", "Downloading: " + zipName);

        if (_downloadBinaryToSD(downloadUrl, tmpZip))
        {
          myNex.writeStr("tape.g0.txt", "Extracting: " + zipName);
          int n = _unzipGameFilesToDir(tmpZip, destDir);
          filesDownloaded += n;
          SD_MMC.remove(tmpZip);  // borrar ZIP temporal
          
          LAST_MESSAGE = "Downloading done. See /DOWNLOAD";
          myNex.writeStr("tape.g0.txt", LAST_MESSAGE);


        }
        else 
        { 
          logln("Error descargando: " + zipName); 
          LAST_MESSAGE = "Downloading error";
          myNex.writeStr("tape.g0.txt", LAST_MESSAGE);
        }
      }

      fileIdx++;
    }
    releaseIdx++;
  }

  if (filesDownloaded == 0)
  {
    logln("No se encontraron ficheros de juego para ID: " + gameId);
    myNex.writeStr("tao.message.txt", "No game files found");
  }
  else
  {
    logln("Descargados " + String(filesDownloaded) + " fichero(s) en " + destDir);
    myNex.writeStr("tape.g0.txt", "Done: " + String(filesDownloaded) + " file(s)");
  }

  DOWNLOADING_ZXDB = false;
}

int get_total_files_ZXDB(const char* baseurl)
{
  HTTPClient http;
  int totalFiles = 0;
  
  // Capturamos la información de ZXDB
  String url = String(baseurl) + "&size=5&offset=0";
  http.begin(url.c_str());
  int httpCode = http.GET();
  
  if (httpCode == 200) 
  {
    String payload = http.getString();
    int count = 0;
    int hitNum = 0;

    String totalKey = "total.value=";
    int totalPos = payload.indexOf(totalKey);
    String total = payload.substring(totalPos + totalKey.length(), payload.indexOf('\n', totalPos));
    total.trim();
    totalFiles = total.toInt();
    
  }
  else 
  {
    Serial.print("Error HTTP: ");
    Serial.println(httpCode);
  }
  http.end();
  return totalFiles;  
}

// Descarga y actualiza el catalogo de ZXDB en la SD
void updateZXDB(String letter = "0")
{
    String searchChain = "#ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    String urlToSearch = "";
    String dir = "";
    int totalItemsFound = 0;
    
    if (WIFI_CONNECTED && WIFI_ENABLE)
    {
        // Reseteamos la barra de progreso
        myNex.writeNum("zxdb.j0.val", 0);

        //logln("Capturing ZXDB catalogue from letter: " + letter);
        //
        // Recorremos una cadena de busqueda para obtener todo el catálogo de ZXDB usando la API v3
        //
        if (letter != "0")
        {
          searchChain = letter;
          logln("Capturing ZXDB catalogue for letter: " + letter);
        }
        else
        {
          logln("Capturing entire ZXDB catalogue");
        }

        // Calculamos el total de items encontrados para mostrar una banda de progreso
        // recorremos el catalogo propuesto o una letra concreta
        for (int i = 0; i < searchChain.length(); i++)
        {
          char letter = searchChain.charAt(i);

          // Componemos la URL de busqueda
          if (letter == '#') 
            // Buscamos juegos que empiezan por un número.
            urlToSearch = "https://api.zxinfo.dk/v3/games/byletter/%23?contenttype=SOFTWARE&machinetype=ZXSPECTRUM&output=flat";
          else
            // Buscamos juegos que empiezan por letras
            urlToSearch = "https://api.zxinfo.dk/v3/games/byletter/" + String(letter) + "?contenttype=SOFTWARE&machinetype=ZXSPECTRUM&output=flat";  

            totalItemsFound += get_total_files_ZXDB(urlToSearch.c_str());
            myNex.writeStr("zxdb.message.txt", "Calculating total items: " + String(totalItemsFound));
        }

        logln("Total " + String(totalItemsFound) + " files in ZXDB");

        // Bucle principal
        for (int i = 0; i < searchChain.length(); i++)
        {

          logln("Capturing files list for letter: " + String(letter));
          myNex.writeStr("zxdb.message.txt", "Capturing for letter: <" + String(letter) + ">");

          char letter = searchChain.charAt(i);
          logln("Generating files list for letter: " + String(letter));
          
          // Buscamos. Actualizamos el subpath y ruta
          const char* subpath = &letter;
          dir = String(letter);

          // Vemos si existe el subpath. Si no existe, lo creamos.
          if (!SD_MMC.exists("/ONLINE/" + dir)) 
          {
            logln("Creating directory /ONLINE/" + dir);
            if(!SD_MMC.mkdir("/ONLINE/" + dir))
            {
              logln("Error al crear /ONLINE/" + dir);
              continue;
            }
          }

          // Truncamos/creamos _files.lst (vacía) antes del bucle HTTP
          {
            File f = SD_MMC.open("/ONLINE/" + dir + "/_files.lst", FILE_WRITE);
            if (!f)
            {
              logln("No se pudo crear _files.lst: /ONLINE/" + dir + "/_files.lst");
              myNex.writeStr("zxdb.message.txt", "Error on _files.lst");
              continue;
            }
            f.close();
          }

          // Truncamos/creamos _files.inf (vacía) antes del bucle HTTP, igual que _files.lst
          {
            File f = SD_MMC.open("/ONLINE/" + dir + "/_files.inf", FILE_WRITE);
            if (!f)
            {
              logln("No se pudo crear _files.inf: /ONLINE/" + dir + "/_files.inf");
              myNex.writeStr("zxdb.message.txt", "Error on _files.inf");
              continue;
            }
            f.close();
          }

          // Inicializamos parametros
          int nrows = 100;
          int npages = 0;
          int count = 0;
          int lineNum = 0;  // Contador global de líneas a través de todas las páginas

          // Componemos la URL de busqueda
          if (letter == '#') 
            // Buscamos juegos que empiezan por un número.
            urlToSearch = "https://api.zxinfo.dk/v3/games/byletter/%23?contenttype=SOFTWARE&machinetype=ZXSPECTRUM&output=flat";
          else
            // Buscamos juegos que empiezan por letras
            urlToSearch = "https://api.zxinfo.dk/v3/games/byletter/" + String(letter) + "?contenttype=SOFTWARE&machinetype=ZXSPECTRUM&output=flat";

          // Cogemos el total de items
          int total = get_total_files_ZXDB(urlToSearch.c_str());
          logln("Total " + String(total) + " files in ZXDB");

          //
          // Ahora bucle para coger todos los items de cada letra
          //
          npages = total / nrows;
          if (total % nrows > 0) npages++;
          logln("Total pages to capture: " + String(npages));

          while (count < npages)
          {
            logln("Page: " + String(count+1) + " of " + String(npages));

            String linesBuffer = "";  // Buffer para acumular líneas antes de escribir en SD

            HTTPClient http;
            String url = String(urlToSearch.c_str()) + "&size=" + String(nrows) + "&offset=" + String(count);
            http.begin(url.c_str());
            int httpCode = http.GET();
              
            if (httpCode == 200) 
            {
              String payload = http.getString();
              Serial.print("Payload length: ");
              Serial.println(payload.length());

              int pageHit = 0;  // Siempre empieza en 0 para cada página

              while (true) 
              {
                String idKey = "hits." + String(pageHit) + "._id=";
                String titleKey = "hits." + String(pageHit) + ".title=";
                int idPos = payload.indexOf(idKey);
                int titlePos = payload.indexOf(titleKey);

                if (idPos == -1 || titlePos == -1) break;
                  
                int idValStart = idPos + idKey.length();
                int idValEnd = payload.indexOf('\n', idValStart);
                String id = payload.substring(idValStart, idValEnd);
                  
                int titleValStart = titlePos + titleKey.length();
                int titleValEnd = payload.indexOf('\n', titleValStart);
                String title = payload.substring(titleValStart, titleValEnd);
                  
                id.trim(); 
                title.trim();
                  
                if (id.length() != 0 && title.length() != 0)
                {
                  //logln("[" + String(lineNum) + "] Found: " + title + " (ID: " + id + ")");
                  linesBuffer += String(lineNum) + "|F|0|" + title + ".zxdb|" + id + "\n";
                  pageHit++;
                  lineNum++;
                }
                else break;
              }
            } 
            else 
            {
              Serial.print("Error HTTP: ");
              Serial.println(httpCode);
            }
            http.end();  // Cerramos HTTP ANTES de escribir en SD

            // Escribimos en SD una vez cerrada la conexión HTTP
            if (linesBuffer.length() > 0)
            {
              File lst = SD_MMC.open("/ONLINE/" + dir + "/_files.lst", FILE_APPEND);
              if (lst)
              {
                lst.print(linesBuffer);
                lst.flush();
                lst.close();
                Serial.print("Written to _files.lst: ");
                Serial.print(linesBuffer.length());
                Serial.println(" bytes");
              }
              else
              {
                logln("Error al abrir _files.lst para FILE_APPEND");
              }
            }

            // Banda de progreso
            if (totalItemsFound > 0) myNex.writeNum("zxdb.j0.val", (count * nrows * 100) / totalItemsFound);  
            count++;
          }
          
          // Crear el fichero .inf (abrimos ahora, después del bucle HTTP)
          {
            File inf = SD_MMC.open("/ONLINE/" + dir + "/_files.inf", FILE_WRITE);
            if (inf) 
            {
              inf.println("PATH=/ONLINE/" + dir + "/");
              inf.print("CFIL=");
              inf.println(total);
              inf.println("CDIR=0");
              inf.flush();
              inf.close();
              logln("Closed _files.inf");
            } 
            else 
            {
              logln("No se pudo abrir _files.inf para escribir: /ONLINE/" + dir + "/_files.inf");
              myNex.writeStr("zxdb.message.txt", "Error on _files.inf");
            }
          }
          logln("_files.lst y _files.inf generados correctamente desde ZXDB flat");

          // Mensaje de finalización
          myNex.writeStr("zxdb.message.txt", "Capturing finished");
          myNex.writeNum("zxdb.j0.val", 100);
        }
    }
}
