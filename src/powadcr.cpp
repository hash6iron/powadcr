/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: powadcr.ino
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023 [@hash6iron]
      https://github.com/hash6iron/powadcr
      https://powagames.itch.io/

    Agradecimientos:
      - Kike Martín [kyv]
      - Jose [Xosmar]
      - Mario J
      - Isra
      - Fernando Mosquera
      - Victor [Eremus]
      - Juan Antonio Rubio
      - Guillermo
      - Pedro Pelaez
      - Carlos Palmero [shaeon]
      - Carlos Martinez [TxarlyM69]
    
    
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

#include <Arduino.h>
#include <iostream>

#include "config.h"

#include "esp32-hal-psram.h"
#include "EasyNextionLibrary.h"

// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
TaskHandle_t Task0;
TaskHandle_t Task1;

#define SerialHWDataBits 921600

#include <esp_task_wdt.h>
#define WDT_TIMEOUT 360000  

#include <HardwareSerial.h>
HardwareSerial SerialHW(0);

EasyNex myNex(SerialHW);

#include "SdFat.h"
#include "globales.h"

// Para WAV recording
#include "AudioKitHAL.h"
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
TAPprocessor::tTAP myTAP;
// Procesador de TZX
TZXprocessor::tTZX myTZX;

bool last_headPhoneDetection = false;
uint8_t tapeState = 0;

bool taskStop = true;
bool pageScreenIsShown = false;

#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
  uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
#endif

// Elegant OTA
// -----------------------------------------------------------------------
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

WebServer server(80);

#include <ESPAsyncWebServer.h>
AsyncWebServer fserver(81);

#include <ElegantOTA.h>
// -----------------------------------------------------------------------

#include "powaFileServer.h"
powaFileServer fileServer(&fserver,sdf);

#include "AudioTools.h"
#include "AudioLibs/AudioKit.h"

