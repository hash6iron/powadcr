
/*
 powadcr.ino
 Antonio Tamairón. 2023

 Descripción:
 Programa principal del firmware del powadcr - Powa Digital Cassette Recording

 Version: 0.1

 */



// Librerias necesarias prioritaria
#include "interface.h"

// Otras
#include "config.h"
#include "AudioKitHAL.h"
#include "ZXProccesor.h"
#include "SDmanager.h"
#include "TAPproccesor.h"
#include "test.h"



//using namespace audio_tools;

AudioKit ESP32kit;

SdFat sd;
SdFile sdFile;
File32 sdFile32;

// Inicializamos la clase ZXProccesor con el kit de audio.
#ifdef MACHINE==0
  ZXProccesor zxp(ESP32kit);
#endif


void test()
{
  Serial.println("----- TEST ACTIVE ------");

  zxp.playBlock(testHeader, 19, testData, 154);
  zxp.playBlock(testScreenHeader, 19, testScreenData, 6914);
  sleep(10);  
}

int getLoadingStatus(int current_block)
{
   // Esto lo hacemos para las funciones de pausa / paro
    int blockInProgress = 0;

    if (LOADING_STATE == 2)
    {
        blockInProgress = current_block;
    }

    return blockInProgress;
}

void playTAPfile(char* path)
{

    // Listamos el directorio
    //sdf.ls("/games/Classic48/Trashman/",LS_R);
    
    // Abrimos el fichero
    sdFile32 = openFile32(sdFile32, path);

    // Leemos todo el fichero
    int rlen = sdFile32.available();
    byte* buffer = new byte[rlen];
    buffer = readFile32(sdFile32);
    Serial.println("");
    Serial.println("File open.");
    Serial.println("Extracted BYTES succes!");
    
    
    // Comenzamos
    TAPproccesor pTAP(buffer, rlen);
    byte* bufferPlay = NULL;
    //sleep(10);

    Serial.println("Num. blocks: " + String(pTAP.myTAP.numBlocks));
    Serial.println();
    Serial.println();
    
    //int m = getLoadingStatus(pTAP.CURRENT_LOADING_BLOCK);
    int m =0;
    // Ahora reproducimos todo
    for (int i=m;i<pTAP.myTAP.numBlocks;i++)
    {

        //pTAP.CURRENT_LOADING_BLOCK = i;
        Serial.println("Block --> [" + String(i) + "]");


        if (PAUSE==true && LOADING_STATE == 1)
        {
            Serial.println();
            Serial.println("** PAUSE");
            Serial.println();
            LOADING_STATE = 2;  //Pausa mantenemos el bloque
            break;
        }

        if (STOP==true && LOADING_STATE == 1)
        {
            Serial.println();
            Serial.println("** STOP");
            Serial.println();
            LOADING_STATE = 0;  //STOP - Comenzamos de nuevo
            break;
        }

        //Ahora vamos lanzando bloques
        switch(pTAP.bDscr[i].type)
        {
            case 0:

                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("> PROGRAM HEADER");
                #endif
                
                break;

            case 1:

                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("");
                  Serial.println("> BYTE HEADER");
                #endif
                
                break;

            case 7:

                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("");
                  Serial.println("> SCREEN HEADER");
                #endif
                
                break;

            case 2:
                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("");
                  Serial.println("> BASIC PROGRAM");
                #endif

                break;

            case 3:
                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("");
                  Serial.println("> SCREEN");
                #endif

                break;

            case 4:
                // Definimos el buffer del PLAYER igual al tamaño del bloque
                #ifdef LOG==3
                  Serial.println("");
                  Serial.println("> BYTE CODE");
                #endif

                break;

        }

        // Reproducimos el fichero
        if (pTAP.bDscr[i].type == 0 || pTAP.bDscr[i].type == 1 || pTAP.bDscr[i].type == 7)
        {
            // CABECERA
            bufferPlay = new byte[pTAP.bDscr[i].size];
            bufferPlay = readFileRange32(sdFile32,pTAP.bDscr[i].offset,pTAP.bDscr[i].size);
            
            zxp.playHeader(bufferPlay, pTAP.bDscr[i].size);        
        }
        else
        {
            // DATA
            int blockSize = pTAP.bDscr[i].size;

            if (blockSize > 20000)
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
                        offsetPlay = pTAP.bDscr[i].offset;
                        bufferPlay = new byte[blockPlaySize];

                        Serial.println("   > Offset [1/2] " + String(offsetPlay));
                        Serial.println("   > Size   [1/2] " + String(blockPlaySize));
                        bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize);
                        zxp.playDataBegin(bufferPlay, blockPlaySize);
                    }
                    else
                    {                        
                        blockPlaySize = bl2;
                        offsetPlay = offsetPlay + bl1;

                        Serial.println("   > Offset [2/2] " + String(offsetPlay));
                        Serial.println("   > Size   [2/2] " + String(blockPlaySize));

                        //
                        // AQUI FALLA (AL CREAR EL BUFFERPLAY) Y PUEDE SER POR EL USO INDEBIDO DE PUNTEROS Y HE COLAPSADO
                        // LA MEMORIA. REVISAR Y LIBERAR PUNTEROS
                        //
                        bufferPlay = new byte[blockPlaySize];


                        bufferPlay = readFileRange32(sdFile32,offsetPlay,blockPlaySize);
                        zxp.playDataEnd(bufferPlay, blockPlaySize);
                    }
                }
            }
            else
            {
                bufferPlay = new byte[blockSize];
                bufferPlay = readFileRange32(sdFile32,pTAP.bDscr[i].offset,pTAP.bDscr[i].size);
                zxp.playData(bufferPlay, pTAP.bDscr[i].size);
            }
        }
    } 
    sleep(3);

    Serial.println("");
    Serial.println("Finish. STOP THE TAPE.");

    sdFile32.close();

}

