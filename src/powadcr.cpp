/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: powadcr.ino

    Creado por:
      Copyright (c) Antonio Tamairón. 2023 [@hash6iron]
      https://github.com/hash6iron/powadcr
      https://powagames.itch.io/

    Agradecimientos:
      - Kike Martín [kyv]
      - Carlos Martinez [TxarlyM69]
      - Fernando Mosquera
      - Isra
      - Mario J
      - Jose [Xosmar]
      - Victor [Eremus]
      - Guillermo
      - Pedro Pelaez
      - Carlos Palmero [shaeon]

    Descripción:
    Programa principal del firmware del powadcr - Powa Digital Cassette
 Recording

    Version: 1.0

    Historico de versiones
    v.0.1 - Version de pruebas. En desarrollo
    v.0.2 - New update to develop branch


    Configuración en platformIO
    ---------------------------
    Ver en github - readme

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

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// Includes
// ===============================================================
#include "config.h"

#include <Arduino.h>
#include <FS.h>
#include <ESP32Time.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//#include <WiFiUdp.h>
#include <Wire.h>
#include <vector>

#include "EasyNextionLibrary.h"
#include "esp32-hal-psram.h"
ESP32Time rtc;

//#include "NTPClient.h"
// WiFiUDP ntpUDP;
// NTPClient timeclient(ntpUDP);

// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
TaskHandle_t Task0;
TaskHandle_t Task1;

#include <esp_task_wdt.h>

#include <HardwareSerial.h>
HardwareSerial SerialHW(2);

EasyNex myNex(SerialHW);

#include "globales.h"
//
#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"


// Definimos la placa de audio
#include "AudioTools/AudioCodecs/CodecFLACFoxen.h"
//#include "AudioTools/AudioCodecs/CodecFLAC.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "AudioTools/CoreAudio/AudioFilter/Equalizer3Bands.h"
#include "AudioTools/Disk/AudioSourceIdxSDMMC.h"
#include "AudioTools/Disk/AudioSourceURL.h"
// #include "AudioTools/CoreAudio/AudioLoggerSTD.h"

#ifdef BLUETOOTH_ENABLE
#include "AudioTools/Communication/A2DPStream.h"
#endif

// Definimos el stream para Audiokit
DriverPins powadcr_pins;
AudioBoard powadcr_board(audio_driver::AudioDriverES8388, powadcr_pins);
AudioBoardStream kitStream(powadcr_board);

// SpotifyESP32
#include <SpotifyEsp32.h>
Spotify* sp;

#include "HMI.h"
HMI hmi;

// #include "interface.h"
File wavfile;
File audioFile;

// ADPCM
#ifdef USE_ADPCM_ENCODER
ADPCMEncoder adpcm_encoder(AV_CODEC_ID_ADPCM_MS, ADAPCM_DEFAULT_BLOCK_SIZE);
WAVEncoder wav_encoder(adpcm_encoder, AudioFormat::ADPCM);
EncodedAudioStream encoderOutWAV(&wavfile, &wav_encoder);
#else
// PCM
WAVEncoder wavEncoder;
EncodedAudioStream encoderOutWAV(&wavfile, &wavEncoder);
#endif

#include "ZXProcessor.h"

// ZX Spectrum. Procesador de audio output
ZXProcessor zxp;

// Procesadores de cinta
// #include "BlockProcessor.h"
#include "PZXprocessor.h"
#include "TAPprocessor.h"
#include "TZXprocessor.h"
#include "miniz.h"

// File sdFile;
// File sdFile32;

// Creamos los distintos objetos
PZXprocessor pPZX;
TZXprocessor pTZX;
TAPprocessor pTAP;

// Procesador de audio input
#include "TAPrecorder.h"
TAPrecorder taprec;

bool last_headPhoneDetection = false;

bool taskStop = true;
bool pageScreenIsShown = false;

#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif

// SmartRadioBuffer
// -----------------------------------------------------------------------
#include "SimpleCircularBuffer.h"
// #include "SmartRadioBuffer.h"
// #include "PredictiveRadioBuffer.h"

// SPIFFS
// -----------------------------------------------------------------------
// #include "esp_err.h"
// #include "esp_spiffs.h"

#ifdef FTP_SERVER_ENABLE
#include "ESP32FtpServer.h"
FtpServer ftpSrv;
#endif



#ifdef WEB_SERVER_ENABLE
WiFiServer server(80);
String header;
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
#endif

using namespace audio_tools;

static String lastArtist;
static String lastTrackname;


// Variables
//
// -----------------------------------------------------------------------
int posRotateName = 0; // Posicion del texto rotativo en tape.name.txt (HMI)
int moveDirection = 1;
bool rotate_enable = false;

int plLastSize = 0;
int plLastpos = 0;
int plLastFsize = 0;
uint8_t plduty = 0;
uint8_t chduty = 1;
String plLastName = "";

bool statusTXLine = false;
bool statusRXLine = false;
// -----------------------------------------------------------------------
// Prototype function

void ejectingFile();
void isGroupStart();
void isGroupEnd();
void getRandomFilenameWAV(char *&currentPath, String currentFileBaseName);
void rewindAnimation(int direction);
void showOption(String id, String value);
String getFileNameFromPath(const String &filePath);
String removeExtension(const String &filename);

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
bool statusPoweLed = false;
bool powerLedFixed = false;

uint8_t lastKeyValue = 0;
uint8_t keyStatus = 0;
int keykp_count[7];


// Bluetooth
#ifdef BLUETOOTH_ENABLE
BluetoothA2DPSource a2dp;
SynchronizedNBuffer buffer(1024, 30, portMAX_DELAY, 10);
QueueStream<uint8_t> outbt(buffer); // convert Buffer to Stream

// Provide data to A2DP
int32_t get_data(uint8_t *data, int32_t bytes) {
  size_t result_bytes = buffer.readArray(data, bytes);
  // LOGI("get_data_channels %d -> %d of (%d)", bytes, result_bytes ,
  // buffer.available());
  return result_bytes;
}
#endif


void actuatePowerLed(uint8_t state) {
  // analogWrite(powerLed, dutyEnd);
  // mcp1.digitalWrite(8, mcp0.digitalRead(0));
  // if (!ENABLE_POWER_LED)
  //   state = LOW;

  if (!MCP23017_AVAILABLE) {
    // Salida por el pin #powerLed (pinMode se configura en setup)
    //digitalWrite(powerLed, state ? LOW : HIGH);
    pinMode(powerLed, OUTPUT);
    digitalWrite(powerLed, state ? LOW : HIGH);
    //analogWrite(powerLed, state * 255);    
  } else {
    MCP23017_writePin(MCP_LED_IO_PIN, state, I2C_MCP23017_ADDR);
  }
}

void freeMemoryFromDescriptorTZX(tTZXBlockDescriptor *descriptor) {
  // Vamos a liberar el descriptor completo
  for (int n = 0; n < TOTAL_BLOCKS; n++) {
    switch (descriptor[n].ID) {
    case 19: // bloque 0x13
      // Hay que liberar el array
      // delete[] descriptor[n].timming.pulse_seq_array;
      free(descriptor[n].timming.pulse_seq_array);
      break;

    case 25: // bloque 0x19 - GDB
      // Liberar symDefPilot y sus pulse_array internos
      if (descriptor[n].symbol.symDefPilot != nullptr) {
        for (int i = 0; i < descriptor[n].symbol.ASP; i++) {
          if (descriptor[n].symbol.symDefPilot[i].pulse_array != nullptr) {
            free(descriptor[n].symbol.symDefPilot[i].pulse_array);
            descriptor[n].symbol.symDefPilot[i].pulse_array = nullptr;
          }
        }
        free(descriptor[n].symbol.symDefPilot);
        descriptor[n].symbol.symDefPilot = nullptr;
      }

      // Liberar pilotStream
      if (descriptor[n].symbol.pilotStream != nullptr) {
        free(descriptor[n].symbol.pilotStream);
        descriptor[n].symbol.pilotStream = nullptr;
      }

      // Liberar symDefData y sus pulse_array internos
      if (descriptor[n].symbol.symDefData != nullptr) {
        for (int i = 0; i < descriptor[n].symbol.ASD; i++) {
          if (descriptor[n].symbol.symDefData[i].pulse_array != nullptr) {
            free(descriptor[n].symbol.symDefData[i].pulse_array);
            descriptor[n].symbol.symDefData[i].pulse_array = nullptr;
          }
        }
        free(descriptor[n].symbol.symDefData);
        descriptor[n].symbol.symDefData = nullptr;
      }

      // Liberar dataStream
      if (descriptor[n].symbol.dataStream != nullptr) {
        free(descriptor[n].symbol.dataStream);
        descriptor[n].symbol.dataStream = nullptr;
      }
      break;

    default:
      break;
    }
  }
}

int *strToIPAddress(String strIPAddr) {
  int *ipnum = new int[4];
  int wc = 0;

  // Parto la cadena en los distintos numeros que componen la IP
  while (strIPAddr.length() > 0) {
    int index = strIPAddr.indexOf('.');
    // Si no se encuentran mas puntos es que es
    // el ultimo digito.
    if (index == -1) {
      ipnum[wc] = strIPAddr.toInt();
      return ipnum;
    } else {
      // Tomo el dato y lo paso a INT
      ipnum[wc] = (strIPAddr.substring(0, index)).toInt();
      // Elimino la parte ya leida
      strIPAddr = strIPAddr.substring(index + 1);
      wc++;
    }
  }

  // Si no hay nada para devolver, envio un 0.
  return 0;
}

String getValueOfParam(String line, String param) {
#ifdef DEBUGMODE
  logln("Cfg. line: " + line);
  log(" - Param: " + param);
#endif

  int firstClosingBracket = line.indexOf('>');

  if (firstClosingBracket != -1) {
    // int paramLen = param.length();
    int firstOpeningEndLine = line.indexOf("</", firstClosingBracket + 1);

    if (firstOpeningEndLine != -1) {
      String res = line.substring(firstClosingBracket + 1, firstOpeningEndLine);
#ifdef DEBUGMODE
      log(" / Value = " + res);
#endif
      return res;
    }
  }

  return "null";
}

tConfig *readAllParamCfg(File mFile, int maxParameters) {
  tConfig *cfgData = new tConfig[maxParameters + 1];

  // Vamos a ir linea a linea obteniendo la información de cada parámetro.
  char line[256];
  String tLine;
  //
  int n;
  int i = 0;

  // Vemos si el fichero ya está abierto
  if (mFile) {
    // read lines from the file
    while (mFile.available()) {
      tLine = mFile.readStringUntil('\n');
      tLine.toCharArray(line, sizeof(line));
      // remove '\n'
      line[n - 1] = 0;
      String strRes = "";

      strRes = line;
#ifdef DEBUGMODE
      logln("[" + String(i) + "]" + strRes);
#endif

      if (i <= maxParameters) {
        cfgData[i].cfgLine = strRes;
        cfgData[i].enable = true;
      } else {
        log("Out of space for cfg parameters. Max. " + String(maxParameters));
      }

      i++;
    }
  }

  return cfgData;
}

bool loadCfgFile() {

  bool cfgloaded = false;

  if (SD_MMC.exists("/powadcr.cfg")) 
  {

    #ifdef DEBUGMODE
        logln("File powadcr.cfg exists");
    #endif

    char pathCfgFile[32] = {};
    strcpy(pathCfgFile, "/powadcr.cfg");

    File fCfg = SD_MMC.open(pathCfgFile, FILE_READ);
    int *IP;

    if (fCfg) {
      char *ip1 = new char[17];
      char *param = new char[32];

      CFGSYSTEM = readAllParamCfg(fCfg, 100);

      // WiFi settings
      logln("");
      logln("");
      logln("");
      logln("");
      logln("");
      logln("Settings:");
      logln(
          "------------------------------------------------------------------");
      logln("");

      // Hostname
      strcpy(HOSTNAME, (getValueOfParam(CFGSYSTEM[0].cfgLine, "hostname")).c_str());
      logln(HOSTNAME);
      // SSID - Wifi
      ssid = (getValueOfParam(CFGSYSTEM[1].cfgLine, "ssid")).c_str();
      if (ssid.length() == 0) 
      {
        WIFI_ENABLE = false;
        saveHMIcfg("WIFIopt");
      }
      logln(ssid);
      // Password - WiFi
      strcpy(password, (getValueOfParam(CFGSYSTEM[2].cfgLine, "password")).c_str());
      logln(password);

      // Local IP
      strcpy(ip1, (getValueOfParam(CFGSYSTEM[3].cfgLine, "IP")).c_str());
      logln("IP: " + String(ip1));
      POWAIP = ip1;
      IP = strToIPAddress(String(ip1));
      local_IP = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // Subnet
      strcpy(ip1, (getValueOfParam(CFGSYSTEM[4].cfgLine, "SN")).c_str());
      logln("SN: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      subnet = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // gateway
      strcpy(ip1, (getValueOfParam(CFGSYSTEM[5].cfgLine, "GW")).c_str());
      logln("GW: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      gateway = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS1
      strcpy(ip1, (getValueOfParam(CFGSYSTEM[6].cfgLine, "DNS1")).c_str());
      logln("DNS1: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      primaryDNS = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS2
      strcpy(ip1, (getValueOfParam(CFGSYSTEM[7].cfgLine, "DNS2")).c_str());
      logln("DNS2: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      secondaryDNS = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      //MCP23017 (on/off)
      strcpy(param, (getValueOfParam(CFGSYSTEM[8].cfgLine, "MCP23017")).c_str());
      logln("MCP23017: " + String(param));
      String(param).toLowerCase();
      MCP23017_AVAILABLE = String(param) == "on" || String(param) == "1" ? true : false;

      // NTP-SERVER
      strcpy(param, (getValueOfParam(CFGSYSTEM[9].cfgLine, "NTPSERVER")).c_str());
      logln("NTP-SERVER: " + String(param));
      NTPSERVER = param;

      // NTP-TIMEZONE
      strcpy(param, (getValueOfParam(CFGSYSTEM[10].cfgLine, "TIMEZONE")).c_str());
      TIMEZONE = String(param).toInt();
      logln("TIMEZONE: " + String(TIMEZONE));

      // NTP-SUMMER TIME (on/off)
      strcpy(param, (getValueOfParam(CFGSYSTEM[11].cfgLine, "SUMMERTIME")).c_str());
      logln("SUMMERTIME: " + String(param));
      String(param).toLowerCase();
      SUMMERTIME = String(param) == "on" || String(param) == "1" ? true : false;

      // Spotify credentials (if exist in cfg file)
      // Spotify username
      strcpy(param, (getValueOfParam(CFGSYSTEM[12].cfgLine, "SPOTIFY")).c_str());
      logln("SPOTIFY Enable: " + String(param));
      SPOTIFY_EN = String(param) == "on" || String(param) == "1" ? true : false;;  

      SPOTIFY_CLIENT_ID = getValueOfParam(CFGSYSTEM[13].cfgLine, "SPOTIFY_CID");
      logln("SPOTIFY_CID: " + SPOTIFY_CLIENT_ID);
      //
      SPOTIFY_CLIENT_SECRET = getValueOfParam(CFGSYSTEM[14].cfgLine, "SPOTIFY_CSE");
      logln("SPOTIFY_CSE: " + SPOTIFY_CLIENT_SECRET);

      strcpy(param, (getValueOfParam(CFGSYSTEM[15].cfgLine, "QUICKBOOT")).c_str());
      logln("QUICK Boot: " + String(param));
      QUICK_BOOT = String(param) == "on" || String(param) == "1" ? true : false;

      strcpy(param, (getValueOfParam(CFGSYSTEM[15].cfgLine, "BEEP")).c_str());
      logln("BEEP: " + String(param));
      BEEP = String(param) == "on" || String(param) == "1" ? true : false;

      logln("");
      logln(
          "------------------------------------------------------------------");
      logln("");
      logln("");

      fCfg.close();
      cfgloaded = true;
    }
  } else {
    // Si no existe creo uno de referencia
    File fCfg;
    if (!SD_MMC.exists("/powadcr_ori.cfg")) {
      // fWifi.open("/wifi_ori.cfg", O_WRITE | O_CREAT);
      fCfg = SD_MMC.open("/powadcr_ori.cfg", FILE_WRITE);

      if (fCfg) 
      {
        fCfg.println("<hostname>powaDCR</hostname>");
        fCfg.println("<ssid></ssid>");
        fCfg.println("<password></password>");
        fCfg.println("<IP>192.168.1.10</IP>");
        fCfg.println("<SN>255.255.255.0</SN>");
        fCfg.println("<GW>192.168.1.1</GW>");
        fCfg.println("<DNS1>192.168.1.1</DNS1>");
        fCfg.println("<DNS2>192.168.1.1</DNS2>");
        fCfg.println("<MCP23017>off</MCP23017>");
        fCfg.println("<NTPSERVER>pool.ntp.org</NTPSERVER>");
        fCfg.println("<TIMEZONE>0</TIMEZONE>");
        fCfg.println("<SUMMERTIME>off</SUMMERTIME>");
        fCfg.println("<SPOTIFY>off</SPOTIFY>");
        fCfg.println("<SPOTIFY_CID></SPOTIFY_CID>");
        fCfg.println("<SPOTIFY_CSE></SPOTIFY_CSE>");
        fCfg.println("<QUICKBOOT>off</QUICKBOOT>");
        fCfg.println("<BEEP>on</BEEP>");

        #ifdef DEBUGMODE
                logln("powadcr.cfg new file created");
        #endif

        fCfg.close();

        cfgloaded = false;

        hmi.writeString("statusLCD.txt=\"powadcr.cfg not found.\"");
        delay(2000);
      }
    }
  }

  return cfgloaded;
}

void proccesingTAP(char *file_ch) {
  // pTAP.set_SdFat32(sdf);

  pTAP.initialize();

  if (!PLAY) {
    pTAP.getInfoFileTAP(file_ch);

    if (!FILE_CORRUPTED && TOTAL_BLOCKS != 0) {
      // El fichero está preparado para PLAY
      FILE_PREPARED = true;

#ifdef DEBUGMODE
      logAlert("TAP prepared");
#endif
    } else {
      LAST_MESSAGE = "ERROR! Selected file is CORRUPTED.";
      FILE_PREPARED = false;
    }
  } else {

    LAST_MESSAGE = "Error. PLAY in progress. Try select file again.";
    //
    delay(2000);
    //
    FILE_PREPARED = false;
  }
}

void proccesingTZX(char *file_ch) {
  // Procesamos ficheros CDT, TSX y TZX
  pTZX.initialize();

  pTZX.process(file_ch);

  if (ABORT) {
    FILE_PREPARED = false;
    // ABORT=false;
  } else {
    if (TOTAL_BLOCKS != 0) {
      FILE_PREPARED = true;

#ifdef DEBUGMODE
      logAlert("TZX prepared");
#endif
    } else {
      FILE_PREPARED = false;
    }
  }
}

void proccesingPZX(char *file_ch) {
  // Procesamos ficheros CDT, TSX y TZX
  pPZX.initialize();

  pPZX.process(file_ch);

  if (ABORT) {
    FILE_PREPARED = false;
    // ABORT=false;
    //  No hacemos nada con el descriptor si se aborta
  } else {
    if (TOTAL_BLOCKS != 0) {
      FILE_PREPARED = true;

      // ✅ CORRECCIÓN: Mover la llamada aquí, al bloque de éxito.
      myPZX = pPZX.getDescriptor();

#ifdef DEBUGMODE
      logAlert("PZX prepared");
#endif
    } else {
      FILE_PREPARED = false;
    }
  }
}

void sendStatus(int action, int value = 0) {

  switch (action) 
  {
  case PLAY_ST:
    // hmi.writeString("");
    hmi.writeString("PLAYst.val=" + String(value));
    break;

  case STOP_ST:
    // hmi.writeString("");
    hmi.writeString("STOPst.val=" + String(value));
    break;

  case PAUSE_ST:
    // hmi.writeString("");
    hmi.writeString("PAUSEst.val=" + String(value));
    break;

  case END_ST:
    // hmi.writeString("");
    hmi.writeString("ENDst.val=" + String(value));
    break;

  case REC_ST:
    hmi.writeString("RECst.val=" + String(value));
    break;

  case READY_ST:
    // hmi.writeString("");
    hmi.writeString("READYst.val=" + String(value));
    break;

  case ACK_LCD:
    // hmi.writeString("");
    logln("Sending ACK status to HMI: " + String(value));
    hmi.writeString("tape.LCDACK.val=" + String(value));
    // hmi.writeString("");
    // hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");
    delay(1500);
    // Enviamos la version del firmware del powaDCR
    // hmi.writeString("mainmenu.verFirmware.txt=\" powadcr " + String(VERSION)
    // + "\"");
    logln("HMI ACK received. LCD is ready.");
    hmi.writeString("page tape0");
    CURRENT_PAGE = 0;
    break;

  case RESET:
    // hmi.writeString("");
    hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
    break;
  }
}

bool waitForHMI(bool waitAndNotForze) {
  // Le decimos que no queremos esperar sincronización
  // y forzamos un READY del HMI
  int timeout = 0;

  if (!waitAndNotForze) {
    LCD_ON = true;
    //sendStatus(ACK_LCD, 1);
  } else {
    // Esperamos a la pantalla
    statusTXLine = true;
    statusRXLine = false;

    while (!LCD_ON) 
    {
      hmi.readUART();
      delay(100);
      if (timeout > 2000) 
      {
        // Timeout de 10 segundos
        logln("HMI Timeout. Review pinout for TX / RX");
        statusRXLine = false;
        LCD_ON = false;
        return false;
      }
      timeout += 100;
    }
    LCD_ON = true;
    statusRXLine = true;
  }
  return true;
}

void showRadioDial() {
  // Mostramos el dial de radio
  hmi.writeString("tape.animation.pic=52");
  hmi.writeString("tape.animation.pic=52");
  //
}

void hideRadioDial() {
  // Escondemos el dial de radio
  hmi.writeString("tape.animation.pic=4");
  delay(250);
  hmi.writeString("tape.animation.pic=4");
}

void tapeAnimationON() {
  // Activamos animacion cinta
  hmi.writeString("tape2.tmAnimation.en=1");
  hmi.writeString("tape.tmAnimation.en=1");
  delay(250);
  hmi.writeString("tape2.tmAnimation.en=1");
  hmi.writeString("tape.tmAnimation.en=1");
}

void tapeAnimationOFF() {
  // Desactivamos animacion cinta
  hmi.writeString("tape2.tmAnimation.en=0");
  hmi.writeString("tape.tmAnimation.en=0");
  delay(250);
  hmi.writeString("tape2.tmAnimation.en=0");
  hmi.writeString("tape.tmAnimation.en=0");
}

void recAnimationON() {
  // Indicador REC parpadea
  hmi.writeString("tape.RECst.val=1");
  delay(250);
  hmi.writeString("tape.RECst.val=1");
  powerLedFixed = false;
}

void recAnimationOFF() {
  // Indicador REC deja de parpadear
  hmi.writeString("tape.RECst.val=0");
  delay(250);
  hmi.writeString("tape.RECst.val=0");
  powerLedFixed = true;
}

void recAnimationFIXED_ON() {
  // Indicador LED fijo ON
  hmi.writeString("tape.recIndicator.bco=63848");
  delay(250);
  hmi.writeString("tape.recIndicator.bco=63848");
  powerLedFixed = true;
}

void recAnimationFIXED_OFF() {
  // Indicador LED fijo OFF
  hmi.writeString("tape.recIndicator.bco=32768");
  delay(250);
  hmi.writeString("tape.recIndicator.bco=32768");
  powerLedFixed = false;
}

void wavrec()
{
    AudioBoardStream kitStream(AudioKitEs8388V1);
    AudioInfo info(DEFAULT_WAV_SAMPLING_RATE_REC, 1, 16);

    //AudioBoardStream kitStream()

    auto ncfg = kitStream.defaultConfig(RXTX_MODE);
    // Guardamos la configuracion de sampling rate
    SAMPLING_RATE = ncfg.sample_rate;
    ncfg.sample_rate = DEFAULT_WAV_SAMPLING_RATE_REC;
    ncfg.input_device = ADC_INPUT_LINE2;
    ncfg.output_device = DAC_OUTPUT_ALL;
    kitStream.setAudioInfo(ncfg);
    
    File wavfile = SD_MMC.open("/test.wav", FILE_WRITE);
    
    NumberFormatConverterStreamT<int16_t, uint8_t> converter(kitStream);
    EncodedAudioStream encoder(&wavfile, new WAVEncoder()); // Encoder WAV PCM

    StreamCopy copier(encoder,converter);
    copier.setSynchAudioInfo(true);

    converter.setAudioInfo(info);
    converter.begin();
    // Inicializamos el encoder
    encoder.begin(converter.audioInfoOut());
    copier.begin(converter, encoder);

    while (!STOP)
    {
        copier.copy();
    }

    copier.end();
    converter.end();
    encoder.end();
}

void WavRecording() {
  //-----------------------------------------------------------
  //
  // Esta rutina graba en WAV el audio que entra por LINE IN
  //
  //-----------------------------------------------------------
  String wavfilename = wavfile.name();

  unsigned long progress_millis = 0;
  unsigned long progress_millis2 = 0;
  int rectime_s = 0;
  int rectime_m = 0;
  size_t wavfilesize = 0;

  auto new_sr = kitStream.defaultConfig(RXTX_MODE);
  // Guardamos la configuracion de sampling rate
  SAMPLING_RATE = new_sr.sample_rate;
  new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE_REC;
  kitStream.setAudioInfo(new_sr);
  // Actuamos sobre el amplificador
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);  

  logln("Starting WAV recording... on file " + wavfilename);

  AudioInfo info(DEFAULT_WAV_SAMPLING_RATE_REC, 1, 16);
  AudioInfo infoStereo(DEFAULT_WAV_SAMPLING_RATE_REC, 2, 16);
  // Stream de audio del WAV
  EncodedAudioStream encoder(&wavfile, new WAVEncoder()); // Encoder WAV PCM
  // Convertidor 16 a 8 bits
  NumberFormatConverterStreamT<int16_t, uint8_t> nfc;

  // --- MultiOutput y copier para WAV ---
  MultiOutput multi;

  if (WAV_8BIT_MONO)
  {
    multi.add(nfc);
    multi.add(kitStream);

    nfc.setOutput(encoder);
  }
  else
  {
    multi.add(encoder);
    multi.add(kitStream);
  }

  StreamCopy copier;

  // Esperamos a que la pantalla esté lista
  recAnimationOFF();
  delay(125);
  recAnimationFIXED_ON();
  tapeAnimationON();

  // Agregamos las salidas al multiple
  
  if (WAV_8BIT_MONO) 
  {
    // Configuramos el convertidor para 8-bit mono, con la misma frecuencia de muestreo que el encoder
    
    // Configuracion del convertidor
    nfc.setAudioInfo(info);
    nfc.begin(info);

    // Inicializamos el multiple output con la configuración de señal del convertidor
    //multi.setAudioInfo(info);
    multi.begin(info);
    // Configuramos el copier
    copier.begin(multi, kitStream); // WAV: fuente kitStream, destinos encoder y kitStream
    copier.setSynchAudioInfo(true);
    // IMPORTATNTE: Inicializamos el encoder y kitStream con la configuración de señal del convertidor
    encoder.begin(nfc.audioInfoOut());
  } 
  else 
  {
    // Configuramos el encoder para 44KHz, 16-bit stereo, 2 canales
    multi.setAudioInfo(infoStereo);
    multi.begin();
    // Configuramos el copier
    copier.begin(multi, kitStream); // WAV: fuente kitStream, destinos encoder y kitStream
    copier.setSynchAudioInfo(true);
    // Inicializamos el encoder
    encoder.begin(infoStereo);
  }

  // Iniciamos el encoder con la configuración de señal
  

  // Reset de variables
  STOP = false;
  WAVFILE_PRELOAD = false;
  BTNREC_PRESSED = false;

  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(kitStream.audioInfo().sample_rate / 1000)) +
                  "KHz\"");
  
  uint32_t samplesWritten = 0;

  //
  LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";

  //
  while (!STOP && !BTNREC_PRESSED) {
    size_t samplesCopied = copier.copy();
    wavfilesize += samplesCopied;

    // Actualiza tiempo y UI
    if ((millis() - progress_millis) > 1000) {
      rectime_s++;
      if (rectime_s > 59) {
        rectime_s = 0;
        rectime_m++;
      }
      progress_millis = millis();
    }
    if ((millis() - progress_millis2) > 1000) {
      LAST_MESSAGE = "Recording time: " +
                     ((rectime_m < 10 ? "0" : "") + String(rectime_m)) + ":" +
                     ((rectime_s < 10 ? "0" : "") + String(rectime_s));
      hmi.writeString("size.txt=\"" +
                      String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024
                                                   : wavfilesize / 1024) +
                      (wavfilesize > 1000000 ? " MB" : " KB") + "\"");
      progress_millis2 = millis();
    }
  }

  logln("File has ");
  log(String(wavfilesize / 1024));
  log(" Kbytes");

  // Paramos todo
  TAPESTATE = 0;
  LOADING_STATE = 0;
  RECORDING_ERROR = 0;
  REC = false;
  recAnimationOFF();
  recAnimationFIXED_OFF();
  tapeAnimationOFF();

  LAST_MESSAGE = "Recording finish";
  logln("Recording finish!");

  hmi.writeString("size.txt=\"" +
                  String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024
                                               : wavfilesize / 1024) +
                  (wavfilesize > 1000000 ? " MB" : " KB") + "\"");

  copier.end();
  encoder.end();
  nfc.end();
  multi.end();

  // Cerramos el fichero WAV
  wavfile.flush();
  wavfile.close();

  WAVFILE_PRELOAD = true;

}

// void WavRecording_old() {
//   //-----------------------------------------------------------
//   //
//   // Esta rutina graba en WAV el audio que entra por LINE IN
//   //
//   //-----------------------------------------------------------
//   String wavfilename = wavfile.name();

//   unsigned long progress_millis = 0;
//   unsigned long progress_millis2 = 0;
//   int rectime_s = 0;
//   int rectime_m = 0;
//   size_t wavfilesize = 0;

//   auto new_sr = kitStream.defaultConfig(RXTX_MODE);
//   // Guardamos la configuracion de sampling rate
//   SAMPLING_RATE = new_sr.sample_rate;
//   new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE_REC;
//   kitStream.setAudioInfo(new_sr);
//   // Actuamos sobre el amplificador
//   kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);  

//   logln("Starting WAV recording... on file " + wavfilename);


//   AudioInfo info(DEFAULT_WAV_SAMPLING_RATE_REC, 1, 16);
//   AudioInfo infoStereo(DEFAULT_WAV_SAMPLING_RATE_REC, 2, 16);
//   // Stream de audio del WAV
//   EncodedAudioStream encoder(&wavfile, new WAVEncoder()); // Encoder WAV PCM
//   // Convertidor 16 a 8 bits
//   NumberFormatConverterStreamT<int16_t, uint8_t> nfc(kitStream);

//   // --- MultiOutput y copier para WAV ---
//   MultiOutput multi;
//   multi.add(encoder);
//   multi.add(kitStream);

//   StreamCopy copier;

//   // Esperamos a que la pantalla esté lista
//   recAnimationOFF();
//   delay(125);
//   recAnimationFIXED_ON();
//   tapeAnimationON();

//   // Agregamos las salidas al multiple
  
//   if (WAV_8BIT_MONO) 
//   {
//     // Configuramos el convertidor para 8-bit mono, con la misma frecuencia de muestreo que el encoder
    
//     // Configuracion del convertidor
//     nfc.setAudioInfo(info);
//     nfc.begin(info);

//     // Inicializamos el multiple output con la configuración de señal del convertidor
//     //multi.setAudioInfo(info);
//     multi.begin(nfc.audioInfo());
//     // Configuramos el copier
//     copier.begin(multi, nfc); // WAV: fuente kitStream, destinos encoder y kitStream
//     copier.setSynchAudioInfo(true);
//     // IMPORTATNTE: Inicializamos el encoder y kitStream con la configuración de señal del convertidor
//     encoder.begin(nfc.audioInfoOut());
//   } 
//   else 
//   {
//     // Configuramos el encoder para 44KHz, 16-bit stereo, 2 canales
//     multi.setAudioInfo(infoStereo);
//     multi.begin();
//     // Configuramos el copier
//     copier.begin(multi, kitStream); // WAV: fuente kitStream, destinos encoder y kitStream
//     copier.setSynchAudioInfo(true);
//     // Inicializamos el encoder
//     encoder.begin(infoStereo);
//   }

//   // Iniciamos el encoder con la configuración de señal
  

//   // Reset de variables
//   STOP = false;
//   WAVFILE_PRELOAD = false;
//   BTNREC_PRESSED = false;

//   // Indicamos
//   hmi.writeString("tape.lblFreq.txt=\"" + String(int(kitStream.audioInfo().sample_rate / 1000)) +
//                   "KHz\"");
  
//   uint32_t samplesWritten = 0;

//   if (WAV_8BIT_MONO) 
//   {
//     LAST_MESSAGE = "Warning: I2S output not supports 8-bits";
//     delay(1500);
//   } 
//   //
//   LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";

//   //
//   while (!STOP && !BTNREC_PRESSED) {
//     size_t samplesCopied = copier.copy();
//     wavfilesize += samplesCopied;

//     // Actualiza tiempo y UI
//     if ((millis() - progress_millis) > 1000) {
//       rectime_s++;
//       if (rectime_s > 59) {
//         rectime_s = 0;
//         rectime_m++;
//       }
//       progress_millis = millis();
//     }
//     if ((millis() - progress_millis2) > 1000) {
//       LAST_MESSAGE = "Recording time: " +
//                      ((rectime_m < 10 ? "0" : "") + String(rectime_m)) + ":" +
//                      ((rectime_s < 10 ? "0" : "") + String(rectime_s));
//       hmi.writeString("size.txt=\"" +
//                       String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024
//                                                    : wavfilesize / 1024) +
//                       (wavfilesize > 1000000 ? " MB" : " KB") + "\"");
//       progress_millis2 = millis();
//     }
//   }

//   logln("File has ");
//   log(String(wavfilesize / 1024));
//   log(" Kbytes");

//   // Paramos todo
//   TAPESTATE = 0;
//   LOADING_STATE = 0;
//   RECORDING_ERROR = 0;
//   REC = false;
//   recAnimationOFF();
//   recAnimationFIXED_OFF();
//   tapeAnimationOFF();

//   LAST_MESSAGE = "Recording finish";
//   logln("Recording finish!");

//   hmi.writeString("size.txt=\"" +
//                   String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024
//                                                : wavfilesize / 1024) +
//                   (wavfilesize > 1000000 ? " MB" : " KB") + "\"");

//   copier.end();
//   encoder.end();
//   nfc.end();
//   multi.end();

//   // Cerramos el fichero WAV
//   wavfile.flush();
//   wavfile.close();

//   WAVFILE_PRELOAD = true;

// }

void stopRecording() {

  // No quitar esto, permite leer la variable totalBlockTransfered
  int tbt = taprec.totalBlockTransfered;

  if (tbt >= 1) {

    if (!taprec.errorInDataRecording && !taprec.fileWasNotCreated) {
      // La grabación fue parada pero no hubo errores
      // entonces salvamos el fichero sin borrarlo
      LAST_MESSAGE = "Total blocks captured: " + String(tbt);
      delay(1500);
      //
      taprec.terminate(false);
      //
      LAST_MESSAGE = "Recording STOP.";
      tapeAnimationOFF();
      //
      delay(1500);
    } else {
      //
      // Hubo errores, entonces:
      //
      if (taprec.errorInDataRecording) {
        // La grabación fue parada porque hubo errores
        // entonces NO salvamos el fichero, lo borramos
        // Recording STOP

        taprec.terminate(true);
        LAST_MESSAGE = "Recording STOP.";
        tapeAnimationOFF();
        //
        delay(1500);
      } else if (taprec.fileWasNotCreated) {
        // Si no se crea el fichero no se puede seguir grabando
        taprec.terminate(false);
        LAST_MESSAGE = "Error in filesystem or SD.";
        tapeAnimationOFF();
        //
        delay(1500);
      }
    }
  } else {
    //
    delay(1500);
  }

  //}

  // Reiniciamos variables
  taprec.totalBlockTransfered = 0;
  // Reiniciamos flags de recording errors
  taprec.errorInDataRecording = false;
  taprec.fileWasNotCreated = true;

  // Paramos la animación del indicador de recording
  tapeAnimationOFF();
  //
  hmi.activateWifi(true);
  //
}

bool wifiSetup() {
  bool wifiActive = true;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  hmi.writeString("statusLCD.txt=\"SSID: [" + String(ssid) + "]\"");
  delay(1500);

  // Configures Static IP Address
  if (!DHCP_ENABLE) {
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
      hmi.writeString("statusLCD.txt=\"WiFi-STA setting failed!\"");
      wifiActive = false;
    }
  }

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    hmi.writeString("statusLCD.txt=\"WiFi Connection failed!");
    wifiActive = false;
  } else {
    wifiActive = true;
  }

  if (wifiActive) {

    hmi.writeString("statusLCD.txt=\"IP " + WiFi.localIP().toString() + "\"");
    delay(50);
  } else {
    hmi.writeString("statusLCD.txt=\"Wifi disabled\"");
    wifiActive = false;
    delay(750);
  }

  return wifiActive;
}

void writeStatusLCD(String txt) {
  hmi.writeString("statusLCD.txt=\"" + txt + "\"");
}

void prepareOutputToWav() {
  String wavnamepath = "";
  String wavfileBaseName = "/WAV/rec";

  char file_name[255];
  // Limpiamos el nombre del fichero
  memset(file_name, 0, sizeof(file_name));

  // Si es una copia de un TAPE llevara un FILE_LOAD pero,
  // si no, hay que generarlo
  if (!PLAY_TO_WAV_FILE) {
    // Generamos el nombre del fichero WAV SIEMPRE, cuando es grabación a WAV y
    // no PLAY a WAV.
    char *cPath = (char *)ps_calloc(55, sizeof(char));
    getRandomFilenameWAV(cPath, wavfileBaseName);
    wavnamepath = String(cPath);
    free(cPath);
  } else {
    // Si es PLAY a WAV
    if (FILE_LOAD.length() > 0 && PLAY_TO_WAV_FILE) {
      // Cogemos el nombre del TAP/TZX
      wavnamepath = "/WAV/" + removeExtension(FILE_LOAD) + ".wav";
    } else if (FILE_LOAD.length() < 1 && PLAY_TO_WAV_FILE) {
      // Cogemos un nombre nuevo porque no existe
      FILE_LOAD = "rec_tape";

      wavnamepath = "/WAV/" + removeExtension(FILE_LOAD) + ".wav";
    }
  }

  // Pasamos la ruta del fichero
  REC_FILENAME = wavnamepath;

  strncpy(file_name, wavnamepath.c_str(), wavnamepath.length());
  logln("Output to WAV: " + wavnamepath);
  logln("Output to WAV (filename): " + String(file_name));

  // AudioLogger::instance().begin(Serial, AudioLogger::Error);
  if (wavfile) {
    wavfile.close();
    logln("WAV file already open. Closing it.");
  }

  // open file for recording WAV
  wavfile = SD_MMC.open(file_name, FILE_WRITE);
  FILE_LOAD = getFileNameFromPath(wavnamepath);

  // Muestro solo el nombre. Le elimino la primera parte que es el directorio.
  hmi.writeString("name.txt=\"" + String(file_name).substring(5) + "\"");
  hmi.writeString("type.txt=\"[to WAV file]\"");

  if (!wavfile) {
    logln("Error open file to output playing");
    LAST_MESSAGE = "Error output playing to WAV.";
    delay(2000);

    // OUT_TO_WAV = false;
    STOP = true;
    return;
  } else {
    logln("Out to WAV file. Ready!");
  }

  //
}

