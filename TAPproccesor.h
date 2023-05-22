#include "SdFat.h"


class TAPproccesor
{
    public:

      // TYPE block
      #define HPRGM 0
      #define HCODE 1
      #define HSCRN 7
      #define HARRAYNUM 5
      #define HARRAYCHR 6
      #define PRGM 2
      #define SCRN 3
      #define CODE 4

      struct tBlock
      {
          int index;
          int offset = 0;
          byte* header;
          byte* data;
      };

      struct tTAP
      {
          char* nameTAP;
          int size;
          int numBlocks;
          tBlock* container;
      };

      struct tBlockDescriptor
      {
          int offset;
          int size;
          int chk;
          String name;
          bool nameDetected;
          bool header;
          bool screen;
          int type;
      };

      SdFat fFile;
      SdFat32 fFile32;

      // Definicion de un TAP
      tTAP myTAP;

      // Creamos el contenedor de bloques. Descriptor de bloques
      tBlockDescriptor* bDscr = new tBlockDescriptor[255];

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
            Serial.println("");
            Serial.println("Block len: ");
            Serial.print(sizeof(bBlock)/sizeof(byte*));
          #endif

          // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
          
          for (int n=startByte;n<(startByte+numBytes);n++)
          {
              newChk = newChk ^ bBlock[n];
          }

          #if LOG>3
            Serial.println("");
            Serial.println("Checksum: " + String(newChk));
          #endif

