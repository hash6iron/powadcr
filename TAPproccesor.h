#include "SdFat.h"


class TAPproccesor
{
    public:

      // TYPE block
      #define HPRGM 0                     //Cabecera PROGRAM
      #define HCODE 1                     //Cabecera BYTE  
      #define HSCRN 7                     //Cabecera BYTE SCREEN$
      #define HARRAYNUM 5                 //Cabecera ARRAY numerico
      #define HARRAYCHR 6                 //Cabecera ARRAY char
      #define PRGM 2                      //Bloque de datos BASIC
      #define SCRN 3                      //Bloque de datos Screen$
      #define CODE 4                      //Bloque de datps CM (BYTE)

      // Definición de estructuras
      // estructura tipo Bloque
      // struct tBlock
      // {
      //     int index;                      // Numero del bloque
      //     int offset = 0;                 // Byte donde empieza
      //     byte* header;                   // Cabecera del bloque
      //     byte* data;                     // Datos del bloque
      // };

      // struct tBlockDescriptor
      // {
      //     int offset;
      //     int size;
      //     int chk;
      //     String name;
      //     bool nameDetected;
      //     bool header;
      //     bool screen;
      //     int type;
      // };

      // // Estructura tipo TAP
      // struct tTAP
      // {
      //     String name;                    // Nombre del TAP
      //     int size;                       // Tamaño
      //     int numBlocks;                  // Numero de bloques
      //     tBlockDescriptor* descriptor;   // Descriptor
      // };

      SdFat fFile;
      SdFat32 fFile32;

      // Definicion de un TAP
      tTAP myTAP;

      int CURRENT_LOADING_BLOCK = 0;

      // Creamos el contenedor de bloques. Descriptor de bloques
      //tBlockDescriptor* bDscr = new tBlockDescriptor[255];

      byte* resize(byte* buffer, size_t items, size_t newCapacity)
      {
        size_t count = items;
        byte* newBuffer = new byte[newCapacity];
        memmove(newBuffer, buffer, count * sizeof(byte));
        buffer = newBuffer;

        return buffer;
      }

      byte calculateChecksum(byte* bBlock, int startByte, int numBytes)
      {
          // Calculamos el checksum de un bloque de bytes
          byte newChk = 0;

          #if LOG>3
            warningLog("Block len: ");
            warningLog(sizeof(bBlock)/sizeof(byte*));
          #endif

          // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
          
          for (int n=startByte;n<(startByte+numBytes);n++)
          {
              newChk = newChk ^ bBlock[n];
          }

          #if LOG>3
            warningLog("Checksum: " + String(newChk));
          #endif

          return newChk;
      }

      // void clearBlockDescriptor()
      // {
      //     for (int n=0;n<255;n++)
      //     {
      //         bDscr[n].offset = 0;
      //         bDscr[n].size = 0;
      //         bDscr[n].chk = 0;
      //         bDscr[n].header = false;
      //         bDscr[n].screen = false;
      //         bDscr[n].type = -1;
      //         // Esta parte es muy importante
      //         bDscr[n].name = new char[10];
      //     }
      // }

      bool isCorrectHeader(byte* header, int startByte)
      {
          // Verifica que es una cabecera
          bool isHeader = false;
          byte checksum = 0;
          byte calcBlockChk = 0;

          if (header[startByte]==19 && header[startByte+1]==0 && header[startByte+2]==0)
          {
              checksum = header[startByte+20];
              calcBlockChk = calculateChecksum(header,startByte,18);

              if (checksum == calcBlockChk)
              {
                  isHeader = true;
              }
          }

          return isHeader;
      }

      bool isProgramHeader(byte* header, int startByte)
      {
          // Verifica que es una cabecera
          //bool isHeaderPrg = false;
          byte checksum = 0;
          byte calcBlockChk = 0;

          if (isCorrectHeader(header, startByte))
          {
              if (header[startByte+3]==0)
              {
                  checksum = header[startByte+20];
                  calcBlockChk = calculateChecksum(header,startByte,18);

                  if (checksum == calcBlockChk)
                  {
                      //isHeaderPrg = true;
                      return true;
                  }
              }
              else
              //{isHeaderPrg = false;}
              {
                return false;
              }
          }

          //return isHeaderPrg;
          return false;
      }