void setSTOP() {
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;

  BLOCK_SELECTED = 0;

  if (!taprec.errorInDataRecording && !taprec.fileWasNotCreated) {
    LAST_MESSAGE = "Tape stop.";
  }
  delay(125);
  // Paramos la animación del indicador de recording
  tapeAnimationOFF();
  //
  delay(125);
  //
  hmi.writeString("currentBlock.val=1");
  hmi.writeString("progressTotal.val=0");
  hmi.writeString("progression.val=0");

  // sendStatus(STOP_ST, 0);
  // sendStatus(PLAY_ST, 0);
  // sendStatus(PAUSE_ST, 0);
  sendStatus(REC_ST, 0);

  // sendStatus(END_ST, 1);
  // sendStatus(READY_ST, 1);
}

int getStreamfileSizeWithFilename(const String &path, bool showInHMI = true) {

  char pfile[255] = {};
  strcpy(pfile, path.c_str());
  File file = SD_MMC.open(pfile, FILE_READ);
  if (!file) {
#ifdef DEBUGMODE
    logln("Failed to open file: " + path);
#endif
    return -1; // Retornamos -1 si no se puede abrir el archivo
  }

  int fileSize = file.size();
  file.close();

  // Actualizamos el tamaño en el HMI
  if (showInHMI) {
    String sizeStr = (fileSize < 1000000)
                         ? String(fileSize / 1024) + " KB"
                         : String(fileSize / 1024 / 1024) + " MB";
    hmi.writeString("size.txt=\"" + sizeStr + "\"");
  }

  return fileSize;
}

int getStreamfileSize(File *pFile, bool showInHMI = false) {
  size_t fileSize = 0;
  if (pFile != nullptr) {
    fileSize = pFile->size();
  }

  // Actualizamos el tamaño en el HMI
  if (showInHMI) {
    String sizeStr = (fileSize < 1000000)
                         ? String(fileSize / 1024) + " KB"
                         : String(fileSize / 1024 / 1024) + " MB";
    hmi.writeString("size.txt=\"" + sizeStr + "\"");
  }

  return fileSize;
}

void updateSamplingRateIndicator(int rate, int bps, String ext) {
  // Cambiamos el sampling rate a 22KHz para el kitStream
  //
  if (rate > 0)
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(rate / 1000)) + "KHz\"");
  else
    hmi.writeString("tape.lblFreq.txt=\"-- KHz\"");

  if (ext == "wav") {
    if (bps == 8) {
      // Cambiamos el sampling rate a 22KHz para el kitStream
      //
      hmi.writeString("tape.wavind.txt=\"8BIT\"");
    } else {
      if (OUT_TO_WAV) {
        hmi.writeString("tape.wavind.txt=\"WAV\"");
      } else {
        hmi.writeString("tape.wavind.txt=\"\"");
      }
    }
  } else {
    hmi.writeString("tape.wavind.txt=\"\"");
  }
}

void updateIndicators(int size, int pos, uint32_t fsize, int bitrate,
                      String fname) {
  //
  String strBitrate = "";
  fname = fname.substring(fname.lastIndexOf("/") + 1);
  // Esta linea es sumamente importante para que el rotate mueva el texto
  FILE_LOAD = fname;

  if (bitrate != 0) {
    if (TYPE_FILE_LOAD == "WAV") {
      strBitrate = "(" + String(bitrate / 1000) + " KBps)";
    } else {
      strBitrate = "(" + String(bitrate / 1000) + " Kbps)";
    }
  } else {
    strBitrate = "(Vble. br)";
  }

  if (fname != LASTFNAME) {
    // Actualizamos el nombre del fichero
    LASTFNAME = fname;
    // Actualizamos el nombre del fichero en la pantalla
    hmi.writeString("tape.fileName.txt=\"" + fname + "\"");
    hmi.writeString("mp3browser.path.txt=\"" + fname + "\"");
  }

  hmi.writeString("tape.totalBlocks.val=" + String(size));
  hmi.writeString("tape.currentBlock.val=" + String(pos));
  hmi.writeString("type.txt=\"" + TYPE_FILE_LOAD + " file " + strBitrate +
                  "\"");

  if (fsize < 1000000) {
    hmi.writeString("size.txt=\"" + String(fsize / 1024) + " KB\"");
  } else {
    hmi.writeString("size.txt=\"" + String(fsize / 1024 / 1024) + " MB\"");
  }
}

void updateSamplingRate(AudioPlayer &player, Equalizer3Bands &eq,
                        AudioInfo realInfo) {
  logln("Reading sampling rate and updating.");

  if (realInfo.sample_rate > 0) {
    // Actualizamos la configuración con los valores reales
    kitStream.setAudioInfo(realInfo);
    eq.setAudioInfo(realInfo);
    player.setAudioInfo(realInfo);
    //
    logln("Real Audio Info - Sample Rate: " + String(realInfo.sample_rate) +
          "Hz, Bits: " + String(realInfo.bits_per_sample) +
          ", Channels: " + String(realInfo.channels));

    // Mostramos la información
    hmi.writeString("tape.lblFreq.txt=\"" +
                    String(int(realInfo.sample_rate / 1000)) + "KHz\"");
    delay(125);
  } else {
    logln("Warning: Invalid sample rate detected in audio file");
    hmi.writeString("tape.lblFreq.txt=\" -- KHz\"");
  }
}

void estimatePlayingTime(int fileread, int filesize, int samprate) {

  if (samprate > 0) {
    float totalTime = 0;

    if (TYPE_FILE_LOAD == "WAV") {
      totalTime = (filesize) / (samprate);
    } else {
      totalTime = (filesize * 8) / (samprate);
    }

    int tmin = totalTime / 60.0;
    int tsec = totalTime - (tmin * 60);

    String utsec = (tsec < 10) ? "0" : "";
    String utmin = (tmin < 10) ? "0" : "";

    // Elapsed time
    // logln("Bit rate por sample: " + String(ainfo.bits_per_sample));
    // logln("Sampling rate:       " + String(ainfo.sample_rate));

    float time = 0;

    if (TYPE_FILE_LOAD == "WAV") {
      time = (fileread) / (samprate);
    } else {
      time = (fileread * 8) / (samprate);
    }

    int min = time / 60.0;
    int sec = time - (min * 60);

    String usec = (sec < 10) ? "0" : "";
    String umin = (min < 10) ? "0" : "";

    LAST_MESSAGE = "Total time:  [" + utmin + String(tmin) + ":" + utsec +
                   String(tsec) + "]   -   Elapsed:  [" + umin + String(min) +
                   ":" + usec + String(sec) + "]";
  }
}

String removeExtension(const String &filename) {
  int dotIndex = filename.lastIndexOf('.');
  if (dotIndex > 0) {
    return filename.substring(0, dotIndex);
  }
  return filename; // Si no hay punto, devuelve el nombre tal cual
}

// Devuelve solo el directorio de una ruta completa
inline String getDirFromPath(String fullPath) {
  int lastSlash = fullPath.lastIndexOf('/');
  if (lastSlash > 0) {
    return fullPath.substring(0, lastSlash);
  }
  return "";
}

String getFileNameFromPath(const String &filePath) {
  int lastSlash = filePath.lastIndexOf('/');
  if (lastSlash == -1) {
    lastSlash = filePath.lastIndexOf('\\'); // Por si se usa '\' como separador
  }
  if (lastSlash != -1) {
    return filePath.substring(
        lastSlash + 1); // Devuelve el nombre del archivo con su extensión
  }
  return filePath; // Si no hay separador, devuelve la cadena completa
}

// ✅ NUEVA FUNCIÓN - CORREGIDA
int generateRadioList(tAudioList *&radioList) {
  // Asignamos memoria para la lista de radios
  radioList = (tAudioList *)ps_calloc(MAX_RADIO_STATIONS, sizeof(tAudioList));
  if (radioList == nullptr) {
    logln("Error: Failed to allocate memory for radio list.");
    return 0;
  }

  int radioCount = 0;
  const char *filepath = PATH_FILE_TO_LOAD.c_str();

  // 1. Verificar que el archivo de radios existe
  if (!SD_MMC.exists(filepath)) {
    logln("Error: Radio list file not found: " + String(filepath));
    free(radioList);
    radioList = nullptr;
    return 0;
  }

  // 2. Abrir el archivo para lectura
  File radioFile = SD_MMC.open(filepath, FILE_READ);
  if (!radioFile) {
    logln("Error: Could not open radio list file: " + String(filepath));
    free(radioList);
    radioList = nullptr;
    return 0;
  }

  logln("Generating radio list from: " + String(filepath));

  // 3. Leer el archivo línea por línea - ✅ INCLUIR LA ÚLTIMA LÍNEA
  while (radioCount < MAX_RADIO_STATIONS) {
    String line = radioFile.readStringUntil('\n');
    line.trim();

    // ✅ COMPROBAR EOF - si está vacío y estamos al final, salir
    if (line.length() == 0) {
      if (radioFile.position() >= radioFile.size()) {
        break; // Fin del archivo
      }
      continue; // Línea vacía en medio del archivo
    }

    // Ignorar líneas sin el separador ','
    int commaIndex = line.indexOf(',');
    if (commaIndex == -1) {
      continue;
    }

    // 4. Extraer nombre y URL
    // logln("Radio read: " + line);

    String stationName = line.substring(0, commaIndex);
    String stationUrl = line.substring(commaIndex + 1);

    // Limpiar espacios en blanco
    stationName.trim();
    stationUrl.trim();

    // logln(" > Station Name: " + stationName + " | Station URL: " +
    // stationUrl);

    // 5. Eliminar comillas del nombre de la emisora
    if (stationName.length() >= 2) {
      if (stationName.startsWith("\"") && stationName.endsWith("\"")) {
        stationName = stationName.substring(1, stationName.length() - 1);
      } else if (stationName.startsWith("'") && stationName.endsWith("'")) {
        stationName = stationName.substring(1, stationName.length() - 1);
      }
    }

    // 6. Almacenar en la estructura si los datos son válidos
    if (stationName.length() > 0 && stationUrl.length() > 0) {
      radioList[radioCount].filename = stationName;
      radioList[radioCount].path = stationUrl;
      radioCount++;
    }
  }

  // 7. Cerrar el archivo y devolver el total
  radioFile.close();

  logln("Radio list generated with " + String(radioCount) + " stations.");
  return radioCount;
}

// // ✅ NUEVA FUNCIÓN
// int generateRadioList(tAudioList* &radioList) {
//     // Asignamos memoria para la lista de radios
//     radioList = (tAudioList*)ps_calloc(MAX_RADIO_STATIONS,
//     sizeof(tAudioList)); if (radioList == nullptr) {
//         logln("Error: Failed to allocate memory for radio list.");
//         return 0;
//     }

//     int radioCount = 0;
//     const char* filepath = PATH_FILE_TO_LOAD.c_str();

//     // 1. Verificar que el archivo de radios existe
//     if (!SD_MMC.exists(filepath)) {
//         logln("Error: Radio list file not found: " + String(filepath));
//         free(radioList); // Liberar memoria si el archivo no existe
//         radioList = nullptr;
//         return 0;
//     }

//     // 2. Abrir el archivo para lectura
//     File radioFile = SD_MMC.open(filepath, FILE_READ);
//     if (!radioFile) {
//         logln("Error: Could not open radio list file: " + String(filepath));
//         free(radioList); // Liberar memoria si no se puede abrir
//         radioList = nullptr;
//         return 0;
//     }

//     logln("Generating radio list from: " + String(filepath));

//     // 3. Leer el archivo línea por línea
//     while (radioFile.available() && radioCount < MAX_RADIO_STATIONS) {
//         String line = radioFile.readStringUntil('\n');
//         line.trim();

//         // Ignorar líneas vacías o sin el separador ','
//         int commaIndex = line.indexOf(',');
//         if (line.length() == 0 || commaIndex == -1) {
//             continue;
//         }

//         // 4. Extraer nombre y URL
//         String stationName = line.substring(0, commaIndex);
//         String stationUrl = line.substring(commaIndex + 1);

//         // Limpiar espacios en blanco al principio y al final
//         stationName.trim();
//         stationUrl.trim();

//         // 5. Eliminar comillas del nombre de la emisora
//         if (stationName.length() >= 2) {
//             if (stationName.startsWith("\"") && stationName.endsWith("\"")) {
//                 stationName = stationName.substring(1, stationName.length() -
//                 1);
//             } else if (stationName.startsWith("'") &&
//             stationName.endsWith("'")) {
//                 stationName = stationName.substring(1, stationName.length() -
//                 1);
//             }
//         }

//         // 6. Almacenar en la estructura si los datos son válidos
//         if (stationName.length() > 0 && stationUrl.length() > 0) {
//             radioList[radioCount].filename = stationName;
//             radioList[radioCount].path = stationUrl;
//             radioCount++;
//         }
//     }

//     // 7. Cerrar el archivo y devolver el total
//     radioFile.close();

//     logln("Radio list generated with " + String(radioCount) + " stations.");
//     return radioCount;
// }

int generateAudioList(tAudioList *&audioList, String extension = ".mp3") {
  audioList = (tAudioList *)ps_calloc(MAX_FILES_AUDIO_LIST, sizeof(tAudioList));
  int size = 0;

  String parentDir = FILE_LAST_DIR;
  if (!parentDir.endsWith("/"))
    parentDir += "/";
  String filesListPath = parentDir + "_files.lst";
  String fileSelected = getFileNameFromPath(PATH_FILE_TO_LOAD);

  // ✅ NUEVAS RUTAS para los archivos de índice
  String idxPath = parentDir + "idx.txt";
  String idxDefPath = parentDir + "idx-def.txt";

  // ✅ Verificar que el archivo _files.lst existe
  if (!SD_MMC.exists(filesListPath.c_str())) {
    logln("Error: Files list not found: " + filesListPath);
    return 0;
  }

  File filesListFile = SD_MMC.open(filesListPath.c_str(), FILE_READ);
  if (!filesListFile) {
    logln("Error: Cannot open " + filesListPath);
    return 0;
  }

  // ✅ CREAR archivos de índice para AudioSourceIdxSDMMC
  File idxFile;
  File idxDefFile;

  // Eliminar archivos de índice existentes si existen
  if (SD_MMC.exists(idxPath.c_str())) {
    SD_MMC.remove(idxPath.c_str());
    logln("Removed existing idx.txt");
  }
  if (SD_MMC.exists(idxDefPath.c_str())) {
    SD_MMC.remove(idxDefPath.c_str());
    logln("Removed existing idx-def.txt");
  }

  // Crear nuevos archivos de índice
  idxFile = SD_MMC.open(idxPath.c_str(), FILE_WRITE);
  if (!idxFile) {
    logln("Error: Cannot create idx.txt");
    filesListFile.close();
    return 0;
  }

  char *line = (char *)ps_calloc(512, sizeof(char));
  String strline;
  int indexCounter = 0; // Contador para el índice de AudioTools

  try {
    logln("Generating audio list and index files for extension: " + extension);

    while (filesListFile.available() && size < MAX_FILES_AUDIO_LIST) {
      // Limpiar buffer
      memset(line, 0, 512);

      int bytesRead = filesListFile.readBytesUntil('\n', line, 511);
      if (bytesRead == 0)
        continue;

      strline = String(line);
      strline.trim();
      if (strline.length() == 0)
        continue;

      // Parseamos los 4 campos: indice|tipo|tamaño|nombre|
      int idx1 = strline.indexOf('|');
      int idx2 = strline.indexOf('|', idx1 + 1);
      int idx3 = strline.indexOf('|', idx2 + 1);
      int idx4 = strline.indexOf('|', idx3 + 1);

      if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1)
        continue;

      String tipo = strline.substring(idx1 + 1, idx2);
      String nombre = strline.substring(idx3 + 1, idx4);
      String tamano = strline.substring(idx2 + 1, idx3);
      String indice = strline.substring(0, idx1);

      tipo.trim();
      nombre.trim();

      // ✅ Validaciones adicionales de seguridad
      if (nombre.length() == 0 || nombre.length() > 255) {
        logln("Invalid filename length: " + nombre);
        continue;
      }

      // Verificar caracteres válidos en el nombre
      bool validName = true;
      for (int i = 0; i < nombre.length(); i++) {
        char c = nombre.charAt(i);
        if (c == 0 || c < 32) {
          validName = false;
          break;
        }
      }

      if (!validName) {
        logln("Invalid characters in filename: " + nombre);
        continue;
      }

      // Solo ficheros y extensión coincidente
      String nombreLower = nombre;
      nombreLower.toLowerCase();
      String extensionLower = extension;
      extensionLower.toLowerCase();

      if (tipo == "F" && nombreLower.endsWith(extensionLower)) {
        // ✅ Añadir a la lista de audio
        audioList[size].index = indice.toInt();
        audioList[size].filename = nombre;
        audioList[size].path = parentDir;
        audioList[size].size = tamano.toInt();

        // Capturar el índice del fichero seleccionado en el browser
        if (audioList[size].filename.equalsIgnoreCase(fileSelected)) {
          MEDIA_CURRENT_POINTER = size;
        }

        // ✅ ESCRIBIR en idx.txt (formato requerido por AudioSourceIdxSDMMC)
        String fullPath = parentDir + nombre;
        idxFile.println(fullPath);

#ifdef DEBUGMODE
        logln("Added to audio list and index: " + nombre +
              " (index: " + String(size) + ")");
#endif

        size++;
        indexCounter++;
      }
    }

    // ✅ Cerrar el archivo idx.txt
    idxFile.close();

    // ✅ CREAR archivo idx-def.txt
    idxDefFile = SD_MMC.open(idxDefPath.c_str(), FILE_WRITE);
    if (idxDefFile) {
      // Crear la clave de definición (formato requerido por
      // AudioSourceIdxSDMMC)
      String keyDef = parentDir + "|" + extension + "|*";
      idxDefFile.println(keyDef);
      idxDefFile.close();

      logln("Created idx-def.txt with key: " + keyDef);
    } else {
      logln("Warning: Could not create idx-def.txt");
    }

  } catch (const std::exception &e) {
    logln("Exception in generateAudioList: " + String(e.what()));
  } catch (...) {
    logln("Unknown exception in generateAudioList");
  }

  // ✅ Limpieza garantizada
  free(line);
  line = nullptr;

  filesListFile.close();

  // Asegurar que los archivos de índice estén cerrados
  if (idxFile)
    idxFile.close();
  if (idxDefFile)
    idxDefFile.close();

  TOTAL_BLOCKS = size;
  logln("Generated audio list with " + String(size) + " files.");
  logln("Created index files: idx.txt (" + String(indexCounter) +
        " entries) and idx-def.txt");

  return size;
}

int getIndexFromAudioList(tAudioList *audioList, int size,
                          const String &searchValue) {
  for (int i = 0; i < size; i++) {
    // logln("Comparing: " + audioList[i].filename + " with " + searchValue);

    if (audioList[i].filename.equalsIgnoreCase(searchValue)) {
      return audioList[i].index; // Retorna el índice del archivo encontrado
    }
  }
  return -1; // Retorna -1 si no se encuentra el archivo
}

void nextAudio(int &currentPointer, int audioListSize) {
  // Avanzamos al siguiente archivo
  currentPointer++;
  if (currentPointer >= audioListSize) {
    currentPointer = 0; // Volvemos al inicio si llegamos al final
  }
}

void prevAudio(int &currentPointer, int audioListSize) {
  // Retrocedemos al archivo anterior
  currentPointer--;
  if (currentPointer < 0) {
    currentPointer =
        audioListSize - 1; // Volvemos al final si llegamos al inicio
  }
}

int nextRadioStation(const String &filepath, String &radioname, char *url, size_t urlMaxSize, bool forward = true, int index = -1, bool use_index = false) {
  static String lastFilepath = "";
  static int currentLine = -1; // Empezamos en -1 para que la primera llamada devuelva línea 0
  static int totalLines = 0;
  static bool firstCall = true;

  // Si el filepath cambió, reiniciar
  if (lastFilepath != filepath) {
    lastFilepath = filepath;
    currentLine = -1;
    totalLines = 0;
    firstCall = true;
  }

  // Verificar que el archivo existe
  if (!SD_MMC.exists(filepath.c_str())) {
    logln("Radio file not found: " + filepath);
    return 0;
  }

  // Contar líneas válidas en la primera llamada
  if (firstCall) {
    File radioFile = SD_MMC.open(filepath.c_str(), FILE_READ);
    if (!radioFile) {
      logln("Cannot open radio file: " + filepath);
      return 0;
    }

    totalLines = 0;
    while (radioFile.available()) {
      String line = radioFile.readStringUntil('\n');
      line.trim();
      if (line.length() > 0 && line.indexOf(',') != -1) {
        totalLines++;
      }
    }
    radioFile.close();

    if (totalLines == 0) {
      logln("No valid radio stations found in: " + filepath);
      return 0;
    }

    firstCall = false;
    logln("Found " + String(totalLines) + " radio stations in file");
  }

  // ✅ LÓGICA DE NAVEGACIÓN MEJORADA
  if (use_index) {
    // Modo índice: saltar a una línea específica
    if (index >= 0 && index < totalLines) {
      currentLine = index;
      logln("Jumping to index: " + String(currentLine));
    } else {
      logln("Error: Index " + String(index) + " is out of bounds.");
      return totalLines; // Devolver total de líneas pero no cambiar nada
    }
  } else {
    // Modo secuencial (adelante/atrás)
    if (forward) {
      currentLine = (currentLine + 1) % totalLines;
      logln("Moving forward to line: " + String(currentLine));
    } else {
      currentLine = (currentLine - 1 + totalLines) % totalLines;
      logln("Moving backward to line: " + String(currentLine));
    }
  }

  // Abrir archivo para lectura
  File radioFile = SD_MMC.open(filepath.c_str(), FILE_READ);
  if (!radioFile) {
    logln("Cannot open radio file for reading: " + filepath);
    return totalLines;
  }

  int lineIndex = 0;
  String currentLineText = "";
  bool found = false;

  // Leer hasta encontrar la línea deseada
  while (radioFile.available() && !found) {
    currentLineText = radioFile.readStringUntil('\n');
    currentLineText.trim();

    if (currentLineText.length() > 0 && currentLineText.indexOf(',') != -1) {
      if (lineIndex == currentLine) {
        found = true;
        break;
      }
      lineIndex++;
    }
  }

  radioFile.close();

  if (!found) {
    logln("Error: Could not find line " + String(currentLine) +
          " in radio file");
    return totalLines;
  }

  // Parsear la línea: "nombre,url"
  int commaIndex = currentLineText.indexOf(',');
  if (commaIndex == -1) {
    logln("Error: Invalid format in line: " + currentLineText);
    return totalLines;
  }

  // Extraer nombre y URL
  radioname = currentLineText.substring(0, commaIndex);
  String tempUrl = currentLineText.substring(commaIndex + 1);

  // Limpiar espacios
  radioname.trim();
  tempUrl.trim();

  // Eliminar comillas del nombre
  if (radioname.length() >= 2) {
    if (radioname.startsWith("\"") && radioname.endsWith("\"")) {
      radioname = radioname.substring(1, radioname.length() - 1);
    } else if (radioname.startsWith("'") && radioname.endsWith("'")) {
      radioname = radioname.substring(1, radioname.length() - 1);
    }
  }

  // Eliminar comillas de la URL
  if (tempUrl.length() >= 2) {
    if (tempUrl.startsWith("\"") && tempUrl.endsWith("\"")) {
      tempUrl = tempUrl.substring(1, tempUrl.length() - 1);
    } else if (tempUrl.startsWith("'") && tempUrl.endsWith("'")) {
      tempUrl = tempUrl.substring(1, tempUrl.length() - 1);
    }
  }

  radioname.trim();
  tempUrl.trim();

  if (radioname.length() == 0 || tempUrl.length() == 0) {
    logln("Error: Empty name or URL after processing quotes in line: " +
          currentLineText);
    return totalLines;
  }

  if (tempUrl.length() >= urlMaxSize) {
    logln("Error: URL too long for buffer: " + tempUrl);
    return totalLines;
  }

  // Copiar URL al buffer char* (radioUrlBuffer en el contexto de la llamada)
  strncpy(url, tempUrl.c_str(), urlMaxSize - 1);
  url[urlMaxSize - 1] = '\0'; // Asegurar terminación null

  String mode =
      use_index ? "index " + String(index) : (forward ? "forward" : "backward");
  logln("Radio station selected (" + mode + "): " + radioname + " -> " +
        String(url));
  logln("Current position: " + String(currentLine + 1) + "/" +
        String(totalLines));

  // Devolver la linea leida.
  return currentLine + 1;
}

String getRadioUrlByIndex(int index) {
  const char *filepath = PATH_FILE_TO_LOAD.c_str();

  // 1. Verificar que el archivo de radios existe
  if (!SD_MMC.exists(filepath)) {
    logln("Error (getRadioUrlByIndex): Radio list file not found: " +
          String(filepath));
    return ""; // Devuelve una cadena vacía si el archivo no existe
  }

  // 2. Abrir el archivo para lectura
  File radioFile = SD_MMC.open(filepath, FILE_READ);
  if (!radioFile) {
    logln("Error (getRadioUrlByIndex): Could not open radio list file: " +
          String(filepath));
    return ""; // Devuelve una cadena vacía si no se puede abrir
  }

  logln("Searching for URL at index " + String(index) + " in " +
        String(filepath));

  int currentLine = 0;
  String url = "";

  // 3. Leer el archivo línea por línea hasta encontrar el índice deseado
  while (radioFile.available()) {
    String line = radioFile.readStringUntil('\n');
    line.trim();

    // Ignorar líneas vacías o sin el separador ','
    int commaIndex = line.indexOf(',');
    if (line.length() == 0 || commaIndex == -1) {
      continue;
    }

    // 4. Comprobar si la línea actual es la que buscamos
    if (currentLine == index) {
      // Extraer la URL
      url = line.substring(commaIndex + 1);
      url.trim();

      // Eliminar comillas si existen
      if (url.length() >= 2) {
        if (url.startsWith("\"") && url.endsWith("\"")) {
          url = url.substring(1, url.length() - 1);
        } else if (url.startsWith("'") && url.endsWith("'")) {
          url = url.substring(1, url.length() - 1);
        }
      }

      // Salir del bucle una vez encontrada la URL
      break;
    }

    // Incrementar el contador de líneas válidas
    currentLine++;
  }

  // 5. Cerrar el archivo
  radioFile.close();

  if (url.isEmpty()) {
    logln("Warning (getRadioUrlByIndex): Index " + String(index) +
          " not found or line is invalid.");
  } else {
    logln("Found URL at index " + String(index) + ": " + url);
  }

  // 6. Devolver la URL encontrada o una cadena vacía si no se encontró
  return url;
}

void updateDialIndicator(int pos) 
{
  // Parametros.

  // Con el reloj abierto mejor no hacemos nada con la barra del dial
  // ya que se superpone en el reloj.
  if (CURRENT_PAGE != 99)
  {
      const int xini = 130;  // x ini of dial
      const int width = 232; // dial range pixels

      if (TOTAL_BLOCKS <= 0 || BB_OPEN)
        return;

      showRadioDial();
      // No quitar esta pausa porque es necesaria para que de tiempo a acabar el
      // proceso de pintado del dial si no, se dispara el dibujado de la linea antes
      // y queda tapado por la imagen.
      delay(125);
      hmi.writeString("fill " + String(xini + (width / TOTAL_BLOCKS) * pos) +
                      ",152,5,40," + String(DIAL_COLOR));
      hmi.writeString("fill " + String(xini + (width / TOTAL_BLOCKS) * pos) +
                      ",152,5,40," + String(DIAL_COLOR));
      hmi.writeString("fill " + String(xini + (width / TOTAL_BLOCKS) * pos) +
                      ",152,5,40," + String(DIAL_COLOR));
      hmi.writeString("fill " + String(xini + (width / TOTAL_BLOCKS) * pos) +
                      ",152,5,40," + String(DIAL_COLOR));    
  }

}

void dialIndicator(bool enable) {
  if (enable) {
    hmi.writeString("tape.recIndicator.bco=" +
                    String(RADIO_SYNTONIZATION_LED_COLOR));
    hmi.writeString("tape.recIndicator.bco=" +
                    String(RADIO_SYNTONIZATION_LED_COLOR));
  } else {
    hmi.writeString("tape.recIndicator.bco=32768");
    hmi.writeString("tape.recIndicator.bco=32768");
  }
}

struct RadioNetworkTaskParams {
  URLStream *stream;
  SimpleCircularBuffer *buffer;
  volatile bool running;
  volatile bool new_url;
  volatile bool taskDone;  // la tarea lo pone a true justo antes de vTaskDelete(NULL)
  char url_buffer[256];
};

