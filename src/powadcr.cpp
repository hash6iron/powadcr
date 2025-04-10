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
    Programa principal del firmware del powadcr - Powa Digital Cassette Recording

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

// Definimos la ganancia de la entrada de linea (para RECORDING)
#define WORKAROUND_ES8388_LINE1_GAIN MIC_GAIN_MAX
#define WORKAROUND_ES8388_LINE2_GAIN MIC_GAIN_MAX

// Mezcla LINE 1 and LINE 2.
#define WORKAROUND_MIC_LINEIN_MIXED false

// Esencial para poder variar la ganancia de salida de HPLINE y ROUT, LOUT
#define AI_THINKER_ES8388_VOLUME_HACK 1
// #define USE_A2DP

// For SDFat >2.3.0 version
#define FILE_COPY_CONSTRUCTOR_SELECT FILE_COPY_CONSTRUCTOR_PUBLIC

// Enable / Disable Audiokit logging
#define USE_AUDIO_LOGGING false

// Includes
// ===============================================================

#include <Arduino.h>
#include <WiFi.h>

// #include <SD.h>
// #include <SPI.h>

#include "config.h"

#include "esp32-hal-psram.h"
#include "EasyNextionLibrary.h"

// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
TaskHandle_t Task0;
TaskHandle_t Task1;

// Definicion del puerto serie para la pantalla
#define SerialHWDataBits 921600
#define hmiTxD 23
#define hmiRxD 18
#define powerLed 22

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 360000

#include <HardwareSerial.h>
HardwareSerial SerialHW(2);

EasyNex myNex(SerialHW);


#include "globales.h"
//
#include "AudioTools.h"
#include "AudioBoard.h"
#include "AudioTools/AudioLibs/AudioBoardStream.h"

// Definimos la placa de audio
#include "AudioTools/Disk/AudioSourceSDFAT.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
//#include "AudioTools/AudioCodecs/CodecMP3LAME.h"
#include "AudioTools/CoreAudio/AudioFilter/Equalizer.h"

#include "AudioTools/CoreAudio/AudioIO.h"
//#include "AudioTools/AudioLibs/A2DPStream.h"

// Definimos el stream para Audiokit
DriverPins powadcr_pins;
AudioBoard powadcr_board(audio_driver::AudioDriverES8388,powadcr_pins);
AudioBoardStream kitStream(powadcr_board);

//BluetoothA2DPSource a2dp_source;
//A2DPStream btstream;
//I2SCodecStream  kitStream(powadcr_board);

// Estos includes deben ir en este orden por dependencias
#include "SDmanager.h"

// Creamos el gestor de ficheros para usarlo en todo el firmware
SDmanager sdm;

#include "HMI.h"
HMI hmi;

// #include "interface.h"
File32 wavfile;
EncodedAudioStream encoderOutWAV(&wavfile, new WAVEncoder());
#include "ZXProcessor.h"

// ZX Spectrum. Procesador de audio output
ZXProcessor zxp;

// Procesadores de cinta
#include "BlockProcessor.h"

#include "TZXprocessor.h"
#include "TAPprocessor.h"

//#include "test.h"

SdFat32 sdf;

SdFile sdFile;
File32 sdFile32;

// Creamos los distintos objetos
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

// OTA SD Update
// -----------------------------------------------------------------------
#include <Update.h>

// SPIFFS
// -----------------------------------------------------------------------
#include "esp_err.h"
#include "esp_spiffs.h"

// WEBFILE SERVER
// -----------------------------------------------------------------------
#include "webpage.h"
#include "webserver.h"



using namespace audio_tools;

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

// -----------------------------------------------------------------------
// Prototype function

void ejectingFile();
void isGroupStart();
void isGroupEnd();
void getRandomFilenameWAV(char *&currentPath, String currentFileBaseName);
void rewindAnimation(int direction);

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
bool statusPoweLed = false;
bool powerLedFixed = false;
//
void actuatePowerLed(bool enable, uint8_t duty)
{
  uint8_t dutyEnd = 0;

  dutyEnd = enable ? duty : 0;
  analogWrite(powerLed, dutyEnd);
}

void freeMemoryFromDescriptorTZX(tTZXBlockDescriptor *descriptor)
{
  // Vamos a liberar el descriptor completo
  for (int n = 0; n < TOTAL_BLOCKS; n++)
  {
    switch (descriptor[n].ID)
    {
    case 19: // bloque 0x13
      // Hay que liberar el array
      // delete[] descriptor[n].timming.pulse_seq_array;
      free(descriptor[n].timming.pulse_seq_array);
      break;

    default:
      break;
    }
  }
}

void freeMemoryFromDescriptorTSX(tTZXBlockDescriptor *descriptor)
{
  // Vamos a liberar el descriptor completo
  for (int n = 0; n < TOTAL_BLOCKS; n++)
  {
    switch (descriptor[n].ID)
    {
    case 19: // bloque 0x13
      // Hay que liberar el array
      // delete[] descriptor[n].timming.pulse_seq_array;
      free(descriptor[n].timming.pulse_seq_array);
      break;

    default:
      break;
    }
  }
}

int *strToIPAddress(String strIPAddr)
{
  int *ipnum = new int[4];
  int wc = 0;

  // Parto la cadena en los distintos numeros que componen la IP
  while (strIPAddr.length() > 0)
  {
    int index = strIPAddr.indexOf('.');
    // Si no se encuentran mas puntos es que es
    // el ultimo digito.
    if (index == -1)
    {
      ipnum[wc] = strIPAddr.toInt();
      return ipnum;
    }
    else
    {
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

bool loadWifiCfgFile()
{

  bool cfgloaded = false;

  if (sdf.exists("/wifi.cfg"))
  {
#ifdef DEBUGMODE
    logln("File wifi.cfg exists");
#endif

    char pathCfgFile[10] = {};
    strcpy(pathCfgFile, "/wifi.cfg");

    File32 fWifi = sdm.openFile32(pathCfgFile);
    int *IP;

    if (fWifi.isOpen())
    {
      // HOSTNAME = new char[32];
      // ssid = new char[64];
      // password = new char[64];

      char *ip1 = new char[17];

      CFGWIFI = sdm.readAllParamCfg(fWifi, 9);

      // WiFi settings
      logln("");
      logln("");
      logln("");
      logln("");
      logln("");
      logln("WiFi settings:");
      logln("------------------------------------------------------------------");
      logln("");

      // Hostname
      strcpy(HOSTNAME, (sdm.getValueOfParam(CFGWIFI[0].cfgLine, "hostname")).c_str());
      logln(HOSTNAME);
      // SSID - Wifi
      ssid = (sdm.getValueOfParam(CFGWIFI[1].cfgLine, "ssid")).c_str();
      logln(ssid);
      // Password - WiFi
      strcpy(password, (sdm.getValueOfParam(CFGWIFI[2].cfgLine, "password")).c_str());
      logln(password);

      //Local IP
      strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[3].cfgLine, "IP")).c_str());
      logln("IP: " + String(ip1));
      POWAIP = ip1;
      IP = strToIPAddress(String(ip1));
      local_IP = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // Subnet
      strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[4].cfgLine, "SN")).c_str());
      logln("SN: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      subnet = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // gateway
      strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[5].cfgLine, "GW")).c_str());
      logln("GW: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      gateway = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS1
      strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[6].cfgLine, "DNS1")).c_str());
      logln("DNS1: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      primaryDNS = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS2
      strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[7].cfgLine, "DNS2")).c_str());
      logln("DNS2: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      secondaryDNS = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      logln("Open config. WiFi-success");
      logln("");
      logln("------------------------------------------------------------------");
      logln("");
      logln("");

      fWifi.close();
      cfgloaded = true;
    }
  }
  else
  {
    // Si no existe creo uno de referencia
    File32 fWifi;
    if (!sdf.exists("/wifi_ori.cfg"))
    {
      fWifi.open("/wifi_ori.cfg", O_WRITE | O_CREAT);

      if (fWifi.isOpen())
      {
        fWifi.println("<hostname>powaDCR</hostname>");
        fWifi.println("<ssid></ssid>");
        fWifi.println("<password></password>");
        fWifi.println("<IP>192.168.1.10</IP>");
        fWifi.println("<SN>255.255.255.0</SN>");
        fWifi.println("<GW>192.168.1.1</GW>");
        fWifi.println("<DNS1>192.168.1.1</DNS1>");
        fWifi.println("<DNS2>192.168.1.1</DNS2>");

#ifdef DEBUGMODE
        logln("wifi.cfg new file created");
#endif

        fWifi.close();
      }
    }
  }

  return cfgloaded;
}

void proccesingTAP(char *file_ch)
{
  //pTAP.set_SdFat32(sdf);

  pTAP.initialize();

  if (!PLAY)
  {
    pTAP.getInfoFileTAP(file_ch);

    if (!FILE_CORRUPTED && TOTAL_BLOCKS != 0)
    {
      // El fichero está preparado para PLAY
      FILE_PREPARED = true;

      #ifdef DEBUGMODE
            logAlert("TAP prepared");
      #endif
    }
    else
    {
      LAST_MESSAGE = "ERROR! Selected file is CORRUPTED.";
      FILE_PREPARED = false;
    }
  }
  else
  {

    LAST_MESSAGE = "Error. PLAY in progress. Try select file again.";
    //
    delay(2000);
    //
    FILE_PREPARED = false;
  }
}

void proccesingTZX(char *file_ch)
{
  // Procesamos ficheros CDT, TSX y TZX
  pTZX.initialize();

  pTZX.getInfoFileTZX(file_ch);

  if (ABORT)
  {
    FILE_PREPARED = false;
    //ABORT=false;
  }
  else
  {
    if (TOTAL_BLOCKS != 0)
    {
      FILE_PREPARED = true;

#ifdef DEBUGMODE
      logAlert("TZX prepared");
#endif
    }
    else
    {
      FILE_PREPARED = false;
    }
  }
}

void sendStatus(int action, int value = 0)
{

  switch (action)
  {
  case PLAY_ST:
    //hmi.writeString("");
    hmi.writeString("PLAYst.val=" + String(value));
    break;

  case STOP_ST:
    //hmi.writeString("");
    hmi.writeString("STOPst.val=" + String(value));
    break;

  case PAUSE_ST:
    //hmi.writeString("");
    hmi.writeString("PAUSEst.val=" + String(value));
    break;

  case END_ST:
    //hmi.writeString("");
    hmi.writeString("ENDst.val=" + String(value));
    break;

  case REC_ST:
    hmi.writeString("RECst.val=" + String(value));
    break;

  case READY_ST:
    //hmi.writeString("");
    hmi.writeString("READYst.val=" + String(value));
    break;

  case ACK_LCD:
      //hmi.writeString("");
      hmi.writeString("tape.LCDACK.val=" + String(value));
      //hmi.writeString("");
      //hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");

      // Enviamos la version del firmware del powaDCR
      // hmi.writeString("mainmenu.verFirmware.txt=\" powadcr " + String(VERSION) + "\"");
      hmi.writeString("page tape0");
      break;
    
    case RESET:
      //hmi.writeString("");
      hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
      break;
    
  }
}

int setSDFrequency(int SD_Speed)
{
  bool SD_ok = false;
  bool lastStatus = false;

  // Hasta que la SD no quede testeada no se sale del bucle.
  // Si la SD no es compatible, no se puede hacer uso del dispositivo
  while (!SD_ok)
  {
    // Comenzamos con una frecuencia dada
    SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS, SHARED_SPI, SD_SCK_MHZ(SD_SPEED_MHZ), &SPI);
    if (!sdf.begin(sdcfg) || lastStatus)
    {

      // La frecuencia anterior no fue compatible
      // Probamos otras frecuencias
      hmi.writeString("statusLCD.txt=\"SD FREQ. AT " + String(SD_Speed) + " MHz\"");
      SD_Speed = SD_Speed - 1;

      // Hemos llegado a un limite
      if (SD_Speed < 1)
      {
        // Ninguna frecuencia funciona.
        // loop infinito - NO COMPATIBLE
        while (true)
        {
          hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"");
          delay(1500);
          hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"");
          delay(1500);
        }
      }
    }
    else
    {
      // La SD ha aceptado la frecuencia de acceso
      hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"");
      delay(750);

      // Probamos a listar un directorio
      if (!sdm.dir.open("/"))
      {
        SD_ok = false;
        lastStatus = false;

        // loop infinito
        while (true)
        {
          hmi.writeString("statusLCD.txt=\"SD READ TEST FAILED\"");
          delay(1500);
          hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"");
          delay(1500);
        }
      }
      else
      {
        // Probamos a crear un fichero.
        if (!sdFile32.exists("_test"))
        {
          if (!sdFile32.open("_test", O_WRITE | O_CREAT | O_TRUNC))
          {
            SD_ok = false;
            lastStatus = false;

            // loop infinito
            while (true)
            {
              hmi.writeString("statusLCD.txt=\"SD WRITE TEST FAILED\"");
              delay(1500);
              hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"");
              delay(1500);
            }
          }
          else
          {
            sdFile32.remove();
            sdFile32.close();
            SD_ok = true;
            lastStatus = true;
          }
        }
        else
        {
          while (true)
          {
            hmi.writeString("statusLCD.txt=\"REMOVE _test FILE FROM SD\"");
            delay(1500);
          }
        }
      }

      // Cerramos
      if (sdFile32.isOpen())
      {
        sdFile32.close();
      }

      if (sdm.dir.isOpen())
      {
        sdm.dir.close();
      }
    }
  }

  // Devolvemos la frecuencia
  return SD_Speed;
}

void waitForHMI(bool waitAndNotForze)
{
  // Le decimos que no queremos esperar sincronización
  // y forzamos un READY del HMI
  if (!waitAndNotForze)
  {
    LCD_ON = true;
    sendStatus(ACK_LCD, 1);
  }
  else
  {
    //Esperamos a la pantalla
    while (!LCD_ON)
    {
      hmi.readUART();
    }

    LCD_ON = true;

    sendStatus(ACK_LCD, 1);
  }
}

