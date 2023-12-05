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
    
    
    Descripción:
    Programa principal del firmware del powadcr - Powa Digital Cassette Recording

    Version: 1.0

    Historico de versiones
    v.0.1 - Version de pruebas. En desarrollo
    v.0.2 - New update to develop branch
    

    Configuración en ARDUINO IDE
    ----------------------------
    1. Seleccionar la placa ESP32 Dev Module 
    2. Acceder al menú "Herramientas" --> 
      - Configuración del ESP32 por defecto 
      - PSRAM = Enabled

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
#include <Arduino.h>
#include "esp32-hal-psram.h"
// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
TaskHandle_t Task0;
TaskHandle_t Task1;

#define SerialHWDataBits 921600

#include <esp_task_wdt.h>

#include <HardwareSerial.h>
HardwareSerial SerialHW(0);

// #include "EasyNextionLibrary.h"
// EasyNex myNex(SerialHW);

#include "config.h"

#include "SdFat.h"
#include "globales.h"

//
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
ZXProccesor zxp;

// Procesadores de cinta
#include "TZXprocessor.h"
#include "TAPprocessor.h"

#include "test.h"

// Declaraciones para SdFat
SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;

// Creamos los distintos objetos
TZXproccesor pTZX(ESP32kit);
TAPproccesor pTAP(ESP32kit);

// Procesador de audio input
#include "TAPrecorder.h"
TAPrecorder taprec;

// Procesador de TAP
TAPproccesor::tTAP myTAP;

bool last_headPhoneDetection = false;
uint8_t tapeState = 0;

void proccesingTAP(char* file_ch)
{
    // Liberamos myTAP.descriptor de la memoria si existe 
    if (myTAP.descriptor!= NULL) 
    {
      free(myTAP.descriptor);
    }
    
    //pTAP.set_SdFat32(sdf);
    pTAP.initialize();
    
    // Comprobamos que se ha liberado, anteriormente
    if (myTAP.descriptor != nullptr)
    {
      free(myTAP.descriptor);
    }
    
    if (!PLAY)
    {
        // Reservamos memoria para el objeto
        myTAP.descriptor = (TAPproccesor::tBlockDescriptor*)ps_malloc(MAX_BLOCKS_IN_TAP * sizeof(struct TAPproccesor::tBlockDescriptor));
        
        // Pasamos el control a la clase
        pTAP.setTAP(myTAP);
        pTAP.getInfoFileTAP(file_ch);

        if (!FILE_CORRUPTED)
        {
          LAST_MESSAGE = "Press PLAY to enjoy!";
          hmi.updateInformationMainPage();
          FILE_PREPARED = true;
        }
        else
        {
          LAST_MESSAGE = "ERROR! Selected file is CORRUPTED.";
          hmi.updateInformationMainPage();
          FILE_PREPARED = false;
        }

        FILE_NOTIFIED = true;

        // Actualizamos el indicador de memoria consumida para TAPprocessor.
        pTAP.updateMemIndicator();      
    } 
    else
    {
          LAST_MESSAGE = "Error. PLAY in progress. Try select file again.";
          hmi.updateInformationMainPage();
          FILE_PREPARED = false;
          FILE_NOTIFIED = false;
    }  
}

void proccesingTZX(char* file_ch)
{
    pTZX.getInfoFileTZX(file_ch);

    if (ABORT)
    {
      LAST_MESSAGE = "Aborting proccess.";
      hmi.updateInformationMainPage();
      FILE_PREPARED = false;      
      ABORT=false;
    }
    else
    {
      LAST_MESSAGE = "Press PLAY to enjoy!";
      hmi.updateInformationMainPage();
      FILE_PREPARED = true;
    }

    FILE_NOTIFIED = true;  

}

