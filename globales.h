// Variables GLOBALES a todo el proyecto
    struct tBlock
    {
        int index;                      // Numero del bloque
        int offset = 0;                 // Byte donde empieza
        byte* header = NULL;                   // Cabecera del bloque
        byte* data = NULL;                     // Datos del bloque
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
        char* name;                    // Nombre del TAP
        int size;                       // Tamaño
        int numBlocks;                  // Numero de bloques
        tBlockDescriptor* descriptor = NULL;   // Descriptor
    };

tTAP globalTAP;

// Tamaño del fichero abierto
int FILE_LENGTH = 0;

// Turbo mode
// 0 - No
// 1 - Yes
bool TURBOMODE=false;

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
bool FILE_SELECTED = false;
String FILE_PATH_SELECTED = "";

//Estado de acciones de reproducción
const int PLAY_ST = 0;
const int STOP_ST = 1;
const int PAUSE_ST = 2;
const int READY_ST = 3;
const int END_ST = 4;
const int ACK_LCD = 5;
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

