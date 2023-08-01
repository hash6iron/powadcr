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

// ZX Proccesor config
// ********************************************************************
//
#ifdef MACHINE_ZX
  const int DPILOT_HEADER = 8063;
  const int DPILOT_DATA = 3223;
  
  // Timming estandar de la ROM
  float DfreqCPU = 3500000.0;
  int DSYNC1 = 667;
  int DSYNC2 = 735;
  int DBIT_0 = 855;
  int DBIT_1 = 1710;
  int DPULSE_PILOT = 2168;
  int DPILOT_TONE = DPILOT_HEADER;
  
  // Definición del silencio entre bloques en ms
  int DSILENT = 1500;
#endif

// Inicializadores para los char*
char INITCHAR[] = "\0";
char INITCHAR2[] = "..\0";
char INITFILEPATH[] = "/\0";
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

// Estructura de un bloque
struct tBlock 
{
  int index = 0;            // Numero del bloque
  int offset = 0;           // Byte donde empieza
  byte* header = NULL;      // Cabecera del bloque
  byte* data = NULL;        // Datos del bloque
};

// Descriptor de bloque de un TAP
struct tBlockDescriptor 
{
  int offset = 0;
  int size = 0;
  int chk = 0;
  char* name = &INITCHAR[0];
  bool nameDetected = false;
  bool header = false;
  bool screen = false;
  int type = 0;
  char* typeName = &INITCHAR[0];
};

// Descriptor de bloque de un TZX
    // Timming de la ROM
    // SYNC1 = 667;
    // SYNC2 = 735;
    // BIT_0 = 855;
    // BIT_1 = 1710;
    // PULSE_PILOT = 2168;
    // PULSE_PILOT_HEADER = PULSE_PILOT * 8063;
    // PULSE_PILOT_DATA = PULSE_PILOT * 3223;

struct tTimming
{
  int bit_0 = 855;
  int bit_1 = 1710;
  int pulse_pilot = 2168;
  int pilot_tone = 8063;
  int pilot_header = 8063;
  int pilot_data = 3223;
  int sync_1 = 667;
  int sync_2 = 735;
};

struct tTZXBlockDescriptor 
{
  uint ID = 0;
  uint offset = 0;
  uint size = 0;
  uint chk = 0;
  uint pauseAfterThisBlock = 1000;   //ms
  uint lengthOfData = 0;
  uint offsetData = 0;
  char* name = &INITCHAR[0];
  bool nameDetected = false;
  bool header = false;
  bool screen = false;
  uint type = 0;
  bool playeable = false;
  byte maskLastByte = 8;
  tTimming timming;
  char* typeName = &INITCHAR[0];
};

// Estructura tipo TAP
struct tTAP 
{
  char* name = &INITCHAR[0];                // Nombre del TAP
  int size = 0;                             // Tamaño
  int numBlocks = 0;                        // Numero de bloques
  tBlockDescriptor* descriptor = NULL;      // Descriptor
};

// Estructura tipo TZX
struct tTZX
{
  char* name = &INITCHAR[0];                // Nombre del TZX
  int size = 0;                             // Tamaño
  int numBlocks = 0;                        // Numero de bloques
  tTZXBlockDescriptor* descriptor = NULL;   // Descriptor
};

// Estructura para el HMI
struct tFileBuffer
{
    bool isDir = false;
    String path = "";
    String type = "";
};

// Tamaño del fichero abierto
int FILE_LENGTH = 0;

// Turbo mode
// 0 - No
// 1 - Yes
bool TURBOMODE = false;

// Variables para intercambio de información con el HMI

bool TEST_RUNNING = false;
int LOADING_STATE = 0;
int CURRENT_BLOCK_IN_PROGRESS = 0;
int BLOCK_SELECTED = 0;
String TYPE_FILE_LOAD = "";
char* LAST_NAME = &INITCHAR[0];
char* LAST_TYPE = &INITCHAR[0];
String LAST_MESSAGE = "";
String PROGRAM_NAME = "";
int LAST_SIZE = 0;
int BYTES_LOADED = 0;
int BYTES_TOBE_LOAD = 0;
int BYTES_LAST_BLOCK = 0;
int TOTAL_BLOCKS = 0;
// Screen
bool SCREEN_LOADING = 0;
int SCREEN_LINE = 0;
int SCREEN_COL = 0;
int SCREEN_SECTION = 0;

// File system
int FILE_INDEX = 0;           // Índice de la fila seleccionada
int FILE_PAGE = 0;            // Contador de la pagina leida
char* FILE_PATH = &INITCHAR[0];         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0;  // Nivel de profundida de directorio
char* FILE_LAST_DIR = &INITFILEPATH[0];
char* FILE_PREVIOUS_DIR = &INITFILEPATH[0];
int FILE_LAST_INDEX = 0;
int FILE_IDX_SELECTED = -1;

tFileBuffer* FILES_BUFF = NULL;
tFileBuffer* FILES_FOUND_BUFF = NULL;

String FILE_TO_LOAD = "";
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

// Variables de control de la reproducción
bool PLAY = false;
bool PAUSE = true;
bool REC = false;
bool STOP = false;
bool FFWIND = false;
bool RWIND = false;
bool UP = false;
bool DOWN = false;
bool LEFT = false;
bool RIGHT = false;
bool ENTER = false;
bool LCD_ON = false;

//Estado de acciones de reproducción
const int PLAY_ST = 0;
const int STOP_ST = 1;
const int PAUSE_ST = 2;
const int READY_ST = 3;
const int END_ST = 4;
const int ACK_LCD = 5;
const int RESET = 6;
//
int MAIN_VOL = 90;

// Gestion de menú
bool MENU = false;
bool menuOn = false;
int nMENU = 0;

// Variables locals
const int button1_GPIO = 36;
const int button2_GPIO = 13;
const int button3_GPIO = 19;
const int button4_GPIO = 23;
const int button5_GPIO = 18;
const int button6_GPIO = 5;

// Control de los botones
int button1 = 0;
int button2 = 0;
int button3 = 0;
int button4 = 0;
int button5 = 0;
int button6 = 0;

// Button state
bool button1_down = false;
bool button2_down = false;
bool button3_down = false;
bool button4_down = false;
bool button5_down = false;
bool button6_down = false;

bool button1_pressed = false;
bool button2_pressed = false;
bool button3_pressed = false;
bool button4_pressed = false;
bool button5_pressed = false;
bool button6_pressed = false;

void getMemFree()
{
    Serial.println("");
    Serial.println("");
    Serial.println("> MEM REPORT");
    Serial.println("------------------------------------------------------------");
    Serial.println("");
    Serial.println("Total heap: " + String(ESP.getHeapSize() / 1024) + "KB");
    Serial.println("Free heap: " + String(ESP.getFreeHeap() / 1024) + "KB");
    Serial.println("Total PSRAM: " + String(ESP.getPsramSize() / 1024 / 1024) + "MB");
    Serial.println("Free PSRAM: " + String (ESP.getFreePsram() / 1024 / 1024) + "MB");  
}