      int getBlockLen(byte* header, int startByte)
      {
          // Este procedimiento nos devuelve el tamaño definido un bloque de datos
          // analizando los 22 bytes de la cabecera pasada por valor.
          int blockLen = 0;
          if (isCorrectHeader(header,startByte))
          {
              blockLen = (256*header[startByte+22]) + header[startByte+21];
          }
          return blockLen;
      }


      byte* getHeaderBlockForOut(byte* bBlock)
      {
          // Este procedimiento obtiene la parte de la cabacera de un bloque de bytes
          // obtenido con getOneByteBlock, pero listo para sacar por la salida de audio.
          byte* headerBlkTmp = new byte[19];
          
          int i=0;
          // NOTA: Se elimina de la cabecera el 0x13,0x00, para la reproducción (los dos primeros bytes)
          for (int n=2;n<21;n++)
          {
              headerBlkTmp[i]=bBlock[n];
              i++;
          }
          return headerBlkTmp;  
      }
      
      byte* getHeaderBlock(byte* bBlock, int offset)
      {
          // Este procedimiento obtiene la parte de la cabacera de un bloque de bytes
          // obtenido con getOneByteBlock
          byte* headerBlkTmp = new byte[21];
          
          for (int n=offset;n<(offset + 22);n++)
          {
              headerBlkTmp[n]=bBlock[n];
          }
          return headerBlkTmp;
      }

      char* getNameFromHeader(byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          char* prgName = new char[10];

          if (isCorrectHeader(header,startByte))
          {
              // Es una cabecera PROGRAM 
              // el nombre está en los bytes del 4 al 13 (empezando en 0)
              #ifdef LOG==3
                warningLog("Name detected ");
              #endif

              // Extraemos el nombre del programa
              for (int n=4;n<14;n++)
              {   
                  prgName[n-4] = (char)header[n];
              }
          }
          else
          {
              prgName = "";
          }

          // Pasamos la cadena de caracteres
          return prgName;

      }

      char* getBlockName(byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          char* prgName = new char[10];

          // if (isCorrectHeader(header,startByte))
          // {
              // Es una cabecera PROGRAM 
              // el nombre está en los bytes del 4 al 13 (empezando en 0)
              #if LOG>3
                warningLog("Name detected ");
              #endif

              int i = 0;
              // Extraemos el nombre del programa. 
              // Contando con que la cabecera no tiene 0x13 0x00 (el tamaño)
              for (int n=startByte+2;n<(startByte+12);n++)
              {   
                  prgName[i] = (char)header[n];
                  
                  #if LOG>3
                      warningLog(prgName[i]);
                  #endif
                  
                  i++;
              }

          // Pasamos la cadena de caracteres
          return prgName;

      }

      byte* getBlockRange(byte* bBlock, int byteStart, int byteEnd)
      {

          // Extraemos un bloque de bytes indicando el byte inicio y final
          byte* blockExtracted = new byte[(byteEnd - byteStart)+1];

          int i=0;
          for (int n=byteStart;n<(byteStart + (byteEnd - byteStart)+1);n++)
          {
              blockExtracted[i] = bBlock[n];
          }

          return blockExtracted;
      }

