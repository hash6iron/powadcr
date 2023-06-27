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

      SdFat fFile;
      SdFat32 fFile32;

      // Definicion de un TAP
      tTAP myTAP;

      int CURRENT_LOADING_BLOCK = 0;

      // Creamos el contenedor de bloques. Descriptor de bloques
      //tBlockDescriptor* bDscr = new tBlockDescriptor[255];

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

      char* getTypeTAPBlock(int nBlock)
      {
          char* typeName = (char*)calloc(10,sizeof(char));
          typeName = "\0";

          switch(nBlock)
          {
              case 0:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  typeName = (char*)"PROGRAM";
                  break;

              case 1:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  typeName = (char*)"BYTE.H";
                  break;

              case 7:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  typeName = (char*)"SCREEN.H";
                  break;

              case 2:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  typeName = (char*)"BASIC";
                  break;

              case 3:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  typeName = (char*)"SCREEN";
                  break;

              case 4:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  if (LAST_SIZE != 6914)
                  {
                      typeName = (char*)"BYTE";
                  }
                  else
                  {
                      typeName = (char*)"SCREEN";
                  }
                  break;
          }        

          return typeName;
      }
      char* getNameFromHeader(byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          char* prgName = (char*)calloc(10+1,sizeof(char));

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

      void getBlockName(char** prgName, byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          // Es una cabecera PROGRAM 
          // el nombre está en los bytes del 4 al 13 (empezando en 0)         
          header[startByte+12]=0;
          *prgName = (char*) (header+2);             
      }

      byte* getBlockRange(byte* bBlock, int byteStart, int byteEnd)
      {

          // Extraemos un bloque de bytes indicando el byte inicio y final
          //byte* blockExtracted = new byte[(byteEnd - byteStart)+1];
          byte* blockExtracted = (byte*)calloc((byteEnd - byteStart)+1, sizeof(byte));

          int i=0;
          for (int n=byteStart;n<(byteStart + (byteEnd - byteStart)+1);n++)
          {
              blockExtracted[i] = bBlock[n];
          }

          return blockExtracted;
      }

      int getNumBlocks(File32 mFile, int sizeTAP)
      {

          int startBlock = 0;
          int lastStartBlock = 0;
          int sizeB = 0;
          int newSizeB = 0;
          int chk = 0;
          int blockChk = 0;
          int numBlocks = 0;
          char* blockName = (char*)calloc(10+1,sizeof(char));
          blockName = "\0";

          bool blockNameDetected = false;
          
          bool reachEndByte = false;

          int state = 0;    

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          Serial.println("");
          Serial.println("Analyzing TAP file. Please wait ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*readFileRange32(mFile,startBlock+1,1,false)[0]) + readFileRange32(mFile,startBlock,1,false)[0];
          startBlock = 2;

          while(reachEndByte==false)
          {

              //Serial.println("START LOOP AGAIN: ");  
              byte* tmpRng = (byte*)calloc(sizeB,sizeof(byte));
              tmpRng = readFileRange32(mFile,startBlock,sizeB-1,false);
              chk = calculateChecksum(tmpRng,0,sizeB-1);
              free(tmpRng);
              //Serial.println("MARK CHK: ");
              
              blockChk = readFileRange32(mFile,startBlock+sizeB-1,1,false)[0];
              //Serial.println("MARK BLOCK CHK: ");            

              if (blockChk == chk)
              {
                  
                  int flagByte = readFileRange32(mFile,startBlock,1,false)[0];

                  int typeBlock = readFileRange32(mFile,startBlock+1,1,false)[0];
                  
                  // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                  if (flagByte == 0)
                  {
                     
                      // Es una CABECERA
                      if (typeBlock==0)
                      {

                          blockNameDetected = true;
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

                          int tmpSizeBlock = (256*readFileRange32(mFile,startBlock + sizeB+1,1,false)[0]) + readFileRange32(mFile,startBlock + sizeB,1,false)[0];

                          if (tmpSizeBlock == 6914)
                          {
                              state = 2;
                          }                   
                      }

                  }
                  else
                  {
                      if (state == 1)
                      {
                          state = 0;
                      }
                      else if (state == 2)
                      {
                          state = 0;
                      }
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
                  //break;
                  Serial.println("");
                  Serial.println("Success. End: ");
              }

          }

          return numBlocks;

      }
      void getBlockDescriptor(File32 mFile, int sizeTAP)
      {
          // Para ello tenemos que ir leyendo el TAP poco a poco
          // y llegando a los bytes que indican TAMAÑO(2 bytes) + 0xFF(1 byte)
          int numBlks = getNumBlocks(mFile, sizeTAP);
         
          
          Serial.println("");
          Serial.println("Num. de bloques: " + String(numBlks));
          Serial.println("");

          //tBlockDescriptor* bDscr = (tBlockDescriptor*)calloc(numBlks,sizeof(tBlockDescriptor));
          globalTAP.descriptor = (tBlockDescriptor*)calloc(numBlks,sizeof(tBlockDescriptor));
          // Guardamos el numero total de bloques
          //globalTAP.numBlocks = numBlks;
          
          char* nameTAP = (char*)calloc(10+1,sizeof(char));
          nameTAP = "\0";

          char* typeName = (char*)calloc(10,sizeof(char));
          typeName = "\0";

          int startBlock = 0;
          int lastStartBlock = 0;
          int sizeB = 0;
          int newSizeB = 0;
          int chk = 0;
          int blockChk = 0;
          int numBlocks = 0;
          char* blockName = (char*)calloc(10+1,sizeof(char));
          blockName = "\0";

          bool blockNameDetected = false;
          
          bool reachEndByte = false;

          int state = 0;    

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          Serial.println("");
          Serial.println("Analyzing TAP file. Please wait ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*readFileRange32(mFile,startBlock+1,1,false)[0]) + readFileRange32(mFile,startBlock,1,false)[0];
          startBlock = 2;

          while(reachEndByte==false)
          {

              // Inicializamos
              blockName = "\0";
              //bDscr[numBlocks].name = blockName;
              globalTAP.descriptor[numBlocks].name = blockName;

              blockNameDetected = false;
              
              //Serial.println("START LOOP AGAIN: ");  
              byte* tmpRng = (byte*)calloc(sizeB,sizeof(byte));
              tmpRng = readFileRange32(mFile,startBlock,sizeB-1,false);
              chk = calculateChecksum(tmpRng,0,sizeB-1);
              free(tmpRng);
              //Serial.println("MARK CHK: ");
              
              blockChk = readFileRange32(mFile,startBlock+sizeB-1,1,false)[0];
              //Serial.println("MARK BLOCK CHK: ");            

              if (blockChk == chk)
              {
                  
                  // bDscr[numBlocks].offset = startBlock;
                  // bDscr[numBlocks].size = sizeB;
                  // bDscr[numBlocks].chk = chk;

                  globalTAP.descriptor[numBlocks].offset = startBlock;
                  globalTAP.descriptor[numBlocks].size = sizeB;
                  globalTAP.descriptor[numBlocks].chk = chk;


                  //Serial.println("SAVE 1: ");            

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
                  
                  // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                  if (flagByte == 0)
                  {

                      // Inicializamos
                      // bDscr[numBlocks].type = 0;
                      // bDscr[numBlocks].typeName = typeName;
                      
                      globalTAP.descriptor[numBlocks].type = 0;
                      globalTAP.descriptor[numBlocks].typeName = typeName;
                      
                      // Es una CABECERA
                      if (typeBlock==0)
                      {
                          // Es un header PROGRAM
                          // bDscr[numBlocks].header = true;
                          // bDscr[numBlocks].type = HPRGM;

                          globalTAP.descriptor[numBlocks].header = true;
                          globalTAP.descriptor[numBlocks].type = HPRGM;

                          blockNameDetected = true;

                          // Almacenamos el nombre
                          //getBlockName(&bDscr[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);
                          getBlockName(&globalTAP.descriptor[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);


                          //Cogemos el nombre del TAP de la primera cabecera
                          if (startBlock < 23)
                          {
                              //nameTAP = (String)bDscr[numBlocks].name;
                              //nameTAP = bDscr[numBlocks].name;
                              nameTAP = globalTAP.descriptor[numBlocks].name;
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
                          
                          // Almacenamos el nombre
                          //getBlockName(&bDscr[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);
                          getBlockName(&globalTAP.descriptor[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);                          

                          int tmpSizeBlock = (256*readFileRange32(mFile,startBlock + sizeB+1,1,false)[0]) + readFileRange32(mFile,startBlock + sizeB,1,false)[0];

                          if (tmpSizeBlock == 6914)
                          {
                              // Es una cabecera de un Screen
                              // bDscr[numBlocks].screen = true;
                              // bDscr[numBlocks].type = HSCRN;
                              
                              globalTAP.descriptor[numBlocks].screen = true;
                              globalTAP.descriptor[numBlocks].type = HSCRN;                              

                              state = 2;
                          }
                          else
                          {
                              //bDscr[numBlocks].type = HCODE;                       
                              globalTAP.descriptor[numBlocks].type = HCODE;    
                          }
                   
                      }

                  }
                  else
                  {
                      if (state == 1)
                      {
                          state = 0;
                          // Es un bloque BASIC
                          //bDscr[numBlocks].type = PRGM;                         
                          globalTAP.descriptor[numBlocks].type = PRGM;
                      }
                      else if (state == 2)
                      {
                          state = 0;
                          // Es un bloque SCREEN
                          //bDscr[numBlocks].type = SCRN;                         
                          globalTAP.descriptor[numBlocks].type = SCRN;                         
                      }
                      else
                      {
                          // Es un bloque CM
                          //bDscr[numBlocks].type = CODE;                         
                          globalTAP.descriptor[numBlocks].type = CODE;                         
                      }
                  }


                  //bDscr[numBlocks].typeName = getTypeTAPBlock(bDscr[numBlocks].type);
                  globalTAP.descriptor[numBlocks].typeName = getTypeTAPBlock(globalTAP.descriptor[numBlocks].type);                         


                  if (blockNameDetected)
                  {
                      //bDscr[numBlocks].nameDetected = true;                                      
                      globalTAP.descriptor[numBlocks].nameDetected = true;                                      
                  }
                  else
                  {
                      //bDscr[numBlocks].nameDetected = false;
                      globalTAP.descriptor[numBlocks].nameDetected = false;
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
                  //break;
                  Serial.println("");
                  Serial.println("Success. End: ");
              }

          }

          
          // Añadimos información importante
          globalTAP.name = nameTAP;
          globalTAP.size = sizeTAP;
          globalTAP.numBlocks = numBlocks;
          

      }

      void showDescriptorTable()
      {
          Serial.println("");
          Serial.println("");
          Serial.println("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

          int totalBlocks = globalTAP.numBlocks;

          for (int n=0;n<totalBlocks;n++)
          {
              Serial.print("[" + String(n) + "]" + " - Offset: " + String(globalTAP.descriptor[n].offset) + " - Size: " + String(globalTAP.descriptor[n].size));
              char* strType = "";
              switch(globalTAP.descriptor[n].type)
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

              if (globalTAP.descriptor[n].nameDetected)
              {
                  Serial.println("");
                  //Serial.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")");
                  Serial.print("[" + String(n) + "]" + " - Name: " + globalTAP.descriptor[n].name + " - (" + strType + ")");
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
          //byte* bBlock = new byte[sizeTAP];
          byte* bBlock = (byte*)calloc(sizeTAP,sizeof(byte));
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
                  LAST_TYPE = "PROGRAM";
                  break;

              case 1:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    Serial.println("");
                    Serial.println("> BYTE HEADER");
                  #endif
                  LAST_TYPE = "BYTE.H";
                  break;

              case 7:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    Serial.println("");
                    Serial.println("> SCREEN HEADER");
                  #endif
                  LAST_TYPE = "SCREEN.H";
                  break;

              case 2:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    Serial.println("");
                    Serial.println("> BASIC PROGRAM");
                  #endif
                  LAST_TYPE = "BASIC";
                  break;

              case 3:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    Serial.println("");
                    Serial.println("> SCREEN");
                  #endif
                  LAST_TYPE = "SCREEN";
                  break;

              case 4:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    Serial.println("");
                    Serial.println("> BYTE CODE");
                  #endif
                  if (LAST_SIZE != 6914)
                  {
                      LAST_TYPE = "BYTE";
                  }
                  else
                  {
                      LAST_TYPE = "SCREEN";
                  }
                  break;
          }        
      }
      
      private:

      bool isFileTAP(File32 tapFileName)
      {
          // char* szName = (char*)calloc(255,sizeof(char));
          // String fileName = tapFileName.getName(szName,sizeof(char));
          
          // if (fileName != "")
          // {
          //     String fileExtension = (fileName.substring(fileName.length()-3)).toUpperCase()
          //     if (fileExtension = "TAP")
          //     {
          //         return true;
          //     }
          //     else
          //     {
          //         return false;
          //     }
          // }
          // else
          // {
          //     return false;
          // }
          return true;
      }

      public:

      TAPproccesor(File32 mFile, int sizeTAP)
      {
          // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
          // entonces se analiza y se construye el buffer, etc para poder manejarlo.
          int totalBlocks = 0;

          #ifdef LOG==3
              Serial.println("");
              Serial.println("Getting total blocks...");
          #endif

          if (isFileTAP(mFile))
          {
              getBlockDescriptor(mFile, sizeTAP);

              showDescriptorTable();
              myTAP = globalTAP;   
          }
      }      

      TAPproccesor()
      { }

};