          return newChk;
      }

      void clearBlockDescriptor()
      {
          for (int n=0;n<255;n++)
          {
              bDscr[n].offset = 0;
              bDscr[n].size = 0;
              bDscr[n].chk = 0;
              bDscr[n].header = false;
              bDscr[n].screen = false;
              bDscr[n].type = -1;
              // Esta parte es muy importante
              bDscr[n].name = new char[10];
          }
      }

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
          bool isHeaderPrg = false;
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
                      isHeaderPrg = true;
                  }
              }
              else
              {isHeaderPrg = false;}
          }

          return isHeaderPrg;
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
                Serial.println("");
                Serial.println("Name detected ");
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
                Serial.println("");
                Serial.println("Name detected ");
              #endif

              int i = 0;
              // Extraemos el nombre del programa. 
              // Contando con que la cabecera no tiene 0x13 0x00 (el tamaño)
              for (int n=startByte+2;n<(startByte+12);n++)
              {   
                  prgName[i] = (char)header[n];
                  
                  #if LOG>3
                      Serial.print(prgName[i]);
                  #endif
                  
                  i++;
              }
          // }
          // else
          // {
          //     prgName = "";
          // }

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

      int getBlockDescriptor(byte* fileTAP, int sizeTAP)
      {
          // Para ello tenemos que ir leyendo el TAP poco a poco
          // y llegando a los bytes que indican TAMAÑO(2 bytes) + 0xFF(1 byte)
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

          Serial.println("");
          Serial.println("Cleaning block descriptor table ...");


          clearBlockDescriptor();

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          Serial.println("");
          Serial.println("Starting block counting ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*fileTAP[startBlock+1]) + fileTAP[startBlock];
          startBlock = 2;

          while(reachEndByte==false)
          {

              blockNameDetected = false;
              chk = calculateChecksum(fileTAP,startBlock,sizeB-1);
              
              #if LOG>3
                Serial.println("");
                Serial.print("[" + String (numBlocks) + "] - ");
                Serial.print("*Chk calc: ");
                Serial.print(chk,HEX);
                Serial.println("");
              #endif

              blockChk = fileTAP[startBlock+sizeB-1];

              #if LOG>3
                Serial.print(" / Chk block: ");
                Serial.print(blockChk,HEX);
                Serial.println("");
              #endif

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
                  int flagByte = fileTAP[startBlock];

                  // 0x00 - PROGRAM
                  // 0x01 - ARRAY NUM
                  // 0x02 - ARRAY CHAR
                  // 0x03 - CODE FILE
                  int typeBlock = fileTAP[startBlock+1];
                  #if LOG>3
                    Serial.print(" - Flag: " + String(flagByte));
                    Serial.print(" - Type: " + String(typeBlock));
                  #endif

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
                          bDscr[numBlocks].name = (String(getBlockName(fileTAP,startBlock))).substring(0,9);
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
                          bDscr[numBlocks].name = (String(getBlockName(fileTAP,startBlock))).substring(0,9);

                          int tmpSizeBlock = (256*fileTAP[startBlock + sizeB+1]) + fileTAP[startBlock + sizeB];
                          
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

                  #if LOG>0
                      Serial.println("");
                      Serial.print("[" + String (numBlocks) + "] - offset: " + String(startBlock));
                      Serial.print(" - Size: " + String(sizeB) + "  ");   
                  #endif

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
                  newSizeB = (256*fileTAP[startBlock+1]) + fileTAP[startBlock];
  
                  numBlocks++;
                  sizeB = newSizeB;
                  startBlock = startBlock + 2;
              }
              else
              {
                  reachEndByte = true;
                  Serial.println("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock));
              }

              // ¿Hemos llegado al ultimo byte
              if (startBlock > sizeTAP)                
              {
                  reachEndByte = true;
                  Serial.println("");
                  Serial.println("End: ");
              }

          }

          return numBlocks;
      }

      void showDescriptorTable(int totalBlocks)
      {
          Serial.println("");
          Serial.println("");
          Serial.println("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");
          for (int n=0;n<totalBlocks;n++)
          {
              Serial.print("[" + String(n) + "]" + " - Offset: " + String(bDscr[n].offset) + " - Size: " + String(bDscr[n].size));
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
                  Serial.println("");
                  Serial.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,9) + " - (" + strType + ")");
              }
              else
              { 
                  Serial.println("");
                  Serial.print("[" + String(n) + "] - " + strType + " ");
              }

              Serial.println("");
              Serial.println("");

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

    private:
      // void analyzeTAPfile(byte* fileTAP, int sizeTAP)
      // {
      //     // Analizamos el TAP
      //     //1. Obtenemos el numero de bloques
      //     //2. Extraemos cabeceras y bloques
      //     //3. Finalizamos
      //     int offset = 0;
      //     int offsetInitial = 0;
      //     int bLen = 0;
      //     byte* header = new byte[21];
      //     byte* data = new byte[0];

      //     #ifdef LOG==3
      //         Serial.println("");
      //         Serial.println("Analyzen file - header and data...");
      //     #endif

      //     for (int n=0;n<sizeTAP;n++)
      //     {
      //         header = getHeaderBlock(fileTAP, offset);
      //         if (isCorrectHeader(header,0))
      //         {
      //           // Cogemos la longitud del bloque de datos
      //           // esta información nos dará el offset desde el flag FF
      //           // para la siguiente cabecera
      //           #ifdef LOG==3
      //               Serial.print("Header is correct...");
      //           #endif
                
      //           bLen = getBlockLen(header,0);
                
      //           // aumentamos el total de bloques
      //           myTAP.numBlocks++;
                
      //           // Si es el primer bloque
      //           if (offset == 0)
      //           {
      //               // Si es la primera cabecera y es PROGRAM
      //               if (isProgramHeader(header))
      //               {
      //                   myTAP.nameTAP = getNameFromHeader(header);
      //                   myTAP.size = sizeTAP;
      //               }
      //           }
      //         }
      //         break;

      //     //       // Cogemos ahora el bloque de datos.
      //     //       // la cabecera en un bloque TAP completo acaba con CHECKSUM 
      //     //       // Después se indica el TAMAÑO DEL BLOQUE DE DATOS + 0xFF
      //     //       // el bloque de datos empieza después del 0xFF
      //     //       // entonces, son 22 bytes, el 23 ya es el bloque
                
      //     //       offsetInitial = offset;
      //     //       offset = offset + 23;

      //     //       // cogemos el bloque de datos
      //     //       data = new byte[bLen];
      //     //       data = getBlockRange(fileTAP,offset,offset+bLen);

      //     //       // offset para la nueva cabecera
      //     //       offset = offset + bLen + 1;

      //     //       // Almacenamos la informacion que tenemos
      //     //       myTAP.container[myTAP.numBlocks-1].header = header;
      //     //       myTAP.container[myTAP.numBlocks-1].data = data;
      //     //       myTAP.container[myTAP.numBlocks-1].offset = offsetInitial;
      //     //       myTAP.container[myTAP.numBlocks-1].index = myTAP.numBlocks;
      //     //     }             
      //     }
      // }

      public:

      TAPproccesor(byte* fileTAP, int sizeTAP)
      {
          // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
          // entonces se analiza y se construye el buffer, etc para poder manejarlo.
          int totalBlocks = 0;

          #ifdef LOG==3
              Serial.println("");
              Serial.println("Getting total blocks...");
          #endif

          totalBlocks = getBlockDescriptor(fileTAP, sizeTAP);

          showDescriptorTable(totalBlocks);         
      }

      TAPproccesor()
      { }

};