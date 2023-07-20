/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: config.h
    
    Creado por:
      Antonio Tamairón. 2023  
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Fichero de variables, estructuras globales
   
    Version: 1.0

    Historico de versiones

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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
  char* name = "\0";
  bool nameDetected = false;
  bool header = false;
  bool screen = false;
  int type = 0;
  char* typeName = "\0";
};

// Descriptor de bloque de un TZX
struct tTZXBlockDescriptor 
{
  int offset = 0;
  int size = 0;
  int chk = 0;
  char* name = "\0";
  bool nameDetected = false;
  bool header = false;
  bool screen = false;
  int type = 0;
  char* typeName = "\0";
};

// Estructura tipo TAP
struct tTAP 
{
  char* name = "\0";                           // Nombre del TAP
  int size = 0;                             // Tamaño
  int numBlocks = 0;                        // Numero de bloques
  tBlockDescriptor* descriptor = NULL;  // Descriptor
};

// Estructura tipo TZX
struct tTZX
{
  char* name = "\0";                           // Nombre del TAP
  int size = 0;                             // Tamaño
  int numBlocks = 0;                        // Numero de bloques
  tTZXBlockDescriptor* descriptor = NULL;  // Descriptor
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
char* LAST_NAME = "\0";
char* LAST_TYPE = "\0";
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
char* FILE_PATH = "\0";         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0;  // Nivel de profundida de directorio
char* FILE_LAST_DIR = "/\0";
char* FILE_PREVIOUS_DIR = "/\0";
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
