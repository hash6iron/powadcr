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
    v.1.0 - Primera versión optimizada, comentada y preparada con el procesador de TAP y generador de señal de audio.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// Librerias (mantener este orden)
//   -- En esta se encuentran las variables globales a todo el proyecto
#include "globales.h"

//
#include "AudioKitHAL.h"
// Declaración para eñ audiokitHal
AudioKit ESP32kit;

#include "HMI.h"

#include "interface.h"
#include "config.h"

#include "ZXProccesor.h"
#include "SDmanager.h"
#include "TAPproccesor.h"

#include "test.h"
//
// #include <ESP32TimerInterrupt.h>
// #include <ESP32TimerInterrupt.hpp>



// Declaraciones para SdFat
SdFat sd;
SdFile sdFile;
File32 sdFile32;

//
// hw_timer_t *Timer0_Cfg = NULL;

// Inicializamos la clase ZXProccesor con el kit de audio.
// ZX Spectrum
#ifdef MACHINE==0
  ZXProccesor zxp(ESP32kit);
#endif



void setSDFrequency(int SD_Speed)
{
    bool SD_ok = false;
    bool lastStatus = false;
    while(!SD_ok)
    {
        if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed)) && lastStatus) 
        {
            Serial.println("SD card error!");
            SD_Speed = SD_Speed - 5;
            if (SD_Speed < 4)
            {
                SD_Speed = 4;
                lastStatus = true;
            }
            Serial.println("SD downgrade at " + String(SD_Speed) + "MHz");
        }
        else if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed)) && !lastStatus)
        {
          LAST_MESSAGE = "Error in SD Card. Check and reset POWADCR.";
          updateInformationMainPage();
        }
        else
        {
            Serial.println("SD card initialized at " + String(SD_Speed) + " MHz");
            SD_ok = true;

            LAST_MESSAGE = "Please, select file and press PLAY.";
            updateInformationMainPage();
        }
    }
}

void test()
{
  // Si queremos activar el test que hay en memoria, para chequear con el ordenador
  #ifdef MACHINE==0
      //ZX Spectrum
      Serial.println("----- TEST ACTIVE ------");

      zxp.playBlock(testHeader, 19, testData, 154);
      zxp.playBlock(testScreenHeader, 19, testScreenData, 6914);
      sleep(10);  
  #endif
}

