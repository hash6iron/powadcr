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
#define LOG_LEVEL AudioLogger::Info

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
  ecfg.channels = 2;
  ecfg.bits_per_sample = 16;
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
    if (WAV_8BIT_MONO)
    {
      wavencodercfg.bits_per_sample = 8;
      wavencodercfg.channels = 1;
    }
    else
    {
      wavencodercfg.bits_per_sample = 16;
      wavencodercfg.channels = 2;
    }
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

int getStreamfileSize(const String &path) 
{
    
    char pfile[255] = {};
    strcpy(pfile, path.c_str());
    File32 file = sdm.openFile32(pfile);
    if (!file) {
        #ifdef DEBUGMODE
            logln("Failed to open file: " + path);
        #endif
        return -1; // Retornamos -1 si no se puede abrir el archivo
    }

    int fileSize = file.size();
    file.close();

    // Actualizamos el tamaño en el HMI
    String sizeStr = (fileSize < 1000000) 
        ? String(fileSize / 1024) + " KB" 
        : String(fileSize / 1024 / 1024) + " MB";
    hmi.writeString("size.txt=\"" + sizeStr + "\"");

    return fileSize;
}

// int getStreamfileSize(String path)
// {
//   char pfile[255] = {};
//   strcpy(pfile, path.c_str());
//   File32 f = sdm.openFile32(pfile);
//   int fsize = f.size();
//   f.close();

//   if (fsize < 1000000)
//   {
//     hmi.writeString("size.txt=\"" + String(fsize / 1024) + " KB\"");
//   }
//   else
//   {
//     hmi.writeString("size.txt=\"" + String(fsize / 1024 / 1024) + " MB\"");
//   }

//   return fsize;
// }

void updateSamplingRateIndicator(int rate, int bps, bool isWav)
{
  // Cambiamos el sampling rate a 22KHz para el kitStream
  //                
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(rate / 1000)) + "KHz\"");

  if (isWav)
  {
      if (bps == 8)
      {
        // Cambiamos el sampling rate a 22KHz para el kitStream
        //                
        hmi.writeString("tape.wavind.txt=\"8BIT\"");
      }
      else
      {
        if (OUT_TO_WAV)
        {
          hmi.writeString("tape.wavind.txt=\"WAV\"");
        }
        else
        {
          hmi.writeString("tape.wavind.txt=\"\"");
        }
      }              
  }
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

int findIDByLastField(File32 &file, const String &searchValue) {
    if (!file.isOpen()) {
        #ifdef DEBUGMODE
            logln("File is not open.");
        #endif
        return -1; // Archivo no abierto
    }

    file.rewind(); // Comenzamos desde el principio del archivo
    char line[256];
    int n = 0;

    while ((n = file.fgets(line, sizeof(line))) > 0) {
        line[n - 1] = 0; // Eliminamos el terminador '\n'
        String lineStr = String(line);

        // Extraemos el último dato contenido entre '|'
        int lastPipe = lineStr.lastIndexOf('|');
        int secondLastPipe = lineStr.lastIndexOf('|', lastPipe - 1);
        String lastField = "";

        if (secondLastPipe != -1 && lastPipe != -1 && secondLastPipe < lastPipe) {
            lastField = lineStr.substring(secondLastPipe + 1, lastPipe);
            lastField.trim(); // Eliminamos espacios en blanco
        }

        // Comparamos el último campo con el valor buscado
        if (lastField.equalsIgnoreCase(searchValue)) {
            // Extraemos el número al principio de la línea
            int firstPipe = lineStr.indexOf('|');
            if (firstPipe != -1) {
                String idStr = lineStr.substring(0, firstPipe);
                return idStr.toInt(); // Devolvemos el ID como entero
            }
        }
    }

    return -1; // No se encontró coincidencia
}

