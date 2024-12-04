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

// Setup for audiokitHAL

// typedef enum {
//     MIC_GAIN_MIN = -1,
//     MIC_GAIN_0DB = 0,
//     MIC_GAIN_3DB = 3,
//     MIC_GAIN_6DB = 6,
//     MIC_GAIN_9DB = 9,
//     MIC_GAIN_12DB = 12,
//     MIC_GAIN_15DB = 15,
//     MIC_GAIN_18DB = 18,
//     MIC_GAIN_21DB = 21,
//     MIC_GAIN_24DB = 24,
//     MIC_GAIN_MAX,
// } es_mic_gain_t;

// Seleccionamos la placa. Puede ser 5 o 7.
#define AUDIOKIT_BOARD  5

// Ganancia de entrada por defecto. Al máximo
//#define ES8388_DEFAULT_INPUT_GAIN MIC_GAIN_MAX

// Definimos la ganancia de la entrada de linea (para RECORDING)
#define WORKAROUND_ES8388_LINE1_GAIN MIC_GAIN_MAX
#define WORKAROUND_ES8388_LINE2_GAIN MIC_GAIN_MAX

// Mezcla LINE 1 and LINE 2.
#define WORKAROUND_MIC_LINEIN_MIXED false

// Esencial para poder variar la ganancia de salida de HPLINE y ROUT, LOUT
#define AI_THINKER_ES8388_VOLUME_HACK 1
// #define USE_A2DP

#include <Arduino.h>
#include <WiFi.h>

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


#include <esp_task_wdt.h>
#define WDT_TIMEOUT 360000  

#include <HardwareSerial.h>
HardwareSerial SerialHW(2);

EasyNex myNex(SerialHW);

#include "SdFat.h"
#include "globales.h"

// #include "AudioKitHAL.h"
#include "AudioTools/AudioLibs/AudioKit.h"
AudioKit ESP32kit;

// Estos includes deben ir en este orden por dependencias
#include "SDmanager.h"

// Creamos el gestor de ficheros para usarlo en todo el firmware
SDmanager sdm;

#include "HMI.h"
HMI hmi;

#include "interface.h"
#include "ZXProcessor.h"

// ZX Spectrum. Procesador de audio output
ZXProcessor zxp;

// Procesadores de cinta
#include "BlockProcessor.h"

#include "TZXprocessor.h"
#include "TAPprocessor.h"

//#include "test.h"

// Declaraciones para SdFat
SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;
File32 wavfile;

// Creamos los distintos objetos
TZXprocessor pTZX(ESP32kit);
TAPprocessor pTAP(ESP32kit);

// Procesador de audio input
#include "TAPrecorder.h"
TAPrecorder taprec;

// Procesador de TAP
tTAP myTAP;
// Procesador de TZX/TSX/CDT
tTZX myTZX;


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

// WAV Recorder
// -----------------------------------------------------------------------
#include "AudioTools.h"
#include "AudioTools/AudioLibs/AudioKit.h"
#include "AudioTools/AudioLibs/AudioSourceSDFAT.h"

using namespace audio_tools;  
// 
AudioKitStream kit;
StreamCopy copier(kit, kit);  // copies data

// -----------------------------------------------------------------------
// Prototype function
void ejectingFile();

void freeMemoryFromDescriptorTZX(tTZXBlockDescriptor* descriptor)
{
  // Vamos a liberar el descriptor completo
  for (int n=0;n<TOTAL_BLOCKS;n++)
  {
    switch (descriptor[n].ID)
    {
      case 19:  // bloque 0x13
        // Hay que liberar el array
        // delete[] descriptor[n].timming.pulse_seq_array;
        free(descriptor[n].timming.pulse_seq_array);
        break;
      
      default:
        break;
    }
  }
}

void freeMemoryFromDescriptorTSX(tTZXBlockDescriptor* descriptor)
{
  // Vamos a liberar el descriptor completo
  for (int n=0;n<TOTAL_BLOCKS;n++)
  {
    switch (descriptor[n].ID)
    {
      case 19:  // bloque 0x13
        // Hay que liberar el array
        // delete[] descriptor[n].timming.pulse_seq_array;
        free(descriptor[n].timming.pulse_seq_array);
        break;

      default:
        break;
    }
  }
}

int* strToIPAddress(String strIPAddr)
{
    int* ipnum = new int[4];
    int wc = 0;

    // Parto la cadena en los distintos numeros que componen la IP
    while (strIPAddr.length() > 0)
    {
      int index = strIPAddr.indexOf('.');
      // Si no se encuentran mas puntos es que es
      // el ultimo digito.
      if(index == -1)
      {
        ipnum[wc] = strIPAddr.toInt();
        return ipnum;
      }
      else
      {
        // Tomo el dato y lo paso a INT
        ipnum[wc] = (strIPAddr.substring(0, index)).toInt();
        // Elimino la parte ya leida
        strIPAddr = strIPAddr.substring(index+1);
        wc++;
      }
    }

    // Si no hay nada para devolver, envio un 0.
    return 0;
}  
bool loadWifiCfgFile()
{
   
  bool cfgloaded = false;

  if(sdf.exists("/wifi.cfg"))
  {
    #ifdef DEBUGMODE
      logln("File wifi.cfg exists");
    #endif

    char pathCfgFile[10] = {};
    strcpy(pathCfgFile,"/wifi.cfg");

    File32 fWifi = sdm.openFile32(pathCfgFile);
    int* IP;

    if(fWifi.isOpen())
    {
        // HOSTNAME = new char[32];
        // ssid = new char[64];
        // password = new char[64];

        char* ip1 = new char[17];

        CFGWIFI = sdm.readAllParamCfg(fWifi,9);

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
        strcpy(HOSTNAME, (sdm.getValueOfParam(CFGWIFI[0].cfgLine,"hostname")).c_str());
        logln(HOSTNAME);
        // SSID - Wifi
        ssid = (sdm.getValueOfParam(CFGWIFI[1].cfgLine,"ssid")).c_str();
        logln(ssid);
        // Password - WiFi
        strcpy(password, (sdm.getValueOfParam(CFGWIFI[2].cfgLine,"password")).c_str());
        logln(password);

        //Local IP
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[3].cfgLine,"IP")).c_str());
        logln("IP: " + String(ip1));
        POWAIP = ip1;
        IP = strToIPAddress(String(ip1));
        local_IP = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // Subnet
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[4].cfgLine,"SN")).c_str());
        logln("SN: " + String(ip1));
        IP = strToIPAddress(String(ip1));
        subnet = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // gateway
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[5].cfgLine,"GW")).c_str());
        logln("GW: " + String(ip1));
        IP = strToIPAddress(String(ip1));
        gateway = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // DNS1
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[6].cfgLine,"DNS1")).c_str());
        logln("DNS1: " + String(ip1));
        IP = strToIPAddress(String(ip1));
        primaryDNS = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // DNS2
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[7].cfgLine,"DNS2")).c_str());
        logln("DNS2: " + String(ip1));
        IP = strToIPAddress(String(ip1));
        secondaryDNS = IPAddress(IP[0],IP[1],IP[2],IP[3]);

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

