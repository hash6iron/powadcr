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

// Machine
//
// 0 for ZX Spectrum

#define VERSION "v0.300424.1"
#define MACHINE_ZX

// Define sampling rate a 44.1KHz. En otro caso será a 32KHz
//#define SAMPLING44
//#define SAMPLING48
//#define SAMPLING32
#define SAMPLING22

// Activa el test de sampling para calibrado de la salida ES83883
//#define SAMPLINGTESTACTIVE

// Para activar el modo debug
//#define DEBUGMODE

//#define MACHINE_AMSTRAD

// Descomentar para test de reproducción en memoria
//#define TEST

// Define nivel de log
// 0 - Apagado
// 1 - Essential
// 2 - Error
// 3 - Warning
// 4 - Info     / Full trace info
#define LOGPW 0

// Inserta codigo de trazabilidad para desarrollo
#define TESTDEV 

// Interfaz
// --------------------------------------------------------------
// CFG_FORZE_SINC_HMI = false --> No espera sincronizar con HMI
// CFG_FORZE_SINC_HMI = true  --> Espera sincronizar con HMI
#define CFG_FORZE_SINC_HMI true

// Browser
// --------------------------------------------------------------
// Numero máximo de ficheros que se listan por directorio.
#define MAX_FILES_TO_LOAD 1000
#define MAX_FILES_FOUND_BUFF 100
// Cada n ficheros refresca el marcador. Por defecto 5
#define EACH_FILES_REFRESH 5
// Numero total de lineas de una pagina del file browser
#define TOTAL_FILES_IN_BROWSER_PAGE 13

// Player / SD
// -------------------------------------------------------------------
// Frecuencia inicial de la SD
#define SD_FRQ_MHZ_INITIAL 40


// TAP config.
// ********************************************************************
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

char* HOSTNAME = "";
char* ssid = "";
char* password = "";

// Static IP - 2.4GHz WiFi AP
IPAddress local_IP(0, 0, 0, 0); // Your Desired Static IP Address
IPAddress subnet(0, 0, 0, 0);
IPAddress gateway(0, 0, 0, 0);
IPAddress primaryDNS(0, 0, 0, 0); // Not Mandatory
IPAddress secondaryDNS(0, 0, 0, 0);     // Not Mandatory



