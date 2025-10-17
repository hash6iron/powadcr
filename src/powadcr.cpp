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


// Includes
// ===============================================================
#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <vector>

#include "config.h"

#include "esp32-hal-psram.h"
#include "EasyNextionLibrary.h"

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
#include "AudioTools/Disk/AudioSourceIdxSDMMC.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/AudioCodecs/CodecFLACFoxen.h"
#include "AudioTools/CoreAudio/AudioFilter/Equalizer.h"

#ifdef BLUETOOTH_ENABLE
  #include "AudioTools/Communication/A2DPStream.h"
#endif

// Definimos el stream para Audiokit
DriverPins powadcr_pins;
AudioBoard powadcr_board(audio_driver::AudioDriverES8388,powadcr_pins);
AudioBoardStream kitStream(powadcr_board);

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
#include "BlockProcessor.h"

#include "TZXprocessor.h"
#include "TAPprocessor.h"

//File sdFile;
//File sdFile32;

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
// #include "esp_err.h"
// #include "esp_spiffs.h"

#ifdef FTP_SERVER_ENABLE
  #include "ESP32FtpServer.h"
  FtpServer ftpSrv;
#endif


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
void showOption(String id, String value);
String getFileNameFromPath(const String &filePath);
String removeExtension(const String &filename);

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
bool statusPoweLed = false;
bool powerLedFixed = false;

// Bluetooth
#ifdef BLUETOOTH_ENABLE
  BluetoothA2DPSource a2dp;
  SynchronizedNBuffer buffer(1024,30, portMAX_DELAY, 10);
  QueueStream<uint8_t> outbt(buffer); // convert Buffer to Stream

  // Provide data to A2DP
  int32_t get_data(uint8_t *data, int32_t bytes) {
    size_t result_bytes = buffer.readArray(data, bytes);
    //LOGI("get_data_channels %d -> %d of (%d)", bytes, result_bytes , buffer.available());
    return result_bytes;
  }
#endif

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

String getValueOfParam(String line, String param)
{
    #ifdef DEBUGMODE
        logln("Cfg. line: " + line);
        log(" - Param: " + param);
    #endif

    int firstClosingBracket = line.indexOf('>');

    if( firstClosingBracket != -1)
    {
        // int paramLen = param.length();
        int firstOpeningEndLine = line.indexOf("</",firstClosingBracket + 1);  
        
        if (firstOpeningEndLine != -1)
        {
            String res = line.substring(firstClosingBracket+1, firstOpeningEndLine);
            #ifdef DEBUGMODE
                log(" / Value = " + res);
            #endif
            return res;
        }
    }
    
    return "null";
}

tConfig* readAllParamCfg(File mFile, int maxParameters)
{
    tConfig* cfgData = new tConfig[maxParameters+1];

    // Vamos a ir linea a linea obteniendo la información de cada parámetro.
    char line[256];
    String tLine;
    //
    int n;
    int i=0;

    // Vemos si el fichero ya está abierto
    if (mFile)
    {
        // read lines from the file
        while (mFile.available()) 
        {
            tLine = mFile.readStringUntil('\n');
            tLine.toCharArray(line, sizeof(line));
            // remove '\n'
            line[n-1] = 0;
            String strRes = "";

            strRes = line;
            #ifdef DEBUGMODE
                logln("[" + String(i) + "]" + strRes);
            #endif

            if (i<=maxParameters)
            {
                cfgData[i].cfgLine = strRes;
                cfgData[i].enable = true;
            }
            else
            {
                log("Out of space for cfg parameters. Max. " + String(maxParameters));
            }

            i++;

        }
    }

    return cfgData;
}