void tapeAnimationON()
{
  // Activamos animacion cinta
  hmi.writeString("tape2.tmAnimation.en=1");
  hmi.writeString("tape.tmAnimation.en=1");
  delay(250);
  hmi.writeString("tape2.tmAnimation.en=1");
  hmi.writeString("tape.tmAnimation.en=1");
}

void tapeAnimationOFF()
{
  // Desactivamos animacion cinta
  hmi.writeString("tape2.tmAnimation.en=0");
  hmi.writeString("tape.tmAnimation.en=0");
  delay(250);
  hmi.writeString("tape2.tmAnimation.en=0");
  hmi.writeString("tape.tmAnimation.en=0");
}

void recAnimationON()
{
  // Indicador REC parpadea
  hmi.writeString("tape.RECst.val=1");
  delay(250);
  hmi.writeString("tape.RECst.val=1");
  powerLedFixed = false;
}

void recAnimationOFF()
{
  // Indicador REC deja de parpadear
  hmi.writeString("tape.RECst.val=0");
  delay(250);
  hmi.writeString("tape.RECst.val=0");
  powerLedFixed = true;
}

void recAnimationFIXED_ON()
{
  // Indicador LED fijo ON
  hmi.writeString("tape.recIndicator.bco=63848");
  delay(250);
  hmi.writeString("tape.recIndicator.bco=63848");
  powerLedFixed = true;
}

void recAnimationFIXED_OFF()
{
  // Indicador LED fijo OFF
  hmi.writeString("tape.recIndicator.bco=32768");
  delay(250);
  hmi.writeString("tape.recIndicator.bco=32768");
  powerLedFixed = false;
}

void WavRecording()
{
  unsigned long progress_millis = 0;
  int rectime_s = 0;
  int rectime_m = 0;

  AudioInfo new_sr = kitStream.defaultConfig();
  //
  new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);       
  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_WAV_SAMPLING_RATE/1000)) + "KHz\"" );

  AudioInfo enc_sr = encoderOutWAV.defaultConfig();
  //
  enc_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE;
  encoderOutWAV.setAudioInfo(enc_sr);

  MultiOutput cmulti;   
  StreamCopy copier(cmulti, kitStream); // copies data to both file and line_out      
 
  LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";

  recAnimationOFF();
  delay(125);
  recAnimationFIXED_ON();
  tapeAnimationON();

  // Agregamos las salidas al multiple
  cmulti.add(encoderOutWAV);
  cmulti.add(kitStream);

  AudioInfo ecfg = encoderOutWAV.defaultConfig();
  //
  ecfg.sample_rate = DEFAULT_WAV_SAMPLING_RATE;
  encoderOutWAV.setAudioInfo(ecfg);   

  STOP = false;

  // loop de grabacion
  while (!STOP)
  {
    // Grabamos a WAV file
    copier.copy();
    // Sacamos audio por la salida

    if ((millis() - progress_millis) > 1500)
    {
      LAST_MESSAGE = "Recording time: " + ((rectime_m < 10 ? "0" : "") + String(rectime_m)) + ":" + ((rectime_s < 10 ? "0" : "") + String(rectime_s));

      rectime_s++;

      if (rectime_s > 59)
      {
        rectime_s = 0;
        rectime_m++;
      }

      if (wavfile.size() > 1000000)
      {
        // Megabytes
        hmi.writeString("size.txt=\"" + String(wavfile.size() / 1024 / 1024) + " MB\"");
      }
      else
      {
        // Kilobytes
        hmi.writeString("size.txt=\"" + String(wavfile.size() / 1024) + " KB\"");
      }
      
      progress_millis = millis();
    }
  }

  wavfile.flush();

  logln("File has ");
  log(String(wavfile.size() / 1024));
  log(" Kbytes");

  TAPESTATE = 0;
  LOADING_STATE = 0;
  RECORDING_ERROR = 0;
  REC = false;
  recAnimationOFF();
  recAnimationFIXED_OFF();
  tapeAnimationOFF();

  LAST_MESSAGE = "Recording finish";
  logln("Recording finish!");

  // delay(1500);

  if (wavfile.size() > 1000000)
  {
    // Megabytes
    hmi.writeString("size.txt=\"" + String(wavfile.size() / 1024 / 1024) + " MB\"");
  }
  else
  {
    // Kilobytes
    hmi.writeString("size.txt=\"" + String(wavfile.size() / 1024) + " KB\"");
  }

  wavfile.close();

  //encoder.end();
  copier.end();

  //
  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);      
  // Cambiamos el sampling rate en el HW
  // new_sr2.sample_rate = SAMPLING_RATE;
  // kitStream.setAudioInfo(new_sr2);   
  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );  

}

void pauseRecording()
{
  // Desconectamos la entrada para evitar interferencias
  //setAudioOutput();
  //waitingRecMessageShown = false;
  REC = false;
  LAST_MESSAGE = "Recording paused.";
  //
}

void stopRecording()
{

  // Verificamos cual fue el motivo de la parada
  // if (REC)
  // {

  // No quitar esto, permite leer la variable totalBlockTransfered
  int tbt = taprec.totalBlockTransfered;

  if (tbt >= 1)
  {

    if (!taprec.errorInDataRecording && !taprec.fileWasNotCreated)
    {
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
    }
    else
    {
      //
      // Hubo errores, entonces:
      //
      if (taprec.errorInDataRecording)
      {
        // La grabación fue parada porque hubo errores
        // entonces NO salvamos el fichero, lo borramos
        // Recording STOP

        taprec.terminate(true);
        LAST_MESSAGE = "Recording STOP.";
        tapeAnimationOFF();
        //
        delay(1500);
      }
      else if (taprec.fileWasNotCreated)
      {
        // Si no se crea el fichero no se puede seguir grabando
        taprec.terminate(false);
        LAST_MESSAGE = "Error in filesystem or SD.";
        tapeAnimationOFF();
        //
        delay(1500);
      }
    }
  }
  else
  {
    // 26/02/2025

    //taprec.terminate(true);
    //LAST_MESSAGE = "Recording STOP without blocks captured.";
    //tapeAnimationOFF();
    //
    delay(1500);
  }

  //}

  // Reiniciamos variables
  taprec.totalBlockTransfered = 0;
  //taprec.WasfirstStepInTheRecordingProccess = false;
  // Reiniciamos flags de recording errors
  taprec.errorInDataRecording = false;
  taprec.fileWasNotCreated = true;

  //Paramos la animación del indicador de recording
  tapeAnimationOFF();
  //
  hmi.activateWifi(true);
  //
  // Cambiamos la frecuencia de la SD
  // sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_SPEED_MHZ));
}

// -------------------------------------------------------------------------------------------------------------------
// unsigned long ota_progress_millis = 0;

void onOTAStart()
{
  pageScreenIsShown = false;
}

void onOTAProgress(size_t currSize, size_t totalSize)
{
  // log_v("CALLBACK:  Update process at %d of %d bytes...", currSize, totalSize);

#ifdef DEBUGMODE
  logln("Uploading status: " + String(currSize / 1024) + "/" + String(totalSize / 1024) + " KB");
#endif

  if (!pageScreenIsShown)
  {
    hmi.writeString("page screen");
    hmi.writeString("screen.updateBar.bco=23275");
    pageScreenIsShown = true;
  }

  int prg = 0;
  prg = (currSize * 100) / totalSize;

  hmi.writeString("statusLCD.txt=\"FIRMWARE UPDATING " + String(prg) + " %\"");
  hmi.writeString("screen.updateBar.val=" + String(prg));
}

void onOTAEnd(bool success)
{
  if (success)
  {
    hmi.writeString("statusLCD.txt=\"SUCCEESSFUL UPDATE\"");
  }
  else
  {
    hmi.writeString("statusLCD.txt=\"UPDATE FAIL\"");
  }
}

bool wifiSetup()
{
  bool wifiActive = true;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  hmi.writeString("statusLCD.txt=\"SSID: [" + String(ssid) + "]\"");
  delay(1500);

  // Configures Static IP Address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    hmi.writeString("statusLCD.txt=\"WiFi-STA setting failed!\"");
    wifiActive = false;
  }

  //int trying_wifi = 0;

  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    //trying_wifi++;
    hmi.writeString("statusLCD.txt=\"WiFi Connection failed!");
    wifiActive = false;
    // delay(125);
  }
  else
  {
    wifiActive = true;
  }

  if (wifiActive)
  {
    // hmi.writeString("statusLCD.txt=\"Wifi + OTA - Enabled\"");
    // delay(125);

    hmi.writeString("statusLCD.txt=\"IP " + WiFi.localIP().toString() + "\"");
    delay(50);
  }
  else
  {
    hmi.writeString("statusLCD.txt=\"Wifi disabled\"");
    wifiActive = false;
    delay(750);
  }

  return wifiActive;
}

void writeStatusLCD(String txt)
{
  hmi.writeString("statusLCD.txt=\"" + txt + "\"");
}

void uploadFirmDisplay(char *filetft)
{
  writeStatusLCD("New display firmware");
  File32 file;
  String ans = "";

  char strpath[20] = {};
  strcpy(strpath, "/powadcr_iface.tft");

  file = sdm.openFile32(strpath);
  uint32_t filesize = file.size();

  //Enviamos comando de conexión
  // Forzamos un reinicio de la pantalla
  hmi.write("DRAKJHSUYDGBNCJHGJKSHBDN");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  SerialHW.write(0x00);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  hmi.write("connect");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(500);

  hmi.write("connect");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);

  ans = hmi.readStr();
  delay(500);

  hmi.write("runmod=2");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);

  hmi.write("print \"mystop_yesABC\"");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);

  ans = hmi.readStr();

  hmi.write("get sleep");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  hmi.write("get dim");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  hmi.write("get baud");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  hmi.write("prints \"ABC\",0");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  ans = hmi.readStr();

  SerialHW.write(0x00);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  delay(200);

  hmi.write("whmi-wris " + String(filesize) + ",921600,1");
  SerialHW.write(0xff);
  SerialHW.write(0xff);
  SerialHW.write(0xff);

  delay(500);
  ans = hmi.readStr();

  //
  file.rewind();

  // Enviamos los datos

  char buf[4096];
  size_t avail;
  size_t readcount;
  int bl = 0;

  while (filesize > 0)
  {
    // Leemos un bloque de 4096 bytes o el residual final < 4096
    readcount = file.read(buf, filesize > 4096 ? 4096 : filesize);

    // Lo enviamos
    SerialHW.write(buf, readcount);

    // Si es el primer bloque esperamos respuesta de 0x05 o 0x08
    // en el caso de 0x08 saltaremos a la posición que indica la pantalla.

    if (bl == 0)
    {
      String res = "";

      // Una vez enviado el primer bloque esperamos 2s
      delay(2000);

      while (1)
      {
        res = hmi.readStr();

        if (res.indexOf(0x08) != -1)
        {
          int offset = (pow(16, 6) * int(res[4])) + (pow(16, 4) * int(res[3])) + (pow(16, 2) * int(res[2])) + int(res[1]);

          if (offset != 0)
          {
            file.rewind();
            delay(50);
            file.seek(offset);
            delay(50);
          }
          break;
        }
        delay(50);
      }
    }
    else
    {
      String res = "";
      while (1)
      {
        // Esperams un ACK 0x05
        res = hmi.readStr();
        if (res.indexOf(0x05) != -1)
        {
          break;
        }
      }
    }

    //
    bl++;
    // Vemos lo que queda una vez he movido el seek o he leido un bloque
    // ".available" mide lo que queda desde el puntero a EOF.
    filesize = file.available();
  }

  file.close();
}
// -------------------------------------------------------------------------------------------------------------------

void prepareOutputToWav()
{
    String wavnamepath = "";
    String wavfileBaseName = "/WAV/rec";
    char file_name[255];

    // Si es una copia de un TAPE llevara un FILE_LOAD pero,
    // si no, hay que generarlo
    if (FILE_LOAD.length() < 1)
    {
      char *cPath = (char *)ps_calloc(55, sizeof(char));
      getRandomFilenameWAV(cPath, wavfileBaseName);
      wavnamepath = String(cPath);
      free(cPath);
    }
    else
    {
      wavnamepath = "/WAV/" + FILE_LOAD.substring(0, FILE_LOAD.length() - 4) + ".wav";
    }

    // Pasamos la ruta del fichero
    REC_FILENAME = wavnamepath;
    

    strcpy(file_name, wavnamepath.c_str());  
    logln("Output to WAV: " + wavnamepath);

    // AudioLogger::instance().begin(Serial, AudioLogger::Error);

    // open file for recording WAV
    wavfile = sdf.open(file_name, O_WRITE | O_CREAT);
    FILE_LOAD = file_name;
    
    // Muestro solo el nombre. Le elimino la primera parte que es el directorio.
    hmi.writeString("name.txt=\"" + String(file_name).substring(5) + "\"");
    hmi.writeString("type.txt=\"[to WAV file]\"");

    if (!wavfile)
    {
      logln("Error open file to output playing");
      OUT_TO_WAV = false;
      return;
    }
    else
    {
      logln("Out to WAV file. Ready!");
    }

    // Configuramos el encoder para salida a DEFAULT_WAV_SAMPLING_RATE, 16-bits, STEREO
    AudioInfo wavencodercfg(DEFAULT_WAV_SAMPLING_RATE, 2, 16);
    // Inicializamos el encoder
    encoderOutWAV.begin(wavencodercfg);
    //
}