void sendStatus(int action, int value) {

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
      hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");
      // Enviamos la version del firmware del powaDCR
      hmi.writeString("menu.verFirmware.txt=\" PowaDCR " + String(VERSION) + "\"");

      break;
    
    case RESET:
      //hmi.writeString("");
      hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
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

      SerialHW.println("");
      SerialHW.println("");
      SerialHW.println("SD card error!");

      SerialHW.println("");
      SerialHW.println("");
      SerialHW.println("SD downgrade at " + String(SD_Speed) + "MHz");

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
              SerialHW.println("");
              SerialHW.println("");
              SerialHW.println("Fatal error. SD card not compatible. Change SD");      
              
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
        SerialHW.println("");
        SerialHW.println("");
        SerialHW.println("SD card initialized at " + String(SD_Speed) + " MHz");

        hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"" );

        // Probamos a listar un directorio
        if (!sdm.dir.open("/"))
        {
            sdm.dir.close();
            SD_ok = false;
            lastStatus = false;

            SerialHW.println("");
            SerialHW.println("");
            SerialHW.println("Listing files. Error!");

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

                SerialHW.println("");
                SerialHW.println("");
                SerialHW.println("File creation. Error!");

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
    // Si queremos activar el test que hay en memoria, para chequear con el ordenador
    #ifdef MACHINE_ZX
      //ZX Spectrum
      SerialHW.println();
      SerialHW.println();
      SerialHW.println("----- TEST ACTIVE ------");

      zxp.playBlock(testHeader, 19, testData, 154, DPILOT_HEADER,DPILOT_DATA);
      zxp.playBlock(testScreenHeader, 19, testScreenData, 6914, DPILOT_HEADER,DPILOT_DATA);

      SerialHW.println();
      SerialHW.println();
      SerialHW.println("------ END TEST -------");
    #endif
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

      SerialHW.println("");
      SerialHW.println("LCD READY");
      SerialHW.println("");

      sendStatus(ACK_LCD, 1);  
    }
}


void setAudioOutput()
{
  auto cfg = ESP32kit.defaultConfig(KitOutput);

  //SerialHW.println("Initialized Audiokit. Output - Playing");

  ESP32kit.begin(cfg);
  ESP32kit.setVolume(MAIN_VOL);
  //ESP32kit.setVolume(MAX_MAIN_VOL);   
  
}

void setAudioInput()
{
  auto cfg = ESP32kit.defaultConfig(KitInput);

  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // microphone?
  cfg.sample_rate = AUDIO_HAL_48K_SAMPLES;

  ESP32kit.begin(cfg);
}

void setAudioInOut()
{
  auto cfg = ESP32kit.defaultConfig(KitInputOutput);
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // microphone
  cfg.sample_rate = AUDIO_HAL_48K_SAMPLES;
  ESP32kit.begin(cfg);
  ESP32kit.setVolume(MAIN_VOL);  
}

void pauseRecording()
{
    // Desconectamos la entrada para evitar interferencias
    setAudioOutput();
    waitingRecMessageShown = false;
    REC = false;

    LAST_MESSAGE = "Recording paused.";
    hmi.updateInformationMainPage();

    SerialHW.println("");
    SerialHW.println("REC. Procces paused.");
    SerialHW.println("");
}