void loadHMICfgFile()
{
  char pathCfgFile[10] = {};
  strcpy(pathCfgFile,"/hmi.cfg");

  File32 fHMI = sdm.openFile32(pathCfgFile);
  
  //Leemos ahora toda la configuración
  if(fHMI)
  {
      CFGHMI = sdm.readAllParamCfg(fHMI,20);
      log("Open config. HMI-success");
  }
  fHMI.close();
}

void proccesingTAP(char* file_ch)
{    
    //pTAP.set_SdFat32(sdf);
    pTAP.initialize();

    if (!PLAY)
    {
        pTAP.getInfoFileTAP(file_ch);

        if (!FILE_CORRUPTED && TOTAL_BLOCKS !=0)
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

void proccesingTZX(char* file_ch)
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
      if (TOTAL_BLOCKS !=0)
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

void sendStatus(int action, int value=0) {

  switch (action) {
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
      hmi.writeString("menu.verFirmware.txt=\" PowaDCR " + String(VERSION) + "\"");
      hmi.writeString("page tape");
      break;
    
    case RESET:
      //hmi.writeString("");
      hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
      break;
    
    case SAMPLINGTEST:
    
      zxp.samplingtest();
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
      if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed)) || lastStatus) 
      {

          // La frecuencia anterior no fue compatible
          // Probamos otras frecuencias
          hmi.writeString("statusLCD.txt=\"SD FREQ. AT " + String(SD_Speed) + " MHz\"" );
          SD_Speed = SD_Speed - 1;

          // Hemos llegado a un limite
          if (SD_Speed < 1) 
          {
              // Ninguna frecuencia funciona.
              // loop infinito - NO COMPATIBLE
              while(true)
              {
                  hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"" );
                  delay(1500);
                  hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"" );
                  delay(1500);
              }
          }

      }
      else 
      {
          // La SD ha aceptado la frecuencia de acceso
          hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"" );
          delay(750);

          // Probamos a listar un directorio
          if (!sdm.dir.open("/"))
          {
              SD_ok = false;
              lastStatus = false;

              // loop infinito
              while(true)
              {
                hmi.writeString("statusLCD.txt=\"SD READ TEST FAILED\"" );
                delay(1500);
                hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"" );
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
                      while(true)
                      {
                          hmi.writeString("statusLCD.txt=\"SD WRITE TEST FAILED\"" );
                          delay(1500);
                          hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"" );
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
                  while(true)
                  {
                      hmi.writeString("statusLCD.txt=\"REMOVE _test FILE FROM SD\"" );
                      delay(1500);
                  }
              }
          }
      
        // Cerramos
        if (sdFile32.isOpen())
        {sdFile32.close();}

        if (sdm.dir.isOpen())
        {sdm.dir.close();}
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

void setAudioOutput()
{
  auto cfg = ESP32kit.defaultConfig(KitOutput);

  if(!ESP32kit.begin(cfg))
  {
    //log("Error in Audiokit output setting");
  }

  switch (SAMPLING_RATE)
  {
      case 48000:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_48K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.lblFreq.txt=\"48KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 48 KHz\"" );
          delay(1500);
        }    
        break;

      case 44100:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.lblFreq.txt=\"44KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 44.1 KHz\"" );          
          delay(1500);
        }            
        break;

      case 32000:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_32K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.lblFreq.txt=\"32KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 32 KHz\"" );          
          delay(1500);
        }            
        break;

      default:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_22K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.lblFreq.txt=\"22KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 22.05 KHz\"" );          
          delay(1500);
        }            
        break;
  }


  if (!ESP32kit.setVolume(100))
  {
    //log("Error in volumen setting");
  }  
}