void setSTOP()
{
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;

  BLOCK_SELECTED = 0;

  if (!taprec.errorInDataRecording && !taprec.fileWasNotCreated)
  {
    LAST_MESSAGE = "Tape stop.";
  }
  delay(125);
  //Paramos la animación del indicador de recording
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

int getStreamfileSize(String path)
{
  char pfile[255] = {};
  strcpy(pfile, path.c_str());
  File32 f = sdm.openFile32(pfile);
  int fsize = f.size();
  f.close();

  if (fsize < 1000000)
  {
    hmi.writeString("size.txt=\"" + String(fsize / 1024) + " KB\"");
  }
  else
  {
    hmi.writeString("size.txt=\"" + String(fsize / 1024 / 1024) + " MB\"");
  }

  return fsize;
}

void updateIndicators(int size, int pos, int fsize, int bitrate, String fname)
{
  //
  String strBitrate = "";
  fname = fname.substring(fname.lastIndexOf("/") + 1);
  // Esta linea es sumamente importante para que el rotate mueva el texto
  FILE_LOAD = fname;

  if (bitrate != 0)
  {
    if (TYPE_FILE_LOAD == "WAV")
    {
      strBitrate = "(" + String(bitrate / 1000) + " KBps)";
    }
    else
    {
      strBitrate = "(" + String(bitrate / 1000) + " Kbps)";
    }
  }
  else
  {
    strBitrate = "(Vble. br)";
  }

  // if (TYPE_FILE_LOAD == "WAV")
  // {
  //   strBitrate = "";
  // }

  hmi.writeString("tape.totalBlocks.val=" + String(size));
  hmi.writeString("tape.currentBlock.val=" + String(pos));
  hmi.writeString("type.txt=\"" + TYPE_FILE_LOAD + " file " + strBitrate + "\"");

  if (fsize < 1000000)
  {
    hmi.writeString("size.txt=\"" + String(fsize / 1024) + " KB\"");
  }
  else
  {
    hmi.writeString("size.txt=\"" + String(fsize / 1024 / 1024) + " MB\"");
  }
}

void estimatePlayingTime(int fileread, int filesize, int samprate)
{

  if (samprate > 0)
  {
    float totalTime = 0;

    if (TYPE_FILE_LOAD == "WAV")
    {
      totalTime = (filesize) / (samprate);
    }
    else
    {
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

    if (TYPE_FILE_LOAD == "WAV")
    {
      time = (fileread) / (samprate);
    }
    else
    {
      time = (fileread * 8) / (samprate);
    }

    int min = time / 60.0;
    int sec = time - (min * 60);

    String usec = (sec < 10) ? "0" : "";
    String umin = (min < 10) ? "0" : "";

    LAST_MESSAGE = "Total time:  [" + utmin + String(tmin) + ":" + utsec + String(tsec) + "]   -   Elapsed:  [" + umin + String(min) + ":" + usec + String(sec) + "]";
  }
}

void MediaPlayer(bool isWav = false)
{

  // struct mediaInfo
  // {
  //   String fileName="";
  //   int fileSize=0;
  // };
  
  // mediaInfo mediaList[255];

  // Cambiamos el sampling rate en el HW
  AudioInfo new_sr = kitStream.defaultConfig();

  // Configuramos el amplificador de salida
  kitStream.setPAPower(ACTIVE_AMP);

  if (isWav)
  {
    new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE;    
    // Indicamos
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_WAV_SAMPLING_RATE/1000)) + "KHz\"" );
  }
  else
  {
    // new_sr.sample_rate = DEFAULT_MP3_SAMPLING_RATE;
    // // Indicamos
    // hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_MP3_SAMPLING_RATE/1000)) + "KHz\"" );
  }
  // Configuramos
  kitStream.setAudioInfo(new_sr);      

  int status = 0;
  size_t fileSize = 0;
  size_t fileread = 0;
  size_t last_fileread = 0;
  int startTime = millis();
  int stateStreamplayer = 0;
  int totalFilesIdx = 0;
  int endStreamFile = 0;
  int bitRateRead = 0;

  int offset = 0;

  // Buffer de MP3 a 256KB
  const size_t bufferSize = (256 * 1024);

  int pressedCountFFWD = 0;
  int pressedCountRWD = 0;

  int currentSamplingRate = 0;

  String txtR = "";
  
  // Configuracion de la fuente desde SD y audio
  // Declaraciones para SdFat
  AudioSourceSDFAT source(sdf);

  if (isWav)
  {
    // Ruta de la fuente de ficheros
    source.setPath(FILE_LAST_DIR.c_str());
    source.setFileFilter("*.wav");        
  }
  else
  {
    // Ruta de la fuente de ficheros
    source.setPath(FILE_LAST_DIR.c_str());
    source.setFileFilter("*.mp3");    
  }


  // Configuracion de audio
  // Definidmos el decoder para MP3
  MP3DecoderHelix decoderMP3;
  WAVDecoder decoderWAV;
  // Lo usaremos para filtrar la metadata
  // MetaDataID3 ID3;
  MetaDataFilterDecoder metadatafilter(decoderMP3);
  
  // Configuramos el player
  AudioPlayer player;
  AudioInfo newCfgWav;


  if (isWav)
  {
    //decoderWAV.begin();
    player.setAudioSource(source);
    player.setOutput(kitStream);
    player.setDecoder(decoderWAV);
    decoderWAV.addNotifyAudioChange(kitStream);
    decoderWAV.begin();
  }
  else
  {
    //decoderMP3.begin();
    player.setAudioSource(source);
    player.setOutput(kitStream);
    player.setDecoder(metadatafilter);
    decoderMP3.addNotifyAudioChange(kitStream);
    decoderMP3.begin();
  }

  // Ecualizer
  audio_tools::Equalizer3Bands eq(kitStream);
  audio_tools::ConfigEqualizer3Bands cfg_eq;

  // Configuracion del booster
  Boost boost(0.5);

  // Generamos la configuracion para la salida de audio del Audiokit

  // Configuramos el ecualizador
  cfg_eq = eq.defaultConfig();
  // cfg_eq.copyFrom(player.audioInfo());
  cfg_eq.gain_low = EQ_LOW;
  cfg_eq.gain_medium = EQ_MID;
  cfg_eq.gain_high = EQ_HIGH;

  // Hacemos pasar el decoder por el ecualizador de bandas
  if (isWav)
  {
    decoderWAV.setOutput(eq);
  }
  else
  {
    decoderMP3.setOutput(eq);
  }

  // Configuramos la salida de audio del Audiokit

  // kitStream.setSpeakerActive(ACTIVE_AMP);
  kitStream.setVolume(MAIN_VOL / 100);
    // kitStream.config().rx_tx_mode = TX_MODE; 

  // Configuramos el player
  // dejamos que el player al 100% de volumen y ajustamos el volumen en el Audiokit
  player.setVolume(1);
  player.setAutoNext(false);
  player.setBufferSize(bufferSize);

  // Iniciamos el ecualizador
  eq.begin(cfg_eq);

  // Arrancamos el player0
  if (player.begin())
  {
    LAST_MESSAGE = "Audio ready to play";

    // Ruta del fichero seleccionado
    player.setPath(PATH_FILE_TO_LOAD.c_str());
    fileSize = getStreamfileSize(PATH_FILE_TO_LOAD);

    player.setActive(true);
    // Indicamos cual es el fichero seleccionado
    source.selectStream(FILE_PTR_POS + FILE_IDX_SELECTED - 1);

    // Esto lo hacemos para parar cuando termine el audio.
    int currentIdx = (FILE_PTR_POS + FILE_IDX_SELECTED) - 1;

    // Punteros de ficheros total y en curso
    char path[255];
    strcpy(path, FILE_LAST_DIR.c_str());
    totalFilesIdx = source.size();
    //
    endStreamFile = totalFilesIdx - 1;

    // Esto lo hacemos para coger el bitrate
    if (!isWav)
    {
      bitRateRead = decoderMP3.audioInfoEx().bitrate;
    }
    else
    {
      bitRateRead = decoderWAV.audioInfoEx().byte_rate;
    }
    updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());  

    // Comenzamos el bucle de control
    while (!EJECT && !REC)
    {

      player.setVolume(1);
      kitStream.setVolume(MAIN_VOL / 100);

      if (EQ_CHANGE)
      {
        EQ_CHANGE = false;

        cfg_eq.gain_low = EQ_LOW;
        cfg_eq.gain_medium = EQ_MID;
        cfg_eq.gain_high = EQ_HIGH;

        eq.begin(cfg_eq);
      }

      // Cambiamos las caracteristicas de la onda
      if ((WAV_UPDATE_SR || WAV_UPDATE_BS || WAV_UPDATE_CH) && isWav)
      {
        newCfgWav.sample_rate = WAV_SAMPLING_RATE;
        newCfgWav.bits_per_sample = WAV_BITS_PER_SAMPLE;
        newCfgWav.channels = WAV_CHAN;
        
        WAV_UPDATE_SR = false;
        WAV_UPDATE_BS = false;
        WAV_UPDATE_CH = false;

        player.setAudioInfo(newCfgWav);
      }

      switch (stateStreamplayer)
      {
      case 0:
        // Fichero a la espera
        if (PLAY)
        {
          LAST_MESSAGE = "Playing Audio.";
          currentIdx = source.index();
          player.setVolume(0);
          kitStream.setVolume(0);

          // Esto lo hacemos para coger el bitrate
          if (!isWav)
          {
            bitRateRead = decoderMP3.audioInfoEx().bitrate;
            
            // // Obtenemos el sampling rate del player y actualizamos el kitStream
            sample_rate_t srd = player.audioInfo().sample_rate;
            logln("Native sampling rate: " + String(srd));

            hmi.writeString("tape.lblFreq.txt=\"" + String(int(srd/1000)) + "KHz\"" );
            // }             
          }
          else
          {
            bitRateRead = decoderWAV.audioInfoEx().byte_rate;            
          }

          updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr()); 

          logln("Current IDX: " + String(currentIdx));

          tapeAnimationON();
          stateStreamplayer = 1;
        }

        // Control de avance de ficheros
        if (FFWIND || RWIND)
        {
          if (FFWIND)
          {
            rewindAnimation(1);

            player.next();

            if (source.index() > endStreamFile)
            {
              source.selectStream(0);
              source.setIndex(0);
              player.begin();
            }
          }
          else if (RWIND)
          {
            rewindAnimation(-1);

            player.previous();

            // Comprobamos
            if (source.index() < 0)
            {
              source.selectStream(endStreamFile);
              source.setIndex(endStreamFile);
              player.begin(endStreamFile);
            }
          }

          fileSize = getStreamfileSize(source.toStr());

          // Ahora el current index ha cambiado lo actualizamos
          currentIdx = source.index();

          // Esto lo hacemos para coger el bitrate
          if (!isWav)
          {
            bitRateRead = decoderMP3.audioInfoEx().bitrate;
          }
          else
          {
            bitRateRead = decoderWAV.audioInfoEx().byte_rate;
          }
          updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr()); 

          FFWIND = false;
          RWIND = false;
          fileread = 0;
        }

        break;

      case 1:

        // Play
        if (PLAY)
        {
          if (BTN_PLAY_PRESSED)
          {
            BTN_PLAY_PRESSED = false;

            if (disable_auto_media_stop)
            {
              disable_auto_media_stop = false;
              // Play auto-stop
              hmi.writeString("tape.playType.txt=\"*\"");
              hmi.writeString("tape.playType.y=3");
              hmi.writeString("doevents");
            }
            else
            {
              disable_auto_media_stop = true;
              // Play continuo
              hmi.writeString("tape.playType.txt=\"C\"");
              hmi.writeString("tape.playType.y=6");
              hmi.writeString("doevents");
            }
          }

          // Vamos transmitiendo y quedandonos con el numero de bytes transmitidos
          if (offset != 0)
          {
            fileread += player.copy(last_fileread + offset);
            fileread += offset;
            LAST_MESSAGE = "FFWD - " + String(last_fileread + offset);
          }
          else
          {
            fileread += player.copy();
          }

          last_fileread = fileread;

          // Mostramos la progresion de la reproduccion
          if (fileSize > 0)
          {
            PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
            PROGRESS_BAR_BLOCK_VALUE = PROGRESS_BAR_TOTAL_VALUE;  
          }

          // Se detecta cambio de fichero al siguiente.
          //int idx = source.index();
          if (((fileread + 1) / (fileSize + 1)) >= 1)
          {

            PROGRESS_BAR_TOTAL_VALUE = 0;
            PROGRESS_BAR_BLOCK_VALUE = 0;

            // Si auto-stop activo, entonces paro
            if (!disable_auto_media_stop)
            {
              LAST_MESSAGE = "Stop playing";
              PLAY = false;
              stateStreamplayer = 4;
              tapeAnimationOFF();
            }
            else
            {
              // Pasamos a la siguiente pista
              // Actualizamos la referencia de cambio de pista
              currentIdx = source.index();
              logln("Current IDX: " + String(currentIdx) + " / " + String(source.size()));
              // Comprobamos si hemos llegado al final de
              // la lista de reproduccion (play list)
              if (currentIdx >= endStreamFile)
              {
                source.selectStream(0);
                source.setIndex(0);
                player.begin();

                logln("Change IDX: " + String(source.index()) + " / " + String(source.size() - 1));
                logln("End music list");
              }
              //
              fileSize = getStreamfileSize(source.toStr());
              updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());
              fileread = 0;
            }
          }
        }
        else if (PAUSE)
        {
          tapeAnimationOFF();
          PAUSE = false;
          stateStreamplayer = 2;
        }
        else if (STOP)
        {
          stateStreamplayer = 3;
          fileread = 0;
          // Lo ponemos asi para que empiece en CONTINUOS PLAYING
          // ya que en el if se hace un cambio dependiendo del valor anterior
          disable_auto_media_stop = false;
          PROGRESS_BAR_TOTAL_VALUE = 0;
          PROGRESS_BAR_BLOCK_VALUE = 0;
        }
        // Avance de pistas
        if (FFWIND || RWIND)
        {
          if (FFWIND)
          {
            rewindAnimation(1);

            // Lo hacemos asi porque si avanzamos con player.next()
            // da error si se pasa del total
            // Comprobamos
            player.next();

            if (source.index() > endStreamFile)
            {
              source.selectStream(0);
              source.setIndex(0);
              player.begin();
            }

            // }
          }
          else if (RWIND)
          {

            rewindAnimation(-1);

            // Lo hacemos asi porque si avanzamos con player.next()
            // da error si se pasa del total
            // Comprobamos
            player.previous();

            if (source.index() < 0)
            {
              source.selectStream(endStreamFile);
              source.setIndex(endStreamFile);
              player.begin(endStreamFile);
            }
          }

          fileSize = getStreamfileSize(source.toStr());

          // Ahora el current index ha cambiado
          currentIdx = source.index();

          // Esto lo hacemos para coger el bitrate
          if (!isWav)
          {
            bitRateRead = decoderMP3.audioInfoEx().bitrate;
            // // Obtenemos el sampling rate del player y actualizamos el kitStream
            sample_rate_t srd = player.audioInfo().sample_rate;
            logln("Native sampling rate: " + String(srd));

            hmi.writeString("tape.lblFreq.txt=\"" + String(int(srd/1000)) + "KHz\"" );            
          }
          else
          {
            bitRateRead = decoderWAV.audioInfoEx().byte_rate;
          }
          updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr()); 

          FFWIND = false;
          RWIND = false;
          fileread = 0;
        }
        break;

      case 2:
        // Pausa reanudamos el PLAY
        if (PAUSE || PLAY)
        {
          tapeAnimationON();
          stateStreamplayer = 1;
          PLAY = true;
          PAUSE = false;
          // Lo ponemos asi para que empiece en CONTINUOS PLAYING
          // ya que en el if se hace un cambio dependiendo del valor anterior
          disable_auto_media_stop = false;
        }
        break;

      case 3:
        // STOP tape
        if (STOP)
        {
          LAST_MESSAGE = "Stop playing";
          tapeAnimationOFF();
          stateStreamplayer = 0;
          player.begin();
          currentIdx = source.index();
          source.selectStream(currentIdx);

          // Indicadores
          updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());

          // Lo ponemos asi para que empiece en CONTINUOS PLAYING
          // ya que en el if se hace un cambio dependiendo del valor anterior
          disable_auto_media_stop = false;
          PROGRESS_BAR_TOTAL_VALUE = 0;
          PROGRESS_BAR_BLOCK_VALUE = 0;
        }
        break;
      
      case 4:
        // Auto-stop
        // Mandamos parar el reproductor
        hmi.verifyCommand("STOP");
        // Lo indicamos
        LAST_MESSAGE = "Stop playing";
        // Paramos la animacion de la cinta
        tapeAnimationOFF();
        // Ponemos la pista al principio
        source.selectStream(currentIdx);
        player.begin(currentIdx);

        // Indicadores
        updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());

        // hmi.writeString("name.txt=\"" + String(source.toStr()) + "\"");
        // Reseteamos el contador de progreso
        fileread = 0;
        // Saltamos al estado de reposo
        stateStreamplayer = 0;
        // Lo ponemos asi para que empiece en CONTINUOS PLAYING
        // ya que en el if se hace un cambio dependiendo del valor anterior
        disable_auto_media_stop = false;

        PLAY = false;
        PROGRESS_BAR_TOTAL_VALUE = 0;
        PROGRESS_BAR_BLOCK_VALUE = 0;

        break;

      default:
        break;
      }

      // Actualizamos el indicador
      if ((millis() - startTime) > 4000)
      {
        updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());
        startTime = millis();
      }
    }

    tapeAnimationOFF();
    
    // Paramos
    STOP = true;

    // Liberamos todo
    player.end();   
    decoderMP3.end();
    decoderWAV.end();
    metadatafilter.end();
    eq.end();

    source.end();

    delay(1000);

    moveDirection = 1;
    posRotateName = 0;
  }
  else
  {
    logln("Error player initialization");
    STOP = true;
    PLAY = false;
  }

  // Finalizamos
  rotate_enable = false;

  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);      
  // Cambiamos el sampling rate en el HW
  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );

}

