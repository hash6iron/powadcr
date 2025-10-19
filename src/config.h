/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: config.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Fichero de configuración
   
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

//
// Dependency Graph works fine with v1.0r6 (06/10/2025)
//
// |-- WiFi @ 2.0.0
// |-- WiFiClientSecure @ 2.0.0
// |-- Easy Nextion Library @ 1.0.6+sha.4bd06b2
// |-- ESP32-A2DP @ 1.8.8+sha.e86ab93
// |-- audio-driver @ 0.1.4+sha.c8e422b
// |-- audio-tools @ 1.2.0+sha.0ad0a61
// |-- libhelix @ 0.8.9+sha.0d77734
// |-- libflac @ 1.3.4+sha.2618e4d
// |-- libopus @ 1.3.2+sha.bae0f85
// |-- ESP32_FTPSERVER_SD @ 0.0.0+20251006181015.sha.0f6d214
// |-- libfoxenflac @ 1.0.0+sha.e491d2e
// |-- SPI @ 2.0.0
// |-- SD @ 2.0.0
// |-- FS @ 2.0.0
// |-- SD_MMC @ 2.0.0
// |-- Wire @ 2.0.0
// |-- Update @ 2.0.0

// --------------------------------------------------------------
// Configuración de memoria optimizada
// --------------------------------------------------------------

// #define CONFIG_SPIRAM_SUPPORT 1
// #define CONFIG_SPIRAM_USE_MALLOC 1
// #define CONFIG_SPIRAM_TYPE_AUTO 1

// // Reducir uso de IRAM
// #define CONFIG_ESP32_WIFI_IRAM_OPT 0
// #define CONFIG_ESP32_WIFI_RX_IRAM_OPT 0

// // Optimizar stack sizes
// #ifndef CONFIG_ARDUINO_LOOP_STACK_SIZE
// #define CONFIG_ARDUINO_LOOP_STACK_SIZE 8192
// #endif

// --------------------------------------------------------------
// Configuración necesaria para powaDCR
// --------------------------------------------------------------

// Definimos la ganancia de la entrada de linea (para RECORDING)
#define WORKAROUND_ES8388_LINE1_GAIN MIC_GAIN_MAX
#define WORKAROUND_ES8388_LINE2_GAIN MIC_GAIN_MAX

// Mezcla LINE 1 and LINE 2.
#define WORKAROUND_MIC_LINEIN_MIXED false

// Esencial para poder variar la ganancia de salida de HPLINE y ROUT, LOUT
#define AI_THINKER_ES8388_VOLUME_HACK 1

// For SDFat >2.3.0 version
#define FILE_COPY_CONSTRUCTOR_SELECT FILE_COPY_CONSTRUCTOR_PUBLIC

// Enable / Disable Audiokit logging
#define USE_AUDIO_LOGGING false
// States of LOG_LEVEL: Debug, Info, Warning, Error
#define LOG_LEVEL AudioLogger::Error
// Definicion del puerto serie para la pantalla
#define SerialHWDataBits 921600
#define hmiTxD 23
#define hmiRxD 18
#define powerLed 22
#define GPIO_MSX_REMOTE_PAUSE 19
#define WDT_TIMEOUT 360000

// --------------------------------------------------------------
// Configuración del sistema
// --------------------------------------------------------------

#define VERSION "1.0r6.3"
#define MACHINE_ZX

// Define sampling rate a 44.1KHz. En otro caso será a 32KHz
//#define SAMPLING44
//#define SAMPLING48
//#define SAMPLING32
//#define SAMPLING22

// Activa el test de sampling para calibrado de la salida ES83883
//#define SAMPLINGTESTACTIVE

// Descomentar para activar el modo debug
//#define DEBUGMODE

// Descomentar para test de reproducción en memoria
//#define TEST

#define SD_CHIP_SELECT 13 // Pin CS de la SD

// --------------------------------------------------------------
// Interfaz
// --------------------------------------------------------------
// CFG_FORZE_SINC_HMI = false --> No espera sincronizar con HMI
// CFG_FORZE_SINC_HMI = true  --> Espera sincronizar con HMI
#define CFG_FORZE_SINC_HMI true

// --------------------------------------------------------------
// Browser
// --------------------------------------------------------------
#define TOTAL_FILES_IN_BROWSER_PAGE 13  // No se cuenta la primera fila que es para el path
#define MAX_FILES_TO_LOAD           14  // Esto siempre es TOTAL_FILES_IN_BROWSER_PAGE + 1
#define MAX_BLOCKS_IN_BROWSER       13
#define MAX_FILES_FOUND_BUFF        255
#define EACH_FILES_REFRESH          5   // Cada n ficheros refresca el marcador. Por defecto 50

// Colores del browser
#define DEFAULT_COLOR       65535
#define DIR_COLOR           60868
#define DSC_FILE_COLOR      0
#define FAVORITE_FILE_COLOR 34815
#define SPECIAL_FILE_COLOR  44405
#define OTHER_FILES_COLOR   44405