bool loadWifiCfgFile()
{

  bool cfgloaded = false;

  if (SD_MMC.exists("/wifi.cfg"))
  {
#ifdef DEBUGMODE
    logln("File wifi.cfg exists");
#endif

    char pathCfgFile[10] = {};
    strcpy(pathCfgFile, "/wifi.cfg");

    File fWifi = SD_MMC.open(pathCfgFile,FILE_READ);
    int *IP;

    if (fWifi)
    {
      // HOSTNAME = new char[32];
      // ssid = new char[64];
      // password = new char[64];

      char *ip1 = new char[17];

      CFGWIFI = readAllParamCfg(fWifi, 9);

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
      strcpy(HOSTNAME, (getValueOfParam(CFGWIFI[0].cfgLine, "hostname")).c_str());
      logln(HOSTNAME);
      // SSID - Wifi
      ssid = (getValueOfParam(CFGWIFI[1].cfgLine, "ssid")).c_str();
      logln(ssid);
      // Password - WiFi
      strcpy(password, (getValueOfParam(CFGWIFI[2].cfgLine, "password")).c_str());
      logln(password);

      //Local IP
      strcpy(ip1, (getValueOfParam(CFGWIFI[3].cfgLine, "IP")).c_str());
      logln("IP: " + String(ip1));
      POWAIP = ip1;
      IP = strToIPAddress(String(ip1));
      local_IP = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // Subnet
      strcpy(ip1, (getValueOfParam(CFGWIFI[4].cfgLine, "SN")).c_str());
      logln("SN: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      subnet = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // gateway
      strcpy(ip1, (getValueOfParam(CFGWIFI[5].cfgLine, "GW")).c_str());
      logln("GW: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      gateway = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS1
      strcpy(ip1, (getValueOfParam(CFGWIFI[6].cfgLine, "DNS1")).c_str());
      logln("DNS1: " + String(ip1));
      IP = strToIPAddress(String(ip1));
      primaryDNS = IPAddress(IP[0], IP[1], IP[2], IP[3]);

      // DNS2
      strcpy(ip1, (getValueOfParam(CFGWIFI[7].cfgLine, "DNS2")).c_str());
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
    File fWifi;
    if (!SD_MMC.exists("/wifi_ori.cfg"))
    {
      //fWifi.open("/wifi_ori.cfg", O_WRITE | O_CREAT);
      fWifi = SD_MMC.open("/wifi_ori.cfg", FILE_WRITE);

      if (fWifi)
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

  pTZX.process(file_ch);

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
    //SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS, SHARED_SPI, SD_SCK_MHZ(SD_SPEED_MHZ), &SPI);
    // if () {
    
  //}
    if (!SD_MMC.begin() || lastStatus)
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
      if (!SD_MMC.open("/"))
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
        
        if (!SD_MMC.exists("/_test"))
        {
          if (!SD_MMC.open("_test", FILE_WRITE))
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
            SD_MMC.remove("/_test");
            //sdFile32.close();
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
  //-----------------------------------------------------------
  //
  // Esta rutina graba en WAV el audio que entra por LINE IN
  //
  //-----------------------------------------------------------
  unsigned long progress_millis = 0;
  unsigned long progress_millis2 = 0;
  int rectime_s = 0;
  int rectime_m = 0;
  size_t wavfilesize = 0;

  AudioInfo new_sr = kitStream.defaultConfig();
  // Guardamos la configuracion de sampling rate
  SAMPLING_RATE = new_sr.sample_rate;

  new_sr.sample_rate = DEFAULT_WAV_SAMPLING_RATE_REC;
  kitStream.setAudioInfo(new_sr);       
  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_WAV_SAMPLING_RATE_REC/1000)) + "KHz\"" );

  const size_t BUFFER_SIZE = 4096;
  int16_t buffer[BUFFER_SIZE];

  EncodedAudioStream encoder(&wavfile, new WAVEncoder());
  NumberFormatConverterStreamT<int16_t, uint8_t> nfc(kitStream); // Convierte de 16-bits a 8-bits);

  //
  // Definimos el multi-output
  //MultiOutput cmulti;   

  // Definición de los copiers
  StreamCopy copier8bWAV(encoder, nfc);
  StreamCopy copier(encoder, kitStream); // copies data to both file and line_out
  StreamCopy copierOutput(kitStream, kitStream); // copies data to both file and line_out

  // Esperamos a que la pantalla esté lista
  LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";
  recAnimationOFF();
  delay(125);
  recAnimationFIXED_ON();
  tapeAnimationON();

  // Agregamos las salidas al multiple
  // cmulti.add(encoder);
  // cmulti.add(kitStream);

  // Inicializamos los copiers
  AudioInfo ecfg = encoder.defaultConfig();
  ecfg.sample_rate = DEFAULT_WAV_SAMPLING_RATE_REC;

  if (WAV_8BIT_MONO)
  {
    // Configuramos el encoder para 8-bit mono
    ecfg.bits_per_sample = 8; 
    ecfg.channels = 1; // Mono
  }
  
  encoder.begin(ecfg);


  // Inicializamos encoders
  nfc.begin();

  // Deshabilitamos el amplificador de salida
  // Con esto ajustamos la salida a la del recorder.
  showOption("menuAudio.mutAmp.val",String(false));
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER); 
  STOP = false;

  WAVFILE_PRELOAD = false;
  copier.setSynchAudioInfo(true);
  copier8bWAV.setSynchAudioInfo(true);

  if(WAV_8BIT_MONO)
  {
    copier8bWAV.begin();
  }
  else
  {
    copier.begin();
  }

  // loop de grabacion
  while (!STOP)
  {
    // Grabamos a WAV file
    wavfilesize += WAV_8BIT_MONO ? copier8bWAV.copy() : copier.copy();    
    // if (WAV_8BIT_MONO)
    // {
    //   //copier8bitPCM.copy();
    //   wavfilesize += copier8bWAV.copy();
    // }
    // else
    // {
    //   wavfilesize += copier.copy();
    // }

    // Sacamos audio por la salida

    if ((millis() - progress_millis) > 1000)
    {
      rectime_s++;

      if (rectime_s > 59)
      {
        rectime_s = 0;
        rectime_m++;
      }
     
      progress_millis = millis();
    }

    if ((millis() - progress_millis2) > 2000)
    {
      LAST_MESSAGE = "Recording time: " + ((rectime_m < 10 ? "0" : "") + String(rectime_m)) + ":" + ((rectime_s < 10 ? "0" : "") + String(rectime_s));     
      hmi.writeString("size.txt=\"" + String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024 : wavfilesize / 1024) + 
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

   hmi.writeString("size.txt=\"" + String(wavfilesize > 1000000 ? wavfilesize / 1024 / 1024 : wavfilesize / 1024) + 
                                         (wavfilesize > 1000000 ? " MB" : " KB") + "\"");
  
  // Cerramos el stream
  copier8bWAV.end();
  copier.end();
  encoder.end();
  nfc.end();

  // Cerramos el fichero WAV
  wavfile.flush();
  wavfile.close();


  // Devolvemos la configuracion a la board.
  new_sr.sample_rate = SAMPLING_RATE;
  new_sr.bits_per_sample = 16;
  new_sr.channels = 2;

  kitStream.setAudioInfo(new_sr);      
  
  // Cambiamos el sampling rate en el HW
  // Indicamos
  hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );  
  //
  showOption("menuAudio.mutAmp.val",String(ACTIVE_AMP));
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);

  WAVFILE_PRELOAD = true;
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
  File file;
  String ans = "";

  char strpath[20] = {};
  strcpy(strpath, "/powadcr_iface.tft");

  file = SD_MMC.open(strpath,FILE_READ);
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
  file.seek(0);

  // Enviamos los datos

  uint8_t buf[4096];
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
            file.seek(0);
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
    // Limpiamos el nombre del fichero
    memset(file_name, 0, sizeof(file_name));

    // Si es una copia de un TAPE llevara un FILE_LOAD pero,
    // si no, hay que generarlo
    if (!PLAY_TO_WAV_FILE)
    {   
      // Generamos el nombre del fichero WAV SIEMPRE, cuando es grabación a WAV y no PLAY a WAV.
      char *cPath = (char *)ps_calloc(55, sizeof(char));
      getRandomFilenameWAV(cPath, wavfileBaseName);
      wavnamepath = String(cPath);
      free(cPath);
    }
    else
    {
      // Si es PLAY a WAV
      if (FILE_LOAD.length() > 0 && PLAY_TO_WAV_FILE)
      {
          // Cogemos el nombre del TAP/TZX
          wavnamepath = "/WAV/" + removeExtension(FILE_LOAD) + ".wav";
      }
      else if (FILE_LOAD.length() < 1 && PLAY_TO_WAV_FILE)
      {
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
    if (wavfile)
    {
      wavfile.close();
      logln("WAV file already open. Closing it.");
    }
 
    // open file for recording WAV
    wavfile = SD_MMC.open(file_name, FILE_WRITE);
    FILE_LOAD = getFileNameFromPath(wavnamepath);    

    // Muestro solo el nombre. Le elimino la primera parte que es el directorio.
    hmi.writeString("name.txt=\"" + String(file_name).substring(5) + "\"");
    hmi.writeString("type.txt=\"[to WAV file]\"");

    if (!wavfile)
    {
      logln("Error open file to output playing");
      LAST_MESSAGE = "Error output playing to WAV.";
      delay(2000);

      //OUT_TO_WAV = false;
      STOP = true;
      return;
    }
    else
    {
      logln("Out to WAV file. Ready!");
    }


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

int getStreamfileSizeWithFilename(const String &path, bool showInHMI = true) 
{
    
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
    if (showInHMI)
    {    String sizeStr = (fileSize < 1000000) 
      ? String(fileSize / 1024) + " KB" 
      : String(fileSize / 1024 / 1024) + " MB";
      hmi.writeString("size.txt=\"" + sizeStr + "\"");
    }

    return fileSize;
}

int getStreamfileSize(File* pFile, bool showInHMI = false) 
{
    size_t fileSize = 0;
    if(pFile!=nullptr)  
    {
        fileSize = pFile->size();
    }

    // Actualizamos el tamaño en el HMI
    if (showInHMI)
    {    String sizeStr = (fileSize < 1000000) 
      ? String(fileSize / 1024) + " KB" 
      : String(fileSize / 1024 / 1024) + " MB";
      hmi.writeString("size.txt=\"" + sizeStr + "\"");
    }

    return fileSize;
}

void updateSamplingRateIndicator(int rate, int bps, String ext)
{
  // Cambiamos el sampling rate a 22KHz para el kitStream
  //                
  if (rate > 0)
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(rate / 1000)) + "KHz\"");
  else
    hmi.writeString("tape.lblFreq.txt=\"-- KHz\"");
  

  if (ext == "wav")
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
  else
  {
      hmi.writeString("tape.wavind.txt=\"\"");
  }
}

void updateIndicators(int size, int pos, uint32_t fsize, int bitrate, String fname)
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

  if (fname != LASTFNAME)
  {
    // Actualizamos el nombre del fichero
    LASTFNAME = fname;
    // Actualizamos el nombre del fichero en la pantalla
    hmi.writeString("tape.fileName.txt=\"" + fname + "\"");
  }
  {
    hmi.writeString("mp3browser.path.txt=\"" + fname + "\"");
  }

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

void updateSamplingRate(AudioPlayer &player, Equalizer3Bands &eq, AudioInfo realInfo)
{      
    logln("Reading sampling rate and updating.");

    if (realInfo.sample_rate > 0) 
    {
        // Actualizamos la configuración con los valores reales
        kitStream.setAudioInfo(realInfo);
        eq.setAudioInfo(realInfo);
        player.setAudioInfo(realInfo);
        //
        logln("Real Audio Info - Sample Rate: " + String(realInfo.sample_rate) + 
          "Hz, Bits: " + String(realInfo.bits_per_sample) + 
          ", Channels: " + String(realInfo.channels));
        
          // Mostramos la información
        hmi.writeString("tape.lblFreq.txt=\"" + String(int(realInfo.sample_rate/1000)) + "KHz\"" );
        delay(125);
    }
    else 
    {
        logln("Warning: Invalid sample rate detected in audio file");
        hmi.writeString("tape.lblFreq.txt=\" -- KHz\"" );
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

String removeExtension(const String &filename) {
    int dotIndex = filename.lastIndexOf('.');
    if (dotIndex > 0) {
        return filename.substring(0, dotIndex);
    }
    return filename; // Si no hay punto, devuelve el nombre tal cual
}

String getFileNameFromPath(const String &filePath) {
  int lastSlash = filePath.lastIndexOf('/');
  if (lastSlash == -1) {
      lastSlash = filePath.lastIndexOf('\\'); // Por si se usa '\' como separador
  }
  if (lastSlash != -1) {
      return filePath.substring(lastSlash + 1); // Devuelve el nombre del archivo con su extensión
  }
  return filePath; // Si no hay separador, devuelve la cadena completa
}

int generateAudioList(tAudioList* &audioList, String extension = ".mp3") {
    audioList = (tAudioList*)ps_calloc(MAX_FILES_AUDIO_LIST, sizeof(tAudioList));
    int size = 0;

    String parentDir = FILE_LAST_DIR;
    if (!parentDir.endsWith("/")) parentDir += "/";
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

    char *line = (char*)ps_calloc(512, sizeof(char));
    String strline;
    int indexCounter = 0; // Contador para el índice de AudioTools

    try {
        logln("Generating audio list and index files for extension: " + extension);
        
        while (filesListFile.available() && size < MAX_FILES_AUDIO_LIST) 
        {
            // Limpiar buffer
            memset(line, 0, 512);
            
            int bytesRead = filesListFile.readBytesUntil('\n', line, 511);
            if (bytesRead == 0) continue;
            
            strline = String(line);
            strline.trim();
            if (strline.length() == 0) continue;

            // Parseamos los 4 campos: indice|tipo|tamaño|nombre|
            int idx1 = strline.indexOf('|');
            int idx2 = strline.indexOf('|', idx1 + 1);
            int idx3 = strline.indexOf('|', idx2 + 1);
            int idx4 = strline.indexOf('|', idx3 + 1);

            if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) continue;

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
                    logln("Added to audio list and index: " + nombre + " (index: " + String(size) + ")");
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
            // Crear la clave de definición (formato requerido por AudioSourceIdxSDMMC)
            String keyDef = parentDir + "|" + extension + "|*";
            idxDefFile.println(keyDef);
            idxDefFile.close();
            
            logln("Created idx-def.txt with key: " + keyDef);
        } else {
            logln("Warning: Could not create idx-def.txt");
        }
        
    } catch (const std::exception& e) {
        logln("Exception in generateAudioList: " + String(e.what()));
    } catch (...) {
        logln("Unknown exception in generateAudioList");
    }

    // ✅ Limpieza garantizada
    free(line);
    line = nullptr;
    
    filesListFile.close();
    
    // Asegurar que los archivos de índice estén cerrados
    if (idxFile) idxFile.close();
    if (idxDefFile) idxDefFile.close();
    
    TOTAL_BLOCKS = size;
    logln("Generated audio list with " + String(size) + " files.");
    logln("Created index files: idx.txt (" + String(indexCounter) + " entries) and idx-def.txt");
    
    return size;
}

int generateAudioList_old(tAudioList* &audioList, String extension = "mp3") {
    audioList = (tAudioList*)ps_calloc(MAX_FILES_AUDIO_LIST, sizeof(tAudioList));
    int size = 0;

    String parentDir = FILE_LAST_DIR;
    if (!parentDir.endsWith("/")) parentDir += "/";
    String filesListPath = parentDir + "_files.lst";
    String fileSelected = getFileNameFromPath(PATH_FILE_TO_LOAD);

    File filesListFile = SD_MMC.open(filesListPath.c_str(), FILE_READ);
    if (!filesListFile) {
        logln("Error: Cannot open " + filesListPath);
        return 0;
    }

    char *line = (char*)ps_calloc(512, sizeof(char));
    String strline;

    try {
      while (filesListFile.available() && size < MAX_FILES_AUDIO_LIST) 
      {

          filesListFile.readBytesUntil('\n', line, 511);
          strline = String(line);
          strline.trim();
          if (strline.length() == 0) continue;

          // Parseamos los 4 campos: indice|tipo|tamaño|nombre|
          int idx1 = strline.indexOf('|');
          int idx2 = strline.indexOf('|', idx1 + 1);
          int idx3 = strline.indexOf('|', idx2 + 1);
          int idx4 = strline.indexOf('|', idx3 + 1);

          if (idx1 == -1 || idx2 == -1 || idx3 == -1 || idx4 == -1) continue;

          String tipo = strline.substring(idx1 + 1, idx2);
          String nombre = strline.substring(idx3 + 1, idx4);
          String tamano = strline.substring(idx2 + 1, idx3);
          String indice = strline.substring(0, idx1);

          tipo.trim();
          nombre.trim();

          // Solo ficheros y extensión coincidente
          String nombreLower = nombre;
          nombreLower.toLowerCase();
          String extensionLower = extension;
          extensionLower.toLowerCase();
          if (tipo == "F" && nombreLower.endsWith(extensionLower)) {
              audioList[size].index = indice.toInt();
              audioList[size].filename = nombre;
              audioList[size].path = parentDir;
              audioList[size].size = tamano.toInt();

              // capturamos el indice del fichero seleccionado en el browser
              if (audioList[size].filename.equalsIgnoreCase(fileSelected)) {
                  MEDIA_CURRENT_POINTER = size;
              }
              size++;
          }
      }
    }
    catch (...) {
        // Aseguramos que el archivo se cierre incluso si hay errores
        free(line);
        filesListFile.close();
        throw;
    }

    free(line);
    filesListFile.close(); // Aseguramos que el archivo se cierre
    TOTAL_BLOCKS = size;
    logln("Generating audio list with " + String(size) + " files.");    
    return size;
}

int getIndexFromAudioList(tAudioList* audioList, int size, const String &searchValue) 
{
    for (int i = 0; i < size; i++) 
    {
        //logln("Comparing: " + audioList[i].filename + " with " + searchValue);

        if (audioList[i].filename.equalsIgnoreCase(searchValue)) 
        {
            return audioList[i].index; // Retorna el índice del archivo encontrado
        }
    }
    return -1; // Retorna -1 si no se encuentra el archivo
}

void nextAudio(int &currentPointer, int audioListSize)
{
    // Avanzamos al siguiente archivo
    currentPointer++;
    if (currentPointer >= audioListSize) 
    {
        currentPointer = 0; // Volvemos al inicio si llegamos al final
    }
}

void prevAudio(int &currentPointer, int audioListSize)
{
    // Retrocedemos al archivo anterior
    currentPointer--;
    if (currentPointer < 0) 
    {
        currentPointer = audioListSize - 1; // Volvemos al final si llegamos al inicio
    }
}

void MediaPlayer() 
{   
    
    // ---------------------------------------------------------
    //
    // Reproductor de medios
    //
    // ---------------------------------------------------------

    LAST_MESSAGE = "Waiting...";


    // Variables
    // ---------------------------------------------------------
    // Lista de audio
    int audioListSize = 0;
    tAudioList* audiolist; 

    String ext = TYPE_FILE_LOAD;
    ext.toLowerCase();

    File* pFile = nullptr;

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
    AudioSourceIdxSDMMC source(FILE_LAST_DIR.c_str(), ext.c_str(),false);
    //REGENERATE_IDX = false;

    // Configuración de los decodificadores
    // ---------------------------------------------------------
    // Decodificador MP3
    MP3DecoderHelix decoderMP3;
    
    // Decodificador WAV tipo PCM
    WAVDecoder decoderWAV;

    // Decodificador FLAC
    FLACDecoderFoxen decoderFLAC;

    // Configuración del filtrado de metadatos para el codec MP3
    // ---------------------------------------------------------
    MetaDataFilterDecoder metadatafilter(decoderMP3);  

    // Configuración de las mediciones de tiempo
    // ---------------------------------------------------------
    //MeasuringStream measureWAV(decoderWAV);
    MeasuringStream measureMP3(metadatafilter);
    //MeasuringStream measureFLAC(decoderFLAC);
 
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

    // Esto nos permite propagación del setting del fichero, sampling, bits, canales.
    //WAV
    decoderWAV.addNotifyAudioChange(kitStream);
    decoderWAV.addNotifyAudioChange(eq);
    //MP3
    metadatafilter.addNotifyAudioChange(measureMP3);
    decoderMP3.addNotifyAudioChange(kitStream);
    decoderMP3.addNotifyAudioChange(eq);
    //FLAC
    decoderFLAC.addNotifyAudioChange(kitStream);
    decoderFLAC.addNotifyAudioChange(eq);

    // Configuración del reproductor
    // ---------------------------------------------------------
    AudioInfo audiosr;
    AudioPlayer player;

    player.setAudioSource(source);
    player.setOutput(eq);
    
    player.setVolume(1);
    
    auto tempConfig = kitStream.audioInfo();

    // Esto es necesario para que el player sepa donde rediregir el audio
    switch(ext[0])
    {
      case 'w':
        // WAV
        // Iniciamos el decoder
        decoderWAV.begin();
        
        // Configuramos el player con el decoder
        player.setDecoder(decoderWAV);
        
        // Configuramos temporalmente el audio a 22050Hz hasta que leamos el archivo real
        tempConfig = kitStream.defaultConfig();
        tempConfig.sample_rate = 44100;
        tempConfig.bits_per_sample = 16;
        tempConfig.channels = 2;
        kitStream.setAudioInfo(tempConfig);
        eq.setAudioInfo(tempConfig);
        
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
        
        // Configuramos temporalmente el audio a 44100Hz hasta que leamos el archivo real
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
        //measureFLAC.begin();

        if (decoderFLAC.begin())
        {
          logln("FLAC decoder initialized");
          
          // Configuramos temporalmente el audio a 44100Hz hasta que leamos el archivo real
          tempConfig = kitStream.defaultConfig();
          tempConfig.sample_rate = 44100;
          tempConfig.bits_per_sample = 16;
          tempConfig.channels = 2;
          kitStream.setAudioInfo(tempConfig);
          eq.setAudioInfo(tempConfig);
        }
        else
        {
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
      if (BLUETOOTH_ACTIVE)
      {
        player.setOutput(outbt);
      }
    #endif

    LAST_MESSAGE = "Scanning files ...";

    // Apuntamos al fichero seleccionado en el browser
    logln("Selecting file: " + PATH_FILE_TO_LOAD);
    
    pFile = (File*)source.selectStream(PATH_FILE_TO_LOAD.c_str()); 
    if (pFile == nullptr) 
    {
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
    //yield();
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
    // Regeneramos el indice del player.
    // String parentDir = FILE_LAST_DIR;
    // if (!parentDir.endsWith("/")) parentDir += "/";
    //source.end();
    //source.reindex();
    
    //
    logln("Checking path: " + FILE_LAST_DIR);
    if (!SD_MMC.exists((FILE_LAST_DIR).c_str()))
    {
        logln("Path not found -> " + FILE_LAST_DIR);
        LAST_MESSAGE = "Path not found.";
        hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
        delay(125);
        return;
    }

    // Reindexamos la fuente de audio.
    //source.reindex((FILE_LAST_DIR).c_str(), ext.c_str()); //*****  06102025 Prueba con la AudioTools original (se necesita eliminar esto)
    delay(125);
    
    // Obtenemos indice del fichero seleccionado
    currentPointer = MEDIA_CURRENT_POINTER;
    logln("Current pointer: " + String(currentPointer) + " - File: " + audiolist[currentPointer].filename);

    // Mostramos mensaje de creación de la lista de audio
    LAST_MESSAGE = "Audio list ready.";
    hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
    delay(125);
    hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");
    delay(125);

    // Asignación de variables
    totalFilesIdx = audioListSize;
    TOTAL_BLOCKS = totalFilesIdx + 1;
    BLOCK_SELECTED = source.index() + 1; // Para mostrar el bloque seleccionado en la pantalla
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
    updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead, audiolist[currentPointer].filename);
    
    // -------------------------------------------------------------------
    // -
    // - Arranque del player
    // -
    // -------------------------------------------------------------------
    LAST_MESSAGE = "Initializing player...";
    hmi.writeString("statusLCD.txt=\"" + LAST_MESSAGE + "\"");

    waitflag = 0;
    while(!player.begin() && !EJECT) 
    {
      if(waitflag++>32)   //Esperamos un maximo de 32*125ms = 4s
      {
        LAST_MESSAGE = "Error indexing files. Open again.";
        delay(2000);
        break;
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
        //kitStream.setVolume(MAIN_VOL / 100);
        //kitStream.setPAPower(ACTIVE_AMP & EN_SPEAKER);
        
        // Control del ecualizador
        if (EQ_CHANGE) 
        {
            EQ_CHANGE = false;
            cfg = kitStream.audioInfo();
            cfg_eq.setAudioInfo(cfg);
            cfg_eq.gain_low = EQ_LOW;
            cfg_eq.gain_medium = EQ_MID;
            cfg_eq.gain_high = EQ_HIGH;

            if (!eq.begin(cfg_eq))
            {
                LAST_MESSAGE = "Error EQ initialization";
                STOP = true;
                PLAY = false;
                break;
            }
        }

        if (AMP_CHANGE || SPK_CHANGE)
        {
            kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);
            AMP_CHANGE = false;
            SPK_CHANGE = false;
        }

        // Estados del reproductor
        switch (stateStreamplayer) 
        {
            case 0: // Esperando reproducción
                if (PLAY) 
                {
                    // Iniciamos el reproductor
                    
                    if (WAVFILE_PRELOAD) WAVFILE_PRELOAD = false;

                    // Primer ms
                    waitflag = 0;
                    while(!player.begin(currentPointer))
                    {
                        if(waitflag++>255){player.stop();}
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

                        if (format != AudioFormat::PCM)
                        {
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

                    audiosr = (ext == "wav") ? decoderWAV.audioInfo() : (ext == "mp3") ? decoderMP3.audioInfo() : decoderFLAC.audioInfo();
                    updateSamplingRate(player, eq, audiosr);

                    LAST_MESSAGE = "...";

                    pFile = (File*)player.getStream();

                    if (pFile != nullptr) 
                    {
                        // Obtenemos el tamaño del archivo
                        fileSize = pFile->size();
                        logln("File size: " + String(fileSize) + " bytes");
                        // Inicializamos el número de bytes leídos
                        fileread = pFile->position(); 
                    } 

                    // Actualización inicial de indicadores
                    updateIndicators(totalFilesIdx, currentPointer+1, fileSize, bitRateRead, source.toStr());

                    // Cambiamos de estado
                    stateStreamplayer = 1;
                    tapeAnimationON();                      
                  
                }
                break;

            case 1: // Reproduciendo PLAY
              
                // Salida inesperada desde MediaPlayer.
                if (!AUDIO_FORMART_IS_VALID)
                {
                  LAST_MESSAGE = "Audio format error.";
                  STOP = true;
                  PLAY = false;
                  stateStreamplayer = 0;
                  tapeAnimationOFF();
                  break;
                }

                // Con esto evitamos que se oiga el player mientras rebobinamos
                if (fast_wind_status <= 2)
                {
                  if (CHANGE_TRACK_FILTER)
                  {
                    // Cambio de pista
                    kitStream.setVolume(0);
                    delay(250);
                    player.copy();
                    player.copy();
                    kitStream.setVolume(MAIN_VOL / 100);
                    CHANGE_TRACK_FILTER = false;
                  }
                  else
                  {
                    // Reproducción normal
                    player.copy();
                  }
                    
                  fileread = pFile->position(); // Actualizamos el número de bytes leídos

                  if (fileread < 128)
                  {
                    // En los primeros 128 bytes actualizo el sampling rate
                    // en los que aseguro haber leido la cabecera
                    audiosr = (ext == "wav") ? decoderWAV.audioInfo() : (ext == "mp3") ? decoderMP3.audioInfo() : decoderFLAC.audioInfo();
                    updateSamplingRate(player, eq, audiosr);
                  }
                }

                // Cuando haya datos que mostrar, visualizamos la barra de progreso
                if (fileSize > 0) 
                {
                    // Esto lo hacemos asi para evitar hacer out of range del tipo de variable.
                    PROGRESS_BAR_TOTAL_VALUE = (fileread / (fileSize / 100));
                }
                
                // Control de EOF
                if (fileread == fileSize) 
                {
                  // Se ha alcanzado el ultimo fichero de la lista y no hay mas
                  if ((currentPointer + 1) >= (TOTAL_BLOCKS-1))
                  {
                      // Modalidad 1:
                      // El fichero es el ultimo y no hay loop
                      if (!disable_auto_media_stop)
                      {
                        fileread = 0;
                        stateStreamplayer = 4; // Auto-stop    
                        hmi.writeString("tape.currentBlock.val=1");  
                        updateIndicators(totalFilesIdx, 1, fileSize, bitRateRead, audiolist[0].filename);
                        delay(125);
                      }
                      // Modalidad 2:
                      // El fichero es el ultimo y pero se hace loop
                      else
                      {
                        //
                        // Comenzamos desde el principio de la playlist
                        //
                        BLOCK_SELECTED = 0;
                        
                        currentPointer = currentIdx = 0; // Reiniciamos el índice
                        fileread = 0;          
                        hmi.writeString("tape.currentBlock.val=1");  
                        delay(125);
                        //LAST_MESSAGE = "Searching...";
                        //currentIdx = source.indexOf((audiolist[currentPointer].filename).c_str());
                        // currentIdx = source.indexOf((audiolist[currentPointer].filename).c_str());
                        // if (currentIdx == -1) 
                        // {
                        //     logln("File not found in source: " + audiolist[currentPointer].filename);
                        //     LAST_MESSAGE = "File not found in playlist.";
                        //     STOP = true;
                        //     PLAY = false;
                        //     break;
                        // }
                        // else
                        // {
                        //     LAST_MESSAGE = "...";
                        // }
                        waitflag = 0;
                        while(!player.begin(currentIdx))
                        {
                            if(waitflag++>255){player.stop();}
                        }
                        // Reiniciar el reproductor
                        fileSize = getStreamfileSize(pFile);
                        updateIndicators(totalFilesIdx, currentPointer, fileSize, bitRateRead, audiolist[currentPointer].filename);
                        delay(125);

                      }
                  }
                  else
                  {
                    // El player continua solo a la siguiente pista no hace falta forzarlo
                    // actualizamos el puntero.
                    currentPointer++;
                    currentIdx = currentPointer;
                    // Esto es un nuevo fichero en el player
                    //player.stop();
                    
                    fileread = 0;
                    //
                    //LAST_MESSAGE = "Searching...";
                    //currentIdx = source.indexOf(audiolist[currentPointer].filename.c_str());
                    // currentIdx = source.indexOf((audiolist[currentPointer].filename).c_str());
                    // if (currentIdx == -1) {
                    //     logln("File not found in source: " + audiolist[currentPointer].filename);
                    //     LAST_MESSAGE = "File not found in playlist.";
                    //     STOP = true;
                    //     PLAY = false;
                    //     break;
                    // }
                    // else
                    // {
                    //     LAST_MESSAGE = "...";
                    // }                    
                    waitflag = 0;
                    // Iniciamos el reproductor
                    player.stop();
                    while(!player.begin(currentIdx))
                    {
                        if(waitflag++>255){player.stop();}
                    }

                    fileSize = getStreamfileSize(pFile);
                    updateIndicators(totalFilesIdx, currentPointer, fileSize, bitRateRead, audiolist[currentPointer].filename);
                    delay(125);
                  }                 
                }

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

                // Actualizamos indicadores cada 2 segundos
                // En la actualización de indicadores cada segundo

                if (millis() - lastUpdate > 1000) 
                {
                    // Mostramos informacion del fichero.
                    updateIndicators(totalFilesIdx, currentPointer+1, fileSize, bitRateRead, audiolist[currentPointer].filename);
                
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
                        //logln("Song changed - resetting timer for: " + audiolist[currentPointer].filename);
                    }
                    
                    // Inicializar el tiempo de inicio la primera vez
                    if (!timeInitialized) {
                        playStartTime = millis();
                        timeInitialized = true;
                    }
                    
                    // Verificar si han pasado xx segundos desde el inicio
                    //unsigned long elapsedTime = millis() - playStartTime;
                    //bool showTime = (elapsedTime >= TIME_TO_SHOW_ESTIMATED_TIME * 1000); // 40 segundos = 10000 ms

                    //bool showTime = PROGRESS_BAR_TOTAL_VALUE > PER_TO_SHOW_ESTIMATED_TIME;
                
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
                        }
                        else
                        {
                            // OPCIÓN 2: Fallback con estimación basada en bitrate promedio MP3
                            
                            // Intentar obtener el bitrate real del MP3 si está disponible
                            uint32_t mp3_bitrate = 0;
                            
                            // Algunos decodificadores pueden proporcionar el bitrate
                            // Si no está disponible, usar estimación basada en tamaño/tiempo conocido
                            
                            if (mp3_bitrate == 0) 
                            {
                                // Estimación basada en bitrates comunes de MP3
                                // Análisis heurístico del tamaño del archivo
                                if (fileSize < 2000000) // < 2MB, probablemente 96-128 kbps
                                {
                                    mp3_bitrate = 112000; // 112 kbps promedio
                                }
                                else if (fileSize < 5000000) // < 5MB, probablemente 128-192 kbps  
                                {
                                    mp3_bitrate = 160000; // 160 kbps promedio
                                }
                                else // > 5MB, probablemente 192-320 kbps
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
                            logln("Calculated elapsed time: " + String(stime_elapsed) + " seconds");
                        #endif

                
                        // Validar que los tiempos sean razonables
                        if (stime_total > 86400) // Si es más de 24 horas, probablemente hay error
                        {
                            stime_total = fileSize / 16000; // Usar estimación básica
                            stime_elapsed = pFile->position() / 16000;
                        }
                
                        // Limitar tiempos a máximo 59:59
                        if (stime_total > 3599) stime_total = 3599; // 59:59 = 3599 segundos
                        if (stime_elapsed > 3599) stime_elapsed = 3599; // 59:59 = 3599 segundos
                        
                        // Asegurar que el tiempo transcurrido no sea mayor que el total
                        if (stime_elapsed > stime_total) stime_elapsed = stime_total;

                        // Convertir tiempo transcurrido
                        uint32_t tiempoElapsed = stime_elapsed;
                        uint32_t minutosElapsed = tiempoElapsed / 60;
                        uint32_t segundosElapsed = tiempoElapsed % 60;
                        
                        // Limitar minutos a máximo 59
                        if (minutosElapsed > 59) minutosElapsed = 59;
                        
                        String tiempoElapsedStr = (minutosElapsed < 10 ? "0" : "") + String(minutosElapsed) + ":" + 
                                                  (segundosElapsed < 10 ? "0" : "") + String(segundosElapsed);

                        // Convertir tiempo total
                        uint32_t tiempoTotal = stime_total;
                        uint32_t minutosTotal = tiempoTotal / 60;
                        uint32_t segundosTotal = tiempoTotal % 60;
                        
                        // Limitar minutos a máximo 59
                        if (minutosTotal > 59) minutosTotal = 59;
                        
                        String tiempoTotalStr = (minutosTotal < 10 ? "0" : "") + String(minutosTotal) + ":" + 
                                                (segundosTotal < 10 ? "0" : "") + String(segundosTotal);

                        // Mostrar ambos tiempos
                        LAST_MESSAGE = "Time: " + tiempoElapsedStr + " /  " + tiempoTotalStr;
                        
                        #ifdef DEBUG
                            logln("Elapsed seconds: " + String(stime_elapsed) + " Total seconds: " + String(stime_total));
                            logln("File size: " + String(fileSize) + " File read: " + String(fileread));
                        #endif
                    } else {
                        // Mostrar mensaje de espera durante los primeros 40 segundos
                        //int secondsRemaining = TIME_TO_SHOW_ESTIMATED_TIME - (elapsedTime / 1000);
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
                player.stop();
                tapeAnimationOFF();

                // Reiniciamos el índice
                BLOCK_SELECTED = 0;
                currentPointer = currentIdx = 0;   
                waitflag = 0;
                while(!source.selectStream(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
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
                while(!player.begin(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
                }
                break;

            default:
                break;
        }

        // Control de avance/retroceso
        if ((FFWIND || RWIND) && !was_pressed_wd) 
        {
            //LAST_MESSAGE = "Searching...";
            rewindAnimation(FFWIND ? 1 : -1);

            if (FFWIND) 
            {
              if ((currentPointer + 1) >= (TOTAL_BLOCKS-1))
              {
                currentPointer = currentIdx = 0; // Reiniciamos el índice
               
                waitflag = 0;
                while(!source.selectStream(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
                    delay(125);
                }                
              }
              else
              {
                currentPointer++;// = source.index();
                currentIdx = currentPointer;
                //currentIdx = source.indexOf((audiolist[currentPointer].filename).c_str());
                CHANGE_TRACK_FILTER = true;
                player.stop(); // Detener el reproductor
                waitflag = 0;
                while(!player.begin(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
                }
              }
               // Avanzamos al siguiente bloque
              logln("Next file: " + String(currentPointer + " - " + audiolist[currentPointer + 1].filename));
            } 
            else
            {
              // Modo origen solo se hace en reproducción. Si se pulsa RWD dentro del tiempo TIME_MAX_TO_PREVIOUS_TRACK
              // se pasa al fichero anterior, en otro caso se reproduce desde el principio.
              if ((millis() - twiceRWDTime > TIME_MAX_TO_PREVIOUS_TRACK) && PLAY)
              {
                // Empiezo desde el principio
                currentIdx = currentPointer;            
                player.stop();
                waitflag = 0;
                while(!player.begin(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
                }
                //
                twiceRWDTime = millis();
                CHANGE_TRACK_FILTER = true;
              }
              else
              {
                // Si pulso rapido en los proximos segundos               
                prevAudio(currentPointer, audioListSize);
                //LAST_MESSAGE = "Searching...";
                currentIdx = currentPointer;              
                waitflag = 0;
                while(!source.selectStream(currentIdx))
                {
                    if(waitflag++>255){player.stop();}
                    delay(125);
                }                
              }
            }

            // Control de EOF
            if (fileread >= fileSize) 
            {
                // Fin de la lista
                if ((currentPointer + 1) >= (TOTAL_BLOCKS-1))
                {
                      BLOCK_SELECTED = 0;
                      currentPointer = 0; // Reiniciamos el índice
                      currentIdx = currentPointer;
                      hmi.writeString("tape.currentBlock.val=" + String(currentPointer+1));
                      delay(125);
                      hmi.writeString("tape.currentBlock.val=" + String(currentPointer+1));
                      fileread = 0;                    
                      waitflag = 0;
                      while(!player.begin(currentIdx))
                      {
                          if(waitflag++>255){player.stop();}
                      }
                      // Reiniciar el reproductor
                      player.stop(); // Detener el reproductor
                      fileSize = getStreamfileSize(pFile);
                      STOP=true;
                      PLAY=false;
                }
            }
            else
            {
                if ((currentPointer + 1) > (TOTAL_BLOCKS))
                {
                      BLOCK_SELECTED = 0;
                      currentPointer = 0; // Reiniciamos el índice
                      currentIdx = currentPointer;
                      hmi.writeString("tape.currentBlock.val=" + String(currentPointer+1));
                      delay(125);
                      hmi.writeString("tape.currentBlock.val=" + String(currentPointer+1));
                      waitflag = 0;
                      while(!source.selectStream(currentIdx))
                      {
                          if(waitflag++>255){player.stop();}
                          delay(125);
                      }  
                      fileread = 0;                    
                      waitflag = 0;
                      while(!player.begin(currentIdx))
                      {
                          if(waitflag++>255){player.stop();}
                      }
                      // Reiniciar el reproductor
                      player.stop(); // Detener el reproductor
                      fileSize = getStreamfileSize(pFile);
                      STOP=true;
                      PLAY=false;
                }
            }
 
            FFWIND = RWIND = false;
            fileread = 0;
            //
            updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead, audiolist[currentPointer].filename);

            // Para evitar errores de lectura en el buffer
            delay(250);

            // Cargamos la cabecera del nuevo fichero
            player.setAutoNext(false);
            if (player.copy() != 0)
            {
              LAST_MESSAGE = "...";
            } 

            AudioFormat format;
            AUDIO_FORMART_IS_VALID = true;
            if (ext == "wav") {
                format = decoderWAV.audioInfoEx().format;
                logln("WAV format: " + String((int)format));

                if (format != AudioFormat::PCM)
                {
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

        }
        else if ((FFWIND || RWIND) && was_pressed_wd)
        {
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
          if (p_file_seek != nullptr) 
          {
            fileread = p_file_seek->position();
            fileSize = p_file_seek->size();
          }  
          // reseteamos el estado fast_wind
          fast_wind_status = 0;  
        }
   
        // Avance rapido y retroceso rapido
        if (KEEP_FFWIND || KEEP_RWIND) 
        {
          if (KEEP_FFWIND && ((currentPointer + 1) <= TOTAL_BLOCKS))
          {
            // Avance rapido (acelerado)
            if (fast_wind_status==0)
            {
              osr = kitStream.audioInfo().sample_rate;
              // Ajustamos al SR mas alto para avance rapido
              AudioInfo info = kitStream.audioInfo();
              info.sample_rate = 352800;
              kitStream.setAudioInfo(info);
              hmi.writeString("tape.stepTape.val=4");
              //
              p_file_seek = (File*)player.getStream();
              if (p_file_seek != nullptr) 
              {
                p_file_seek_pos = p_file_seek->position();
              }
              
              t_button_pressed = millis();
              logln("Avance rapido");

              //
              fast_wind_status = 1;
            }
            else
            {
              if (millis() - t_button_pressed > TIME_TO_FAST_FORWRD) 
              {
                if (fast_wind_status==1)
                {
                  fileSize = getStreamfileSize(pFile);
                  logln("Avance ultra rapido");

                  fast_wind_status = 2;              
                }
                else if (fast_wind_status==2)
                {
                    // Avance ultra-rapido
                    if (p_file_seek != nullptr)
                    {
                      if (p_file_seek_pos < (p_file_seek->size() - (p_file_seek->size() * FAST_FORWARD_PER))) 
                      {
                        p_file_seek_pos += (p_file_seek->size() * FAST_FORWARD_PER);
                        p_file_seek->seek(p_file_seek_pos);
                        if (p_file_seek != nullptr) 
                        {
                          fileread = p_file_seek->position();
                          fileSize = p_file_seek->size();
                        }                    
                        delay(DELAY_ON_EACH_STEP_FAST_FORWARD);
                      }
                    }
                }
              }
            }
          }
          else if (KEEP_RWIND && ((currentPointer + 1) <= TOTAL_BLOCKS))
          {
              // Retroceso rapido
              if (fast_wind_status==0)
              {
                  // Capturamos el sample rate original antes de cambiarlo
                  osr = kitStream.audioInfo().sample_rate;
                  //
                  p_file_seek = (File*)player.getStream();
                  if (p_file_seek != nullptr) 
                  {
                    p_file_seek_pos = p_file_seek->position();
                  }
                  logln("Retroceso rapido");
                  fileSize = getStreamfileSize(pFile);
                  // Entramos en modo retroceder
                  fast_wind_status = 1;              
              }
              else if (fast_wind_status==1)
              {
                  if (p_file_seek != nullptr)
                  {
                    size_t rewind_amount = p_file_seek->size() * FAST_REWIND_PER;

                    if (p_file_seek_pos > rewind_amount) 
                    {
                        p_file_seek_pos -= rewind_amount;
                    }
                    else
                    {
                        // Si no hay suficiente para retroceder, ir al inicio
                        p_file_seek_pos = 0;
                    }
                    
                    p_file_seek->seek(p_file_seek_pos);

                    if (p_file_seek != nullptr) 
                    {
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
        if (BB_OPEN || BB_UPDATE)
        {
          //
          hmi.openBlockMediaBrowser(audiolist);
        }
        // Salida del Block Browser
        else if (UPDATE_HMI)
        {
          logln("UPDATE HMI");

          // *******************************************************
          // Hemos seleccionado una pista en la lista de audio
          // *******************************************************
          if (BLOCK_SELECTED > 0 && BLOCK_SELECTED <= TOTAL_BLOCKS)
          {
              fileread = 0;
              // Cogemos el indice que empieza desde 0 ..
              currentPointer = BLOCK_SELECTED - 1;
              currentIdx = currentPointer;             
              logln("Selected file: " + (audiolist[currentPointer].filename) + " - Index: " + String(currentIdx));
              player.stop(); // Detener el reproductor
              waitflag = 0;
              while(!player.begin(currentIdx))
              {
                  if(waitflag++>255){player.stop();}
              }
              // Iniciamos el reproductor
              fileSize = getStreamfileSize(pFile);
              
              // Esto lo hacemos para coger el sampling rate
              sample_rate_t srd = player.audioInfo().sample_rate;
              hmi.writeString("tape.lblFreq.txt=\"" + String(int(srd/1000)) + "KHz\"" ); 
              // Actualizamos HMI
              updateIndicators(totalFilesIdx, currentPointer+1, fileSize, bitRateRead, audiolist[currentPointer].filename);  

          }

          UPDATE_HMI = false;
        }  
        // Seleccion de pista mediante el BLOCK_SELECTED
        else if (UPDATE)
        {
          logln("UPDATE");

          // Si el bloque seleccionado es válido y no es el último
          if (BLOCK_SELECTED > 0 && (BLOCK_SELECTED <= (totalFilesIdx+1))) {
              // Reproducir el bloque seleccionado
              currentPointer = BLOCK_SELECTED - 1;
              currentIdx = currentPointer;             
              player.stop(); // Detener el reproductor
              //player.end();
              waitflag = 0;
              while(!player.begin(currentIdx))
              {
                  if(waitflag++>255){player.stop();}
              }
              
          } else
          {
              // Reproducir desde el principio
              currentPointer = 0; // Reiniciamos el índice
              currentIdx = currentPointer;            
              player.stop(); // Detener el reproductor
              //player.end();
              waitflag = 0;
              while(!player.begin(currentIdx))
              {
                  if(waitflag++>255){player.stop();}
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

          //bitRateRead = isWav ? decoderWAV.audioInfoEx().byte_rate : 0;
          updateIndicators(totalFilesIdx, currentPointer + 1, fileSize, bitRateRead, audiolist[currentPointer].filename);
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
    logln("Final Audio Settings - Sample Rate: " + String(finalAudioInfo.sample_rate) + 
          "Hz, Bits: " + String(finalAudioInfo.bits_per_sample) + 
          ", Channels: " + String(finalAudioInfo.channels));
    updateSamplingRateIndicator(finalAudioInfo.sample_rate, finalAudioInfo.bits_per_sample, ext);   // Paramos animación
   tapeAnimationOFF();
   
   // Descargamos objetos
   player.end();
   eq.end();
   //
   decoderMP3.end();
   decoderWAV.end(); 
   decoderFLAC.end();
   measureMP3.end();
   metadatafilter.end();
      
   // Cerramos el puntero de avance rapido y lo eliminamos.
   if (p_file_seek != nullptr && p_file_seek)
   {
     p_file_seek->close();
   }

   // Liberamos la memoria del audiolist
   free(audiolist); 

  //  // Devolvemos la configuracion a la board.
  //  AudioInfo new_sr = kitStream.defaultConfig();
  //  new_sr.sample_rate = SAMPLING_RATE;
  //  new_sr.bits_per_sample = 16;
  //  new_sr.channels = 2;
  //  kitStream.setAudioInfo(new_sr);      
   
   // Indicamos
   //hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE/1000)) + "KHz\"" );  

   //
   //showOption("menuAudio.mutAmp.val",String(ACTIVE_AMP));
   //kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);      
}

// Función helper para cambiar configuración de audio de manera segura
bool setAudioInfoSafe(audio_tools::AudioInfo newInfo, const String& context = "") 
{
    static audio_tools::AudioInfo lastConfig;
    static bool firstTime = true;
    
    // Verificar si la configuración es diferente
    if (!firstTime && 
        lastConfig.sample_rate == newInfo.sample_rate &&
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
void playingFile()
{
  rotate_enable = true;
  kitStream.setVolume(MAX_MAIN_VOL);
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);

  AudioInfo new_sr;
  
  // Por defecto

  if (TYPE_FILE_LOAD == "TAP")
  {
    // Ajustamos la salida de audio al valor estandar de las maquinas de 8 bits
    new_sr = kitStream.defaultConfig();
    LAST_SAMPLING_RATE = SAMPLING_RATE + TONE_ADJUST;
    SAMPLING_RATE = STANDARD_SR_8_BIT_MACHINE + TONE_ADJUST;
    new_sr.sample_rate = STANDARD_SR_8_BIT_MACHINE + TONE_ADJUST;

    if (!setAudioInfoSafe(new_sr, "TAP playback")) {
        LAST_MESSAGE = "Error configuring audio for TAP";
        STOP = true;
        PLAY = false;
        return;
    }      

    //kitStream.setAudioInfo(new_sr);    

    
    //
    if (OUT_TO_WAV)
    {
        // Configuramos el encoder WAV directament
        AudioInfo wavencodercfg(SAMPLING_RATE, 2, 16);
        // Iniciamos el stream
        encoderOutWAV.begin(wavencodercfg);
        
        // Verificamos la configuración final
        logln("Sampling rate changed for TAP file: " + String(SAMPLING_RATE)  + "Hz");
        logln("WAV encoder - Out to WAV: " + String(encoderOutWAV.audioInfo().sample_rate) + 
              "Hz, Bits: " + String(encoderOutWAV.audioInfo().bits_per_sample) + 
              ", Channels: " + String(encoderOutWAV.audioInfo().channels));  
    }

    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(STANDARD_SR_8_BIT_MACHINE/1000)) + "KHz\"" );
    //
    sendStatus(REC_ST, 0);
    pTAP.play();
    //Paramos la animación
    tapeAnimationOFF();

    if (OUT_TO_WAV)
    {
      encoderOutWAV.end();
    }
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
    // Ajustamos la salida de audio al valor estandar de las maquinas de 8 bits
    new_sr = kitStream.defaultConfig();
    LAST_SAMPLING_RATE = SAMPLING_RATE + TONE_ADJUST;
    SAMPLING_RATE = STANDARD_SR_8_BIT_MACHINE + TONE_ADJUST;
    new_sr.sample_rate = STANDARD_SR_8_BIT_MACHINE + TONE_ADJUST;

    logln("New sampling rate = " + String(SAMPLING_RATE));
    
    if (!setAudioInfoSafe(new_sr, "TZX playback")) {
        LAST_MESSAGE = "Error configuring audio for TZX";
        STOP = true;
        PLAY = false;
        return;
    }    

    //kitStream.setAudioInfo(new_sr);  
    
    //
    if (OUT_TO_WAV)
    {
        // Configuramos el encoder WAV directament
        AudioInfo wavencodercfg(SAMPLING_RATE, 2, 16);
        // Iniciamos el stream
        encoderOutWAV.begin(wavencodercfg);
        
        // Verificamos la configuración final
        logln("Sampling rate changed for TZX format file: " + String(SAMPLING_RATE)  + "Hz");
        logln("WAV encoder - Out to WAV: " + String(encoderOutWAV.audioInfo().sample_rate) + 
              "Hz, Bits: " + String(encoderOutWAV.audioInfo().bits_per_sample) + 
              ", Channels: " + String(encoderOutWAV.audioInfo().channels));  
    }

    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(STANDARD_SR_8_BIT_MACHINE/1000)) + "KHz\"" );
    //
    sendStatus(REC_ST, 0);
    pTZX.play();
    //Paramos la animación
    tapeAnimationOFF();
    //
    if (OUT_TO_WAV)
    {
      encoderOutWAV.end();
    }
  }
  else if (TYPE_FILE_LOAD == "WAV")
  { 
    // Indicamos el sampling rate
    hmi.writeString("tape.lblFreq.txt=\"" + String(int(DEFAULT_WAV_SAMPLING_RATE/1000)) + "KHz\"" );

    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    LAST_MESSAGE = "Wait for scanning end.";

    MediaPlayer();
    logln("Finish WAV playing file");
  }
  else if (TYPE_FILE_LOAD == "MP3")
  {
    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el MP3 file
    LAST_MESSAGE = "Wait for scanning end.";
    MediaPlayer();
    logln("Finish MP3 playing file");
  }
  else if (TYPE_FILE_LOAD == "FLAC")
  {
    logln("Type file load: " + TYPE_FILE_LOAD);
    // Reproducimos el FLAC file
    LAST_MESSAGE = "Wait for scanning end.";
    MediaPlayer();
    //FLACplayer();
    logln("Finish FLAC playing file");
  }
  else
  {
    logAlert("Unknown type_file_load");
  }

  // Modificado 17/10/2025

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
  File cfg;

  logln("");
  log("path: " + String(strpath));

  if (cfg)
  {
    cfg.close();
  }

  if (SD_MMC.exists(strpath))
  {
    logln("");
    log("cfg path: " + String(strpath));

    cfg = SD_MMC.open(strpath, FILE_READ);
    if (cfg)
    {
      // Leemos todos los parametros
      //cfg.rewind();
      fileCfg = readAllParamCfg(cfg, max_params);

      logln("");
      log("cfg open");

      for (int i = 0; i < max_params; i++)
      {
        logln("");
        log("Param: " + fileCfg[i].cfgLine);

        if ((fileCfg[i].cfgLine).indexOf("freq") != -1)
        {
          SAMPLING_RATE = (getValueOfParam(fileCfg[i].cfgLine, "freq")).toInt();
          
          // CAmbiamos el sampling rate del hardware de salida
          //auto cfg = kitStream.audioInfo();
          auto cfg = kitStream.defaultConfig();
          cfg.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg);

          //auto cfg2 = kitStream.audioInfo();
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
          ZEROLEVEL = getValueOfParam(fileCfg[i].cfgLine, "zerolevel").toInt();

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
          APPLY_END = getValueOfParam(fileCfg[i].cfgLine, "blockend").toInt();

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
          if ((getValueOfParam(fileCfg[i].cfgLine, "polarized")) == "1")
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
    if (PATH_FILE_TO_LOAD.indexOf(".TAP", PATH_FILE_TO_LOAD.length() - 4) != -1)
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
    else if ((PATH_FILE_TO_LOAD.indexOf(".TZX", PATH_FILE_TO_LOAD.length() - 4) != -1) || 
             (PATH_FILE_TO_LOAD.indexOf(".TSX", PATH_FILE_TO_LOAD.length() - 4) != -1) || 
             (PATH_FILE_TO_LOAD.indexOf(".CDT", PATH_FILE_TO_LOAD.length() - 4) != -1))
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

      if (PATH_FILE_TO_LOAD.indexOf(".TZX", PATH_FILE_TO_LOAD.length() - 4) != -1)
      {
        TYPE_FILE_LOAD = "TZX";
      }
      else if (PATH_FILE_TO_LOAD.indexOf(".TSX", PATH_FILE_TO_LOAD.length() - 4) != -1)
      {
        TYPE_FILE_LOAD = "TSX";
      }
      else
      {
        TYPE_FILE_LOAD = "CDT";
      }
      BYTES_TOBE_LOAD = myTZX.size;
    }
    else if (PATH_FILE_TO_LOAD.indexOf(".WAV", PATH_FILE_TO_LOAD.length() - 4) != -1)
    {
      logln("Wav file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "WAV";
    }
    else if (PATH_FILE_TO_LOAD.indexOf(".MP3", PATH_FILE_TO_LOAD.length() - 4) != -1)
    {
      logln("MP3 file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "MP3";
    }
    else if (PATH_FILE_TO_LOAD.indexOf(".FLAC", PATH_FILE_TO_LOAD.length() - 5) != -1)
    {
      logln("FLAC file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "FLAC";
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

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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

  if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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
  else if (TYPE_FILE_LOAD == "FLAC")
  {
    // FLAC
    hmi.writeString("tape.logo.pic=49");
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
    int i = 0;

    while (!myTZX.descriptor[i].playeable)
    {
      BLOCK_SELECTED = i;
      i++;
    }

    if (i > MAX_BLOCKS_IN_TZX)
    {
      i = 0;
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


  if(TYPE_FILE_LOAD != "TAP")
  { 
    // Para las estructuras TZX, CDT y TSX
    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 1))
    {
        BLOCK_SELECTED = 1;
        isGroupStart();    
    }
  }    
  else
  {
    // Para el fichero TAP
    if (BLOCK_SELECTED > (TOTAL_BLOCKS - 2))
    {
        BLOCK_SELECTED = 0;
    }
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

  // Para las estructuras TZX, CDT y TSX
  if(TYPE_FILE_LOAD != "TAP")
  {   
    if (BLOCK_SELECTED < 1)
    {
      BLOCK_SELECTED = (TOTAL_BLOCKS - 1);
      isGroupEnd();    
    }
  }
  else
  {
    // Para el fichero TAP
    if (BLOCK_SELECTED < 0)
    {
      BLOCK_SELECTED = (TOTAL_BLOCKS - 2);
    }
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

// Modificado 17/10/2025
void recoverEdgeBeginOfBlock()
{
  if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
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
    encoderOutWAV.end();
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
      // DISABLE_SD = false;
      // // Deshabilitamos temporalmente la SD para poder subir un firmware
      // SD_MMC.end();
      // int tout = 59;
      // while (tout != 0)
      // {
      //   delay(1000);
      //   hmi.writeString("debug.blockLoading.txt=\"" + String(tout) + "\"");
      //   tout--;
      // }

      // hmi.writeString("debug.blockLoading.txt=\"..\"");
      
      // if (!SD_MMC.begin(SD_CHIP_SELECT, SD_SCK_MHZ(SD_SPEED_MHZ)))
      // {
      //   logln("Error recoverind SD access");
      // };
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
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
      {
        LAST_MESSAGE = "Loading in progress. Please wait.";
      }
      else
      {
        LAST_MESSAGE = "Playing audio file.";
      }
      //
      playingFile();
      logln("End of CASE 1 - PLAY from REC");
    }
    else if (EJECT && !STOP_OR_PAUSE_REQUEST)
    {
      TAPESTATE = 0;
      LOADING_STATE = 0;

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
      }
    }
    else if (PAUSE)
    {
      // Nos vamos al estado PAUSA para seleccion de bloques
      TAPESTATE = 3;
      // Esto la hacemos para evitar una salida inesperada de seleccion de bloques.
      PAUSE = false;
      //
      // if (TYPE_FILE_LOAD == "WAV")
      // {
      //   LAST_MESSAGE = "Playing paused.";
      // }
      // else
      // {
      if (AUTO_PAUSE)
      {
        logln("Auto-pause activated");
        LAST_MESSAGE = "Tape auto-paused. Follow machine instructions.";
        ABORT = false;
        STOP = false;
      }
      else
      {
        if (LAST_BLOCK_WAS_GROUP_START)
        {
          // Localizamos el GROUP START
          prevGroupBlock();
        }
      }
    
        // }

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
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

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
      {
        getTheFirstPlayeableBlock();
      }

      if (OUT_TO_WAV)
      {
        wavfile.flush();
        wavfile.close();
        encoderOutWAV.end();
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
        encoderOutWAV.end();
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

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
      {
        getTheFirstPlayeableBlock();
      }
      //
      setPolarization();
    }
    else if (FFWIND || RWIND)
    {

      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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

        // Ponemos esto aqui porque prepareOutputToWav() puede cambiar el
        // flujo de TAPESTATE
        TAPESTATE = 1;
        LOADING_STATE = 1;

        if (OUT_TO_WAV)
        {
          prepareOutputToWav();
        }


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
      if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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

            // Para poner mas logos actualizar el HMI
            putLogo();

            if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
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
              //PROGRAM_NAME = FILE_LOAD;
              hmi.writeString("name.txt=\"" + FILE_LOAD + "\"");

              HMI_FNAME = FILE_LOAD;

              TAPESTATE = 10;
              LOADING_STATE = 0;

              // Esto lo hacemos para llevar el control desde el WAV player
              logln("Playing file CASE 100:");
              playingFile();
              logln("End of CASE 100 - PLAY from REC");

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
      LAST_MESSAGE = "Recording canceled.";
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
        PATH_FILE_TO_LOAD = FILE_LAST_DIR + "/" + FILE_LOAD;

        hmi.reloadDir();
        //
        LAST_MESSAGE = "File preparing to play.";
        // Eliminamos la ruta del fichero y nos quedamos con el nombre y la extensión
        FILE_LOAD = getFileNameFromPath(FILE_LOAD);
        // Esto lo hacemos para llevar el control desde el WAV player
        playingFile();    
        logln("End of CASE 130 - PLAY from REC");      
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
      LAST_MESSAGE = "Recording canceled.";
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
      while (!taprec.recording())
      {
        delay(1);
      }

      // RECORDING_IN_PROGRESS = false;
      // logln("REC mode - Recording in progress " + String(RECORDING_IN_PROGRESS));
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
        // Escaneamos por los nuevos ficheros
        //LAST_MESSAGE = "Rescaning files";
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

  // Actualizamos el HMI - indicadores
  if (UPDATE)
  {
    if (TYPE_FILE_LOAD != "TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "FLAC")
    {
      // Forzamos un refresco de los indicadores para TZX, CDT y TSX
      hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,
                                  myTZX.descriptor[BLOCK_SELECTED].group,
                                  myTZX.descriptor[BLOCK_SELECTED].name,
                                  myTZX.descriptor[BLOCK_SELECTED].typeName,
                                  myTZX.descriptor[BLOCK_SELECTED].size,
                                  myTZX.descriptor[BLOCK_SELECTED].playeable);
    }
    else
    {
      // Forzamos un refresco de los indicadores para TAP
      hmi.setBasicFileInformation(0, 0, myTAP.descriptor[BLOCK_SELECTED].name,
                                  myTAP.descriptor[BLOCK_SELECTED].typeName,
                                  myTAP.descriptor[BLOCK_SELECTED].size,
                                  true);
    }
  
    hmi.updateInformationMainPage(true);  
    UPDATE = false;    
  }  

  if (START_FFWD_ANIMATION)
  {
    rewindAnimation(1);  
    START_FFWD_ANIMATION = false;  
  }
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
    
    // Control del FTP
    #ifdef FTP_SERVER_ENABLE
      ftpSrv.handleFTP();
    #endif


    // Control por botones
    //buttonsControl();

    // Esto lo ponemos para evitar errores en la lectura del puerto serie
    delay(25);

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
        else
        {
          // Lo dejamos fijo a low power
          if (!REC)
          {
            actuatePowerLed(true,POWERLED_DUTY);
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
            if ((TYPE_FILE_LOAD == "WAV" || TYPE_FILE_LOAD == "MP3" || TYPE_FILE_LOAD == "FLAC"))
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

bool createSpecialDirectory(String fDir)
{
  //Esto lo hacemos para ver si el directorio existe
  if (!SD_MMC.open(fDir))
  {
    if (!SD_MMC.mkdir(fDir))
    {
        #ifdef DEBUGMODE
          logln("");
          log("Error! Directory exists or wasn't created");
        #endif
        return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    #ifdef DEBUGMODE
      logln("");
      log("Directory exists.");
    #endif
    return false;
  }  
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
  //powadcr_pins.addPin(PinFunction::KEY, 36, PinLogic::InputActiveLow, 1);
  //powadcr_pins.addPin(PinFunction::KEY, 13, PinLogic::InputActiveLow, 2);
  //powadcr_pins.addPin(PinFunction::KEY, 19, PinLogic::InputActiveLow, 3);
  //powadcr_pins.addPin(PinFunction::KEY, 5, PinLogic::InputActiveLow, 6);
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
  // Esto es importante para el uso de SD_MMC
  cfg.sd_active = false;
  
  //
  kitStream.begin(cfg);

  // Ajustamos el volumen de entrada
  kitStream.setInputVolume(IN_REC_VOL);

  // Para que esta linea haga efecto debe estar el define 
  //
  #if USE_AUDIO_LOGGING
    //AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Debug);
  #endif  

  // Arrancamos el indicador de power
  actuatePowerLed(true,POWERLED_DUTY);
  //
  // ----------------------------------------------------------------------

  #ifdef MSX_REMOTE_PAUSE
    hmi.writeString("statusLCD.txt=\"Setting Remote Pause\"");
    delay(1250);
    pinMode(GPIO_MSX_REMOTE_PAUSE, INPUT_PULLUP);
  #endif

  // Configuramos acceso a la SD

  logln("Waiting for SD Card");
  // GpioPin pincs = kitStream.getPinID(PinFunction::SD,0);

  // logln("Pin CS: " + String(pincs));
  
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
  if (!SD_MMC.begin("/sdcard",false,false,SD_Speed))
  {

    // SD no inicializada
    while(1)
    {
      hmi.writeString("statusLCD.txt=\"SD_MMC not mounted\"");
      delay(2000);
      hmi.writeString("statusLCD.txt=\"1. Verify audiokit switches.\"");
      delay(2000);
      hmi.writeString("statusLCD.txt=\"2. Verify SD card type.\"");
      delay(2000);
    }
  }
  else
  {
    logln("SD Card mounted");
    hmi.writeString("statusLCD.txt=\"SD_MMC available\"");
    delay(125);
  }


  //SD_SPEED_MHZ = setSDFrequency(SD_Speed);

  //logln("SD Card - Speed " + String(SD_SPEED_MHZ));

  // Para forzar a un valor, descomentar esta linea
  // y comentar las dos de arriba
  // Le pasamos al HMI el gestor de SD

  //hmi.set_sdf(sdf);

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
  // hmi.writeString("statusLCD.txt=\"Initializing SPIFFS\"");
  // initSPIFFS();
  // if (esp_spiffs_mounted)
  // {
  //   hmi.writeString("statusLCD.txt=\"SPIFFS mounted\"");
  // }
  // else
  // {
  //   hmi.writeString("statusLCD.txt=\"SPIFFS error\"");
  // }
  // delay(500);

  // -------------------------------------------------------------------------
  // Actualización OTA por SD
  // -------------------------------------------------------------------------

  log_v("");
  log_v("Search for firmware..");
  char strpath[20] = {};
  strcpy(strpath, "/firmware.bin");
  File firmware = SD_MMC.open(strpath, FILE_READ);
  logln("Firmware file opened " + String(strpath));
  delay(1500);

  if (firmware)
  {
    hmi.writeString("statusLCD.txt=\"New powadcr firmware found\"");
    logln("Firmware found on SD_MMC");
    onOTAStart();
    logln("Updating firmware...");
    log_v("found!");
    log_v("Try to update!");

    Update.onProgress(onOTAProgress);

    size_t firmwareSize = firmware.available();
    logln("Firmware size: " + String(firmwareSize) + " bytes");

    Update.begin(firmwareSize, U_FLASH);
    Update.writeStream(firmware);
    if (Update.end())
    {
      log_v("Update finished!");
      hmi.writeString("statusLCD.txt=\"Update finished\"");
      onOTAEnd(true);
      delay(2000);
    }
    else
    {
      log_e("Update error!");
      hmi.writeString("statusLCD.txt=\"Update error\"");
      onOTAEnd(false);
      delay(2000);
    }

    firmware.close();

    if (SD_MMC.remove(strpath))
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
  if (SD_MMC.exists("/powadcr_iface.tft"))
  {
    // NOTA: Este metodo necesita que la pantalla esté alimentada con 5V
    //writeStatusLCD("Uploading display firmware");

    // Alternativa 1
    char strpath[20] = {};
    strcpy(strpath, "/powadcr_iface.tft");

    uploadFirmDisplay(strpath);
    SD_MMC.remove(strpath);
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
      // configureWebServer();
      // server.begin();
      // logln("Webserver configured");

      // FTP Server
      #ifdef FTP_SERVER_ENABLE
        ftpSrv.begin(&SD_MMC,"powa","powa");
      #endif

    }
  }

  delay(750);


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

    while(!a2dp.is_connected())
    {
      if (a2dp.get_audio_state() == 0)
      {
          logln("MEDIA CONTROL ACK SUCCESS");
      }
    }

    hmi.writeString("statusLCD.txt=\"Bluetooth connected\"");
    delay(1250);
  #endif

  // ----------------------------------------------------------
  // Estructura de la SD
  //
  // ----------------------------------------------------------

  // Creamos el directorio /fav
  String fDir = "/FAV";

  //Esto lo hacemos para ver si el directorio existe
  if(createSpecialDirectory(fDir))
  {
    hmi.writeString("statusLCD.txt=\"Creating FAV directory\"");
    hmi.reloadCustomDir("/");
    delay(750);    
  }


  // Creamos el directorio /rec
  fDir = "/REC";

  //Esto lo hacemos para ver si el directorio existe
  if(createSpecialDirectory(fDir))
  {
    hmi.writeString("statusLCD.txt=\"Creating REC directory\"");
    hmi.reloadCustomDir("/");
    delay(750);    
  }


  // Creamos el directorio /wav
  fDir = "/WAV";

  //Esto lo hacemos para ver si el directorio existe
  if(createSpecialDirectory(fDir))
  {
    hmi.writeString("statusLCD.txt=\"Creating WAV directory\"");
    hmi.reloadCustomDir("/");
    delay(750);    
  }


  // Creamos el directorio /mp3
  fDir = "/MP3";

  if(createSpecialDirectory(fDir))
  {
    hmi.writeString("statusLCD.txt=\"Creating MP3 directory\"");
    hmi.reloadCustomDir("/");
    delay(750);    
  }

  // Creamos el directorio /flac
  fDir = "/FLAC";

  if(createSpecialDirectory(fDir))
  {
    hmi.writeString("statusLCD.txt=\"Creating FLAC directory\"");
    hmi.reloadCustomDir("/");
    delay(750);    
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

  hmi.writeString("statusLCD.txt=\"STARTING HMI\"");
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
  SAMPLING_RATE = 22050;  // Por defecto 22050

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
  // pTAP.set_SDM(sdm);
  // pTZX.set_SDM(sdm);

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

  // --------------------------------------------------------------------------
  //
  // Cargamos la configuracion del HMI desde la particion NVS
  //
  // --------------------------------------------------------------------------
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
  // First view files on sorting
  showOption("menu.sortFil.val",String(!SORT_FILES_FIRST_DIR));


  if (ACTIVE_AMP)
  {
    MAIN_VOL_L = 5;
    // Actualizamos el HMI
    hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_L)); 
  }

  if (EN_SPEAKER)
  {
    // Actualizamos el HMI
    showOption("menuAudio.enSpeak.val",String(EN_SPEAKER));
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
  
  // Esto lo hacemos porque depende de la configuración cargada.
  kitStream.setPAPower(ACTIVE_AMP && EN_SPEAKER);   
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
  //taprec.set_SdFat32(sdf);

  taskStop = false;

  #ifdef DEBUGMODE
    hmi.writeString("tape.name.txt=\"DEBUG MODE ACTIVE\"");
  #endif



  // fin del setup()
}

void loop()
{
}