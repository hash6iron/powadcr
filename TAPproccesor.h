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

<<<<<<< Updated upstream
      byte* resize(byte* buffer, size_t items, size_t newCapacity)
      {
        size_t count = items;
        byte* newBuffer = new byte[newCapacity];
        memmove(newBuffer, buffer, count * sizeof(byte));
        buffer = newBuffer;

        return buffer;
=======
     bool isHeaderTAP(File32 tapFileName)
     {
        if (_mFile != 0)
        {

              Serial.println("");
              Serial.println("");
              Serial.println("Begin isHeaderTAP");

              // La cabecera son 10 bytes
              byte* bBlock = (byte*)calloc(19+1,sizeof(byte));
              bBlock = _sdm.readFileRange32(tapFileName,0,19,true);

              Serial.println("");
              Serial.println("");
              Serial.println("Got bBlock");

              // Obtenemos la firma del TZX
              char* signTZXHeader = (char*)calloc(3+1,sizeof(char));
              signTZXHeader = &INITCHAR[0];

              Serial.println("");
              Serial.println("");
              Serial.println("Initialized signTAP Header");

              // Analizamos la cabecera
              // Extraemos el nombre del programa
              for (int n=0;n<3;n++)
              {   
                  signTZXHeader[n] = (byte)bBlock[n];
                  
                  Serial.println("");
                  Serial.println("");
                  Serial.println((int)signTZXHeader[n]);                  
              }

              if (signTZXHeader[0] == 19 && signTZXHeader[1] == 0 && signTZXHeader[2] == 0)
              {
                  Serial.println("");
                  Serial.println("");
                  Serial.println("is TAP ok");                
                  return true;
              }
              else
              {
                  return false;
              }
        }
        else
        { 
          return false;
        }
     }

      bool isFileTAP(File32 tapFileName)
      {
          char* szName = (char*)calloc(255,sizeof(char));
          tapFileName.getName(szName,254);
          
          String fileName = String(szName);

          if (fileName != "")
          {
              fileName.toUpperCase();
              if (fileName.indexOf("TAP") != -1)
              {
                  if (isHeaderTAP(tapFileName))
                  {
                    return true;
                  }
                  else
                  {
                    return false;
                  }
              }
              else
              {
                  return false;
              }
          }
          else
          {
              return false;
          }
>>>>>>> Stashed changes
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
          Serial.println("");
          Serial.println("Analyzing TAP file. Please wait ...");
          
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
                  Serial.println("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock));
              }

              // ¿Hemos llegado al ultimo byte
              if (startBlock > sizeTAP)                
              {
                  reachEndByte = true;
                  Serial.println("");
                  Serial.println("Success. End: ");
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
          Serial.println("");
          Serial.println("");
          Serial.println("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

          tBlockDescriptor* bDscr = (tBlockDescriptor*)malloc(tmpTAP.numBlocks);
          bDscr = tmpTAP.descriptor;
          int totalBlocks = tmpTAP.numBlocks;

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
                  Serial.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")");
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

      public:
      void showInfoBlockInProgress(int nBlock)
      {
          switch(nBlock)
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
<<<<<<< Updated upstream
          // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
          // entonces se analiza y se construye el buffer, etc para poder manejarlo.
          int totalBlocks = 0;
=======
          _sdf32 = sdf32;
      }

      void set_file(File32 mFile, int sizeTAP)
      {
          // Pasamos los parametros a la clase
          _mFile = mFile;
          _sizeTAP = sizeTAP;
      }

      tTAP get_tap()
      {
          // Devolvemos el descriptor del TAP
          return _myTAP;
      }

      char* get_tap_name()
      {
          // Devolvemos el nombre del TAP
          return _myTAP.name;
      }

      int get_tap_numBlocks()
      {
          // Devolvemos el numero de bloques del TAP
          return _myTAP.numBlocks;
      }

//      void set_rlen(int l)
//      {
//          _rlen = l;
//      }

      void set_HMI(HMI hmi)
      {
         _hmi = hmi;
      }
      
      void initialize()
      {
          if (_myTAP.descriptor != NULL)
          {
            free(_myTAP.descriptor);
            free(_myTAP.name);
            _myTAP.descriptor = NULL;
            _myTAP.name = &INITCHAR[0];
            _myTAP.numBlocks = 0;
            _myTAP.size = 0;
          }          
      }

      void proccess_tap()
      {
          // Procesamos el fichero
          Serial.println("");
          Serial.println("Getting total blocks...");

          if (isFileTAP(_mFile))
          {
              getBlockDescriptor(_mFile, _sizeTAP);
              showDescriptorTable();
              //myTAP = _myTAP;   
          }
      }

      void getInfoFileTAP(char* path) 
      {
      
        LAST_MESSAGE = "Analyzing TAP file";
        _hmi.updateInformationMainPage();
      
        // Abrimos el fichero
        _mFile = _sdm.openFile32(_mFile, path);
        // Obtenemos su tamaño total
        _rlen = _mFile.available();
      
        // creamos un objeto TAPproccesor
        set_file(_mFile, _rlen);
        proccess_tap();
        
        Serial.println("");
        Serial.println("");
        Serial.println("END PROCCESING TAP: ");
      
        if (_myTAP.descriptor != NULL)
        {
            // Entregamos información por consola
            PROGRAM_NAME = get_tap_name();
            TOTAL_BLOCKS = get_tap_numBlocks();
            LAST_NAME = &INITCHAR2[0];
      
            Serial.println("");
            Serial.println("");
            Serial.println("PROGRAM_NAME: " + PROGRAM_NAME);
            Serial.println("TOTAL_BLOCKS: " + String(TOTAL_BLOCKS));
      
            // Pasamos el descriptor           
            _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

            _hmi.updateInformationMainPage();
        }
      }

      void play() 
      {

        //_hmi.writeString("");
        _hmi.writeString("READYst.val=0");

        //_hmi.writeString("");
        _hmi.writeString("ENDst.val=0");

        if (_myTAP.descriptor != NULL)
        {         
        
              // Inicializamos el buffer de reproducción. Memoria dinamica
              byte* bufferPlay = NULL;

              // Entregamos información por consola
              PROGRAM_NAME = _myTAP.name;
              TOTAL_BLOCKS = _myTAP.numBlocks;
              LAST_NAME = &INITCHAR2[0];

              // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
              int m = BLOCK_SELECTED;
              //BYTES_TOBE_LOAD = _rlen;

              // Reiniciamos
              if (BLOCK_SELECTED == 0) {
                BYTES_LOADED = 0;
                BYTES_TOBE_LOAD = _rlen;
                //_hmi.writeString("");
                _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED * 100) / (BYTES_TOBE_LOAD))));
              } else {
                BYTES_TOBE_LOAD = _rlen - _myTAP.descriptor[BLOCK_SELECTED - 1].offset;
              }

              for (int i = m; i < _myTAP.numBlocks; i++) {

                //LAST_NAME = bDscr[i].name;

                // Obtenemos el nombre del bloque
                LAST_NAME = _myTAP.descriptor[i].name;
                LAST_SIZE = _myTAP.descriptor[i].size;

                // Almacenmas el bloque en curso para un posible PAUSE
                if (LOADING_STATE != 2) {
                  CURRENT_BLOCK_IN_PROGRESS = i;
                  BLOCK_SELECTED = i;

                  //_hmi.writeString("");
                  _hmi.writeString("currentBlock.val=" + String(i + 1));

                  //_hmi.writeString("");
                  _hmi.writeString("progression.val=" + String(0));
                }

                //Paramos la reproducción.
                if (LOADING_STATE == 2) {
                  PAUSE = false;
                  STOP = false;
                  PLAY = false;
                  LOADING_STATE = 0;
                  break;
                }

                //Ahora vamos lanzando bloques dependiendo de su tipo
                //Esto actualiza el LAST_TYPE
                showInfoBlockInProgress(_myTAP.descriptor[i].type);

                // Actualizamos HMI
                _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

                _hmi.updateInformationMainPage();

                // Reproducimos el fichero
                if (_myTAP.descriptor[i].type == 0) {
                  
                  // CABECERAS
                  if(bufferPlay!=NULL)
                  {
                      free(bufferPlay);
                      bufferPlay=NULL;

                  }

                  bufferPlay = (byte*)calloc(_myTAP.descriptor[i].size, sizeof(byte));
                  bufferPlay = _sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);

                  // Llamamos a la clase de reproducción
                  _zxp.playHeaderProgram(bufferPlay, _myTAP.descriptor[i].size);

                } else if (_myTAP.descriptor[i].type == 1 || _myTAP.descriptor[i].type == 7) {
                  
                  // CABECERAS
                  if(bufferPlay!=NULL)
                  {
                      free(bufferPlay);
                      bufferPlay=NULL;
                  }      

                  bufferPlay = (byte*)calloc(_myTAP.descriptor[i].size, sizeof(byte));
                  bufferPlay = _sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);

                  _zxp.playHeader(bufferPlay, _myTAP.descriptor[i].size);
                } else {
                  // DATA
                  int blockSize = _myTAP.descriptor[i].size;

                  // Si el SPLIT esta activado y el bloque es mayor de 20KB hacemos Split.
                  if ((SPLIT_ENABLED) && (blockSize > SIZE_TO_ACTIVATE_SPLIT)) {

                    // Lanzamos dos bloques
                    int bl1 = blockSize / 2;
                    int bl2 = blockSize - bl1;
                    int blockPlaySize = 0;
                    int offsetPlay = 0;

                    //Serial.println("   > Splitted block. Size [" + String(blockSize) + "]");

                    for (int j = 0; j < 2; j++) {
                      if (j == 0) {
                        blockPlaySize = bl1;
                        offsetPlay = _myTAP.descriptor[i].offset;

                        if(bufferPlay!=NULL)
                        {
                            free(bufferPlay);
                            bufferPlay=NULL;
                        }

                        bufferPlay = (byte*)calloc(blockPlaySize, sizeof(byte));

                        bufferPlay = _sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);
                        _zxp.playDataBegin(bufferPlay, blockPlaySize);
                        //free(bufferPlay);

                      } else {
                        blockPlaySize = bl2;
                        offsetPlay = offsetPlay + bl1;

                        if(bufferPlay!=NULL)
                        {
                            free(bufferPlay);
                            bufferPlay=NULL;
                        }

                        bufferPlay = (byte*)calloc(blockPlaySize, sizeof(byte));
                        bufferPlay = _sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);

                        _zxp.playDataEnd(bufferPlay, blockPlaySize);
                      }
                    }
                  } else {
                    // En el caso de NO USAR SPLIT o el bloque es menor de 20K

                    if(bufferPlay!=NULL)
                    {
                        free(bufferPlay);
                        bufferPlay=NULL;
                    }

                    bufferPlay = (byte*)calloc(_myTAP.descriptor[i].size, sizeof(byte));
                    bufferPlay = _sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);
                    
                    _zxp.playData(bufferPlay, _myTAP.descriptor[i].size);
                  }
                }
              }
>>>>>>> Stashed changes

          #ifdef LOG==3
              Serial.println("");
              Serial.println("Getting total blocks...");
          #endif

          myTAP = getBlockDescriptor(mFile, sizeTAP);

          showDescriptorTable(myTAP);    
      }      

      TAPproccesor()
      { }

};