void playTAPfile_ZXSpectrum(char* path)
{   
    // Abrimos el fichero
    sdFile32 = openFile32(sdFile32, path);
    // Obtenemos su tamaño total
    int rlen = sdFile32.available();
    // creamos un objeto TAPproccesor
    TAPproccesor pTAP(sdFile32, rlen);

    //struct Vector *vector = malloc(sizeof (struct Vector));
    tBlockDescriptor* bDscr = (tBlockDescriptor*)calloc(globalTAP.numBlocks,sizeof(tBlockDescriptor));
    //bDscr = globalTAP.descriptor;
       
    // Inicializamos el buffer de reproducción. Memoria dinamica
    byte* bufferPlay = NULL;

    // Entregamos información por consola
    PROGRAM_NAME = globalTAP.name;
    TOTAL_BLOCKS = globalTAP.numBlocks;
    LAST_NAME = "..";

    // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
    int m = BLOCK_SELECTED;
    //BYTES_TOBE_LOAD = rlen;

    // Reiniciamos
    if (BLOCK_SELECTED==0)
    {
      BYTES_LOADED = 0;
      BYTES_TOBE_LOAD = rlen;
      writeString("");
      writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));      
    }
    else
    {
      BYTES_TOBE_LOAD = rlen - globalTAP.descriptor[BLOCK_SELECTED-1].offset;
    }


    

    for (int i=m;i<globalTAP.numBlocks;i++)
    {   

        //LAST_NAME = bDscr[i].name;

        // Obtenemos el nombre del bloque
        LAST_NAME = globalTAP.descriptor[i].name;
        LAST_SIZE = globalTAP.descriptor[i].size;

        // Almacenmas el bloque en curso para un posible PAUSE
        if (LOADING_STATE != 2)
        {
            CURRENT_BLOCK_IN_PROGRESS = i;
            BLOCK_SELECTED = i;

            writeString("");
            writeString("currentBlock.val=" + String(i+1));

            writeString("");
            writeString("progression.val=" + String(0));        
        }

        // Vamos entregando información por consola
        //Serial.println("Block --> [" + String(i) + "]");

        //Paramos la reproducción.
        if (LOADING_STATE == 2)
        {
            PAUSE = false;
            STOP = false;
            PLAY = false;
            LOADING_STATE == 0;
            break;
        }

        //Ahora vamos lanzando bloques dependiendo de su tipo
        //Esto actualiza el LAST_TYPE
        pTAP.showInfoBlockInProgress(globalTAP.descriptor[i].type);
        
        // Actualizamos HMI
        updateInformationMainPage();

        // Reproducimos el fichero
        if (globalTAP.descriptor[i].type == 0)
        {
            // CABECERAS
            bufferPlay = (byte*)calloc(globalTAP.descriptor[i].size,sizeof(byte));
            bufferPlay = readFileRange32(sdFile32,globalTAP.descriptor[i].offset,globalTAP.descriptor[i].size,true);
            
            zxp.playHeaderProgram(bufferPlay, globalTAP.descriptor[i].size);
            //free(bufferPlay);
        
        }
        else if (globalTAP.descriptor[i].type == 1 || globalTAP.descriptor[i].type == 7)
        {
            // CABECERAS
            bufferPlay = (byte*)calloc(globalTAP.descriptor[i].size,sizeof(byte));
            bufferPlay = readFileRange32(sdFile32,globalTAP.descriptor[i].offset,globalTAP.descriptor[i].size,true);
            
            zxp.playHeader(bufferPlay, globalTAP.descriptor[i].size);          
        }
        else 
        {
            // DATA
            int blockSize = globalTAP.descriptor[i].size;

            // Si el SPLIT esta activado y el bloque es mayor de 20KB hacemos Split.
            if ((SPLIT_ENABLED) && (blockSize > SIZE_TO_ACTIVATE_SPLIT))
            {

                // Lanzamos dos bloques
                int bl1 = blockSize/2;
                int bl2 = blockSize - bl1;
                int blockPlaySize = 0;
                int offsetPlay = 0;

                //Serial.println("   > Splitted block. Size [" + String(blockSize) + "]");

                for (int j=0;j<2;j++)
                {
                   if (j==0)
                   {
                       blockPlaySize = bl1;
                       offsetPlay = globalTAP.descriptor[i].offset;
                       bufferPlay = (byte*)calloc(blockPlaySize,sizeof(byte));

                       bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize,true);
                       zxp.playDataBegin(bufferPlay, blockPlaySize);
                       //free(bufferPlay);

                   }
                   else
                   {                        
                       blockPlaySize = bl2;
                       offsetPlay = offsetPlay + bl1;

                       bufferPlay = (byte*)calloc(blockPlaySize,sizeof(byte));
                       bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize,true);
                       zxp.playDataEnd(bufferPlay, blockPlaySize);
                       //free(bufferPlay);
                   }
                }                          
            }
            else
            {
                // En el caso de NO USAR SPLIT o el bloque es menor de 20K
                bufferPlay = (byte*)calloc(globalTAP.descriptor[i].size,sizeof(byte));
                bufferPlay = readFileRange32(sdFile32,globalTAP.descriptor[i].offset,globalTAP.descriptor[i].size,true);
                zxp.playData(bufferPlay, globalTAP.descriptor[i].size);
                //free(bufferPlay);
            }
        }
    } 

    Serial.println("");
    Serial.println("Playing was finish.");
    // En el caso de no haber parado manualmente, es por finalizar 
    // la reproducción
    if (LOADING_STATE == 1)
    {
        PLAY=false;
        STOP=true;
        PAUSE=false;
        BLOCK_SELECTED = 0;
        LAST_MESSAGE = "Playing end.";

        updateInformationMainPage();

        // Ahora ya podemos tocar el HMI panel otra vez    
        writeString("");
        writeString("rx.txt=\"END\"");
    }

    // Cerrando
    LOADING_STATE = 0;
    
    free(bufferPlay);

    delay(1000);

    sdFile32.close();

    Serial.flush();

    // Ahora ya podemos tocar el HMI panel otra vez    
    writeString("");
    writeString("rx.txt=\"READY\"");

}