// void playWAV()
// {
//   AudioInfo new_sr = kitStream.defaultConfig();
//   new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE;
//   kitStream.setAudioInfo(new_sr);      
//   // Indicamos
//   hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_WAV_SAMPLING_RATE/1000)) + "KHz\"" );

//   int status = 0;
//   size_t fileSize = 0;
//   size_t fileread = 0;
//   size_t last_fileread = 0;
//   int startTime = millis();
//   int stateStreamplayer = 0;
//   int totalFilesIdx = 0;
//   int endStreamFile = 0;
//   int bitRateRead = 0;

//   int offset = 0;

//   AudioSourceSDFAT source(sdf);

//   logln("PlayWAV - File last dir: " + FILE_LAST_DIR);

//   source.setPath(FILE_LAST_DIR.c_str());
//   source.setFileFilter("*.wav");

//   logln("Path file: " + FILE_LAST_DIR);

//   WAVDecoder decoder;
//   AudioPlayer player(source, kitStream, decoder);
//   // Ecualizer
//   audio_tools::Equalizer3Bands eq(kitStream);
//   audio_tools::ConfigEqualizer3Bands cfg_eq;
//   // Configuracion del booster
//   Boost boost(0.5);

//   // Configuramos el ecualizador
//   cfg_eq = eq.defaultConfig();
//   cfg_eq.gain_low = EQ_LOW;
//   cfg_eq.gain_medium = EQ_MID;
//   cfg_eq.gain_high = EQ_HIGH;

//   // Hacemos pasar el decoder por el ecualizador de bandas
//   decoder.begin();
//   decoder.setOutput(eq);

//   kitStream.setVolume(MAIN_VOL / 100);

//   // setup player
//   player.setVolume(1);
//   kitStream.setVolume(MAIN_VOL / 100);

//   // playerbt.setVolume(MAIN_VOL/100);
//   player.setAutoNext(false);
//   player.setBufferSize(256 * 1024);

//   // Iniciamos el ecualizador
//   eq.begin(cfg_eq);

//   logln("Current position: " + String(FILE_PTR_POS + FILE_IDX_SELECTED));

//   // Arrancamos el player
//   if (player.begin())
//   {
//     LAST_MESSAGE = "Audio ready to play";

//     player.setPath(PATH_FILE_TO_LOAD.c_str());
//     fileSize = getStreamfileSize(PATH_FILE_TO_LOAD);

//     player.setActive(true);
//     // Indicamos cual es el fichero seleccionado
//     source.selectStream(FILE_PTR_POS + FILE_IDX_SELECTED - 1);

//     // Esto lo hacemos para parar cuando termine el audio.
//     int currentIdx = (FILE_PTR_POS + FILE_IDX_SELECTED) - 1;

//     // Punteros de ficheros total y en curso
//     totalFilesIdx = source.size();
//     endStreamFile = totalFilesIdx - 1;

//     // Esto lo hacemos para coger el bitrate

//     bitRateRead = 0;
//     updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());

//     // Comenzamos el bucle de control
//     while (!EJECT && !REC)
//     {

//       player.setVolume(1);
//       kitStream.setVolume(MAIN_VOL / 100);

//       if (EQ_CHANGE)
//       {
//         EQ_CHANGE = false;

//         cfg_eq.gain_low = EQ_LOW;
//         cfg_eq.gain_medium = EQ_MID;
//         cfg_eq.gain_high = EQ_HIGH;

//         eq.begin(cfg_eq);
//       }      

//       switch (stateStreamplayer)
//       {
//       case 0:
//         // Fichero a la espera
//         if (PLAY)
//         {
//           LAST_MESSAGE = "Playing Audio.";
//           currentIdx = source.index();
//           player.setVolume(0);
//           kitStream.setVolume(0);
//           bitRateRead = 0;
//           updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());

//           logln("Current IDX: " + String(currentIdx));

//           tapeAnimationON();
//           stateStreamplayer = 1;
//         }

//         // Control de avance de ficheros
//         if (FFWIND || RWIND)
//         {
//           if (FFWIND)
//           {
//             rewindAnimation(1);

//             player.next();

//             if (source.index() > endStreamFile)
//             {
//               source.selectStream(0);
//               source.setIndex(0);
//               player.begin();
//             }
//           }
//           else if (RWIND)
//           {
//             rewindAnimation(-1);

//             player.previous();

//             // Comprobamos
//             if (source.index() < 0)
//             {
//               source.selectStream(endStreamFile);
//               source.setIndex(endStreamFile);
//               player.begin(endStreamFile);
//             }
//           }

//           fileSize = getStreamfileSize(source.toStr());

//           // Ahora el current index ha cambiado lo actualizamos
//           currentIdx = source.index();

//           bitRateRead = 0;
//           updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());

//           FFWIND = false;
//           RWIND = false;
//           fileread = 0;
//         }

//         break;

//       case 1:

//         // Play
//         if (PLAY)
//         {
//           if (BTN_PLAY_PRESSED)
//           {
//             BTN_PLAY_PRESSED = false;

//             if (disable_auto_media_stop)
//             {
//               disable_auto_media_stop = false;
//               // Play auto-stop
//               hmi.writeString("tape.playType.txt=\"*\"");
//               hmi.writeString("tape.playType.y=3");
//               hmi.writeString("doevents");
//             }
//             else
//             {
//               disable_auto_media_stop = true;
//               // Play continuo
//               hmi.writeString("tape.playType.txt=\"C\"");
//               hmi.writeString("tape.playType.y=6");
//               hmi.writeString("doevents");
//             }
//           }

//           // Vamos transmitiendo y quedandonos con el numero de bytes transmitidos
//           if (offset != 0)
//           {
//             fileread += player.copy(last_fileread + offset);
//             fileread += offset;
//             LAST_MESSAGE = "FFWD - " + String(last_fileread + offset);
//           }
//           else
//           {
//             fileread += player.copy();
//           }

//           last_fileread = fileread;

//           // Mostramos la progresion de la reproduccion
//           PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
//           PROGRESS_BAR_BLOCK_VALUE = PROGRESS_BAR_TOTAL_VALUE;

//           // Se detecta cambio de fichero al siguiente.
//           //int idx = source.index();
//           if (((fileread + 1) / (fileSize + 1)) >= 1)
//           {

//             PROGRESS_BAR_TOTAL_VALUE = 0;
//             PROGRESS_BAR_BLOCK_VALUE = 0;

//             // Si auto-stop activo, entonces paro
//             if (!disable_auto_media_stop)
//             {
//               LAST_MESSAGE = "Stop playing";
//               PLAY = false;
//               stateStreamplayer = 4;
//               tapeAnimationOFF();
//             }
//             else
//             {
//               // Pasamos a la siguiente pista
//               logln("Current IDX: " + String(currentIdx) + " / " + String(source.size()));

//               // Comprobamos si hemos llegado al final de
//               // la lista de reproduccion (play list)
//               if (currentIdx > endStreamFile)
//               {
//                 source.selectStream(0);
//                 source.setIndex(0);
//                 player.begin();

//                 logln("Change IDX: " + String(source.index()) + " / " + String(source.size()));
//               }

//               // Actualizamos la referencia de cambio de pista
//               currentIdx = source.index();
//               //
//               fileSize = getStreamfileSize(source.toStr());
//               updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());
//               fileread = 0;
//             }
//           }
//         }
//         else if (PAUSE)
//         {
//           tapeAnimationOFF();
//           PAUSE = false;
//           stateStreamplayer = 2;
//         }
//         else if (STOP)
//         {
//           stateStreamplayer = 3;
//           fileread = 0;
//           // Lo ponemos asi para que empiece en CONTINUOS PLAYING
//           // ya que en el if se hace un cambio dependiendo del valor anterior
//           disable_auto_media_stop = false;
//           PROGRESS_BAR_TOTAL_VALUE = 0;
//           PROGRESS_BAR_BLOCK_VALUE = 0;
//         }

//         // Avance de pistas
//         if (FFWIND || RWIND)
//         {
//           if (FFWIND)
//           {
//             rewindAnimation(1);

//             // Lo hacemos asi porque si avanzamos con player.next()
//             // da error si se pasa del total
//             // Comprobamos
//             player.next();

//             if (source.index() > endStreamFile)
//             {
//               source.selectStream(0);
//               source.setIndex(0);
//               player.begin();
//             }

//             // }
//           }
//           else if (RWIND)
//           {

//             rewindAnimation(-1);

//             // Lo hacemos asi porque si avanzamos con player.next()
//             // da error si se pasa del total
//             // Comprobamos
//             player.previous();

//             if (source.index() < 0)
//             {
//               source.selectStream(endStreamFile);
//               source.setIndex(endStreamFile);
//               player.begin(endStreamFile);
//             }
//           }

//           fileSize = getStreamfileSize(source.toStr());

//           // Ahora el current index ha cambiado
//           currentIdx = source.index();
//           bitRateRead = 0;
//           updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());

//           FFWIND = false;
//           RWIND = false;
//           fileread = 0;
//         }
//         break;

//       case 2:

//         // Pausa reanudamos el PLAY
//         if (PAUSE || PLAY)
//         {
//           tapeAnimationON();
//           stateStreamplayer = 1;
//           PLAY = true;
//           PAUSE = false;
//           // Lo ponemos asi para que empiece en CONTINUOS PLAYING
//           // ya que en el if se hace un cambio dependiendo del valor anterior
//           disable_auto_media_stop = false;
//         }
//         break;

//       case 3:
//         // STOP tape

//         if (STOP)
//         {
//           LAST_MESSAGE = "Stop playing";
//           tapeAnimationOFF();
//           stateStreamplayer = 0;
//           player.begin();
//           currentIdx = source.index();
//           source.selectStream(currentIdx);

//           // Indicadores
//           updateIndicators(totalFilesIdx, currentIdx + 1, fileSize, bitRateRead, source.toStr());

//           // Lo ponemos asi para que empiece en CONTINUOS PLAYING
//           // ya que en el if se hace un cambio dependiendo del valor anterior
//           disable_auto_media_stop = false;
//           PROGRESS_BAR_TOTAL_VALUE = 0;
//           PROGRESS_BAR_BLOCK_VALUE = 0;
//         }
//         break;
//       case 4:
//         // Auto-stop
//         // Mandamos parar el reproductor
//         hmi.verifyCommand("STOP");
//         // Lo indicamos
//         LAST_MESSAGE = "Stop playing";
//         // Paramos la animacion de la cinta
//         tapeAnimationOFF();
//         // Ponemos la pista al principio
//         source.selectStream(currentIdx);
//         player.begin(currentIdx);

//         // Indicadores
//         updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());

//         // hmi.writeString("name.txt=\"" + String(source.toStr()) + "\"");
//         // Reseteamos el contador de progreso
//         fileread = 0;
//         // Saltamos al estado de reposo
//         stateStreamplayer = 0;
//         // Lo ponemos asi para que empiece en CONTINUOS PLAYING
//         // ya que en el if se hace un cambio dependiendo del valor anterior
//         disable_auto_media_stop = false;