void radio_network_task(void *parameter) 
{
  logln("Heap - NET" + String(ESP.getFreeHeap()) + " (max bloque: " + String(ESP.getMaxAllocHeap()) + ")");
  RadioNetworkTaskParams *params = (RadioNetworkTaskParams *)parameter;
  uint8_t networkBuffer[RADIO_NETWORK_BUFFER_SIZE];

  // logln("Network task started on core %d", xPortGetCoreID());

  while (params->running) {
    // Si hay una nueva URL, nos conectamos
    if (params->new_url) {
      params->stream->end(); // Cerramos conexión anterior si la hubiera
      // logln("[NetTask] Connecting to: %s", params->url_buffer);
      if (params->stream->begin(params->url_buffer)) {
        logln("[NetTask] Connected.");
        params->new_url = false; // Marcamos la URL como procesada
      } else {
        logln("[NetTask] Connection failed.");
        params->new_url =
            false; // Dejamos de intentar hasta que nos den otra URL
        vTaskDelay(pdMS_TO_TICKS(1000)); // Esperamos antes de reintentar
        continue;
      }
    }

    // Si estamos conectados y hay espacio en el buffer, leemos de la red
    if (params->stream->httpRequest().connected() &&
        params->buffer->getFreeSpace() > sizeof(networkBuffer)) {
      size_t bytesRead =
          params->stream->readBytes(networkBuffer, sizeof(networkBuffer));
      if (bytesRead > 0) {
        params->buffer->write(networkBuffer, bytesRead);
      } else {
        // Si readBytes devuelve 0 o -1, puede que el stream haya terminado o
        // haya un error. Una pequeña pausa para no saturar la CPU en un bucle
        // cerrado.
        vTaskDelay(pdMS_TO_TICKS(10));
      }
    } else {
      // Si no hay conexión o el buffer está lleno, esperamos un poco.
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  params->stream->end();
  logln("Network task finished.");
  params->taskDone = true;  // marcar antes de autodestruirse
  vTaskDelete(NULL); // La tarea se autodestruye al salir
  logln("Heap - NET: " + String(ESP.getFreeHeap()) + " (max bloque: " + String(ESP.getMaxAllocHeap()) + ")");
}

// ... (código existente, incluyendo radio_network_task) ...

void RadioPlayer() {

  logln("Heap: " + String(ESP.getFreeHeap()) + " (max bloque: " + String(ESP.getMaxAllocHeap()) + ")");
  logln("[Radio INICIO] SRAM interna libre  : " + String(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
  logln("[Radio INICIO] Max bloque SRAM int  : " + String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));

  rotate_enable = false;
  ENABLE_ROTATE_FILEBROWSER = false;
  REM_ENABLE = false;
  EJECT = false;
  RADIO_IS_PLAYING = true;
  // 1. CONFIGURACIÓN DE TAREAS Y BUFFERS
  SimpleCircularBuffer radioBuffer(RADIO_BUFFER_SIZE);
  URLStream urlStream(ssid.c_str(), password);

  RadioNetworkTaskParams taskParams;
  taskParams.stream = &urlStream;
  taskParams.buffer = &radioBuffer;
  taskParams.running = false;   // no arrancar tarea hasta que se pulse PLAY
  taskParams.new_url = false;
  taskParams.taskDone = true;   // marcar como "no hay tarea viva"
  taskParams.url_buffer[0] = '\0';

  TaskHandle_t networkTaskHandle = NULL;
  // La tarea de red se crea en case 0, solo cuando PLAY es pulsado y la URL es válida

  const size_t BUFFER_START_THRESHOLD = RADIO_BUFFER_SIZE * 0.75;
  const size_t BUFFER_STOP_THRESHOLD = RADIO_BUFFER_SIZE * 0.25;
  bool isBuffering = true;
  bool dialIndicatorIsShown = false;
  // Paramos los timers y animaciones
  hmi.writeString("tape.tm0.en=0");
  hmi.writeString("tape.tm1.en=0");
  tapeAnimationOFF();
  showRadioDial();

  // Configuración del pipeline de audio
  tAudioList *audiolist = nullptr;
  kitStream.flush();

  auto cfg = kitStream.defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  kitStream.setAudioInfo(cfg);

  hmi.writeString("tape.lblFreq.txt=\"" + String(int(cfg.sample_rate / 1000)) +
                  "KHz\"");
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);
  kitStream.setVolume(MAIN_VOL / 100.0);

  IRADIO_EN = true;

  audio_tools::Equalizer3Bands eq(kitStream);
  audio_tools::ConfigEqualizer3Bands cfg_eq;

  MP3DecoderHelix decoder;
  EncodedAudioStream decodedStream(&eq, &decoder);

  cfg_eq = eq.defaultConfig();
  cfg_eq.setAudioInfo(cfg);
  cfg_eq.gain_low = EQ_LOW;
  cfg_eq.gain_medium = EQ_MID;
  cfg_eq.gain_high = EQ_HIGH;
  eq.begin(cfg_eq);

  // Variables de estado
  uint8_t playerState = 0;
  unsigned long trefresh = millis();
  size_t bufferw = 0;
  int currentRadioStation = 0;
  String radioName = "";
  static char radioUrlBuffer[256];
  bool statusSignalOk = false;
  unsigned long bufferingStartTime = 0;  // para timeout de buffering
  size_t lastBufferSnapshot = 0;         // bytes en buffer la última vez que miramos
  unsigned long lastBufferGrowth = 0;    // última vez que el buffer creció

  if (!decodedStream.begin()) {
    logln("Error initializing decoder");
    LAST_MESSAGE = "Decoder init failed";
    taskParams.running = false;
    vTaskDelay(pdMS_TO_TICKS(100));
    IRADIO_EN = false;
    hideRadioDial();
    return;
  }

  // Estado inicial
  STOP = false;
  EJECT = false;
  TOTAL_BLOCKS = generateRadioList(audiolist);
  currentRadioStation =
      nextRadioStation(PATH_FILE_TO_LOAD, radioName, radioUrlBuffer,
                       sizeof(radioUrlBuffer), true, 0, true);
  updateDialIndicator(currentRadioStation);
  LAST_MESSAGE = "Ready. Press PLAY.";
  playerState = 10;
  
  isBuffering = false;

  logln("Heap: " + String(ESP.getFreeHeap()) + " (max bloque: " + String(ESP.getMaxAllocHeap()) + ")");

  while (!EJECT) {

    // Gestión de botones FFWD/RWIND
    if (!isBuffering && (FFWIND || RWIND)) {
      dialIndicator(false);
      bufferw = 0; 
      // ✅ CORRECCIÓN DEFINITIVA: Parada y reinicio completo del pipeline de
      // audio
      logln("Avance / Retroceso de emisoras - ");
      if (PLAY) 
      {
        // 1. Detener la tarea de red para que no escriba más en el buffer.
        if (networkTaskHandle != NULL) 
        {
          taskParams.running = false;
          // Esperar a que se autodestruya; si no, forzar eliminación
          unsigned long t0 = millis();
          while (!taskParams.taskDone && millis() - t0 < 200) vTaskDelay(pdMS_TO_TICKS(10));
          if (!taskParams.taskDone) vTaskDelete(networkTaskHandle);
          networkTaskHandle = NULL;
          taskParams.taskDone = false;
          urlStream.end();              // Liberar contexto SSL/HTTP
          logln("Network task stopped for station change.");
        }

        // 2. Detener y limpiar el pipeline de audio por completo.
        decodedStream.end();
        kitStream.setMute(true);
      }

      // 3. Limpiar el buffer de software.
      radioBuffer.clear();
      //

      currentRadioStation = nextRadioStation(PATH_FILE_TO_LOAD, radioName, radioUrlBuffer, sizeof(radioUrlBuffer), FFWIND);
      updateDialIndicator(currentRadioStation);

      if (PLAY) 
      {
        isBuffering = true;
        LAST_MESSAGE = "Tuning to " + radioName + "...";

        // 4. Reiniciar el pipeline de audio.
        // kitStream.begin(cfg); // Reinicia el hardware de audio con su
        // configuración.
        kitStream.setMute(false);
        decodedStream.begin();

        // 5. Reiniciar y lanzar de nuevo la tarea de red con la nueva URL.
        taskParams.running = true;
        taskParams.taskDone = false;
        strcpy(taskParams.url_buffer, radioUrlBuffer);
        taskParams.new_url = true;
        // Dar tiempo al Idle task (Core 0) para liberar el stack del task
        // anterior antes de alloc del nuevo: evita fragmentación de SRAM interna.
        vTaskDelay(pdMS_TO_TICKS(50));
        xTaskCreatePinnedToCore(radio_network_task, "RadioNetworkTask", 8192, &taskParams, 1, &networkTaskHandle, 0);

      } else {
        LAST_MESSAGE = "Select: " + radioName;
      }

      FFWIND = RWIND = false;
      statusSignalOk = false;
      // Solo pasar a state 1 (reproducción) si PLAY estaba activo.
      // Si no, volver a state 10 para que el próximo PLAY pase por case 0
      // (que copia la URL a url_buffer y crea la tarea de red correctamente).
      playerState = PLAY ? 1 : 10;
    }

    // ✅ GESTIÓN DEL EXPLORADOR DE EMISORAS (REINTEGRADO)
    if (BB_OPEN || BB_UPDATE) {
      while (BB_OPEN || BB_UPDATE) 
      {
        hmi.openBlockMediaBrowser(audiolist);
      }
    } else if (UPDATE_HMI || UPDATE) {
      if (BLOCK_SELECTED > 0 && BLOCK_SELECTED <= TOTAL_BLOCKS) 
      {
        int currentPointer = BLOCK_SELECTED - 1;
        currentRadioStation = nextRadioStation(
            PATH_FILE_TO_LOAD, radioName, radioUrlBuffer,
            sizeof(radioUrlBuffer), true, currentPointer, true);

        // Si ya estábamos reproduciendo, cambiamos de emisora
        if (PLAY) 
        {
          bufferw = 0;
          dialIndicator(false);

          // ✅ APLICAR LA MISMA LÓGICA DE PARADA Y REINICIO AQUÍ
          // 1. Detener la tarea de red.
          if (networkTaskHandle != NULL) {
            taskParams.running = false;
            unsigned long t0 = millis();
            while (!taskParams.taskDone && millis() - t0 < 200) vTaskDelay(pdMS_TO_TICKS(10));
            if (!taskParams.taskDone) vTaskDelete(networkTaskHandle);
            networkTaskHandle = NULL;
            taskParams.taskDone = false;
            urlStream.end();              // Liberar contexto SSL/HTTP
          }

          // 2. Detener y limpiar el pipeline.
          decodedStream.end();
          kitStream.setMute(true);
          // kitStream.end();

          // 3. Limpiar buffer.
          radioBuffer.clear();
          isBuffering = true;

          // 4. Reiniciar pipeline.
          // kitStream.begin(cfg);
          kitStream.setMute(false);
          decodedStream.begin();

          // 5. Reiniciar tarea de red.
          taskParams.running = true;
          taskParams.taskDone = false;
          strcpy(taskParams.url_buffer, radioUrlBuffer);
          taskParams.new_url = true;
          // Dar tiempo al Idle task (Core 0) para liberar el stack del task
          // anterior antes de alloc del nuevo: evita fragmentación de SRAM interna.
          vTaskDelay(pdMS_TO_TICKS(50));
          xTaskCreatePinnedToCore(radio_network_task, "RadioNetworkTask", 8192,
                                  &taskParams, 1, &networkTaskHandle, 0);

          LAST_MESSAGE = "Tuning to " + radioName + "...";
          playerState = 1;
        }

      }
      UPDATE_HMI = false;
      UPDATE = false;
    }

    if (RADIO_PAGE_SHOWN) {
      // Si no, solo seleccionamos para la próxima vez que se pulse PLAY
      updateDialIndicator(currentRadioStation);
      LAST_MESSAGE = "Select: " + radioName;
      RADIO_PAGE_SHOWN = false;
      dialIndicatorIsShown = false;
    }

    // Máquina de estados del reproductor
    switch (playerState) {
    case 10: // Estado inicial esperando PLAY
      if (PLAY) {
        playerState = 0;
        if (!dialIndicatorIsShown) {
          dialIndicator(true);
          dialIndicatorIsShown = true;
        }
      }
      break;

    case 0: // Conectando a la emisora
      if (PLAY) {
        bufferw = 0;
        statusSignalOk = false;
        isBuffering = true;
        radioBuffer.clear();
        taskParams.new_url = false;
        // Parar tarea residual si existe (ej: segundo PLAY tras STOP)
        if (networkTaskHandle != NULL) {
          taskParams.running = false;
          unsigned long t0 = millis();
          while (!taskParams.taskDone && millis() - t0 < 200) vTaskDelay(pdMS_TO_TICKS(10));
          if (!taskParams.taskDone) vTaskDelete(networkTaskHandle);
          networkTaskHandle = NULL;
          taskParams.taskDone = false;
        }
        urlStream.end();

        if (!USE_SSL_STATIONS &&
            String(radioUrlBuffer).startsWith("https://")) {
          LAST_MESSAGE = "Error: SSL URL not permitted.";
          PLAY = false;
          break;
        }
        radioBuffer.clear();
        isBuffering = true;
        bufferingStartTime = millis();
        lastBufferSnapshot = 0;
        lastBufferGrowth = millis();
        logln("Station: " + radioName + " -> " + String(radioUrlBuffer));
        LAST_MESSAGE = "Connecting to " + radioName + "...";

        strcpy(taskParams.url_buffer, radioUrlBuffer);
        // Verificar que tenemos URL válida antes de lanzar la tarea
        if (taskParams.url_buffer[0] == '\0') {
          LAST_MESSAGE = "Error: no URL for station";
          PLAY = false;
          break;
        }
        taskParams.running = true;
        taskParams.taskDone = false;
        taskParams.new_url = true;
        // Dar tiempo al Idle task (Core 0) para liberar el stack del task
        // anterior antes de alloc del nuevo: evita fragmentación de SRAM interna.
        vTaskDelay(pdMS_TO_TICKS(50));
        xTaskCreatePinnedToCore(radio_network_task, "RadioNetworkTask", 8192,
                                &taskParams, 1, &networkTaskHandle, 0);

        playerState = 1;
        bufferw = 0;
        dialIndicator(true);
      }
      break;

    case 1: // ESTADO PRINCIPAL: REPRODUCCIÓN (CONSUMIDOR)
      if (PLAY) {
        if (isBuffering) {
          size_t bufAvail = radioBuffer.getAvailable();
          LAST_MESSAGE =
              "Buffering: " +
              String((bufAvail * 100) / RADIO_BUFFER_SIZE) +
              "%";

          // Si el buffer ha crecido, actualizar el timestamp
          if (bufAvail > lastBufferSnapshot) {
            lastBufferSnapshot = bufAvail;
            lastBufferGrowth = millis();
          }

          // Timeout: si el buffer no crece en RADIO_BUFFER_TIMEOUT_MS, reintentar
          if (millis() - lastBufferGrowth > RADIO_BUFFER_TIMEOUT_MS) {
            logln("[Radio] Timeout buffering - reintentando conexion...");
            LAST_MESSAGE = "Timeout - Retrying...";

            if (networkTaskHandle != NULL) {
              taskParams.running = false;
              unsigned long t0 = millis();
              while (!taskParams.taskDone && millis() - t0 < 200) vTaskDelay(pdMS_TO_TICKS(10));
              if (!taskParams.taskDone) vTaskDelete(networkTaskHandle);
              networkTaskHandle = NULL;
              taskParams.taskDone = false;
              urlStream.end();
            }
            decodedStream.end();
            radioBuffer.clear();
            decodedStream.begin();

            taskParams.running = true;
            taskParams.taskDone = false;
            taskParams.new_url = true;
            lastBufferSnapshot = 0;
            lastBufferGrowth = millis();
            // Dar tiempo al Idle task (Core 0) para liberar el stack del task
            // anterior antes de alloc del nuevo: evita fragmentación de SRAM interna.
            vTaskDelay(pdMS_TO_TICKS(50));
            xTaskCreatePinnedToCore(radio_network_task, "RadioNetworkTask", 8192,
                                    &taskParams, 1, &networkTaskHandle, 0);
          }

          if (bufAvail >= BUFFER_START_THRESHOLD) {
            logln("Buffer filled. Starting playback.");
            isBuffering = false;
            LAST_MESSAGE = "Playing: " + radioName;
          }
        } else {
          if (radioBuffer.getAvailable() < BUFFER_STOP_THRESHOLD) {
            logln("Buffer low. Pausing to re-buffer.");
            isBuffering = true;
          } else {
            size_t available = radioBuffer.getAvailable();
            size_t bytesToRead =
                min(available, (size_t)RADIO_DECODE_BUFFER_SIZE);
            if (bytesToRead > 0) {
              uint8_t decodedBuffer[bytesToRead];
              size_t bytesReadFromBuffer =
                  radioBuffer.read(decodedBuffer, bytesToRead);
              if (bytesReadFromBuffer > 0) {
                decodedStream.write(decodedBuffer, bytesReadFromBuffer);
                bufferw += bytesReadFromBuffer;
              }
            }
          }
          //
          dialIndicator(true);

        }
      } else if (STOP || PAUSE) {
        playerState = 10;
        PLAY = false;
        bufferw = 0;
        statusSignalOk = false;
        isBuffering = true;
        radioBuffer.clear();
        dialIndicator(false);

        taskParams.new_url = false;
        urlStream.end();

        LAST_MESSAGE = "Radio stopped.";
        STOP = PAUSE = false;
      }
      break;
    }

    // Actualización de la interfaz de usuario
    if ((millis() - trefresh > 1000)) {
      float bufferUsage =
          (radioBuffer.getAvailable() * 100.0) / RADIO_BUFFER_SIZE;
      PROGRESS_BAR_TOTAL_VALUE = bufferUsage;
      PROGRESS_BAR_BLOCK_VALUE = bufferUsage;
      updateIndicators(TOTAL_BLOCKS, currentRadioStation, bufferw,
                       decoder.audioInfoEx().bitrate, radioName);
      hmi.writeString("name.txt=\"" + radioName + "\"");
      trefresh = millis();
    }

    delay(10);
  }

  // Limpieza final
  logln("Stopping RADIO playback...");
  taskParams.running = false;

  // Cerrar el stream ANTES de esperar la tarea: así si la tarea está bloqueada
  // en readBytes() el socket se cierra y la tarea sale inmediatamente.
  urlStream.end();

  // Esperar a que la tarea llame a vTaskDelete(NULL); timeout generoso (3s)
  // porque la tarea llama a stream->end() interna antes de salir.
  {
    unsigned long t0 = millis();
    while (!taskParams.taskDone && millis() - t0 < 3000) vTaskDelay(pdMS_TO_TICKS(10));
    if (!taskParams.taskDone && networkTaskHandle != NULL)
    {
      logln("Radio task timeout - force kill");
      vTaskDelete(networkTaskHandle);
    }
    networkTaskHandle = NULL;
  }

  IRADIO_EN = false;
  decodedStream.end();
  decoder.end();
  kitStream.clearNotifyAudioChange();
  LAST_MESSAGE = "...";
  hideRadioDial();
  if (audiolist != nullptr) {
    free(audiolist);
  }
  dialIndicator(false);
  hmi.writeString("tape.tm0.en=1");
  hmi.writeString("tape.tm1.en=1");

  vTaskDelay(pdMS_TO_TICKS(50));
  RADIO_IS_PLAYING = false;
}
// ... (resto del código) ...

void MediaPlayer() {

  // ---------------------------------------------------------
  //
  // Reproductor de medios
  //
  // ---------------------------------------------------------
  LAST_MESSAGE = "Waiting...";
  // resetOutputCodec();

  MEDIA_PLAYER_EN = true;
  MUSIC_IS_PLAYING = true;
  EJECT = false;

  // Variables
  // ---------------------------------------------------------
  // Lista de audio
  int audioListSize = 0;
  tAudioList *audiolist;

  String ext = TYPE_FILE_LOAD;
  ext.toLowerCase();

  File *pFile = nullptr;

  // Indices de ficheros
  int currentIdx = 0;
  int currentPointer = 0;
  uint32_t fileSize = 0;
  uint32_t fileread = 0;

  int bitRateRead = 0;
  int totalFilesIdx = 0;
  int stateStreamplayer = 0;
  unsigned long lastUpdate = 0;
  unsigned long twiceRWDTime = 0;
  int wait_time_playing = 0;
  bool was_pressed_wd = false;
  int fast_wind_status = 0;

  // Busqueda rapida
  File *p_file_seek = nullptr;
  size_t p_file_seek_pos = 0;
  long t_button_pressed = 0;

  // Sampling rate
  audio_tools::sample_rate_t osr = kitStream.audioInfo().sample_rate;
  bool isFLAC = false;
  uint8_t waitflag = 0;

  int tmpoToRfsh = 1000;

  //-----------------------------------------------------------
  //
  // Declaramos y configuramos objetos necesarios
  //
  //-----------------------------------------------------------

  // Aplicamos configuración a la board
  auto cfg = kitStream.audioInfo();

  // Actuamos el amplificador
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);
  kitStream.setVolume(MAIN_VOL / 100);

  // Configuración de la fuente de audio - old version
  // ----------------------------------------------------------
  // Por defecto no descubre fichero, eso ya lo hará reindex()
  AudioSourceIdxSDMMC source(FILE_LAST_DIR.c_str(), ext.c_str(), false);
  // REGENERATE_IDX = false;

  // Configuración de los decodificadores
  // ---------------------------------------------------------
  // Decodificador MP3
  MP3DecoderHelix decoderMP3;

  // Decodificador WAV tipo PCM
  WAVDecoder decoderWAV;

  // Decodificador FLAC
  FLACDecoderFoxen decoderFLAC;
  //FLACDecoder decoderFLAC;
  // FLACDecoder hereda de StreamingDecoder, no de AudioDecoder.
  // AudioPlayer::setDecoder solo acepta AudioDecoder -> envolver con DecoderAdapter.
  //DecoderAdapter decoderFLACAdapter(decoderFLAC, 16384);

  

  // Configuración del filtrado de metadatos para el codec MP3
  // ---------------------------------------------------------
  MetaDataFilterDecoder metadatafilter(decoderMP3);

  // Configuración de las mediciones de tiempo
  // ---------------------------------------------------------
  // MeasuringStream measureWAV(decoderWAV);
  MeasuringStream measureMP3(metadatafilter);
  // MeasuringStream measureFLAC(decoderFLAC);

  // Variables para medida de tiempo
  uint32_t stime_total = 0; // Seconds for MP3
  uint32_t stime_begin = 0; // Seconds for MP3

  // Configuración del ecualizador
  // ---------------------------------------------------------
  audio_tools::Equalizer3Bands eq(kitStream);
  audio_tools::ConfigEqualizer3Bands cfg_eq;

  cfg_eq = eq.defaultConfig();
  cfg_eq.setAudioInfo(cfg);
  cfg_eq.gain_low = EQ_LOW;
  cfg_eq.gain_medium = EQ_MID;
  cfg_eq.gain_high = EQ_HIGH;
  eq.begin(cfg_eq);

  // Esto nos permite propagación del setting del fichero, sampling, bits,
  // canales.
  // WAV
  decoderWAV.addNotifyAudioChange(kitStream);
  decoderWAV.addNotifyAudioChange(eq);
  // MP3
  metadatafilter.addNotifyAudioChange(measureMP3);
  decoderMP3.addNotifyAudioChange(kitStream);
  decoderMP3.addNotifyAudioChange(eq);
  // FLAC
  decoderFLAC.addNotifyAudioChange(kitStream);
  decoderFLAC.addNotifyAudioChange(eq);

  // Configuración del reproductor
  // ---------------------------------------------------------
  AudioInfo audiosr;
  AudioPlayer player;

  player.setAudioSource(source);
  player.setOutput(eq);
  player.setVolume(1);

  auto tempConfig = kitStream.defaultConfig();

  // Esto es necesario para que el player sepa donde rediregir el audio
  switch (ext[0]) {
  case 'w':
    // WAV
    // Iniciamos el decoder
    if (decoderWAV.begin()) {
      // Configuramos temporalmente el audio a 44100Hz hasta que leamos el
      // archivo real
      tempConfig = kitStream.defaultConfig();
      tempConfig.sample_rate = 44100;
      tempConfig.bits_per_sample = 16;
      tempConfig.channels = 2;
      kitStream.setAudioInfo(tempConfig);
      eq.setAudioInfo(tempConfig);
    } else {
      logln("Error initializing WAV decoder");
      LAST_MESSAGE = "Error initializing WAV decoder";
      STOP = true;
      PLAY = false;
      return;
    }
    // decoderWAV.setOutput(eq);
    //  Configuramos el player con el decoder
    player.setDecoder(decoderWAV);

    // Otras configuraciones del player
    player.setAutoNext(AUTO_NEXT);
    player.setAutoFade(AUTO_FADE);
    logln("WAV decoder set in player");
    break;

  case 'm':
    // MP3
    measureMP3.begin();
    decoderMP3.begin();
    measureMP3.setOutput(eq);
    player.setDecoder(metadatafilter);
    player.setOutput(measureMP3);
    // Dimensionado del buffer de mp3
    player.setBufferSize(2048); // 4096 KB para MP3

    // Configuramos temporalmente el audio a 44100Hz hasta que leamos el archivo
    // real
    tempConfig = kitStream.defaultConfig();
    tempConfig.sample_rate = 44100;
    tempConfig.bits_per_sample = 16;
    tempConfig.channels = 2;
    kitStream.setAudioInfo(tempConfig);
    eq.setAudioInfo(tempConfig);

    // Otras configuraciones del player
    player.setAutoNext(AUTO_NEXT);
    player.setAutoFade(AUTO_FADE);
    logln("MP3 decoder set in player");
    break;

  case 'f':
    // FLAC
    isFLAC = true;
    tmpoToRfsh = 2000;
    // measureFLAC.begin();
    FLAC_IS_PLAYING = true;

    if (decoderFLAC.begin()) {
      logln("FLAC decoder initialized");

      // Configuramos temporalmente el audio a 44100Hz hasta que leamos el
      // archivo real
      tempConfig = kitStream.defaultConfig();
      tempConfig.sample_rate = 44100;
      tempConfig.bits_per_sample = 16;
      tempConfig.channels = 2;
      kitStream.setAudioInfo(tempConfig);
      eq.setAudioInfo(tempConfig);
    } else {
      logln("Error initializing FLAC decoder");
      LAST_MESSAGE = "Error initializing FLAC decoder";
      STOP = true;
      PLAY = false;
      return;
    }
    player.setDecoder(decoderFLAC);
    logln("FLAC decoder set in player");
    break;

  default:
    LAST_MESSAGE = "Unsupported format";
    delay(1500);
    return;
    break;
  }

#ifdef BLUETOOTH_ENABLE
  if (BLUETOOTH_ACTIVE) {
    player.setOutput(outbt);
  }
#endif

  LAST_MESSAGE = "Scanning files ...";

  // Apuntamos al fichero seleccionado en el browser
  logln("Selecting file: " + PATH_FILE_TO_LOAD);

  pFile = (File *)source.selectStream(PATH_FILE_TO_LOAD.c_str());
  if (pFile == nullptr) {
    logln("Error: Cannot open selected file: " + PATH_FILE_TO_LOAD);
    LAST_MESSAGE = "Error opening file. Eject again.";
    REGENERATE_IDX = true;
    STOP = true;
    PLAY = false;
    return;
  }
  // Obtenemos el tamaño del archivo actual
  fileSize = pFile->size();
  // Inicializamos el número de bytes leídos
  fileread = pFile->position();

  // Generamos la lista de audio del directorio raiz del fichero seleccionado.
  MEDIA_CURRENT_POINTER = 0;

  // Nos aseguramos que FILE_LAST_DIR comience con /
  if (!FILE_LAST_DIR.startsWith("/")) {
    FILE_LAST_DIR = "/" + FILE_LAST_DIR;
  }

  LAST_MESSAGE = "Buffering file list...";
  hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
  // yield();
  delay(125);

  // Primero validamos que el path comience con /
  if (!FILE_LAST_DIR.startsWith("/")) {
    logln("Invalid path format (must start with /) -> " + FILE_LAST_DIR);
    LAST_MESSAGE = "Invalid path format.";
    hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
    delay(125);
    return;
  }

  // **********************************************************
  //
  // Generamos la lista de audio y la del player la regeneramos
  //
  // **********************************************************
  audioListSize = generateAudioList(audiolist, ext);
  //
  logln("Checking path: " + FILE_LAST_DIR);
  if (!SD_MMC.exists((FILE_LAST_DIR).c_str())) {
    logln("Path not found -> " + FILE_LAST_DIR);
    LAST_MESSAGE = "Path not found.";
    hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
    delay(125);
    return;
  }

  // Reindexamos la fuente de audio.
  // source.reindex((FILE_LAST_DIR).c_str(), ext.c_str()); //*****  06102025
  // Prueba con la AudioTools original (se necesita eliminar esto)
  delay(125);

  // Obtenemos indice del fichero seleccionado
  currentPointer = MEDIA_CURRENT_POINTER;
  logln("Current pointer: " + String(currentPointer) +
        " - File: " + audiolist[currentPointer].filename);

  // Mostramos mensaje de creación de la lista de audio
  LAST_MESSAGE = "Audio list ready.";
  hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
  delay(125);
  hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
  delay(125);

  // Asignación de variables
  totalFilesIdx = audioListSize;
  TOTAL_BLOCKS = totalFilesIdx + 1;
  BLOCK_SELECTED =
      source.index() + 1; // Para mostrar el bloque seleccionado en la pantalla
  // Actualizamos indicadores de bloques
  hmi.writeString("tape.totalBlocks.val=" + String(TOTAL_BLOCKS));
  delay(125);
  hmi.writeString("tape.currentBlock.val=1");
  delay(125);
  hmi.writeString("tape.BBOK.val=1");

  // Inicialización de temporizadores
  lastUpdate = millis();
  twiceRWDTime = millis();
  t_button_pressed = millis();
  // Reset del EQ
  EQ_CHANGE = false;

  // Mostramos informacion del fichero seleccionado en el browser.
  updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead,
                   audiolist[currentPointer].filename);
  // -------------------------------------------------------------------
  // -
  // - Arranque del player
  // -
  // -------------------------------------------------------------------
  LAST_MESSAGE = "Initializing player...";
  hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
  waitflag = 0;
  while (!player.begin() && !EJECT) 
  {
    if (waitflag++ > 32) // Esperamos un maximo de 32*125ms = 4s
    {
      LAST_MESSAGE = "Error indexing files. Open again.";
      delay(2000);
      return;
    }
    delay(125);
  }

  LAST_MESSAGE = "Player ready. Press Play.";
  hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");

  // Si la salida de bluetooth está a tope, no demoramos
  #ifdef BLUETOOTH_ENABLE
    player.setDelayIfOutputFull(0);
  #endif

  // ---------------------------------------------------------------
  //
  // Bucle principal
  //
  // ---------------------------------------------------------------

  while (!EJECT && !REC) 
  {
    // Actualizamos el volumen y el ecualizador si es necesario
    // Control del ecualizador
    // if (!FLAC_IS_PLAYING) 
    // {
    if (EQ_CHANGE) 
    {
      EQ_CHANGE = false;
      cfg = kitStream.audioInfo();
      cfg_eq.setAudioInfo(cfg);
      cfg_eq.gain_low = EQ_LOW;
      cfg_eq.gain_medium = EQ_MID;
      cfg_eq.gain_high = EQ_HIGH;

      if (!eq.begin(cfg_eq)) {
        LAST_MESSAGE = "Error EQ initialization";
        STOP = true;
        PLAY = false;
        break;
      }
    }

    if (AMP_CHANGE || SPK_CHANGE) {
      kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);
      AMP_CHANGE = false;
      SPK_CHANGE = false; 
    }
  // }

    // Estados del reproductor
    switch (stateStreamplayer) {
    case 0: // Esperando reproducción

      if (PLAY) {
        // Iniciamos el reproductor
        if (WAVFILE_PRELOAD)
          WAVFILE_PRELOAD = false;

        // Primer ms
        waitflag = 0;
        while (!player.begin(currentPointer)) {
          if (waitflag++ > 255) {
            player.stop();
          }
        }

        player.setAutoNext(false);
        player.copy();
        delay(250); // Esperamos 5s a que abra el archivo

        AUDIO_FORMART_IS_VALID = true;
        // Ahora que el archivo está abierto, obtenemos su configuración real
        // Leemos la primera parte para que el decoder detecte el header
        AudioFormat format;

        if (ext == "wav") {
          // Mostramos información del fichero
          format = decoderWAV.audioInfoEx().format;
          int sr = decoderWAV.audioInfo().sample_rate;
          logln("WAV sample rate: " + String(sr) + " Hz");
          logln("WAV format: " + String((int)format));

          if (format != AudioFormat::PCM) {
            logln("WAV format not supported.");
            LAST_MESSAGE = "WAV format not supported.";
            STOP = true;
            PLAY = false;
            AUDIO_FORMART_IS_VALID = false;
            tapeAnimationOFF();
            player.stop();
            break;
          }
        }

        player.setAutoNext(AUTO_NEXT);

        audiosr = (ext == "wav")   ? decoderWAV.audioInfo()
                  : (ext == "mp3") ? decoderMP3.audioInfo()
                                   : decoderFLAC.audioInfo();
        updateSamplingRate(player, eq, audiosr);

        LAST_MESSAGE = "...";

        pFile = (File *)player.getStream();

        if (pFile != nullptr) {
          // Obtenemos el tamaño del archivo
          fileSize = pFile->size();
          logln("File size: " + String(fileSize) + " bytes");
          // Inicializamos el número de bytes leídos
          fileread = pFile->position();
        }

        // Actualización inicial de indicadores
        updateIndicators(totalFilesIdx, currentPointer + 1, fileSize,
                         bitRateRead, source.toStr());

        // Cambiamos de estado
        stateStreamplayer = 1;
        tapeAnimationON();
      }

      if (UPDATE_FROM_REMOTE_CONTROL && FILE_POS_REMOTE_CONTROL >= 0) {
        // Actualizamos el puntero desde el control remoto
        logln("File selected from remote control");
        logln("Position of the file: " + String(FILE_POS_REMOTE_CONTROL));
        UPDATE_FROM_REMOTE_CONTROL = false;
        currentPointer = FILE_POS_REMOTE_CONTROL;
        PLAY = true;
        STOP = false;
        PAUSE = false;
        fileread = 0;
      }

      break;

    case 1: // Reproduciendo PLAY

      // Salida inesperada desde MediaPlayer.
      if (!AUDIO_FORMART_IS_VALID) {
        LAST_MESSAGE = "Audio format error.";
        STOP = true;
        PLAY = false;
        stateStreamplayer = 0;
        tapeAnimationOFF();
        break;
      }

      // Con esto evitamos que se oiga el player mientras rebobinamos
      if (fast_wind_status <= 2) {
        if (CHANGE_TRACK_FILTER) {
          // Cambio de pista
          kitStream.setVolume(0);
          delay(250);
          player.copy();
          player.copy();
          kitStream.setVolume(MAIN_VOL / 100);
          CHANGE_TRACK_FILTER = false;
        } else {
          // Reproducción normal
          player.copy();
        }

        fileread = pFile->position(); // Actualizamos el número de bytes leídos

        if (fileread < 128) {
          // En los primeros 128 bytes actualizo el sampling rate
          // en los que aseguro haber leido la cabecera
          audiosr = (ext == "wav")   ? decoderWAV.audioInfo()
                    : (ext == "mp3") ? decoderMP3.audioInfo()
                                     : decoderFLAC.audioInfo();
          updateSamplingRate(player, eq, audiosr);
        }
      }

      // Cuando haya datos que mostrar, visualizamos la barra de progreso
      if (fileSize > 0) {
        // Esto lo hacemos asi para evitar hacer out of range del tipo de
        // variable.
        PROGRESS_BAR_TOTAL_VALUE = (fileread / (fileSize / 100));
      }

      // Control de EOF
      if (fileread == fileSize) {
        // Se ha alcanzado el ultimo fichero de la lista y no hay mas
        if ((currentPointer + 1) >= (TOTAL_BLOCKS - 1)) {
          // Modalidad 1:
          // El fichero es el ultimo y no hay loop
          if (!disable_auto_media_stop) {
            fileread = 0;
            stateStreamplayer = 4; // Auto-stop
            hmi.writeString("tape.currentBlock.val=1");
            updateIndicators(totalFilesIdx, 1, fileSize, bitRateRead,
                             audiolist[0].filename);
            delay(125);
          }
          // Modalidad 2:
          // El fichero es el ultimo y pero se hace loop
          else {
            //
            // Comenzamos desde el principio de la playlist
            //
            BLOCK_SELECTED = 0;

            currentPointer = currentIdx = 0; // Reiniciamos el índice
            fileread = 0;
            hmi.writeString("tape.currentBlock.val=1");
            delay(125);

            waitflag = 0;
            while (!player.begin(currentIdx)) {
              if (waitflag++ > 255) {
                player.stop();
              }
            }
            // Reiniciar el reproductor
            fileSize = getStreamfileSize(pFile);
            updateIndicators(totalFilesIdx, currentPointer, fileSize,
                             bitRateRead, audiolist[currentPointer].filename);
            delay(125);
          }
        } else {
          // El player continua solo a la siguiente pista no hace falta forzarlo
          // actualizamos el puntero.
          currentPointer++;
          currentIdx = currentPointer;
          // Esto es un nuevo fichero en el player
          // player.stop();

          fileread = 0;

          waitflag = 0;
          // Iniciamos el reproductor
          player.stop();
          while (!player.begin(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
          }

          fileSize = getStreamfileSize(pFile);
          updateIndicators(totalFilesIdx, currentPointer, fileSize, bitRateRead,
                           audiolist[currentPointer].filename);
          delay(125);
        }
      }

      if (STOP) {
        stateStreamplayer = 0;
        fileread = 0;
        tapeAnimationOFF();
      }

      if (PAUSE) {
        stateStreamplayer = 2; // Pausa
        tapeAnimationOFF();
        PLAY = false;
        PAUSE = false;
      }

      if (UPDATE_FROM_REMOTE_CONTROL && FILE_POS_REMOTE_CONTROL >= 0) {
        // Actualizamos el puntero desde el control remoto
        logln("File selected from remote control");
        logln("Position of the file: " + String(FILE_POS_REMOTE_CONTROL));
        UPDATE_FROM_REMOTE_CONTROL = false;
        currentPointer = FILE_POS_REMOTE_CONTROL;
        PLAY = true;
        STOP = false;
        PAUSE = false;
        fileread = 0;
        player.stop();
        delay(125);
        stateStreamplayer = 0;
      }
      // Actualizamos indicadores cada 2 segundos
      // En la actualización de indicadores cada segundo

      if (millis() - lastUpdate > tmpoToRfsh) {
        // Mostramos informacion del fichero.
        updateIndicators(totalFilesIdx, currentPointer + 1, fileSize,
                         bitRateRead, audiolist[currentPointer].filename);

        uint32_t stime_total = 0;   // Tiempo total del archivo en segundos
        uint32_t stime_elapsed = 0; // Tiempo transcurrido en segundos

        // Variables estáticas para controlar el tiempo de inicio
        static unsigned long playStartTime = 0;
        static bool timeInitialized = false;

        static int lastCurrentPointer = -1; // Rastrear cambios de canción

        // REINICIAR temporizador cuando cambia la canción
        if (lastCurrentPointer != currentPointer) {
          timeInitialized = false;
          lastCurrentPointer = currentPointer;
          // logln("Song changed - resetting timer for: " +
          // audiolist[currentPointer].filename);
        }

        // Inicializar el tiempo de inicio la primera vez
        if (!timeInitialized) {
          playStartTime = millis();
          timeInitialized = true;
        }

        if (ext == "mp3") {

          uint32_t stime_total_ms = 0;
          uint32_t stime_elapsed_ms = 0;

          // OPCIÓN 1: Intentar usar el MeasuringStream primero
          stime_total_ms = measureMP3.estimatedOpenTimeFor(pFile->size());
          stime_elapsed_ms = measureMP3.estimatedOpenTimeFor(pFile->position());

          // Validar si los resultados son razonables (no más de 24 horas)
          if (stime_total_ms > 0 && stime_total_ms < 86400000) // 24 horas en ms
          {
            stime_total = stime_total_ms / 1000;
            stime_elapsed = stime_elapsed_ms / 1000;
          } else {
            // OPCIÓN 2: Fallback con estimación basada en bitrate promedio MP3

            // Intentar obtener el bitrate real del MP3 si está disponible
            uint32_t mp3_bitrate = 0;

            // Algunos decodificadores pueden proporcionar el bitrate
            // Si no está disponible, usar estimación basada en tamaño/tiempo
            // conocido

            if (mp3_bitrate == 0) {
              // Estimación basada en bitrates comunes de MP3
              // Análisis heurístico del tamaño del archivo
              if (fileSize < 2000000) // < 2MB, probablemente 96-128 kbps
              {
                mp3_bitrate = 112000; // 112 kbps promedio
              } else if (fileSize <
                         5000000) // < 5MB, probablemente 128-192 kbps
              {
                mp3_bitrate = 160000; // 160 kbps promedio
              } else                  // > 5MB, probablemente 192-320 kbps
              {
                mp3_bitrate = 256000; // 256 kbps promedio
              }
            }

            // Calcular tiempo basado en bitrate estimado
            uint32_t bytesPerSecond = mp3_bitrate / 8;
            stime_total = fileSize / bytesPerSecond;
            stime_elapsed = pFile->position() / bytesPerSecond;

#ifdef DEBUG
            logln("MP3 Fallback calculation:");
            logln("Estimated bitrate: " + String(mp3_bitrate) + " bps");
            logln("Bytes per second: " + String(bytesPerSecond));
            logln("File size: " + String(fileSize) + " bytes");
            logln("Calculated duration: " + String(stime_total) + " seconds");
#endif
          }

#ifdef DEBUG
          AudioInfo info = decoderMP3.audioInfo();
          logln("MP3 Decoder Info:");
          logln("Sample Rate: " + String(info.sample_rate) + " Hz");
          logln("Channels: " + String(info.channels));
          logln("Bits per sample: " + String(info.bits_per_sample));
          logln("Calculated total time: " + String(stime_total) + " seconds");
          logln("Calculated elapsed time: " + String(stime_elapsed) +
                " seconds");
#endif

          // Validar que los tiempos sean razonables
          if (stime_total >
              86400) // Si es más de 24 horas, probablemente hay error
          {
            stime_total = fileSize / 16000; // Usar estimación básica
            stime_elapsed = pFile->position() / 16000;
          }

          // Limitar tiempos a máximo 59:59
          if (stime_total > 3599)
            stime_total = 3599; // 59:59 = 3599 segundos
          if (stime_elapsed > 3599)
            stime_elapsed = 3599; // 59:59 = 3599 segundos

          // Asegurar que el tiempo transcurrido no sea mayor que el total
          if (stime_elapsed > stime_total)
            stime_elapsed = stime_total;

          // Convertir tiempo transcurrido
          uint32_t tiempoElapsed = stime_elapsed;
          uint32_t minutosElapsed = tiempoElapsed / 60;
          uint32_t segundosElapsed = tiempoElapsed % 60;

          // Limitar minutos a máximo 59
          if (minutosElapsed > 59)
            minutosElapsed = 59;

          String tiempoElapsedStr =
              (minutosElapsed < 10 ? "0" : "") + String(minutosElapsed) + ":" +
              (segundosElapsed < 10 ? "0" : "") + String(segundosElapsed);

          // Convertir tiempo total
          uint32_t tiempoTotal = stime_total;
          uint32_t minutosTotal = tiempoTotal / 60;
          uint32_t segundosTotal = tiempoTotal % 60;

          // Limitar minutos a máximo 59
          if (minutosTotal > 59)
            minutosTotal = 59;

          String tiempoTotalStr =
              (minutosTotal < 10 ? "0" : "") + String(minutosTotal) + ":" +
              (segundosTotal < 10 ? "0" : "") + String(segundosTotal);

          // Mostrar ambos tiempos
          LAST_MESSAGE = "Time: " + tiempoElapsedStr + " /  " + tiempoTotalStr;

#ifdef DEBUG
          logln("Elapsed seconds: " + String(stime_elapsed) +
                " Total seconds: " + String(stime_total));
          logln("File size: " + String(fileSize) +
                " File read: " + String(fileread));
#endif
        } else {
          // Mostrar mensaje de espera durante los primeros 40 segundos
          // int secondsRemaining = TIME_TO_SHOW_ESTIMATED_TIME - (elapsedTime /
          // 1000);
          LAST_MESSAGE = "Time: --:-- /  --:--";
        }

        lastUpdate = millis();

        // Reinicializar el temporizador cuando se detiene la reproducción
        if (STOP || PAUSE) {
          timeInitialized = false;
          lastCurrentPointer = -1; // Reset también el tracking de canciones
        }
      }
      break;

    case 2: // PAUSE
      if (PAUSE || PLAY) {
        stateStreamplayer = 1; // Reproduciendo
        tapeAnimationON();
        PAUSE = false;
      } else if (STOP) {
        stateStreamplayer = 0;
        fileread = 0;
        tapeAnimationOFF();
      }
      break;

    case 4: // Auto-stop
      player.stop();
      tapeAnimationOFF();

      // Reiniciamos el índice
      BLOCK_SELECTED = 0;
      currentPointer = currentIdx = 0;
      waitflag = 0;
      while (!source.selectStream(currentIdx)) {
        if (waitflag++ > 255) {
          player.stop();
        }
        delay(125);
      }
      // Preparamos el fichero a reproducir
      fileSize = pFile->size();
      // Actualizamos estados del player
      stateStreamplayer = 0;
      PLAY = false;
      STOP = true;
      // Reiniciar el reproductor
      fileread = 0;
      waitflag = 0;
      while (!player.begin(currentIdx)) {
        if (waitflag++ > 255) {
          player.stop();
        }
      }
      break;

    default:
      break;
    }

    // Control de avance/retroceso
    if ((FFWIND || RWIND) && !was_pressed_wd) {
      // LAST_MESSAGE = "Searching...";
      rewindAnimation(FFWIND ? 1 : -1);

      if (FFWIND) {
        if ((currentPointer + 1) >= (TOTAL_BLOCKS - 1)) {
          currentPointer = currentIdx = 0; // Reiniciamos el índice

          waitflag = 0;
          while (!source.selectStream(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
            delay(125);
          }

        } else {
          currentPointer++; // = source.index();
          currentIdx = currentPointer;
          // currentIdx =
          // source.indexOf((audiolist[currentPointer].filename).c_str());
          CHANGE_TRACK_FILTER = true;
          player.stop(); // Detener el reproductor

          waitflag = 0;
          while (!player.begin(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
          }
        }
        // Avanzamos al siguiente bloque
#ifdef DEBUG
        logln("FFWIND pressed. Moving to next track.");
        logln("Next file: " + String(currentPointer + " - " +
                                     audiolist[currentPointer + 1].filename));
#endif

        // 26/11/2025
        fileSize = getStreamfileSize(pFile, true);

      } else {
        // Modo origen solo se hace en reproducción. Si se pulsa RWD dentro del
        // tiempo TIME_MAX_TO_PREVIOUS_TRACK se pasa al fichero anterior, en
        // otro caso se reproduce desde el principio.
        if ((millis() - twiceRWDTime > TIME_MAX_TO_PREVIOUS_TRACK) && PLAY) {
          // Empiezo desde el principio
          currentIdx = currentPointer;
          player.stop();
          waitflag = 0;

          while (!player.begin(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
          }
          //
          twiceRWDTime = millis();
          CHANGE_TRACK_FILTER = true;

          // 26/11/2025
          fileSize = getStreamfileSize(pFile, true);

        } else {
          // Volvemos al inicio de la pista
          prevAudio(currentPointer, audioListSize);
          // LAST_MESSAGE = "Searching...";
          currentIdx = currentPointer;
          waitflag = 0;
          while (!source.selectStream(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
            delay(125);
          }
        }

        // 26/11/2025
        fileSize = getStreamfileSize(pFile, true);
      }

      // Control de EOF
      if (fileread >= fileSize) {
        // Fin de la lista
        if ((currentPointer + 1) >= (TOTAL_BLOCKS - 1)) {
          BLOCK_SELECTED = 0;
          currentPointer = 0; // Reiniciamos el índice
          currentIdx = currentPointer;
          hmi.writeString("tape.currentBlock.val=" +
                          String(currentPointer + 1));
          delay(125);
          hmi.writeString("tape.currentBlock.val=" +
                          String(currentPointer + 1));
          fileread = 0;
          waitflag = 0;
          while (!player.begin(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
          }
          // Reiniciar el reproductor
          player.stop(); // Detener el reproductor
          fileSize = getStreamfileSize(pFile);
          STOP = true;
          PLAY = false;
        }
      } else {
        if ((currentPointer + 1) > (TOTAL_BLOCKS)) {
          BLOCK_SELECTED = 0;
          currentPointer = 0; // Reiniciamos el índice
          currentIdx = currentPointer;
          hmi.writeString("tape.currentBlock.val=" +
                          String(currentPointer + 1));
          delay(125);
          hmi.writeString("tape.currentBlock.val=" +
                          String(currentPointer + 1));
          waitflag = 0;
          while (!source.selectStream(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
            delay(125);
          }
          fileread = 0;
          waitflag = 0;
          while (!player.begin(currentIdx)) {
            if (waitflag++ > 255) {
              player.stop();
            }
          }
          // Reiniciar el reproductor
          player.stop(); // Detener el reproductor
          fileSize = getStreamfileSize(pFile);
          STOP = true;
          PLAY = false;
        }
      }

      FFWIND = RWIND = false;
      fileread = 0;
      //
      updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead,
                       audiolist[currentPointer].filename);

      // Para evitar errores de lectura en el buffer
      delay(250);

      // Cargamos la cabecera del nuevo fichero
      player.setAutoNext(false);
      if (player.copy() != 0) {
        LAST_MESSAGE = "...";
      }

      AudioFormat format;
      AUDIO_FORMART_IS_VALID = true;
      if (ext == "wav") {
        format = decoderWAV.audioInfoEx().format;
        logln("WAV format: " + String((int)format));

        if (format != AudioFormat::PCM) {
          logln("WAV format not supported.");
          LAST_MESSAGE = "WAV format not supported.";
          AUDIO_FORMART_IS_VALID = false;
          STOP = true;
          PLAY = false;
          tapeAnimationOFF();
          stateStreamplayer = 0;
          player.stop();
        }
      }

      player.setAutoNext(AUTO_NEXT);

    } else if ((FFWIND || RWIND) && was_pressed_wd) {
      // Tras despulsar Avance rapido o Retroceso rapido
      was_pressed_wd = false;
      KEEP_FFWIND = false;
      KEEP_RWIND = false;
      FFWIND = false;
      RWIND = false;

      // Recuperamos el sampling rate
      AudioInfo info = kitStream.audioInfo();
      info.sample_rate = osr;
      kitStream.setAudioInfo(info);

      hmi.writeString("tape.stepTape.val=1");
      if (p_file_seek != nullptr) {
        fileread = p_file_seek->position();
        fileSize = p_file_seek->size();
      }
      // reseteamos el estado fast_wind
      fast_wind_status = 0;
    }

    // Avance rapido y retroceso rapido
    if (KEEP_FFWIND || KEEP_RWIND) {
      if (KEEP_FFWIND && ((currentPointer + 1) <= TOTAL_BLOCKS)) {
        // Avance rapido (acelerado)
        if (fast_wind_status == 0) {
          osr = kitStream.audioInfo().sample_rate;
          // Ajustamos al SR mas alto para avance rapido
          AudioInfo info = kitStream.audioInfo();
          info.sample_rate = 352800;
          kitStream.setAudioInfo(info);
          hmi.writeString("tape.stepTape.val=4");
          //
          p_file_seek = (File *)player.getStream();
          if (p_file_seek != nullptr) {
            p_file_seek_pos = p_file_seek->position();
          }

          t_button_pressed = millis();
          logln("Avance rapido");

          //
          fast_wind_status = 1;
        } else {
          if (millis() - t_button_pressed > TIME_TO_FAST_FORWRD) {
            if (fast_wind_status == 1) {
              fileSize = getStreamfileSize(pFile);
              logln("Avance ultra rapido");

              fast_wind_status = 2;
            } else if (fast_wind_status == 2) {
              // Avance ultra-rapido
              if (p_file_seek != nullptr) {
                if (p_file_seek_pos <
                    (p_file_seek->size() -
                     (p_file_seek->size() * FAST_FORWARD_PER))) {
                  p_file_seek_pos += (p_file_seek->size() * FAST_FORWARD_PER);
                  p_file_seek->seek(p_file_seek_pos);
                  if (p_file_seek != nullptr) {
                    fileread = p_file_seek->position();
                    fileSize = p_file_seek->size();
                  }
                  delay(DELAY_ON_EACH_STEP_FAST_FORWARD);
                }
              }
            }
          }
        }
      } else if (KEEP_RWIND && ((currentPointer + 1) <= TOTAL_BLOCKS)) {
        // Retroceso rapido
        if (fast_wind_status == 0) {
          // Capturamos el sample rate original antes de cambiarlo
          osr = kitStream.audioInfo().sample_rate;
          //
          p_file_seek = (File *)player.getStream();
          if (p_file_seek != nullptr) {
            p_file_seek_pos = p_file_seek->position();
          }
          logln("Retroceso rapido");
          fileSize = getStreamfileSize(pFile);
          // Entramos en modo retroceder
          fast_wind_status = 1;
        } else if (fast_wind_status == 1) {
          if (p_file_seek != nullptr) {
            size_t rewind_amount = p_file_seek->size() * FAST_REWIND_PER;

            if (p_file_seek_pos > rewind_amount) {
              p_file_seek_pos -= rewind_amount;
            } else {
              // Si no hay suficiente para retroceder, ir al inicio
              p_file_seek_pos = 0;
            }

            p_file_seek->seek(p_file_seek_pos);

            if (p_file_seek != nullptr) {
              fileread = p_file_seek->position();
              fileSize = p_file_seek->size();
            }

#ifdef DEBUG
            logln("Retroceso: " + String(rewind_amount) + " bytes");
            logln("Posicion retroceso: " + String(p_file_seek_pos));
#endif

            delay(DELAY_ON_EACH_STEP_FAST_REWIND);
          }
        }
      }
      //
      was_pressed_wd = true;
    }

    // Seleccion de pista con Block Browser
    if (BB_OPEN || BB_UPDATE) {
      //
      hmi.openBlockMediaBrowser(audiolist);
    }
    // Salida del Block Browser
    else if (UPDATE_HMI) {
      logln("UPDATE HMI");

      // *******************************************************
      // Hemos seleccionado una pista en la lista de audio
      // *******************************************************
      if (BLOCK_SELECTED > 0 && BLOCK_SELECTED <= TOTAL_BLOCKS) {
        fileread = 0;
        // Cogemos el indice que empieza desde 0 ..
        currentPointer = BLOCK_SELECTED - 1;
        currentIdx = currentPointer;
        logln("Selected file: " + (audiolist[currentPointer].filename) +
              " - Index: " + String(currentIdx));
        player.stop(); // Detener el reproductor
        waitflag = 0;
        while (!player.begin(currentIdx)) {
          if (waitflag++ > 255) {
            player.stop();
          }
        }
        // Iniciamos el reproductor
        fileSize = getStreamfileSize(pFile);

        // Esto lo hacemos para coger el sampling rate
        sample_rate_t srd = player.audioInfo().sample_rate;
        hmi.writeString("tape.lblFreq.txt=\"" + String(int(srd / 1000)) +
                        "KHz\"");
        // Actualizamos HMI
        updateIndicators(totalFilesIdx, currentPointer + 1, fileSize,
                         bitRateRead, audiolist[currentPointer].filename);
      }

      UPDATE_HMI = false;
    }
    // Seleccion de pista mediante el BLOCK_SELECTED
    else if (UPDATE) {
      logln("UPDATE");

      // Si el bloque seleccionado es válido y no es el último
      if (BLOCK_SELECTED > 0 && (BLOCK_SELECTED <= (totalFilesIdx + 1))) {
        // Reproducir el bloque seleccionado
        currentPointer = BLOCK_SELECTED - 1;
        currentIdx = currentPointer;
        player.stop(); // Detener el reproductor
        // player.end();
        waitflag = 0;
        while (!player.begin(currentIdx)) {
          if (waitflag++ > 255) {
            player.stop();
          }
        }

      } else {
        // Reproducir desde el principio
        currentPointer = 0; // Reiniciamos el índice
        currentIdx = currentPointer;
        player.stop(); // Detener el reproductor
        // player.end();
        waitflag = 0;
        while (!player.begin(currentIdx)) {
          if (waitflag++ > 255) {
            player.stop();
          }
        }
      }
      // Actualizamos de manera inmediata
      //
      delay(250);

      fileSize = getStreamfileSize(pFile);

      // Esto lo hacemos para indicar el sampling rate del reproductor
      if (ext == "mp3")
        bitRateRead = 0;
      else if (ext == "wav")
        bitRateRead = decoderWAV.audioInfoEx().byte_rate;
      else
        bitRateRead = 0;

      // bitRateRead = isWav ? decoderWAV.audioInfoEx().byte_rate : 0;
      updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead,
                       audiolist[currentPointer].filename);
      //
      UPDATE = false;
    }
  }

  // -----------------------------------------------------
  //
  // Finalizamos
  //
  //

  // Actualizamos la configuración de audio según el archivo actual
  auto finalAudioInfo = player.audioInfo();
  logln("Final Audio Settings - Sample Rate: " +
        String(finalAudioInfo.sample_rate) +
        "Hz, Bits: " + String(finalAudioInfo.bits_per_sample) +
        ", Channels: " + String(finalAudioInfo.channels));
  updateSamplingRateIndicator(finalAudioInfo.sample_rate,
                              finalAudioInfo.bits_per_sample,
                              ext); // Paramos animación
  tapeAnimationOFF();

  // Descargamos objetos
  // player.end();
  eq.end();

  // Desvinculamos todas las notificaciones. Importante para evitar problemas
  decoderMP3.clearNotifyAudioChange();
  decoderWAV.clearNotifyAudioChange();
  decoderFLAC.clearNotifyAudioChange();
  //
  decoderMP3.end();
  decoderWAV.end();
  decoderFLAC.end();
  measureMP3.end();
  metadatafilter.end();

  // Desvinculamos todas las notificaciones. Importante para evitar problemas
  kitStream.clearNotifyAudioChange();

  // Cerramos el puntero de avance rapido y lo eliminamos.
  if (p_file_seek != nullptr && p_file_seek) {
    p_file_seek->close();
  }

  // Liberamos la memoria del audiolist
  free(audiolist);
  MUSIC_IS_PLAYING = false;
  FLAC_IS_PLAYING = false;
}

// Función helper para cambiar configuración de audio de manera segura
bool setAudioInfoSafe(audio_tools::AudioInfo newInfo,
                      const String &context = "") {
  static audio_tools::AudioInfo lastConfig;
  static bool firstTime = true;

  // Verificar si la configuración es diferente
  if (!firstTime && lastConfig.sample_rate == newInfo.sample_rate &&
      lastConfig.channels == newInfo.channels &&
      lastConfig.bits_per_sample == newInfo.bits_per_sample) {
      #ifdef DEBUGMODE
          logln("Audio config unchanged, skipping: " + context);
      #endif
    return true;
  }

  try {
    // Acabar lo que tenia en curso
    kitStream.flush();

    // Aplicar nueva configuración
    kitStream.setAudioInfo(newInfo);
    return true;
  } catch (...) {
    #ifdef DEBUGMODE
        logln("Exception while updating audio config: " + context);
    #endif
    return false;
  }
}

// Modificar MediaPlayer() - configuración inicial
void playingFile() {
  rotate_enable = true;
  kitStream.setVolume(MAIN_VOL / 100.0);
  kitStream.setPAPower(ACTIVE_AMP);

  auto new_sr = kitStream.defaultConfig();
  new_sr.channels = 2;

  LAST_SAMPLING_RATE = SAMPLING_RATE + TONE_ADJUST;
  SAMPLING_RATE = BASE_SR + TONE_ADJUST;
  new_sr.sample_rate = BASE_SR + TONE_ADJUST;

  kitStream.setAudioInfo(new_sr);

  // Inicializamos el nivel de la señal según la polarización seleccionada
  // EDGE_EAR_IS = INVERSETRAIN ? POLARIZATION ^ 1: POLARIZATION;

  // Por defecto

  if (TYPE_FILE_LOAD == "TAP") {
    // Ajustamos la salida de audio al valor estandar de las maquinas de 8 bits
    // new_sr = kitStream.defaultConfig();

    if (!setAudioInfoSafe(new_sr, "TAP playback")) {
      LAST_MESSAGE = "Error configuring audio for TAP";
      STOP = true;
      PLAY = false;
      return;
    }

    //
    if (OUT_TO_WAV) {
      // Configuramos el encoder WAV directament
      AudioInfo wavencodercfg(DEFAULT_WAV_SAMPLING_RATE_REC, 2, 16);
      // Iniciamos el stream
      encoderOutWAV.begin(wavencodercfg);

      // Verificamos la configuración final
      logln("Sampling rate changed for TAP file: " +
            String(DEFAULT_WAV_SAMPLING_RATE_REC) + "Hz");
      logln("WAV encoder - Out to WAV: " +
            String(encoderOutWAV.audioInfo().sample_rate) +
            "Hz, Bits: " + String(encoderOutWAV.audioInfo().bits_per_sample) +
            ", Channels: " + String(encoderOutWAV.audioInfo().channels));

      LAST_MESSAGE = "Now output will be muted.";
      delay(2000);
    }

    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE / 1000)) +
                    "KHz\"");
    //
    sendStatus(REC_ST, 0);

    // Reiniciamos estas variables para que no aparezca informacion erronea
    // en la pagina debug
    TOTAL_PARTS = 0;
    PARTITION_BLOCK = 0;
    //
    pTAP.play();
    // Paramos la animación
    tapeAnimationOFF();

    if (OUT_TO_WAV) {
      encoderOutWAV.end();
    }
  } else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" ||
             TYPE_FILE_LOAD == "TSX") {
    // Ajustamos la salida de audio al valor estandar de las maquinas de 8 bits
    // new_sr = kitStream.defaultConfig();
    // LAST_SAMPLING_RATE = SAMPLING_RATE + TONE_ADJUST;
    // SAMPLING_RATE = BASE_SR + TONE_ADJUST;
    // new_sr.sample_rate = BASE_SR + TONE_ADJUST;

    logln("New sampling rate = " + String(SAMPLING_RATE));

    if (!setAudioInfoSafe(new_sr, "TZX playback")) {
      LAST_MESSAGE = "Error configuring audio for TZX";
      STOP = true;
      PLAY = false;
      return;
    }

    // kitStream.setAudioInfo(new_sr);

    //
    if (OUT_TO_WAV) {
      // Configuramos el encoder WAV directament
      AudioInfo wavencodercfg(DEFAULT_WAV_SAMPLING_RATE_REC, 2, 16);
      // Iniciamos el stream
      encoderOutWAV.begin(wavencodercfg);

      // Verificamos la configuración final
      logln("Sampling rate changed for TZX format file: " +
            String(DEFAULT_WAV_SAMPLING_RATE_REC) + "Hz");
      logln("WAV encoder - Out to WAV: " +
            String(encoderOutWAV.audioInfo().sample_rate) +
            "Hz, Bits: " + String(encoderOutWAV.audioInfo().bits_per_sample) +
            ", Channels: " + String(encoderOutWAV.audioInfo().channels));

      LAST_MESSAGE = "Now output will be muted.";
      delay(2000);
    }

    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE / 1000)) +
                    "KHz\"");
    //
    sendStatus(REC_ST, 0);
    // Reiniciamos estas variables para que no aparezca informacion erronea
    // en la pagina debug
    TOTAL_PARTS = 0;
    PARTITION_BLOCK = 0;
    //
    pTZX.play();
    // Paramos la animación
    tapeAnimationOFF();
    //
    if (OUT_TO_WAV) {
      encoderOutWAV.end();
    }
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Ajustamos la salida de audio al valor estandar de las maquinas de 8 bits
    // new_sr = kitStream.defaultConfig();
    // LAST_SAMPLING_RATE = SAMPLING_RATE + TONE_ADJUST;
    // SAMPLING_RATE = BASE_SR + TONE_ADJUST;
    // new_sr.sample_rate = BASE_SR + TONE_ADJUST;

    logln("New sampling rate = " + String(SAMPLING_RATE));

    if (!setAudioInfoSafe(new_sr, "PZX playback")) {
      LAST_MESSAGE = "Error configuring audio for PZX";
      STOP = true;
      PLAY = false;
      return;
    }

    // kitStream.setAudioInfo(new_sr);

    if (OUT_TO_WAV) {
      // Configuramos el encoder WAV directament
      AudioInfo wavencodercfg(DEFAULT_WAV_SAMPLING_RATE_REC, 2, 16);
      // Iniciamos el stream
      encoderOutWAV.begin(wavencodercfg);

      // Verificamos la configuración final
      logln("Sampling rate changed for PZX format file: " +
            String(DEFAULT_WAV_SAMPLING_RATE_REC) + "Hz");
      logln("WAV encoder - Out to WAV: " +
            String(encoderOutWAV.audioInfo().sample_rate) +
            "Hz, Bits: " + String(encoderOutWAV.audioInfo().bits_per_sample) +
            ", Channels: " + String(encoderOutWAV.audioInfo().channels));
    }

    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE / 1000)) +
                    "KHz\"");
    //
    sendStatus(REC_ST, 0);
    // Reiniciamos estas variables para que no aparezca informacion erronea
    // en la pagina debug
    TOTAL_PARTS = 0;
    PARTITION_BLOCK = 0;
    //
    pPZX.play();
    // Paramos la animación
    tapeAnimationOFF();
    //
  } else if (TYPE_FILE_LOAD == "WAV") {
    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" +
                    String(int(DEFAULT_WAV_SAMPLING_RATE / 1000)) + "KHz\"");

    logln("Type file load: " + TYPE_FILE_LOAD +
          " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    LAST_MESSAGE = "Wait for scanning end.";

    MediaPlayer();
    logln("Finish WAV playing file");
  } else if (TYPE_FILE_LOAD == "MP3") {
    logln("Type file load: " + TYPE_FILE_LOAD +
          " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el MP3 file
    LAST_MESSAGE = "Wait for scanning end.";
    MediaPlayer();
    logln("Finish MP3 playing file");
  } else if (TYPE_FILE_LOAD == "FLAC") {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Reproducimos el FLAC file
    LAST_MESSAGE = "Wait for scanning end.";
    MediaPlayer();
    // FLACplayer();
    logln("Finish FLAC playing file");
  } else if (TYPE_FILE_LOAD == "RADIO") {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Reproducimos el FLAC file
    if (WIFI_CONNECTED) {
      RadioPlayer();
      logln("Finish RADIO playing.");
    } else {
      LAST_MESSAGE = "WIFI not connected.";
    }
  } else if (TYPE_FILE_LOAD == "ZXDB") {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Reproducimos el ZXDB file
    LAST_MESSAGE = "Wait for scanning end.";
    //ZXDBPlayer();
    logln("Finish ZXDB playing file");
  } else if (TYPE_FILE_LOAD == "CPCDB") {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Fichero virtual CPCDB - la descarga se gestiona desde HMI
    LAST_MESSAGE = "Wait for scanning end.";
    logln("Finish CPCDB playing file");
  } else if (TYPE_FILE_LOAD == "MSXDB") {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Fichero virtual MSXDB - la descarga se gestiona desde HMI
    LAST_MESSAGE = "Wait for scanning end.";
    logln("Finish MSXDB playing file");
  } else {
    logAlert("Unknown type_file_load");
  }

  // Modificado 17/10/2025

  // Por defecto
  // Cambiamos el sampling rate en el HW
  SAMPLING_RATE = LAST_SAMPLING_RATE;
  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);
  // Indicamos el sampling rate
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE / 1000)) +
                  "KHz\"");
}