void setup() 
{
    // Configuramos el nivel de log
    Serial.begin(115200);
    
    LAST_MESSAGE = "Resseting CPU. Please wait.";
    updateInformationMainPage();
    delay(2000);

    //Serial2.begin(9600);
    Serial.println("Setting Audiokit.");
    
    // Configuramos los pulsadores
    configureButtons();

    // Configuramos el ESP32kit
    LOGLEVEL_AUDIOKIT = AudioKitError; 

    // Configuracion de las librerias del AudioKit
    auto cfg = ESP32kit.defaultConfig(AudioOutput);
      
    Serial.println("Initialized Audiokit.");

    ESP32kit.begin(cfg);  
    ESP32kit.setVolume(MAIN_VOL);
  
    Serial.println("Done!");

    //Esperamos a la pantalla
    while (!LCD_ON)
    {
        readUART();      
    }

    delay(2000);

    // Configuramos la velocidad de acceso a la SD
    int SD_Speed = SD_FRQ_MHZ_INITIAL;         // Velocidad en MHz (config.h)
    setSDFrequency(SD_Speed);

    // Si es test está activo. Lo lanzamos
    #if TEST==1
      test();
    #endif

    // Interrupciones HW
    // Timer0_Cfg = timerBegin(0, 80, true);
    // timerAttachInterrupt(Timer0_Cfg, &Timer0_ISR, true);
    // timerAlarmWrite(Timer0_Cfg, 1000000, true);
    // timerAlarmEnable(Timer0_Cfg); 
    LOADING_STATE = 0; 
    BLOCK_SELECTED = 0;  
}


void loop() 
{

  // Procedimiento principal

  // \games\Classic48\Trashman\TRASHMAN.TAP
  //sdf.ls("/", LS_R);
  //sdf.ls("/games/Classic48/Trashman/",LS_R);
  
  //buttonsControl();
  readUART();

  if (STOP==true)
  {
      LOADING_STATE = 0;
      BLOCK_SELECTED = 0;
      updateInformationMainPage();      
      STOP=false;
      PLAY=false;
      PAUSE=false;
  }

  if (PLAY==true && LOADING_STATE == 0)
  {
      
      ESP32kit.setVolume(MAIN_VOL);

      if (FILE_SELECTED)
      {

          //Serial.println("");
          //Serial.println(ESP.getFreeHeap());
          //Serial.println("");

          // Pasamos a estado de reproducción
          LOADING_STATE = 1;

          //Serial.println("");
          //Serial.println("Starting TAPE PLAYER.");
          //Serial.println("");
          playTAPfile_ZXSpectrum("/games/Classic48/Trashman/TRASHMAN.TAP");
          //playTAPfile_ZXSpectrum("/games/Classic128/Castlevania/Castlevania.tap");
          //playTAPfile_ZXSpectrum((char*)"/games/Classic128/Shovel Adventure/Shovel Adventure ZX 1.2.tap");
          //playTAPfile_ZXSpectrum("/games/Actuales/Donum/Donum_ESPv1.1.tap");
          //playTAPfile_ZXSpectrum("/games/ROMSET/5000 juegos ordenados/D/Dark Fusion (1988)(Gremlin Graphics Software).tap");
          //playTAPfile_ZXSpectrum("/games/ROMSET/5000 juegos ordenados/A/Arkanoid II - Revenge of Doh (1988)(Imagine Software)[128K][Multiface copy].tap");
          //playTAPfile_ZXSpectrum("/games/ROMSET/5000 juegos ordenados/A/Arkanoid II - Revenge of Doh (1988)(Imagine Software)[128K].tap");
          //playTAPfile_ZXSpectrum("/games/ROMSET/5000 juegos ordenados/A/Arkanoid II - Revenge of Doh (1988)(Imagine Software)[cr][128K].tap");
      }
      else
      {
            LAST_MESSAGE = "No file was selected.";
            updateInformationMainPage();
      }

  }

}