void setAudioInput()
{
  auto cfg = ESP32kit.defaultConfig(KitInput);

  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // Line with high gain
  cfg.sample_rate = AUDIO_HAL_44K_SAMPLES;

  if(!ESP32kit.begin(cfg))
  {
    //log("Error in Audiokit input setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
  {
    //log("Error in Audiokit sampling rate setting");
  }
}

void setAudioInOut()
{
  auto cfg = ESP32kit.defaultConfig(KitInputOutput);
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // Line with high gain
  
  if(!ESP32kit.begin(cfg))
  {
    //log("Error in Audiokit output setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
  {
    //log("Error in Audiokit sampling rate setting");
  }

  if (!ESP32kit.setVolume(100))
  {
    //log("Error in volumen setting");
  }   
}

void tapeAnimationON()
{
    hmi.writeString("tape2.tmAnimation.en=1"); 
    hmi.writeString("tape.tmAnimation.en=1");   
}

void tapeAnimationOFF()
{
    hmi.writeString("tape2.tmAnimation.en=0"); 
    hmi.writeString("tape.tmAnimation.en=0");   
}

void recAnimationON()
{
  hmi.writeString("tape.RECst.val=1");
}

void recAnimationOFF()
{
  hmi.writeString("tape.RECst.val=0");
}

void recAnimationFIXED_ON()
{
  hmi.writeString("tape.recIndicator.bco=63848");
}

void recAnimationFIXED_OFF()
{
  hmi.writeString("tape.recIndicator.bco=32768");
}

void setWavRecording(char* file_name,bool start=true)
{
    AudioLogger::instance().begin(Serial, AudioLogger::Error);

    if (start)
    {
        if (sdf.exists(file_name))
        {
            sdf.remove(file_name);
        }  

        // open file for recording WAV
        wavfile = sdf.open(file_name, O_WRITE | O_CREAT | O_TRUNC);

        if (!wavfile)
        {
            LAST_MESSAGE = "WAVFILE error!";
            delay(1500);
            logln("file failed!");
            STOP=true;
            REC=false;
            TAPESTATE=0;
        }            
        else
        {
            // Configure WAVEncoder
            AudioInfo info(16000, 1, 16);
            EncodedAudioStream out(&wavfile, new WAVEncoder());
            info.bits_per_sample = 16;
            info.sample_rate = 44100;
            info.channels = 2;

            WAVEncoder().begin(info);

            // setup input  
            auto cfg = kit.defaultConfig(RX_MODE);
            cfg.is_master = true;
            cfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2;
            cfg.bits_per_sample = 16;
            cfg.sample_rate = 44100;
            cfg.channels = 2;
            cfg.sd_active = true;
            cfg.copyFrom(info);

            // Inicializamos la entrada al ADC
            kit.begin(cfg);

            logln("Setting i2C");
            logln("");

            // Inicializamos la salida a un WAV encoder
            AudioInfo out_info(44100,2,16);
            out.begin(out_info);
            // Inicializamos el copier
            copier.setCheckAvailableForWrite(false);
            copier.begin(wavfile, kit);  
            AudioLogger::instance().begin(Serial, AudioLogger::Warning);

            LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";
            
            recAnimationOFF();
            delay(500);
            recAnimationFIXED_ON();
            tapeAnimationON();

            while(!STOP)
            {
              copier.copy();
            }

            wavfile.flush();
            logln("File has ");
            logln(String(wavfile.size()));
            logln(" bytes");

            wavfile.close();
            
            LAST_MESSAGE = "Recording to WAV - Finished.";
            logln("Recording finish!");

            delay(2000);    

            TAPESTATE = 0;
            LOADING_STATE = 0;
            RECORDING_ERROR = 0;
            REC = false;
            recAnimationOFF();
            recAnimationFIXED_OFF(); 
            tapeAnimationOFF();            
        }
    }
    else
    {

    }
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
          taprec.terminate(true);
          LAST_MESSAGE = "Recording STOP without blocks captured.";
          tapeAnimationOFF();
          //      
          delay(1500);
      }

    //}

    // Reiniciamos variables
    taprec.totalBlockTransfered = 0;
    taprec.WasfirstStepInTheRecordingProccess = false;
    // Reiniciamos flags de recording errors
    taprec.errorInDataRecording = false;
    taprec.fileWasNotCreated = true;

    //Paramos la animación del indicador de recording
    tapeAnimationOFF();
    
    // Reiniciamos el estado de los botones
    // hmi.writeString("tape.STOPst.val=1");
    // hmi.writeString("tape.RECst.val=0");        
    //
    // delay(2000);

    hmi.activateWifi(true);
    //
    // Cambiamos la frecuencia de la SD
    // sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_SPEED_MHZ));    

}

// -------------------------------------------------------------------------------------------------------------------
unsigned long ota_progress_millis = 0;

void onOTAStart() 
{
  pageScreenIsShown = false;
}

void onOTAProgress(size_t currSize, size_t totalSize)
{
  log_v("CALLBACK:  Update process at %d of %d bytes...", currSize, totalSize);

  // Log every 1 second
  if (millis() - ota_progress_millis > 250) 
  {
      ota_progress_millis = millis();

      if(!pageScreenIsShown)
      {
        hmi.writeString("page screen");
        //hmi.writeString("screen.updateBar.bco=23275");     
        pageScreenIsShown = true;
      }

      size_t fileSize = totalSize / 1024;
      fileSize = (round(fileSize*10)) / 10;
      // int prg = 0;

      // if (upload.totalSize != 0)
      // {
      //   prg = (upload.currentSize / upload.totalSize)*100;
      // }
      // else
      // {
      //   prg = 0;
      // }
      
      //String fileName = upload.filename;
      hmi.writeString("statusLCD.txt=\"FIRMWARE UPDATING " + String(fileSize) + " KB\""); 

      // if(final!=0)
      // {
      //   hmi.writeString("screen.updateBar.val=" + String((current * 100) / final));    
      // }
      
  }
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
       
    hmi.writeString("statusLCD.txt=\"SSID: [" + String(ssid) + "]\"" );
    delay(1500);
    
    // Configures Static IP Address
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
        hmi.writeString("statusLCD.txt=\"WiFi-STA setting failed!\"" );
        wifiActive = false;
    }

    int trying_wifi = 0;

    while ((WiFi.waitForConnectResult() != WL_CONNECTED) && trying_wifi <= 10)
    {
      hmi.writeString("statusLCD.txt=\"WiFi Connection failed! - try " + String(trying_wifi) + "\"" );
      trying_wifi++;
      wifiActive = false;
      delay(500);
    }  

    if (trying_wifi > 10)
    {
      wifiActive = false;
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
    strcpy(strpath,"/powadcr_iface.tft");

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
    
        if (bl==0)
        {
          String res = "";

          // Una vez enviado el primer bloque esperamos 2s
          delay(2000);

          while (1)
          {
              res = hmi.readStr();

              if (res.indexOf(0x08) != -1)
              {
                int offset = (pow(16,6) * int(res[4])) + (pow(16,4) * int(res[3])) + (pow(16,2) * int(res[2])) + int(res[1]);
                
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
          while(1)
          {
            // Esperams un ACK 0x05
            res = hmi.readStr();
            if(res.indexOf(0x05) != -1)
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

void playWAV()
{
  WAVDecoder decoder;
  AudioSourceSDFAT source("/wav", ".wav");
  AudioKitStream i2s;
  AudioPlayer player(source, i2s, decoder);

  // setup output
  source.setFileFilter("*file_*");

  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sd_active = true;
  cfg.channels = 2;
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  i2s.begin(cfg);

  // setup player

  source.selectStream(FILE_LOAD.c_str());

  player.setVolume(1.0); 
  player.begin();
  
  tapeAnimationON();
  while(!STOP)
  {
    player.copy();
  }
  tapeAnimationOFF();
}

void playingFile()
{
  setAudioOutput();
  ESP32kit.setVolume(MAX_MAIN_VOL);

  sendStatus(REC_ST, 0);

  if (TYPE_FILE_LOAD == "TAP")
  {
      //hmi.getMemFree();
      pTAP.play();
      //Paramos la animación
      tapeAnimationOFF();  
      //pTAP.updateMemIndicator();
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
      //hmi.getMemFree();
      pTZX.play();
      //Paramos la animación
      tapeAnimationOFF(); 
  }
  else if (TYPE_FILE_LOAD == "WAV")
  {
    // Reproducimos el WAV file
    LAST_MESSAGE = "WAV file loaded";
    playWAV();
  }
  // else if (TYPE_FILE_LOAD == "TSX")
  // {
  //     #ifdef DEBUGMODE
  //       logln("");
  //       log("TSX - Starting to PLAY");
  //     #endif
  //     //hmi.getMemFree();
  //     pTSX.play();
  //     //Paramos la animación
  //     tapeAnimationOFF(); 
  // }  
}

void verifyConfigFileForSelection()
{
  // Vamos a localizar el fichero de configuracion especifico para el fichero seleccionado
  const int max_params = 4;
  String path = FILE_LAST_DIR;
  tConfig *fileCfg;
  char strpath[PATH_FILE_TO_LOAD.length() + 5] = {};
  strcpy(strpath,PATH_FILE_TO_LOAD.c_str());
  strcat(strpath,".cfg");

  // Abrimos el fichero de configuracion.
  File32 cfg;

  logln("");
  log("path: " + String(strpath));

  if (sdf.exists(strpath))
  {
    logln("");
    log("cfg path: " + String(strpath));

    if (cfg.open(strpath,O_READ))
    {
      // Leemos todos los parametros
      fileCfg = sdm.readAllParamCfg(cfg,max_params);

      logln("");
      log("cfg open");  

      for (int i;i<max_params;i++)
      {

          logln("");
          log("Param: " + fileCfg[i].cfgLine);

          if((fileCfg[i].cfgLine).indexOf("freq") != -1)
          {
            SAMPLING_RATE = (sdm.getValueOfParam(fileCfg[i].cfgLine,"freq")).toInt();

            logln("");
            log("Sampling rate: " + String(SAMPLING_RATE));

            switch (SAMPLING_RATE)
            {
            case 48000:
              hmi.writeString("menuAudio2.r0.val=1");
              break;

            case 44100:
              hmi.writeString("menuAudio2.r1.val=1");
              break;

            case 32000:
              hmi.writeString("menuAudio2.r2.val=1");
              break;

            case 22050:
              hmi.writeString("menuAudio2.r3.val=1");
              break;

            default:
              hmi.writeString("menuAudio2.r1.val=1");
              break;
            }
          }
          else if((fileCfg[i].cfgLine).indexOf("zerolevel") != -1)
          {
            ZEROLEVEL = sdm.getValueOfParam(fileCfg[i].cfgLine,"zerolevel").toInt();

            logln("");
            log("Zero level: " + String(ZEROLEVEL));

            if (ZEROLEVEL==1)
            {
              hmi.writeString("menuAudio2.lvlLowZero.val=1");
            }
            else
            {
              hmi.writeString("menuAudio2.lvlLowZero.val=0");
            }
          }
          else if((fileCfg[i].cfgLine).indexOf("blockend") != -1)
          {
            APPLY_END = sdm.getValueOfParam(fileCfg[i].cfgLine,"blockend").toInt();

            logln("");
            log("Terminator forzed: " + String(APPLY_END));

            if (APPLY_END==1)
            {
              hmi.writeString("menuAudio2.enTerm.val=1");
            }
            else
            {
              hmi.writeString("menuAudio2.enTerm.val=0");
            }
          }
          else if((fileCfg[i].cfgLine).indexOf("polarized") != -1)
          {
            if((sdm.getValueOfParam(fileCfg[i].cfgLine,"polarized")) == "1")
            {
              POLARIZATION = down;
              LAST_EAR_IS = down;
              INVERSETRAIN = true;

              if (POLARIZATION==down)
              {
                hmi.writeString("menuAudio2.polValue.val=1");

                logln("");
                log("Polarization INV: " + String(APPLY_END));

              }
              else
              {
                hmi.writeString("menuAudio2.polValue.val=0");

                logln("");
                log("Polarization DIR: " + String(APPLY_END));
              }
            }
            else
            {
              POLARIZATION = up;
              LAST_EAR_IS = up;
              INVERSETRAIN = false;
            }            
          }
      }

      if (INVERSETRAIN)
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
      else
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
    }

    cfg.close();
  }
}

void loadingFile(char* file_ch)
{
  // Cogemos el fichero seleccionado y lo cargamos           

  // Si no está vacio
  if (FILE_SELECTED) 
  {

    // Convierto a mayusculas
    PATH_FILE_TO_LOAD.toUpperCase();
  
    // Eliminamos la memoria ocupado por el actual insertado
    // y lo ejectamos
    if (FILE_PREPARED)
    {
      ejectingFile();
      FILE_PREPARED = false;      
    }
    // Verificamos si hay fichero de configuracion para este archivo seleccionado
    verifyConfigFileForSelection();

    // Cargamos el seleccionado.
    if (PATH_FILE_TO_LOAD.indexOf(".TAP") != -1)
    {
        // changeLogo(41);
        // Reservamos memoria
        myTAP.descriptor = (tTAPBlockDescriptor*)ps_calloc(MAX_BLOCKS_IN_TAP, sizeof(struct tTAPBlockDescriptor));        
        // Pasamos el control a la clase
        pTAP.setTAP(myTAP);   
        // Lo procesamos
        proccesingTAP(file_ch);
        TYPE_FILE_LOAD = "TAP";  
        BYTES_TOBE_LOAD = myTAP.size;
     
    }
    else if ((PATH_FILE_TO_LOAD.indexOf(".TZX") != -1) || (PATH_FILE_TO_LOAD.indexOf(".TSX") != -1) || (PATH_FILE_TO_LOAD.indexOf(".CDT") != -1))    
    {

        // Reservamos memoria
        myTZX.descriptor = (tTZXBlockDescriptor*)ps_calloc(MAX_BLOCKS_IN_TZX , sizeof(struct tTZXBlockDescriptor));        
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
    else if(PATH_FILE_TO_LOAD.indexOf(".WAV") != -1)
    {
      FILE_PREPARED = true;
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
        ejectingFile();
        FILE_PREPARED = false;      
      }
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
  //log("Eject executing");
  // Terminamos los players
  if (TYPE_FILE_LOAD == "TAP")
  {
      // Solicitamos el puntero _myTAP de la clase
      // para liberarlo
      if (myTAP.descriptor != nullptr)
      {
        // LAST_MESSAGE = "PSRAM cleanning";
        // delay(1500);
        free(pTAP.getDescriptor());
        // Finalizamos
        pTAP.terminate();
      }
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {
      // Solicitamos el puntero _myTZX de la clase
      // para liberarlo
      if (myTZX.descriptor != nullptr)
      {
        LAST_MESSAGE = "PSRAM cleanning";
        delay(1500);
        freeMemoryFromDescriptorTZX(pTZX.getDescriptor());
        free(pTZX.getDescriptor());
        // Finalizamos
        pTZX.terminate();
      }
  }  
  // else if (TYPE_FILE_LOAD == "TSX")
  // {
  //     // Solicitamos el puntero _myTSX de la clase
  //     // para liberarlo
  //     if (myTSX.descriptor != nullptr)
  //     {
  //       LAST_MESSAGE = "PSRAM cleanning";
  //       delay(1500);
  //       freeMemoryFromDescriptorTSX(pTSX.getDescriptor());
  //       free(pTSX.getDescriptor());
  //       // Finalizamos
  //       pTSX.terminate();
  //     }
  // }    
}



void prepareRecording()
{
    hmi.activateWifi(false);

    // // Cambiamos la frecuencia de la SD
    // // Forzamos a 4MHz
    // sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(4));    

    // Liberamos myTAP.descriptor de la memoria si existe 
    if (myTAP.descriptor!= nullptr) 
    {
      free(myTAP.descriptor);
    }
      
    // Inicializamos audio input
    if (REC_AUDIO_LOOP)
    {
      setAudioInOut();
    }
    else
    {
      setAudioInput();
    }
    
    
    taprec.set_kit(ESP32kit);    
    taprec.initialize(); 

    if (!taprec.createTempTAPfile())
    {
      LAST_MESSAGE = "Recorder not prepared.";
      //
    }
    else
    {
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
    
    //hmi.getMemFree();        
}

void RECready()
{
    prepareRecording();
}

void recordingFile()
{
    // Iniciamos la grabación (esto es un bucle "taprec.recording()")
    // si devuelve true --> acaba
    if (taprec.recording())
    {

        // No ha finalizado la grabación.
        // LOADING_STATE = 4;
    }
    else
    {
      // Ha acabado de grabar
      REC = false;
      STOP = true;
    }
}
void getAudioSettingFromHMI()
{
    if(myNex.readNumber("menuAdio.enTerm.val")==1)
    {
      APPLY_END = true;
    }
    else
    {
      APPLY_END = false;
    }

    if(myNex.readNumber("menuAdio.polValue.val")==1)
    {
      POLARIZATION = down;
    }
    else
    {
      POLARIZATION = up;
    }

    if(myNex.readNumber("menuAdio.lvlLowZero.val")==1)
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
    if (INVERSETRAIN)
    {
        // Empieza en UP
        POLARIZATION = down;
        LAST_EAR_IS = down;
    }
    else
    {
        // Empieza en DOWN
        POLARIZATION = up;
        LAST_EAR_IS = up;
    }  
}

void getPlayeableBlockTZX(tTZX tB, int inc)
{
    // Esta funcion busca el bloque valido siguiente
    // inc sera +1 o -1 dependiendo de si es FFWD o RWD
    while(!((tB.descriptor)[BLOCK_SELECTED].playeable))
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

void getPlayeableBlockTSX(tTZX tB, int inc)
{
    // Esta funcion busca el bloque valido siguiente
    // inc sera +1 o -1 dependiendo de si es FFWD o RWD
    while(!((tB.descriptor)[BLOCK_SELECTED].playeable))
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

void updateHMIOnBlockChange()
{
    if (TYPE_FILE_LOAD=="TAP")
    {
      hmi.setBasicFileInformation(myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size,true);
      // BYTES_LOADED = myTAP.size;
      // PROGRESS_BAR_TOTAL_VALUE = (myTAP.descriptor[BLOCK_SELECTED].offset * 100 ) / BYTES_TOBE_LOAD ;
    }
    else if(TYPE_FILE_LOAD=="TZX" || TYPE_FILE_LOAD=="CDT" || TYPE_FILE_LOAD=="TSX")
    {
      hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size,myTZX.descriptor[BLOCK_SELECTED].playeable);
      // BYTES_LOADED = myTZX.size;
      // PROGRESS_BAR_TOTAL_VALUE = (myTZX.descriptor[BLOCK_SELECTED].offset * 100 ) / BYTES_TOBE_LOAD ;
    }
    // else if(TYPE_FILE_LOAD=="TSX")
    // {
    //   hmi.setBasicFileInformation(myTSX.descriptor[BLOCK_SELECTED].name,myTSX.descriptor[BLOCK_SELECTED].typeName,myTSX.descriptor[BLOCK_SELECTED].size,myTSX.descriptor[BLOCK_SELECTED].playeable);
    //   // BYTES_LOADED = myTSX.size;
    //   // PROGRESS_BAR_TOTAL_VALUE = (myTSX.descriptor[BLOCK_SELECTED].offset * 100 ) / BYTES_TOBE_LOAD ;
    //   // logln("");
    //   // logln("File size: " + String(BYTES_TOBE_LOAD));
    // }   

    hmi.updateInformationMainPage(true);
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

          //LAST_MESSAGE = "Ejecting cassette.";

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
          // 
          setPolarization();

        }
        else if (REC)
        {
          LAST_MESSAGE = "Rec paused. Press PAUSE to start recording.";
          recAnimationON();

          if (MODEWAV)
          {
              TAPESTATE = 120;
          }
          else
          {
            TAPESTATE = 220;
          }
          // 
          AUTO_STOP = false;
        }
        else if (STOP)
        {
          // Esto lo hacemos para evitar repetir el mensaje infinitamente
          LAST_MESSAGE = "Press EJECT to select a file or REC.";
          STOP=false;
          setPolarization();                
        }
        else if (DISABLE_SD)
        {
          DISABLE_SD = false;
          // Deshabilitamos temporalmente la SD para poder subir un firmware
          sdf.end();
          int tout = 59;
          while(tout != 0)
          {
            delay(1000);
            hmi.writeString("debug.blockLoading.txt=\"" + String(tout) +"\"");
            tout--;
          }

          hmi.writeString("debug.blockLoading.txt=\"..\"");
          sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_SPEED_MHZ));  
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
      #ifdef DEBUGMODE
        logAlert("Tape state 1");
      #endif

      if (PLAY)
      {
          // Inicializamos la polarización de la señal al iniciar la reproducción.
          LAST_EAR_IS = POLARIZATION; 
          //
          LOADING_STATE = 1;      
          //Activamos la animación
          tapeAnimationON();   
          // Reproducimos el fichero
          LAST_MESSAGE = "Loading in progress. Please wait.";
          //            
          playingFile();
      }
      else if(EJECT)
      {
        TAPESTATE = 0;
        LOADING_STATE = 0;
      }
      else if (FFWIND || RWIND)
      {
        // Actualizamos el HMI
        updateHMIOnBlockChange();                  
        //
        FFWIND = false;
        RWIND = false;         
      }       
      else if (PAUSE)       
      {
          TAPESTATE = 3;
          LAST_MESSAGE = "Tape paused. Press play or select block.";
      }
      else if (STOP)
      {
        // Desactivamos la animación
        // esta condicion evita solicitar la parada de manera infinita
        logln("");
        log("Tape state 1");

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
        }

        setPolarization();
        STOP=false; //28/11
      }
      else if (REC)
      {
        // Expulsamos la cinta
        if (LOADING_STATE == 0 || LOADING_STATE == 2)
        {
          if (FILE_PREPARED)
          {
            ejectingFile();
          }

          FILE_PREPARED = false;
          FILE_SELECTED = false;
          hmi.clearInformationFile();        
          LAST_MESSAGE = "Rec paused. Press PAUSE to start recording.";
          tapeAnimationOFF();
          recAnimationON();
          TAPESTATE = 220;          
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
      // LOADING_STATE = 2;
      // //Activamos la animación
      // tapeAnimationOFF();
      // // Reproducimos el fichero
      // LAST_MESSAGE = "Stop playing.";
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
          TAPESTATE = 1;
      }
      else if (STOP)
      {
          TAPESTATE = 1;
      }
      else if (FFWIND || RWIND)
      {
        // Actualizamos el HMI
        updateHMIOnBlockChange();        
        //
        TAPESTATE = 3;
        FFWIND = false;
        RWIND = false; 
      }
      else if(EJECT)
      {
        TAPESTATE = 0;
        LOADING_STATE = 0;
      }      
      else
      {
        TAPESTATE = 3;
      }
      break;

    case 10: 
        //
        //File prepared. Esperando a PLAY / FFWD o RWD
        //
        if (PLAY)
        {
          if (FILE_PREPARED)
          {
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
            ejectingFile();
          }
          
          FILE_PREPARED = false;
          FILE_SELECTED = false;
          hmi.clearInformationFile();        
          TAPESTATE = 0; 
        }
        else if (FFWIND || RWIND)
        {
          // Actualizamos el HMI
          updateHMIOnBlockChange();
          //
          FFWIND = false;
          RWIND = false; 
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

          if(FILE_SELECTED)
          {
              // Si se ha seleccionado lo cargo en el cassette.     
              char* file_ch = (char*)ps_calloc(256,sizeof(char));
              PATH_FILE_TO_LOAD.toCharArray(file_ch, 256);
              
              loadingFile(file_ch);
              free(file_ch);

              // Ponemos FILE_SELECTED = false, para el proximo fichero     
              FILE_SELECTED = false;
              
              // Ahora miro si está preparado
              if(FILE_PREPARED)
              {
                #ifdef DEBUGMODE
                  logAlert("File inside the tape.");
                #endif
                
                if (!ABORT)
                {
                  LAST_MESSAGE = "File inside the TAPE.";
                  TAPESTATE = 10;
                  LOADING_STATE = 0;
                }
                else
                {
                  LAST_MESSAGE = "Aborting proccess.";
                  delay(2000);
                  TAPESTATE = 0;
                  LOADING_STATE = 0;
                }
              }
              else
              {

                #ifdef DEBUGMODE
                  logAlert("No file selected or empty file.");
                #endif

                LAST_MESSAGE = "No file selected or empty file.";                
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

                ejectingFile();
                FILE_PREPARED = false;
              }              
          }
      }
      else
      {
          TAPESTATE = 100;
          LOADING_STATE = 0;
      }
      break;

    case 120:
      if (PAUSE)
      {
        
        //Generamos un numero aleatorio para el final del nombre
        char* cPath = (char*)ps_calloc(55,sizeof(char));
        String wavfile = "/WAV/rec";
        cPath = strcpy(cPath, wavfile.c_str());
        srand(time(0));
        delay(125);
        int rn = rand()%999999;
        //Le unimos la extensión .TAP
        String txtRn = "-" + String(rn) + ".wav";
        char const *extPath = txtRn.c_str();
        strcat(cPath,extPath);

        // Comenzamos la grabacion
        setWavRecording(cPath);

        TAPESTATE = 0;
        LOADING_STATE = 0;
        RECORDING_ERROR = 0;
        REC = false;
        recAnimationOFF();
        recAnimationFIXED_OFF();  
        free(cPath);
      }
      else
      {
        TAPESTATE = 120;
      }
      break;

    case 220:
      if(PAUSE)
      {
        // Pulsacion de PAUSE
        // Iniciamos la grabacion
        PLAY = false;
        PAUSE = false;
        STOP = false;
        REC = true;
        ABORT = false;
        EJECT = false;
        // Preparamos la grabacion
        recAnimationOFF();
        prepareRecording();
        recAnimationFIXED_ON();
        TAPESTATE = 200;
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
        LOADING_STATE = 0;
        TAPESTATE = 220;
      }
      break;
    case 200: 
      //
      // REC
      //
      if(STOP || taprec.actuateAutoRECStop || !REC || EJECT || RECORDING_ERROR!=0)
      {
        //
        stopRecording();
        recAnimationFIXED_OFF();
        //
        taprec.stopRecordingProccess = false;
        taprec.actuateAutoRECStop = false;
        REC=false;
        //Volvemos al estado de reposo
        TAPESTATE = 0;
        LOADING_STATE = 0;
        RECORDING_ERROR = 0;
      }
      else
      {
        esp_task_wdt_reset();
        recordingFile();
        LOADING_STATE = 4;
        TAPESTATE = 200;
      }
      break;

    default:
      #ifdef DEBUGMODE
        logAlert("Tape state unknow.");
      #endif
      break;
  }
}


bool headPhoneDetection()
{
  return !gpio_get_level((gpio_num_t)HEADPHONE_DETECT);
}

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
    .format_if_mount_failed = false
  };

  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret !=ESP_OK)
  {
    if (ret == ESP_FAIL)
      log_e("Failed to mount or format filesystem");
    else if (ret == ESP_ERR_NOT_FOUND)
      log_e("Failed to find SPIFFS partition");
    else
      log_e("Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret!= ESP_OK)
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

void Task1code( void * pvParameters )
{
    //setup();
    for(;;) 
    {
        if (serialEventRun) serialEventRun();

        esp_task_wdt_reset();
        tapeControl();
    }
}

void Task0code( void * pvParameters )
{

  #ifndef SAMPLINGTEST
    // Core 0 - Para el HMI
    int startTime = millis();
    int startTime2 = millis();
    int tClock = millis();
    int ho=0;int mi=0;int se=0;
    int tScrRfsh = 125;
    int tAckRfsh = 1500;


    for(;;)
    {

        hmi.readUART();

        // Control por botones
        //buttonsControl();
        //delay(50);

        esp_task_wdt_reset();

        #ifndef DEBUGMODE
          if (REC)
          {
              tScrRfsh = 250;
          }
          else
          {
              tScrRfsh = 125;
          }

          if ((millis() - startTime) > tScrRfsh)
          {
            startTime = millis();
            stackFreeCore1 = uxTaskGetStackHighWaterMark(Task1);    
            stackFreeCore0 = uxTaskGetStackHighWaterMark(Task0);        
            hmi.updateInformationMainPage();
          }           
        #endif
    }
  #endif
}

void setup() 
{
    // Inicializar puerto USB Serial a 115200 para depurar / subida de firm
    Serial.begin(115200);
    
    // Configuramos el size de los buffers de TX y RX del puerto serie
    SerialHW.setRxBufferSize(4096);
    SerialHW.setTxBufferSize(4096);
    // Configuramos la velocidad del puerto serie
    SerialHW.begin(SerialHWDataBits,SERIAL_8N1,hmiRxD,hmiTxD);
    //SerialHW.begin(512000);
    delay(125);

    // Forzamos un reinicio de la pantalla
    hmi.writeString("rest");
    delay(125);

    // Indicamos que estamos haciendo reset
    sendStatus(RESET, 1);
    delay(250);

    
    hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"" );   
    delay(2000);

    //SerialHW.println("Setting Audiokit.");

    // Configuramos los pulsadores
    // configureButtons();

    // Configuramos el ESP32kit
    LOGLEVEL_AUDIOKIT = AudioKitError;

    // Configuracion de las librerias del AudioKit
    hmi.writeString("statusLCD.txt=\"Setting audio\"");
    setAudioOutput();

    //SerialHW.println("Done!");

    //SerialHW.println("Initializing SD SLOT.");
    
    // Configuramos acceso a la SD
    hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"" );
    delay(125);

    int SD_Speed = SD_FRQ_MHZ_INITIAL;  // Velocidad en MHz (config.h)
    SD_SPEED_MHZ = setSDFrequency(SD_Speed);
    
    // Para forzar a un valor, descomentar esta linea
    // y comentar las dos de arriba
    //sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(8));
   
    // Le pasamos al HMI el gestor de SD
    hmi.set_sdf(sdf);

    if(psramInit()){
      //SerialHW.println("\nPSRAM is correctly initialized");
      hmi.writeString("statusLCD.txt=\"PSRAM OK\"" );

    }else{
      //SerialHW.println("PSRAM not available");
      hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"" );
    }    
    delay(125);

    // -------------------------------------------------------------------------
    // Inicializar SPIFFS mínimo
    // -------------------------------------------------------------------------
    hmi.writeString("statusLCD.txt=\"Initializing SPIFFS\"" );    
    initSPIFFS();
    if (esp_spiffs_mounted)
    {
      hmi.writeString("statusLCD.txt=\"SPIFFS mounted\"" );
    }
    else
    {
      hmi.writeString("statusLCD.txt=\"SPIFFS error\"" );
    }
    delay(2000);
    

    // -------------------------------------------------------------------------
    // Actualización OTA por SD
    // -------------------------------------------------------------------------

    log_v("");
    log_v("Search for firmware..");
    char strpath[20] = {};
    strcpy(strpath,"/firmware.bin");
    File32 firmware =  sdm.openFile32(strpath);
    if (firmware) 
    {
      hmi.writeString("statusLCD.txt=\"New powadcr firmware found\"" );       
      onOTAStart();
      log_v("found!");
      log_v("Try to update!");
 
      Update.onProgress(onOTAProgress);
 
      Update.begin(firmware.size(), U_FLASH);
      Update.writeStream(firmware);
      if (Update.end())
      {
        log_v("Update finished!");
        hmi.writeString("statusLCD.txt=\"Update finished\"" );        
        onOTAEnd(true);
      }
      else
      {
        log_e("Update error!");
        hmi.writeString("statusLCD.txt=\"Update error\"" );         
        onOTAEnd(false);
      }
 
      firmware.close();
 
      if (sdf.remove(strpath))
      {
        log_v("Firmware deleted succesfully!");
      }
      else{
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
        strcpy(strpath,"/powadcr_iface.tft");

        uploadFirmDisplay(strpath);
        sdf.remove(strpath);
        // Esperamos al reinicio de la pantalla
        delay(5000);
    }

    // -------------------------------------------------------------------------
    // Cargamos configuración WiFi
    // -------------------------------------------------------------------------
    // Si hay configuración activamos el wifi
    if(loadWifiCfgFile());
    {
      //Si la conexión es correcta se actualiza el estado del HMI
      if(wifiSetup())
      {
          // Enviamos información al menu
          hmi.writeString("menu.wifissid.txt=\"" + String(ssid) + "\"");
          delay(125);
          hmi.writeString("menu.wifipass.txt=\"" + String(password) + "\"");
          delay(125);
          hmi.writeString("menu.wifiIP.txt=\"" + WiFi.localIP().toString() + "\"");
          delay(125);
          hmi.writeString("menu.wifiEn.val=1");      
          delay(125);


          configureWebServer();
          server.begin(); 
      }
    }

    delay(750);

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
          hmi.writeString("statusLCD.txt=\"Creating FAV directory\"" );
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
          hmi.writeString("statusLCD.txt=\"Creating REC directory\"" );
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
          hmi.writeString("statusLCD.txt=\"Creating WAV directory\"" );
          hmi.reloadCustomDir("/");
          delay(750);
        }
    }    
    // -------------------------------------------------------------------------
    // Esperando control del HMI
    // -------------------------------------------------------------------------
    
    //Paramos la animación de la cinta1
    tapeAnimationOFF();  
    // changeLogo(0);

    hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"" );
    waitForHMI(CFG_FORZE_SINC_HMI);


    // -------------------------------------------------------------------------
    // Forzamos configuración estatica del HMI
    // -------------------------------------------------------------------------
    // Inicializa volumen en HMI
    hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volR.val=" + String(MAIN_VOL_R));
    hmi.writeString("menuAudio.volLevelL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_R));

    // Asignamos el HMI
    pTAP.set_HMI(hmi);
    pTZX.set_HMI(hmi);
    // pTSX.set_HMI(hmi);
    //y el gestor de ficheros
    pTAP.set_SDM(sdm);
    pTZX.set_SDM(sdm);
    // pTSX.set_SDM(sdm);

    zxp.set_ESP32kit(ESP32kit);
    
    // Si es test está activo. Lo lanzamos
    #ifdef TEST
      TEST_RUNNING = true;
      hmi.writeString("statusLCD.txt=\"TEST RUNNING\"" );
      test();
      hmi.writeString("statusLCD.txt=\"PRESS SCREEN\"" );
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

    // sendStatus(STOP_ST, 1);
    // sendStatus(PLAY_ST, 0);
    // sendStatus(PAUSE_ST, 0);
    // sendStatus(READY_ST, 1);
    // sendStatus(END_ST, 0);

    sendStatus(REC_ST);

    LAST_MESSAGE = "Press EJECT to select a file or REC.";
    
    esp_task_wdt_init(WDT_TIMEOUT, false);  // enable panic so ESP32 restarts
    // Control del tape
    xTaskCreatePinnedToCore(Task1code, "TaskCORE1", 16384, NULL, 3|portPRIVILEGE_BIT, &Task1, 0);
    esp_task_wdt_add(&Task1);  
    delay(500);
    
    // Control de la UART - HMI
    xTaskCreatePinnedToCore(Task0code, "TaskCORE0", 8192, NULL, 3|portPRIVILEGE_BIT, &Task0, 1);
    esp_task_wdt_add(&Task0);  
    delay(500);

    // Inicializamos el modulo de recording
    taprec.set_HMI(hmi);
    taprec.set_SdFat32(sdf);
    
    //hmi.getMemFree();
    taskStop = false;

    // Ponemos el color del scope en amarillo
    //hmi.writeString("tape.scope.pco0=60868");
    #ifdef DEBUGMODE
      hmi.writeString("tape.name.txt=\"DEBUG MODE ACTIVE\"" );
    #endif
}

void loop()
{

}