AudioKitStream kit;
StreamCopy copier(kit, kit);  // copies data

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
void loadWifiCfgFile()
{
   
  if(sdf.exists("/wifi.cfg"))
  {
    logln("File wifi.cfg exists");

    File32 fWifi = sdm.openFile32("/wifi.cfg"); 
    int* IP;

    if(fWifi.isOpen())
    {
        HOSTNAME = new char[32];
        ssid = new char[64];
        password = new char[64];

        char* ip1 = new char[17];

        CFGWIFI = sdm.readAllParamCfg(fWifi,9);

        // WiFi settings
        // Hostname
        strcpy(HOSTNAME, (sdm.getValueOfParam(CFGWIFI[0].cfgLine,"hostname")).c_str());
        log(HOSTNAME);
        // SSID - Wifi
        strcpy(ssid, (sdm.getValueOfParam(CFGWIFI[1].cfgLine,"ssid")).c_str());
        log(ssid);
        // Password - WiFi
        strcpy(password, (sdm.getValueOfParam(CFGWIFI[2].cfgLine,"password")).c_str());
        log(password);

        //Local IP
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[3].cfgLine,"IP")).c_str());
        logln("ip1: " + String(ip1));
        IP = strToIPAddress(String(ip1));
        local_IP = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // Subnet
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[4].cfgLine,"SN")).c_str());
        IP = strToIPAddress(String(ip1));
        subnet = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // gateway
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[5].cfgLine,"GW")).c_str());
        IP = strToIPAddress(String(ip1));
        gateway = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // DNS1
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[6].cfgLine,"DNS1")).c_str());
        IP = strToIPAddress(String(ip1));
        primaryDNS = IPAddress(IP[0],IP[1],IP[2],IP[3]);
        
        // DNS2
        strcpy(ip1, (sdm.getValueOfParam(CFGWIFI[7].cfgLine,"DNS2")).c_str());
        IP = strToIPAddress(String(ip1));
        secondaryDNS = IPAddress(IP[0],IP[1],IP[2],IP[3]);

        logln("Open config. WiFi-success");
        fWifi.close();
    }
  }
  else
  {
    // Si no existe lo creo.
    File32 fWifi;
    fWifi.open("/wifi.cfg", O_WRITE | O_CREAT);

    if (fWifi.isOpen())
    {

      fWifi.println("<hostname>powaDCR</hostname>");
      fWifi.println("<ssid></ssid>");
      fWifi.println("<password></password>");
      fWifi.println("<IP>192.168.1.10</IP>");
      fWifi.println("<SN>255.255.255.0</SN>");
      fWifi.println("<GW>192.168.1.1</GW>");
      fWifi.println("<DNS1>192.168.1.1</DNS1>");
      fWifi.println("<DNS2>192.168.1.1</DNS1>");

      logln("wifi.cfg new file created");

      fWifi.close();
    }
  }


  
}
void loadHMICfgFile()
{
  File32 fHMI = sdm.openFile32("/hmi.cfg");
  
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

        if (!FILE_CORRUPTED)
        {
          LAST_MESSAGE = "Press PLAY to enjoy!";
          //
          FILE_PREPARED = true;
        }
        else
        {
          LAST_MESSAGE = "ERROR! Selected file is CORRUPTED.";
          //
          FILE_PREPARED = false;
        }

        FILE_NOTIFIED = true;

        // Actualizamos el indicador de memoria consumida para TAPprocessor.
        //pTAP.updateMemIndicator();      
    } 
    else
    {
          LAST_MESSAGE = "Error. PLAY in progress. Try select file again.";
          //
          FILE_PREPARED = false;
          FILE_NOTIFIED = false;
    }  
}
void proccesingTZX(char* file_ch)
{
    pTZX.initialize();

    pTZX.getInfoFileTZX(file_ch);

    if (ABORT)
    {
      LAST_MESSAGE = "Aborting proccess.";
      //
      FILE_PREPARED = false;      
      ABORT=false;
    }
    else
    {
      LAST_MESSAGE = "Press PLAY to enjoy!";
      //
      FILE_PREPARED = true;
    }

    FILE_NOTIFIED = true;  

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
void setSDFrequency(int SD_Speed) 
{
  bool SD_ok = false;
  bool lastStatus = false;
  while (!SD_ok) 
  {
    if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed)) || lastStatus) 
    {
      
      sdFile32.close();

      //SerialHW.println("");
      //SerialHW.println("");
      //SerialHW.println("SD card error!");

      //SerialHW.println("");
      //SerialHW.println("");
      //SerialHW.println("SD downgrade at " + String(SD_Speed) + "MHz");

      hmi.writeString("statusLCD.txt=\"SD FREQ. AT " + String(SD_Speed) + " MHz\"" );

      SD_Speed = SD_Speed - 1;

      if (SD_Speed < 4) 
      {
          if (SD_Speed < 2)
          {
            // Very low speed
            SD_Speed = 1;
            lastStatus = true;
          }
          else
          {
              // Error fatal
              // La SD no es accesible. Entonces no es problema de la frecuencia de acceso.
              //SerialHW.println("");
              //SerialHW.println("");
              //SerialHW.println("Fatal error. SD card not compatible. Change SD");      
              
              // loop infinito
              while(true)
              {
                hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"" );
                delay(1500);
                hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"" );
                delay(1500);
              }
          } 
      }
    }
    else 
    {
        // La SD ha aceptado la frecuencia de acceso
        //SerialHW.println("");
        //SerialHW.println("");
        //SerialHW.println("SD card initialized at " + String(SD_Speed) + " MHz");

        hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"" );

        // Probamos a listar un directorio
        if (!sdm.dir.open("/"))
        {
            sdm.dir.close();
            SD_ok = false;
            lastStatus = false;

            //SerialHW.println("");
            //SerialHW.println("");
            //SerialHW.println("Listing files. Error!");

            // loop infinito
            while(true)
            {
              hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"" );
              delay(1500);
              hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"" );
              delay(1500);
            }
        }
        else
        {
            sdm.dir.close();

            // Probamos a crear un fichero.
            if (!sdFile32.open("_test", O_WRITE | O_CREAT | O_TRUNC)) 
            {
                SD_ok = false;
                lastStatus = false;

                //SerialHW.println("");
                //SerialHW.println("");
                //SerialHW.println("File creation. Error!");

                // loop infinito
                while(true)
                {
                  hmi.writeString("statusLCD.txt=\"SD CORRUPTED\"" );
                  delay(1500);
                  hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"" );
                  delay(1500);
                }
            }
            else
            {
              sdFile32.close();
              SD_ok = true;
              lastStatus = true;
            }
        }
    }
  }

  delay(1250);
}
void test() 
{
  // Bloque de pruebas
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

      //SerialHW.println("");
      //SerialHW.println("LCD READY");
      //SerialHW.println("");

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
          hmi.writeString("tape.g0.txt=\"SAMPLING AT 48 KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 48 KHz\"" );
          delay(750);
        }    
        break;

      case 44100:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.g0.txt=\"SAMPLING AT 44.1 KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 44.1 KHz\"" );          
          delay(750);
        }            
        break;

      case 32000:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_32K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.g0.txt=\"SAMPLING AT 32 KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 32 KHz\"" );          
          delay(750);
        }            
        break;

      default:
        if(!ESP32kit.setSampleRate(AUDIO_HAL_22K_SAMPLES))
        {
          //log("Error in Audiokit sampling rate setting");
        }
        else
        {
          hmi.writeString("tape.g0.txt=\"SAMPLING AT 22.05 KHz\"" );
          hmi.writeString("statusLCD.txt=\"SAMPLING AT 22.05 KHz\"" );          
          delay(750);
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

void setWavRecording(char* file_name)
{
    // AudioLogger::instance().begin(Serial, AudioLogger::Error);

    // // Cleanup if necessary
    // if (sdf.exists(file_name))
    // {
    //     sdf.remove(file_name);
    // }  

    // // open file for recording WAV
    // wavfile = sdf.open(file_name, O_WRITE | O_CREAT);
    // if (!wavfile)
    // {
    //     logln("file failed!");
    //     delay(5000);
    //     tapeState=0;
    //     STOP=true;
    //     REC=false;
    //     return;
    // }

    // // Configure WAVEncoder
    // auto cfg_WAVEncoder = WAVEncoder().defaultConfig();
    // wavInfo.bits_per_sample = 16;
    // wavInfo.sample_rate = 44100;
    // wavInfo.channels = 2;
    // WAVEncoder().begin(wavInfo);

    // // setup input
    // kitCfg = kitStrm.defaultConfig(TX_MODE);
    // kitCfg.driver = AUDIO_CODEC_ES8388_DEFAULT_HANDLE;
    // kitCfg.is_master = true;
    // kitCfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2;
    // kitCfg.bits_per_sample = 16;
    // kitCfg.sample_rate = 44100;
    // kitCfg.channels = 2;
    // kitCfg.sd_active = true;
    // kitCfg.copyFrom(info);

    // kitStrm.begin(kitCfg);
    // logln("Setting i2C");
    // logln("");
    // delay(10000);

    // // Inicializamos la salida del encoder
    // AudioInfo out_info(44100,2,16);
    // out.begin(out_info);
    // // Inicializamos el copier
    // copier.setCheckAvailableForWrite(false);
    // copier.begin(wavfile, kitStrm);  
    AudioLogger::instance().begin(Serial, AudioLogger::Warning);

    auto cfg = kit.defaultConfig(RXTX_MODE);
    cfg.input_device = AUDIO_HAL_ADC_INPUT_LINE2;
    kit.begin(cfg);

}

void pauseRecording()
{
    // Desconectamos la entrada para evitar interferencias
    //setAudioOutput();
    //waitingRecMessageShown = false;
    REC = false;

    LAST_MESSAGE = "Recording paused.";
    //

    //SerialHW.println("");
    //SerialHW.println("REC. Procces paused.");
    //SerialHW.println("");
}
void stopRecording()
{

    // Verificamos cual fue el motivo de la parada
    if (REC)
    {

      // No quitar esto, permite leer la variable totalBlockTransfered
      int tbt = taprec.totalBlockTransfered;
      LAST_MESSAGE = "Total blocks: " + String(tbt);
      delay(2000);

      if (tbt >= 1)
      {

          if (!taprec.errorInDataRecording && !taprec.wasFileNotCreated)
          {
          // La grabación fue parada pero no hubo errores
          // entonces salvamos el fichero sin borrarlo
          
            taprec.terminate(false);
            if (BLOCK_REC_COMPLETED)
            {
                LAST_MESSAGE = "Recording STOP. File succesfully saved.";
            }
            else
            {
                LAST_MESSAGE = "Recording STOP. No file saved";
            }
            delay(1000);
          }
          else
          {

              if (taprec.errorInDataRecording)
              {
                // La grabación fue parada porque hubo errores
                // entonces NO salvamos el fichero, lo borramos
                // Recording STOP
                //
                // switch (RECORDING_ERROR)
                // {
                //   case 1: //corrupted data
                //   LAST_MESSAGE = "Recording STOP. Corrupted data."; 
                //   //      
                //   delay(1000);        
                //   break;

                //   case 2:
                //   // Todos los bytes fueron capturados pero el checksum es incorrecto
                //   LAST_MESSAGE = "Recording STOP. Error in checksum."; 
                //   //      
                //   delay(1000);        
                //   break;

                //   case 3:
                //   LAST_MESSAGE = "Recording STOP. ERROR 3"; 
                //   //      
                //   delay(1000);
                //   break;

                //   case 4:
                //   LAST_MESSAGE = "Recording STOP. Unknow  ERROR"; 
                //   //      
                //   delay(1000);        
                //   break;

                // }

                taprec.terminate(true);

              }
              else if (taprec.wasFileNotCreated)
              {
                  // Si no se crea el fichero no se puede seguir grabando
                  taprec.terminate(false);
                  LAST_MESSAGE = "Error in filesystem or SD.";
                  delay(1000);
              }

          }

      }
      else
      {
          taprec.terminate(true);
          LAST_MESSAGE = "Recording STOP. No file saved";
          //      
          delay(1000);
      }

    }

    // Una vez hemos cerrado la sesión de recording
    // ahora cambiamos el estado de los controles

    STOP = true;
    REC = false;

    // Reiniciamos variables
    taprec.totalBlockTransfered = 0;
    taprec.WasfirstStepInTheRecordingProccess = false;
    // Reiniciamos flags de recording errors
    taprec.errorInDataRecording = false;
    taprec.wasFileNotCreated = true;

    //Paramos la animación del indicador de recording
    hmi.writeString("tape2.tmAnimation.en=0");    
}

// -------------------------------------------------------------------------------------------------------------------
unsigned long ota_progress_millis = 0;

void onOTAStart() 
{
  // Log when OTA has started
  pageScreenIsShown = false;
  // <Add your own code here>
}
void onOTAProgress(size_t current, size_t final) 
{
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

      HTTPUpload &upload = server.upload();
      size_t fileSize = upload.totalSize / 1024;
      fileSize = (round(fileSize*10)) / 10;
      String fileName = upload.filename;

      hmi.writeString("statusLCD.txt=\"FIRMWARE UPDATING " + String(fileSize) + " KB\""); 

      // if(final!=0)
      // {
      //   hmi.writeString("screen.updateBar.val=" + String((current * 100) / final));    
      // }
      
  }
}
void onOTAEnd(bool success) 
{
  // Log when OTA has finished
  if (success) 
  {
    hmi.writeString("statusLCD.txt=\"SUCCEESSFUL UPDATE\"");
    // Cerramos la conexión antes de resetear el ESP32
    server.sendHeader("Connection", "close");
    server.send(200,"text/plain","OK");    
    delay(2000);
    // Reiniciamos  
    ESP.restart();
  } 
  else 
  {
    hmi.writeString("statusLCD.txt=\"UPDATE FAIL\"");
    // Cerramos la conexión
    server.sendHeader("Connection", "close");
    server.send(400,"text/plain","Uploading process fail.");     
  }
  // <Add your own code here>
}
void wifiOTASetup()
{
  bool failed = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
    // Disable Auto Reboot
  ElegantOTA.setAutoReboot(false);
    
  hmi.writeString("statusLCD.txt=\"SSID: [" + String(ssid) + "]\"" );
  delay(1500);
  
  // Configures Static IP Address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
      hmi.writeString("statusLCD.txt=\"WiFi-STA setting failed!\"" );
      failed = true;     
  }

  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    hmi.writeString("statusLCD.txt=\"WiFi Connection failed!\"" );
    failed = true;
  }  

  if (!failed)
  {
      File32 sFile = sdm.openFile32("/html/index.html");

      server.on("/", HTTP_GET,[]()
      {
        server.send(200, "text/plain", "powaDCR OTA Server. Powered by Elegant OTA");
      });


      ElegantOTA.begin(&server);    // Start ElegantOTA
      // ElegantOTA callbacks
      ElegantOTA.onStart(onOTAStart);
      ElegantOTA.onProgress(onOTAProgress);
      ElegantOTA.onEnd(onOTAEnd);

      server.begin();

      hmi.writeString("statusLCD.txt=\"Wifi + OTA - Enabled\"");
      delay(750);

      hmi.writeString("statusLCD.txt=\"IP " + WiFi.localIP().toString() + "\""); 

      // Enviamos información al menu
      hmi.writeString("menu.wifissid.txt=\"" + String(ssid) + + "\"");
      hmi.writeString("menu.wifipass.txt=\"" + String(password) + + "\"");
      hmi.writeString("menu.wifiIP.txt=\"" + WiFi.localIP().toString() + "\"");
      hmi.writeString("menu.wifiEn.val=1");
  }
  else
  {
      hmi.writeString("statusLCD.txt=\"Wifi disabled\"");
      delay(750);
  }

  delay(1500);
}