void stopRecording()
{
    //Paramos la animación
    hmi.writeString("tape.tmAnimation.en=0");    
    
    //Configuramos ya en modo Output.
    setAudioOutput();

    // Verificamos cual fue el motivo de la parada
    if (!taprec.errorInDataRecording && !taprec.wasFileNotCreated)
    {
      // La grabación fue parada pero no hubo errores
      // entonces salvamos el fichero sin borrarlo
      
      // No quitar esto, permite leer la variable totalBlockTransfered
      int tbt = taprec.totalBlockTransfered;
      
      if (tbt > 1)
      {
        taprec.terminate(false);
        LAST_MESSAGE = "Recording STOP. File succesfully saved.";
        hmi.updateInformationMainPage();
        delay(1000);
      }
      else
      {
        taprec.terminate(true);
        LAST_MESSAGE = "Recording STOP. No file saved";
        hmi.updateInformationMainPage();      
        delay(1000);
      }
    }
    else if (taprec.errorInDataRecording)
    {
      // La grabación fue parada porque hubo errores
      // entonces NO salvamos el fichero, lo borramos
      // Recording STOP
      //
      taprec.terminate(true);
      LAST_MESSAGE = "Recording STOP. No file saved."; 
      // Actualizamos la pantalla
      hmi.updateInformationMainPage();      
      delay(1000);
    }
    else if (taprec.wasFileNotCreated)
    {
        // Si no se crea el fichero no se puede seguir grabando
        taprec.terminate(false);
        LAST_MESSAGE = "Error in filesystem or SD.";
        // Actualizamos la pantalla
        hmi.updateInformationMainPage();        
        delay(1000);
    }

    // Inicializamos variables de control
    waitingRecMessageShown = false;
    REC = false;

    // Actualizamos mensaje en HMI
    //taprec.prepareHMI();

    SerialHW.println("");        
    SerialHW.println("Recording procces finish.");
    SerialHW.println("");    

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

  SerialHW.println("Setting Audiokit.");

  // Configuramos los pulsadores
  configureButtons();

  // Configuramos el ESP32kit
  LOGLEVEL_AUDIOKIT = AudioKitError;

  // Configuracion de las librerias del AudioKit
  setAudioOutput();

  SerialHW.println("Done!");

  SerialHW.println("Initializing SD SLOT.");
  
  // Configuramos acceso a la SD
  hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"" );
  delay(750);

  int SD_Speed = SD_FRQ_MHZ_INITIAL;  // Velocidad en MHz (config.h)
  setSDFrequency(SD_Speed);

  // Forzamos a 26MHz
  sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(26));

  // Le pasamos al HMI el gestor de SD
  hmi.set_sdf(sdf);

  SerialHW.println("Done!");

  // Esperamos finalmente a la pantalla
  SerialHW.println("");
  SerialHW.println("Waiting for LCD.");
  SerialHW.println("");

  if(psramInit()){
    SerialHW.println("\nPSRAM is correctly initialized");
    hmi.writeString("statusLCD.txt=\"PSRAM OK\"" );

  }else{
    SerialHW.println("PSRAM not available");
    hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"" );
  }    
  delay(750);

  hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"" );
  
  // Si false --> No esperamos sincronización
  // Si true  --> Si esperamos
  waitForHMI(CFG_FORZE_SINC_HMI);

  pTAP.set_HMI(hmi);
  pTZX.set_HMI(hmi);

  pTAP.set_SDM(sdm);
  pTZX.set_SDM(sdm);

  zxp.set_ESP32kit(ESP32kit);

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
  sendStatus(REC_ST, 0);


  LAST_MESSAGE = "Press EJECT to select a file.";

  #ifdef TESTDEV
      SerialHW.println("");
      SerialHW.println("");
      SerialHW.println("Testing readFileRange32(..)"); 

      uint8_t* buffer = (uint8_t*)calloc(11,sizeof(uint8_t));
      char fileTest[65];
      strncpy(fileTest,"/Correcaminos - Nifty Lifty (1984)(Visions Software Factory).tzx",64);
      sdFile32 = sdm.openFile32(sdFile32,fileTest);
      buffer = sdm.readFileRange32(sdFile32,0,10,true);
      free(buffer);

      SerialHW.println("");
      SerialHW.println("");
      SerialHW.println("Test OK");  
  #endif

  // Call the Task0code primitive function to avoid the error of Task0code not defined
  void Task0code( void * pvParameters );
    
  // Control de la UART - HMI
  xTaskCreatePinnedToCore(Task0code, "TaskCORE0", 4096, NULL, 5, &Task0,  0);
  delay(500);

  // Deshabilitamos el WDT en cada core
  disableCore0WDT();
  disableCore1WDT();

  // Inicializamos el modulo de recording
  taprec.set_HMI(hmi);
  taprec.set_SdFat32(sdf);
  
  hmi.getMemFree();
}