      tTAP getBlockDescriptor(File32 mFile, int sizeTAP)
      {
          // Para ello tenemos que ir leyendo el TAP poco a poco
          // y llegando a los bytes que indican TAMAÑO(2 bytes) + 0xFF(1 byte)
          tBlockDescriptor* bDscr = (tBlockDescriptor*)malloc(255);
          tTAP describedTAP;
          String nameTAP = "";

          int startBlock = 0;
          int lastStartBlock = 0;
          int sizeB = 0;
          int newSizeB = 0;
          int chk = 0;
          int blockChk = 0;
          int numBlocks = 0;
          int offset = 0;
          char* blockName = new char[10];
          bool blockNameDetected = false;
          
          bool reachEndByte = false;

          int state = 0;

          //byte* fileTAP = NULL;

          //Serial.println("");
          //Serial.println("Cleaning block descriptor table ...");


          //clearBlockDescriptor();

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          infoLog("Analyzing TAP file. Please wait ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*readFileRange32(mFile,startBlock+1,1,false)[0]) + readFileRange32(mFile,startBlock,1,false)[0];
          startBlock = 2;

          while(reachEndByte==false)
          {

              blockNameDetected = false;
              
              chk = calculateChecksum(readFileRange32(mFile,startBlock,sizeB-1,false),0,sizeB-1);
              
              // #if LOG>3
              //   Serial.println("");
              //   Serial.print("[" + String (numBlocks) + "] - ");
              //   Serial.print("*Chk calc: ");
              //   Serial.print(chk,HEX);
              //   Serial.println("");
              // #endif

              blockChk = readFileRange32(mFile,startBlock+sizeB-1,1,false)[0];

              // #if LOG>3
              //   Serial.print(" / Chk block: ");
              //   Serial.print(blockChk,HEX);
              //   Serial.println("");
              // #endif

              if (blockChk == chk)
              {
                  
                  bDscr[numBlocks].offset = startBlock;
                  bDscr[numBlocks].size = sizeB;
                  bDscr[numBlocks].chk = chk;

                  // Guardamos el ultimo offset
                  //lastStartBlock = startBlock;

                  // Cogemos info del bloque
                  
                  // Flagbyte
                  // 0x00 - HEADER
                  // 0xFF - DATA BLOCK
                  int flagByte = readFileRange32(mFile,startBlock,1,false)[0];

                  // 0x00 - PROGRAM
                  // 0x01 - ARRAY NUM
                  // 0x02 - ARRAY CHAR
                  // 0x03 - CODE FILE
                  int typeBlock = readFileRange32(mFile,startBlock+1,1,false)[0];
                  // #if LOG>3
                  //   Serial.print(" - Flag: " + String(flagByte));
                  //   Serial.print(" - Type: " + String(typeBlock));
                  // #endif

                  // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                  if (flagByte == 0)
                  {

                      // Es una CABECERA
                      if (typeBlock==0)
                      {
                          // Es un header PROGRAM
                          bDscr[numBlocks].header = true;
                          bDscr[numBlocks].type = HPRGM;
                          blockNameDetected = true;
                          // Almacenamos el nombre
                          bDscr[numBlocks].name = (String(getBlockName(readFileRange32(mFile,startBlock,19,false),0))).substring(0,10);

                          //Cogemos el nombre del TAP de la primera cabecera
                          if (startBlock < 23)
                          {
                              nameTAP = (String)bDscr[numBlocks].name;
                          }

                          state = 1;
                      }
                      else if (typeBlock==1)
                      {
                          // Array num header
                      }
                      else if (typeBlock==2)
                      {
                          // Array char header

                      }
                      else if (typeBlock==3)
                      {
                          // Byte block

                          // Vemos si es una cabecera de una SCREEN
                          // para ello temporalmente vemos el tamaño del bloque de codigo. Si es 6914 bytes (incluido el checksum y el flag - 6912 + 2 bytes)
                          // es una pantalla SCREEN
                          blockNameDetected = true;
                          bDscr[numBlocks].name = (String(getBlockName(readFileRange32(mFile,startBlock,19,false),0))).substring(0,10);

                          int tmpSizeBlock = (256*readFileRange32(mFile,startBlock + sizeB+1,1,false)[0]) + readFileRange32(mFile,startBlock + sizeB,1,false)[0];
                          
                          if (tmpSizeBlock == 6914)
                          {
                              // Es una cabecera de un Screen
                              bDscr[numBlocks].screen = true;
                              bDscr[numBlocks].type = HSCRN;
                              state = 2;
                          }
                          else
                          {
                              bDscr[numBlocks].type = HCODE;                         
                          }
                   
                      }

                  }
                  else
                  {
                      if (state == 1)
                      {
                          state = 0;
                          // Es un bloque BASIC
                          bDscr[numBlocks].type = PRGM;                         
                      }
                      else if (state == 2)
                      {
                          state = 0;
                          // Es un bloque SCREEN
                          bDscr[numBlocks].type = SCRN;                         
                      }
                      else
                      {
                          // Es un bloque CM
                          bDscr[numBlocks].type = CODE;                         
                      }
                  }

                  // #if LOG>0
                  //     Serial.println("");
                  //     Serial.print("[" + String (numBlocks) + "] - offset: " + String(startBlock));
                  //     Serial.print(" - Size: " + String(sizeB) + "  ");   
                  // #endif

                  if (blockNameDetected)
                  {
                      bDscr[numBlocks].nameDetected = true;                                      
                  }
                  else
                  {
                      bDscr[numBlocks].nameDetected = false;
                  }

                  // Siguiente bloque
                  // Direcion de inicio (offset)
                  startBlock = startBlock + sizeB;
                  // Tamaño
                  newSizeB = (256*readFileRange32(mFile,startBlock+1,1,false)[0]) + readFileRange32(mFile,startBlock,1,false)[0];
  
                  numBlocks++;
                  sizeB = newSizeB;
                  startBlock = startBlock + 2;
              }
              else
              {
                  reachEndByte = true;
                  errorLog(("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock)).c_str());
              }

              // ¿Hemos llegado al ultimo byte
              if (startBlock > sizeTAP)                
              {
                  reachEndByte = true;
                  infoLog("Success. End: ");
              }

          }

          // Componemos el TAP para devolverlo
          describedTAP.name = nameTAP;
          describedTAP.descriptor = bDscr;
          describedTAP.size = sizeTAP;
          describedTAP.numBlocks = numBlocks;

          return describedTAP;

      }

      // tTAP getBlockDescriptor(byte* fileTAP, int sizeTAP)
      // {
      //     // Para ello tenemos que ir leyendo el TAP poco a poco
      //     // y llegando a los bytes que indican TAMAÑO(2 bytes) + 0xFF(1 byte)
      //     int startBlock = 0;
      //     int lastStartBlock = 0;
      //     int sizeB = 0;
      //     int newSizeB = 0;
      //     int chk = 0;
      //     int blockChk = 0;
      //     int numBlocks = 0;
      //     int offset = 0;
      //     char* blockName = new char[10];
      //     bool blockNameDetected = false;
          
      //     bool reachEndByte = false;

      //     int state = 0;

      //     Serial.println("");
      //     Serial.println("Cleaning block descriptor table ...");


      //     clearBlockDescriptor();

      //     //Entonces recorremos el TAP. 
      //     // La primera cabecera SIEMPRE debe darse.
      //     Serial.println("");
      //     Serial.println("Starting block counting ...");
          
      //     // Los dos primeros bytes son el tamaño a contar
      //     sizeB = (256*fileTAP[startBlock+1]) + fileTAP[startBlock];
      //     startBlock = 2;

      //     while(reachEndByte==false)
      //     {

      //         blockNameDetected = false;
      //         chk = calculateChecksum(fileTAP,startBlock,sizeB-1);
              
      //         // #if LOG>3
      //         //   Serial.println("");
      //         //   Serial.print("[" + String (numBlocks) + "] - ");
      //         //   Serial.print("*Chk calc: ");
      //         //   Serial.print(chk,HEX);
      //         //   Serial.println("");
      //         // #endif

      //         blockChk = fileTAP[startBlock+sizeB-1];

      //         // #if LOG>3
      //         //   Serial.print(" / Chk block: ");
      //         //   Serial.print(blockChk,HEX);
      //         //   Serial.println("");
      //         // #endif

      //         if (blockChk == chk)
      //         {
                  
      //             bDscr[numBlocks].offset = startBlock;
      //             bDscr[numBlocks].size = sizeB;
      //             bDscr[numBlocks].chk = chk;

      //             // Guardamos el ultimo offset
      //             //lastStartBlock = startBlock;

      //             // Cogemos info del bloque
                  
      //             // Flagbyte
      //             // 0x00 - HEADER
      //             // 0xFF - DATA BLOCK
      //             int flagByte = fileTAP[startBlock];

      //             // 0x00 - PROGRAM
      //             // 0x01 - ARRAY NUM
      //             // 0x02 - ARRAY CHAR
      //             // 0x03 - CODE FILE
      //             int typeBlock = fileTAP[startBlock+1];
      //             // #if LOG>3
      //             //   Serial.print(" - Flag: " + String(flagByte));
      //             //   Serial.print(" - Type: " + String(typeBlock));
      //             // #endif

      //             // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
      //             if (flagByte == 0)
      //             {

      //                 // Es una CABECERA
      //                 if (typeBlock==0)
      //                 {
      //                     // Es un header PROGRAM
      //                     bDscr[numBlocks].header = true;
      //                     bDscr[numBlocks].type = HPRGM;
      //                     blockNameDetected = true;
      //                     // Almacenamos el nombre
      //                     bDscr[numBlocks].name = (String(getBlockName(fileTAP,startBlock))).substring(0,9);
      //                     state = 1;
      //                 }
      //                 else if (typeBlock==1)
      //                 {
      //                     // Array num header
      //                 }
      //                 else if (typeBlock==2)
      //                 {
      //                     // Array char header

      //                 }
      //                 else if (typeBlock==3)
      //                 {
      //                     // Byte block

      //                     // Vemos si es una cabecera de una SCREEN
      //                     // para ello temporalmente vemos el tamaño del bloque de codigo. Si es 6914 bytes (incluido el checksum y el flag - 6912 + 2 bytes)
      //                     // es una pantalla SCREEN
      //                     blockNameDetected = true;
      //                     bDscr[numBlocks].name = (String(getBlockName(fileTAP,startBlock))).substring(0,9);

      //                     int tmpSizeBlock = (256*fileTAP[startBlock + sizeB+1]) + fileTAP[startBlock + sizeB];
                          
      //                     if (tmpSizeBlock == 6914)
      //                     {
      //                         // Es una cabecera de un Screen
      //                         bDscr[numBlocks].screen = true;
      //                         bDscr[numBlocks].type = HSCRN;
      //                         state = 2;
      //                     }
      //                     else
      //                     {
      //                         bDscr[numBlocks].type = HCODE;                         
      //                     }
                   
      //                 }

      //             }
      //             else
      //             {
      //                 if (state == 1)
      //                 {
      //                     state = 0;
      //                     // Es un bloque BASIC
      //                     bDscr[numBlocks].type = PRGM;                         
      //                 }
      //                 else if (state == 2)
      //                 {
      //                     state = 0;
      //                     // Es un bloque SCREEN
      //                     bDscr[numBlocks].type = SCRN;                         
      //                 }
      //                 else
      //                 {
      //                     // Es un bloque CM
      //                     bDscr[numBlocks].type = CODE;                         
      //                 }
      //             }

      //             // #if LOG>0
      //             //     Serial.println("");
      //             //     Serial.print("[" + String (numBlocks) + "] - offset: " + String(startBlock));
      //             //     Serial.print(" - Size: " + String(sizeB) + "  ");   
      //             // #endif

      //             if (blockNameDetected)
      //             {
      //                 bDscr[numBlocks].nameDetected = true;                                      
      //             }
      //             else
      //             {
      //                 bDscr[numBlocks].nameDetected = false;
      //             }

      //             // Siguiente bloque
      //             // Direcion de inicio (offset)
      //             startBlock = startBlock + sizeB;
      //             // Tamaño
      //             newSizeB = (256*fileTAP[startBlock+1]) + fileTAP[startBlock];
  
      //             numBlocks++;
      //             sizeB = newSizeB;
      //             startBlock = startBlock + 2;
      //         }
      //         else
      //         {
      //             reachEndByte = true;
      //             Serial.println("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock));
      //         }

      //         // ¿Hemos llegado al ultimo byte
      //         if (startBlock > sizeTAP)                
      //         {
      //             reachEndByte = true;
      //             Serial.println("");
      //             Serial.println("Success. End: ");
      //         }

      //     }

      //     return numBlocks;
      // }

      void showDescriptorTable(tTAP tmpTAP)
      {
          debugLog("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

          tBlockDescriptor* bDscr = (tBlockDescriptor*)malloc(tmpTAP.numBlocks);
          bDscr = tmpTAP.descriptor;
          int totalBlocks = tmpTAP.numBlocks;

          for (int n=0;n<totalBlocks;n++)
          {
              debugLog(("[" + String(n) + "]" + " - Offset: " + String(bDscr[n].offset) + " - Size: " + String(bDscr[n].size)).c_str());
              String strType = "";
              switch(bDscr[n].type)
              {
                  case 0: 
                    strType = "PROGRAM - HEADER";
                  break;

                  case 1:
                    strType = "BYTE - HEADER";
                  break;

                  case 7:
                    strType = "BYTE <SCREEN> - HEADER";
                  break;

                  case 2:
                    strType = "BASIC DATA";
                  break;

                  case 3:
                    strType = "SCREEN DATA";
                  break;

                  case 4:
                    strType = "BYTE DATA";
                  break;

                  default:
                    strType="Standard data";
              }

              if (bDscr[n].nameDetected)
              {
                  debugLog(("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")").c_str());
              }
              else
              { 
                  debugLog(("[" + String(n) + "] - " + strType + " ").c_str());
              }
          }      
      }

      int getTotalHeaders(byte* fileTAP, int sizeTAP)
      {
          // Este procedimiento devuelve el total de bloques que contiene el fichero
          int nblocks = 0;
          byte* bBlock = new byte[sizeTAP];
          bBlock = fileTAP; 
          
          // Para ello buscamos la secuencia "0x13 0x00 0x00"
          for (int n=0;n<sizeTAP;n++)
          {
              if (bBlock[n] == 19)
              {
                  if ((n+1 < sizeTAP) && (bBlock[n+1] == 0))
                  {
                      if ((n+2 < sizeTAP) && (bBlock[n+2] == 0))
                      {
                          nblocks++;
                          n = n + 3;
                      }
                  }
              }
          }

          return nblocks;
      }

      public:
      void showInfoBlockInProgress(int nBlock)
      {
          switch(nBlock)
          {
              case 0:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> PROGRAM HEADER");
                  #endif
                  break;

              case 1:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BYTE HEADER");
                  #endif
                  break;

              case 7:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> SCREEN HEADER");
                  #endif
                  break;

              case 2:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BASIC PROGRAM");
                  #endif
                  break;

              case 3:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> SCREEN");
                  #endif
                  break;

              case 4:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BYTE CODE");
                  #endif
                  break;
          }        
      }
      
      private:


      public:

      // TAPproccesor(byte* fileTAP, int sizeTAP)
      // {
      //     // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
      //     // entonces se analiza y se construye el buffer, etc para poder manejarlo.
      //     int totalBlocks = 0;

      //     #ifdef LOG==3
      //         Serial.println("");
      //         Serial.println("Getting total blocks...");
      //     #endif

      //     totalBlocks = getBlockDescriptor(fileTAP, sizeTAP);

      //     showDescriptorTable(totalBlocks);    
      //     myTAP.numBlocks = totalBlocks;     
      // }
      TAPproccesor(File32 mFile, int sizeTAP)
      {
          // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
          // entonces se analiza y se construye el buffer, etc para poder manejarlo.
          int totalBlocks = 0;

          #ifdef LOG==3
              warningLog("Getting total blocks...");
          #endif

          myTAP = getBlockDescriptor(mFile, sizeTAP);

          showDescriptorTable(myTAP);    
      }      

      TAPproccesor()
      { }

};