// -------------------------------------------------------------------------------------------------------------------
void setSTOP()
{
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;

  BLOCK_SELECTED = 0;
  
  hmi.writeString("tape2.tmAnimation.en=0"); 

  if (!taprec.errorInDataRecording && !taprec.wasFileNotCreated)
  {
    LAST_MESSAGE = "Tape stop.";
  }

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
void playingFile()
{
  setAudioOutput();
  ESP32kit.setVolume(MAX_MAIN_VOL);

  sendStatus(REC_ST, 0);

  //Activamos la animación
  hmi.writeString("tape2.tmAnimation.en=1"); 

  // Reproducimos el fichero
  LAST_MESSAGE = "Loading in progress. Please wait.";
  //

  if (TYPE_FILE_LOAD == "TAP")
  {
      //hmi.getMemFree();
      pTAP.play();
      //Paramos la animación
      hmi.writeString("tape2.tmAnimation.en=0"); 
      //pTAP.updateMemIndicator();
  }
  else if (TYPE_FILE_LOAD = "TZX")
  {
      //hmi.getMemFree();
      pTZX.play();
      //Paramos la animación
      hmi.writeString("tape2.tmAnimation.en=0"); 
  }
}
bool loadingFile()
{
  // Cogemos el fichero seleccionado y lo cargamos              
  char* file_ch = (char*)malloc(256 * sizeof(char));
  FILE_TO_LOAD.toCharArray(file_ch, 256);

  // Si no está vacio
  if (FILE_TO_LOAD != "") 
  {
    
    // Limpiamos los campos del TAPE
    hmi.clearInformationFile();

    // Convierto a mayusculas
    FILE_TO_LOAD.toUpperCase();

    if (FILE_TO_LOAD.indexOf(".TAP") != -1)
    {
        // Reservamos memoria
        myTAP.descriptor = (TAPprocessor::tBlockDescriptor*)ps_malloc(MAX_BLOCKS_IN_TAP * sizeof(struct TAPprocessor::tBlockDescriptor));        
        // Pasamos el control a la clase
        pTAP.setTAP(myTAP);
    
        // Lo procesamos
        proccesingTAP(file_ch);
        TYPE_FILE_LOAD = "TAP";
    }
    else if (FILE_TO_LOAD.indexOf(".TZX") != -1)    
    {
        // Reservamos memoria
        myTZX.descriptor = (TZXprocessor::tTZXBlockDescriptor*)ps_malloc(MAX_BLOCKS_IN_TZX * sizeof(struct TZXprocessor::tTZXBlockDescriptor));        
        // Pasamos el control a la clase
        pTZX.setTZX(myTZX);

        // Lo procesamos. Para ZX Spectrum
        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TZX";
    }
    else if (FILE_TO_LOAD.indexOf(".TSX") != -1)
    {
        // PROGRESS_BAR_REFRESH = 256;
        // PROGRESS_BAR_REFRESH_2 = 32;

        // Lo procesamos. Para MSX
        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TSX";             
    }   
  }
  else
  {
    LAST_MESSAGE = "No file was selected.";
    return false;
  } 

  // Liberamos
  free(file_ch); 
  // Controlamos un posible error si el fichero tiene 0 bytes
  // o no se cargó correctamente
  if (TOTAL_BLOCKS!=0)
  {
    return true;
  }
  else
  {
    LAST_MESSAGE = "Error in file.";
    return false;
  }
}
void stopFile()
{
  //Paramos la animación
  setSTOP();     
  hmi.writeString("tape2.tmAnimation.en=0"); 
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
      free(pTAP.getDescriptor());
      // Finalizamos
      pTAP.terminate();
  }
  else if (TYPE_FILE_LOAD = "TZX")
  {
      // Solicitamos el puntero _myTZX de la clase
      // para liberarlo
      free(pTZX.getDescriptor());
      // Finalizamos
      pTZX.terminate();
  }  
}
void prepareRecording()
{
    // Liberamos myTAP.descriptor de la memoria si existe 
    if (myTAP.descriptor!= nullptr) 
    {
      free(myTAP.descriptor);
    }
      
    // Preparamos para recording

    //Activamos la animación
    hmi.writeString("tape2.tmAnimation.en=1"); 

    // Inicializamos audio input
    //setAudioInput();
    setAudioInOut();
    
    taprec.set_kit(ESP32kit);    
    taprec.initialize(); 

    if (!taprec.createTempTAPfile())
    {
      LAST_MESSAGE = "Recorder not prepared.";
      //
    }
    else
    {
      LAST_MESSAGE = "Recorder ready. Play source data.";

      //writeString("");
      hmi.writeString("currentBlock.val=1");
      //writeString("");
      hmi.writeString("progressTotal.val=0");
      //writeString("");
      hmi.writeString("progression.val=0");
    }
    
    //hmi.getMemFree();        
}
void recordingFile()
{
    // Iniciamos la grabación
    if (taprec.recording())
    {
        // Ha finalizado la grabación de un bloque
        // ahora estudiamos la causa
        // if (taprec.stopRecordingProccess)   
        // {
        //     // Hay tres casos
        //     // - El fichero para grabar el TAP no fue creado
        //     // - La grabación fue parada pero fue correcta (no hay errores)
        //     // - La grabación fue parada pero fue incorrecta (hay errores)
        //     //SerialHW.println("");
        //     //SerialHW.println("STOP 2 - REC");

        //     LOADING_STATE = 0;

        //     //stopRecording();
        //     //setSTOP();
        // }
        LOADING_STATE = 0;
    }
    else
    {
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
void tapeControl()
{ 
  #ifdef SAMPLINGTESTACTIVE
    setAudioOutput();
    // Para salir del test hay que reiniciar el powaDCR
    hmi.writeString("statusLCD.txt=\"SAMPLING TEST RUNNING\"" );
    sendStatus(SAMPLINGTEST);
    delay(250);
  #else

    switch (tapeState)
    {
      case 0:
        // Estado inicial
        SCOPE = down;

        if (FILE_BROWSER_OPEN)
        {
          tapeState = 99;
          LOADING_STATE = 0;          
        }
        else if(FILE_SELECTED)
        {
          // FICHERO CARGADO EN TAPE
          if(loadingFile())
          {
            LAST_MESSAGE = "File inside the TAPE.";
            //  

            tapeState = 1;
            LOADING_STATE = 0;          
            FILE_SELECTED = false;
            FFWIND = false;
            RWIND = false;           
          }
          else
          {
            tapeState = 0;
            LOADING_STATE = 0;          
            FILE_SELECTED = false;
            FFWIND = false;
            RWIND = false;           
          }
        }
        else if(PLAY)
        {
          LAST_MESSAGE = "No file was selected.";
          //
          LOADING_STATE = 0;          
          tapeState = 0;       
          FFWIND = false;
          RWIND = false;              
        }    
        else if(REC)
        {
          if (!MODEWAV)
          {
            // Modo .TAP
            FFWIND = false;
            RWIND = false;   
                    
            prepareRecording();
            //log("REC. Waiting for guide tone");
            tapeState = 200;
          }
          else
          {
            // Modo WAV
            LAST_MESSAGE = "Recording to WAV file.";
            setWavRecording("/REC/record.wav");
            tapeState = 110;
          }
        }
        else
        {
          tapeState = 0;
          LOADING_STATE = 0;          
          FILE_SELECTED = false;
          FFWIND = false;
          RWIND = false;           
        }
        break;

      case 1:
        // Esperando que hacer con el fichero cargado
        if (PLAY)
        {
          // Inicializamos la polarización de la señal al iniciar la reproducción.
          LAST_EAR_IS = POLARIZATION; 
          //
          //getAudioSettingFromHMI();

          // Lo reproducimos
          FFWIND = false;
          RWIND = false;        
          LOADING_STATE = 1;          
          playingFile();
          tapeState = 2;
          //SerialHW.println(tapeState);
        }
        else if(EJECT)
        {
          FFWIND = false;
          RWIND = false;
          tapeState = 5;
        }
        else if (FFWIND || RWIND)
        {
          // Actualizamos el HMI
          if (TYPE_FILE_LOAD=="TAP")
          {
            hmi.setBasicFileInformation(myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size);
          }
          else if(TYPE_FILE_LOAD=="TZX")
          {
            hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size);
          }        
          //
          tapeState = 1;
          FFWIND = false;
          RWIND = false;         
        }       
        else if(REC)
        {
          LOADING_STATE = 0;
          FFWIND = false;
          RWIND = false;   
                  
          prepareRecording();
          //log("REC. Waiting for guide tone");
          tapeState = 200;
        }         
        else
        {
          tapeState = 1;
        }
        break;

      case 2:
        // Reproducción en curso.
        if (PAUSE)
        {
          //pauseFile();
          LOADING_STATE = 3;
          tapeState = 3;
          SCOPE = down;
          //log("Estoy en PAUSA.");
        }
        else if(STOP)
        {
          stopFile();
          tapeState = 4;
          LOADING_STATE = 0;     
          SCOPE = down;
          //SerialHW.println(tapeState);
        }
        else if(EJECT)
        {
          // Lo sacamaos del TAPE       
          tapeState = 5;
        }
        else
        {
          tapeState = 2;
        }
        break;

      case 3:
        // FICHERO EN PAUSE
        if (PLAY)
        {
          // Lo reproducimos
          LOADING_STATE = 1; 
          PAUSE = false;         
          FFWIND = false;
          RWIND = false;        

          tapeState = 2;
          playingFile();          
        }
        else if(STOP)
        {
          stopFile();
          LOADING_STATE = 0;          
          tapeState = 4;
          SCOPE = down;
          FFWIND = false;
          RWIND = false;           
        }
        else if(EJECT)
        {      
          tapeState = 5;

          FFWIND = false;
          RWIND = false;   
        }
        else if (FFWIND || RWIND)
        {
          // Actualizamos el HMI
          if (TYPE_FILE_LOAD=="TAP")
          {
            hmi.setBasicFileInformation(myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size);
          }
          else if(TYPE_FILE_LOAD=="TZX")
          {
            hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size);
          }
          //
          tapeState = 3;
          FFWIND = false;
          RWIND = false; 
        }
        else
        {
          tapeState = 3;
        }
        break;
      
      case 4:
        // FICHERO EN STOP
        if (PLAY)
        {
          // Lo reproducimos
          LOADING_STATE = 1;          
          playingFile();          
          tapeState = 2;
        }
        else if(EJECT)
        {
          tapeState = 5;
        }
        else if(REC)
        {
          LOADING_STATE = 0;
          FFWIND = false;
          RWIND = false;   
                  
          prepareRecording();
          tapeState = 200;
        }
        else
        {
          tapeState = 4;
        }
        break;
      
      case 5:     
          // Eject
          stopFile();
          ejectingFile();
          LOADING_STATE = 0;          
          tapeState = 0;
          SCOPE = down;
        break;
      
      case 99:
        if (!FILE_BROWSER_OPEN)
        {
          tapeState = 0;
          LOADING_STATE = 0;          
        }
        else if (REC)
        {
          LOADING_STATE = 0;
          FFWIND = false;
          RWIND = false;  
          FILE_SELECTED = false;                  
          prepareRecording();
          tapeState = 200;
        }
        else
        {
          tapeState = 99;
        }
        break;

      case 200:
        // modo REC
        if(STOP)
        {
          stopRecording();
          taprec.stopRecordingProccess = false;
          //setSTOP();
          tapeState = 0;
          SCOPE = down;
          STOP=true;
          REC=false;
        }
        else
        {
          recordingFile();
          tapeState = 200;
        }
        break;

      case 110:
        // Modo WAV recording
        if (STOP)
        {
            // if (wavfile) 
            // {
                // wavfile.flush();
                // logln("File has ");
                // logln(String(wavfile.size()));
                // log(" bytes");
                // wavfile.close();

                // logln("Recording finish!");
                LAST_MESSAGE = "Recording STOP.";
                tapeState = 0;
                REC = false;
                STOP = true;
            // }        
        }
        else
        {
          copier.copy();  
        }
        break;

      default:
        break;
    }

  #endif

}
bool headPhoneDetection()
{
  return !gpio_get_level((gpio_num_t)HEADPHONE_DETECT);
}


