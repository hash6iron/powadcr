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
#define powerLed 15


#include <esp_task_wdt.h>
#define WDT_TIMEOUT 360000  

#include <HardwareSerial.h>
HardwareSerial SerialHW(2);

EasyNex myNex(SerialHW);

#include "SdFat.h"
#include "globales.h"

#include "AudioTools/AudioLibs/AudioKit.h"
AudioKit ESP32kit;

// const char* file_name = "/output.wav";
// audio_tools::AudioInfo info(44100, 2, 8);
// I2SStream inWAV;
// File32 fileOutToWav;
// // File file;  // final output stream
// EncodedAudioStream outWAV(&fileOutToWav, new WAVEncoder());
// StreamCopy copierOutToWav(outWAV, inWAV);  // copies data

//#include "AudioTools/AudioLibs/I2SCodecStream.h"



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

// Declaraciones para SdFat
SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;


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
//  #include "webpage.h"
//  #include "webserver.h"

// WAV Recorder
// -----------------------------------------------------------------------
// #include "AudioTools/AudioLibs/AudioSourceSDFAT.h"
#include "AudioTools/Disk/AudioSourceSDFAT.h"
#include "AudioTools/AudioCodecs/CodecWAV.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
// #include "AudioTools/AudioLibs/I2SCodecStream.h"
// #include <AudioTools/AudioCodecs/CodecADPCM.h>
// #include "AudioTools/AudioLibs/A2DPStream.h"
// #include "AudioTools/AudioCodecs/CodecFLAC.h"

using namespace audio_tools;  




// Variables
//
// -----------------------------------------------------------------------
int posRotateName = 0;  // Posicion del texto rotativo en tape.name.txt (HMI)
int moveDirection = 1;
bool rotate_enable = false;

int plLastSize = 0;
int plLastpos = 0;
int plLastFsize = 0;
String plLastName = "";

// -----------------------------------------------------------------------
// Prototype function

void ejectingFile();
void isGroupStart();
void isGroupEnd();

void rewindAnimation(int direction);