String findLastFieldByID(File32 &file, int searchID) {
    if (!file.isOpen()) {
        #ifdef DEBUGMODE
            logln("File is not open.");
        #endif
        return ""; // Archivo no abierto
    }

    file.rewind(); // Comenzamos desde el principio del archivo
    char line[256];
    int n = 0;

    while ((n = file.fgets(line, sizeof(line))) > 0) {
        line[n - 1] = 0; // Eliminamos el terminador '\n'
        String lineStr = String(line);

        // Extraemos el ID al principio de la línea
        int firstPipe = lineStr.indexOf('|');
        if (firstPipe != -1) {
            String idStr = lineStr.substring(0, firstPipe);
            int id = idStr.toInt();

            // Comparamos el ID con el valor buscado
            if (id == searchID) {
                // Extraemos el último dato contenido entre '|'
                int lastPipe = lineStr.lastIndexOf('|');
                int secondLastPipe = lineStr.lastIndexOf('|', lastPipe - 1);
                if (secondLastPipe != -1 && lastPipe != -1 && secondLastPipe < lastPipe) {
                    String lastField = lineStr.substring(secondLastPipe + 1, lastPipe);
                    lastField.trim(); // Eliminamos espacios en blanco
                    return lastField; // Devolvemos el último campo
                }
            }
        }
    }

    return ""; // No se encontró coincidencia
}