void setSTOP()
{
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;

  BLOCK_SELECTED = 0;

  LAST_MESSAGE = "Tape stop.";
  hmi.updateInformationMainPage();
  
  SerialHW.println("");
  SerialHW.println("Tape stop.");

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
  ESP32kit.setVolume(MAIN_VOL);

  sendStatus(REC_ST, 0);

  //Activamos la animación
  hmi.writeString("tape.tmAnimation.en=1"); 

  // Reproducimos el fichero
  LAST_MESSAGE = "Loading in progress. Please wait.";
  hmi.updateInformationMainPage();

  if (TYPE_FILE_LOAD == "TAP")
  {
      hmi.getMemFree();
      pTAP.play();
      //Paramos la animación
      hmi.writeString("tape.tmAnimation.en=0"); 
      pTAP.updateMemIndicator();
  }
  else if (TYPE_FILE_LOAD = "TZX")
  {
      hmi.getMemFree();
      pTZX.play();
      //Paramos la animación
      hmi.writeString("tape.tmAnimation.en=0"); 
  }
}

void loadingFile()
{
  // Cogemos el fichero seleccionado y lo cargamos              
  char* file_ch = (char*)ps_malloc(256 * sizeof(char));
  FILE_TO_LOAD.toCharArray(file_ch, 256);

  // Si no está vacio
  if (FILE_TO_LOAD != "") {
    
    // Limpiamos los campos del TAPE
    hmi.clearInformationFile();

    // Convierto a mayusculas
    FILE_TO_LOAD.toUpperCase();

    if (FILE_TO_LOAD.indexOf(".TAP") != -1)
    {
        // Lo procesamos
        proccesingTAP(file_ch);
        TYPE_FILE_LOAD = "TAP";
        //hmi.getMemFree();
    }
    else if (FILE_TO_LOAD.indexOf(".TZX") != -1)    
    {
        // Lo procesamos. Para ZX Spectrum
        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TZX";
        //hmi.getMemFree();              
    }
    else if (FILE_TO_LOAD.indexOf(".TSX") != -1)
    {
        // Lo procesamos. Para MSX
        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TSX";
    }   
  }

  // Liberamos
  free(file_ch);  
}

void stopFile()
{
  //Paramos la animación
  setSTOP();     
  hmi.writeString("tape.tmAnimation.en=0"); 
}

void pauseFile()
{}

void ejectingFile()
{
  log("Eject executing");

  // Terminamos los players
  if (TYPE_FILE_LOAD == "TAP")
  {
      //free(myTAP.descriptor);

      pTAP.terminate();
      hmi.getMemFree();
  }
  else if (TYPE_FILE_LOAD = "TZX")
  {
      pTZX.terminate();
      hmi.getMemFree();
  }  
}

void recordingFile()
{
  // Liberamos myTAP.descriptor de la memoria si existe 
  if (myTAP.descriptor!= NULL) 
  {
    free(myTAP.descriptor);
  }
    
  // Preparamos para recording
  if (!waitingRecMessageShown)
  {
    SerialHW.println("");
    SerialHW.println("REC. Waiting for guide tone");
    SerialHW.println("");        

    //Activamos la animación
    hmi.writeString("tape.tmAnimation.en=1"); 

    // Inicializamos audio input
    setAudioInput();
    taprec.set_kit(ESP32kit);
    
    taprec.initialize(); 

    if (!taprec.createTempTAPfile())
    {
      LAST_MESSAGE = "Recorder not prepared.";
      hmi.updateInformationMainPage();
      waitingRecMessageShown = false;
    }
    else
    {
      waitingRecMessageShown = true;

      //writeString("");
      hmi.writeString("currentBlock.val=1");
      //writeString("");
      hmi.writeString("progressTotal.val=0");
      //writeString("");
      hmi.writeString("progression.val=0");
    }
    
    hmi.getMemFree();
      
  }
  else
  {
      // Iniciamos la grabación
      if (taprec.recording())
      {
          // Ha finalizado la grabación de un bloque
          // ahora estudiamos la causa
          if (taprec.stopRecordingProccess)   
          {
              // Hay tres casos
              // - El fichero para grabar el TAP no fue creado
              // - La grabación fue parada pero fue correcta (no hay errores)
              // - La grabación fue parada pero fue incorrecta (hay errores)
              SerialHW.println("");
              SerialHW.println("STOP 2 - REC");

              LOADING_STATE = 0;

              stopRecording();
              setSTOP();
          }
      }            
  }
}