// -----------------------------------------------------------------------



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
      hmi.writeString("mainmenu.verFirmware.txt=\" powadcr " + String(VERSION) + "\"");
      hmi.writeString("page tape");
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
      SdSpiConfig sdcfg(ESP32kit.pinSpiCs(),SHARED_SPI,SD_SCK_MHZ(SD_SPEED_MHZ),&SPI);
      if (!sdf.begin(sdcfg) || lastStatus) 
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
    logln("");
    log("Error in Audiokit output setting");
  }

  switch (SAMPLING_RATE)
  {
      case 48000:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_48K_SAMPLES))
        {
          //logln("Error in Audiokit sampling rate setting");
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
          //logln("Error in Audiokit sampling rate setting");
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
          //logln("Error in Audiokit sampling rate setting");
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
          //logln("Error in Audiokit sampling rate setting");
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
  cfg.bits_per_sample = AUDIO_HAL_BIT_LENGTH_16BITS;

  if(!ESP32kit.begin(cfg))
  {
    //log("Error in Audiokit input setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
  {
    //logln("Sampling rate setting not 44.1KHz");
  }

  ESP32kit.setSpeakerActive(ACTIVE_AMP);
}

void setAudioInOut()
{
  auto cfg = ESP32kit.defaultConfig(KitInputOutput);
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // Line with high gain
  cfg.bits_per_sample = AUDIO_HAL_BIT_LENGTH_16BITS;
  
  if(!ESP32kit.begin(cfg))
  {
    //log("Error in Audiokit output setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
  {
    //logln("Sampling rate setting not 44.1KHz");
  }

  if (!ESP32kit.setVolume(100))
  {
    //log("Error in volumen setting");
  }   

  ESP32kit.setSpeakerActive(ACTIVE_AMP);

}


void tapeAnimationON()
{
    hmi.writeString("tape2.tmAnimation.en=1"); 
    hmi.writeString("tape.tmAnimation.en=1");
    delay(250);
    hmi.writeString("tape2.tmAnimation.en=1"); 
    hmi.writeString("tape.tmAnimation.en=1");         
}

void tapeAnimationOFF()
{
    hmi.writeString("tape2.tmAnimation.en=0"); 
    hmi.writeString("tape.tmAnimation.en=0"); 
    delay(250);  
    hmi.writeString("tape2.tmAnimation.en=0"); 
    hmi.writeString("tape.tmAnimation.en=0"); 
}

void recAnimationON()
{
  hmi.writeString("tape.RECst.val=1");
    delay(250);  
  hmi.writeString("tape.RECst.val=1");
}

void recAnimationOFF()
{
  hmi.writeString("tape.RECst.val=0");
    delay(250);  
  hmi.writeString("tape.RECst.val=0");
}

void recAnimationFIXED_ON()
{
  hmi.writeString("tape.recIndicator.bco=63848");
    delay(250);  
  hmi.writeString("tape.recIndicator.bco=63848");
}

void recAnimationFIXED_OFF()
{
  hmi.writeString("tape.recIndicator.bco=32768");
    delay(250);  
  hmi.writeString("tape.recIndicator.bco=32768");
}

void setWavRecording(char* file_name,bool start=true)
{
    // AudioLogger::instance().begin(Serial, AudioLogger::Error);

    unsigned long progress_millis = 0;
    int rectime_s = 0;
    int rectime_m = 0;

    if (start)
    {
        if (sdf.exists(file_name))
        {
            sdf.remove(file_name);
        }  

        // open file for recording WAV
        wavfile = sdf.open(file_name, O_WRITE | O_CREAT);

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
            FILE_LOAD = file_name;

            // Paramos el ESp32kit antes de configurar otra vez
            ESP32kit.end();
            AudioInfo lineInCfg(44100, 2, 16);
            // ADPCMEncoder adpcm_encoder(AV_CODEC_ID_ADPCM_MS); 
            // WAVEncoder wavencoder(adpcm_encoder, AudioFormat::ADPCM);            
            AudioKitStream kit;
            EncodedAudioStream encoder(&wavfile, new WAVEncoder());
            StreamCopy copier(encoder, kit);  // copies data to SD
 
            auto cfg = kit.defaultConfig(RX_MODE);
            cfg.copyFrom(lineInCfg);

            cfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2; // Line in
            cfg.channel_format = I2S_CHANNEL_STEREO;
            cfg.channels = 2;
            cfg.bits_per_sample = 16;
            cfg.sample_rate = 44100;

            // Inicializamos
            kit.begin(cfg);

            // we need to provide the audio information to the encoder
            encoder.begin(lineInCfg);
            
            LAST_MESSAGE = "Recording to WAV - Press STOP to finish.";
            
            recAnimationOFF();
            delay(125);
            recAnimationFIXED_ON();
            tapeAnimationON();

            // Muestro solo el nombre. Le elimino la primera parte que es el directorio.
            hmi.writeString("name.txt=\"" + String(file_name).substring(5) + "\"");
            hmi.writeString("type.txt=\"WAV file\"");

            // Redimensionamos el buffer de grabacion 
            copier.resize(256 * 1024);

            while(!STOP)
            {
              copier.copy();

              if (millis() - progress_millis > 1000) 
              {
                  progress_millis = millis();
                  LAST_MESSAGE = "Recording time: " + ((rectime_m < 10 ? "0" : "") + String(rectime_m)) 
                                          + ":" 
                                          + ((rectime_s < 10 ? "0" : "") + String(rectime_s));

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

              }

            }

            wavfile.flush();

              logln("File has ");
              log(String(wavfile.size()/1024));
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

            kit.end();
            encoder.end();
            // copier.end();

            // delay(2000);               
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
// unsigned long ota_progress_millis = 0;

void onOTAStart() 
{
  pageScreenIsShown = false;
}

void onOTAProgress(size_t currSize, size_t totalSize)
{
    // log_v("CALLBACK:  Update process at %d of %d bytes...", currSize, totalSize);
  
    #ifdef DEBUGMODE
      logln("Uploading status: " + String(currSize/1024) + "/" + String(totalSize/1024) + " KB");
    #endif

    if(!pageScreenIsShown)
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
       
    hmi.writeString("statusLCD.txt=\"SSID: [" + String(ssid) + "]\"" );
    delay(1500);
    
    // Configures Static IP Address
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
        hmi.writeString("statusLCD.txt=\"WiFi-STA setting failed!\"" );
        wifiActive = false;
    }

    int trying_wifi = 0;

    while ((WiFi.waitForConnectResult() != WL_CONNECTED) && trying_wifi <= 2)
    {
      hmi.writeString("statusLCD.txt=\"WiFi Connection failed! - try " + String(trying_wifi) + "\"" );
      trying_wifi++;
      wifiActive = false;
      // delay(125);
    }  

    if (trying_wifi > 2)
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

void prepareOutputToWav()
{
    String wavnamepath = "/WAV/" + FILE_LOAD.substring(0,FILE_LOAD.length()-4) + ".wav";
    char file_name[255];
    strcpy(file_name,wavnamepath.c_str());

    logln("Output to WAV: " + wavnamepath);

    // AudioLogger::instance().begin(Serial, AudioLogger::Error);

    // open file for recording WAV
    wavfile = sdf.open(file_name, O_WRITE | O_CREAT);

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

    // Configuramos el encoder para salida a 44.1KHz, 16-bits, STEREO
    AudioInfo wavencodercfg(44100, 2, 16);
    // Inicializamos el encoder
    encoderOutWAV.begin(wavencodercfg);

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
    strcpy(pfile,path.c_str());
    File32 f = sdm.openFile32(pfile);
    int fsize = f.size();
    f.close();  

    if (fsize < 1000000)
    {
      hmi.writeString("size.txt=\"" + String(fsize/1024) + " KB\"");
    }
    else
    {
      hmi.writeString("size.txt=\"" + String(fsize/1024/1024) + " MB\"");
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
      strBitrate = "(" + String(bitrate / 1000) + " Kbps)";
    }
    else
    {
      strBitrate = "(-- Kbps)";
    }
    
    if (TYPE_FILE_LOAD =="WAV")
    {
      strBitrate = "";
    }

    hmi.writeString("tape.totalBlocks.val=" + String(size));
    hmi.writeString("tape.currentBlock.val=" + String(pos));
    hmi.writeString("type.txt=\"" + TYPE_FILE_LOAD + " file " + strBitrate + "\"");

    if (fsize < 1000000)
    {
      hmi.writeString("size.txt=\"" + String(fsize/1024) + " KB\"");
    }
    else
    {
      hmi.writeString("size.txt=\"" + String(fsize/1024/1024) + " MB\"");
    }
}

void estimatePlayingTime(int fileread, int filesize, int samprate)
{

    if (samprate > 0 ) 
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

void playMP3()
{
    ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES);
    ESP32kit.setSpeakerActive(ACTIVE_AMP);

    SAMPLING_RATE = 44100;
    
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
    size_t bufferSize = 256 * 1024;

    int pressedCountFFWD = 0;
    int pressedCountRWD = 0;

    String txtR = "";

    // Configuracion de la fuente desde SD y audio
    SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS,SHARED_SPI,SD_SCK_MHZ(SD_SPEED_MHZ),&SPI);
    AudioSourceSDFAT source(FILE_LAST_DIR.c_str(),"mp3",sdcfg);
    // Configuracion de audio
    // AudioInfo info(44100, 2, 16);
    AudioKitStream kit;

    // Definidmos el decoder para MP3
    MP3DecoderHelix decoder;
    // Lo usaremos para filtrar la metadata
    // MetaDataID3 ID3;
    MetaDataFilterDecoder metadatafilter(decoder);

    // ID3.begin();
    // ID3.setCallback()

    // Configuramos el player
    AudioPlayer player(source,kit,metadatafilter);

    // Ecualizer
    audio_tools::Equalizer3Bands eq(kit);
    audio_tools::ConfigEqualizer3Bands cfg_eq;

    // Configuracion del booster
    Boost boost(0.5);

    // Generamos la configuracion para la salida de audio del Audiokit
    auto cfg = kit.defaultConfig(TX_MODE);  
    cfg.bits_per_sample = 16;
    cfg.channels = 2;
    cfg.sample_rate = 44100;
    cfg.buffer_count = 2;
    cfg.buffer_size = bufferSize;

    // Configuramos el ecualizador
    cfg_eq = eq.defaultConfig();
    cfg_eq.setAudioInfo(cfg); // use channels, bits_per_sample and sample_rate from kit
    cfg_eq.gain_low = EQ_LOW; 
    cfg_eq.gain_medium = EQ_MID;
    cfg_eq.gain_high = EQ_HIGH;
    
    // Hacemos pasar el decoder por el ecualizador de bandas
    decoder.setOutput(eq);

    // Configuramos la salida de audio del Audiokit
    kit.begin(cfg);
    kit.setSpeakerActive(ACTIVE_AMP);
    kit.setVolume(MAIN_VOL/100);
    
    // Configuramos el player
    // dejamos que el player al 100% de volumen y ajustamos el volumen en el Audiokit
    player.setVolume(1);
    player.setAutoNext(false); 
    player.setBufferSize(bufferSize);

    // Iniciamos el ecualizador
    eq.begin(cfg_eq);
    
    // Arrancamos el player0
    if(player.begin())
    {
        LAST_MESSAGE =  "Audio ready to play";

        player.setPath(PATH_FILE_TO_LOAD.c_str());
        fileSize = getStreamfileSize(PATH_FILE_TO_LOAD);

        player.setActive(true);
        // Indicamos cual es el fichero seleccionado
        source.selectStream(FILE_PTR_POS + FILE_IDX_SELECTED - 1);

        // Esto lo hacemos para parar cuando termine el audio.
        int currentIdx = (FILE_PTR_POS + FILE_IDX_SELECTED) - 1;

        // Punteros de ficheros total y en curso
        char path[255];
        strcpy(path,FILE_LAST_DIR.c_str());
        totalFilesIdx = source.size();
        //
        endStreamFile = totalFilesIdx - 1;

        // Esto lo hacemos para coger el bitrate

        bitRateRead = decoder.audioInfoEx().bitrate;
        updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());

        // Comenzamos el bucle de control
        while(!EJECT && !REC)
        {          
          
          player.setVolume(1);
          kit.setVolume(MAIN_VOL/100);

          if (EQ_CHANGE)
          {
            EQ_CHANGE = false;

            cfg_eq.gain_low = EQ_LOW; 
            cfg_eq.gain_medium = EQ_MID;
            cfg_eq.gain_high = EQ_HIGH;

            eq.begin(cfg_eq);
          }
  
          switch (stateStreamplayer)
          {
            case 0:
                // Fichero a la espera
                if (PLAY)
                {
                    LAST_MESSAGE =  "Playing Audio.";
                    currentIdx = source.index();
                    player.setVolume(0);
                    kit.setVolume(0);     
                    bitRateRead = decoder.audioInfoEx().bitrate;             
                    updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

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
  
                    bitRateRead = decoder.audioInfoEx().bitrate;
                    updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

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
                      
                      if (disable_auto_wav_stop)
                      {
                          disable_auto_wav_stop = false;
                          // Play auto-stop
                          hmi.writeString("tape.playType.txt=\"*\"");
                          hmi.writeString("tape.playType.y=3");
                          hmi.writeString("doevents");
                      }
                      else
                      {
                          disable_auto_wav_stop = true;
                          // Play continuo
                          hmi.writeString("tape.playType.txt=\"C\"");
                          hmi.writeString("tape.playType.y=6");
                          hmi.writeString("doevents");
                      }
                    }

                    // Vamos transmitiendo y quedandonos con el numero de bytes transmitidos
                    if (offset!=0)
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
                    PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
                    PROGRESS_BAR_BLOCK_VALUE = PROGRESS_BAR_TOTAL_VALUE;

                    // Se detecta cambio de fichero al siguiente.
                    //int idx = source.index();
                    if (((fileread+1)/(fileSize+1)) >= 1)
                    {

                        PROGRESS_BAR_TOTAL_VALUE = 0;
                        PROGRESS_BAR_BLOCK_VALUE = 0;

                        // Si auto-stop activo, entonces paro
                        if (!disable_auto_wav_stop)
                        {
                            LAST_MESSAGE = "Stop playing";
                            PLAY=false;
                            stateStreamplayer = 4;
                            tapeAnimationOFF();
                        }
                        else
                        {
                            // Pasamos a la siguiente pista
                            logln("Current IDX: " + String(currentIdx) + " / " + String(source.size()));

                            // Comprobamos si hemos llegado al final de
                            // la lista de reproduccion (play list)
                            if (currentIdx > endStreamFile)
                            {
                                source.selectStream(0);
                                source.setIndex(0);
                                player.begin();

                                logln("Change IDX: " + String(source.index()) + " / " + String(source.size()));
                            }
                            
                            // Actualizamos la referencia de cambio de pista 
                            currentIdx = source.index();  
                            //
                            fileSize = getStreamfileSize(source.toStr());
                            updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());
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
                    disable_auto_wav_stop = false;
                    PROGRESS_BAR_TOTAL_VALUE = 0;
                    PROGRESS_BAR_BLOCK_VALUE = 0;
                }
                
                // Avance de pistas
                if (FFWIND || RWIND)
                {
                    if (FFWIND)
                    {
                        // if (pressedCountFFWD > 2)
                        // {
                        //     pressedCountFFWD = 0;
                        //     FFWIND = false;
                        //     RWIND = false;
                        //     offset = 0;
                        // }
                        // else
                        // {
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
                    bitRateRead = decoder.audioInfoEx().bitrate;
                    updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

                    FFWIND = false;
                    RWIND = false;
                    fileread = 0;                    
                }
                // else if (KEEP_FFWIND)
                // {
                  
                //   offset += bufferSize;
                //   pressedCountFFWD++;
                //   delay(25);

                // }
                // else if (KEEP_RWIND)
                // {
                  
                //   offset += bufferSize;
                //   offset *= -1;

                //   if ((last_fileread + offset) < 0)
                //   {
                //     last_fileread = 0;
                //     offset = 1;
                //   }

                //   delay(25);
                // }
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
                    disable_auto_wav_stop = false;                    
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
                    updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

                    // Lo ponemos asi para que empiece en CONTINUOS PLAYING
                    // ya que en el if se hace un cambio dependiendo del valor anterior
                    disable_auto_wav_stop = false;  
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
                disable_auto_wav_stop = false; 

                PLAY=false;         
                PROGRESS_BAR_TOTAL_VALUE = 0;
                PROGRESS_BAR_BLOCK_VALUE = 0;

                break;

          default:
            break;
          }

          // Actualizamos el indicador
          if ((millis() - startTime) > 4000)
          {
            updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());
            startTime = millis();
          }       
          
        }
        
        tapeAnimationOFF(); 
        player.end();

        moveDirection = 1;
        posRotateName = 0;        
    }
    else
    {
      logln("Error player initialization");
      STOP=true;
    } 

    // Recupero la SD para el resto de procesadores.
    if (!sdf.begin(sdcfg)) 
    {
      logln("Error recovering spi initialization");
    }  

    rotate_enable = false;
}

void playWAV()
{
  ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES);
  ESP32kit.setSpeakerActive(ACTIVE_AMP);

  SAMPLING_RATE = 44100;
  
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

  SdSpiConfig sdcfg(PIN_AUDIO_KIT_SD_CARD_CS,SHARED_SPI,SD_SCK_MHZ(SD_SPEED_MHZ),&SPI);
  AudioSourceSDFAT source(FILE_LAST_DIR.c_str(),"wav",sdcfg);
  AudioKitStream kit;
  WAVDecoder decoder;

  FormatConverterStream conv(kit);              

  AudioInfo from(decoder.audioInfo().sample_rate, decoder.audioInfo().channels, decoder.audioInfo().bits_per_sample);                                           
  AudioInfo to(decoder.audioInfo().sample_rate, decoder.audioInfo().channels, decoder.audioInfo().bits_per_sample); 

  conv.setBuffered(true);
  conv.begin(from,to);  

  AudioPlayer player(source,kit,decoder);

  auto cfg = kit.defaultConfig(TX_MODE);

  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.sample_rate = 44100;

  kit.begin(cfg);
  kit.setSpeakerActive(ACTIVE_AMP);
  
  // setup player
  player.setVolume(1);
  kit.setVolume(MAIN_VOL/100);

  // playerbt.setVolume(MAIN_VOL/100);
  player.setAutoNext(false); 
  player.setBufferSize(256 * 1024);


  logln("Current position: " + String(FILE_PTR_POS + FILE_IDX_SELECTED));
  
  // Arrancamos el player0
  if(player.begin())
  {
      LAST_MESSAGE =  "Audio ready to play";

      player.setPath(PATH_FILE_TO_LOAD.c_str());
      fileSize = getStreamfileSize(PATH_FILE_TO_LOAD);

      player.setActive(true);
      // Indicamos cual es el fichero seleccionado
      source.selectStream(FILE_PTR_POS + FILE_IDX_SELECTED - 1);

      // Esto lo hacemos para parar cuando termine el audio.
      int currentIdx = (FILE_PTR_POS + FILE_IDX_SELECTED) - 1;

      // Punteros de ficheros total y en curso
      totalFilesIdx = source.size();
      endStreamFile = totalFilesIdx - 1;

      // Esto lo hacemos para coger el bitrate

      bitRateRead = 0;
      updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());

      // Comenzamos el bucle de control
      while(!EJECT && !REC)
      {          
        
        player.setVolume(1);
        kit.setVolume(MAIN_VOL/100);

        // if (EQ_CHANGE)
        // {
        //   EQ_CHANGE = false;

        //   cfg_eq.gain_low = EQ_LOW; 
        //   cfg_eq.gain_medium = EQ_MID;
        //   cfg_eq.gain_high = EQ_HIGH;

        //   eq.begin(cfg_eq);
        // }

        switch (stateStreamplayer)
        {
          case 0:
              // Fichero a la espera
              if (PLAY)
              {
                  LAST_MESSAGE =  "Playing Audio.";
                  currentIdx = source.index();
                  player.setVolume(0);
                  kit.setVolume(0);     
                  bitRateRead = 0;             
                  updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

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

                  bitRateRead = 0;
                  updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

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
                    
                    if (disable_auto_wav_stop)
                    {
                        disable_auto_wav_stop = false;
                        // Play auto-stop
                        hmi.writeString("tape.playType.txt=\"*\"");
                        hmi.writeString("tape.playType.y=3");
                        hmi.writeString("doevents");
                    }
                    else
                    {
                        disable_auto_wav_stop = true;
                        // Play continuo
                        hmi.writeString("tape.playType.txt=\"C\"");
                        hmi.writeString("tape.playType.y=6");
                        hmi.writeString("doevents");
                    }
                  }

                  // Vamos transmitiendo y quedandonos con el numero de bytes transmitidos
                  if (offset!=0)
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
                  PROGRESS_BAR_TOTAL_VALUE = (fileread * 100) / fileSize;
                  PROGRESS_BAR_BLOCK_VALUE = PROGRESS_BAR_TOTAL_VALUE;

                  // Se detecta cambio de fichero al siguiente.
                  //int idx = source.index();
                  if (((fileread+1)/(fileSize+1)) >= 1)
                  {

                      PROGRESS_BAR_TOTAL_VALUE = 0;
                      PROGRESS_BAR_BLOCK_VALUE = 0;

                      // Si auto-stop activo, entonces paro
                      if (!disable_auto_wav_stop)
                      {
                          LAST_MESSAGE = "Stop playing";
                          PLAY=false;
                          stateStreamplayer = 4;
                          tapeAnimationOFF();
                      }
                      else
                      {
                          // Pasamos a la siguiente pista
                          logln("Current IDX: " + String(currentIdx) + " / " + String(source.size()));

                          // Comprobamos si hemos llegado al final de
                          // la lista de reproduccion (play list)
                          if (currentIdx > endStreamFile)
                          {
                              source.selectStream(0);
                              source.setIndex(0);
                              player.begin();

                              logln("Change IDX: " + String(source.index()) + " / " + String(source.size()));
                          }
                          
                          // Actualizamos la referencia de cambio de pista 
                          currentIdx = source.index();  
                          //
                          fileSize = getStreamfileSize(source.toStr());
                          updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());
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
                  disable_auto_wav_stop = false;
                  PROGRESS_BAR_TOTAL_VALUE = 0;
                  PROGRESS_BAR_BLOCK_VALUE = 0;
              }
              
              // Avance de pistas
              if (FFWIND || RWIND)
              {
                  if (FFWIND)
                  {
                      // if (pressedCountFFWD > 2)
                      // {
                      //     pressedCountFFWD = 0;
                      //     FFWIND = false;
                      //     RWIND = false;
                      //     offset = 0;
                      // }
                      // else
                      // {
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
                  bitRateRead = 0;
                  updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

                  FFWIND = false;
                  RWIND = false;
                  fileread = 0;                    
              }
              // else if (KEEP_FFWIND)
              // {
                
              //   offset += bufferSize;
              //   pressedCountFFWD++;
              //   delay(25);

              // }
              // else if (KEEP_RWIND)
              // {
                
              //   offset += bufferSize;
              //   offset *= -1;

              //   if ((last_fileread + offset) < 0)
              //   {
              //     last_fileread = 0;
              //     offset = 1;
              //   }

              //   delay(25);
              // }
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
                  disable_auto_wav_stop = false;                    
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
                  updateIndicators(totalFilesIdx, currentIdx+1, fileSize, bitRateRead, source.toStr());

                  // Lo ponemos asi para que empiece en CONTINUOS PLAYING
                  // ya que en el if se hace un cambio dependiendo del valor anterior
                  disable_auto_wav_stop = false;  
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
              disable_auto_wav_stop = false; 

              PLAY=false;         
              PROGRESS_BAR_TOTAL_VALUE = 0;
              PROGRESS_BAR_BLOCK_VALUE = 0;

              break;

        default:
          break;
        }

        // Actualizamos el indicador
        if ((millis() - startTime) > 4000)
        {
          updateIndicators(totalFilesIdx, source.index()+1, fileSize, bitRateRead, source.toStr());
          startTime = millis();
        }       
        
      }
      
      tapeAnimationOFF(); 
      player.end();

      moveDirection = 1;
      posRotateName = 0;        
  }
  else
  {
    logln("Error player initialization");
    STOP=true;
  } 

  // Recupero la SD para el resto de procesadores.
  if (!sdf.begin(sdcfg)) 
  {
    logln("Error recovering spi initialization");
  }  

  rotate_enable = false;  
}

void playingFile()
{
  rotate_enable = true;

  if (TYPE_FILE_LOAD == "TAP")
  {

      setAudioOutput();
      ESP32kit.setVolume(MAX_MAIN_VOL);

      sendStatus(REC_ST, 0);

      //hmi.getMemFree();
      pTAP.play();
      //Paramos la animación
      tapeAnimationOFF();  
      //pTAP.updateMemIndicator();
  }
  else if (TYPE_FILE_LOAD == "TZX" || TYPE_FILE_LOAD == "CDT" || TYPE_FILE_LOAD == "TSX")
  {

      setAudioOutput();
      ESP32kit.setVolume(MAX_MAIN_VOL);

      sendStatus(REC_ST, 0);

      //hmi.getMemFree();
      pTZX.play();
      //Paramos la animación
      tapeAnimationOFF(); 
  }
  else if (TYPE_FILE_LOAD == "WAV")
  {
    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    playWAV();
    logln("Finish WAV playing file");
  }
  else if (TYPE_FILE_LOAD == "MP3")
  {
    logln("Type file load: " + TYPE_FILE_LOAD + " and MODEWAV: " + String(MODEWAV));
    // Reproducimos el WAV file
    playMP3();
    logln("Finish MP3 playing file");
  }  
  else
  {
      logAlert("Unknown type_file_load");
  }
 
}

void verifyConfigFileForSelection()
{
  // Vamos a localizar el fichero de configuracion especifico para el fichero seleccionado
  const int max_params = 4;
  String path = FILE_LAST_DIR;
  String tmpPath = PATH_FILE_TO_LOAD;
  tConfig *fileCfg;// = (tConfig*)ps_calloc(max_params,sizeof(tConfig));
  char strpath[tmpPath.length() + 5] = {};
  strcpy(strpath,tmpPath.c_str());
  strcat(strpath,".cfg");

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

    if (cfg.open(strpath,O_READ))
    {
      // Leemos todos los parametros
      //cfg.rewind();
      fileCfg = sdm.readAllParamCfg(cfg,max_params);

      logln("");
      log("cfg open");  

      // =================================================================================
      //
      // Aqui falla
      //
      // =================================================================================
      for (int i=0;i<max_params;i++)
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
      logln("Now ejecting file - Step 3");
      ejectingFile();
      FILE_PREPARED = false;      
    }

    // Cargamos el seleccionado.
    if (PATH_FILE_TO_LOAD.indexOf(".TAP") != -1)
    {
        // Verificamos si hay fichero de configuracion para este archivo seleccionado
        verifyConfigFileForSelection();
        // changeLogo(41);
        // Reservamos memoria
        if (!myTAPmemoryReserved)
        {
          myTAP.descriptor = (tTAPBlockDescriptor*)ps_calloc(MAX_BLOCKS_IN_TAP, sizeof(struct tTAPBlockDescriptor));     
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
        verifyConfigFileForSelection();        // Reservamos memoria
        // 
        if (!myTZXmemoryReserved)
        {
          myTZX.descriptor = (tTZXBlockDescriptor*)ps_calloc(MAX_BLOCKS_IN_TZX , sizeof(struct tTZXBlockDescriptor));      
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
    else if(PATH_FILE_TO_LOAD.indexOf(".WAV") != -1)
    {
      logln("Wav file to load: " + PATH_FILE_TO_LOAD);
      FILE_PREPARED = true;
      TYPE_FILE_LOAD = "WAV";
    }
    else if(PATH_FILE_TO_LOAD.indexOf(".MP3") != -1)
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

    // if (!taprec.createTempTAPfile())
    // {
    //   LAST_MESSAGE = "Recorder not prepared.";
    //   delay(2000);
    //   //
    // }
    // else
    // {
    //writeString("");
    hmi.writeString("currentBlock.val=1");
    //writeString("");
    hmi.writeString("progressTotal.val=0");
    //writeString("");
    hmi.writeString("progression.val=0");
    // Preparamos para recording

    //Activamos la animación
    tapeAnimationON();      
    // }
    
    //hmi.getMemFree();        
}

void RECready()
{
    prepareRecording();
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
      // Para que empiece en DOWN tenemos que poner POLARIZATION en UP
      // Una señal con INVERSION de pulso es POLARIZACION = UP (empieza en DOWN)
      INVERSETRAIN = true;
    }
    else
    {
      INVERSETRAIN = false;
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
        // Inicializa la polarizacion
        EDGE_EAR_IS = POLARIZATION; 
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

void nextGroupBlock()
{
  logln("Seacrh Next Group Block");
  
  if (TYPE_FILE_LOAD !="TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    while(myTZX.descriptor[BLOCK_SELECTED].ID != 34 && BLOCK_SELECTED <= TOTAL_BLOCKS)
    {
      BLOCK_SELECTED++;
    }

    if (BLOCK_SELECTED > (TOTAL_BLOCKS-1))
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

  if (TYPE_FILE_LOAD !="TAP" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
  {
    while(myTZX.descriptor[BLOCK_SELECTED].ID != 33 && BLOCK_SELECTED > 1)
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
    if (TYPE_FILE_LOAD=="TAP")
    {
      hmi.setBasicFileInformation(0,0,myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size,true);
    }
    else if(TYPE_FILE_LOAD=="TZX" || TYPE_FILE_LOAD=="CDT" || TYPE_FILE_LOAD=="TSX")
    {
      hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,myTZX.descriptor[BLOCK_SELECTED].group,myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size,myTZX.descriptor[BLOCK_SELECTED].playeable);
    } 

    hmi.updateInformationMainPage(true);
}

void getRandomFilename (char* &currentPath, String currentFileBaseName)
{
      currentPath = strcpy(currentPath, currentFileBaseName.c_str());
      srand(time(0));
      delay(125);
      int rn = rand()%999999;
      //Le unimos la extensión .TAP
      String txtRn = "-" + String(rn) + ".wav";
      char const *extPath = txtRn.c_str();
      strcat(currentPath,extPath);  
}

void rewindAnimation(int direction)
{
    int p=0;
    int frames = 19;
    int fdelay = 5;

    while (p < frames)
    {
      
      POS_ROTATE_CASSETTE+=direction;

      if (POS_ROTATE_CASSETTE > 23)
      { POS_ROTATE_CASSETTE = 4;}

      if (POS_ROTATE_CASSETTE < 4)
      { POS_ROTATE_CASSETTE = 23;}

      hmi.writeString("tape.animation.pic=" + String(POS_ROTATE_CASSETTE));
      delay(20);

      p++;
    }  
}

void getTheFirstPlayeableBlock()
{
  // Buscamos ahora el primer bloque playeable
  
  if (TYPE_FILE_LOAD !="TAP")
  {
      int i=1;
      while(!myTZX.descriptor[i].playeable)
      {
        BLOCK_SELECTED=i;
        i++;
      }

      if (i > MAX_BLOCKS_IN_TZX)
      {
        i=1;
      }

      BLOCK_SELECTED=i;
      logln("Primero playeable: " + String(i));

      // PROGRAM_NAME = myTZX.descriptor[i].name;
      strcpy(LAST_TYPE,myTZX.descriptor[i].typeName);
      LAST_SIZE = myTZX.descriptor[i].size;

      hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].ID,myTZX.descriptor[BLOCK_SELECTED].group,myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size,myTZX.descriptor[BLOCK_SELECTED].playeable);
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
        hmi.setBasicFileInformation(0,0,myTAP.descriptor[BLOCK_SELECTED].name,
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
      BLOCK_SELECTED = (TOTAL_BLOCKS-1);
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
        hmi.setBasicFileInformation(0,0,myTAP.descriptor[BLOCK_SELECTED].name,
                                        myTAP.descriptor[BLOCK_SELECTED].typeName,
                                        myTAP.descriptor[BLOCK_SELECTED].size,
                                        myTAP.descriptor[BLOCK_SELECTED].playeable);
    }

    hmi.updateInformationMainPage(true);   

    rewindAnimation(-1);

}

void openBlocksBrowser()
{
    // Rellenamos el browser con todos los bloques

    int max = MAX_BLOCKS_IN_BROWSER;
    int totalPages = 0;

    if (TOTAL_BLOCKS > max)
    {
      max = MAX_BLOCKS_IN_BROWSER;
    }
    else
    {
      max = TOTAL_BLOCKS - 1;
    }

    BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
    
    hmi.writeString("blocks.path.txt=\"" + HMI_FNAME + "\"");
    hmi.writeString("blocks.totalBl.txt=\"" + String(TOTAL_BLOCKS-1) + "\"");
    hmi.writeString("blocks.bbpag.txt=\"" + String(BB_PAGE_SELECTED) + "\"");

    totalPages = ((TOTAL_BLOCKS) / MAX_BLOCKS_IN_BROWSER);
    if ((FILE_TOTAL_FILES) % MAX_BLOCKS_IN_BROWSER != 0)
    {
        totalPages+=1;
    }
    hmi.writeString("blocks.totalPag.txt=\"" + String(totalPages) + "\"");

    for(int i=1;i<=max;i++)
    {
      if (i+BB_PTR_ITEM > TOTAL_BLOCKS-1)
      {
        // Los dejamos limpios pero sin informacion
        hmi.writeString("blocks.id" + String(i) + ".txt=\"\"");
        hmi.writeString("blocks.data" + String(i) + ".txt=\"\"");
        hmi.writeString("blocks.size" + String(i) + ".txt=\"\"");
        hmi.writeString("blocks.name" + String(i) + ".txt=\"\"");
      }
      else
      {
          // En otro caso metemos informacion
          hmi.writeString("blocks.id" + String(i) + ".txt=\"" + String(i + BB_PTR_ITEM) + "\"");

          if (TYPE_FILE_LOAD != "TAP")
          {       

                if (String(myTZX.descriptor[i + BB_PTR_ITEM].typeName).indexOf("ID 21") != -1)
                {
                    hmi.writeString("blocks.id" + String(i) + ".pco=2016");
                    hmi.writeString("blocks.data" + String(i) + ".pco=2016");
                    hmi.writeString("blocks.size" + String(i) + ".pco=2016");
                    hmi.writeString("blocks.name" + String(i) + ".pco=2016");
                }
                else if (String(myTZX.descriptor[i + BB_PTR_ITEM].typeName).indexOf("ID 20") != -1)
                {
                    hmi.writeString("blocks.id" + String(i) + ".pco=64512");
                    hmi.writeString("blocks.data" + String(i) + ".pco=64512");
                    hmi.writeString("blocks.size" + String(i) + ".pco=64512");
                    hmi.writeString("blocks.name" + String(i) + ".pco=64512");
                }                
                else
                {
                    hmi.writeString("blocks.id" + String(i) + ".pco=57051");
                    hmi.writeString("blocks.data" + String(i) + ".pco=57051");
                    hmi.writeString("blocks.size" + String(i) + ".pco=57051");
                    hmi.writeString("blocks.name" + String(i) + ".pco=57051");
                }

                int tzxSize = myTZX.descriptor[i + BB_PTR_ITEM].size;
                hmi.writeString("blocks.data" + String(i) + ".txt=\"" + myTZX.descriptor[i + BB_PTR_ITEM].typeName + "\"");
                hmi.writeString("blocks.name" + String(i) + ".txt=\"" + myTZX.descriptor[i + BB_PTR_ITEM].name + "\"");
                hmi.writeString("blocks.size" + String(i) + ".txt=\"" + String(tzxSize) + "\"");
          }
          else
          {
                int tapSize = myTAP.descriptor[i + BB_PTR_ITEM - 1].size;
                hmi.writeString("blocks.data" + String(i) + ".txt=\"" + myTAP.descriptor[i + BB_PTR_ITEM - 1].typeName + "\"");
                hmi.writeString("blocks.name" + String(i) + ".txt=\"" + myTAP.descriptor[i + BB_PTR_ITEM - 1].name + "\"");
                hmi.writeString("blocks.size" + String(i) + ".txt=\"" + String(tapSize) + "\"");
          }        
      }
    }  
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
        else if(SAMPLINGTEST)
        {
          logln("Testing is output");
          zxp.samplingtest(14000);

          SAMPLINGTEST = false;
          
        }
        else if (REC)
        {
          LAST_MESSAGE = "Press PAUSE to start recording.";
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

      // Esto nos permite abrir el Block Browser en reproduccion
      if (BB_OPEN || BB_UPDATE)
      {
          // Ojo! no meter delay! que esta en una reproduccion
          openBlocksBrowser();
          openBlocksBrowser();

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
          //            
          playingFile();
      }
      else if(EJECT)
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
        STOP=false; //28/11

        if (TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "MP3")
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
          LAST_MESSAGE = "Press PAUSE to start recording.";
          tapeAnimationOFF();
          recAnimationON();

          if (MODEWAV)
          {
              TAPESTATE = 120;
          }
          else
          {
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
          openBlocksBrowser();
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
              hmi.setBasicFileInformation(0,0,myTAP.descriptor[BLOCK_SELECTED].name,
                                              myTAP.descriptor[BLOCK_SELECTED].typeName,
                                              myTAP.descriptor[BLOCK_SELECTED].size,
                                              myTAP.descriptor[BLOCK_SELECTED].playeable);
          }

          hmi.updateInformationMainPage(true);   
          UPDATE = false;     
      }
      else if(EJECT)
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
          openBlocksBrowser();
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
                hmi.setBasicFileInformation(0,0,myTAP.descriptor[BLOCK_SELECTED].name,
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
      if (PAUSE)
      {
        
        //Generamos un numero aleatorio para el final del nombre
        char* cPath = (char*)ps_calloc(55,sizeof(char));
        String wavfileBaseName = "/WAV/rec";

        getRandomFilename(cPath, wavfileBaseName);

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
      if(STOP || taprec.actuateAutoRECStop || !REC || EJECT || RECORDING_ERROR!=0 || PAUSE)
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
        
        //recordingFile();
        while (!taprec.recording())
        {
          delay(1);
        }

        // Ha acabado con errores
        LAST_MESSAGE = "Stop recording";
        REC = false;
        STOP = true;

        logln("Acaba el recorder");
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

  // const int windowNameLength = 44;

  #ifndef SAMPLINGTEST
    // Core 0 - Para el HMI
    int startTime = millis();
    int startTime2 = millis();
    int startTime3 = millis();
    int tClock = millis();
    int ho=0;int mi=0;int se=0;
    int tScrRfsh = 125;
    // int tRotateNameRfsh = 200;

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

    // ----------------------------------------------------------------------
    // ES IMPORTANTE NO ELIMINAR ESTA LINEA PORQUE HABILITA EL SLOT de la SD
    // Configuracion de las librerias del AudioKit
    hmi.writeString("statusLCD.txt=\"Setting audio\"");
    setAudioOutput();
    // 
    // ----------------------------------------------------------------------

    
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

          // Webserver 
          // ---------------------------------------------------
          // configureWebServer();
          // server.begin(); 
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

    // Por defecto
    // -------------------------------------------------------------------------
    // Enable block end
    // hmi.writeString("menuAudio2.enTerm.val=");
    // APPLY_END = true;

    // 48KHz
    hmi.writeString("menuAudio2.r0.val=0");
    // 44KHz
    hmi.writeString("menuAudio2.r1.val=0");
    // 32KHz
    hmi.writeString("menuAudio2.r2.val=0");
    // 22KHz
    hmi.writeString("menuAudio2.r3.val=1");
    SAMPLING_RATE = 22050;
    hmi.writeString("tape.lblFreq.txt=\"22KHz\"" );
    hmi.refreshPulseIcons(INVERSETRAIN,ZEROLEVEL); 
    // -------------------------------------------------------------------------

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