// ******************************************************************
//
//            Gestión de nucleos del procesador
//
// ******************************************************************

void Task1code( void * pvParameters )
{
    //setup();
    for(;;) {
        tapeControl();
        if (serialEventRun) serialEventRun();
        // Deshabilitamos el WDT en cada core
        esp_task_wdt_reset();
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

    for(;;)
    {

        server.handleClient();
        //ElegantOTA.loop();     

        hmi.readUART();

        // Control por botones
        //buttonsControl();
        //delay(50);
        #ifndef DEBUGMODE
          if ((millis() - startTime) > 500)
          {
            startTime = millis();
            stackFreeCore1 = uxTaskGetStackHighWaterMark(Task1);    
            stackFreeCore0 = uxTaskGetStackHighWaterMark(Task0);        
            hmi.updateInformationMainPage();
          }

          if((millis() - startTime2) > 250)
          {
            startTime2 = millis();
            if (SCOPE==up)
            {
              hmi.writeString("add 34,0,6");
              //hmi.writeString("add 34,0,8");
            }
            else
            {
              hmi.writeString("add 34,0,2");
              //hmi.writeString("add 34,0,2");
            }     
          }

          // if ((millis() - tClock) > 1000)
          // {
          //   tClock = millis();     
          //   se++;
          //   if (se>59)
          //   {
          //     se=0;
          //   }
          //   LAST_MESSAGE = String(se) + " s";
          //   hmi.updateInformationMainPage();
          // }

        #endif
        esp_task_wdt_reset();
    }
  #endif
}

void setup() 
{
    //rtc_wdt_protect_off();    // Turns off the automatic wdt service
    // rtc_wdt_enable();         // Turn it on manually
    // rtc_wdt_set_time(RTC_WDT_STAGE0, 20000);  // Define how long you desire to let dog wait.

    // Configuramos el nivel de log
    SerialHW.setRxBufferSize(2048);
    SerialHW.begin(921600);
    delay(250);

    //myNex.begin(921600);

    // Forzamos un reinicio de la pantalla
    hmi.writeString("rest");
    delay(250);

    // Indicamos que estamos haciendo reset
    sendStatus(RESET, 1);
    delay(750);

    hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"" );   
    delay(1250);


    //SerialHW.println("Setting Audiokit.");

    // Configuramos los pulsadores
    configureButtons();

    // Configuramos el ESP32kit
    LOGLEVEL_AUDIOKIT = AudioKitError;

    // Configuracion de las librerias del AudioKit
    setAudioOutput();

    //SerialHW.println("Done!");

    //SerialHW.println("Initializing SD SLOT.");
    
    // Configuramos acceso a la SD
    hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"" );
    delay(750);

    int SD_Speed = SD_FRQ_MHZ_INITIAL;  // Velocidad en MHz (config.h)
    setSDFrequency(SD_Speed);

    // Forzamos a 26MHz
    //sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(26));

    // Le pasamos al HMI el gestor de SD
    hmi.set_sdf(sdf);

    //SerialHW.println("Done!");

    // Esperamos finalmente a la pantalla
    //SerialHW.println("");
    //SerialHW.println("Waiting for LCD.");
    //SerialHW.println("");

    if(psramInit()){
      //SerialHW.println("\nPSRAM is correctly initialized");
      hmi.writeString("statusLCD.txt=\"PSRAM OK\"" );

    }else{
      //SerialHW.println("PSRAM not available");
      hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"" );
    }    
    delay(750);

    // ------------------------------------------------------
    //
    //
    // **********************************
    loadWifiCfgFile();
    wifiOTASetup();
    //
    //
    // ------------------------------------------------------

    delay(750);

    hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"" );
    waitForHMI(CFG_FORZE_SINC_HMI);



    // Inicializa volumen en HMI
    hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volR.val=" + String(MAIN_VOL_R));
    hmi.writeString("menuAudio.volLevelL.val=" + String(MAIN_VOL_L));
    hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_R));

    //
    pTAP.set_HMI(hmi);
    pTZX.set_HMI(hmi);

    pTAP.set_SDM(sdm);
    pTZX.set_SDM(sdm);

    zxp.set_ESP32kit(ESP32kit);
    //pTZX.setZXP_audio(ESP32kit);

    // Si es test está activo. Lo lanzamos
    #ifdef TEST
      TEST_RUNNING = true;
      hmi.writeString("statusLCD.txt=\"TEST RUNNING\"" );
      test();
      hmi.writeString("statusLCD.txt=\"PRESS SCREEN\"" );
      TEST_RUNNING = false;
    #endif

    // Interrupciones HW
    // Timer0_Cfg = timerBegin(0, 80, true);
    // timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
    // timerAlarmWrite(Timer0_Cfg, 1000000, true);
    // timerAlarmEnable(Timer0_Cfg);
    LOADING_STATE = 0;
    BLOCK_SELECTED = 0;
    FILE_SELECTED = false;

    // Inicialmente el POWADCR está en STOP
    STOP = true;
    PLAY = false;
    PAUSE = false;
    REC = false;

    // sendStatus(STOP_ST, 1);
    // sendStatus(PLAY_ST, 0);
    // sendStatus(PAUSE_ST, 0);
    // sendStatus(READY_ST, 1);
    // sendStatus(END_ST, 0);
    
    sendStatus(REC_ST);


    LAST_MESSAGE = "Press EJECT to select a file.";
    
    esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
    // Control del tape
    xTaskCreatePinnedToCore(Task1code, "TaskCORE1", 16834, NULL, 3|portPRIVILEGE_BIT, &Task1, 0);
    esp_task_wdt_add(&Task1);  
    delay(500);
    
    // Control de la UART - HMI
    xTaskCreatePinnedToCore(Task0code, "TaskCORE0", 4096, NULL, 3|portPRIVILEGE_BIT, &Task0, 1);
    esp_task_wdt_add(&Task0);  
    delay(500);

    // Inicializamos el modulo de recording
    taprec.set_HMI(hmi);
    taprec.set_SdFat32(sdf);
    
    //hmi.getMemFree();
    taskStop = false;

    // Ponemos el color del scope en amarillo
    hmi.writeString("tape.scope.pco0=60868");
}

void loop()
{}