//         PLAY = false;
//         PROGRESS_BAR_TOTAL_VALUE = 0;
//         PROGRESS_BAR_BLOCK_VALUE = 0;

//         break;

//       default:
//         break;
//       }

//       // Actualizamos el indicador
//       if ((millis() - startTime) > 4000)
//       {
//         updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());
//         startTime = millis();
//       }
//     }

//     tapeAnimationOFF();
//     player.end();

//     moveDirection = 1;
//     posRotateName = 0;
//   }
//   else
//   {
//     logln("Error player initialization");
//     STOP = true;
//     PLAY = false;
//   }

//   // Finalizamos
//   rotate_enable = false;
//   player.end();
//   decoder.end();
//   source.end();

//   // Recuperamos el sampling rate
//   new_sr.sample_rate = SAMPLING_RATE;
//   kitStream.setAudioInfo(new_sr);       
//   // Indicamos
//   hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );
// }

void playingFile()
{
  rotate_enable = true;
  kitStream.setVolume(MAX_MAIN_VOL);
  
  AudioInfo new_sr = kitStream.defaultConfig();
  // Por defecto
  LAST_SAMPLING_RATE = SAMPLING_RATE;
  SAMPLING_RATE = STANDARD_SR_ZX_SPECTRUM;
  new_sr.sample_rate = STANDARD_SR_ZX_SPECTRUM;
  kitStream.setAudioInfo(new_sr);      
  // Indicamos el sampling rate
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(STANDARD_SR_ZX_SPECTRUM/1000)) + "KHz\"" );


  if (TYPE_FILE_LOAD == "TAP")
  {
    //
    sendStatus(REC_ST, 0);
    pTAP.play();
    //Paramos la animación
    tapeAnimationOFF();
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
    //
    sendStatus(REC_ST, 0);
    pTZX.play();
    //Paramos la animación
    tapeAnimationOFF();
  }
  else if (TYPE_FILE_LOAD == "WAV")
  {
    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    MediaPlayer(true);
    logln("Finish WAV playing file");
  }
  else if (TYPE_FILE_LOAD == "MP3")
  {
    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    MediaPlayer();
    logln("Finish MP3 playing file");
  }
  else
  {
    logAlert("Unknown type_file_load");
  }

  // Por defecto
  // Cambiamos el sampling rate en el HW
  SAMPLING_RATE = LAST_SAMPLING_RATE;
  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);      
  // Indicamos el sampling rate
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );
}

void verifyConfigFileForSelection()
{
  // Vamos a localizar el fichero de configuracion especifico para el fichero seleccionado
  const int max_params = 4;
  String path = FILE_LAST_DIR;
  String tmpPath = PATH_FILE_TO_LOAD;
  tConfig *fileCfg; // = (tConfig*)ps_calloc(max_params,sizeof(tConfig));
  char strpath[tmpPath.length() + 5] = {};
  strcpy(strpath, tmpPath.c_str());
  strcat(strpath, ".cfg");

  // Abrimos el fichero de configuracion.
  File32 cfg;

  logln("");
  log("path: " + String(strpath));

  if (cfg.isOpen())
  {
    cfg.close();
  }

  if (sdf.exists(strpath))
  {
    logln("");
    log("cfg path: " + String(strpath));

    if (cfg.open(strpath, O_READ))
    {
      // Leemos todos los parametros
      //cfg.rewind();
      fileCfg = sdm.readAllParamCfg(cfg, max_params);

      logln("");
      log("cfg open");

      for (int i = 0; i < max_params; i++)
      {
        logln("");
        log("Param: " + fileCfg[i].cfgLine);

        if ((fileCfg[i].cfgLine).indexOf("freq") != -1)
        {
          SAMPLING_RATE = (sdm.getValueOfParam(fileCfg[i].cfgLine, "freq")).toInt();
          
          // CAmbiamos el sampling rate del hardware de salida
          auto cfg = kitStream.defaultConfig();
          cfg.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg);

          auto cfg2 = kitStream.defaultConfig();
          cfg2.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg2);


          logln("");
          log("Sampling rate: " + String(SAMPLING_RATE));

          switch (SAMPLING_RATE)
          {
          case 48000:
            hmi.writeString("menuAudio2.r0.val=1");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=0");
            break;

          case 44100:
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=1");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=0");
            break;

          case 32000:
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=1");
            hmi.writeString("menuAudio2.r3.val=0");
            break;

          case 22050:
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=1");
            break;

          default:
            // Por defecto es 44.1KHz
            hmi.writeString("menuAudio2.r0.val=0");
            hmi.writeString("menuAudio2.r1.val=0");
            hmi.writeString("menuAudio2.r2.val=0");
            hmi.writeString("menuAudio2.r3.val=1");
            break;
          }
        }
        else if ((fileCfg[i].cfgLine).indexOf("zerolevel") != -1)
        {
          ZEROLEVEL = sdm.getValueOfParam(fileCfg[i].cfgLine, "zerolevel").toInt();

          logln("");
          log("Zero level: " + String(ZEROLEVEL));

          if (ZEROLEVEL == 1)
          {
            hmi.writeString("menuAudio2.lvlLowZero.val=1");
          }
          else
          {
            hmi.writeString("menuAudio2.lvlLowZero.val=0");
          }
        }
        else if ((fileCfg[i].cfgLine).indexOf("blockend") != -1)
        {
          APPLY_END = sdm.getValueOfParam(fileCfg[i].cfgLine, "blockend").toInt();

          logln("");
          log("Terminator forzed: " + String(APPLY_END));

          if (APPLY_END == 1)
          {
            hmi.writeString("menuAudio2.enTerm.val=1");
          }
          else
          {
            hmi.writeString("menuAudio2.enTerm.val=0");
          }
        }
        else if ((fileCfg[i].cfgLine).indexOf("polarized") != -1)
        {
          if ((sdm.getValueOfParam(fileCfg[i].cfgLine, "polarized")) == "1")
          {
            // Para que empiece en DOWN tenemos que poner POLARIZATION en UP
            // Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en DOWN)
            //POLARIZATION = down;
            EDGE_EAR_IS = POLARIZATION ^ 1;
            INVERSETRAIN = true;

            if (INVERSETRAIN)
            {
              hmi.writeString("menuAudio2.polValue.val=1");
            }
            else
            {
              hmi.writeString("menuAudio2.polValue.val=0");
            }
          }
          else
          {
            //POLARIZATION = up;
            //EDGE_EAR_IS = up;
            hmi.writeString("menuAudio2.polValue.val=0");
            EDGE_EAR_IS = POLARIZATION;
            INVERSETRAIN = false;
          }
        }
      }

      if (INVERSETRAIN)
      {
        if (ZEROLEVEL)
        {
          hmi.writeString("tape.pulseInd.pic=36");
        }
        else
        {
          hmi.writeString("tape.pulseInd.pic=33");
        }
      }
      else
      {
        if (ZEROLEVEL)
        {
          hmi.writeString("tape.pulseInd.pic=35");
        }
        else
        {
          hmi.writeString("tape.pulseInd.pic=34");
        }
      }
    }

    cfg.close();
  }

  //free(fileCfg);
}

void loadingFile(char *file_ch)
{
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
    if (PATH_FILE_TO_LOAD.indexOf(".TAP") != -1)
    {
      logln("");
      logln("TAP file");
      logln("");

      // Verificamos si hay fichero de configuracion para este archivo seleccionado
      verifyConfigFileForSelection();
      // changeLogo(41);
      // Reservamos memoria
      if (!myTAPmemoryReserved)
      {
        myTAP.descriptor = (tTAPBlockDescriptor *)ps_calloc(MAX_BLOCKS_IN_TAP, sizeof(struct tTAPBlockDescriptor));
        myTAPmemoryReserved = true;
      }

      // Pasamos el control a la clase
      pTAP.setTAP(myTAP);
      // Lo procesamos
      proccesingTAP(file_ch);
      TYPE_FILE_LOAD = "TAP";
      BYTES_TOBE_LOAD = myTAP.size;
    }
    else if ((PATH_FILE_TO_LOAD.indexOf(".TZX") != -1) || (PATH_FILE_TO_LOAD.indexOf(".TSX") != -1) || (PATH_FILE_TO_LOAD.indexOf(".CDT") != -1))
    {

      // Verificamos si hay fichero de configuracion para este archivo seleccionado
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

      if (PATH_FILE_TO_LOAD.indexOf(".TZX") != -1)
      {
        TYPE_FILE_LOAD = "TZX";
      }
      else if (PATH_FILE_TO_LOAD.indexOf(".TSX") != -1)
      {
        TYPE_FILE_LOAD = "TSX";
      }
      else
      {
        TYPE_FILE_LOAD = "CDT";
      }
      BYTES_TOBE_LOAD = myTZX.size;
    }
    else if (PATH_FILE_TO_LOAD.indexOf(".WAV") != -1)
    {
      logln("Wav file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "WAV";
    }
    else if (PATH_FILE_TO_LOAD.indexOf(".MP3") != -1)
    {
      logln("MP3 file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "MP3";
    }
  }
  else
  {
#ifdef DEBUGMODE
    logAlert("Nothing was prepared.");
#endif
    // changeLogo(0);

    if (FILE_PREPARED)
    {
      logln("Now ejecting file - Step 2");
      ejectingFile();
      FILE_PREPARED = false;
    }

    LAST_MESSAGE = "No file inside the tape";
  }
}

void stopFile()
{
  //Paramos la animación
  setSTOP();
  //Paramos la animación
  tapeAnimationOFF();
}

void pauseFile()
{
  STOP = false;
  PLAY = false;
  PAUSE = true;
  REC = false;
}

void ejectingFile()
{
  logln("Eject executing for " + TYPE_FILE_LOAD);
  // Terminamos los players
  if (TYPE_FILE_LOAD == "TAP")
  {
    // Solicitamos el puntero _myTAP de la clase
    // para liberarlo
    logln("Eject TAP");
    if (myTAPmemoryReserved)
    {
      LAST_MESSAGE = "Preparing structure";
      free(pTAP.getDescriptor());
      // Finalizamos
      pTAP.terminate();
      myTAPmemoryReserved = false;
    }
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
    // Solicitamos el puntero _myTZX de la clase
    // para liberarlo
    logln("Eject TZX");
    if (myTZXmemoryReserved)
    {
      LAST_MESSAGE = "Preparing structure";
      freeMemoryFromDescriptorTZX(pTZX.getDescriptor());
      free(pTZX.getDescriptor());
      // Finalizamos
      pTZX.terminate();
      myTZXmemoryReserved = false;
    }
  }
  else
  {
    logln("Eject WAV / MP3 not needed");
  }

  LAST_MESSAGE = "No file inside the tape";
}

void prepareRecording()
{
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

  //taprec.set_kit(ESP32kit);
  taprec.initialize();

  //writeString("");
  hmi.writeString("currentBlock.val=1");
  //writeString("");
  hmi.writeString("progressTotal.val=0");
  //writeString("");
  hmi.writeString("progression.val=0");
  // Preparamos para recording

  //Activamos la animación
  tapeAnimationON();

}

void RECready()
{
  prepareRecording();
}

void getAudioSettingFromHMI()
{
  if (myNex.readNumber("menuAdio.enTerm.val") == 1)
  {
    APPLY_END = true;
  }
  else
  {
    APPLY_END = false;
  }

  if (myNex.readNumber("menuAdio.polValue.val") == 1)
  {
    // Para que empiece en DOWN tenemos que poner POLARIZATION en UP
    // Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en DOWN)
    INVERSETRAIN = true;
  }
  else
  {
    INVERSETRAIN = false;
  }

  if (myNex.readNumber("menuAdio.lvlLowZero.val") == 1)
  {
    ZEROLEVEL = true;
  }
  else
  {
    ZEROLEVEL = false;
  }
}

void setPolarization()
{
  // Inicializa la polarizacion
  EDGE_EAR_IS = POLARIZATION;
}

void getPlayeableBlockTZX(tTZX tB, int inc)
{
  // Esta funcion busca el bloque valido siguiente
  // inc sera +1 o -1 dependiendo de si es FFWD o RWD
  while (!((tB.descriptor)[BLOCK_SELECTED].playeable))
  {
    BLOCK_SELECTED += inc;

    if (BLOCK_SELECTED > TOTAL_BLOCKS)
    {
      BLOCK_SELECTED = 1;
    }

    if (BLOCK_SELECTED < 1)
    {
      BLOCK_SELECTED = TOTAL_BLOCKS;
    }
  }
}

void nextGroupBlock()
{
  logln("Seacrh Next Group Block");

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    while (myTZX.descriptor[BLOCK_SELECTED].ID != 34 && BLOCK_SELECTED <= TOTAL_BLOCKS)
    {
      BLOCK_SELECTED++;
    }

    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1))
    {
      // No he encontrado el Group End
      BLOCK_SELECTED = 1;
    }
    else
    {
      // Avanzo uno mas
      PROGRAM_NAME_2 = "";
      LAST_BLOCK_WAS_GROUP_END = true;
      LAST_BLOCK_WAS_GROUP_START = false;
      // BLOCK_SELECTED++;
      // isGroupStart();
    }
  }
}

