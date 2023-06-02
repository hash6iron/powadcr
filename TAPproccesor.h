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


      char* getNameFromHeader(byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          char* prgName = (char*)malloc(10+1);
          //char* prgName[10];

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

      void getBlockName(char** prgName, byte* header, int startByte)
      {
          // Obtenemos el nombre del bloque cabecera
          //char* prgName = new char[10];
          //char* prgName = (char*)malloc(10);

          // if (isCorrectHeader(header,startByte))
          // {
              // Es una cabecera PROGRAM 
              // el nombre está en los bytes del 4 al 13 (empezando en 0)
              #if LOG>3
                warningLog("Name detected ");
              #endif
              
              header[startByte+12]=0;
              *prgName = (char*) (header+2);
              
              /*

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
          prgName[i]='\0';*/

      }

      byte* getBlockRange(byte* bBlock, int byteStart, int byteEnd)
      {

          // Extraemos un bloque de bytes indicando el byte inicio y final
          //byte* blockExtracted = new byte[(byteEnd - byteStart)+1];
          byte* blockExtracted = (byte*)malloc((byteEnd - byteStart)+1);

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
          char* nameTAP = (char*)malloc(10+1);
          nameTAP = "\0";

          int startBlock = 0;
          int lastStartBlock = 0;
          int sizeB = 0;
          int newSizeB = 0;
          int chk = 0;
          int blockChk = 0;
          int numBlocks = 0;
          char* blockName = (char*)malloc(10+1);
          blockName = "\0";

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

              // Inicializamos
              blockName = "\0";
              bDscr[numBlocks].name = blockName;

              blockNameDetected = false;
              
              //Serial.println("START LOOP AGAIN: ");  
              byte* tmpRng = (byte*)malloc(sizeB);
              tmpRng = readFileRange32(mFile,startBlock,sizeB-1,false);
              chk = calculateChecksum(tmpRng,0,sizeB-1);
              free(tmpRng);
              //Serial.println("MARK CHK: ");
              
              blockChk = readFileRange32(mFile,startBlock+sizeB-1,1,false)[0];
              //Serial.println("MARK BLOCK CHK: ");            

              if (blockChk == chk)
              {
                  
                  bDscr[numBlocks].offset = startBlock;
                  bDscr[numBlocks].size = sizeB;
                  bDscr[numBlocks].chk = chk;

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
                  
                  // Serial.println("");
                  // Serial.println("MARK 1");
                  // Serial.println("");

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
                          // byte* blockTmp = readFileRange32(mFile,startBlock,19,false);
                          // String nameTmp = (String)getBlockName(blockTmp,0);

                          // bDscr[numBlocks].name = nameTmp.substring(0,10);
                          getBlockName(&bDscr[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);


                          //Cogemos el nombre del TAP de la primera cabecera
                          if (startBlock < 23)
                          {
                              //nameTAP = (String)bDscr[numBlocks].name;
                              nameTAP = bDscr[numBlocks].name;
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
                          // byte* blockTmp = readFileRange32(mFile,startBlock,19,false);
                          // String nameTmp = (String)getBlockName(blockTmp,0);
                          // bDscr[numBlocks].name = nameTmp.substring(0,10);
                          getBlockName(&bDscr[numBlocks].name,readFileRange32(mFile,startBlock,19,false),0);                          

                          int tmpSizeBlock = (256*readFileRange32(mFile,startBlock + sizeB+1,1,false)[0]) + readFileRange32(mFile,startBlock + sizeB,1,false)[0];

                          // Serial.println("");
                          // Serial.println("MARK 2");
                          // Serial.println("");

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

                  // Serial.println("");
                  // Serial.println("MARK 3");
                  // Serial.println("StartBlock: " + String(startBlock) + "/" + String(sizeTAP));
                  // Serial.println("Num block: " + String(numBlocks));
                  // Serial.println("sizeB: " + String(sizeB));
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
<<<<<<< Updated upstream
                  infoLog("Success. End: ");
=======
                  //break;
                  Serial.println("");
                  Serial.println("Success. End: ");
>>>>>>> Stashed changes
              }

          }

          // Serial.println("");
          // Serial.println("MARK 4");
          // Serial.println("");

          // Componemos el TAP para devolverlo
          describedTAP.name = nameTAP;
          // Serial.println("nameTAP: OK " + String(nameTAP));
          describedTAP.descriptor = bDscr;
          // Serial.println("DESCRIPTOR OK ");
          describedTAP.size = sizeTAP;
          // Serial.println("SIZETAP OK ");
          describedTAP.numBlocks = numBlocks;
          // Serial.println("NUMBLOCKS OK ");


          // Serial.println("");
          // Serial.println("MARK 5");
          // Serial.println("");

          //free(bDscr);
          
          return describedTAP;

      }

      void showDescriptorTable(tTAP tmpTAP)
      {
          debugLog("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

          tBlockDescriptor* bDscr = (tBlockDescriptor*)malloc(tmpTAP.numBlocks);
          bDscr = tmpTAP.descriptor;
          int totalBlocks = tmpTAP.numBlocks;

          for (int n=0;n<totalBlocks;n++)
          {
<<<<<<< Updated upstream
              debugLog(("[" + String(n) + "]" + " - Offset: " + String(bDscr[n].offset) + " - Size: " + String(bDscr[n].size)).c_str());
              String strType = "";
=======
              Serial.print("[" + String(n) + "]" + " - Offset: " + String(bDscr[n].offset) + " - Size: " + String(bDscr[n].size));
              char* strType = "";
>>>>>>> Stashed changes
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
<<<<<<< Updated upstream
                  debugLog(("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")").c_str());
=======
                  Serial.println("");
                  //Serial.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")");
                  Serial.print("[" + String(n) + "]" + " - Name: " + bDscr[n].name + " - (" + strType + ")");
>>>>>>> Stashed changes
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
          //byte* bBlock = new byte[sizeTAP];
          byte* bBlock = (byte*)malloc(sizeTAP);
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
                  LAST_TYPE = "PROGRAM";
                  break;

              case 1:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BYTE HEADER");
                  #endif
                  LAST_TYPE = "BYTE.H";
                  break;

              case 7:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> SCREEN HEADER");
                  #endif
                  LAST_TYPE = "SCREEN.H";
                  break;

              case 2:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BASIC PROGRAM");
                  #endif
                  LAST_TYPE = "BASIC";
                  break;

              case 3:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> SCREEN");
                  #endif
                  LAST_TYPE = "SCREEN";
                  break;

              case 4:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #ifdef LOG==3
                    warningLog("> BYTE CODE");
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
