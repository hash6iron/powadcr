/*
 powadcr.ino
 Antonio Tamairón. 2023

 Descripción:
 Programa principal del firmware del powadcr - Powa Digital Cassette Recording

 Version: 0.1

 */

#include "config.h"
#include "AudioKitHAL.h"
#include "ZXProccesor.h"
#include "SDmanager.h"
#include "TAPproccesor.h"
#include "test.h"

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

void setup() 
{
  // Configuramos el nivel de log
  //LOGLEVEL_AUDIOKIT = AudioKitError; 
  Serial.begin(115200);
  
  Serial.println("Setting Audiokit.");
  
  // Configuramos el ESP32kit
  LOGLEVEL_AUDIOKIT = AudioKitError; 
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


  // *****************************************
   #if TEST==1
     test();
   #endif
  // *****************************************
  // Listamos el directorio
  sdf.ls("/games/Classic48/Trashman/",LS_R);

  // Abrimos el fichero
  char *path="/games/Classic48/Trashman/TRASHMAN.TAP";
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
  sleep(10);

  Serial.println("Num. blocks: " + String(pTAP.myTAP.numBlocks));
  
  // Ahora reproducimos todo
  for (int i=0;i<pTAP.myTAP.numBlocks;i++)
  {
      //Ahora vamos lanzando bloques
      switch(pTAP.bDscr[i].type)
      {
          case 0:

              // Definimos el buffer del PLAYER igual al tamaño del bloque
              #ifdef LOG==3
                Serial.println("");
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
          bufferPlay = new byte[pTAP.bDscr[i].size];
          bufferPlay = readFileRange32(sdFile32,pTAP.bDscr[i].offset,pTAP.bDscr[i].size);

          zxp.playData(bufferPlay, pTAP.bDscr[i].size);
      }
  } 
  sleep(3);

  Serial.println("");
  Serial.println("Finish. STOP THE TAPE.");
  
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
  



  
}