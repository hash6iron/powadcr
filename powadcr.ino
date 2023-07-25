/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: powadcr.ino
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

    Colaboradores en el proyecto:
      - Fernando Mosquera
      - Guillermo
      - Mario J
      - Pedro Pelaez
    
    Descripción:
    Programa principal del firmware del powadcr - Powa Digital Cassette Recording

    Version: 1.0

    Historico de versiones
    v.0.1 - Version de pruebas. En desarrollo
    v.1.0.0 - Primera versión optimizada, comentada y preparada con el procesador de TAP y generador de señal de audio.
    v.1.0.1 - Mejorada la rutina de inicializado de SD Card. Cambios relativos a las nuevas clases generadas. Corregido bug con el modo TEST


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

// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
#include "SdFat.h"
#include "globales.h"

//
#include "AudioKitHAL.h"
AudioKit ESP32kit;

#include "interface.h"
#include "config.h"

// Estos includes deben ir en este orden por dependencias
#include "SDmanager.h"

// Creamos el gestor de ficheros para usarlo en todo el firmware
SDmanager sdm;

#include "HMI.h"
#include "ZXProccesor.h"
// ZX Spectrum
#ifdef MACHINE_ZX
ZXProccesor zxp;
#endif


#include "TZXproccesor.h"
#include "TAPproccesor.h"


#include "test.h"
//
// #include <ESP32TimerInterrupt.h>
// #include <ESP32TimerInterrupt.hpp>

// Declaraciones para SdFat
SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;



HMI hmi;


TZXproccesor pTZX(ESP32kit);
TAPproccesor pTAP(ESP32kit);



void proccesingTAP(char* file_ch)
{
    //pTAP.set_SdFat32(sdf);
    pTAP.getInfoFileTAP(file_ch);

    LAST_MESSAGE = "Press PLAY to enjoy!";
    delay(125);
    hmi.updateInformationMainPage();

    FILE_NOTIFIED = true;  
}

void proccesingTZX(char* file_ch)
{
    //pTZX.set_SdFat32(sdf);
    pTZX.getInfoFileTZX(file_ch);

    LAST_MESSAGE = "Press PLAY to enjoy!";
    delay(125);
    hmi.updateInformationMainPage();

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

    case READY_ST:
      //hmi.writeString("");
      hmi.writeString("READYst.val=" + String(value));
      break;

    case ACK_LCD:
      //hmi.writeString("");
      hmi.writeString("tape.LCDACK.val=" + String(value));
      //hmi.writeString("");
      hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");
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

      Serial.println("");
      Serial.println("");
      Serial.println("SD card error!");

      Serial.println("");
      Serial.println("");
      Serial.println("SD downgrade at " + String(SD_Speed) + "MHz");

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
              Serial.println("");
              Serial.println("");
              Serial.println("Fatal error. SD card not compatible. Change SD");      
              
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
        Serial.println("");
        Serial.println("");
        Serial.println("SD card initialized at " + String(SD_Speed) + " MHz");

        hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"" );

        // Probamos a listar un directorio
        if (!sdm.dir.open("/"))
        {
            sdm.dir.close();
            SD_ok = false;
            lastStatus = false;

            Serial.println("");
            Serial.println("");
            Serial.println("Listing files. Error!");

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
            SD_ok = true;
            lastStatus = true;
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
      Serial.println();
      Serial.println();
      Serial.println("----- TEST ACTIVE ------");

      zxp.playBlock(testHeader, 19, testData, 154);
      zxp.playBlock(testScreenHeader, 19, testScreenData, 6914);

      Serial.println();
      Serial.println();
      Serial.println("------ END TEST -------");
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

      Serial.println("");
      Serial.println("LCD READY");
      Serial.println("");

      sendStatus(ACK_LCD, 1);  
    }
}