void MediaPlayer(bool isWav = false) {
    
    // Generamos el media descriptor
    String mediapath = "";
    File32 file;
    mediapath = FILE_LAST_DIR + "_files.lst";
    if(!file.open(mediapath.c_str(), O_RDONLY))
    {
      LAST_MESSAGE = "Error opening file list";
      return;
    }

    // Setup
    //EncodedAudioStream outStream;

    // Configuramos el amplificador de salida
    kitStream.setPAPower(ACTIVE_AMP);
    kitStream.setVolume(MAIN_VOL / 100);

    // Configuración de la fuente de audio
    AudioSourceSDFAT source(sdf);
    source.setPath(FILE_LAST_DIR.c_str());
    source.setFileFilter(isWav ? "*.wav" : "*.mp3");
    // Configuración del decodificador
    MultiDecoder decoder;
    MP3DecoderHelix decoderMP3;
    WAVDecoder decoderWAV;    
    decoder.addDecoder(decoderMP3, "audio/mpeg");
    decoder.addDecoder(decoderWAV, "audio/vnd.wave");
    decoder.addNotifyAudioChange(kitStream);


    MetaDataFilterDecoder metadatafilter(decoder);

    
    // Actualizamos el sampling rate del kitStream
    // NumberFormatConverterStreamT<int16_t,int8_t> nfc8(kitStream);
    // NumberFormatConverterStreamT<int16_t,int24_t> nfc24(kitStream);
    // NumberFormatConverterStreamT<int16_t,int32_t> nfc32(kitStream);
    // NumberFormatConverterStreamT<int16_t,int16_t> nfc16(kitStream);
    // nfc16.begin();
    // outStream.setOutput(nfc16);
    // outStream.begin();

    // Configuración del ecualizador
    audio_tools::Equalizer3Bands eq(kitStream);
    audio_tools::ConfigEqualizer3Bands cfg_eq;

    cfg_eq.gain_low = EQ_LOW;
    cfg_eq.gain_medium = EQ_MID;
    cfg_eq.gain_high = EQ_HIGH;


    // Configuración del reproductor
    AudioPlayer player;

    // Inicializamos el player
    player.setAudioSource(source);
    player.setOutput(eq);
    player.setDecoder(metadatafilter); // Usamos el puntero al decodificador MP3
    player.setVolume(1);
    player.setBufferSize(512 * 1024); // Buffer de 512 KB
    player.setAutoNext(false);
    
    if (!player.begin()) 
    {
      logln("Error player initialization");
      STOP = true;
      PLAY = false;
      return;
    }
    
    // Variables del bucle

    // Esto lo hacemos para capturar el fichero seleccionado
    source.selectStream(PATH_FILE_TO_LOAD.c_str()); // Seleccionamos el archivo actual
    source.setIndex(findIDByLastField(file, FILE_LOAD.c_str()) - 1); // Buscamos el índice del archivo actual

    // Variables
    size_t fileSize = getStreamfileSize(PATH_FILE_TO_LOAD);
    size_t fileread = 0;
    int bitRateRead = 0;
    int totalFilesIdx = source.size();
    int currentIdx = source.index(); // El índice actual del archivo
    int stateStreamplayer = 0;
    unsigned long lastUpdate = millis();
    unsigned long twiceRWDTime = millis();
    int last_blockSelected = 0;
    bool was_pressed_wd = false;
    //
    TOTAL_BLOCKS = totalFilesIdx + 1;
    BLOCK_SELECTED = currentIdx + 1; // Para mostrar el bloque seleccionado en la pantalla

    // Esto lo hacemos para poder abrir el block browser sin haber pulsado PLAY.
    hmi.writeString("tape.BBOK.val=1");
    // Actualización inicial de indicadores
    updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());

    // Bucle principal
    while (!EJECT && !REC) 
    {
        // Actualizamos el volumen y el ecualizador si es necesario
        kitStream.setVolume(MAIN_VOL / 100);
        if (EQ_CHANGE) {
            EQ_CHANGE = false;
            cfg_eq.gain_low = EQ_LOW;
            cfg_eq.gain_medium = EQ_MID;
            cfg_eq.gain_high = EQ_HIGH;
            eq.begin(cfg_eq);
        }
        // Estados del reproductor
        switch (stateStreamplayer) {
            case 0: // Esperando reproducción
                if (PLAY) 
                {
                    player.begin(currentIdx); // Iniciamos el reproductor
                    // Esto es necesario para que el player sepa el sampling rate
                    fileread += player.copy();
                    //
                    // nfc8.end(); // Reiniciamos el convertidor de bits
                    // nfc16.end(); // Reiniciamos el convertidor de bits
                    // nfc24.end(); // Reiniciamos el convertidor de bits
                    // nfc32.end(); // Reiniciamos el convertidor de bits
                    // //
                    // switch (decoder.audioInfo().bits_per_sample) 
                    // {
                    //     case 8:
                    //         nfc8.begin(); // Reiniciamos el convertidor de bits
                    //         outStream.setOutput(nfc8);
                    //         break;
                    //     case 16:
                    //         nfc16.begin(); // Reiniciamos el convertidor de bits
                    //         outStream.setOutput(nfc16);
                    //         break;
                    //     case 24:
                    //         nfc24.begin(); // Reiniciamos el convertidor de bits
                    //         outStream.setOutput(nfc24);
                    //         break;
                    //     case 32:
                    //         nfc32.begin(); // Reiniciamos el convertidor de bits
                    //         outStream.setOutput(nfc32);
                    //         break;
                    //     default:
                    //         nfc16.begin();
                    //         outStream.setOutput(nfc16);
                    //         break;
                    // }
                    //
                    AudioInfo info = kitStream.defaultConfig();
                    info.sample_rate = decoder.audioInfo().sample_rate;
                    kitStream.setAudioInfo(info);

                    // Esto lo hacemos para indicar el sampling rate del reproductor
                    updateSamplingRateIndicator(decoder.audioInfo().sample_rate, decoder.audioInfo().bits_per_sample, isWav);

                    // Cambiamos de estado
                    stateStreamplayer = 1;
                    tapeAnimationON();
                }
                break;

            case 1: // Reproduciendo
                fileread += player.copy();
                if (fileSize > 0) {
                    PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
                }
 
                // Actualizamos indicadores cada 4 segundos
                if (millis() - lastUpdate > 2000) 
                {
                    updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());
                    lastUpdate = millis();
                }

                // Verificamos si se terminó el archivo
                // if (fileread >= fileSize) 
                // {
                //     if (!disable_auto_media_stop)
                //     {
                //       stateStreamplayer = 4; // Auto-stop      
                //     }
                // }

                if (STOP) 
                {
                  stateStreamplayer = 0;
                  fileread = 0;
                  tapeAnimationOFF();
                }    
                
                if (PAUSE) 
                {
                  stateStreamplayer = 2; // Pausa
                  tapeAnimationOFF();
                  PLAY=false;
                  PAUSE=false;
                }  
                break;

            case 2: // PAUSE
                if (PAUSE || PLAY)
                {
                  stateStreamplayer = 1; // Reproduciendo
                  tapeAnimationON();
                  PAUSE = false;
                }
                else if (STOP)
                {
                  stateStreamplayer = 0;
                  fileread = 0;
                  tapeAnimationOFF();
                }
                break;
            
            case 4: // Auto-stop
                tapeAnimationOFF();
                stateStreamplayer = 0;
                PLAY = false;
                STOP = true;
                break;                
            
            default:
                break;
        }

        // Control de avance/retroceso
        if ((FFWIND || RWIND) && !was_pressed_wd) 
        {
            rewindAnimation(FFWIND ? 1 : -1);
            if (FFWIND) 
            {
              player.next();
            } 
            else
            {
              // Modo origen solo se hace en reproducción
              if ((millis() - twiceRWDTime > 5000) && PLAY)
              {
                // Empiezo desde el principio
                player.begin(source.index());
                twiceRWDTime = millis();
              }
              else
              {
                // Si pulso rapido en los proximos segundos               
                player.previous();
              }
            }

            // delay(250);
            currentIdx = source.index();
            player.begin(currentIdx);
            delay(250);
            fileSize = getStreamfileSize(source.toStr());
              
            // Esto es necesario para que el player sepa el sampling rate
            fileread += player.copy();
            //
            AudioInfo info = kitStream.defaultConfig();
            info.sample_rate = decoder.audioInfo().sample_rate;
            kitStream.setAudioInfo(info);

            // Esto lo hacemos para indicar el sampling rate del reproductor
            updateSamplingRateIndicator(decoder.audioInfo().sample_rate, decoder.audioInfo().bits_per_sample, isWav);         

            if ((source.index() > (source.size()-1)))
            {
              if(disable_auto_media_stop) 
              {
                source.selectStream(0);
                //source.setIndex(0);
                currentIdx = 0;
                player.begin(); // Reiniciar el reproductor
              } 
              else if (!disable_auto_media_stop && (source.index() > (source.size()-1))) 
              {
                source.selectStream(0);  
                //source.setIndex(0);            
                currentIdx = 0;
                player.begin(); // Reiniciar el reproductor
                player.stop(); // Detener el reproductor
                fileSize = getStreamfileSize(source.toStr());
                STOP=true;
                PLAY=false;
              } 
            }   
            FFWIND = RWIND = false;
            fileread = 0;
            //
            updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());
            
            // Para evitar errores de lectura en el buffer
            delay(250);
        }
        else
        {
          was_pressed_wd = false;
          //
          KEEP_FFWIND = false;
          KEEP_RWIND = false;
          //
          FFWIND = false;
          RWIND = false;
        }
   
        if (KEEP_FFWIND || KEEP_RWIND) 
        {
            if (KEEP_FFWIND)
            {
              // Avance rapido
              fileread += player.copy(5 * DEFAULT_BUFFER_SIZE);
            }
            else
            {
              // Retroceso rapido
              if (fileread > (5 * DEFAULT_BUFFER_SIZE))
              {
                fileread = player.copy(fileread - (5 * DEFAULT_BUFFER_SIZE));
              }
            }

            if (fileSize > 0) 
            {
              PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
            }
            //
            // Esto se usa para evitar entrar en FFWD o RWD despues de una pulsacion prolongada (y soltar)
            was_pressed_wd = true;
         }
        // Seleccion de pista
        if (UPDATE)
        {
          // Si el bloque seleccionado es válido y no es el último
          if (BLOCK_SELECTED > 0 && (BLOCK_SELECTED <= (totalFilesIdx+1))) {
              // Reproducir el bloque seleccionado
              //source.selectStream((FILE_LAST_DIR + findLastFieldByID(file,BLOCK_SELECTED)).c_str());
              source.setIndex(BLOCK_SELECTED); // Buscamos el índice del archivo actual
              currentIdx = source.index() - 1; // Buscamos el índice del archivo actual
          } else {
              // Reproducir desde el principio
              currentIdx = 0;
              source.selectStream(0);
          }  
          // Actualizamos de manera inmediata
          //

          player.begin(currentIdx); // Iniciamos el reproductor      
          delay(250);
            
          fileSize = getStreamfileSize(source.toStr());

          // Esto lo hacemos para indicar el sampling rate del reproductor
          updateSamplingRateIndicator(decoder.audioInfo().sample_rate, decoder.audioInfo().bits_per_sample, isWav);
  
          bitRateRead = isWav ? decoderWAV.audioInfoEx().byte_rate : decoderMP3.audioInfoEx().bitrate;
          updateIndicators(totalFilesIdx, source.index() + 1, fileSize, bitRateRead, source.toStr());  
          //                
          UPDATE = false;                
        }

        if (BB_OPEN || BB_UPDATE)
        {
          last_blockSelected = BLOCK_SELECTED;
          //
          hmi.openBlockMediaBrowser(source);
          //delay(25);
          hmi.openBlockMediaBrowser(source);
          BB_OPEN = false;
          BB_UPDATE = false; 
        }

        if (UPDATE_HMI)
        {
          if (BLOCK_SELECTED > 0 && BLOCK_SELECTED <= TOTAL_BLOCKS)
          {
              // Actualizamos la pantalla HMI con el nombre del archivo actual
              //source.selectStream((FILE_LAST_DIR + myMediaDescriptor[BLOCK_SELECTED-1].name).c_str());
              // Cogemos el indice que empieza desde 0 ..
              source.setIndex(BLOCK_SELECTED - 1); // Buscamos el índice del archivo actual
              currentIdx = source.index();

              // logln("Update HMI");
              // logln("------------------------------");
              // logln("Block selected: " + String(BLOCK_SELECTED));
              // logln("source index: " + String(source.index()));

              // Inicializamos el player con ese indice
              player.begin(currentIdx); // Iniciamos el reproductor

              // Cogemos informacion del fichero
              delay(250);
              fileSize = getStreamfileSize(source.toStr());
              
              // Esto lo hacemos para coger el sampling rate
              sample_rate_t srd = player.audioInfo().sample_rate;
              hmi.writeString("tape.lblFreq.txt=\"" + String(int(srd/1000)) + "KHz\"" );                         
              // Actualizamos HMI
              updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());  

              #ifdef DEBUGMODE
                  logln("Update HMI: " + String(BLOCK_SELECTED) + " - " + String(totalFilesIdx));
                  logln("Current IDX: " + String(currentIdx));
                  logln("Current file: " + String(source.toStr()));
                  logln("Current file size: " + String(fileSize) + " bytes");
                  logln("Current file bitrate: " + String(bitRateRead) + " bps");
                  logln("Current file index: " + String(source.index()));
                  logln("Current file name: " + String(myMediaDescriptor[BLOCK_SELECTED-1].name));
                  logln("Current file path: " + String(myMediaDescriptor[BLOCK_SELECTED-1].path));
                  logln("Current file ID: " + String(myMediaDescriptor[BLOCK_SELECTED-1].ID));
              #endif                    
          }
          else
          {
            BLOCK_SELECTED = last_blockSelected;
          }

          UPDATE_HMI = false;
        }        
   
   }

    // Esto lo hacemos para indicar WAV o 8BIT antes de irnos
    updateSamplingRateIndicator(decoder.audioInfo().sample_rate, decoder.audioInfo().bits_per_sample, isWav);

    // Finalización
    tapeAnimationOFF();
    player.end();
    eq.end();
    //decoder->end();
    decoderMP3.end();
    decoderWAV.end(); 
    source.end();
    file.close();
}

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

    if (EJECT && !STOP_OR_PAUSE_REQUEST)
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
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
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
        STOP_OR_PAUSE_REQUEST = false;
      }
      else
      {
        LAST_MESSAGE = "Auto-Stop playing.";
        AUTO_STOP = false;
        PLAY = false;
        PAUSE = false;
        REC = false;
        EJECT = false;
        STOP_OR_PAUSE_REQUEST = false;
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
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
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
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
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

      // Limpiamos el nombre del fichero a cargar
      FILE_LOAD = "";
      hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");
      //

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
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
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
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
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
    if (EJECT && !STOP_OR_PAUSE_REQUEST)
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