void verifyConfigFileForSelection() {
  // Vamos a localizar el fichero de configuracion especifico para el fichero
  // seleccionado
  const int max_params = 5;
  String path = FILE_LAST_DIR;
  String tmpPath = PATH_FILE_TO_LOAD;
  tConfig *fileCfg; // = (tConfig*)ps_calloc(max_params,sizeof(tConfig));
  char strpath[tmpPath.length() + 5] = {};
  strcpy(strpath, tmpPath.c_str());
  strcat(strpath, ".cfg");

  // Abrimos el fichero de configuracion.
  File cfg;

  logln("");
  log("path: " + String(strpath));

  if (cfg) {
    cfg.close();
  }

  if (SD_MMC.exists(strpath)) {
    logln("");
    log("cfg path: " + String(strpath));

    cfg = SD_MMC.open(strpath, FILE_READ);
    if (cfg) {
      // Leemos todos los parametros
      // cfg.rewind();
      fileCfg = readAllParamCfg(cfg, max_params);

      logln("");
      log("cfg open");

      for (int i = 0; i < max_params; i++) {
        logln("");
        log("Param: " + fileCfg[i].cfgLine);

        if ((fileCfg[i].cfgLine).indexOf("freq") != -1) {
          SAMPLING_RATE = (getValueOfParam(fileCfg[i].cfgLine, "freq")).toInt();

          // CAmbiamos el sampling rate del hardware de salida
          // auto cfg = kitStream.audioInfo();
          auto cfg = kitStream.defaultConfig();
          cfg.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg);

          // auto cfg2 = kitStream.audioInfo();
          auto cfg2 = kitStream.defaultConfig();
          cfg2.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg2);

          logln("");
          log("Sampling rate: " + String(SAMPLING_RATE));

          int sr_sel = (int)SAMPLING_RATE;

          switch (sr_sel) {
          case 48000: {
            hmi.writeString("menuAudio2.r0.val=1");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=0");
            break;
          }
          case 44100: {
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=1");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=0");
            break;
          }
          case 32000: {
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=1");
            hmi.writeString("menuAudio2.r3.val=0");
            break;
          }
          case 22050: {
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=1");
            break;
          }
          default: {
            // Por defecto es 44.1KHz
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=1");
            break;
          }
          }
        } else if ((fileCfg[i].cfgLine).indexOf("zerolevel") != -1) {
          ZEROLEVEL = getValueOfParam(fileCfg[i].cfgLine, "zerolevel").toInt();

          logln("");
          log("Zero level: " + String(ZEROLEVEL));

          if (ZEROLEVEL == 1) {
            hmi.writeString("menuAudio2.lvlLowZero.val=1");
          } else {
            hmi.writeString("menuAudio2.lvlLowZero.val=0");
          }
        }
        // else if ((fileCfg[i].cfgLine).indexOf("blockend") != -1)
        // {
        //   APPLY_END = getValueOfParam(fileCfg[i].cfgLine,
        //   "blockend").toInt();

        //   logln("");
        //   log("Terminator forzed: " + String(APPLY_END));

        //   if (APPLY_END == 1)
        //   {
        //     hmi.writeString("menuAudio2.enTerm.val=1");
        //   }
        //   else
        //   {
        //     hmi.writeString("menuAudio2.enTerm.val=0");
        //   }
        // }
        else if ((fileCfg[i].cfgLine).indexOf("polarized") != -1) {
          if ((getValueOfParam(fileCfg[i].cfgLine, "polarized")) == "1") {
            // Para que empiece en DOWN tenemos que poner POLARIZATION en UP
            // Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en
            // DOWN)
            // POLARIZATION = down;
            EDGE_EAR_IS = POLARIZATION ^ 1;
            INVERSETRAIN = true;

            if (INVERSETRAIN) {
              hmi.writeString("menuAudio2.polValue.val=1");
            } else {
              hmi.writeString("menuAudio2.polValue.val=0");
            }
          } else {
            // POLARIZATION = up;
            // EDGE_EAR_IS = up;
            hmi.writeString("menuAudio2.polValue.val=0");
            EDGE_EAR_IS = POLARIZATION;
            INVERSETRAIN = false;
          }
        } else if ((fileCfg[i].cfgLine).indexOf("azimut") != -1) {
          // Cogemos el azimut
          AZIMUT = getValueOfParam(fileCfg[i].cfgLine, "azimut").toInt();
          // TONE_ADJUST = (-210)*(TONE_ADJUSTMENT_ZX_SPECTRUM +
          // (AZIMUT-TONE_ADJUSTMENT_ZX_SPECTRUM_LIMIT));
          SAMPLES_ADJUST = (TONE_ADJUSTMENT_ZX_SPECTRUM +
                            (AZIMUT - TONE_ADJUSTMENT_ZX_SPECTRUM_LIMIT));
          // Movemos el slide
          myNex.writeNum("menuAudio2.tone.val", AZIMUT);
          myNex.writeNum("menuAudio2.toneL.val", AZIMUT - 5);

          logln("Azimut get: " + String(AZIMUT) +
                " - Frec. adjust: " + String(TONE_ADJUST));
        }
      }

      if (INVERSETRAIN) {
        if (ZEROLEVEL) {
          hmi.writeString("tape.pulseInd.pic=36");
        } else {
          hmi.writeString("tape.pulseInd.pic=33");
        }
      } else {
        if (ZEROLEVEL) {
          hmi.writeString("tape.pulseInd.pic=35");
        } else {
          hmi.writeString("tape.pulseInd.pic=34");
        }
      }
    }

    cfg.close();
  }

  MEDIA_PLAYER_EN = false;

  // free(fileCfg);
}

void loadingFile(char *file_ch) {
  // Cogemos el fichero seleccionado y lo cargamos

  // Si no está vacio
  if (FILE_SELECTED) 
  {

    // Convierto a mayusculas
    PATH_FILE_TO_LOAD.toUpperCase();

    logln("");
    logln("Path file to load: " + PATH_FILE_TO_LOAD);
    logln("");

    // Eliminamos la memoria ocupado por el actual insertado
    // y lo ejectamos
    if (FILE_PREPARED) 
    {
      logln("Now ejecting file - Step 3");
      ejectingFile();
      FILE_PREPARED = false;
    }

    // Cargamos el seleccionado.
    if (PATH_FILE_TO_LOAD.indexOf(".TAP", PATH_FILE_TO_LOAD.length() - 4) != -1) 
    {
      logln("");
      logln("TAP file");
      logln("");

      // Verificamos si hay fichero de configuracion para este archivo
      // seleccionado
      verifyConfigFileForSelection();
      // changeLogo(41);
      // Reservamos memoria
      if (!myTAPmemoryReserved) {
        myTAP.descriptor = (tTAPBlockDescriptor *)ps_calloc(
            MAX_BLOCKS_IN_TAP, sizeof(struct tTAPBlockDescriptor));
        myTAPmemoryReserved = true;
      }

      // Pasamos el control a la clase
      pTAP.setTAP(myTAP);
      // Lo procesamos
      proccesingTAP(file_ch);
      TYPE_FILE_LOAD = "TAP";
      BYTES_TOBE_LOAD = myTAP.size;
    } 
    else if (PATH_FILE_TO_LOAD.indexOf(".PZX", PATH_FILE_TO_LOAD.length() - 4) != -1) 
    {
      pPZX.setPZX(myPZX);
      // Lo procesamos
      proccesingPZX(file_ch);
      TYPE_FILE_LOAD = "PZX";
      BYTES_TOBE_LOAD = myPZX.size;
    } 
    else if ((PATH_FILE_TO_LOAD.indexOf(".TZX", PATH_FILE_TO_LOAD.length() - 4) != -1) ||
               (PATH_FILE_TO_LOAD.indexOf(".TSX", PATH_FILE_TO_LOAD.length() - 4) != -1) ||
               (PATH_FILE_TO_LOAD.indexOf(".CDT", PATH_FILE_TO_LOAD.length() - 4) != -1)) 
               {

      // Verificamos si hay fichero de configuracion para este archivo
      // seleccionado
      verifyConfigFileForSelection(); // Reservamos memoria
      //
      if (!myTZXmemoryReserved) 
      {
        myTZX.descriptor = (tTZXBlockDescriptor *)ps_calloc(MAX_BLOCKS_IN_TZX, sizeof(struct tTZXBlockDescriptor));
        myTZXmemoryReserved = true;
      }

      // Pasamos el control a la clase
      pTZX.setTZX(myTZX);

      // Lo procesamos. Para ZX Spectrum
      proccesingTZX(file_ch);

      if (PATH_FILE_TO_LOAD.indexOf(".TZX", PATH_FILE_TO_LOAD.length() - 4) != -1) {
        TYPE_FILE_LOAD = "TZX";
      } else if (PATH_FILE_TO_LOAD.indexOf(".TSX", PATH_FILE_TO_LOAD.length() - 4) != -1) {
        TYPE_FILE_LOAD = "TSX";
      } else if (PATH_FILE_TO_LOAD.indexOf(".PZX", PATH_FILE_TO_LOAD.length() - 4) != -1) {
        TYPE_FILE_LOAD = "PZX";
      } else {
        TYPE_FILE_LOAD = "CDT";
      }
      BYTES_TOBE_LOAD = myTZX.size;
    } else if (PATH_FILE_TO_LOAD.indexOf(".WAV", PATH_FILE_TO_LOAD.length() - 4) != -1) {
      logln("Wav file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "WAV";
    } else if (PATH_FILE_TO_LOAD.indexOf(".MP3", PATH_FILE_TO_LOAD.length() - 4) != -1) {
      logln("MP3 file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "MP3";
    } else if (PATH_FILE_TO_LOAD.indexOf(".FLAC", PATH_FILE_TO_LOAD.length() - 5) != -1) {
      logln("FLAC file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "FLAC";
    } else if (PATH_FILE_TO_LOAD.indexOf(".RADIO", PATH_FILE_TO_LOAD.length() - 6) != -1) {
      logln("RADIO file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "RADIO";
    } else if (PATH_FILE_TO_LOAD.indexOf(".ZXDB", PATH_FILE_TO_LOAD.length() - 5) != -1) {
      logln("ZXDB file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "ZXDB";
    } else if (PATH_FILE_TO_LOAD.indexOf(".CPCDB", PATH_FILE_TO_LOAD.length() - 6) != -1) {
      logln("CPCDB file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "CPCDB";
    } else if (PATH_FILE_TO_LOAD.indexOf(".MSXDB", PATH_FILE_TO_LOAD.length() - 6) != -1) {
      logln("MSXDB file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "MSXDB";
    }
  } else {
    #ifdef DEBUGMODE
        logAlert("Nothing was prepared.");
    #endif

    if (FILE_PREPARED) {
      logln("Now ejecting file - Step 2");
      ejectingFile();
      FILE_PREPARED = false;
    }

    LAST_MESSAGE = "No file inside the tape";
  }
}

void stopFile() {
  // Paramos la animación
  setSTOP();
  // Paramos la animación
  tapeAnimationOFF();
}

void freeMemoryFromDescriptorPZX(tPZXBlockDescriptor *descriptor) {
  if (descriptor == nullptr)
    return;

  // Iteramos por todos los bloques para liberar la memoria de los punteros
  // internos
  for (int n = 0; n < TOTAL_BLOCKS; n++) {
    if (strcmp(descriptor[n].tag, "PULS") == 0) {
      // PULS usa timming.pzx_pulse_data (asignado en analyzePULS)
      if (descriptor[n].timming.pzx_pulse_data != nullptr) {
        free(descriptor[n].timming.pzx_pulse_data);
        descriptor[n].timming.pzx_pulse_data = nullptr;
      }
    } else if (strcmp(descriptor[n].tag, "DATA") == 0) {
      if (descriptor[n].data_s0_pulses != nullptr) {
        free(descriptor[n].data_s0_pulses);
        descriptor[n].data_s0_pulses = nullptr;
      }
      if (descriptor[n].data_s1_pulses != nullptr) {
        free(descriptor[n].data_s1_pulses);
        descriptor[n].data_s1_pulses = nullptr;
      }
    } else if (strcmp(descriptor[n].tag, "CSW") == 0) {
      if (descriptor[n].csw_pulse_data != nullptr) {
        free(descriptor[n].csw_pulse_data);
        descriptor[n].csw_pulse_data = nullptr;
      }
    }
  }
}

void ejectingFile() {
  logln("Eject executing for " + TYPE_FILE_LOAD);
  // Terminamos los players
  if (TYPE_FILE_LOAD == "TAP") {
    // Solicitamos el puntero _myTAP de la clase
    // para liberarlo
    logln("Eject TAP");
    if (myTAPmemoryReserved) {
      LAST_MESSAGE = "Preparing structure";
      free(pTAP.getDescriptor());
      // Finalizamos
      pTAP.terminate();
      myTAPmemoryReserved = false;
    }
  } else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" ||
             TYPE_FILE_LOAD == "TSX") {
    // Solicitamos el puntero _myTZX de la clase
    // para liberarlo
    logln("Eject TZX");
    if (myTZXmemoryReserved) {
      LAST_MESSAGE = "Preparing structure";
      freeMemoryFromDescriptorTZX(pTZX.getDescriptor());
      free(pTZX.getDescriptor());
      // Finalizamos
      pTZX.terminate();
      myTZXmemoryReserved = false;
    }
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Solicitamos el puntero _myPZX de la clase para liberarlo
    logln("Eject PZX");
    if (myPZX.descriptor != nullptr) {
      LAST_MESSAGE = "Preparing structure";
      // Primero liberamos la memoria interna de cada bloque
      freeMemoryFromDescriptorPZX(myPZX.descriptor);
      // Luego liberamos el array de descriptores principal
      free(myPZX.descriptor);
      myPZX.descriptor = nullptr;
      // Finalizamos
      // pPZX.terminate();
    }
  } else if (TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3" ||
             TYPE_FILE_LOAD == "FLAC" || TYPE_FILE_LOAD == "RADIO") {
    logln("Eject WAV / MP3 / FLAC / RADIO");
    // No hay nada que liberar
  } else {
    logln("Eject WAV / MP3 not needed");
  }

  LAST_MESSAGE = "No file inside the tape";
}

void prepareRecording() {
  hmi.activateWifi(false);

  // Inicializamos audio input
  // if (REC_AUDIO_LOOP)
  // {
  //   setAudioInOut();
  // }
  // else
  // {
  //   setAudioInput();
  // }

  // taprec.set_kit(ESP32kit);
  taprec.initialize();

  // writeString("");
  hmi.writeString("currentBlock.val=1");
  // writeString("");
  hmi.writeString("progressTotal.val=0");
  // writeString("");
  hmi.writeString("progression.val=0");
  // Preparamos para recording

  // Activamos la animación
  tapeAnimationON();
}

void RECready() { prepareRecording(); }

void getAudioSettingFromHMI() {

  if (myNex.readNumber("menuAdio.polValue.val") == 1) {
    // Para que empiece en DOWN tenemos que poner POLARIZATION en UP
    // Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en DOWN)
    INVERSETRAIN = true;
  } else {
    INVERSETRAIN = false;
  }

  if (myNex.readNumber("menuAdio.lvlLowZero.val") == 1) {
    ZEROLEVEL = true;
  } else {
    ZEROLEVEL = false;
  }
}

void setPolarization() {
  // Inicializa la polarizacion
  EDGE_EAR_IS = POLARIZATION;
}

void getPlayeableBlockTZX(tTZX tB, int inc) {
  // Esta funcion busca el bloque valido siguiente
  // inc sera +1 o -1 dependiendo de si es FFWD o RWD
  while (!((tB.descriptor)[BLOCK_SELECTED].playeable)) {
    BLOCK_SELECTED += inc;

    if (BLOCK_SELECTED > TOTAL_BLOCKS) {
      BLOCK_SELECTED = 1;
    }

    if (BLOCK_SELECTED < 1) {
      BLOCK_SELECTED = TOTAL_BLOCKS;
    }
  }
}

void nextGroupBlock() {
  logln("Seacrh Next Group Block");

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" &&
      TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC" &&
      TYPE_FILE_LOAD != "RADIO") {
    while (myTZX.descriptor[BLOCK_SELECTED].ID != 34 &&
           BLOCK_SELECTED <= TOTAL_BLOCKS) {
      BLOCK_SELECTED++;
    }

    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1)) {
      // No he encontrado el Group End
      BLOCK_SELECTED = 1;
    } else {
      // Avanzo uno mas
      PROGRAM_NAME_2 = "";
      LAST_BLOCK_WAS_GROUP_END = true;
      LAST_BLOCK_WAS_GROUP_START = false;
      // BLOCK_SELECTED++;
      // isGroupStart();
    }
  }
}

void prevGroupBlock() {
  logln("Seacrh Previous Group Block");

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" &&
      TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC" &&
      TYPE_FILE_LOAD != "RADIO") {
    while (myTZX.descriptor[BLOCK_SELECTED].ID != 33 && BLOCK_SELECTED > 1) {
      BLOCK_SELECTED--;
    }

    if (BLOCK_SELECTED < 1) {
      // No he encontrado el Group Start
      BLOCK_SELECTED = TOTAL_BLOCKS - 1;
    } else {
      // Le pasamos el nombre del grupo al PROGRAM_NAME_2
      // BLOCK_SELECTED++;
      LAST_GROUP = myTZX.descriptor[BLOCK_SELECTED].name;
      LAST_BLOCK_WAS_GROUP_START = true;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void isGroupStart() {
  // Verificamos si se entra en un grupo
  logln("ID: " + String(myTZX.descriptor[BLOCK_SELECTED].ID));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" &&
      TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC" &&
      TYPE_FILE_LOAD != "RADIO") {
    if (myTZX.descriptor[BLOCK_SELECTED].ID == 33) {
      // Es un group start
      LAST_BLOCK_WAS_GROUP_START = true;
      LAST_BLOCK_WAS_GROUP_END = false;
      // Le pasamos el nombre del grupo al PROGRAM_NAME_2
      LAST_GROUP = myTZX.descriptor[BLOCK_SELECTED].name;
    } else {
      LAST_BLOCK_WAS_GROUP_START = false;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void isGroupEnd() {
  // Verificamos si se entra en un grupo
  logln("ID: " + String(myTZX.descriptor[BLOCK_SELECTED].ID));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" &&
      TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC" &&
      TYPE_FILE_LOAD != "RADIO") {
    if (myTZX.descriptor[BLOCK_SELECTED].ID == 34) {
      // Es un group start
      LAST_BLOCK_WAS_GROUP_END = true;
      LAST_BLOCK_WAS_GROUP_START = false;
      prevGroupBlock();
    } else {
      LAST_BLOCK_WAS_GROUP_START = false;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void putLogo() {
  if (TYPE_FILE_LOAD == "MP3") {
    // MP3 file
    hmi.writeString("tape.logo.pic=45");
    delay(5);
  } else if (TYPE_FILE_LOAD == "FLAC") {
    // FLAC
    hmi.writeString("tape.logo.pic=49");
    delay(5);
  } else if (TYPE_FILE_LOAD == "RADIO") {
    // iRADIO
    hmi.writeString("tape.logo.pic=50");
    delay(5);
  } else if (TYPE_FILE_LOAD == "WAV") {
    // WAV file
    hmi.writeString("tape.logo.pic=46");
    delay(5);
  } else if (TYPE_FILE_LOAD == "TAP") {
    // Spectrum
    hmi.writeString("tape.logo.pic=41");
    delay(5);
  } else if (TYPE_FILE_LOAD == "TZX") {
    // TZX file
    hmi.writeString("tape.logo.pic=47");
    delay(5);
  } else if (TYPE_FILE_LOAD == "PZX") {
    // PZX file
    hmi.writeString("tape.logo.pic=53");
    delay(5);
  } else if (TYPE_FILE_LOAD == "CDT") {
    // Amstrad
    hmi.writeString("tape.logo.pic=39");
    delay(5);
  } else if (TYPE_FILE_LOAD == "TSX") {
    // MSX
    hmi.writeString("tape.logo.pic=40");
    delay(5);
  } else {
    // En blanco
    hmi.writeString("tape.logo.pic=42");
    delay(5);
  }
  hmi.writeString("doevents");
}

void updateHMIOnBlockChange() {
  if (TYPE_FILE_LOAD == "TAP") {
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                myTAP.descriptor[BLOCK_SELECTED].typeName,
                                myTAP.descriptor[BLOCK_SELECTED].size, true);
  } else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" ||
             TYPE_FILE_LOAD == "TSX") {
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
  } else if (TYPE_FILE_LOAD == "PZX") {
    // hmi.setBasicFileInformation(myPZX.descriptor[BLOCK_SELECTED].ID,
    // myPZX.descriptor[BLOCK_SELECTED].group,
    // myPZX.descriptor[BLOCK_SELECTED].name,
    // myPZX.descriptor[BLOCK_SELECTED].typeName,
    // myPZX.descriptor[BLOCK_SELECTED].size,
    // myPZX.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);
}

void getRandomFilenameWAV(char *&currentPath, String currentFileBaseName) {
  currentPath = strcpy(currentPath, currentFileBaseName.c_str());
  srand(time(0));
  delay(125);
  int rn = rand() % 999999;
  // Le unimos la extensión .TAP
  String txtRn = "-" + String(rn) + ".wav";
  char const *extPath = txtRn.c_str();
  strcat(currentPath, extPath);
}

void rewindAnimation(int direction) {
  int p = 0;
  int frames = 19;
  int fdelay = 5;

  while (p < frames) {

    POS_ROTATE_CASSETTE += direction;

    if (POS_ROTATE_CASSETTE > 23) {
      POS_ROTATE_CASSETTE = 4;
    }

    if (POS_ROTATE_CASSETTE < 4) {
      POS_ROTATE_CASSETTE = 23;
    }

    hmi.writeString("tape.animation.pic=" + String(POS_ROTATE_CASSETTE));
    delay(20);

    p++;
  }
}

void getTheFirstPlayeableBlock() {
  // Buscamos ahora el primer bloque playeable

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    int i = 0;

    while (!myTZX.descriptor[i].playeable) {
      BLOCK_SELECTED = i;
      i++;
    }

    if (i > MAX_BLOCKS_IN_TZX) {
      i = 0;
    }

    BLOCK_SELECTED = i;
    logln("Primero playeable: " + String(i));

    // PROGRAM_NAME = myTZX.descriptor[i].name;
    strcpy(LAST_TYPE, myTZX.descriptor[i].typeName);
    LAST_SIZE = myTZX.descriptor[i].size;

    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
    hmi.updateInformationMainPage(true);
  }
}

void setFWIND() {
  logln("Set FFWD - " + String(LAST_BLOCK_WAS_GROUP_START));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    if (LAST_BLOCK_WAS_GROUP_START) {
      // Si el ultimo bloque fue un GroupStart entonces busco un Group End y
      // avanzo 1
      nextGroupBlock();
      // LAST_BLOCK_WAS_GROUP_START = false;
    } else {
      // Modo normal avanzo 1 a 1 y verifico si hay Group Start
      BLOCK_SELECTED++;
      isGroupStart();
    }
  } else {
    BLOCK_SELECTED++;
  }

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    // Para las estructuras TZX, CDT y TSX
    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1)) {
      BLOCK_SELECTED = 1;
      isGroupStart();
    }
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Para el fichero PZX
    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1)) {
      BLOCK_SELECTED = 1;
    }
  } else if (TYPE_FILE_LOAD == "TAP") {
    // Para el fichero TAP
    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 2)) {
      BLOCK_SELECTED = 0;
    }
  }

  logln("TOTAL_BLOCKS: " + String(TOTAL_BLOCKS));
  logln("BLOCK_SELECTED: " + String(BLOCK_SELECTED));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myPZX.descriptor[BLOCK_SELECTED].name,
                                myPZX.descriptor[BLOCK_SELECTED].typeName,
                                myPZX.descriptor[BLOCK_SELECTED].size,
                                myPZX.descriptor[BLOCK_SELECTED].playeable);
  } else if (TYPE_FILE_LOAD == "TAP") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                myTAP.descriptor[BLOCK_SELECTED].typeName,
                                myTAP.descriptor[BLOCK_SELECTED].size,
                                myTAP.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);

  rewindAnimation(1);
}

