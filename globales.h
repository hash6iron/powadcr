// Variables GLOBALES a todo el proyecto

struct tBlock 
{
  int index;            // Numero del bloque
  int offset = 0;       // Byte donde empieza
  byte* header = NULL;  // Cabecera del bloque
  byte* data = NULL;    // Datos del bloque
};

struct tBlockDescriptor 
{
  int offset;
  int size;
  int chk;
  char* name;
  bool nameDetected;
  bool header;
  bool screen;
  int type;
  char* typeName;
};

// Estructura tipo TAP
struct tTAP 
{
  char* name;                           // Nombre del TAP
  int size;                             // Tamaño
  int numBlocks;                        // Numero de bloques
  tBlockDescriptor* descriptor = NULL;  // Descriptor
};

// Estructura de un buffer de ficheros y directorios
// struct tFile 
// {
//   String path;                  // Ruta del fichero completa
//   char* parentDir;              // Directorio donde está el fichero
//   char* previousDir;            // Directorio anterior
//   int indexOfPreviousDir;       // Indice del array donde está el directorio
//   int indexOfParentDir;         // Indice del directorio actual
// };

// struct tDir 
// {
//   int totalFiles;               // Numero total de ficheros dentro del dir.
//   int indexOfPreviousDir;       // Indice del array donde está el directorio
//   int indexOfParentDir;         // Indice del directorio actual
//   char* dirPreviousPath;        // Ruta del directorio anterior
//   char* dirPath;                // Ruta de este directorio
//   bool scaned = false;          // Indica si el dir. ha sido escaneado
//   tFile* fileBuffer = NULL;     // Buffer de ficheros
//   tDir*  dirBuffer = NULL;      // Buffer de directorios
// };

struct tFileBuffer
{
    bool isDir;
    int dirPos;
    String path;
    String type;
};

tTAP globalTAP;

// Tamaño del fichero abierto
int FILE_LENGTH = 0;

// Turbo mode
// 0 - No
// 1 - Yes
bool TURBOMODE = false;

// Variables para intercambio de información con el HMI
int LOADING_STATE = 0;
int CURRENT_BLOCK_IN_PROGRESS = 0;
int BLOCK_SELECTED = 0;
char* LAST_NAME = "";
char* LAST_TYPE = "";
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
char* FILE_PATH = "";         // Ruta del archivo seleccionado
int FILE_LAST_DIR_LEVEL = 0;  // Nivel de profundida de directorio
char* FILE_LAST_DIR = "/";
char* FILE_PREVIOUS_DIR = "/";
int FILE_LAST_INDEX = 0;
int FILE_IDX_SELECTED = -1;
tFileBuffer* FILES_BUFF = NULL;
String FILE_TO_LOAD = "";
String FILE_DIR_TO_CHANGE = "";
int FILE_PTR_POS = 0;
int FILE_TOTAL_FILES = 0;
int FILE_STATUS = 0;
bool FILE_NOTIFIED = false;
bool FILE_SELECTED = false;
String FILE_PATH_SELECTED = "";
bool FILE_DIR_OPEN_FAILED = false;

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