void prevGroupBlock()
{
  logln("Seacrh Previous Group Block");

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    while (myTZX.descriptor[BLOCK_SELECTED].ID != 33 && BLOCK_SELECTED > 1)
    {
      BLOCK_SELECTED--;
    }

    if (BLOCK_SELECTED < 1)
    {
      // No he encontrado el Group Start
      BLOCK_SELECTED = TOTAL_BLOCKS - 1;
    }
    else
    {
      // Le pasamos el nombre del grupo al PROGRAM_NAME_2
      // BLOCK_SELECTED++;
      LAST_GROUP = myTZX.descriptor[BLOCK_SELECTED].name;
      LAST_BLOCK_WAS_GROUP_START = true;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void isGroupStart()
{
  // Verificamos si se entra en un grupo
  logln("ID: " + String(myTZX.descriptor[BLOCK_SELECTED].ID));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    if (myTZX.descriptor[BLOCK_SELECTED].ID == 33)
    {
      // Es un group start
      LAST_BLOCK_WAS_GROUP_START = true;
      LAST_BLOCK_WAS_GROUP_END = false;
      // Le pasamos el nombre del grupo al PROGRAM_NAME_2
      LAST_GROUP = myTZX.descriptor[BLOCK_SELECTED].name;
    }
    else
    {
      LAST_BLOCK_WAS_GROUP_START = false;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void isGroupEnd()
{
  // Verificamos si se entra en un grupo
  logln("ID: " + String(myTZX.descriptor[BLOCK_SELECTED].ID));

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    if (myTZX.descriptor[BLOCK_SELECTED].ID == 34)
    {
      // Es un group start
      LAST_BLOCK_WAS_GROUP_END = true;
      LAST_BLOCK_WAS_GROUP_START = false;
      prevGroupBlock();
    }
    else
    {
      LAST_BLOCK_WAS_GROUP_START = false;
      LAST_BLOCK_WAS_GROUP_END = false;
    }
  }
}

void putLogo()
{
  if (TYPE_FILE_LOAD == "MP3")
  {
    // MP3 file
    hmi.writeString("tape.logo.pic=45");
    delay(5);
  }
  else if (TYPE_FILE_LOAD == "WAV")
  {
    // WAV file
    hmi.writeString("tape.logo.pic=46");
    delay(5);
  }
  else if (TYPE_FILE_LOAD == "TAP")
  {
    // Spectrum
    hmi.writeString("tape.logo.pic=41");
    delay(5);
  }
  else if (TYPE_FILE_LOAD == "TZX")
  {
    // TZX file
    hmi.writeString("tape.logo.pic=47");
    delay(5);
  }
  else if (TYPE_FILE_LOAD == "CDT")
  {
    // Amstrad
    hmi.writeString("tape.logo.pic=39");
    delay(5);
  }
  else if (TYPE_FILE_LOAD == "TSX")
  {
    // MSX
    hmi.writeString("tape.logo.pic=40");
    delay(5);
  }
  else
  {
    // En blanco
    hmi.writeString("tape.logo.pic=42");
    delay(5);
  }
  hmi.writeString("doevents");
}

void updateHMIOnBlockChange()
{
  if (TYPE_FILE_LOAD == "TAP")
  {
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name, myTAP.descriptor[BLOCK_SELECTED].typeName, myTAP.descriptor[BLOCK_SELECTED].size, true);
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID, myTZX.descriptor[BLOCK_SELECTED].group, myTZX.descriptor[BLOCK_SELECTED].name, myTZX.descriptor[BLOCK_SELECTED].typeName, myTZX.descriptor[BLOCK_SELECTED].size, myTZX.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);
}

void getRandomFilenameWAV(char *&currentPath, String currentFileBaseName)
{
  currentPath = strcpy(currentPath, currentFileBaseName.c_str());
  srand(time(0));
  delay(125);
  int rn = rand() % 999999;
  //Le unimos la extensión .TAP
  String txtRn = "-" + String(rn) + ".wav";
  char const *extPath = txtRn.c_str();
  strcat(currentPath, extPath);
}

void rewindAnimation(int direction)
{
  int p = 0;
  int frames = 19;
  int fdelay = 5;

  while (p < frames)
  {

    POS_ROTATE_CASSETTE += direction;

    if (POS_ROTATE_CASSETTE > 23)
    {
      POS_ROTATE_CASSETTE = 4;
    }

    if (POS_ROTATE_CASSETTE < 4)
    {
      POS_ROTATE_CASSETTE = 23;
    }

    hmi.writeString("tape.animation.pic=" + String(POS_ROTATE_CASSETTE));
    delay(20);

    p++;
  }
}

void getTheFirstPlayeableBlock()
{
  // Buscamos ahora el primer bloque playeable

  if (TYPE_FILE_LOAD != "TAP")
  {
    int i = 1;
    while (!myTZX.descriptor[i].playeable)
    {
      BLOCK_SELECTED = i;
      i++;
    }

    if (i > MAX_BLOCKS_IN_TZX)
    {
      i = 1;
    }

    BLOCK_SELECTED = i;
    logln("Primero playeable: " + String(i));

    // PROGRAM_NAME = myTZX.descriptor[i].name;
    strcpy(LAST_TYPE, myTZX.descriptor[i].typeName);
    LAST_SIZE = myTZX.descriptor[i].size;

    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID, myTZX.descriptor[BLOCK_SELECTED].group, myTZX.descriptor[BLOCK_SELECTED].name, myTZX.descriptor[BLOCK_SELECTED].typeName, myTZX.descriptor[BLOCK_SELECTED].size, myTZX.descriptor[BLOCK_SELECTED].playeable);
    hmi.updateInformationMainPage(true);
  }
}

void setFWIND()
{
  logln("Set FFWD - " + String(LAST_BLOCK_WAS_GROUP_START));

  if (TYPE_FILE_LOAD != "TAP")
  {
    if (LAST_BLOCK_WAS_GROUP_START)
    {
      // Si el ultimo bloque fue un GroupStart entonces busco un Group End y avanzo 1
      nextGroupBlock();
      // LAST_BLOCK_WAS_GROUP_START = false;
    }
    else
    {
      // Modo normal avanzo 1 a 1 y verifico si hay Group Start
      BLOCK_SELECTED++;
      isGroupStart();
    }
  }
  else
  {
    BLOCK_SELECTED++;
  }

  if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1))
  {
    BLOCK_SELECTED = 1;
    isGroupStart();
  }

  if (TYPE_FILE_LOAD != "TAP")
  {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
  }
  else
  {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                myTAP.descriptor[BLOCK_SELECTED].typeName,
                                myTAP.descriptor[BLOCK_SELECTED].size,
                                myTAP.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);

  rewindAnimation(1);
}

void setRWIND()
{

  logln("Set RWD - " + String(LAST_BLOCK_WAS_GROUP_END));

  if (TYPE_FILE_LOAD != "TAP")
  {
    if (LAST_BLOCK_WAS_GROUP_END)
    {
      // Si el ultimo bloque fue un Group End entonces busco un Group Start y avanzo 1
      prevGroupBlock();
      // LAST_BLOCK_WAS_GROUP_END = false;
    }
    else
    {
      BLOCK_SELECTED--;
      isGroupEnd();
    }
  }
  else
  {
    BLOCK_SELECTED--;
  }

  if (BLOCK_SELECTED < 1)
  {
    BLOCK_SELECTED = (TOTAL_BLOCKS - 1);
    isGroupEnd();
  }

  if (TYPE_FILE_LOAD != "TAP")
  {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                myTZX.descriptor[BLOCK_SELECTED].group,
                                myTZX.descriptor[BLOCK_SELECTED].name,
                                myTZX.descriptor[BLOCK_SELECTED].typeName,
                                myTZX.descriptor[BLOCK_SELECTED].size,
                                myTZX.descriptor[BLOCK_SELECTED].playeable);
  }
  else
  {
    // Forzamos un refresco de los indicadores
    hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                myTAP.descriptor[BLOCK_SELECTED].typeName,
                                myTAP.descriptor[BLOCK_SELECTED].size,
                                myTAP.descriptor[BLOCK_SELECTED].playeable);
  }

  hmi.updateInformationMainPage(true);

  rewindAnimation(-1);
}
void recoverEdgeBeginOfBlock()
{
  if (TYPE_FILE_LOAD == "TZX")
  {
    // Recuperamos el flanco con el que empezo el bloque
    EDGE_EAR_IS = myTZX.descriptor[BLOCK_SELECTED].edge;
  }
  else
  {
    // Empezamos con el tipo de polarizacion ya que este filetype no es sensible a esta
    EDGE_EAR_IS = POLARIZATION;
  }
}

void recCondition()
{
  recAnimationON();

  if (MODEWAV)
  { 
    LAST_MESSAGE = "Press PAUSE to start WAV recording.";
    TAPESTATE = 120; 
  }
  else
  { 
    LAST_MESSAGE = "Press PAUSE to start TAP recording.";
    TAPESTATE = 220; 
  }

  if (OUT_TO_WAV)
  {
    wavfile.flush();
    wavfile.close();
    //encoderOutWAV.end();
  }

  //
  AUTO_STOP = false;  
}