void setRWIND() {

  logln("Set RWD - " + String(LAST_BLOCK_WAS_GROUP_END));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    if (LAST_BLOCK_WAS_GROUP_END) {
      // Si el ultimo bloque fue un Group End entonces busco un Group Start y
      // avanzo 1
      prevGroupBlock();
      // LAST_BLOCK_WAS_GROUP_END = false;
    } else {
      BLOCK_SELECTED--;
      isGroupEnd();
    }
  } else {
    BLOCK_SELECTED--;
  }

  // Para las estructuras TZX, CDT y TSX
  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    if (BLOCK_SELECTED < 1) {
      BLOCK_SELECTED = (TOTAL_BLOCKS - 1);
      isGroupEnd();
    }
  } else if (TYPE_FILE_LOAD == "TAP") {
    // Para el fichero TAP
    if (BLOCK_SELECTED < 0) {
      BLOCK_SELECTED = (TOTAL_BLOCKS - 2);
    }
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Para el fichero PZX
    if (BLOCK_SELECTED < 1) {
      BLOCK_SELECTED = (TOTAL_BLOCKS - 1);
    }
  }

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "PZX") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myPZX.descriptor[BLOCK_SELECTED].name,
                                myPZX.descriptor[BLOCK_SELECTED].typeName,
                                myPZX.descriptor[BLOCK_SELECTED].size,
                                myPZX.descriptor[BLOCK_SELECTED].playeable);
  } else if (TYPE_FILE_LOAD == "TAP") {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                myTAP.descriptor[BLOCK_SELECTED].typeName,
                                myTAP.descriptor[BLOCK_SELECTED].size,
                                myTAP.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);

  rewindAnimation(-1);
}

// Modificado 17/10/2025
void recoverEdgeBeginOfBlock() {
  if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" ||
      TYPE_FILE_LOAD == "TSX") {
    // Recuperamos el flanco con el que empezo el bloque
    EDGE_EAR_IS = myTZX.descriptor[BLOCK_SELECTED].edge;
  } else if (TYPE_FILE_LOAD == "PZX") {
    // Recuperamos el flanco con el que empezo el bloque
    EDGE_EAR_IS = myPZX.descriptor[BLOCK_SELECTED].edge;
  } else {
    // Empezamos con el tipo de polarizacion ya que este filetype no es sensible
    // a esta Inicializamos el nivel de la señal según la polarización
    // seleccionada
    EDGE_EAR_IS = INVERSETRAIN ? POLARIZATION ^ 1 : POLARIZATION;
  }
}

void recCondition() {
  recAnimationON();

  if (MODEWAV) {
    LAST_MESSAGE = "Press PAUSE to start WAV recording.";
    TAPESTATE = 120;
  } else {
    LAST_MESSAGE = "Press PAUSE to start TAP recording.";
    TAPESTATE = 220;
  }

  if (OUT_TO_WAV) {
    wavfile.flush();
    wavfile.close();
    encoderOutWAV.end();
  }

  //
  AUTO_STOP = false;
}

void tapeControl() {
  // Estados de funcionamiento del TAPE
  //
  // LOADING_STATE:
  // 0 - INICIAL - APAGADO
  // 1 - PLAY
  // 2 - STOP
  // 3 - PAUSE
  // 4 - REC
  //
  // Nuevo tapeControl

  if (UPDATE_HMI) {
    updateHMIOnBlockChange();
    UPDATE_HMI = false;
  }

  if (LAST_TAPESTATE != TAPESTATE) {
    logln("CASE " + String(TAPESTATE));
    LAST_TAPESTATE = TAPESTATE;
  }

  // Gestor de estados del TAPE
  //
  // NOTA: REM function: Este sistema funciona si automáticamente el ordenador
  // inicia la reproducción con REM
  //       en otro caso se considera acción del usuario y no hace efecto, hasta
  //       pulsar PAUSE o STOP
  //

  switch (TAPESTATE) {
  // Modo reposo. Aqui se está cuando:
  // - Tape: STOP
  // Posibles acciones
  // - Se pulsa EJECT - Nuevo fichero
  // - Se pulsa REC
  //
  case 0: {

    LOADING_STATE = 0;

    #ifdef DEBUGMODE
        logAlert("Tape state 0");
    #endif

        if (EJECT && !STOP_OR_PAUSE_REQUEST) {
    #ifdef DEBUGMODE
          logAlert("EJECT state in TAPESTATE = " + String(TAPESTATE));
    #endif

      // Inicializamos la polarizacion
      EDGE_EAR_IS = INVERSETRAIN ? POLARIZATION ^ 1 : POLARIZATION;

      // Limpiamos los campos del TAPE
      // porque hemos expulsado la cinta.
      // hmi.clearInformationFile();

      TAPESTATE = 99;
      // Expulsamos la cinta
      // FILE_PREPARED = false; // Esto lo comentamos para hacerlo cuando haga
      // el ejecting()
      // para poder descargar el anterior de la PSRAM
      FILE_SELECTED = false;
      //
      AUTO_STOP = false;
      BB_PTR_ITEM = 0;
      BB_PAGE_SELECTED = 0;
      //
      setPolarization();
    }
    else if (SAMPLINGTEST) 
    {
      logln("Testing is output at 96KHz");
      zxp.samplingtest(96000);

      SAMPLINGTEST = false;
    } 
    else if (REC) 
    {
      recCondition();
    }
    else if (DISABLE_SD) 
    {
      // ------------------
      // Disable SD
      // ------------------
    } 
    else if (PLAY) 
    {
      //
      // Reproducimos el ultimo fichero cargado
      //
      logln("State: Play was pressed. Loading last file.");
      // Cargamos el ultimo fichero reproducido en el cassette
      FILE_BROWSER_OPEN = false;
      UPDATE_FROM_REMOTE_CONTROL = false;
      ABORT = false;
      EJECT = false;
      PLAY = false;
      REC = false;
      // Esto se usa para reproducir
      FILE_SELECTED = true;
      FILE_PREPARED = false;
      LOADING_STATE = 0;
      TAPESTATE = 100;

      // Cargamos el medio
      PATH_FILE_TO_LOAD = hmi.loadLastMedia();
      FILE_LOAD = getFileNameFromPath(PATH_FILE_TO_LOAD);
      FILE_LAST_DIR = getDirFromPath(PATH_FILE_TO_LOAD);

      // Mostramos la página de carga
      hmi.writeString("page tape");          
      delay(125);
    } 
    else 
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      PLAY = false;
    }
  } break;

  // PLAY / STOP
  case 1: {
    remDetection();
    // Esto nos permite abrir el Block Browser en reproduccion
    if (BB_OPEN || BB_UPDATE) {
      // Ojo! no meter delay! que esta en una reproduccion
      logln("Solicito abrir el BLOCKBROWSER");

      hmi.openBlocksBrowser(myTZX, myTAP, myPZX);
      // openBlocksBrowser();

      BB_UPDATE = false;
      BB_OPEN = false;
    }

    // Estados en reproduccion
    if (PLAY || REM_DETECTED) {

      if (REM_DETECTED) {
        PLAY = true;
        PAUSE = false;
        STOP = false;
        REC = false;
        EJECT = false;
        ABORT = false;
        BTN_PLAY_PRESSED = true;
        STATUS_REM_ACTUATED = true;
      }

      // Inicializamos la polarización de la señal al iniciar la reproducción.
      //
      LOADING_STATE = 1;
      // Activamos la animación
      tapeAnimationON();
      // Reproducimos el fichero
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
          TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") {
        LAST_MESSAGE = "Loading in progress. Please wait.";
      } else {
        LAST_MESSAGE = "Playing audio file.";
      }
      //
      playingFile();
      logln("End of CASE 1");
    } else if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      TAPESTATE = 0;
      LOADING_STATE = 0;

      if (OUT_TO_WAV) {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
      }
    } else if (PAUSE) {
      TAPESTATE = 3;
      // Esto la hacemos para evitar una salida inesperada de seleccion de
      // bloques.
      PAUSE = false;
      //
      if (AUTO_PAUSE) {
        logln("Auto-pause activated");
        LAST_MESSAGE = "Tape auto-paused. Follow machine instructions.";
        ABORT = false;
        STOP = false;
      } else {
        if (LAST_BLOCK_WAS_GROUP_START) {
          // Localizamos el GROUP START
          prevGroupBlock();
        }
      }

      if (OUT_TO_WAV) {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
      }
    } else if (STOP) {
      // Desactivamos la animación
      // esta condicion evita solicitar la parada de manera infinita
      logln("");
      log("Tape state 1");

      // Volvemos al estado de fichero preparado
      TAPESTATE = 10;

      if (LOADING_STATE == 1) {
        tapeAnimationOFF();
        LOADING_STATE = 2;
      }

      LOADING_STATE = 2;
      // Al parar el c.block del HMI se pone a 1.
      BLOCK_SELECTED = 0; // 29/11

      // Reproducimos el fichero
      if (!AUTO_STOP) {
        LAST_MESSAGE = "Stop playing.";
        STOP_OR_PAUSE_REQUEST = false;
      } else {
        LAST_MESSAGE = "Auto-Stop playing.";
        AUTO_STOP = false;
        PLAY = false;
        PAUSE = false;
        REC = false;
        EJECT = false;
        STOP_OR_PAUSE_REQUEST = false;
      }

      setPolarization();
      STOP = false; // 28/11

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
          TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") {
        getTheFirstPlayeableBlock();
      }

      if (OUT_TO_WAV) {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
      }
    } else if (REC) {
      // Expulsamos la cinta
      if (LOADING_STATE == 0 || LOADING_STATE == 2) {
        if (FILE_PREPARED) {
          logln("Now ejecting file - Step 4");
          ejectingFile();
        }

        FILE_PREPARED = false;
        FILE_SELECTED = false;
        hmi.clearInformationFile();
        tapeAnimationOFF();
        recAnimationON();

        if (MODEWAV) {
          LAST_MESSAGE = "Press PAUSE to start WAV recording.";
          TAPESTATE = 120;
        } else {
          LAST_MESSAGE = "Press PAUSE to start TAP recording.";
          TAPESTATE = 220;
        }
      }

      if (OUT_TO_WAV) {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
      }
    } else {
      TAPESTATE = 1;
    }
  } break;

  // STOP
  case 2: {
#ifdef DEBUGMODE
    logAlert("Tape state 2");
#endif

    TAPESTATE = 1;
    //
  } break;

  // PAUSE
  case 3: {
//
// PAUSE
//
#ifdef DEBUGMODE
    logAlert("Tape state 3");
#endif

    LOADING_STATE = 3;
    // Activamos la animación
    tapeAnimationOFF();

    // Reproducimos el fichero
    LAST_MESSAGE = "Tape paused. Press play or select block.";

    remDetection();
    //
    if (PLAY || REM_DETECTED) 
    {
      if (REM_DETECTED) 
      {
        PLAY = true;
        PAUSE = false;
        STOP = false;
        REC = false;
        EJECT = false;
        ABORT = false;

        BTN_PLAY_PRESSED = true;
      }
      // Reanudamos la reproduccion
      TAPESTATE = 1;
      AUTO_PAUSE = false;

      recoverEdgeBeginOfBlock();
    } else if (PAUSE) {
      // Reanudamos la reproduccion pero con PAUSE
      TAPESTATE = 5;
      AUTO_PAUSE = false;

      HMI_FNAME = FILE_LOAD;

      recoverEdgeBeginOfBlock();
    } else if (STOP) {
      TAPESTATE = 1;
      AUTO_PAUSE = false;

      HMI_FNAME = FILE_LOAD;

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
          TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") {
        getTheFirstPlayeableBlock();
      }
      //
      setPolarization();
    } else if (FFWIND || RWIND) {

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
          TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") {
        logln("Cambio de bloque");
        // Actuamos sobre el cassette
        if (FFWIND) {
          setFWIND();
          FFWIND = false;
        } else {
          setRWIND();
          RWIND = false;
        }

        // Actualizamos el HMI
        updateHMIOnBlockChange();
      }
    } else if (BB_OPEN || BB_UPDATE) {
      hmi.openBlocksBrowser(myTZX, myTAP, myPZX);
      BB_UPDATE = false;
      BB_OPEN = false;
    } else if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      AUTO_PAUSE = false;
    } else {
      TAPESTATE = 3;
    }
  } break;

  case 5: { // Si venimos del estado de seleccion de bloques
    // y pulso PAUSE continuo la carga en TAPESTATE = 1
    TAPESTATE = 1;
    PLAY = true;
    PAUSE = false;
  } break;

  case 10: { //
    // File prepared. Esperando a PLAY / FFWD o RWD
    //
    remDetection();

    if (PLAY || REM_DETECTED) {
      if (REM_DETECTED)
        PLAY = true;

      if (FILE_PREPARED) {
        // Ponemos esto aqui porque prepareOutputToWav() puede cambiar el
        // flujo de TAPESTATE
        TAPESTATE = 1;
        LOADING_STATE = 1;

        if (OUT_TO_WAV) {
          prepareOutputToWav();
        }
      }
    } else if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      TAPESTATE = 0;
      LOADING_STATE = 0;
    } else if (REC) {
      if (FILE_PREPARED) {
        logln("Now ejecting file - Step 5");
        ejectingFile();
      }

      FILE_PREPARED = false;
      FILE_SELECTED = false;
      hmi.clearInformationFile();
      TAPESTATE = 0;
    } else if (FFWIND || RWIND) {
      // Actuamos sobre el cassette
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
          TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") {
        logln("Cambio de bloque - CASE 10");
        // Actuamos sobre el cassette
        if (FFWIND) {
          setFWIND();
          FFWIND = false;
        } else {
          setRWIND();
          RWIND = false;
        }

        // Actualizamos el HMI
        updateHMIOnBlockChange();
      }
    } else if (BB_OPEN || BB_UPDATE) {
      hmi.openBlocksBrowser(myTZX, myTAP, myPZX);
      BB_UPDATE = false;
      BB_OPEN = false;
    } else {
      TAPESTATE = 10;
      LOADING_STATE = 0;
    }
  } break;

  case 99: {
//
// Eject
//
#ifdef DEBUGMODE
    logAlert("Tape state 99");
#endif

    if (FILE_BROWSER_OPEN) {
      // Abrimos el filebrowser
      #ifdef DEBUGMODE
            logAlert("State: File browser keep open.");
      #endif

      TAPESTATE = 100;
      LOADING_STATE = 0;

      // Limpiamos el nombre del fichero a cargar
      FILE_LOAD = "";
      hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");
      //

      EJECT = false;
    } else {
      TAPESTATE = 99;
      LOADING_STATE = 0;     
    }
  } break;

  case 100: 
  {
    //
    // Filebrowser open
    //
    #ifdef DEBUGMODE
        logAlert("Tape state 100");
    #endif

    if (!FILE_BROWSER_OPEN) 
    {
      #ifdef DEBUGMODE
            logln("State: File browser was closed.");
            logln("Vble. FILE_SELECTED = " + String(FILE_SELECTED));
      #endif
      //
      // Cerramos el filebrowser.
      // y entramos en el estado FILE_PREPARED (inside the tape)
      //
      EJECT = false;

      if (UPDATE_FROM_REMOTE_CONTROL)
      {
        logln("Loading file from REMOTE CONTROL: " + FILE_LOAD);
        // Si venimos del control remoto, forzamos la selección del fichero
        UPDATE_FROM_REMOTE_CONTROL = false;
        // Para poner mas logos actualizar el HMI
        putLogo();
        // PROGRAM_NAME = FILE_LOAD;
        hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");

        HMI_FNAME = FILE_LOAD;

        TAPESTATE = 10;
        LOADING_STATE = 0;

        // Damos tiempo a que la pagina del remote recargue y no interfiera
        LAST_MESSAGE = ".. receiving from remote ..";
        delay(1000);
        //
        playingFile();
      } 
      else 
      {
        if (FILE_SELECTED) 
        {
          // Si se ha seleccionado lo cargo en el cassette.
          char file_ch[257];
          PATH_FILE_TO_LOAD.toCharArray(file_ch, 256);
          loadingFile(file_ch);
          
          REQ_LAST_MEDIA = false;

          // Ponemos FILE_SELECTED = false, para el proximo fichero
          FILE_SELECTED = false;

          // Ahora miro si está preparado
          if (FILE_PREPARED) 
          {
            #ifdef DEBUGMODE
                        logAlert("File inside the tape.");
            #endif

            // Avanzamos ahora hasta el primer bloque playeable
            if (!ABORT) 
            {
              logln("Type file load: " + TYPE_FILE_LOAD);

              // Para poner mas logos actualizar el HMI
              putLogo();

              if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" &&
                  TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO") 
                  {
                getTheFirstPlayeableBlock();

                LAST_MESSAGE = "File inside the TAPE.";
                PROGRAM_NAME = FILE_LOAD;
                HMI_FNAME = FILE_LOAD;

                TAPESTATE = 10;
                LOADING_STATE = 0;
              } else {
                // PROGRAM_NAME = FILE_LOAD;
                hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");

                HMI_FNAME = FILE_LOAD;

                TAPESTATE = 10;
                LOADING_STATE = 0;

                // Esto lo hacemos para llevar el control desde el WAV player
                logln("Playing file CASE 100:");
                playingFile();
                logln("End of CASE 100 - PLAY from REC");
              }
            } else {
              // Abortamos proceso de analisis
              LAST_MESSAGE = "No file inside the tape or loading aborted.";
              TAPESTATE = 0;
              LOADING_STATE = 0;
            }
          } else {

            #ifdef DEBUGMODE
                        logAlert("No file selected or empty file.");
            #endif

            LAST_MESSAGE = "No file inside the tape";
          }
        } 
        else 
        {
          // Volvemos al estado inicial.
          TAPESTATE = 0;
          LOADING_STATE = 0;

          // LAST_MESSAGE = "TYPE LOAD: " + TYPE_FILE_LOAD;
          // delay(2000);

          if (FILE_PREPARED) 
          {
            logln("Now ejecting file - Step 1");
            ejectingFile();
            FILE_PREPARED = false;
          }

          if (TYPE_FILE_LOAD != "ZXDB" && TYPE_FILE_LOAD != "CPCDB" && TYPE_FILE_LOAD != "MSXDB")
          {
            LAST_MESSAGE = "No file inside the tape";
          }
          
        }
      }

    } else {
      TAPESTATE = 100;
      LOADING_STATE = 0;
    }
  } break;

  case 120: {
    // Start WAV REC after PAUSE pressed
    if (PAUSE) {
      // Preparmos y grabamos
      prepareOutputToWav();
      WavRecording();

      // Definimos el estado de parada
      REC = false;
      PAUSE = false;
      PLAY = false;
      EJECT = false;
      //
      STOP = false;
      //
      recAnimationOFF();
      recAnimationFIXED_OFF();
      //
      TAPESTATE = 130;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
    } else if (STOP) {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
      REC = false;
      recAnimationOFF();
      recAnimationFIXED_OFF();
      LAST_MESSAGE = "Recording canceled.";
    } else {
      TAPESTATE = 120;
    }
  } break;

  case 130: {
    // WAV REC finished / Waiting for PLAY or others
    if (PLAY) {
      if (REC_FILENAME != "") {
        // logln("REC File on tape - CASE 130: " + String(REC_FILENAME));

        // char file_ch[257];
        // REC_FILENAME.toCharArray(file_ch, 256);
        // loadingFile(file_ch);

        // FILE_SELECTED = false;
        TYPE_FILE_LOAD = "WAV";
        //
        logln("REC File load: " + String(FILE_LOAD));
        //
        PROGRAM_NAME = FILE_LOAD;
        hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");
        //
        LAST_MESSAGE = "File inside the TAPE.";
        HMI_FNAME = FILE_LOAD;

        TAPESTATE = 10;
        LOADING_STATE = 0;
        // Mostramos el logo del fichero
        putLogo();
        // Pasamos la ruta del fichero al mediaPlayer
        FILE_LAST_DIR = "/WAV";
        // y el fichero seleccionado
        PATH_FILE_TO_LOAD = FILE_LAST_DIR + "/" + FILE_LOAD;

        hmi.reloadDir();
        //
        LAST_MESSAGE = "File preparing to play.";
        // Eliminamos la ruta del fichero y nos quedamos con el nombre y la
        // extensión
        FILE_LOAD = getFileNameFromPath(FILE_LOAD);
        // Esto lo hacemos para llevar el control desde el WAV player
        playingFile();
        logln("End of CASE 130 - PLAY from REC");
      }
    } else if (REC) {
      recCondition();
    } else if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      //
      recAnimationOFF();
      recAnimationFIXED_OFF();
      //
      REC = false;
      // Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
    } else {
      TAPESTATE = 130;
    }
  } break;

  case 220: {
    // Start TAP REC after PAUSE pressed
    if (PAUSE) {
      // Pulsacion de PAUSE
      // Preparamos la grabacion
      recAnimationOFF();
      prepareRecording();
      recAnimationFIXED_ON();

      // Inicializamos
      PLAY = false;
      PAUSE = false;
      STOP = false;
      REC = true;
      ABORT = false;
      EJECT = false;
      taprec.actuateAutoRECStop = false;
      taprec.stopRecordingProccess = false;
      RECORDING_ERROR = 0;

      // Saltamos a otro estado
      TAPESTATE = 200;
    } else if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      //
      stopRecording();
      recAnimationFIXED_OFF();
      //
      taprec.stopRecordingProccess = false;
      taprec.actuateAutoRECStop = false;
      REC = false;
      // Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
    } else if (STOP) {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
      REC = false;
      STOP = false;
      recAnimationOFF();
      recAnimationFIXED_OFF();
      LAST_MESSAGE = "Recording canceled.";
    } else {
      LOADING_STATE = 0;
      TAPESTATE = 220;
    }
  } break;

  case 200: {
    //
    // remDetection();
    // recording
    if (REC) // || REM_DETECTED)
    {
      while (!taprec.recording()) // || !REM_DETECTED)
      {
        // remDetection();
        delay(1);
      }

      // RECORDING_IN_PROGRESS = false;
      // logln("REC mode - Recording in progress " +
      // String(RECORDING_IN_PROGRESS)); Ha acabado con errores
      LAST_MESSAGE = "Stop recording";
      stopRecording();
      recAnimationFIXED_OFF();
      //
      taprec.stopRecordingProccess = false;
      taprec.actuateAutoRECStop = false;
      REC = false;
      STOP = false;
      EJECT = false;
      PLAY = false;
      PAUSE = false;
      LOADING_STATE = 4;
      TAPESTATE = 230;
    }
  } break;

  case 230: {
    //
    // After recording. Fast PLAY
    //
    if (EJECT && !STOP_OR_PAUSE_REQUEST) {
      //
      stopRecording();
      recAnimationFIXED_OFF();
      //
      taprec.stopRecordingProccess = false;
      taprec.actuateAutoRECStop = false;
      REC = false;
      // Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
    } else if (REC) {
      recCondition();
    } else if (PLAY) {
      if (REC_FILENAME != "") {
        char fileRecPath[100];
        strcpy(fileRecPath, REC_FILENAME.c_str());
        PATH_FILE_TO_LOAD = REC_FILENAME;

        logln("");
        logln("REC File on tape: CASE 230 - " + REC_FILENAME);
        logln("");

        FILE_SELECTED = true;
        FILE_PREPARED = false;
        PLAY = false;
        // Escaneamos por los nuevos ficheros
        // LAST_MESSAGE = "Rescaning files";
        hmi.reloadDir();
        LAST_MESSAGE = "File preparing to play.";
        loadingFile(fileRecPath);
        TYPE_FILE_LOAD = "TAP";
        putLogo();
        getTheFirstPlayeableBlock();
        FILE_SELECTED = false;
        PLAY = true;
        TAPESTATE = 10;
        LOADING_STATE = 0;
      }
    } else {
      LOADING_STATE = 0;
      TAPESTATE = 230;
    }
  } break;

  default: {
#ifdef DEBUGMODE
    logAlert("Tape state unknow.");
#endif
  } break;
  }

  // Actualizamos el HMI - indicadores
  if (UPDATE) {
    if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" &&
        TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC" &&
        TYPE_FILE_LOAD != "RADIO" && TYPE_FILE_LOAD != "PZX") {
      // Forzamos un refresco de los indicadores para TZX, CDT y TSX
      hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                  myTZX.descriptor[BLOCK_SELECTED].group,
                                  myTZX.descriptor[BLOCK_SELECTED].name,
                                  myTZX.descriptor[BLOCK_SELECTED].typeName,
                                  myTZX.descriptor[BLOCK_SELECTED].size,
                                  myTZX.descriptor[BLOCK_SELECTED].playeable);
    } else if (TYPE_FILE_LOAD == "PZX") {
      // Forzamos un refresco de los indicadores para PZX
      hmi.setBasicFileInformation(0, 0, myPZX.descriptor[BLOCK_SELECTED].name,
                                  myPZX.descriptor[BLOCK_SELECTED].typeName,
                                  myPZX.descriptor[BLOCK_SELECTED].size,
                                  myPZX.descriptor[BLOCK_SELECTED].playeable);
    } else if (TYPE_FILE_LOAD == "TAP") {
      // Forzamos un refresco de los indicadores para TAP
      hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                  myTAP.descriptor[BLOCK_SELECTED].typeName,
                                  myTAP.descriptor[BLOCK_SELECTED].size, true);
    }

    hmi.updateInformationMainPage(true);
    UPDATE = false;
  }

  if (START_FFWD_ANIMATION) {
    rewindAnimation(1);
    START_FFWD_ANIMATION = false;
  }
}

void jsUpdateTextBox(WiFiClient client) {
  // ✅ JAVASCRIPT OPTIMIZADO PARA VOLUMEN
  client.println("<script>");
  client.println("function updateTrackInfo() {");
  client.println("  fetch('/status')");
  client.println("    .then(response => response.text())");
  client.println("    .then(data => {");
  client.println(
      "      const trackInput = document.querySelector('.Input-text');");
  client.println("      if (trackInput && data.trim() !== '') {");
  client.println("        trackInput.value = data.trim();");
  client.println("      }");
  client.println("    })");
  client.println("    .catch(error => console.log('Error:', error));");
  client.println("}");
  client.println("");
  client.println("// ✅ Función optimizada para comandos");
  client.println("function sendCommand(command) {");
  client.println("  // Para comandos de volumen, usar endpoint ultra-ligero");
  client.println("  if (command === 'VOLUP' || command === 'VOLDOWN') {");
  client.println("    fetch('/' + command, {");
  client.println("      method: 'GET'");
  client.println("    })");
  client.println("    .then(response => {");
  client.println("      // Solo log, sin más procesamiento para volumen");
  client.println("      console.log(command + ' sent (lightweight)');");
  client.println("    })");
  client.println("    .catch(error => console.log('Error:', error));");
  client.println("  } else {");
  client.println("    // Para otros comandos, usar el método normal");
  client.println("    fetch('/' + command, {");
  client.println("      method: 'GET'");
  client.println("    })");
  client.println("    .then(response => {");
  client.println("      if (response.ok) {");
  client.println(
      "        console.log(command + ' command sent successfully');");
  client.println(
      "        // ✅ SOLO actualizar textbox para comandos específicos");
  client.println("        if (command === 'PLAY' || command === 'NEXT' || "
                 "command === 'PREV') {");
  client.println("          setTimeout(updateTrackInfo, 500);");
  client.println("        }");
  client.println("      }");
  client.println("    })");
  client.println(
      "    .catch(error => console.log('Error sending command:', error));");
  client.println("  }");
  client.println("}");
  client.println("");
  client.println("// Configurar eventos cuando la página esté lista");
  client.println("document.addEventListener('DOMContentLoaded', function() {");
  client.println("  const trackInput = document.querySelector('.Input-text');");
  client.println("  if (trackInput) {");
  client.println("    trackInput.addEventListener('click', updateTrackInfo);");
  client.println("  }");
  client.println("  ");
  client.println(
      "  // ✅ Configurar eventos para TODOS los botones sin recarga");
  client.println(
      "  const allButtons = document.querySelectorAll('a[href^=\"/\"]');");
  client.println("  allButtons.forEach(button => {");
  client.println("    button.addEventListener('click', function(e) {");
  client.println("      e.preventDefault(); // ✅ Evitar recarga de página");
  client.println("      const href = this.getAttribute('href');");
  client.println("      const command = href.substring(1);");
  client.println("      sendCommand(command);");
  client.println("    });");
  client.println("  });");
  client.println("});");
  client.println("</script>");
}