// --------------------------------------------------------------
// Player / SD
// -------------------------------------------------------------------
// Frecuencia inicial de la SD
#define SD_FRQ_MHZ_INITIAL                            40000   // 40 MHz (velocidad de acceso a la SD)

// Sampling rate TAP/TZX/..
#define DEFAULT_MP3_SAMPLING_RATE                     44100
#define STANDARD_SR_8_BIT_MACHINE                     21000   // Sampling rate adecuado para maquinas de 8 bitsAjuste AZIMUT (Hz) - 22200 Hz
#define STANDARD_SR_REC_ZX_SPECTRUM                   22200   // Sampling rate adecuado para ZX Spectrum (recorder) - Ajuste AZIMUT (Hz) - 22200 Hz

// Sampling rate para WAV y REC WAV (pero ojo, no para PLAY TO WAV)
#define DEFAULT_WAV_SAMPLING_RATE                     44100
#define DEFAULT_WAV_SAMPLING_RATE_REC                 44100
#define DEFAULT_8BIT_WAV_SAMPLING_RATE_REC            22050

// Porcentaje de avance rapido
#define FAST_FORWARD_PER                              0.02     
#define FAST_REWIND_PER                               0.02     

// Demora en ms para saltar a avance super-rapido
#define TIME_TO_FAST_FORWRD                           1500

// Tiempo para volver al principio o pista anterior
#define TIME_MAX_TO_PREVIOUS_TRACK                    5000

// Pausa entre saltos de avance rápido en ms
#define DELAY_ON_EACH_STEP_FAST_FORWARD               125
#define DELAY_ON_EACH_STEP_FAST_REWIND                125

// Tone adjustment for ZX Spectrum - Samples
#define TONE_ADJUSTMENT_ZX_SPECTRUM                   0.0
#define TONE_ADJUSTMENT_ZX_SPECTRUM_LIMIT             5

// Tiempo para mostrar el tiempo estimado
#define TIME_TO_SHOW_ESTIMATED_TIME                   40
#define PER_TO_SHOW_ESTIMATED_TIME                    3

// Maximo numero de ficheros capturados en la lista de audio por directorio
#define MAX_FILES_AUDIO_LIST                          512

// ---------------------------------------------------------------------------------------------
// NO COMENTAR SI SE ESTÁ USANDO 22200 Hz en otro caso hay que usar 44100 para recording
// Si se usa el sampling rate de ZX Spectrum (22.2KHz) para la grabación y reproducción de TAPs
#define USE_ZX_SPECTRUM_SR 1 
//
// ----------------------------------------------------------------------------------------------
// Particion mas pequeña de un silencio
#define MIN_FRAME_FOR_SILENCE_PULSE_GENERATION 256

// --------------------------------------------------------------
// Recording
// -------------------------------------------------------------------

// Comentar esta linea para hacer uso de PCM
//#define USE_ADPCM_ENCODER

// --------------------------------------------------------------
// TAP config.
// --------------------------------------------------------------
// Acorta el tono guia del bloque data despues de header
#define LEVEL_REDUCTION_HEADER_TONE_IN_TAP 1

// Activa el modo de split de los bloques. 
// si superan el tamaño (en bytes) definido por SIZE_TO_ACTIVATE_SPLIT
#define SPLIT_ENABLED 0
#define SIZE_TO_ACTIVATE_SPLIT 20000
#define SIZE_FOR_SPLIT 10000

// Maximo número de bloques para el descriptor.
#define MAX_BLOCKS_IN_TAP 4000
#define MAX_BLOCKS_IN_TZX 4000

// Configuracion del test in/out
bool TEST_LINE_IN_OUT = false;

// -------------------------------
//
// OTA setting
//
// -------------------------------
// Dejar todo esto tal como está. La configuración se hace en el fichero wifi.cfg
//
// Los parámetros a incluir serán estos a continuación y en ese orden:
//
// Ejemplo:
//
// <hostname>powaDCR</hostname>
// <ssid>miAPWiFi</ssid>
// <password>miPasswordDelAp</password>
// <IP>192.168.2.28</IP>
// <SN>255.255.255.0</SN>
// <GW>192.168.2.1</GW>
// <DNS1>192.168.2.1</DNS1>
// <DNS2>192.168.2.1</DNS2>

char HOSTNAME[32] =  {};
String ssid = "";
char password[64] = {};

// Static IP - 2.4GHz WiFi AP
IPAddress local_IP(0, 0, 0, 0); // Your Desired Static IP Address
IPAddress subnet(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress primaryDNS(0, 0, 0, 0); // Not Mandatory
IPAddress secondaryDNS(0, 0, 0, 0);     // Not Mandatory

// HMI
#define windowNameLength    32
#define windowNameLengthFB  50
#define tRotateNameRfsh     230

// Limitador de volumen
#define MAX_VOL_FOR_HEADPHONE_LIMIT 40

// Recursos opcionales
//#define BLUETOOTH_ENABLE
#define FTP_SERVER_ENABLE

// Control remoto de PAUSA
#define MSX_REMOTE_PAUSE

// Parametros internos
#define MAIN_VOL_FACTOR 100