void tapeControl()
{
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

  if (UPDATE_HMI)
  {
    updateHMIOnBlockChange();
    UPDATE_HMI = false;
  }

  if (LAST_TAPESTATE != TAPESTATE)
  {
    logln("CASE " + String(TAPESTATE));
    LAST_TAPESTATE = TAPESTATE;
  }

  switch (TAPESTATE)
  {
  case 0:
    // Modo reposo. Aqui se está cuando:
    // - Tape: STOP
    // Posibles acciones
    // - Se pulsa EJECT - Nuevo fichero
    // - Se pulsa REC
    //
    LOADING_STATE = 0;

    #ifdef DEBUGMODE
        logAlert("Tape state 0");
    #endif

    if (EJECT)
    {
      #ifdef DEBUGMODE
            logAlert("EJECT state in TAPESTATE = " + String(TAPESTATE));
      #endif

      // Inicializamos la polarizacion
      EDGE_EAR_IS = POLARIZATION;

      // Limpiamos los campos del TAPE
      // porque hemos expulsado la cinta.
      //hmi.clearInformationFile();

      TAPESTATE = 99;
      // Expulsamos la cinta
      //FILE_PREPARED = false; // Esto lo comentamos para hacerlo cuando haga el ejecting()
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
      logln("Testing is output");
      zxp.samplingtest(14000);

      SAMPLINGTEST = false;
    }
    else if (REC)
    {
      recCondition();
    }
    else if (DISABLE_SD)
    {
      DISABLE_SD = false;
      // Deshabilitamos temporalmente la SD para poder subir un firmware
      sdf.end();
      int tout = 59;
      while (tout != 0)
      {
        delay(1000);
        hmi.writeString("debug.blockLoading.txt=\"" + String(tout) + "\"");
        tout--;
      }

      hmi.writeString("debug.blockLoading.txt=\"..\"");
      
      if (!sdf.begin(13, SD_SCK_MHZ(SD_SPEED_MHZ)))
      {
        logln("Error recoverind SD access");
      };
    }
    else
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      PLAY = false;
    }

    break;

  case 1:
    //
    // PLAY / STOP
    //

    // Esto nos permite abrir el Block Browser en reproduccion
    if (BB_OPEN || BB_UPDATE)
    {
      // Ojo! no meter delay! que esta en una reproduccion
      logln("Solicito abrir el BLOCKBROWSER");

      hmi.openBlocksBrowser(myTZX,myTAP);
      //openBlocksBrowser();

      BB_UPDATE = false;
      BB_OPEN = false;
    }

    // Estados en reproduccion
    if (PLAY)
    {
      // Inicializamos la polarización de la señal al iniciar la reproducción.
      //
      LOADING_STATE = 1;
      //Activamos la animación
      tapeAnimationON();
      // Reproducimos el fichero
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
      {
        LAST_MESSAGE = "Loading in progress. Please wait.";
      }
      else
      {
        LAST_MESSAGE = "Playing audio file.";
      }
      //
      playingFile();
    }
    else if (EJECT)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        //encoderOutWAV.end();
      }
    }
    else if (PAUSE)
    {
      // Nos vamos al estado PAUSA para seleccion de bloques
      TAPESTATE = 3;
      // Esto la hacemos para evitar una salida inesperada de seleccion de bloques.
      PAUSE = false;
      if (TYPE_FILE_LOAD == "WAV")
      {
        LAST_MESSAGE = "Playing paused.";
      }
      else
      {

        if (AUTO_PAUSE)
        {
          LAST_MESSAGE = "Tape auto-paused. Follow machine instructions.";
        }
        else
        {
          LAST_MESSAGE = "Tape paused. Press play or select block.";

          if (LAST_BLOCK_WAS_GROUP_START)
          {
            // Localizamos el GROUP START
            prevGroupBlock();
          }
        }
      }

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        //encoderOutWAV.end();
      }
    }
    else if (STOP)
    {
      // Desactivamos la animación
      // esta condicion evita solicitar la parada de manera infinita
      logln("");
      log("Tape state 1");

      // Volvemos al estado de fichero preparado
      TAPESTATE = 10;

      if (LOADING_STATE == 1)
      {
        tapeAnimationOFF();
        LOADING_STATE = 2;
      }

      LOADING_STATE = 2;
      // Al parar el c.block del HMI se pone a 1.
      BLOCK_SELECTED = 0; // 29/11

      // Reproducimos el fichero
      if (!AUTO_STOP)
      {
        LAST_MESSAGE = "Stop playing.";
      }
      else
      {
        LAST_MESSAGE = "Auto-Stop playing.";
        AUTO_STOP = false;
        PLAY = false;
        PAUSE = false;
        REC = false;
        EJECT = false;
      }

      setPolarization();
      STOP = false; //28/11

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
      {
        getTheFirstPlayeableBlock();
      }

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        //encoderOutWAV.end();
      }
    }
    else if (REC)
    {
      // Expulsamos la cinta
      if (LOADING_STATE == 0 || LOADING_STATE == 2)
      {
        if (FILE_PREPARED)
        {
          logln("Now ejecting file - Step 4");
          ejectingFile();
        }

        FILE_PREPARED = false;
        FILE_SELECTED = false;
        hmi.clearInformationFile();
        tapeAnimationOFF();
        recAnimationON();

        if (MODEWAV)
        {
          LAST_MESSAGE = "Press PAUSE to start WAV recording.";
          TAPESTATE = 120;
        }
        else
        {
          LAST_MESSAGE = "Press PAUSE to start TAP recording.";
          TAPESTATE = 220;
        }
      }

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        //encoderOutWAV.end();
      }
    }
    else
    {
      TAPESTATE = 1;
    }
    break;

  case 2:
    //
    // STOP
    //
    #ifdef DEBUGMODE
        logAlert("Tape state 2");
    #endif

    TAPESTATE = 1;
    //
    break;

  case 3:
    //
    // PAUSE
    //
    #ifdef DEBUGMODE
        logAlert("Tape state 3");
    #endif

    LOADING_STATE = 3;
    //Activamos la animación
    tapeAnimationOFF();
    // Reproducimos el fichero
    LAST_MESSAGE = "Tape paused. Press play or select block.";
    //
    if (PLAY)
    {
      // Reanudamos la reproduccion
      TAPESTATE = 1;
      AUTO_PAUSE = false;

      recoverEdgeBeginOfBlock();
    }
    else if (PAUSE)
    {
      // Reanudamos la reproduccion pero con PAUSE
      TAPESTATE = 5;
      AUTO_PAUSE = false;

      HMI_FNAME = FILE_LOAD;

      recoverEdgeBeginOfBlock();
    }
    else if (STOP)
    {
      TAPESTATE = 1;
      AUTO_PAUSE = false;

      HMI_FNAME = FILE_LOAD;

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
      {
        getTheFirstPlayeableBlock();
      }
      //
      setPolarization();
    }
    else if (FFWIND || RWIND)
    {

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
      {
        logln("Cambio de bloque");
        // Actuamos sobre el cassette
        if (FFWIND)
        {
          setFWIND();
          FFWIND = false;
        }
        else
        {
          setRWIND();
          RWIND = false;
        }

        // Actualizamos el HMI
        updateHMIOnBlockChange();
      }
    }
    else if (BB_OPEN || BB_UPDATE)
    {
      hmi.openBlocksBrowser(myTZX,myTAP);
      BB_UPDATE = false;
      BB_OPEN = false;
    }
    else if (UPDATE)
    {
      // Esto se hace para solicitar una actualizacion con los parametros del TAP / TZX
      // desde el HMI.
      if (TYPE_FILE_LOAD != "TAP")
      {
        // Forzamos un refresco de los indicadores
        hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                    myTZX.descriptor[BLOCK_SELECTED].group,
                                    myTZX.descriptor[BLOCK_SELECTED].name,
                                    myTZX.descriptor[BLOCK_SELECTED].typeName,
                                    myTZX.descriptor[BLOCK_SELECTED].size,
                                    myTZX.descriptor[BLOCK_SELECTED].playeable);
      }
      else
      {
        // Forzamos un refresco de los indicadores
        hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                    myTAP.descriptor[BLOCK_SELECTED].typeName,
                                    myTAP.descriptor[BLOCK_SELECTED].size,
                                    myTAP.descriptor[BLOCK_SELECTED].playeable);
      }

      hmi.updateInformationMainPage(true);
      UPDATE = false;
    }
    else if (EJECT)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      AUTO_PAUSE = false;
    }
    else
    {
      TAPESTATE = 3;
    }
    break;

  case 5:
    // Si venimos del estado de seleccion de bloques
    // y pulso PAUSE continuo la carga en TAPESTATE = 1
    TAPESTATE = 1;
    PLAY = true;
    PAUSE = false;
    break;

  case 10:
    //
    //File prepared. Esperando a PLAY / FFWD o RWD
    //
    if (PLAY)
    {
      if (FILE_PREPARED)
      {

        if (OUT_TO_WAV)
        {
          prepareOutputToWav();
        }

        TAPESTATE = 1;
        LOADING_STATE = 1;
      }
    }
    else if (EJECT)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
    }
    else if (REC)
    {
      if (FILE_PREPARED)
      {
        logln("Now ejecting file - Step 5");
        ejectingFile();
      }

      FILE_PREPARED = false;
      FILE_SELECTED = false;
      hmi.clearInformationFile();
      TAPESTATE = 0;
    }
    else if (FFWIND || RWIND)
    {
      // Actuamos sobre el cassette
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
      {
        logln("Cambio de bloque - CASE 10");
        // Actuamos sobre el cassette
        if (FFWIND)
        {
          setFWIND();
          FFWIND = false;
        }
        else
        {
          setRWIND();
          RWIND = false;
        }

        // Actualizamos el HMI
        updateHMIOnBlockChange();
      }
    }
    else if (BB_OPEN || BB_UPDATE)
    {
      hmi.openBlocksBrowser(myTZX,myTAP);
      BB_UPDATE = false;
      BB_OPEN = false;
    }
    else if (UPDATE)
    {
      // Esto se hace para solicitar una actualizacion con los parametros del TAP / TZX
      // desde el HMI.
      if (TYPE_FILE_LOAD != "TAP")
      {
        // Forzamos un refresco de los indicadores
        hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                    myTZX.descriptor[BLOCK_SELECTED].group,
                                    myTZX.descriptor[BLOCK_SELECTED].name,
                                    myTZX.descriptor[BLOCK_SELECTED].typeName,
                                    myTZX.descriptor[BLOCK_SELECTED].size,
                                    myTZX.descriptor[BLOCK_SELECTED].playeable);
      }
      else
      {
        // Forzamos un refresco de los indicadores
        hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                    myTAP.descriptor[BLOCK_SELECTED].typeName,
                                    myTAP.descriptor[BLOCK_SELECTED].size,
                                    myTAP.descriptor[BLOCK_SELECTED].playeable);
      }

      hmi.updateInformationMainPage(true);
      UPDATE = false;
    }
    else
    {
      TAPESTATE = 10;
      LOADING_STATE = 0;
    }
    break;

  case 99:
    //
    // Eject
    //
    #ifdef DEBUGMODE
        logAlert("Tape state 99");
    #endif

    if (FILE_BROWSER_OPEN)
    {
      // Abrimos el filebrowser
      #ifdef DEBUGMODE
            logAlert("State: File browser keep open.");
      #endif

      TAPESTATE = 100;
      LOADING_STATE = 0;
      EJECT = false;
    }
    else
    {
      TAPESTATE = 99;
      LOADING_STATE = 0;
    }

    break;

  case 100:
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

      if (FILE_SELECTED)
      {
        // Si se ha seleccionado lo cargo en el cassette.
        char file_ch[257];
        PATH_FILE_TO_LOAD.toCharArray(file_ch, 256);
        loadingFile(file_ch);

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

            putLogo();

            if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
            {
              getTheFirstPlayeableBlock();

              LAST_MESSAGE = "File inside the TAPE.";
              PROGRAM_NAME = FILE_LOAD;
              HMI_FNAME = FILE_LOAD;

              TAPESTATE = 10;
              LOADING_STATE = 0;
            }
            else
            {
              PROGRAM_NAME = FILE_LOAD;
              hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");

              LAST_MESSAGE = "File inside the TAPE.";
              HMI_FNAME = FILE_LOAD;

              TAPESTATE = 10;
              LOADING_STATE = 0;

              // Esto lo hacemos para llevar el control desde el WAV player
              logln("Playing file CASE 100:");
              playingFile();
            }
          }
          else
          {
            // Abortamos proceso de analisis
            LAST_MESSAGE = "No file inside the tape";
            TAPESTATE = 0;
            LOADING_STATE = 0;
          }
        }
        else
        {

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

        LAST_MESSAGE = "No file inside the tape";
      }
    }
    else
    {
      TAPESTATE = 100;
      LOADING_STATE = 0;
    }
    break;

  case 120:
    // Start WAV REC after PAUSE pressed
    if (PAUSE)
    {
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
    }
    else if (STOP)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
      REC = false;
      recAnimationOFF();
      recAnimationFIXED_OFF();
    }     
    else
    {
      TAPESTATE = 120;
    }
    break;

  case 130:
    // WAV REC finished / Waiting for PLAY or others
    if (PLAY)
    {
      if (REC_FILENAME !="")
      {
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
        PATH_FILE_TO_LOAD = FILE_LOAD;
        // Esto lo hacemos para llevar el control desde el WAV player
        playingFile();          
      }
    }  
    else if (REC)
    {
      recCondition();
    }    
    else if (EJECT)
    {
      //
      recAnimationOFF();
      recAnimationFIXED_OFF();
      //
      REC = false;
      //Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;        
    }    
    else
    {
      TAPESTATE = 130;
    }
    break;

  case 220:
    // Start TAP REC after PAUSE pressed
    if (PAUSE)
    {
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
    }
    else if (EJECT)
    {
      //
      stopRecording();
      recAnimationFIXED_OFF();
      //
      taprec.stopRecordingProccess = false;
      taprec.actuateAutoRECStop = false;
      REC = false;
      //Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;        
    }    
    else if (STOP)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;
      REC = false;
      STOP = false;
      recAnimationOFF();
      recAnimationFIXED_OFF();
    } 
    else
    {
      LOADING_STATE = 0;
      TAPESTATE = 220;
    }
    break;

  case 200:
    //
    // recording
    if (REC)
    {
      //esp_task_wdt_reset();

      //recordingFile();
      while (!taprec.recording())
      {
        delay(1);
      }

      // Ha acabado con errores
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
    break;

  case 230:
    //
    // After recording. Fast PLAY
    //
    if (EJECT)
    {
      //
      stopRecording();
      recAnimationFIXED_OFF();
      //
      taprec.stopRecordingProccess = false;
      taprec.actuateAutoRECStop = false;
      REC = false;
      //Volvemos al estado de reposo
      TAPESTATE = 0;
      LOADING_STATE = 0;
      RECORDING_ERROR = 0;        
    }    
    else if (REC)
    {
      recCondition();
    }
    else if (PLAY)
    {
      if (REC_FILENAME !="")
      {
        char fileRecPath[100];
        strcpy(fileRecPath,REC_FILENAME.c_str());
        PATH_FILE_TO_LOAD = REC_FILENAME;

        logln("");
        logln("REC File on tape: CASE 230 - " + REC_FILENAME);
        logln("");

        FILE_SELECTED = true;
        FILE_PREPARED = false;
        PLAY = false;
        loadingFile(fileRecPath);
        TYPE_FILE_LOAD = "TAP";
        putLogo();
        getTheFirstPlayeableBlock();
        FILE_SELECTED = false;
        PLAY = true;
        TAPESTATE = 10;    
        LOADING_STATE = 0;
      }
    }    
    else
    {
      LOADING_STATE = 0;
      TAPESTATE = 230;
    }  
    break;

  default:
#ifdef DEBUGMODE
    logAlert("Tape state unknow.");
#endif
    break;
  }
}

// bool headPhoneDetection()
// {
//   return !gpio_get_level((gpio_num_t)HEADPHONE_DETECT);
// }

/**
 * @brief SPIFFS Init
 *
 */