void tapeControl()
{ 
  switch (tapeState)
  {
    case 0:

      // Estado inicial
      if (FILE_BROWSER_OPEN)
      {
        tapeState = 99;
        LOADING_STATE = 0;          
      }
      else if(FILE_SELECTED)
      {
        // FICHERO CARGADO EN TAPE
        loadingFile();
        LAST_MESSAGE = "File inside the TAPE.";
        hmi.updateInformationMainPage();  

        tapeState = 1;
        LOADING_STATE = 0;          
        SerialHW.println(tapeState);

      }
      else if(PLAY)
      {
        LAST_MESSAGE = "No file was selected.";
        hmi.updateInformationMainPage();
        LOADING_STATE = 0;          
        tapeState = 0;          
      }
      else if(REC)
      {
        recordingFile();
        tapeState = 200;
      }
      else
      {
        tapeState = 0;
      }
      break;

    case 1:
      // Esperando que hacer con el fichero cargado
      if (PLAY)
      {
        // Lo reproducimos
        LOADING_STATE = 1;          
        playingFile();
        tapeState = 2;
        SerialHW.println(tapeState);
      }
      else if(EJECT)
      {
        ejectingFile();            
        tapeState = 0;
        LOADING_STATE = 0;          
        SerialHW.println(tapeState);
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
        pauseFile();
        tapeState = 3;
      }
      else if(STOP)
      {
        stopFile();
        tapeState = 4;
        LOADING_STATE = 2;          
        SerialHW.println(tapeState);
      }
      else if(EJECT)
      {
        // Lo sacamaos del TAPE
        // cuando acabe el bloque, si está reproduciendo        
        LOADING_STATE = 2;          
        stopFile();
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
        playingFile();          
        tapeState = 2;
      }
      else if(STOP)
      {
        stopFile();
        LOADING_STATE = 2;          
        tapeState = 4;
      }
      else if(EJECT)
      {
        log("Ejecting 1");

        stopFile();
        ejectingFile();
        LOADING_STATE = 2;          
        tapeState = 0;
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
        ejectingFile();
        LOADING_STATE = 2;          
        tapeState = 0;
      }
      else
      {
        tapeState = 4;
      }
      break;
    case 5:
      
      if(!BLOCK_PLAYED)
      {
        log("Ejecting 2");
        ejectingFile();
        tapeState = 0;
      }
      else
      {
        tapeState = 5;
      }
      break;
    case 99:
      if (!FILE_BROWSER_OPEN)
      {
        tapeState = 0;
        LOADING_STATE = 0;          
      }
      else
      {
        tapeState = 99;
      }
      break;

    case 200:
      // REC estados
      if(STOP)
      {
        stopRecording();
        SerialHW.println("");
        SerialHW.println("STOP 1 - REC");
        setSTOP();
        tapeState = 0;
      }
      else
      {
        tapeState = 200;
      }
      break;

    default:
      break;
  }
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

void Task0code( void * pvParameters )
{
  // Core 0 - Para el HMI
  while(true)
  {
      hmi.readUART();
      // Control por botones
      //buttonsControl();
      //delay(50);
  }
}

void loop() 
{
    // CORE1
    tapeControl();
    //vTaskDelay(10);
    //delay(50);
}
