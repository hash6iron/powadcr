/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: powadcr.ino
    
    Creado por:
      Antonio Tamairón. 2023  
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
#include "HMI.h"
#include "ZXProccesor.h"
#include "TAPproccesor.h"
#include "TZXproccesor.h"

#include "test.h"
//
// #include <ESP32TimerInterrupt.h>
// #include <ESP32TimerInterrupt.hpp>

// Declaraciones para SdFat
SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;

// Añadimos los distintos manejadores de ficheros, TAP y TZX
SDmanager sdm;
HMI hmi;
TAPproccesor pTAP(ESP32kit);
//TZXproccesor pTZX;


//int rlen = 0;


//
// hw_timer_t *Timer0_Cfg = NULL;

// Inicializamos la clase ZXProccesor con el kit de audio.
// ZX Spectrum
#ifdef MACHINE_ZX
ZXProccesor zxp;
#endif


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

      hmi.writeString("statusLCD.txt=\"SD FREQ. DOWN AT " + String(SD_Speed) + " MHz\"" );



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

void waitForHMI()
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
  waitForHMI();

  pTAP.set_HMI(hmi);


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
              
        char* file_ch = (char*)calloc(FILE_TO_LOAD.length() + 1, sizeof(char));
        FILE_TO_LOAD.toCharArray(file_ch, FILE_TO_LOAD.length() + 1);

        // Serial.println("++++++++++++++++++++++++++++++++++++++++++++++");
        // Serial.println("");
        // Serial.println(String(file_ch));
        // Serial.println("++++++++++++++++++++++++++++++++++++++++++++++");

        if (FILE_TO_LOAD != "") {

          pTAP.set_SdFat32(sdf);
          pTAP.getInfoFileTAP(file_ch);
          
          LAST_MESSAGE = "Press PLAY to enjoy!";
          delay(125);
          hmi.updateInformationMainPage();

          FILE_NOTIFIED = true;
        }
      }

      if (PLAY == true && LOADING_STATE == 0) 
      {

        ESP32kit.setVolume(MAIN_VOL);

        if (FILE_SELECTED) 
        {

          char* file_ch = (char*)calloc(FILE_TO_LOAD.length() + 1, sizeof(char));
          FILE_TO_LOAD.toCharArray(file_ch, FILE_TO_LOAD.length() + 1);

          // Pasamos a estado de reproducción
          LOADING_STATE = 1;
          sendStatus(PLAY_ST, 1);
          sendStatus(STOP_ST, 0);
          sendStatus(PAUSE_ST, 0);
          sendStatus(END_ST, 0);
          
          Serial.println("");
          Serial.println("Fichero seleccionado: " + FILE_TO_LOAD);

          // Reproducimos el fichero
          pTAP.play();
        } 
        else 
        {
          LAST_MESSAGE = "No file was selected.";
          pTAP.initializeTap();
          hmi.updateInformationMainPage();
        }
      }
  }
}
