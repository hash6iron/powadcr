// Variables GLOBALES a todo el proyecto
    struct tBlock
    {
        int index;                      // Numero del bloque
        int offset = 0;                 // Byte donde empieza
        byte* header;                   // Cabecera del bloque
        byte* data;                     // Datos del bloque
    };

    struct tBlockDescriptor
    {
        int offset;
        int size;
        int chk;
        String name;
        bool nameDetected;
        bool header;
        bool screen;
        int type;
    };

    // Estructura tipo TAP
    struct tTAP
    {
        String name;                    // Nombre del TAP
        int size;                       // Tamaño
        int numBlocks;                  // Numero de bloques
        tBlockDescriptor* descriptor;   // Descriptor
    };

// Tamaño del fichero abierto
int FILE_LENGTH = 0;


// Estados de gestión de la reproducción
int LOADING_STATE = 0;
int CURRENT_BLOCK_IN_PROGRESS = 0;
int BLOCK_SELECTED = 0;

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

// Controlador de eventos global
ObserverEvents evManager;