void setup() 
{
  // Configuramos el nivel de log
  //LOGLEVEL_AUDIOKIT = AudioKitError; 
  Serial.begin(115200);
  
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

  if (!sdf.begin(ESP32kit.pinSpiCs(), SPI_SPEED)) 
  {
		Serial.println("SD card error!");
		while (true);
	}
  else
  {
    Serial.println("SD card initialized!");
  }


  // //Asignamos funciones a los botones


  // *****************************************
   #if TEST==1
     test();
   #endif
  // *****************************************

  //PLAY = true;

  //pTAP.calculateChecksum(testHeader,0,18);
  //sleep(5);

  //tp.getTotalBlocks();
  // for (int n=0;n<rlen;n++)
  // {
  //   Serial.print(buffer[n],HEX);
  //   Serial.print(",");
  // }

  //Serial.println("Getting file name: ");

  // char* prgName = new char[10];
  // prgName = getNameOfProgram32(sdFile32);
  // Serial.println("");
  // Serial.print("Name in ZXSpectrum: ");
  // for (int n=0;n<10;n++)
  // {
  //     //String((char)my_hex)
  //     Serial.print(prgName[n]);
  // }

  // // Reproducimos la cabecera
  // byte* headerBl = new byte[19];
  // headerBl = getHeaderBlockForOut(buffer);
  // zxp.playHeaderOnly(headerBl,19);
  // // Cerramos el fichero
  // sdFile.close();


}



void loop() {

  // \games\Classic48\Trashman\TRASHMAN.TAP
  //sdf.ls("/", LS_R);
  
  buttonsControl();

  if (PLAY==true && LOADING_STATE == 0)
  {
      LOADING_STATE = 1;
      Serial.println("");
      Serial.println("Starting TAPE PLAYER.");
      Serial.println("");
      //playTAPfile("/games/Classic48/Trashman/TRASHMAN.TAP");
      playTAPfile("games/Classic128/Shovel Adventure/Shovel Adventure ZX 1.2.tap");
      
  }
  //delay(50);
  //playTAPfile("/games/Classic48/Trashman/TRASHMAN.TAP");
  //sleep(30);

}