void jsUpdateTextBoxWithFileList(WiFiClient client) {

  client.println("<script>");
  client.println("let fileListVisible = false;");
  client.println("");

  client.println("function updateTrackInfo() {");
  client.println("  fetch('/status')");
  client.println("    .then(response => response.text())");
  client.println("    .then(data => {");
  client.println(
      "      const trackInput = document.querySelector('.Input-text');");
  client.println("      if (trackInput && data.trim() !== '') {");
  client.println("        trackInput.value = data.trim();");
  client.println("      }");
  client.println("    })");
  client.println("    .catch(error => console.log('Error:', error));");
  client.println("}");
  client.println("");

  // ✅ NUEVA FUNCIÓN PARA ACTUALIZAR EL BOTÓN PLAY/STOP
  client.println("function updatePlayButton() {");
  client.println("  fetch('/playstatus')");
  client.println("    .then(response => response.text())");
  client.println("    .then(data => {");
  client.println("      const playButton = "
                 "document.querySelector('.center-button .symbol.play-stop');");
  client.println("      if (playButton) {");
  client.println("        playButton.textContent = data.trim();");
  client.println(
      "        console.log('Play button updated to:', data.trim());");
  client.println("      }");
  client.println("    })");
  client.println("    .catch(error => console.log('Error updating play "
                 "button:', error));");
  client.println("}");
  client.println("");

  client.println("function loadFileList() {");
  client.println("  fetch('/filelist')");
  client.println("    .then(response => response.json())");
  client.println("    .then(data => {");
  client.println("      const fileList = document.getElementById('fileList');");
  client.println("      fileList.innerHTML = '';");
  client.println("      ");
  client.println("      if (data.items && data.items.length > 0) {");
  client.println("        data.items.forEach((item, index) => {");
  client.println(
      "          const itemElement = document.createElement('div');");
  client.println("          itemElement.className = 'file-item';");
  client.println("          ");
  client.println("          // ✅ DIFERENTES ESTILOS PARA HOME, PARENT DIR, "
                 "DIRECTORIOS Y ARCHIVOS");
  client.println("          if (item.type === 'home') {");
  client.println("            itemElement.innerHTML = `HOME`;");
  client.println("            itemElement.style.color = '#00ffff';"); // Celeste
  client.println("            itemElement.style.fontWeight = 'bold';");
  client.println("            itemElement.classList.add('home');");
  client.println(
      "            itemElement.addEventListener('click', () => goHome());");
  client.println("          } else if (item.type === 'parent') {");
  client.println("            itemElement.innerHTML = `..`;");
  client.println(
      "            itemElement.style.color = '#00ffff';"); // Celeste también
                                                           // para parent
  client.println("            itemElement.style.fontWeight = 'bold';");
  client.println("            itemElement.classList.add('parent');");
  client.println("            itemElement.addEventListener('click', () => "
                 "changeDirectory('..'));");
  client.println("          } else if (item.type === 'directory') {");
  client.println("            itemElement.innerHTML = `DIR: ${item.name}`;");
  client.println("            itemElement.style.color = '#ffbf00';");
  client.println("            itemElement.style.fontWeight = 'bold';");
  client.println("            itemElement.classList.add('directory');");
  client.println("            itemElement.addEventListener('click', () => "
                 "changeDirectory(item.name));");
  client.println("          } else {");
  client.println("            let filePrefix = '';");
  client.println("            if (item.name.toUpperCase().endsWith('.MP3')) "
                 "filePrefix = 'MP3';");
  client.println(
      "            else if (item.name.toUpperCase().endsWith('.WAV')) "
      "filePrefix = 'WAV';");
  client.println(
      "            else if (item.name.toUpperCase().endsWith('.FLAC')) "
      "filePrefix = 'FLAC';");
  client.println(
      "            else if (item.name.toUpperCase().endsWith('.RADIO')) "
      "filePrefix = 'RADIO';");
  client.println("            else filePrefix = 'AUDIO';");
  client.println("            itemElement.innerHTML = `[${item.index + 1}] "
                 "${filePrefix}: ${item.name}`;");
  client.println("            itemElement.style.color = '#fff';");
  client.println("            itemElement.classList.add('file');");
  client.println("            itemElement.addEventListener('click', () => "
                 "selectFile(item.name, item.index));");
  client.println("          }");
  client.println("          ");
  client.println("          fileList.appendChild(itemElement);");
  client.println("        });");
  client.println("      } else {");
  client.println("        const noItems = document.createElement('div');");
  client.println("        noItems.className = 'file-item';");
  client.println(
      "        noItems.textContent = 'No audio files or directories found';");
  client.println("        fileList.appendChild(noItems);");
  client.println("      }");
  client.println("    })");
  client.println("    .catch(error => {");
  client.println("      console.log('Error loading file list:', error);");
  client.println("      const fileList = document.getElementById('fileList');");
  client.println("      fileList.innerHTML = '<div class=\"file-item\">Error "
                 "loading files</div>';");
  client.println("    });");
  client.println("}");
  client.println("");

  client.println("function goHome() {");
  client.println("  console.log('Going to HOME directory');");
  client.println("  ");
  client.println("  fetch('/gohome', {");
  client.println("    method: 'GET'");
  client.println("  })");
  client.println("  .then(response => {");
  client.println("    if (response.ok) {");
  client.println("      console.log('HOME directory changed successfully');");
  client.println("      setTimeout(loadFileList, 300);");
  client.println("    }");
  client.println("  })");
  client.println(
      "  .catch(error => console.log('Error going to HOME:', error));");
  client.println("}");
  client.println("");

  client.println("function changeDirectory(dirName) {");
  client.println("  console.log('Changing to directory:', dirName);");
  client.println("  ");
  client.println("  const encodedDirName = encodeURIComponent(dirName);");
  client.println("  ");
  client.println("  fetch('/changedir/' + encodedDirName, {");
  client.println("    method: 'GET'");
  client.println("  })");
  client.println("  .then(response => {");
  client.println("    if (response.ok) {");
  client.println("      console.log('Directory changed successfully');");
  client.println("      setTimeout(loadFileList, 300);");
  client.println("    }");
  client.println("  })");
  client.println(
      "  .catch(error => console.log('Error changing directory:', error));");
  client.println("}");
  client.println("");

  client.println("function selectFile(filename, position) {");
  client.println(
      "  console.log('Selecting file:', filename, 'at position:', position);");
  client.println("  ");
  client.println("  const encodedFilename = encodeURIComponent(filename);");
  client.println("  ");
  client.println(
      "  fetch('/select/' + encodedFilename + '?pos=' + position, {");
  client.println("    method: 'GET'");
  client.println("  })");
  client.println("  .then(response => {");
  client.println("    if (response.ok) {");
  client.println("      console.log('File selected successfully at position:', "
                 "position);");
  client.println("      setTimeout(updateTrackInfo, 500);");
  client.println("      toggleFileList();");
  client.println("    }");
  client.println("  })");
  client.println(
      "  .catch(error => console.log('Error selecting file:', error));");
  client.println("}");
  client.println("");

  client.println("function toggleFileList() {");
  client.println("  const fileList = document.getElementById('fileList');");
  client.println("  fileListVisible = !fileListVisible;");
  client.println("  ");
  client.println("  if (fileListVisible) {");
  client.println("    loadFileList();");
  client.println("    fileList.classList.add('show');");
  client.println("  } else {");
  client.println("    fileList.classList.remove('show');");
  client.println("  }");
  client.println("}");
  client.println("");

  client.println("function sendCommand(command) {");
  client.println("  if (command === 'VOLUP' || command === 'VOLDOWN') {");
  client.println("    fetch('/' + command, {");
  client.println("      method: 'GET'");
  client.println("    })");
  client.println("    .then(response => {");
  client.println("      console.log(command + ' sent (lightweight)');");
  client.println("    })");
  client.println("    .catch(error => console.log('Error:', error));");
  client.println("  } else {");
  client.println("    fetch('/' + command, {");
  client.println("      method: 'GET'");
  client.println("    })");
  client.println("    .then(response => {");
  client.println("      if (response.ok) {");
  client.println(
      "        console.log(command + ' command sent successfully');");

  client.println("        // ✅ ACTUALIZAR BOTÓN PLAY/STOP INMEDIATAMENTE");
  client.println("        if (command === 'PLAY') {");
  client.println("          setTimeout(() => {");
  client.println("            updatePlayButton();");
  client.println("            updateTrackInfo();");
  client.println(
      "          }, 300);"); // Pequeño delay para que el servidor procese
  client.println(
      "        } else if (command === 'NEXT' || command === 'PREV') {");
  client.println("          setTimeout(updateTrackInfo, 500);");
  client.println(
      "        } else if (command === 'STOP' || command === 'PAUSE') {");
  client.println("          setTimeout(() => {");
  client.println("            updatePlayButton();");
  client.println("            updateTrackInfo();");
  client.println("          }, 300);");
  client.println("        }");
  client.println("      }");
  client.println("    })");
  client.println(
      "    .catch(error => console.log('Error sending command:', error));");
  client.println("  }");
  client.println("}");
  client.println("");

  client.println("document.addEventListener('DOMContentLoaded', function() {");
  client.println("  const trackInput = document.querySelector('.Input-text');");
  client.println("  if (trackInput) {");
  client.println("    trackInput.addEventListener('click', updateTrackInfo);");
  client.println("  }");
  client.println("  ");
  client.println("  const browseBtn = document.getElementById('browseBtn');");
  client.println("  if (browseBtn) {");
  client.println("    browseBtn.addEventListener('click', toggleFileList);");
  client.println("  }");
  client.println("  ");
  client.println(
      "  const allButtons = document.querySelectorAll('a[href^=\"/\"]');");
  client.println("  allButtons.forEach(button => {");
  client.println("    button.addEventListener('click', function(e) {");
  client.println("      e.preventDefault();");
  client.println("      const href = this.getAttribute('href');");
  client.println("      const command = href.substring(1);");
  client.println("      sendCommand(command);");
  client.println("    });");
  client.println("  });");
  client.println("  ");
  client.println("  // ✅ ACTUALIZAR BOTÓN AL CARGAR LA PÁGINA");
  client.println("  updatePlayButton();");
  client.println("  updateTrackInfo();");
  client.println("  ");

  client.println("  document.addEventListener('click', function(e) {");
  client.println("    const fileList = document.getElementById('fileList');");
  client.println("    const browseBtn = document.getElementById('browseBtn');");
  client.println("    const wrapper = document.querySelector('.Wrapper');");
  client.println("    ");
  client.println("    if (fileListVisible && !wrapper.contains(e.target)) {");
  client.println("      fileList.classList.remove('show');");
  client.println("      fileListVisible = false;");
  client.println("    }");
  client.println("  });");

  client.println("});");
  client.println("</script>");
}

// void jsUpdateTextBoxWithFileList(WiFiClient client)
// {
//     client.println("<script>");
//     client.println("let fileListVisible = false;");
//     client.println("");
//     client.println("function updateTrackInfo() {");
//     client.println("  fetch('/status')");
//     client.println("    .then(response => response.text())");
//     client.println("    .then(data => {");
//     client.println("      const trackInput =
//     document.querySelector('.Input-text');"); client.println("      if
//     (trackInput && data.trim() !== '') {"); client.println(" trackInput.value
//     = data.trim();"); client.println("      }"); client.println("    })");
//     client.println("    .catch(error => console.log('Error:', error));");
//     client.println("}");
//     client.println("");
//     client.println("function loadFileList() {");
//     client.println("  fetch('/filelist')");
//     client.println("    .then(response => response.json())");
//     client.println("    .then(data => {");
//     client.println("      const fileList =
//     document.getElementById('fileList');"); client.println("
//     fileList.innerHTML = '';"); client.println("      "); client.println(" if
//     (data.items && data.items.length > 0) {"); client.println("
//     data.items.forEach((item, index) => {"); client.println("          const
//     itemElement = document.createElement('div');"); client.println("
//     itemElement.className = 'file-item';"); client.println("          ");
//     client.println("          // ✅ DIFERENTES ICONOS Y COLORES PARA
//     DIRECTORIOS Y ARCHIVOS"); client.println("          if (item.type ===
//     'directory') {"); client.println("            itemElement.innerHTML = `📁
//     ${item.name}`;"); client.println("            itemElement.style.color =
//     '#ffbf00';"); client.println("            itemElement.style.fontWeight =
//     'bold';"); client.println(" itemElement.classList.add('directory');");
//     client.println("            itemElement.addEventListener('click', () =>
//     changeDirectory(item.name));"); client.println("          } else {");
//     client.println("            let fileIcon = '🎵';");
//     client.println("            if (item.name.toUpperCase().endsWith('.MP3'))
//     fileIcon = '🎵';"); client.println("            else if
//     (item.name.toUpperCase().endsWith('.WAV')) fileIcon = '🎶';");
//     client.println("            else if
//     (item.name.toUpperCase().endsWith('.FLAC')) fileIcon = '🎼';");
//     client.println("            else if
//     (item.name.toUpperCase().endsWith('.RADIO')) fileIcon = '📻';");
//     client.println("            itemElement.innerHTML = `${fileIcon}
//     [${item.index + 1}] ${item.name}`;"); client.println("
//     itemElement.style.color = '#fff';"); client.println("
//     itemElement.classList.add('file');"); client.println("
//     itemElement.addEventListener('click', () => selectFile(item.name,
//     item.index));"); client.println("          }"); client.println(" ");
//     client.println("          fileList.appendChild(itemElement);");
//     client.println("        });");
//     client.println("      } else {");
//     client.println("        const noItems = document.createElement('div');");
//     client.println("        noItems.className = 'file-item';");
//     client.println("        noItems.textContent = 'No audio files or
//     directories found';"); client.println(" fileList.appendChild(noItems);");
//     client.println("      }");
//     client.println("    })");
//     client.println("    .catch(error => {");
//     client.println("      console.log('Error loading file list:', error);");
//     client.println("      const fileList =
//     document.getElementById('fileList');"); client.println("
//     fileList.innerHTML = '<div class=\"file-item\">Error loading
//     files</div>';"); client.println("    });"); client.println("}");
//     client.println("");
//     client.println("function changeDirectory(dirName) {");
//     client.println("  console.log('Changing to directory:', dirName);");
//     client.println("  ");
//     client.println("  const encodedDirName = encodeURIComponent(dirName);");
//     client.println("  ");
//     client.println("  fetch('/changedir/' + encodedDirName, {");
//     client.println("    method: 'GET'");
//     client.println("  })");
//     client.println("  .then(response => {");
//     client.println("    if (response.ok) {");
//     client.println("      console.log('Directory changed successfully');");
//     client.println("      // ✅ RECARGAR LA LISTA DESPUÉS DEL CAMBIO DE
//     DIRECTORIO"); client.println("      setTimeout(loadFileList, 300);");
//     client.println("    }");
//     client.println("  })");
//     client.println("  .catch(error => console.log('Error changing
//     directory:', error));"); client.println("}"); client.println("");
//     client.println("function selectFile(filename, position) {");
//     client.println("  console.log('Selecting file:', filename, 'at
//     position:', position);"); client.println("  "); client.println("  const
//     encodedFilename = encodeURIComponent(filename);"); client.println("  ");
//     client.println("  fetch('/select/' + encodedFilename + '?pos=' +
//     position, {"); client.println("    method: 'GET'"); client.println("
//     })"); client.println("  .then(response => {"); client.println("    if
//     (response.ok) {"); client.println("      console.log('File selected
//     successfully at position:', position);"); client.println("
//     setTimeout(updateTrackInfo, 500);"); client.println("
//     toggleFileList();"); client.println("    }"); client.println("  })");
//     client.println("  .catch(error => console.log('Error selecting file:',
//     error));"); client.println("}"); client.println("");
//     client.println("function toggleFileList() {");
//     client.println("  const fileList =
//     document.getElementById('fileList');"); client.println("  fileListVisible
//     = !fileListVisible;"); client.println("  "); client.println("  if
//     (fileListVisible) {"); client.println("    loadFileList();");
//     client.println("    fileList.classList.add('show');");
//     client.println("  } else {");
//     client.println("    fileList.classList.remove('show');");
//     client.println("  }");
//     client.println("}");
//     client.println("");
//     client.println("function sendCommand(command) {");
//     client.println("  if (command === 'VOLUP' || command === 'VOLDOWN') {");
//     client.println("    fetch('/' + command, {");
//     client.println("      method: 'GET'");
//     client.println("    })");
//     client.println("    .then(response => {");
//     client.println("      console.log(command + ' sent (lightweight)');");
//     client.println("    })");
//     client.println("    .catch(error => console.log('Error:', error));");
//     client.println("  } else {");
//     client.println("    fetch('/' + command, {");
//     client.println("      method: 'GET'");
//     client.println("    })");
//     client.println("    .then(response => {");
//     client.println("      if (response.ok) {");
//     client.println("        console.log(command + ' command sent
//     successfully');"); client.println("        if (command === 'PLAY' ||
//     command === 'NEXT' || command === 'PREV') {"); client.println("
//     setTimeout(updateTrackInfo, 500);"); client.println("        }");
//     client.println("      }");
//     client.println("    })");
//     client.println("    .catch(error => console.log('Error sending command:',
//     error));"); client.println("  }"); client.println("}");
//     client.println("");
//     client.println("document.addEventListener('DOMContentLoaded', function()
//     {"); client.println("  const trackInput =
//     document.querySelector('.Input-text');"); client.println("  if
//     (trackInput) {"); client.println(" trackInput.addEventListener('click',
//     updateTrackInfo);"); client.println("  }"); client.println("  ");
//     client.println("  const browseBtn =
//     document.getElementById('browseBtn');"); client.println("  if (browseBtn)
//     {"); client.println("    browseBtn.addEventListener('click',
//     toggleFileList);"); client.println("  }"); client.println("  ");
//     client.println("  const allButtons =
//     document.querySelectorAll('a[href^=\"/\"]');"); client.println("
//     allButtons.forEach(button => {"); client.println("
//     button.addEventListener('click', function(e) {"); client.println("
//     e.preventDefault();"); client.println("      const href =
//     this.getAttribute('href');"); client.println("      const command =
//     href.substring(1);"); client.println("      sendCommand(command);");
//     client.println("    });");
//     client.println("  });");
//     client.println("  ");
//     client.println("  document.addEventListener('click', function(e) {");
//     client.println("    const fileList =
//     document.getElementById('fileList');"); client.println("    const
//     browseBtn = document.getElementById('browseBtn');"); client.println("
//     const wrapper = document.querySelector('.Wrapper');"); client.println("
//     "); client.println("    if (fileListVisible &&
//     !wrapper.contains(e.target)) {"); client.println("
//     fileList.classList.remove('show');"); client.println(" fileListVisible =
//     false;"); client.println("    }"); client.println("  });");
//     client.println("});");
//     client.println("</script>");
// }

// void jsUpdateTextBoxWithFileList(WiFiClient client)
// {
//     client.println("<script>");
//     client.println("let fileListVisible = false;");
//     client.println("");
//     client.println("function updateTrackInfo() {");
//     client.println("  fetch('/status')");
//     client.println("    .then(response => response.text())");
//     client.println("    .then(data => {");
//     client.println("      const trackInput =
//     document.querySelector('.Input-text');"); client.println("      if
//     (trackInput && data.trim() !== '') {"); client.println(" trackInput.value
//     = data.trim();"); client.println("      }"); client.println("    })");
//     client.println("    .catch(error => console.log('Error:', error));");
//     client.println("}");
//     client.println("");
//     client.println("function loadFileList() {");
//     client.println("  fetch('/filelist')");
//     client.println("    .then(response => response.json())");
//     client.println("    .then(data => {");
//     client.println("      const fileList =
//     document.getElementById('fileList');"); client.println("
//     fileList.innerHTML = '';"); client.println("      "); client.println(" if
//     (data.files && data.files.length > 0) {"); client.println("
//     data.files.forEach((file, index) => {"); // ✅ Añadir index
//     client.println("          const fileItem =
//     document.createElement('div');"); // ✅ AÑADIDA ESTA LÍNEA QUE FALTABA
//     client.println("          fileItem.className = 'file-item';");
//     client.println("          fileItem.textContent = `[${index + 1}]
//     ${file}`;"); // ✅ Mostrar posición client.println("
//     fileItem.addEventListener('click', () => selectFile(file, index));"); //
//     ✅ Pasar index client.println(" fileList.appendChild(fileItem);");
//     client.println("        });");
//     client.println("      } else {");
//     client.println("        const noFiles = document.createElement('div');");
//     client.println("        noFiles.className = 'file-item';");
//     client.println("        noFiles.textContent = 'No files found';");
//     client.println("        fileList.appendChild(noFiles);");
//     client.println("      }");
//     client.println("    })");
//     client.println("    .catch(error => {");
//     client.println("      console.log('Error loading file list:', error);");
//     client.println("      const fileList =
//     document.getElementById('fileList');"); client.println("
//     fileList.innerHTML = '<div class=\"file-item\">Error loading
//     files</div>';"); client.println("    });"); client.println("}");
//     client.println("");
//     client.println("function selectFile(filename, position) {"); // ✅ Añadir
//     parámetro position client.println("  console.log('Selecting file:',
//     filename, 'at position:', position);"); client.println("  ");
//     client.println("  // Codificar el nombre del archivo para URL");
//     client.println("  const encodedFilename =
//     encodeURIComponent(filename);"); client.println("  "); client.println("
//     fetch('/select/' + encodedFilename + '?pos=' + position, {"); // ✅
//     Añadir ?pos= client.println("    method: 'GET'"); client.println("  })");
//     client.println("  .then(response => {");
//     client.println("    if (response.ok) {");
//     client.println("      console.log('File selected successfully at
//     position:', position);"); client.println("      // Actualizar el
//     textbox"); client.println("      setTimeout(updateTrackInfo, 500);");
//     client.println("      // Ocultar la lista");
//     client.println("      toggleFileList();");
//     client.println("    }");
//     client.println("  })");
//     client.println("  .catch(error => console.log('Error selecting file:',
//     error));"); client.println("}"); client.println("");
//     client.println("function toggleFileList() {");
//     client.println("  const fileList =
//     document.getElementById('fileList');"); client.println("  fileListVisible
//     = !fileListVisible;"); client.println("  "); client.println("  if
//     (fileListVisible) {"); client.println("    loadFileList();");
//     client.println("    fileList.classList.add('show');");
//     client.println("  } else {");
//     client.println("    fileList.classList.remove('show');");
//     client.println("  }");
//     client.println("}");
//     client.println("");
//     client.println("function sendCommand(command) {");
//     client.println("  if (command === 'VOLUP' || command === 'VOLDOWN') {");
//     client.println("    fetch('/' + command, {");
//     client.println("      method: 'GET'");
//     client.println("    })");
//     client.println("    .then(response => {");
//     client.println("      console.log(command + ' sent (lightweight)');");
//     client.println("    })");
//     client.println("    .catch(error => console.log('Error:', error));");
//     client.println("  } else {");
//     client.println("    fetch('/' + command, {");
//     client.println("      method: 'GET'");
//     client.println("    })");
//     client.println("    .then(response => {");
//     client.println("      if (response.ok) {");
//     client.println("        console.log(command + ' command sent
//     successfully');"); client.println("        if (command === 'PLAY' ||
//     command === 'NEXT' || command === 'PREV') {"); client.println("
//     setTimeout(updateTrackInfo, 500);"); client.println("        }");
//     client.println("      }");
//     client.println("    })");
//     client.println("    .catch(error => console.log('Error sending command:',
//     error));"); client.println("  }"); client.println("}");
//     client.println("");
//     client.println("// Configurar eventos cuando la página esté lista");
//     client.println("document.addEventListener('DOMContentLoaded', function()
//     {"); client.println("  const trackInput =
//     document.querySelector('.Input-text');"); client.println("  if
//     (trackInput) {"); client.println(" trackInput.addEventListener('click',
//     updateTrackInfo);"); client.println("  }"); client.println("  ");
//     client.println("  // Configurar el botón browse");
//     client.println("  const browseBtn =
//     document.getElementById('browseBtn');"); client.println("  if (browseBtn)
//     {"); client.println("    browseBtn.addEventListener('click',
//     toggleFileList);"); client.println("  }"); client.println("  ");
//     client.println("  // Configurar eventos para todos los botones del
//     control"); client.println("  const allButtons =
//     document.querySelectorAll('a[href^=\"/\"]');"); client.println("
//     allButtons.forEach(button => {"); client.println("
//     button.addEventListener('click', function(e) {"); client.println("
//     e.preventDefault();"); client.println("      const href =
//     this.getAttribute('href');"); client.println("      const command =
//     href.substring(1);"); client.println("      sendCommand(command);");
//     client.println("    });");
//     client.println("  });");
//     client.println("  ");
//     client.println("  // Cerrar lista al hacer click fuera");
//     client.println("  document.addEventListener('click', function(e) {");
//     client.println("    const fileList =
//     document.getElementById('fileList');"); client.println("    const
//     browseBtn = document.getElementById('browseBtn');"); client.println("
//     const wrapper = document.querySelector('.Wrapper');"); client.println("
//     "); client.println("    if (fileListVisible &&
//     !wrapper.contains(e.target)) {"); client.println("
//     fileList.classList.remove('show');"); client.println(" fileListVisible =
//     false;"); client.println("    }"); client.println("  });");
//     client.println("});");
//     client.println("</script>");
// }

void stylebuttonscss(WiFiClient client) {
  // Botón azul normal
  client.println(".button-71 {");
  client.println("background-color: #0078d0;");
  client.println("border: 0;");
  client.println("border-radius: 56px;");
  client.println("color: #fff;");
  client.println("cursor: pointer;");
  client.println("display: inline-block;");
  client.println("font-family: system-ui,-apple-system,system-ui,\"Segoe "
                 "UI\",Roboto,Ubuntu,\"Helvetica Neue\",sans-serif;");
  client.println("font-size: 18px;");
  client.println("font-weight: 600;");
  client.println("outline: 0;");
  client.println("padding: 16px 21px;");
  client.println("position: relative;");
  client.println("text-align: center;");
  client.println("text-decoration: none;");
  client.println("transition: all .3s;");
  client.println("user-select: none;");
  client.println("-webkit-user-select: none;");
  client.println("touch-action: manipulation;");
  client.println("width: 180px;");
  client.println("box-sizing: border-box;");
  client.println("margin: 5px;");
  client.println("}");
  client.println(".button-71:before {");
  client.println("background-color: initial;");
  client.println("background-image: linear-gradient(#fff 0, rgba(255, 255, "
                 "255, 0) 100%);");
  client.println("border-radius: 125px;");
  client.println("content: \"\";");
  client.println("height: 50%;");
  client.println("left: 4%;");
  client.println("opacity: .5;");
  client.println("position: absolute;");
  client.println("top: 0;");
  client.println("transition: all .3s;");
  client.println("width: 92%;");
  client.println("}");
  client.println(".button-71:hover {");
  client.println("box-shadow: rgba(255, 255, 255, .2) 0 3px 15px inset, "
                 "rgba(0, 0, 0, .1) 0 3px 5px, rgba(0, 0, 0, .1) 0 10px 13px;");
  client.println("transform: scale(1.05);");
  client.println("}");

  // Botón rojo
  client.println(".button-red {");
  client.println("background-color: #dc3545;");
  client.println("border: 0;");
  client.println("border-radius: 56px;");
  client.println("color: #fff;");
  client.println("cursor: pointer;");
  client.println("display: inline-block;");
  client.println("font-family: system-ui,-apple-system,system-ui,\"Segoe "
                 "UI\",Roboto,Ubuntu,\"Helvetica Neue\",sans-serif;");
  client.println("font-size: 18px;");
  client.println("font-weight: 600;");
  client.println("outline: 0;");
  client.println("padding: 16px 21px;");
  client.println("position: relative;");
  client.println("text-align: center;");
  client.println("text-decoration: none;");
  client.println("transition: all .3s;");
  client.println("user-select: none;");
  client.println("-webkit-user-select: none;");
  client.println("touch-action: manipulation;");
  client.println("width: 180px;");
  client.println("box-sizing: border-box;");
  client.println("margin: 5px;");
  client.println("}");
  client.println(".button-red:before {");
  client.println("background-color: initial;");
  client.println("background-image: linear-gradient(#fff 0, rgba(255, 255, "
                 "255, 0) 100%);");
  client.println("border-radius: 125px;");
  client.println("content: \"\";");
  client.println("height: 50%;");
  client.println("left: 4%;");
  client.println("opacity: .5;");
  client.println("position: absolute;");
  client.println("top: 0;");
  client.println("transition: all .3s;");
  client.println("width: 92%;");
  client.println("}");
  client.println(".button-red:hover {");
  client.println("background-color: #c82333;");
  client.println("box-shadow: rgba(255, 255, 255, .2) 0 3px 15px inset, "
                 "rgba(0, 0, 0, .1) 0 3px 5px, rgba(0, 0, 0, .1) 0 10px 13px;");
  client.println("transform: scale(1.05);");
  client.println("}");

  client.println(".volume-controls {");
  client.println("display: flex;");
  client.println("justify-content: center;");
  client.println("align-items: center;");
  client.println("gap: 10px;");
  client.println("margin: 10px 0;");
  client.println("}");
  client.println("@media (min-width: 768px) {");
  client.println(".button-71, .button-red {");
  client.println("padding: 16px 48px;");
  client.println("width: 200px;");
  client.println("}}");
}

