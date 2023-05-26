
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
#include "ObserverEvents.h"
//   -- En esta se encuentran las variables globales a todo el proyecto
#include "globales.h"
//
#include "interface.h"
#include "config.h"
#include "Logger.h"
#include "AudioKitHAL.h"
#include "ZXProccesor.h"
#include "SDmanager.h"
#include "TAPproccesor.h"
#include "test.h"

// Declaración para eñ audiokitHal
AudioKit ESP32kit;

// Declaraciones para SdFat
SdFat sd;
SdFile sdFile;
File32 sdFile32;

// Inicializamos la clase ZXProccesor con el kit de audio.
// ZX Spectrum
#ifdef MACHINE==0
  ZXProccesor zxp(ESP32kit);
#endif

void setSDFrequency(int SD_Speed)
{
    bool SD_ok = false;
    while(!SD_ok)
    {
        if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed))) 
        {
            Serial.println("SD card error!");
            SD_Speed = SD_Speed - 5;
            if (SD_Speed < 4)
            {
                SD_Speed = 4;
            }
            Serial.println("SD downgrade at " + String(SD_Speed) + "MHz");
        }
        else
        {
            Serial.println("SD card initialized at " + String(SD_Speed) + " MHz");
            SD_ok = true;
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
    tBlockDescriptor* bDscr = (tBlockDescriptor*)malloc[pTAP.myTAP.numBlocks];
    bDscr = pTAP.myTAP.descriptor;


    // Entregamos información por consola
    Serial.println("");
    Serial.println("File opened. Size: " + String(rlen) + " bytes");  
    
    // Inicializamos el buffer de reproducción. Memoria dinamica
    byte* bufferPlay = NULL;

    // Entregamos información por consola
    Serial.println("Name TAP:    " + pTAP.myTAP.name);
    Serial.println("Num. blocks: " + String(pTAP.myTAP.numBlocks));
    Serial.println();
    Serial.println();
    
    // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
    int m = BLOCK_SELECTED;
    for (int i=m;i<pTAP.myTAP.numBlocks;i++)
    {

        // Almacenmas el bloque en curso para un posible PAUSE
        CURRENT_BLOCK_IN_PROGRESS = i;

        // Vamos entregando información por consola
        Serial.println("Block --> [" + String(i) + "]");

        // Si hemos pulsado PAUSE y estamos en estado de carga LOADING_STATE = 1
        // paramos
        if (PAUSE==true && LOADING_STATE == 1)
        {
            Serial.println();
            Serial.println("** PAUSE");
            Serial.println();
            //Pausa mantenemos el bloque
            LOADING_STATE = 2;  
            break;
        }

        if (STOP==true && LOADING_STATE == 1)
        {
            Serial.println();
            Serial.println("** STOP");
            Serial.println();
            // Si paramos el bloque vuelve a ser el primero
            CURRENT_BLOCK_IN_PROGRESS = 0;
            LOADING_STATE = 0;  //STOP - Comenzamos de nuevo
            break;
        }

        //Ahora vamos lanzando bloques dependiendo de su tipo
        pTAP.showInfoBlockInProgress(bDscr[i].type);

        // Reproducimos el fichero
        if (bDscr[i].type == 0 || bDscr[i].type == 1 || bDscr[i].type == 7)
        {
            // CABECERAS
            bufferPlay = (byte*)malloc(bDscr[i].size);
            bufferPlay = readFileRange32(sdFile32,bDscr[i].offset,bDscr[i].size,true);
            
            zxp.playHeader(bufferPlay, bDscr[i].size);
            free(bufferPlay);
        
        }
        else
        {
            // DATA
            int blockSize = bDscr[i].size;

            // Si el SPLIT esta activado y el bloque es mayor de 20KB hacemos Split.
            if ((SPLIT_ENABLED) && (blockSize > SIZE_TO_ACTIVATE_SPLIT))
            {

                // Lanzamos dos bloques
                int bl1 = blockSize/2;
                int bl2 = blockSize - bl1;
                int blockPlaySize = 0;
                int offsetPlay = 0;

                Serial.println("   > Splitted block. Size [" + String(blockSize) + "]");

                for (int j=0;j<2;j++)
                {
                   if (j==0)
                   {
                       blockPlaySize = bl1;
                       offsetPlay = bDscr[i].offset;
                       bufferPlay = (byte*)malloc(blockPlaySize);

                       bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize,true);
                       zxp.playDataBegin(bufferPlay, blockPlaySize);
                       free(bufferPlay);

                   }
                   else
                   {                        
                       blockPlaySize = bl2;
                       offsetPlay = offsetPlay + bl1;

                       bufferPlay = (byte*)malloc(blockPlaySize);
                       bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize,true);
                       zxp.playDataEnd(bufferPlay, blockPlaySize);
                       free(bufferPlay);
                   }
                }                          
            }
            else
            {
                // En el caso de NO USAR SPLIT o el bloque es menor de 20K
                bufferPlay = (byte*)malloc(bDscr[i].size);
                bufferPlay = readFileRange32(sdFile32,bDscr[i].offset,bDscr[i].size,true);
                zxp.playData(bufferPlay, bDscr[i].size);
                free(bufferPlay);
            }
        }
    } 
    sleep(3);

    Serial.println("");
    Serial.println("Finish. STOP THE TAPE.");

    free(bDscr);
    sdFile32.close();

}

void setup() 
{
    // Configuramos el nivel de log
    Serial.begin(115200);
    initLogger();
    
    debugLog("Hola mundo");
    Serial.println("Setting Audiokit.");
    
    // Configuramos los pulsadores
    configureButtons();

    // Configuramos el ESP32kit
    LOGLEVEL_AUDIOKIT = AudioKitError; 

    // Configuracion de las librerias del AudioKit
    auto cfg = ESP32kit.defaultConfig(AudioOutput);
      
    Serial.println("Initialized Audiokit.");

    ESP32kit.begin(cfg);  

    Serial.println("Done!");

    // Configuramos la velocidad de acceso a la SD
    int SD_Speed = SD_FRQ_MHZ_INITIAL;         // Velocidad en MHz (config.h)
    setSDFrequency(SD_Speed);

    // Si es test está activo. Lo lanzamos
    #if TEST==1
      test();
    #endif

}


void loop() {

  // Procedimiento principal

  // \games\Classic48\Trashman\TRASHMAN.TAP
  //sdf.ls("/", LS_R);
  //sdf.ls("/games/Classic48/Trashman/",LS_R);
  
  buttonsControl();

  if (PLAY==true && LOADING_STATE == 0)
  {
      LOADING_STATE = 1;
      Serial.println("");
      Serial.println("Starting TAPE PLAYER.");
      Serial.println("");
      //playTAPfile_ZXSpectrum("/games/Classic48/Trashman/TRASHMAN.TAP");
      //playTAPfile_ZXSpectrum("/games/Classic128/Castlevania/Castlevania.tap");
      //playTAPfile_ZXSpectrum("/games/Classic128/Shovel Adventure/Shovel Adventure ZX 1.2.tap");
      //playTAPfile_ZXSpectrum("/games/Actuales/Donum/Donum_ESPv1.1.tap");
      playTAPfile_ZXSpectrum("/games/ROMSET/5000 juegos ordenados/D/Dark Fusion (1988)(Gremlin Graphics Software).tap");
  }

}