void setup() {
  // Configuramos el nivel de log
  //Serial.begin(115200);
  Serial.begin(921600);
  delay(250);

  // Forzamos un reinicio de la pantalla
  hmi.writeString("rest");
  delay(250);

  // Indicamos que estamos haciendo reset
  sendStatus(RESET, 1);
  delay(1250);

  hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"" );
  delay(2000);

  Serial.println("Setting Audiokit.");

  // Configuramos los pulsadores
  //configureButtons();

  // Configuramos el ESP32kit
  LOGLEVEL_AUDIOKIT = AudioKitError;

  // Configuracion de las librerias del AudioKit
  auto cfg = ESP32kit.defaultConfig(AudioOutput);

  Serial.println("Initialized Audiokit.");

  ESP32kit.begin(cfg);
  ESP32kit.setVolume(MAIN_VOL);

  Serial.println("Done!");

  Serial.println("Initializing SD SLOT.");
  
  // Configuramos acceso a la SD
  hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"" );
  delay(1250);

  int SD_Speed = SD_FRQ_MHZ_INITIAL;  // Velocidad en MHz (config.h)
  setSDFrequency(SD_Speed);

  Serial.println("Done!");

  // Esperamos finalmente a la pantalla
  Serial.println("");
  Serial.println("Waiting for LCD.");
  Serial.println("");

  hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"" );
  
  // Si false --> No esperamos sincronización
  // Si true  --> Si esperamos
  waitForHMI(false);

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

  sendStatus(STOP_ST, 1);
  sendStatus(PLAY_ST, 0);
  sendStatus(PAUSE_ST, 0);
  sendStatus(READY_ST, 1);
  sendStatus(END_ST, 0);

  LAST_MESSAGE = "Press EJECT to select a file.";

  #ifdef TESTDEV
      Serial.println("");
      Serial.println("");
      Serial.println("Testing readFileRange32(..)"); 

      byte* buffer = (byte*)calloc(11,sizeof(byte));
      char* fileTest = "/Correcaminos - Nifty Lifty (1984)(Visions Software Factory).tzx";
      sdFile32 = sdm.openFile32(sdFile32,fileTest);
      buffer = sdm.readFileRange32(sdFile32,0,10,true);
      free(buffer);
      buffer = NULL;

      Serial.println("");
      Serial.println("");
      Serial.println("Test OK");  
  #endif

}


void loop() {

  // Procedimiento principal

  //buttonsControl();
  hmi.readUART();

  if (!FILE_BROWSER_OPEN)
  {
      if (STOP == true && LOADING_STATE != 0) {
        LOADING_STATE = 0;
        BLOCK_SELECTED = 0;
        
        hmi.updateInformationMainPage();

        STOP = false;
        PLAY = false;
        PAUSE = false;

        sendStatus(STOP_ST, 0);
        sendStatus(PLAY_ST, 0);
        sendStatus(PAUSE_ST, 0);
        sendStatus(READY_ST, 1);
        sendStatus(END_ST, 0);
      }

      if (FILE_SELECTED && !FILE_NOTIFIED) 
      {

        // Cogemos el fichero seleccionado y lo cargamos              
        char* file_ch = (char*)calloc(FILE_TO_LOAD.length() + 1, sizeof(char));
        FILE_TO_LOAD.toCharArray(file_ch, FILE_TO_LOAD.length() + 1);

        // Si no está vacio
        if (FILE_TO_LOAD != "") {
          
          // Convierto a mayusculas
          FILE_TO_LOAD.toUpperCase();

          if (FILE_TO_LOAD.indexOf(".TAP") != -1)
          {
              // Lo procesamos
              proccesingTAP(file_ch);
              TYPE_FILE_LOAD = "TAP";
          }
          else if (FILE_TO_LOAD.indexOf(".TZX") != -1)    
          {
              // Lo procesamos
              proccesingTZX(file_ch);
              TYPE_FILE_LOAD = "TZX";            
          }   
        }
      }

      if (PLAY == true && LOADING_STATE == 0) 
      {

        ESP32kit.setVolume(MAIN_VOL);

        if (FILE_SELECTED) 
        {
          // Pasamos a estado de reproducción
          LOADING_STATE = 1;
          sendStatus(PLAY_ST, 1);
          sendStatus(STOP_ST, 0);
          sendStatus(PAUSE_ST, 0);
          sendStatus(END_ST, 0);

          // Reproducimos el fichero
          if (TYPE_FILE_LOAD == "TAP")
          {
              pTAP.play();
          }
          else if (TYPE_FILE_LOAD = "TZX")
          {
              pTZX.play();
          }
  
        } 
        else 
        {
          LAST_MESSAGE = "No file was selected.";
          
          if (TYPE_FILE_LOAD == "TAP")
          {
              pTAP.initialize();
          }
          else if (TYPE_FILE_LOAD = "TZX")
          {
              pTZX.initialize();
          }

          hmi.updateInformationMainPage();
        }
      }
  }
}