void remoteControlStyle(WiFiClient client) {
  // ✅ DEFINIR VARIABLES CSS AL INICIO
  client.println(":root {");
  client.println("--circular-bg-color: #534d44ff;");
  client.println("--circular-shadow-dark: #272727ff;");
  client.println("--circular-shadow-light: #afafafff;");
  client.println("--circular-shadow-inner: #b68c19ff;"); // esta
  client.println("--button-gradient-light: #fff;");
  client.println("--button-gradient-dark: #2e2e2eff;");
  client.println("--center-button-bg: #494949ff;");
  client.println("--symbol-shadow: rgba(0, 0, 0, 0.7);");
  client.println("--symbol-color: rgba(255, 255, 255, 0.8);");
  client.println("--symbol-play-stop: rgba(0, 0, 0, 0.8);");
  client.println("}");

  // ✅ CONTENEDOR PARA CENTRAR EL CONTROL CIRCULAR
  client.println(".circular-control-container {");
  client.println("display: flex;");
  client.println("justify-content: center;");
  client.println("align-items: center;");
  client.println("width: 100%;");
  client.println("margin: 20px 0;");
  client.println("}");

  client.println("nav {");
  client.println("width: 400px;");
  client.println("height: 400px;");
  client.println("background: var(--circular-bg-color);");
  client.println("border-radius: 50%;");
  client.println("padding: 20px;");
  client.println("-webkit-transform: rotate(45deg);");
  client.println("-moz-transform: rotate(45deg);");
  client.println("transform: rotate(45deg);");
  client.println(
      "box-shadow: inset -12px 0 12px -6px var(--circular-shadow-dark), inset "
      "12px 0 12px -6px var(--circular-shadow-light), inset 0 0 0 12px "
      "var(--circular-shadow-inner), inset 2px 0 4px 12px rgba(0, 0, 0, 0.4), "
      "1px 0 4px rgba(0, 0, 0, 0.8);");
  client.println("box-sizing: border-box;");
  client.println("position: relative;");
  client.println("margin: 0 auto;"); // ✅ Centrar horizontalmente
  client.println("display: block;"); // ✅ Asegurar que es un bloque
  client.println("}");

  // ✅ RESPONSIVE: Ajustar tamaño en pantallas pequeñas
  client.println("@media (max-width: 480px) {");
  client.println("nav {");
  client.println("width: 300px;");
  client.println("height: 300px;");
  client.println("}");
  client.println("}");

  client.println("a {");
  client.println("text-decoration: none;");
  client.println("}");

  client.println(".center-button {");
  client.println("display: block;");
  client.println("height: 38%;");
  client.println("width: 38%;");
  client.println("position: absolute;");
  client.println("top: 31%;");
  client.println("left: 31%;");
  client.println("background: var(--center-button-bg);");
  client.println("border-radius: 50%;");
  client.println("box-shadow: 1px 0 4px rgba(0, 0, 0, 0.8);");
  client.println("display: flex;");
  client.println("align-items: center;");
  client.println("justify-content: center;");
  client.println("}");

  client.println(".button {");
  client.println("display: block;");
  client.println("width: 46%;");
  client.println("height: 46%;");
  client.println("margin: 2%;");
  client.println("position: relative;");
  client.println("float: left;");
  client.println("box-shadow: 1px 0px 3px 1px rgba(0, 0, 0, 0.4), inset 0 0 0 "
                 "1px var(--circular-shadow-light);");
  client.println("cursor: pointer;");
  client.println("display: flex;");
  client.println("align-items: center;");
  client.println("justify-content: center;");
  client.println("}");

  client.println(".button::after {");
  client.println("content: '';");
  client.println("display: block;");
  client.println("width: 50%;");
  client.println("height: 50%;");
  client.println("background: var(--circular-bg-color);");
  client.println("position: absolute;");
  client.println("border-radius: inherit;");
  client.println("z-index: 1;");
  client.println("}");

  client.println(".button.top {");
  client.println("border-radius: 100% 0 0 0;");
  client.println(
      "background: radial-gradient(bottom right, ellipse cover, "
      "var(--button-gradient-light) 35%, var(--button-gradient-dark) 75%);");
  client.println("}");

  client.println(".button.top::after {");
  client.println("bottom: 0;");
  client.println("right: 0;");
  client.println("box-shadow: inset 2px 1px 2px 0 rgba(0, 0, 0, 0.4), 10px "
                 "10px 0 10px var(--circular-bg-color);");
  client.println("-webkit-transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("-moz-transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("}");

  client.println(".button.right {");
  client.println("border-radius: 0 100% 0 0;");
  client.println(
      "background: radial-gradient(bottom left, ellipse cover, "
      "var(--button-gradient-light) 35%, var(--button-gradient-dark) 75%);");
  client.println("}");

  client.println(".button.right::after {");
  client.println("bottom: 0;");
  client.println("left: 0;");
  client.println("box-shadow: inset -2px 3px 2px -2px rgba(0, 0, 0, 0.4), "
                 "-10px 10px 0 10px var(--circular-bg-color);");
  client.println("-webkit-transform: skew(3deg, 3deg) scale(0.96);");
  client.println("-moz-transform: skew(3deg, 3deg) scale(0.96);");
  client.println("transform: skew(3deg, 3deg) scale(0.96);");
  client.println("}");

  client.println(".button.left {");
  client.println("border-radius: 0 0 0 100%;");
  client.println(
      "background: radial-gradient(top right, ellipse cover, "
      "var(--button-gradient-light) 35%, var(--button-gradient-dark) 75%);");
  client.println("}");

  client.println(".button.left::after {");
  client.println("top: 0;");
  client.println("right: 0;");
  client.println("box-shadow: inset 2px -1px 2px 0 rgba(0, 0, 0, 0.4), 10px "
                 "-10px 0 10px var(--circular-bg-color);");
  client.println("-webkit-transform: skew(3deg, 3deg) scale(0.96);");
  client.println("-moz-transform: skew(3deg, 3deg) scale(0.96);");
  client.println("transform: skew(3deg, 3deg) scale(0.96);");
  client.println("}");

  client.println(".button.bottom {");
  client.println("border-radius: 0 0 100% 0;");
  client.println(
      "background: radial-gradient(top left, ellipse cover, "
      "var(--button-gradient-light) 35%, var(--button-gradient-dark) 75%);");
  client.println("}");

  client.println(".button.bottom::after {");
  client.println("top: 0;");
  client.println("left: 0;");
  client.println("box-shadow: inset -2px -3px 2px -2px rgba(0, 0, 0, 0.4), "
                 "-10px -10px 0 10px var(--circular-bg-color);");
  client.println("-webkit-transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("-moz-transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("transform: skew(-3deg, -3deg) scale(0.96);");
  client.println("}");

  // ESTILOS PARA LOS SÍMBOLOS
  client.println(".symbol {");
  client.println("font-size: 24px;");
  client.println("color: var(--symbol-color);");
  client.println("font-weight: bold;");
  client.println("text-shadow: 1px 1px 2px var(--symbol-shadow);");
  client.println("z-index: 10;");
  client.println("position: relative;");
  client.println("-webkit-transform: rotate(-45deg);");
  client.println("-moz-transform: rotate(-45deg);");
  client.println("transform: rotate(-45deg);");
  client.println("pointer-events: none;");
  client.println("}");

  client.println(".symbol.play-stop {");
  client.println("font-size: 28px;");
  client.println("color: var(--symbol-play-stop);");
  client.println("-webkit-transform: rotate(-45deg);");
  client.println("-moz-transform: rotate(-45deg);");
  client.println("transform: rotate(-45deg);");
  client.println("}");

  client.println(".symbol.volume {");
  client.println("font-size: 20px;");
  client.println("color: var(--symbol-color);");
  client.println("}");

  client.println(".symbol.track {");
  client.println("font-size: 16px;");
  client.println("color: var(--symbol-color);");
  client.println("display: flex;");           // ✅ Añadir
  client.println("align-items: center;");     // ✅ Añadir
  client.println("justify-content: center;"); // ✅ Añadir
  client.println("text-align: center;");      // ✅ Añadir
  client.println("}");
}

void textBoxcss(WiFiClient client) {
  // ✅ VARIABLES CSS SOLO PARA EL TEXTBOX
  client.println(".Wrapper {");
  client.println("width: 100%;");
  client.println("max-width: 400px;");
  client.println("margin: 20px auto;");
  client.println("padding: 0 20px;");
  client.println("}");

  client.println(".Input {");
  client.println("position: relative;");
  client.println("margin-bottom: 20px;");
  client.println("}");

  client.println(".Input-text {");
  client.println("display: block;");
  client.println("margin: 0;");
  client.println("padding: 12px 16px;");
  client.println("color: #fff;");
  client.println("background-color: #333;");
  client.println("width: 100%;");
  client.println("font-family: inherit;");
  client.println("font-size: 16px;");
  client.println("font-weight: normal;");
  client.println("border: 1px solid #555;");
  client.println("border-radius: 4px;");
  client.println("box-sizing: border-box;");
  client.println("text-align: center;");
  client.println("}");

  client.println(".Input-text:focus {");
  client.println("outline: none;");
  client.println("border-color: #ffbf00;");
  client.println("}");

  // ✅ CSS adicional para HOME y elementos de navegación
  client.println(".file-item.home {");
  client.println("background-color: #1a3d4d;"); // Fondo azul oscuro para HOME
  client.println("border-left: 4px solid #00ffff;"); // Borde celeste
  client.println("text-align: left;");
  client.println("font-weight: bold;");
  client.println("padding: 12px 15px;"); // Un poco más de padding para destacar
  client.println("}");

  client.println(".file-item.home:hover {");
  client.println("background-color: #2a4d5d;");
  client.println("border-left-color: #ffffff;");
  client.println("color: #ffffff;");
  client.println("}");

  client.println(".file-item.parent {");
  client.println("background-color: #2a2a2a;");
  client.println("border-left: 3px solid #00ffff;"); // Celeste para parent
  client.println("text-align: left;");
  client.println("}");

  client.println(".file-item.parent:hover {");
  client.println("background-color: #3a3a3a;");
  client.println("border-left-color: #ffffff;");
  client.println("color: #ffffff;");
  client.println("}");
}

void readyPlayFile() {
  UPDATE_FROM_REMOTE_CONTROL = true;
  FILE_PREPARED = true;
  PLAY = true;
  STOP = false;
  PAUSE = false;
  FILE_BROWSER_OPEN = false;
  // Llamamos a la carga de ficheros
  TAPESTATE = 100;
}

void handleWebClient(WiFiClient client) {
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {

            // ✅ ENDPOINT ULTRA-LIGERO PARA VOLUMEN
            if (header.indexOf("GET /VOL") >= 0) {
              client.println("HTTP/1.1 204 No Content");
              client.println("Connection: close");
              client.println();

              if (header.indexOf("GET /VOLUP") >= 0) {
                hmi.verifyCommand("VOLUP");
                hmi.writeString("menuAudio.volM.val=" + String(MAIN_VOL));
                hmi.writeString("menuAudio.volLevelM.val=" + String(MAIN_VOL));
                logln("Volume UP via web: " + String(MAIN_VOL));
              } else if (header.indexOf("GET /VOLDOWN") >= 0) {
                hmi.verifyCommand("VOLDW");
                hmi.writeString("menuAudio.volM.val=" + String(MAIN_VOL));
                hmi.writeString("menuAudio.volLevelM.val=" + String(MAIN_VOL));
                logln("Volume DOWN via web: " + String(MAIN_VOL));
              }

              break;
            }

            // ✅ ENDPOINT PARA LISTAR ARCHIVOS Y DIRECTORIOS - SOLO AUDIO
            if (header.indexOf("GET /filelist") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/json");
              client.println("Connection: close");
              client.println();

              // ✅ VERIFICAR Y ESTABLECER FILE_LAST_DIR
              String parentDir = FILE_LAST_DIR;

              // Si FILE_LAST_DIR está vacío, usar directorio raíz por defecto
              if (parentDir.length() == 0) {
                parentDir = "/";
                logln("FILE_LAST_DIR was empty, using root directory");
              }

              if (!parentDir.endsWith("/"))
                parentDir += "/";

              logln("=== DEBUG FILE LIST ===");
              logln("FILE_LAST_DIR: '" + FILE_LAST_DIR + "'");
              logln("parentDir: '" + parentDir + "'");

              // ✅ USAR _files.lst PARA DIRECTORIOS E idx.txt PARA ARCHIVOS
              String filesListPath = parentDir + "_files.lst";
              String idxFilePath = parentDir + "idx.txt";

              logln("filesListPath: '" + filesListPath + "'");
              logln("idxFilePath: '" + idxFilePath + "'");
              logln("_files.lst exists: " +
                    String(SD_MMC.exists(filesListPath.c_str())));
              logln("idx.txt exists: " +
                    String(SD_MMC.exists(idxFilePath.c_str())));

              // Generar lista JSON
              client.println("{");
              client.println("\"currentPath\":\"" + parentDir + "\",");
              client.println("\"items\":[");

              bool firstItem = true;
              int itemIndex = 0;

              // ✅ AÑADIR HOME COMO PRIMER ELEMENTO SIEMPRE
              client.println(
                  "{\"name\":\"HOME\",\"type\":\"home\",\"index\":-2}");
              firstItem = false;

              // ✅ AÑADIR DIRECTORIO PADRE SI NO ESTAMOS EN RAÍZ
              if (parentDir != "/") {
                client.println(
                    ",{\"name\":\".. (Parent "
                    "Directory)\",\"type\":\"parent\",\"index\":-1}");
              }

              // ✅ AÑADIR DIRECTORIO PADRE SI NO ESTAMOS EN RAÍZ
              // if (parentDir != "/") {
              //   client.println("{\"name\":\".. (Parent
              //   Directory)\",\"type\":\"directory\",\"index\":-1}");
              //   firstItem = false;
              // }

              // ✅ PASO 1: LEER DIRECTORIOS DESDE _files.lst
              if (SD_MMC.exists(filesListPath.c_str())) {
                File filesListFile =
                    SD_MMC.open(filesListPath.c_str(), FILE_READ);
                if (filesListFile) {
                  logln("Reading directories from _files.lst");

                  while (filesListFile.available()) {
                    String line = filesListFile.readStringUntil('\n');
                    line.trim();
                    if (line.length() == 0)
                      continue;

                    // Parsear línea: indice|tipo|tamaño|nombre|
                    int idx1 = line.indexOf('|');
                    int idx2 = line.indexOf('|', idx1 + 1);
                    int idx3 = line.indexOf('|', idx2 + 1);
                    int idx4 = line.indexOf('|', idx3 + 1);

                    if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1)
                      continue;

                    String tipo = line.substring(idx1 + 1, idx2);
                    String nombre = line.substring(idx3 + 1, idx4);

                    tipo.trim();
                    nombre.trim();

                    // ✅ SOLO DIRECTORIOS
                    if (tipo == "D") {
                      if (!firstItem)
                        client.println(",");
                      client.println("{\"name\":\"" + nombre +
                                     "\",\"type\":\"directory\",\"index\":" +
                                     String(itemIndex) + "}");
                      firstItem = false;
                      itemIndex++;
                      logln("Added directory from _files.lst: " + nombre);
                    }
                  }
                  filesListFile.close();
                } else {
                  logln("Could not open _files.lst file");
                }
              }

              // ✅ PASO 2: LEER ARCHIVOS DE AUDIO DESDE idx.txt
              if (SD_MMC.exists(idxFilePath.c_str())) {
                File idxFile = SD_MMC.open(idxFilePath.c_str(), FILE_READ);
                if (idxFile) {
                  logln("Reading audio files from idx.txt");

                  while (idxFile.available()) {
                    String line = idxFile.readStringUntil('\n');
                    line.trim();
                    if (line.length() == 0)
                      continue;

                    // ✅ idx.txt contiene rutas completas, extraer nombre del
                    // archivo
                    String fileName = line;
                    int lastSlash = fileName.lastIndexOf('/');
                    if (lastSlash != -1) {
                      fileName = fileName.substring(lastSlash + 1);
                    }

                    // ✅ Verificar que es un archivo de audio soportado
                    String fileNameUpper = fileName;
                    fileNameUpper.toUpperCase();

                    if (fileNameUpper.endsWith(".WAV") ||
                        fileNameUpper.endsWith(".MP3") ||
                        fileNameUpper.endsWith(".FLAC") ||
                        fileNameUpper.endsWith(".RADIO")) {

                      if (!firstItem)
                        client.println(",");
                      client.println("{\"name\":\"" + fileName +
                                     "\",\"type\":\"file\",\"index\":" +
                                     String(itemIndex) + "}");
                      firstItem = false;
                      itemIndex++;
#ifdef DEBUG
                      logln("Added audio file from idx.txt: " + fileName);
#endif
                    }
                  }
                  idxFile.close();
                } else {
                  logln("Could not open idx.txt file");
                }
              }

              // ✅ PASO 3: BUSCAR ARCHIVOS .RADIO SOLO SI ESTAMOS EN /RADIO
              if (parentDir == "/RADIO/" || parentDir == "/RADIO") {
                logln("Searching for .RADIO files in /RADIO directory");

                // Buscar archivos .RADIO directamente en el directorio /RADIO
                File radioDir = SD_MMC.open("/RADIO");
                if (radioDir && radioDir.isDirectory()) {
                  File radioFile = radioDir.openNextFile();

                  while (radioFile) {
                    if (!radioFile.isDirectory()) {
                      String fileName = radioFile.name();
                      String fileNameUpper = fileName;
                      fileNameUpper.toUpperCase();

                      if (fileNameUpper.endsWith(".RADIO")) {
                        // Verificar que no esté ya incluido en idx.txt
                        bool alreadyIncluded = false;

                        // Búsqueda simple para evitar duplicados
                        // (Solo verificamos si ya se procesó desde idx.txt)

                        if (!alreadyIncluded) {
                          if (!firstItem)
                            client.println(",");
                          client.println("{\"name\":\"" + fileName +
                                         "\",\"type\":\"file\",\"index\":" +
                                         String(itemIndex) + "}");
                          firstItem = false;
                          itemIndex++;
                          logln("Added .RADIO file from direct /RADIO scan: " +
                                fileName);
                        }
                      }
                    }
                    radioFile = radioDir.openNextFile();
                  }
                  radioDir.close();
                } else {
                  logln("Could not open /RADIO directory");
                }
              } else {
                // ✅ SOLO LOG DE DEBUG SI NO ESTAMOS EN /RADIO
                logln("Not in /RADIO directory, skipping .RADIO file search");
              }

              client.println("]}");
              logln("File list response completed with " + String(itemIndex) +
                    " items");
              break;
            }

            // ✅ ENDPOINT PARA CAMBIAR DIRECTORIO
            if (header.indexOf("GET /changedir/") >= 0) {
              client.println("HTTP/1.1 204 No Content");
              client.println("Connection: close");
              client.println();

              // Extraer nombre del directorio de la URL
              int startPos = header.indexOf("GET /changedir/") + 15;
              int endPos = header.indexOf(" HTTP/", startPos);
              if (endPos > startPos) {
                String selectedDir = header.substring(startPos, endPos);
                selectedDir.trim();

                // Decodificar URL
                selectedDir.replace("%20", " ");
                selectedDir.replace("%2F", "/");
                selectedDir.replace("%21", "!");
                selectedDir.replace("%22", "\"");
                selectedDir.replace("%23", "#");
                selectedDir.replace("%24", "$");
                selectedDir.replace("%25", "%");
                selectedDir.replace("%26", "&");
                selectedDir.replace("%27", "'");
                selectedDir.replace("%28", "(");
                selectedDir.replace("%29", ")");
                selectedDir.replace("%2A", "*");
                selectedDir.replace("%2B", "+");
                selectedDir.replace("%2C", ",");
                selectedDir.replace("%2D", "-");
                selectedDir.replace("%2E", ".");

                logln("Directory change requested: " + selectedDir);

                String newDir = "";

                if (selectedDir == ".. (Parent Directory)") {
                  // ✅ NAVEGAR AL DIRECTORIO PADRE
                  String currentDir = FILE_LAST_DIR;
                  if (currentDir.endsWith("/")) {
                    currentDir =
                        currentDir.substring(0, currentDir.length() - 1);
                  }

                  int lastSlash = currentDir.lastIndexOf('/');
                  if (lastSlash > 0) {
                    newDir = currentDir.substring(0, lastSlash);
                  } else {
                    newDir = "/";
                  }
                } else {
                  // ✅ NAVEGAR AL SUBDIRECTORIO
                  String currentDir = FILE_LAST_DIR;
                  if (!currentDir.endsWith("/"))
                    currentDir += "/";
                  newDir = currentDir + selectedDir;
                }

                // ✅ VERIFICAR QUE EL DIRECTORIO EXISTE
                if (SD_MMC.exists(newDir.c_str())) {
                  File testDir = SD_MMC.open(newDir.c_str());
                  if (testDir && testDir.isDirectory()) {
                    FILE_LAST_DIR = newDir;
                    logln("Directory changed to: " + FILE_LAST_DIR);

                    // ✅ GENERAR NUEVO idx.txt PARA EL DIRECTORIO
                    // generateListFile(FILE_LAST_DIR);

                    testDir.close();
                  } else {
                    logln("Invalid directory: " + newDir);
                    if (testDir)
                      testDir.close();
                  }
                } else {
                  logln("Directory does not exist: " + newDir);
                }
              }

              break;
            }

            // ✅ ENDPOINT PARA SELECCIONAR ARCHIVO
            if (header.indexOf("GET /select/") >= 0) {
              WAS_LAUNCHED = false;

              client.println("HTTP/1.1 204 No Content");
              client.println("Connection: close");
              client.println();

              // ✅ EXTRAER PARÁMETRO DE POSICIÓN PRIMERO
              int position = -1;
              int posParam = header.indexOf("?pos=");
              if (posParam != -1) {
                int posEnd = header.indexOf(" HTTP/", posParam);
                if (posEnd > posParam) {
                  String posStr = header.substring(posParam + 5, posEnd);
                  position = posStr.toInt();
                  logln("Position received: " + String(position));
                }
              }

              // ✅ EXTRAER NOMBRE DEL ARCHIVO (SIN EL PARÁMETRO ?pos=)
              int startPos = header.indexOf("GET /select/") + 12;
              int endPos = posParam != -1 ? posParam
                                          : header.indexOf(" HTTP/", startPos);
              if (endPos > startPos) {
                String selectedFile = header.substring(startPos, endPos);
                selectedFile.trim();

                // Decodificar URL (reemplazar %20 con espacios, etc.)
                selectedFile.replace("%20", " ");
                selectedFile.replace("%21", "!");
                selectedFile.replace("%22", "\"");
                selectedFile.replace("%23", "#");
                selectedFile.replace("%24", "$");
                selectedFile.replace("%25", "%");
                selectedFile.replace("%26", "&");
                selectedFile.replace("%27", "'");
                selectedFile.replace("%28", "(");
                selectedFile.replace("%29", ")");
                selectedFile.replace("%2A", "*");
                selectedFile.replace("%2B", "+");
                selectedFile.replace("%2C", ",");
                selectedFile.replace("%2D", "-");
                selectedFile.replace("%2E", ".");
                selectedFile.replace("%2F", "/");

                logln("File selected from web: " + selectedFile +
                      " at position: " + String(position));

                // ✅ VERIFICAR QUE ES UN ARCHIVO DE AUDIO SOPORTADO
                String fileUpper = selectedFile;
                fileUpper.toUpperCase();

                if (fileUpper.endsWith(".WAV") || fileUpper.endsWith(".MP3") ||
                    fileUpper.endsWith(".FLAC") ||
                    fileUpper.endsWith(".RADIO")) {

                  if (fileUpper.endsWith(".MP3"))
                    TYPE_FILE_LOAD = "MP3";
                  else if (fileUpper.endsWith(".WAV"))
                    TYPE_FILE_LOAD = "WAV";
                  else if (fileUpper.endsWith(".FLAC"))
                    TYPE_FILE_LOAD = "FLAC";
                  else if (fileUpper.endsWith(".RADIO"))
                    TYPE_FILE_LOAD = "RADIO";

                  // ✅ INTERRUMPIR PLAYERS ACTIVOS ANTES DE CARGAR EL NUEVO
                  // ARCHIVO Interrumpir MediaPlayer
                  logln("Interrupting MediaPlayer for new file selection");

                  // Establecer el archivo seleccionado
                  String fullPath = FILE_LAST_DIR;
                  if (!fullPath.endsWith("/"))
                    fullPath += "/";
                  fullPath += selectedFile;

                  PATH_FILE_TO_LOAD = fullPath;
                  FILE_POS_REMOTE_CONTROL = position;
                  FILE_LOAD = selectedFile;

                  hmi.writeString("page tape");

                  // Nos aseguramos que se cierra la reproducción actual
                  delay(250);
                  EJECT = true;
                  delay(250);
                  EJECT = true;
                  // Lanzamos el medio que interactua en paralelo con el
                  // tapeControl()
                  readyPlayFile();

                } else {
                  LAST_MESSAGE = "Unsupported file type.";
                  logln("Unsupported file type: " + selectedFile);
                }
              }

              break;
            }

            // ✅ ENDPOINT /status PARA ACTUALIZACIÓN
            if (header.indexOf("GET /status") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();

              String currentTrack = "";
              if (FILE_LOAD.length() > 0) {
                currentTrack = FILE_LOAD;
              } else if (PROGRAM_NAME.length() > 0) {
                currentTrack = PROGRAM_NAME;
              } else {
                currentTrack = "No track loaded";
              }

              client.println(currentTrack);
              client.println();
              break;
            }

            // ✅ NUEVO ENDPOINT PARA IR A HOME
            if (header.indexOf("GET /gohome") >= 0) {
              client.println("HTTP/1.1 204 No Content");
              client.println("Connection: close");
              client.println();

              logln("HOME directory requested");

              // ✅ CAMBIAR AL DIRECTORIO RAÍZ
              FILE_LAST_DIR = "/";
              logln("Changed to HOME directory: " + FILE_LAST_DIR);

              break;
            }

            // ✅ MEJORAR EL ENDPOINT PARA CAMBIAR DIRECTORIO
            if (header.indexOf("GET /changedir/") >= 0) {
              client.println("HTTP/1.1 204 No Content");
              client.println("Connection: close");
              client.println();

              // Extraer nombre del directorio de la URL
              int startPos = header.indexOf("GET /changedir/") + 15;
              int endPos = header.indexOf(" HTTP/", startPos);
              if (endPos > startPos) {
                String selectedDir = header.substring(startPos, endPos);
                selectedDir.trim();

                // Decodificar URL
                selectedDir.replace("%20", " ");
                selectedDir.replace("%2F", "/");
                selectedDir.replace("%21", "!");
                selectedDir.replace("%22", "\"");
                selectedDir.replace("%23", "#");
                selectedDir.replace("%24", "$");
                selectedDir.replace("%25", "%");
                selectedDir.replace("%26", "&");
                selectedDir.replace("%27", "'");
                selectedDir.replace("%28", "(");
                selectedDir.replace("%29", ")");
                selectedDir.replace("%2A", "*");
                selectedDir.replace("%2B", "+");
                selectedDir.replace("%2C", ",");
                selectedDir.replace("%2D", "-");
                selectedDir.replace("%2E", ".");

                logln("Directory change requested: " + selectedDir);

                String newDir = "";

                // ✅ MANEJAR NAVEGACIÓN ESPECIAL
                if (selectedDir == ".." ||
                    selectedDir == ".. (Parent Directory)") {
                  // ✅ NAVEGAR AL DIRECTORIO PADRE
                  String currentDir = FILE_LAST_DIR;
                  if (currentDir.endsWith("/")) {
                    currentDir =
                        currentDir.substring(0, currentDir.length() - 1);
                  }

                  int lastSlash = currentDir.lastIndexOf('/');
                  if (lastSlash > 0) {
                    newDir = currentDir.substring(0, lastSlash);
                  } else {
                    newDir = "/";
                  }
                } else {
                  // ✅ NAVEGAR AL SUBDIRECTORIO
                  String currentDir = FILE_LAST_DIR;
                  if (!currentDir.endsWith("/"))
                    currentDir += "/";
                  newDir = currentDir + selectedDir;
                }

                // ✅ VERIFICAR QUE EL DIRECTORIO EXISTE
                if (SD_MMC.exists(newDir.c_str())) {
                  File testDir = SD_MMC.open(newDir.c_str());
                  if (testDir && testDir.isDirectory()) {
                    FILE_LAST_DIR = newDir;
                    logln("Directory changed to: " + FILE_LAST_DIR);
                    testDir.close();
                  } else {
                    logln("Invalid directory: " + newDir);
                    if (testDir)
                      testDir.close();
                  }
                } else {
                  logln("Directory does not exist: " + newDir);
                }
              }

              break;
            }

            // ✅ NUEVO ENDPOINT PARA ESTADO PLAY/STOP
            if (header.indexOf("GET /playstatus") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/plain");
              client.println("Connection: close");
              client.println();

              // Devolver el estado actual
              if (PLAY) {
                client.println("STOP"); // Si está reproduciendo, mostrar STOP
              } else {
                client.println("PLAY"); // Si está parado, mostrar PLAY
              }
              client.println();
              break;
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // ✅ OTROS COMANDOS
            if (header.indexOf("GET /PLAY") >= 0) {
              if (!PLAY) {
                hmi.verifyCommand("PLAY");
              } else {
                hmi.verifyCommand("STOP");
              }
            } else if (header.indexOf("GET /STOP") >= 0) {
              hmi.verifyCommand("STOP");
            } else if (header.indexOf("GET /PAUSE") >= 0) {
              hmi.verifyCommand("PAUSE");
            } else if (header.indexOf("GET /NEXT") >= 0) {
              CMD_FROM_REMOTE_CONTROL = true;
              hmi.verifyCommand("FFWD");
            } else if (header.indexOf("GET /PREV") >= 0) {
              CMD_FROM_REMOTE_CONTROL = true;
              hmi.verifyCommand("RWD");
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" "
                           "content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            // JavaScript para actualizar el textbox y manejar la lista de
            // archivos
            jsUpdateTextBoxWithFileList(client);

            // CSS styles
            client.println("<style>html { font-family: Helvetica;display: "
                           "inline-block;margin: 0px auto;text-align: "
                           "center;background-color: black;color: #e6e5e2ff;}");
            client.println("h1 { color: #ffbf00; }");
            client.println("h2 { color: #888888; }");

            // CSS para centrar elementos
            client.println("body {");
            client.println("display: flex;");
            client.println("flex-direction: column;");
            client.println("align-items: center;");
            client.println("justify-content: center;");
            client.println("min-height: 100vh;");
            client.println("margin: 0;");
            client.println("padding: 20px;");
            client.println("box-sizing: border-box;");
            client.println("}");

            client.println(".page-container {");
            client.println("display: flex;");
            client.println("flex-direction: column;");
            client.println("align-items: center;");
            client.println("justify-content: center;");
            client.println("max-width: 800px;");
            client.println("width: 100%;");
            client.println("margin: 0 auto;");
            client.println("}");

            client.println(".header-section {");
            client.println("text-align: center;");
            client.println("margin-bottom: 30px;");
            client.println("}");

            client.println(".control-section {");
            client.println("display: flex;");
            client.println("flex-direction: column;");
            client.println("align-items: center;");
            client.println("gap: 30px;");
            client.println("}");

            // ✅ CSS para el botón "..." y el listbox
            client.println(".textbox-container {");
            client.println("display: flex;");
            client.println("align-items: center;");
            client.println("gap: 10px;");
            client.println("width: 100%;");
            client.println("max-width: 400px;");
            client.println("}");

            client.println(".browse-button {");
            client.println("background-color: #555;");
            client.println("color: #fff;");
            client.println("border: 1px solid #777;");
            client.println("border-radius: 4px;");
            client.println("padding: 12px 16px;");
            client.println("cursor: pointer;");
            client.println("font-size: 16px;");
            client.println("min-width: 50px;");
            client.println("text-align: center;");
            client.println("}");

            client.println(".browse-button:hover {");
            client.println("background-color: #666;");
            client.println("border-color: #ffbf00;");
            client.println("}");

            client.println(".file-list {");
            client.println("display: none;");
            client.println("position: absolute;");
            client.println("top: 100%;");
            client.println("left: 0;");
            client.println("width: 100%;");
            client.println("max-height: 300px;");
            client.println("overflow-y: auto;");
            client.println("background-color: #333;");
            client.println("border: 1px solid #555;");
            client.println("border-radius: 4px;");
            client.println("z-index: 1000;");
            client.println("margin-top: 5px;");
            client.println("}");

            client.println(".file-list.show {");
            client.println("display: block;");
            client.println("}");

            client.println(".file-item {");
            client.println("padding: 10px 15px;");
            client.println("cursor: pointer;");
            client.println("border-bottom: 1px solid #555;");
            client.println("color: #fff;");
            client.println("font-size: 14px;");
            client.println(
                "text-align: left;"); // ✅ SOLO ALINEACIÓN A LA IZQUIERDA
            client.println("}");

            client.println(".file-item:hover {");
            client.println("background-color: #444;");
            client.println("color: #ffbf00;");
            client.println("}");

            client.println(".file-item:last-child {");
            client.println("border-bottom: none;");
            client.println("}");

            // ✅ CSS adicional para directorios
            client.println(".file-item.directory {");
            client.println("background-color: #2a2a2a;");
            client.println("border-left: 3px solid #ffbf00;");
            client.println("text-align: left;"); // ✅ ALINEACIÓN A LA IZQUIERDA
                                                 // PARA DIRECTORIOS
            client.println("}");

            client.println(".file-item.file {");
            client.println("border-left: 3px solid #555;");
            client.println("text-align: left;"); // ✅ ALINEACIÓN A LA IZQUIERDA
                                                 // PARA ARCHIVOS
            client.println("}");

            client.println(".file-item.directory:hover {");
            client.println("background-color: #3a3a3a;");
            client.println("border-left-color: #fff;");
            client.println("}");

            // CSS para el footer
            client.println(".footer {");
            client.println("position: fixed;");
            client.println("bottom: 10px;");
            client.println("left: 50%;");
            client.println("transform: translateX(-50%);");
            client.println("text-align: center;");
            client.println("font-size: 12px;");
            client.println("color: #666;");
            client.println("font-family: Arial, sans-serif;");
            client.println("}");

            client.println(".footer a {");
            client.println("color: #fff;");
            client.println("text-decoration: none;");
            client.println("transition: color 0.3s ease;");
            client.println("}");

            client.println(".footer a:hover {");
            client.println("color: #ffbf00;");
            client.println("text-decoration: underline;");
            client.println("}");

            // CSS del control circular y textbox
            remoteControlStyle(client);
            textBoxcss(client);

            client.println("</style></head>");

            // Estructura HTML
            client.println("<body>");
            client.println("<div class=\"page-container\">");

            // Sección de encabezado
            client.println("<div class=\"header-section\">");
            client.println("<h1>PowaDCR " + String(VERSION) + "</h1>");
            client.println("<h2>Remote control</h2>");
            client.println("</div>");

            // Sección de controles
            client.println("<div class=\"control-section\">");

            // ✅ TEXTBOX CON BOTÓN BROWSE
            client.println("<div class=\"circular-control-container\">");
            client.println("<div class=\"Wrapper\">");
            client.println(
                "<div class=\"Input\" style=\"position: relative;\">");

            client.println("<div class=\"textbox-container\">");

            String currentTrack = "";
            if (FILE_LOAD.length() > 0) {
              currentTrack = FILE_LOAD;
            } else if (PROGRAM_NAME.length() > 0) {
              currentTrack = PROGRAM_NAME;
            } else {
              currentTrack = "No track loaded";
            }

            client.println(
                "<input class=\"Input-text\" type=\"text\" value=\"" +
                currentTrack + "\" readonly style=\"flex: 1;\">");
            client.println("<button class=\"browse-button\" "
                           "id=\"browseBtn\">...</button>");
            client.println("</div>");

            // Lista de archivos (inicialmente oculta)
            client.println("<div class=\"file-list\" id=\"fileList\">");
            client.println("<div class=\"file-item\">Loading files...</div>");
            client.println("</div>");

            client.println("</div>");
            client.println("</div>");
            client.println("</div>");

            // Control circular
            client.println("<div class=\"circular-control-container\">");
            client.println("<nav>");
            client.println("<a class=\"button top\" href=\"/VOLUP\">");
            client.println("<span class=\"symbol volume\">VOL+</span>");
            client.println("</a>");
            client.println("<a class=\"button right\" href=\"/NEXT\">");
            client.println("<span class=\"symbol track\">NEXT</span>");
            client.println("</a>");
            client.println("<a class=\"button left\" href=\"/PREV\">");
            client.println("<span class=\"symbol track\">PREV</span>");
            client.println("</a>");
            client.println("<a class=\"button bottom\" href=\"/VOLDOWN\">");
            client.println("<span class=\"symbol volume\">VOL-</span>");
            client.println("</a>");
            client.println("<a class=\"center-button\" href=\"/PLAY\">");

            if (PLAY) {
              client.println("<span class=\"symbol play-stop\">STOP</span>");
            } else {
              client.println("<span class=\"symbol play-stop\">PLAY</span>");
            }
            client.println("</a>");
            client.println("</nav>");
            client.println("</div>");

            client.println("</div>"); // Fin control-section
            client.println("</div>"); // Fin page-container

            // Footer
            client.println("<div class=\"footer\">");
            client.println("<a href=\"https://github.com/hash6iron/powadcr\" target=\"_blank\">Design by Hash6iron</a>");
            client.println("</div>");
            client.println("</body></html>");

            client.println();
            break;

          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }
}

void buttonsControl()
{
  // --------------------------------------------------------------------------------------
  // Control de la botonera conectada al MCP23017
  // --------------------------------------------------------------------------------------

  const int timeToLongPress = 28;             // Número de ciclos para considerar una pulsación larga
  
  uint8_t value = MCP23017_readGPIO(0x12);

  value = ~value;                               // Invertimos los bits porque el hardware devuelve 0 para tecla presionada
  value = value & 0x7F;                         // Eliminamos los bits de salida (aplico mascara 00111111)
  
  uint8_t keyPressed = 0;
  
  // Determinamos si hay tecla pulsada o no.
  if (value==0)
  {
    keyPressed = 8;
  }
  else
  {
    keyPressed = log2(value);                     // Obtenemos el índice del bit a 1, que corresponde a la tecla presionada (0-5)
  }
  
  // Control de estado botones.
  switch (keyStatus)
  {
    case 0:
    {
      // Estado inicial. Esperamos la pulsación de una tecla.
      if (keyPressed < 8)
      {
        keykp_count[keyPressed]=0;
        keykp_count[keyPressed]++;
        keyStatus = 1;
      }
      else
      {
        keyStatus = 0;          
      }
    }
    break;
  
    case 1:
    {
      // Determinamos si es una pulsación corta o larga.
      // para ello vamos contando el número de ciclos que se mantiene pulsada la tecla.
      if (keyPressed < 8)
      {
        keykp_count[keyPressed]++;
        // Si pulsamos sin soltar es larga
        if (keykp_count[keyPressed] > timeToLongPress)
        {
          // Long pressed
          keyStatus = 3;
        }
        lastKeyValue = keyPressed;
      }      
      else
      {
        // Short pressed
        // Si pulsamos y hemos soltado antes de ser larga
        if (keykp_count[lastKeyValue] > 0)
        {
          keyStatus = 2;
          logln("SHORT!! Detected");
        }
      }
    }
    break;

    case 2:
    {
      // Short pressed

      switch (lastKeyValue)
      {
        case 8:
          {
            logln("Unknow key - Short press");
            keyStatus = 5;       
          }
          break;
        
        case MCP_KEY_PLAY:
          {
            logln("PLAY/STOP button pressed - Value: " + String(keyPressed));
            hmi.verifyCommand("PLAY");   
            keyStatus = 5;       
          }
          break;
        
        case MCP_KEY_STOP:
          {
            hmi.verifyCommand("STOP");
            logln("STOP button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        case MCP_KEY_PAUSE:
          {
            hmi.verifyCommand("PAUSE");
            logln("PAUSE button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        case MCP_KEY_FFWD:
          {
            hmi.verifyCommand("FFWD");
            logln("FFWD button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        case MCP_KEY_RWD:
          {
            hmi.verifyCommand("RWD");
            logln("RWD button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        case MCP_KEY_REC:
          {
            hmi.verifyCommand("REC");
            logln("REC button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        case MCP_KEY_EJECT:
          {
            hmi.verifyCommand("EJECT");
            logln("EJECT button pressed - Value: " + String(keyPressed));
            keyStatus = 5;       
          }
          break;

        default:
            logln("Unknow key pressed");
            keyStatus = 0;       
          break;
      }    
    }
    break;

    case 3:
    {
      // Long pressed
      switch (keyPressed)
      {
        case 8:
          // Release button, do nothing
          {
            logln("Key released: " + String(keyPressed) + " - Value: " + String(keyPressed));
            if (KEEP_FFWIND || KEEP_RWIND)
            {
              KEEP_FFWIND = false;
              KEEP_RWIND = false;
              // Esto lo hacemos para salir del modo fast wind
              hmi.verifyCommand("FFWD");
            }
            // Cambiamos de estado
            keyStatus = 0;
            //
          }
          break;
        
        case MCP_KEY_PLAY:
          {
            logln("PLAY/STOP button LONG pressed");
          }
          break;
        
        case MCP_KEY_STOP:
          {
            logln("STOP button LONG pressed");
          }
          break;

        case MCP_KEY_PAUSE:
          {
            logln("PAUSE button LONG pressed");
          }
          break;
        
        case MCP_KEY_RWD:
          {
            hmi.verifyCommand("TTWD");
            logln("RWD button LONG pressed");
          }
          break;

        case MCP_KEY_FFWD:
          {
            hmi.verifyCommand("SFWD");
            logln("FFWD button LONG pressed");
          }
          break;

        case MCP_KEY_REC:
          {
            logln("REC button LONG pressed");
          }
          break;
          
        case MCP_KEY_EJECT:
          {
            hmi.writeString("page mainmenu");
            logln("EJECT button LONG pressed");
            // Forzamos salida para no repetir mas el comando
            keyStatus = 5;
          }
          break;

        default:
          keyStatus = 0;
          break;
      }          
    }
    break;

    case 5:
    {
      // Esperamos soltar el botón
      if (keyPressed == 8)
      {
        keyStatus = 0;
        logln("Key released");      
      }
    }
    break;

    default:
    break;
  }

}

// ******************************************************************
//
//            Gestión de nucleos del procesador
//
// ******************************************************************

void Task1code(void *pvParameters) {
  // setup();

  for (;;) 
  {
    if (serialEventRun) serialEventRun();
    tapeControl();
  }
}

// Task0code se puede liberar desde el config, para usar un solo CORE
void Task0code(void *pvParameters) {

  // const int windowNameLength = 44;
  // esp_task_wdt_reset();

#ifndef SAMPLINGTEST
  // Core 0 - Para el HMI
  int startTime = millis();
  int startTime2 = millis();
  int startTime3 = millis();
  int startTime4 = millis();
  int startTime5 = millis();
  int startTimeKey = millis();
  int startTimeSpotify = millis();

  int tClock = millis();
  int ho = 0;
  int mi = 0;
  int se = 0;
  int tScrRfsh = 1000;
  int timeRTC = 1000;
  int timeKeyPoll = 250;

  uint8_t lasthour = 0;
  uint8_t lastminute = 0;
  uint8_t lastsecond = 0;
  String lastTimeMessage = "";
  bool lastRadioIsPlaying = false;
  bool lastMusicIsPlaying = false;

  // int tRotateNameRfsh = 200;
  // ✅ AÑADE UN TEMPORIZADOR PARA MEDIR LA PILA
  unsigned long stackCheckTime = millis();
  for (;;) 
  {

    // Leemos cada 125ms
    // if ((millis() - startTime5) > 25) 
    // {
    hmi.readUART();
    //   startTime5 = millis();
    // }

    // Mini consola de debug por Serial (UART0) para pruebas
    if (Serial.available())
    {
      String dbgCmd = Serial.readStringUntil('\n');
      dbgCmd.trim();
      if (dbgCmd == "cpcdb_update")
      {
        logln("[DBG] Updating entire CPCDB catalogue...");
        updateCPCDB("0");
      }
      else if (dbgCmd.startsWith("cpcdb_update "))
      {
        String letter = dbgCmd.substring(13);
        letter.trim();
        logln("[DBG] Updating CPCDB for letter: " + letter);
        updateCPCDB(letter);
      }
      else if (dbgCmd.startsWith("cpcdb_download "))
      {
        // Ejemplo: cpcdb_download Ace (E).cdt
        String cdtName = dbgCmd.substring(15);
        cdtName.trim();
        String title = cdtName.substring(0, cdtName.length() - 4);
        logln("[DBG] Downloading CPC CDT: " + cdtName);
        downloadFromCPCDB(cdtName, title);
      }
      else if (dbgCmd == "msxdb_update")
      {
        logln("[DBG] Updating entire MSXDB catalogue...");
        updateMSXDB("0");
      }
      else if (dbgCmd.startsWith("msxdb_update "))
      {
        String letter = dbgCmd.substring(13);
        letter.trim();
        logln("[DBG] Updating MSXDB for letter: " + letter);
        updateMSXDB(letter);
      }
      else if (dbgCmd.startsWith("msxdb_download "))
      {
        // Ejemplo: msxdb_download Zakil Wood (1985)(Mr Micro)(ES)[!][BLOAD'CAS-',R][v0.8.5b].tsx
        String tsxName = dbgCmd.substring(15);
        tsxName.trim();
        String title = tsxName;
        int parenPos = title.indexOf(" (");
        if (parenPos > 0) title = title.substring(0, parenPos);
        logln("[DBG] Downloading MSX TSX: " + tsxName);
        downloadFromMSXDB(tsxName, title);
      }
      else if (dbgCmd == "zxdb_update")
      {
        logln("[DBG] Updating entire ZXDB catalogue...");
        updateZXDB("0");
      }
    }

    delay(25);

    // Control del FTP
    #ifdef FTP_SERVER_ENABLE
      if (!IRADIO_EN && WIFI_ENABLE && WIFI_CONNECTED && !FLAC_IS_PLAYING && !DOWNLOADING_ZXDB && !DOWNLOADING_CPCDB && !DOWNLOADING_MSXDB)
      {
        ftpSrv.handleFTP();
      }
    #endif

    #ifdef WEB_SERVER_ENABLE
    if (!FLAC_IS_PLAYING && WIFI_ENABLE && WIFI_CONNECTED && !DOWNLOADING_ZXDB && !DOWNLOADING_CPCDB && !DOWNLOADING_MSXDB)
    {
      WiFiClient client = server.available();
      if (client) 
      {
        handleWebClient(client);
      }
    }
    #endif

    #ifdef DEBUGMODE
        // Comprobamos la pila cada 10 segundos
        if (millis() - stackCheckTime > 5000) { // Cada 5 segundos
          UBaseType_t hwm_core0 = uxTaskGetStackHighWaterMark(Task0);
          UBaseType_t hwm_core1 = uxTaskGetStackHighWaterMark(Task1);

          logln("Stack HWM Core 0 (HMI): " + String(hwm_core0 * 4 / 1024) +
                " KB free");
          logln("Stack HWM Core 1 (Tape): " + String(hwm_core1 * 4 / 1024) +
                " KB free");

          stackCheckTime = millis();
        }
    #endif

    // Control por botones
    // Estos solo funcionan en la pagina TAPE0 o TAPE del HMI

    // Esto lo ponemos para evitar errores en la lectura del puerto serie
    // delay(25);

    // esp_task_wdt_reset();

    #ifndef DEBUGMODE

        if (REC) 
        {
          tScrRfsh = 250;
        }
        else 
        {
          tScrRfsh = 125;
        }

        if (POWER_LED_MODE==0)
        {
            // LED: encendido durante playback/loading/recording, apagado en standby
            if (PLAY || LOADING_STATE == 1 || LOADING_STATE == 4) {
              if (REC && !IRADIO_EN) {
                // Modo grabacion: parpadeo
                if ((millis() - startTime3) > 500) {
                  startTime3 = millis();
                  if (!powerLedFixed) {
                    statusPoweLed = !statusPoweLed;
                    actuatePowerLed(statusPoweLed);
                  } else {
                    actuatePowerLed(1);
                  }
                }
              } else {
                // Playback/loading: LED fijo encendido
                actuatePowerLed(1);
              }
            } else {
              // Standby: LED apagado
              actuatePowerLed(0);
            }
        }
        else
        {
            // Lo dejamos fijo a low power
            if (!REC) 
            {
              actuatePowerLed(1);
            }

            if ((millis() - startTime3) > 500) 
            {
              // Timer para el powerLed / recording led indicator
              startTime3 = millis();

              if (REC && !IRADIO_EN) 
              {
                // Modo grabacion
                if (!powerLedFixed) 
                {
                  statusPoweLed = !statusPoweLed;
                  actuatePowerLed(statusPoweLed);
                }
                else if (!IRADIO_EN) 
                {
                  actuatePowerLed(1);
                }
              }
          }
      }

      // Actualizamos el RTC
      if ((millis() - startTime4 > timeRTC) && NTP_AVAILABLE) 
      {          
        if (CURRENT_PAGE == 99) 
        {

          char buf[4];
          uint8_t hour = rtc.getHour();
          uint8_t minute = rtc.getMinute();
          uint8_t second = rtc.getSecond();
          String ampm = rtc.getAmPm(false);

          
          
          //logln("RTC Time: " + String(hour) + ":" + String(minute) + ":" + String(second) + " " + ampm);

          // 
          if (ampm=="PM")
          {
              if (hour < 12) hour = hour + 12; // Convertir a formato 12 horas
          }
          else if (ampm=="AM")
          {
              if (hour == 12) hour = 0; // Ajustar medianoche
          }

          //logln("RTC Time (24h format): " + String(hour) + ":" + String(minute) + ":" + String(second));

          // Pintamos en el HMI el reloj
          if (hour != lasthour)
          {
            snprintf(buf, sizeof(buf), "%02u", hour);
            myNex.writeStr("clock.timeH.txt", String(buf));
            lasthour = hour;
          }

          if (minute != lastminute)
          {
            snprintf(buf, sizeof(buf), "%02u", minute);
            myNex.writeStr("clock.timeM.txt", String(buf));
            lastminute = minute;
          }

          if (second != lastsecond && !FLAC_IS_PLAYING)
          {
            snprintf(buf, sizeof(buf), "%02u", second);
            myNex.writeStr("clock.timeS.txt", String(buf));

            lastsecond = second;
          }

          // Fecha
          if (lastTimeMessage != rtc.getDate(true))
          {
            lastTimeMessage = rtc.getDate(true);
            myNex.writeStr("clock.timeMessage.txt", rtc.getDate(true));
          }

          if (MUSIC_IS_PLAYING != lastMusicIsPlaying)
          {
            lastMusicIsPlaying = MUSIC_IS_PLAYING;
            if (MUSIC_IS_PLAYING)
            {
              //Encendido
              myNex.writeNum("clock.m3.pco", CLOCK_ENABLE_INDICATOR_COLOR);
              myNex.writeNum("clock.t3.pco", CLOCK_DISABLE_INDICATOR_COLOR);
            }
            else
            {
              //Apagado
              myNex.writeNum("clock.m3.pco", CLOCK_DISABLE_INDICATOR_COLOR);

            }
          }

          if (RADIO_IS_PLAYING != lastRadioIsPlaying)
          {
            lastRadioIsPlaying = RADIO_IS_PLAYING;
            if (RADIO_IS_PLAYING)
            {
              //Encendido
              myNex.writeNum("clock.t3.pco", CLOCK_ENABLE_INDICATOR_COLOR);
              myNex.writeNum("clock.m3.pco", CLOCK_DISABLE_INDICATOR_COLOR);
            }
            else
            {
              //Apagado
              myNex.writeNum("clock.t3.pco", CLOCK_DISABLE_INDICATOR_COLOR);
            }
          }            
        }  
        else
        {
          lasthour = 0;
          lastminute = 0;
          lastsecond = 0;
          lastTimeMessage = "";
          lastRadioIsPlaying = false;
          lastMusicIsPlaying = false;
          //
          //hmi.writeString("page tape");
        }                              

        startTime4 = millis();           
      }
          
      // Actualizamos el estado de la memoria y los cores
      if ((millis() - startTime) > tScrRfsh) {
        startTime = millis();
        stackFreeCore1 = uxTaskGetStackHighWaterMark(Task1);
        stackFreeCore0 = uxTaskGetStackHighWaterMark(Task0);
        hmi.updateInformationMainPage();
      }

      // Hacemos poll de la botonera
      if (millis() - startTimeKey > timeKeyPoll)
      {
        if (!FILE_BROWSER_OPEN && MCP23017_AVAILABLE) //(CURRENT_PAGE <= 1 || CURRENT_PAGE == 99) &&
        {
            buttonsControl();
        }          
      }        

      if (!FLAC_IS_PLAYING)
      {
        // Control de SpotiFy
        if (SPOTIFY_CONTROL)
        {
            if (millis() - startTimeSpotify > 2500) 
            {
              // El reloj
              if (CURRENT_PAGE == 99)
              {
                if (STOP)
                {
                  hmi.verifyCommand("SP03");
                }
              }

              // La pagina de Spotify
              if (CURRENT_PAGE == 6 && !ftpSrv.FTP_CONNECTED) 
              {
                  String currentArtist = sp->current_artist_names();
                  String currentTrackname = sp->current_track_name();
                  String currentAlbum = sp->current_album_name();
                                
                  logln("Artist: " + currentArtist);
                  myNex.writeStr("spotify.artist.txt",currentArtist);
                  logln("Track: " + currentTrackname);
                  myNex.writeStr("spotify.track.txt",currentTrackname);
                  logln("Album: " + currentAlbum);
                  myNex.writeStr("spotify.album.txt",currentAlbum);
              }
              startTimeSpotify = millis();
            }
        }

        if (rotate_enable || ENABLE_ROTATE_FILEBROWSER) 
        {
          if ((millis() - startTime2) > tRotateNameRfsh && (FILE_LOAD.length() > windowNameLength || ((ROTATE_FILENAME.length() > windowNameLengthFB)) * ENABLE_ROTATE_FILEBROWSER)) 
          {
            // Capturamos el texto con tamaño de la ventana
            if ((TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3" || TYPE_FILE_LOAD == "FLAC")) 
            {
              hmi.writeString("name.txt=\"" + FILE_LOAD.substring(posRotateName, posRotateName + windowNameLength) + "\"");
            }
            else 
            {
              if (!ENABLE_ROTATE_FILEBROWSER) 
              {
                PROGRAM_NAME = FILE_LOAD.substring( posRotateName, posRotateName + windowNameLength);
              }
              else 
              {
                hmi.writeString("file.path.txt=\"" + ROTATE_FILENAME.substring(posRotateName,posRotateName + windowNameLengthFB) + "\"");
              }
            }
            // Lo rotamos segun el sentido que toque
            posRotateName += moveDirection;
            // Comprobamos limites para ver si hay que cambiar sentido
            if (!ENABLE_ROTATE_FILEBROWSER) 
            {
              if (posRotateName > (FILE_LOAD.length() - windowNameLength)) 
              {
                moveDirection = -1;
              }

              if (posRotateName < 0) 
              {
                moveDirection = 1;
                posRotateName = 0;
              }
            } 
            else 
            {
              if (posRotateName > (ROTATE_FILENAME.length() - windowNameLengthFB)) 
              {
                moveDirection = -1;
              }

              if (posRotateName < 0) 
              {
                moveDirection = 1;
                posRotateName = 0;
              }
            }

            // Movemos el display de NAME
            startTime2 = millis();
          } 
          else if (FILE_LOAD.length() <= windowNameLength && (TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3" || TYPE_FILE_LOAD == "FLAC")) 
          {
            if (STOP) 
            {
              hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");
            }
            else 
            {
              if (TYPE_FILE_LOAD == "TAP") 
              {
                PROGRAM_NAME = myTAP.descriptor[BLOCK_SELECTED].name;
              }
              else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX") 
              {
                PROGRAM_NAME = myTZX.descriptor[BLOCK_SELECTED].name;
              } 
              else if (TYPE_FILE_LOAD == "PZX") 
              {
                // PROGRAM_NAME = myPZX.descriptor[BLOCK_SELECTED].name;
              }
              else 
              {
                // Para MP3 y WAV
                PROGRAM_NAME = FILE_LOAD;
              }
              //
              hmi.writeString("name.txt=\"" + PROGRAM_NAME + "\"");
            }
          }
        }
      }
        

    #endif
  }
#endif
}

void showOption(String id, String value) {
  myNex.writeNum(id, value.toInt());
  delay(10);
  myNex.writeNum(id, value.toInt());
  delay(10);
}

bool createSpecialDirectory(String fDir) {
  // Esto lo hacemos para ver si el directorio existe

  // Existe el directorio?
  if (!SD_MMC.open(fDir)) {
    // En ese caso. Saco la pantalla de pregunta.
    hmi.writeString("page create");

    int res = -1; // Valor inicial alto

    // Mientras no haya respuesta continuo ahí
    // el -1 se va a recibir como 77777 por eso res > 1
    while (res != 0 && res != 1) {
      hmi.writeString("create.statusLCD.txt=\"Create " + fDir + "?\"");
      res = myNex.readNumber("create.res.val");
      logln("Response for create dir " + fDir + ": " + String(res));
      delay(500);
    }

    // delay(5000);
    delay(500);

    hmi.writeString("page screen"); // Vuelvo a la pantalla principal

    if (res == 1) {
      if (!SD_MMC.mkdir(fDir)) {
#ifdef DEBUGMODE
        logln("");
        log("Error! Directory exists or wasn't created");
#endif
        return false;
      } else {
        return true;
      }
    }
  } else {
#ifdef DEBUGMODE
    logln("");
    log("Directory exists.");
#endif
    return false;
  }

  return false;
}

void testingAudioKit() 
{
  AudioInfo info(SAMPLING_RATE, 2, 16);
  SineWaveGenerator<int16_t> sineWave(SAMPLING_RATE);                // subclass of SoundGenerator with max amplitude of 32000
  GeneratedSoundStream<int16_t> sound(sineWave);             // Stream generated from sine wave
  StreamCopy copier(kitStream, sound);                             // copies sound into i2s

  kitStream.setVolume(0.5); // Set volume to 50%
  
  // Setup sine wave (1046.5f Hz)
  sineWave.begin(info, N_C6);

  int count = 0;

  count = 0;
  while (count < 24) 
  {
    copier.copy();
    count++;
  }
  //
  delay(125);

  count = 0;
  while (count < 24) 
  {
    copier.copy();
    count++;
  }
  //
  delay(125);

  kitStream.setVolume(MAIN_VOL / 100.0f); // Volumen general
}

void setupAudioKit() {
  // SDA, SCL
  // Wire.begin(33,32);            // default: 21, 22 - Audio board codec I2C
  // Wire1.begin(32,30);       // Extension board
  // // Wire1.setTimeOut(3000);  // 3 seconds timeout
  // logln("---------- Scanning Wire -------------");
  // I2C_ScannerWire();
  // logln("---------- Scanning Wire1 ------------");
  // I2C_ScannerWire1();
  logln("> Setting AUDIOKIT.");
  //hmi.writeString("statusLCD.txt=\"Setting audio board\"");
  //
  // slot SD
  powadcr_pins.addSPI(ESP32PinsSD);
  // add i2c codec pins: scl, sda, port, frequency
  powadcr_pins.addI2C(PinFunction::CODEC, 32, 33);
  // powadcr_pins.addI2C(PinFunction::CODEC,32,33,1,100000U, &Wire1,true); //
  // Audio board codec powadcr_pins.addI2C(PinFunction::EXPANDER, 18,
  // 23,1,100000U, Wire1,true); // Extension board codec

  // add i2s pins: mclk, bck, ws,data_out, data_in ,(port)
  powadcr_pins.addI2S(PinFunction::CODEC, 0, 27, 25, 26, 35);
  // add other pins: PA on gpio 21
  // powadcr_pins.addPin(PinFunction::PA, 21, PinLogic::Output);

  // Teclas
  // powadcr_pins.addPin(PinFunction::KEY, 36, PinLogic::InputActiveLow, 1);
  // powadcr_pins.addPin(PinFunction::KEY, 13, PinLogic::InputActiveLow, 2);
  // powadcr_pins.addPin(PinFunction::KEY, 19, PinLogic::InputActiveLow, 3);
  // powadcr_pins.addPin(PinFunction::KEY, 5, PinLogic::InputActiveLow, 6);
  // Deteccion de cable conectado al AMP
  powadcr_pins.addPin(PinFunction::AUXIN_DETECT, 12, PinLogic::InputActiveLow);
  // Deteccion de cable conectado al HEADPHONE
  powadcr_pins.addPin(PinFunction::HEADPHONE_DETECT, 39,
                      PinLogic::InputActiveLow);
  // Amplificador ENABLE pin
  powadcr_pins.addPin(PinFunction::PA, 21, PinLogic::Output);
  // Power led indicador
  // powadcr_pins.addPin(PinFunction::LED, 22, PinLogic::Output);

  //
  auto cfg = kitStream.defaultConfig(RXTX_MODE);
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.sample_rate = SAMPLING_RATE;
  // cfg.buffer_size = 262144;
  cfg.input_device = ADC_INPUT_LINE2;
  cfg.output_device = DAC_OUTPUT_ALL;
  // Esto es importante para el uso de SD_MMC
  cfg.sd_active = false;

  //
  if (!kitStream.begin(cfg)) 
  {
    logln("ERROR! Failed to initialize AUDIOKIT. Review wiring and I2C interfering.");
    while (1);
  }

  // Ajustamos el volumen de entrada
  kitStream.setInputVolume(IN_REC_VOL);

// Para que esta linea haga efecto debe estar el define = true
//
#if USE_AUDIO_LOGGING
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Debug);
#endif
}

bool setupMCP23017() {
  
  //                            
  // See: https://wolles-elektronikkiste.de/en/port-expander-mcp23017-2
  //

  logln("> Setting MCP23017.");

  if (!Wire1.begin(18, 23, 1700000U)) {

    logln("Error bus I2C for MCP23017.");
    MCP23017_AVAILABLE = false;
    saveHMIcfg("MCPAVAIL");

    Wire1.end();
    return false;

  } else {

    logln("MCP23017 OK.");

    MCP23017_AVAILABLE = true;
    saveHMIcfg("MCPAVAIL");
  
    // Puerto A
    Wire1.beginTransmission(I2C_MCP23017_ADDR);
      Wire1.write(0x00);  // Reg. IODIRA
      Wire1.write(0x7F);  // Configuramos GPIOA como entrada PA0..PA6 y salida pin PA7
    Wire1.endTransmission();    //
    
    // Puerto A pull-up
    Wire1.beginTransmission(I2C_MCP23017_ADDR);
      Wire1.write(0x0C);   // Reg. GPPUA
      Wire1.write(0x7F);   // Activamos pull-up en PA0 .. PA5
    Wire1.endTransmission();    //

    // Puerto B
    Wire1.beginTransmission(I2C_MCP23017_ADDR);
      Wire1.write(0x01); // Reg. IODIRB
      Wire1.write(0x00); // Configuramos el puerto B como salida
    Wire1.endTransmission();    //

    //
    // test with powerLed
    for (int i = 0; i < 10; i++) {
      MCP23017_writePin(MCP_LED_IO_PIN, LOW, I2C_MCP23017_ADDR);
      delay(80);
      MCP23017_writePin(MCP_LED_IO_PIN, HIGH, I2C_MCP23017_ADDR);
      delay(80);
    }

    return true;
  }
}

void setupWifi() {
  logln("> Setting WiFi.");
  // if (loadWifiCfgFile()) {
  //  Si la conexión es correcta se actualiza el estado del HMI
  if (wifiSetup()) {

    WIFI_CONNECTED = true;
    logln("Wifi OK");

    // Enviamos información al menu
    hmi.writeString("menu.wifissid.txt=\"" + String(ssid) + "\"");
    delay(125);
    hmi.writeString("menu.wifipass.txt=\"" + String(password) + "\"");
    delay(125);
    hmi.writeString("menu.wifiIP.txt=\"" + WiFi.localIP().toString() + "\"");
    delay(125);
    hmi.writeString("menu.wifiEn.val=1");
    delay(125);

    // FTP Server
    #ifdef FTP_SERVER_ENABLE
        ftpSrv.begin(&SD_MMC, "powa", "powa");
    #endif

    #ifdef WEB_SERVER_ENABLE
        server.begin();
    #endif

  } else {
    WIFI_CONNECTED = false;
    logln("Wifi FAILED");
    hmi.writeString("menu.wifiEn.val=0");
    delay(125);
  }

  if (!QUICK_BOOT) delay(750);
}

void setupNTP()
{
  logln("> Setting NTP Client.");
  // NTP Client
  hmi.writeString("statusLCD.txt=\"NTP and RTC settings\"");
  const char* ntpServer1 = NTPSERVER;
  const char* ntpServer2 = "pool.ntp.org";
  const char* ntpServer3 = "time.google.com";

  const long gmtOffset_sec = TIMEZONE * 3600;
  const int daylightOffset_sec = SUMMERTIME ? 3600 : 0;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2, ntpServer3);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    logln("NTP. Failed to obtain time");
    hmi.writeString("statusLCD.txt=\"NTP pool failed\"");
    NTP_AVAILABLE = false;
    return;
  }
  rtc.setTimeStruct(timeinfo);
  //rtc.setTime(30, 24, 15, 17, 1, 2021);
  logln(rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  if (!QUICK_BOOT)
  {
    hmi.writeString("statusLCD.txt=\"" + rtc.getTime("%A, %B %d %Y %H:%M:%S") + "\"");
  }
  NTP_AVAILABLE = true;
  if (!QUICK_BOOT) delay(2000);
  //struct tm timeinfo = rtc.getTimeStruct();
}

void setupSpotify() 
{
  // const char* CLIENT_ID = SPOTIFY_CLIENT_ID;
  // const char* CLIENT_SECRET = SPOTIFY_CLIENT_SECRET;
  char* REFRESH_TOKEN = nullptr;

  logln("> Setting Spotify client.");
  logln("Spotify Client ID: " + SPOTIFY_CLIENT_ID);
  logln("Spotify Client Secret: " + SPOTIFY_CLIENT_SECRET); // No es

  // Recuperar el refresh token de Spotify si existe en la SD
  File tokenFile = SD_MMC.open("/_spotifytoken", FILE_READ);
  if (tokenFile) 
  {
    String tokenString = tokenFile.readString();
    tokenString.trim();
    if (tokenString.length() > 0) 
    {
      REFRESH_TOKEN = strdup(tokenString.c_str());
      logln("Refresh token read from /_spotifytoken");
    } 
    else 
    {
      logln("/_spotifytoken is empty");
    }
    
    tokenFile.close();
  } 
  else 
  {
    logln("No /_spotifytoken found, refresh token not retrieved");
  }

  // Crear el objeto Spotify según si hay refresh token
  if (REFRESH_TOKEN && strlen(REFRESH_TOKEN) > 0) 
  {
    sp = new Spotify(SPOTIFY_CLIENT_ID.c_str(), SPOTIFY_CLIENT_SECRET.c_str(), REFRESH_TOKEN);
    logln("Spotify object created with refresh token");
  } 
  else 
  {
    sp = new Spotify(SPOTIFY_CLIENT_ID.c_str(), SPOTIFY_CLIENT_SECRET.c_str());
    logln("Spotify object created without refresh token");
  }

  // Intentamos conectar a Spotify
  sp->begin();
  int spcount = 0;
  const int stime = 6000; // tiempo de espera 2 min.
  hmi.writeString("statusLCD.txt=\"Spotify pending auth\"");
  while(!sp->is_auth() && spcount <= stime)
  {
      sp->handle_client();
      spcount++;
      delay(10);
  }

  if (spcount > stime)
  {
    logln("Timeout to connect to Spotify");
    hmi.writeString("statusLCD.txt=\"Spotify timeout\"");
    delay(1500);
    SPOTIFY_CONTROL = false;
  }
  else
  {
    logln("Connected to Spotify");
    logln("Authenticated! Refresh token: " + String(sp->get_user_tokens().refresh_token));
    // Guardar el refresh_token en un archivo en la raíz de la SD
    File tokenFile = SD_MMC.open("/_spotifytoken", FILE_WRITE);
    if (tokenFile) {
      tokenFile.seek(0);
      tokenFile.print(sp->get_user_tokens().refresh_token);
      tokenFile.close();
      logln("Spotify refresh token saved /_spotifytoken");
    } 
    else 
    {
      logln("Failed to open /_spotifytoken to save the refresh token");
    }
    SPOTIFY_CONTROL = true;
    if (!QUICK_BOOT) 
    {
      hmi.writeString("statusLCD.txt=\"Spotify auth ok\"");
      delay(1500);
    }
  }
}

void prepareCardStructure() {

  logln("> Preparing initial directory structure on SD Card.");

  // Creamos el directorio /fav
  String fDir = "/ONLINE";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating ONLINE directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /ONLINE_CPC para catálogo Amstrad CPC
  fDir = "/ONLINE_CPC";

  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating ONLINE_CPC directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /DOWNLOAD_CPC para descargas Amstrad CPC
  fDir = "/DOWNLOAD_CPC";

  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating DOWNLOAD_CPC directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /fav
  fDir = "/FAV";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating FAV directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /rec
  fDir = "/DATA";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating DATA directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /rec
  fDir = "/REC";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating REC directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /wav
  fDir = "/WAV";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating WAV directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /mp3
  fDir = "/MP3";

  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating MP3 directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /flac
  fDir = "/FLAC";

  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating FLAC directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  // Creamos el directorio /radio
  fDir = "/RADIO";

  // Esto lo hacemos para ver si el directorio existe
  if (createSpecialDirectory(fDir)) {
    hmi.writeString("statusLCD.txt=\"Creating RADIO directory\"");
    hmi.reloadCustomDir("/");
    if (!QUICK_BOOT) delay(750);
  }

  logln("Directory structure ready.");
}

void dateTimeSD(uint16_t* date, uint16_t* time) {
    
    struct tm tm;
    // Aquí pones la fecha/hora deseada
    tm.tm_year = 2026 - 1900;
    tm.tm_mon = 1; // Febrero (0-based)
    tm.tm_mday = 14;
    tm.tm_hour = 12;
    tm.tm_min = 34;
    tm.tm_sec = 56;
    *date = FS_DATE(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    *time = FS_TIME(tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void resetHMI()
{
  // Forzamos un reinicio de la pantalla
  logln("HMI Reset");
  hmi.writeString("rest");
  delay(125);

  // Indicamos que estamos haciendo reset
  sendStatus(RESET, 1);
  delay(250);  
}

void setupSerialComHMI()
{
  logln("> Setting USART for HMI pin: RX=" + String(hmiRxD) +
        " TX=" + String(hmiTxD));
  SerialHW.begin(SerialHWDataBits, SERIAL_8N1, hmiRxD, hmiTxD);
  delay(125);
  // Probamos
  //hmi.writeString("page screen");
  resetHMI();
  //delay(1250);
  //sendStatus(ACK_LCD, 1);
}

void setupMCP() 
{
  logln("> Searching MCP23017.");
  
  if (MCP23017_AVAILABLE) {
    if (setupMCP23017()) {
      // Hay placa de extensión los pines son otros
      // Ahora POWERLED está configurado en la placa MCP23017
      logln("MCP23017 found");
      hmiTxD = 5;
      hmiRxD = 22;
      GPIO_MSX_REMOTE_PAUSE = 19;
    }
  } else {
    // pines originales
    logln("MCP23017 not enable");
    hmiTxD = 23;
    hmiRxD = 18;
    powerLed = 22;
    GPIO_MSX_REMOTE_PAUSE = 19;
  }
}

void setupSDCard() {
  logln("Waiting for SD Card");

  hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"");
  delay(125);

  int SD_Speed = SD_FRQ_MHZ_INITIAL; // Velocidad en Hz (config.h)

  // ****************************************************************
  //
  //
  //      Recordar poner todos los SWITCHES del Audiokit a ON
  //
  //
  // ****************************************************************
  if (!SD_MMC.begin("/sdcard", false, false, SD_Speed)) {

    // SD no inicializada
    while (1) {
      hmi.writeString("statusLCD.txt=\"SD mount failed\"");
      delay(2000);
      hmi.writeString("statusLCD.txt=\"- Insert FAT32 40MHz SD Card\"");
      delay(2000);
      hmi.writeString("statusLCD.txt=\"- Verify audiokit switches.\"");
      delay(2000);
    }
  } else {
    logln("SD Card mounted");
    hmi.writeString("statusLCD.txt=\"SD_MMC available\"");
    delay(125);

    // Crear carpetas base para catálogos online y descargas
    if (!SD_MMC.exists("/ONLINE"))      SD_MMC.mkdir("/ONLINE");
    if (!SD_MMC.exists("/ONLINE_CPC"))  SD_MMC.mkdir("/ONLINE_CPC");
    if (!SD_MMC.exists("/ONLINE_MSX"))  SD_MMC.mkdir("/ONLINE_MSX");
    if (!SD_MMC.exists("/DOWNLOAD"))    SD_MMC.mkdir("/DOWNLOAD");
    if (!SD_MMC.exists("/DOWNLOAD_CPC")) SD_MMC.mkdir("/DOWNLOAD_CPC");
    if (!SD_MMC.exists("/DOWNLOAD_MSX")) SD_MMC.mkdir("/DOWNLOAD_MSX");
  }

  if (psramInit()) {
    // SerialHW.println("\nPSRAM is correctly initialized");
    hmi.writeString("statusLCD.txt=\"PSRAM OK\"");
  } else {
    // SerialHW.println("PSRAM not available");
    hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"");
  }
  delay(125);

  // Cache total heap sizes (they never change after boot)
  CACHED_PSRAM_TOTAL_KB = ESP.getPsramSize() / 1024;
  CACHED_HEAP_TOTAL_KB = ESP.getHeapSize() / 1024;
}

void loadHMICfgfromNVS() {
  //LAST_MESSAGE = "Loading HMI settings.";
  
  logln("> Loading HMI configuration from NVS.");

  //loadHMICfg();

  // Actualizamos la configuracion
  logln("EN_STERO = " + String(EN_STEREO));
  logln("MUTE AMP = " + String(!ACTIVE_AMP));
  logln("VOL_LIMIT_HEADPHONE = " + String(VOL_LIMIT_HEADPHONE));
  logln("MAIN_VOL" + String(MASTER_VOL));
  logln("MAIN_VOL_L" + String(MAIN_VOL_L));
  logln("MAIN_VOL_R" + String(MAIN_VOL_R));
  //
  showOption("menu.wifiEn.val", String(WIFI_ENABLE));
  // EN_STEREO
  showOption("menuAudio.stereoOut.val", String(EN_STEREO));
  // MUTE_AMPLIFIER
  showOption("menuAudio.mutAmp.val", String(!ACTIVE_AMP));
  // // POWERLED_DUTY
  // showOption("menu.ppled.val", String(ENABLE_POWER_LED));
  // First view files on sorting
  showOption("menu2.sortFil.val", String(!SORT_FILES_FIRST_DIR));
  // Powerled always off/on
  showOption("menu2.pwled.val", String(!POWER_LED_MODE));
// Hide virtual keyboard from screen
  showOption("menu2.c0.val", String(HIDE_VIRTUAL_KEY));

  if (HIDE_VIRTUAL_KEY)
  {
    myNex.writeNum("tape0.p1.pic",54);
    myNex.writeNum("tape.p0.pic",54);
    myNex.writeNum("menu2.hkey.val",1);
  }
  else
  {
    myNex.writeNum("tape0.p1.pic",48);
    myNex.writeNum("tape.p0.pic",48);
    myNex.writeNum("menu2.hkey.val",0);
  }

  //

  if (ACTIVE_AMP) {
    MAIN_VOL_L = 5;
  } else {
    MAIN_VOL_L = MAIN_VOL_R;
  }
  // Actualizamos el HMI
  hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
  hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_L));

  if (EN_SPEAKER) {
    // Actualizamos el HMI
    showOption("menuAudio.enSpeak.val", String(EN_SPEAKER));
  }

  // VOL_LIMIT
  showOption("menuAudio.volLimit.val", String(VOL_LIMIT_HEADPHONE));

  MAIN_VOL = float(MASTER_VOL);

  if (MAIN_VOL > MAX_VOL_FOR_HEADPHONE_LIMIT && VOL_LIMIT_HEADPHONE) {
    MAIN_VOL = MAX_VOL_FOR_HEADPHONE_LIMIT;
  }

  // Volumen sliders
  showOption("menuAudio.volM.val", String(int(MAIN_VOL)));
  showOption("menuAudio.volLevelM.val", String(int(MAIN_VOL)));
  //
  showOption("menuAudio.volR.val", String(int(MAIN_VOL_R)));
  showOption("menuAudio.volLevel.val", String(int(MAIN_VOL_R)));
  //
  showOption("menuAudio.volL.val", String(int(MAIN_VOL_L)));
  showOption("menuAudio.volLevelL.val", String(int(MAIN_VOL_L)));

  // Equalizer
  showOption("menuEq.eqHigh.val", String(int(EQ_HIGH * 100)));
  showOption("menuEq.eqHighL.val", String(int(EQ_HIGH * 100)));
  //
  showOption("menuEq.eqMid.val", String(int(EQ_MID * 100)));
  showOption("menuEq.eqMidL.val", String(int(EQ_MID * 100)));
  //
  showOption("menuEq.eqLow.val", String(int(EQ_LOW * 100)));
  showOption("menuEq.eqLowL.val", String(int(EQ_LOW * 100)));
  EQ_CHANGE = true;

  // Esto lo hacemos porque depende de la configuración cargada.
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);
  //
  if (!QUICK_BOOT) delay(500);
}

void I2C_ScannerWire() {
  byte error, address;
  int nDevices;

  logln("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire1.beginTransmission(address);
    error = Wire1.endTransmission();

    if (error == 0) {
      logln("I2C device found at address 0x" + String(address, HEX));
      if (address < 16)
        logln("0");
      logln(String(address, HEX));
      logln("  !");
      nDevices++;
    } else if (error == 4) {
      logln("Unknown error at address 0x");
      if (address < 16)
        logln("0");
      logln(String(address, HEX));
    }
  }
  if (nDevices == 0)
    logln("No I2C devices found\n");
  else
    logln("done\n");
}

void setup() {

  // Inicializar puerto USB Serial a 115200 para depurar / subida de firm
  Serial.begin(115200);

  // Configuramos el size de los buffers de TX y RX del puerto serie
  SerialHW.setRxBufferSize(4096);
  SerialHW.setTxBufferSize(4096);

  logln("PowaDCR " + String(VERSION));
  logln("Initializing system...");
  logln("");


  // Leemos la variable de NVS
  logln("> Loading HMI configuration from NVS. This will determine the pinout for HMI communication.");
  //
  if (!loadHMICfg())
  {
    logln("Failed to load HMI configuration from NVS. Defaulting to standard pinout.");
    // Si no se pudo cargar la configuración, asumimos que no hay MCP23017
    MCP23017_AVAILABLE = false;
    //saveHMIcfg("MCPAVAIL");
  }

  // -------------------------------------------------------------------------
  //
  // Inicializamos el soporte de audio
  //
  // -------------------------------------------------------------------------
  setupAudioKit();

  #ifdef TEST_AUDIOKIT_ON_BOOT
  testingAudioKit();
  #endif

  // Intento normal
  // setupSerialComHMI();
  // resetHMI();
  // ---------------------------------------------------------------------------
  //
  // Configuramos el puerto de comunicaciones con el HMI a 921600
  //
  // ---------------------------------------------------------------------------
  bool hmidetected = false;
  int attempts = 0;

  // if (MCP23017_AVAILABLE) 
  // {
  //   // Hay placa de extensión los pines son otros
  //   hmiTxD = 5;
  //   hmiRxD = 22;
  //   logln("MCP23017 installed. Using alternative pins for HMI communication: RX=" + String(hmiRxD) + " TX=" + String(hmiTxD));
  // } 
  // else 
  // {
  //   // pines originales
  //   hmiTxD = 23;
  //   hmiRxD = 18;
  //   logln("MCP23017 not available. Using default pins for HMI communication: RX=" + String(hmiRxD) + " TX=" + String(hmiTxD));
  // }

  if (!MCP23017_AVAILABLE)
  {
    setupSerialComHMI();
    delay(125);
    myNex.writeNum("screen.ack.val", 1);

    while (myNex.readNumber("screen.reg.val") != 1 && attempts < 5) 
    {
      logln("HMI not detected. Retrying...");
      attempts++;
      delay(125);
    } 

    if (myNex.readNumber("screen.reg.val") == 1) 
    {
      hmidetected = true;
      logln("HMI detected on default pins.");
      myNex.writeNum("screen.ack.val", 2);
      delay(125);       
      myNex.writeNum("screen.tm0.en", 0);
      MCP23017_AVAILABLE = false;
    } 
    else 
    {
      // Probamos otra pareja de pines dedicados
      SerialHW.end();
      //
      hmiTxD = 5;
      hmiRxD = 22;
      attempts = 0;

      setupSerialComHMI();
      delay(125);
      myNex.writeNum("screen.ack.val", 1);
      
      while (myNex.readNumber("screen.reg.val") != 1 && attempts < 5) 
      {
        logln("HMI not detected. Retrying...");
        attempts++;
        delay(125);
      }     

      if (myNex.readNumber("screen.reg.val") == 1) 
      {
        hmidetected = true;
        logln("HMI detected on alternative pins.");
        myNex.writeNum("screen.ack.val", 2);
        delay(125);
        myNex.writeNum("screen.tm0.en", 0);
        MCP23017_AVAILABLE = true;
      } 
      else 
      {
        logln("HMI not detected after multiple attempts. Please check connections and configuration.");
        while (1) 
        {
          // Aquí podrías agregar un mensaje en el LCD o un parpadeo para indicar el error
          hmi.writeString("statusLCD.txt=\"HMI not detected\"");
          delay(1000);
        }
      }
    }    
  }
  else
  {
      // Hay placa de extensión los pines son otros
      hmiTxD = 5;
      hmiRxD = 22;
      logln("MCP23017 installed. Using alternative pins for HMI communication: RX=" + String(hmiRxD) + " TX=" + String(hmiTxD));

      setupSerialComHMI();
      delay(125);
      myNex.writeNum("screen.ack.val", 1);
      
      while (myNex.readNumber("screen.reg.val") != 1 && attempts < 5) 
      {
        logln("HMI not detected. Retrying...");
        attempts++;
        delay(125);
      }     

      if (myNex.readNumber("screen.reg.val") == 1) 
      {
        hmidetected = true;
        logln("HMI detected on alternative pins.");
        myNex.writeNum("screen.ack.val", 2);
        delay(125);
        myNex.writeNum("screen.tm0.en", 0);
        //
        MCP23017_AVAILABLE = true;
      } 
      else 
      {
        logln("HMI not detected after multiple attempts. Please check connections and configuration.");
        while (1) 
        {
          // Aquí podrías agregar un mensaje en el LCD o un parpadeo para indicar el error
          hmi.writeString("statusLCD.txt=\"HMI not detected\"");
          delay(1000);
        }
      }
  }

  hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"");
  delay(3000);  
  
  // -------------------------------------------------------------------------
  //
  // Configuramos acceso a la SD
  //
  // -------------------------------------------------------------------------
  setupSDCard();

  // -------------------------------------------------------------------------
  //
  // Cargamos la configuración del sistema
  //
  // -------------------------------------------------------------------------
  loadCfgFile();

  // -------------------------------------------------------------------------
  //
  // Configuracion I2C - MCP23017
  //
  // -------------------------------------------------------------------------
  // I2C_ScannerWire();
  setupMCP();
  
  // ---------------------------------------------------------------------------
  //
  // Configuramos el puerto de comunicaciones con el HMI a 921600
  //
  // ---------------------------------------------------------------------------
  //setupSerialComHMI();

  // -------------------------------------------------------------------
  //
  // Arrancamos el indicador de power
  //
  // -------------------------------------------------------------------
  if (!MCP23017_AVAILABLE) {
    // Salida por el pin #powerLed
    pinMode(powerLed, OUTPUT);
    digitalWrite(powerLed, HIGH); // Apagado inicial
    for (int i = 0; i < 10; i++) {
      digitalWrite(powerLed, LOW);  // Encender
      delay(80);
      digitalWrite(powerLed, HIGH); // Apagar
      delay(80);
    }
  } else {
    hmi.writeString("statusLCD.txt=\"MCP23017 available\"");

    for (int i = 0; i < 10; i++) {
      MCP23017_writePin(MCP_LED_IO_PIN, LOW, I2C_MCP23017_ADDR);
      delay(80);
      MCP23017_writePin(MCP_LED_IO_PIN, HIGH, I2C_MCP23017_ADDR);
      delay(80);
    }
  }

// -------------------------------------------------------------------
// Configuramos el pin de Remote Pause si está habilitado
// -------------------------------------------------------------------
#ifdef MSX_REMOTE_PAUSE

  hmi.writeString("statusLCD.txt=\"Setting Remote Pause\"");

  // if (!MCP23017_AVAILABLE)
  // {
  if (!QUICK_BOOT) delay(1250);

  pinMode(GPIO_MSX_REMOTE_PAUSE, INPUT_PULLUP);
// }
#endif

  // --------------------------------------------------------------------------
  //
  // Cargamos la configuracion del HMI desde la particion NVS
  //
  // --------------------------------------------------------------------------
  loadHMICfgfromNVS();

  // -------------------------------------------------------------------------
  // Cargamos configuración WiFi
  // -------------------------------------------------------------------------
  // Si hay configuración activamos el wifi
  if (WIFI_ENABLE)
  {
    setupWifi();
  }
  // -------------------------------------------------------------------------
  //
  // Configuramos el reloj interno del sistema
  //
  // -------------------------------------------------------------------------
  if (WIFI_CONNECTED && WIFI_ENABLE)
  {
    setupNTP();
  }
  // Connect to Spotify
  // Leer el token de Spotify si existe en la SD

  #ifdef SPOTIFY_CONTROL_ENABLE
    if (WIFI_CONNECTED && WIFI_ENABLE && SPOTIFY_EN)
    {  
      setupSpotify();
    }
  #endif
  

// -------------------------------------------------------------------------
//
// Confguracion del bluetooth
//
// -------------------------------------------------------------------------
// Configurar Bluetooth si está habilitado en la configuración
#ifdef BLUETOOTH_ENABLE
  // start QueueStream
  outbt.begin(80);
  // start a2dp source
  hmi.writeString("statusLCD.txt=\"Starting Bluetooth\"");
  delay(1250);

  a2dp.set_auto_reconnect(true);
  a2dp.set_ssp_enabled(true);

  a2dp.set_data_callback(get_data);
  a2dp.start(BLUETOOTH_DEVICE_PAIRED.c_str());

  while (!a2dp.is_connected()) {
    if (a2dp.get_audio_state() == 0) {
      logln("MEDIA CONTROL ACK SUCCESS");
    }
  }

  hmi.writeString("statusLCD.txt=\"Bluetooth connected\"");
  delay(1250);

  LAST_MESSAGE = "BT Audio Enabled";
  delay(1500);

#endif

hmi.writeString("statusLCD.txt=\"Preparing environment\"");

  // ----------------------------------------------------------
  // Estructura de la SD
  //
  // ----------------------------------------------------------
  prepareCardStructure();

  // -------------------------------------------------------------------------
  //
  // Forzamos configuración estatica del HMI
  //
  // -------------------------------------------------------------------------
  // Inicializa volumen en HMI
  myNex.writeNum("menuAudio.volM.val", int(MAIN_VOL));
  myNex.writeNum("menuAudio.volLevelM.val", int(MAIN_VOL));
  myNex.writeNum("menuAudio.volL.val", int(MAIN_VOL_L));
  myNex.writeNum("menuAudio.volLevelL.val", int(MAIN_VOL_L));
  myNex.writeNum("menuAudio.volR.val", int(MAIN_VOL_R));
  myNex.writeNum("menuAudio.volLevel.val", int(MAIN_VOL_R));
  //
  myNex.writeStr("tape.tapeVol.txt", String(int(MAIN_VOL)) + "%");

  // -------------------------------------------------------------------------
  //
  // Configuramos el sampling rate por defecto
  //
  // -------------------------------------------------------------------------
  // 48KHz
  myNex.writeNum("menuAudio2.r0.val", 0);
  // 44KHz
  myNex.writeNum("menuAudio2.r1.val", 0);
  // 32KHz
  myNex.writeNum("menuAudio2.r2.val", 0);
  // 22KHz
  myNex.writeNum("menuAudio2.r3.val", 1);
  //
  // SAMPLING_RATE = BASE_SR;  // Por defecto
  BASE_SR = STANDARD_SR_8_BIT_MACHINE;
  BASE_SR_TAP = STANDARD_SR_8_BIT_MACHINE_TAP;

  auto new_sr = kitStream.defaultConfig();
  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);

  myNex.writeStr("tape.lblFreq.txt", "32KHz");
  hmi.refreshPulseIcons(INVERSETRAIN, ZEROLEVEL);

  // -------------------------------------------------------------------------
  //
  // Asignamos el HMI y sdf a los procesadores
  //
  // -------------------------------------------------------------------------
  pTAP.set_HMI(hmi);
  pTZX.set_HMI(hmi);

  // Si es test está activo. Lo lanzamos
  #ifdef TEST
    TEST_RUNNING = true;
    hmi.writeString("statusLCD.txt=\"TEST RUNNING\"");
    test();
    hmi.writeString("statusLCD.txt=\"PRESS SCREEN\"");
    TEST_RUNNING = false;
  #endif

  LOADING_STATE = 0;
  BLOCK_SELECTED = 0;
  FILE_SELECTED = false;

  // Inicialmente el POWADCR está en STOP
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;
  FFWIND = false;
  RWIND = false;

  sendStatus(REC_ST);

  // Ponemos a cero las barras de progreso
  showOption("tape.progressBlock.val", "0");
  showOption("tape.progressTotal.val", "0");

  // ---------------------------------------------------------------------------
  //
  // Configuracion de las TASK paralelas
  //
  // ---------------------------------------------------------------------------

  esp_task_wdt_init(WDT_TIMEOUT, false); // enable panic so ESP32 restarts

  // Control del tape
  // xTaskCreatePinnedToCore(Task1code, "TaskCORE1", 16384, NULL, 3 |
  // portPRIVILEGE_BIT, &Task1, 0);
  xTaskCreatePinnedToCore(Task1code, "TaskCORE1", TASK1_STACK_SIZE, NULL,3 | portPRIVILEGE_BIT, &Task1, 0);
  esp_task_wdt_add(&Task1);
  delay(500);

  // Control de la UART - HMI
  // 7168 worlds
  xTaskCreatePinnedToCore(Task0code, "TaskCORE0", TASK0_STACK_SIZE, NULL,3 | portPRIVILEGE_BIT, &Task0, 1);
  esp_task_wdt_add(&Task0);
  delay(500);

  // Inicializamos el modulo de recording
  taprec.set_HMI(hmi);
  // taprec.set_SdFat32(sdf);

  taskStop = false;

  #ifdef DEBUGMODE
    hmi.writeString("tape.name.txt=\"DEBUG MODE ACTIVE\"");
  #endif

  // -------------------------------------------------------------------------
  // Mostramos el entorno de usuario
  // -------------------------------------------------------------------------
  // Mostramos ya la pantalla inicial TAPE0
  //sendStatus(ACK_LCD, 1);
  logln("HMI ready to use.");

  while (!TAPE0_PAGE_SHOWN)
  {
    hmi.writeString("page tape0");
    delay(250);
  }

  CURRENT_PAGE = 0;
  //
  tapeAnimationOFF();

  // fin del setup()
  LAST_MESSAGE = "Press EJECT to select a file or REC.";
  CURRENT_PAGE = 0;

  
}

void loop() {}