esp_err_t initSPIFFS()
{
  log_i("Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false};

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
      log_e("Failed to mount or format filesystem");
    else if (ret == ESP_ERR_NOT_FOUND)
      log_e("Failed to find SPIFFS partition");
    else
      log_e("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK)
    log_e("Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  else
    log_i("Partition size: total: %d used: %d", total, used);

  return ESP_OK;
}

// ******************************************************************
//
//            Gestión de nucleos del procesador
//
// ******************************************************************

void Task1code(void *pvParameters)
{
  //setup();
  for (;;)
  {
    if (serialEventRun)
      serialEventRun();

    //esp_task_wdt_reset();
    tapeControl();
  }
}

void Task0code(void *pvParameters)
{

  // const int windowNameLength = 44;

#ifndef SAMPLINGTEST
  // Core 0 - Para el HMI
  int startTime = millis();
  int startTime2 = millis();
  int startTime3 = millis();
  int startTime4 = millis();

  int tClock = millis();
  int ho = 0;
  int mi = 0;
  int se = 0;
  int tScrRfsh = 125;
  // int tRotateNameRfsh = 200;

  for (;;)
  {

    hmi.readUART();

    // Control por botones
    //buttonsControl();
    //delay(50);

    //esp_task_wdt_reset();

    #ifndef DEBUGMODE
    
        if (REC)
        {
          tScrRfsh = 250;
        }
        else
        {
          tScrRfsh = 125;
        }

        if (PWM_POWER_LED)
        {
          if ((millis() - startTime4) > 35)
          {
            // Timer para el PWM del powerled
            startTime4 = millis();

            if (!REC)
            {          
              if (plduty > POWERLED_DUTY)
              {
                chduty=-1;
              }
              else if (plduty < 1)
              {
                chduty=1;
              }
              //
              plduty += chduty;
              actuatePowerLed(powerLed,plduty);    
            }
          }
        } 

        if ((millis() - startTime3) > 500)
        {
          // Timer para el powerLed / recording led indicator
          startTime3 = millis();

          if (REC)
          {
            // Modo grabacion
            if (!powerLedFixed)
            {
              statusPoweLed = !statusPoweLed;
              actuatePowerLed(statusPoweLed,255);  
            }
            else
            {
              actuatePowerLed(true,255);
            }
          }
        }

        if ((millis() - startTime) > tScrRfsh)
        {
          startTime = millis();
          stackFreeCore1 = uxTaskGetStackHighWaterMark(Task1);
          stackFreeCore0 = uxTaskGetStackHighWaterMark(Task0);
          hmi.updateInformationMainPage();
        }

        if (rotate_enable || ENABLE_ROTATE_FILEBROWSER)
        {
          if ((millis() - startTime2) > tRotateNameRfsh && (FILE_LOAD.length() > windowNameLength || ((ROTATE_FILENAME.length() > windowNameLengthFB)) * ENABLE_ROTATE_FILEBROWSER))
          {
            // Capturamos el texto con tamaño de la ventana
            if ((TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3"))
            {
              hmi.writeString("name.txt=\"" + FILE_LOAD.substring(posRotateName, posRotateName + windowNameLength) + "\"");
            }
            else
            {
              if (!ENABLE_ROTATE_FILEBROWSER)
              {
                PROGRAM_NAME = FILE_LOAD.substring(posRotateName, posRotateName + windowNameLength);
              }
              else
              {
                hmi.writeString("file.path.txt=\"" + ROTATE_FILENAME.substring(posRotateName, posRotateName + windowNameLengthFB) + "\"");
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
          else if (FILE_LOAD.length() <= windowNameLength && (TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3"))
          {
            hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");
          }
        }

    #endif
  }
#endif
}

void showOption(String id, String value)
{
  //
  hmi.writeString(id + "=" + value);
  delay(10);
  hmi.writeString(id + "=" + value);
  delay(10);
}

void setup()
{
  // Inicializar puerto USB Serial a 115200 para depurar / subida de firm
  Serial.begin(115200);

  // Configuramos el size de los buffers de TX y RX del puerto serie
  SerialHW.setRxBufferSize(4096);
  SerialHW.setTxBufferSize(4096);

  // Configuramos el puerto de comunicaciones con el HMI a 921600
  SerialHW.begin(SerialHWDataBits, SERIAL_8N1, hmiRxD, hmiTxD);
  delay(125);

  // Forzamos un reinicio de la pantalla
  hmi.writeString("rest");
  delay(125);

  // Indicamos que estamos haciendo reset
  sendStatus(RESET, 1);
  delay(250);

  hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"");
  delay(1250);


  // -------------------------------------------------------------------------
  //
  // Inicializamos el soporte de audio
  //
  // -------------------------------------------------------------------------
  hmi.writeString("statusLCD.txt=\"Setting audio board\"");
  //
  // slot SD
  powadcr_pins.addSPI(ESP32PinsSD);
  // add i2c codec pins: scl, sda, port, frequency
  powadcr_pins.addI2C(PinFunction::CODEC, 32, 33);
  // add i2s pins: mclk, bck, ws,data_out, data_in ,(port)
  powadcr_pins.addI2S(PinFunction::CODEC, 0, 27, 25, 26, 35);
  // add other pins: PA on gpio 21
  powadcr_pins.addPin(PinFunction::PA, 21, PinLogic::Output);

  // Teclas
  powadcr_pins.addPin(PinFunction::KEY, 36, PinLogic::InputActiveLow, 1);
  powadcr_pins.addPin(PinFunction::KEY, 13, PinLogic::InputActiveLow, 2);
  powadcr_pins.addPin(PinFunction::KEY, 19, PinLogic::InputActiveLow, 3);
  powadcr_pins.addPin(PinFunction::KEY, 5, PinLogic::InputActiveLow, 6);
  // Deteccion de cable conectado al AMP
  powadcr_pins.addPin(PinFunction::AUXIN_DETECT, 12, PinLogic::InputActiveLow);
  // Deteccion de cable conectado al HEADPHONE
  powadcr_pins.addPin(PinFunction::HEADPHONE_DETECT, 39, PinLogic::InputActiveLow);
  // Amplificador ENABLE pin
  powadcr_pins.addPin(PinFunction::PA, 21, PinLogic::Output);
  // Power led indicador
  powadcr_pins.addPin(PinFunction::LED, 22, PinLogic::Output);
  //
  auto cfg = kitStream.defaultConfig(RXTX_MODE);
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.sample_rate = SAMPLING_RATE;
  //cfg.buffer_size = 262144;
  cfg.input_device =  ADC_INPUT_LINE2;
  cfg.output_device = DAC_OUTPUT_ALL;
  cfg.sd_active = true;
  
  //
  kitStream.begin(cfg);

  // Para que esta linea haga efecto debe estar el define 
  // #define USE_AUDIO_LOGGING true
  //
  #if USE_AUDIO_LOGGING
    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Debug);
  #endif
  
  // hmi.writeString("statusLCD.txt=\"Setting audio board - BT\"");
  // delay(1250);

  // // Bluetooh output
  // auto cfgA2DP = btstream.defaultConfig(TX_MODE);
  // cfgA2DP.name = "POWADCR";
  // btstream.begin(cfgA2DP);

  // // set intial volume
  // btstream.setVolume(0.3);

  // Arrancamos el indicador de power
  actuatePowerLed(true,POWERLED_DUTY);
  //
  // ----------------------------------------------------------------------

  // Configuramos acceso a la SD

  logln("Waiting for SD Card");
  // GpioPin pincs = kitStream.getPinID(PinFunction::SD,0);

  // logln("Pin CS: " + String(pincs));
  
  hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"");
  delay(125);

  int SD_Speed = SD_FRQ_MHZ_INITIAL; // Velocidad en MHz (config.h)
  SD_SPEED_MHZ = setSDFrequency(SD_Speed);

  logln("SD Card - Speed " + String(SD_SPEED_MHZ));

  // Para forzar a un valor, descomentar esta linea
  // y comentar las dos de arriba
  // Le pasamos al HMI el gestor de SD

  hmi.set_sdf(sdf);

  if (psramInit())
  {
    //SerialHW.println("\nPSRAM is correctly initialized");
    hmi.writeString("statusLCD.txt=\"PSRAM OK\"");
  }
  else
  {
    //SerialHW.println("PSRAM not available");
    hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"");
  }
  delay(125);

  // -------------------------------------------------------------------------
  // Inicializar SPIFFS mínimo
  // -------------------------------------------------------------------------
  hmi.writeString("statusLCD.txt=\"Initializing SPIFFS\"");
  initSPIFFS();
  if (esp_spiffs_mounted)
  {
    hmi.writeString("statusLCD.txt=\"SPIFFS mounted\"");
  }
  else
  {
    hmi.writeString("statusLCD.txt=\"SPIFFS error\"");
  }
  delay(500);

  // -------------------------------------------------------------------------
  // Actualización OTA por SD
  // -------------------------------------------------------------------------

  log_v("");
  log_v("Search for firmware..");
  char strpath[20] = {};
  strcpy(strpath, "/firmware.bin");
  File32 firmware = sdm.openFile32(strpath);
  if (firmware)
  {
    hmi.writeString("statusLCD.txt=\"New powadcr firmware found\"");
    onOTAStart();
    log_v("found!");
    log_v("Try to update!");

    Update.onProgress(onOTAProgress);

    Update.begin(firmware.size(), U_FLASH);
    Update.writeStream(firmware);
    if (Update.end())
    {
      log_v("Update finished!");
      hmi.writeString("statusLCD.txt=\"Update finished\"");
      onOTAEnd(true);
    }
    else
    {
      log_e("Update error!");
      hmi.writeString("statusLCD.txt=\"Update error\"");
      onOTAEnd(false);
    }

    firmware.close();

    if (sdf.remove(strpath))
    {
      log_v("Firmware deleted succesfully!");
    }
    else
    {
      log_e("Firmware delete error!");
    }
    delay(2000);

    ESP.restart();
  }
  else
  {
    log_v("not found!");
  }

  // -------------------------------------------------------------------------
  // Actualizacion del HMI
  // -------------------------------------------------------------------------
  if (sdf.exists("/powadcr_iface.tft"))
  {
    // NOTA: Este metodo necesita que la pantalla esté alimentada con 5V
    //writeStatusLCD("Uploading display firmware");

    // Alternativa 1
    char strpath[20] = {};
    strcpy(strpath, "/powadcr_iface.tft");

    uploadFirmDisplay(strpath);
    sdf.remove(strpath);
    // Esperamos al reinicio de la pantalla
    delay(5000);
  }

  // -------------------------------------------------------------------------
  // Cargamos configuración WiFi
  // -------------------------------------------------------------------------
  // Si hay configuración activamos el wifi
  logln("Wifi setting - loading");

  if (loadWifiCfgFile())
  {
    //Si la conexión es correcta se actualiza el estado del HMI
    if (wifiSetup())
    {
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

      // Webserver
      // ---------------------------------------------------
      configureWebServer();
      server.begin();
      logln("Webserver configured");
    }
  }

  delay(750);

  // ----------------------------------------------------------
  // Estructura de la SD
  //
  // ----------------------------------------------------------

  // Creamos el directorio /fav
  String fDir = "/FAV";

  //Esto lo hacemos para ver si el directorio existe
  if (!sdf.chdir(fDir))
  {
    if (!sdf.mkdir(fDir))
    {
#ifdef DEBUGMODE
      logln("");
      log("Error! Directory exists or wasn't created");
#endif
    }
    else
    {
      hmi.writeString("statusLCD.txt=\"Creating FAV directory\"");
      hmi.reloadCustomDir("/");
      delay(750);
    }
  }

  // Creamos el directorio /rec
  fDir = "/REC";

  //Esto lo hacemos para ver si el directorio existe
  if (!sdf.chdir(fDir))
  {
    if (!sdf.mkdir(fDir))
    {
        #ifdef DEBUGMODE
              logln("");
              log("Error! Directory exists or wasn't created");
        #endif
    }
    else
    {
      hmi.writeString("statusLCD.txt=\"Creating REC directory\"");
      hmi.reloadCustomDir("/");
      delay(750);
    }
  }

  // Creamos el directorio /rec
  fDir = "/WAV";

  //Esto lo hacemos para ver si el directorio existe
  if (!sdf.chdir(fDir))
  {
    if (!sdf.mkdir(fDir))
    {
        #ifdef DEBUGMODE
              logln("");
              log("Error! Directory exists or wasn't created");
        #endif
    }
    else
    {
      hmi.writeString("statusLCD.txt=\"Creating WAV directory\"");
      hmi.reloadCustomDir("/");
      delay(750);
    }
  }


  // -------------------------------------------------------------------------
  //
  // Esperando control del HMI
  //
  // -------------------------------------------------------------------------

  //Paramos la animación de la cinta1
  tapeAnimationOFF();
  // changeLogo(0);
  logln("Waiting for HMI");

  hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"");
  waitForHMI(CFG_FORZE_SINC_HMI);

  logln("HMI detected!");

  // -------------------------------------------------------------------------
  //
  // Forzamos configuración estatica del HMI
  //
  // -------------------------------------------------------------------------
  // Inicializa volumen en HMI
  hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
  hmi.writeString("menuAudio.volR.val=" + String(MAIN_VOL_R));
  hmi.writeString("menuAudio.volLevelL.val=" + String(MAIN_VOL_L));
  hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_R));

  // -------------------------------------------------------------------------
  //
  // Configuramos el sampling rate por defecto
  //
  // -------------------------------------------------------------------------

  // 48KHz
  hmi.writeString("menuAudio2.r0.val=0");
  // 44KHz
  hmi.writeString("menuAudio2.r1.val=0");
  // 32KHz
  hmi.writeString("menuAudio2.r2.val=0");
  // 22KHz
  hmi.writeString("menuAudio2.r3.val=1");
  //
  SAMPLING_RATE = 22050;

  AudioInfo new_sr = kitStream.defaultConfig();
  new_sr.sample_rate = SAMPLING_RATE;
  kitStream.setAudioInfo(new_sr);

  //
  hmi.writeString("tape.lblFreq.txt=\"22KHz\"");
  hmi.refreshPulseIcons(INVERSETRAIN, ZEROLEVEL);
  
  // -------------------------------------------------------------------------
  //
  // Asignamos el HMI y sdf a los procesadores
  //
  // -------------------------------------------------------------------------
  pTAP.set_HMI(hmi);
  pTZX.set_HMI(hmi);
  //y el gestor de ficheros
  pTAP.set_SDM(sdm);
  pTZX.set_SDM(sdm);

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
  showOption("tape.progressBlock.val","0");
  showOption("tape.progressTotal.val","0");

  // Cargamos la configuracion del HMI desde la particion NVS
  // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  LAST_MESSAGE = "Loading HMI settings.";

  loadHMICfg();

  // Actualizamos la configuracion
  logln("EN_STERO = " + String(EN_STEREO));
  logln("MUTE AMP = " + String(!ACTIVE_AMP));
  logln("VOL_LIMIT_HEADPHONE = " + String(VOL_LIMIT_HEADPHONE));
  logln("MAIN_VOL" + String(MAIN_VOL));
  logln("MAIN_VOL_L" + String(MAIN_VOL_L));
  logln("MAIN_VOL_R" + String(MAIN_VOL_R));
  //
  // EN_STEREO
  showOption("menuAudio.stereoOut.val",String(EN_STEREO));
  // MUTE_AMPLIFIER
  showOption("menuAudio.mutAmp.val",String(!ACTIVE_AMP));
  // POWERLED_DUTY
  showOption("menu.ppled.val",String(PWM_POWER_LED));
  
  if (!ACTIVE_AMP)
  {
    MAIN_VOL_L = 5;
    // Actualizamos el HMI
    hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_L));    
  }
  // VOL_LIMIT
  showOption("menuAudio.volLimit.val",String(VOL_LIMIT_HEADPHONE));
  // Volumen sliders
  showOption("menuAudio.volM.val",String(MAIN_VOL));
  showOption("menuAudio.volLevelM.val",String(MAIN_VOL));
  //
  showOption("menuAudio.volR.val",String(MAIN_VOL_R));
  showOption("menuAudio.volLevel.val",String(MAIN_VOL_R));
  //
  showOption("menuAudio.volL.val",String(MAIN_VOL_L));
  showOption("menuAudio.volLevelL.val",String(MAIN_VOL_L));

  // Equalizer
  showOption("menuEq.eqHigh.val",String(EQ_HIGH));
  showOption("menuEq.eqHighL.val",String(EQ_HIGH));
  //
  showOption("menuEq.eqMid.val",String(EQ_MID));
  showOption("menuEq.eqMidL.val",String(EQ_MID));
  //
  showOption("menuEq.eqLow.val",String(EQ_LOW));
  showOption("menuEq.eqLowL.val",String(EQ_LOW));
  EQ_CHANGE = true;
  //
  delay(500);

  //

  //
  LAST_MESSAGE = "Press EJECT to select a file or REC.";

  // ---------------------------------------------------------------------------
  //
  // Configuracion de las TASK paralelas
  //
  // ---------------------------------------------------------------------------
  
  esp_task_wdt_init(WDT_TIMEOUT, false); // enable panic so ESP32 restarts
  // Control del tape
  xTaskCreatePinnedToCore(Task1code, "TaskCORE1", 16384, NULL, 3 | portPRIVILEGE_BIT, &Task1, 0);
  esp_task_wdt_add(&Task1);
  delay(500);

  // Control de la UART - HMI
  xTaskCreatePinnedToCore(Task0code, "TaskCORE0", 8192, NULL, 3 | portPRIVILEGE_BIT, &Task0, 1);
  esp_task_wdt_add(&Task0);
  delay(500);

  // Inicializamos el modulo de recording
  taprec.set_HMI(hmi);
  taprec.set_SdFat32(sdf);

  taskStop = false;

  #ifdef DEBUGMODE
    hmi.writeString("tape.name.txt=\"DEBUG MODE ACTIVE\"");
  #endif



  // fin del setup()
}

void loop()
{
}