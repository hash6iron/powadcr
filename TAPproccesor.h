/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TAPproccesor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de recursos para la gestión de ficheros .TAP de ZX Spectrum

    Version: 0.2

    Historico de versiones
    ----------------------
    v.0.1 - Version inicial
    v.0.2 - Se han hecho modificaciones relativas al resto de clases para HMI y SDmanager. Se ha incluido la reproducción y analisis de bloques que estaba antes en el powadcr.ino

    Derechos de autor y distribución
    --------------------------------
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    To Contact the dev team you can write to hash6iron@gmail.com
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#pragma once

// TYPE block
#define HPRGM 0                     //Cabecera PROGRAM
#define HCODE 1                     //Cabecera BYTE  
#define HSCRN 7                     //Cabecera BYTE SCREEN$
#define HARRAYNUM 5                 //Cabecera ARRAY numerico
#define HARRAYCHR 6                 //Cabecera ARRAY char
#define PRGM 2                      //Bloque de datos BASIC
#define SCRN 3                      //Bloque de datos Screen$
#define CODE 4                      //Bloque de datps CM (BYTE)

class TAPproccesor
{

      private:

      // Procesador de audio

      
      // Gestor de SD
      HMI _hmi;

      // Definicion de un TAP
      tTAP _myTAP;
      SdFat32 _sdf32;
      File32 _mFile;
      int _sizeTAP;
      int _rlen;

      int CURRENT_LOADING_BLOCK = 0;

      // Creamos el contenedor de bloques. Descriptor de bloques
      //tBlockDescriptor* bDscr = new tBlockDescriptor[255];

      bool isHeaderTAP(File32 tapFileName)
      {
          if (tapFileName != 0)
          {
                Serial.println("");
                Serial.println("");
                Serial.println("Begin isHeaderTAP");

                // La cabecera son 10 bytes
                byte* bBlock = (byte*)calloc(19+1,sizeof(byte));
                bBlock = sdm.readFileRange32(tapFileName,0,19,true);

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
          typeName = &INITCHAR[0];

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
              #if LOG==3
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
              prgName = &INITCHAR[0];
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
          //char* blockName = (char*)calloc(10+1,sizeof(char));
          //blockName = "\0";

          //bool blockNameDetected = false;
          
          bool reachEndByte = false;

          int state = 0;    

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          Serial.println("");
          Serial.println("Analyzing TAP file. Please wait ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];
          startBlock = 2;

          while(reachEndByte==false)
          {

              //Serial.println("START LOOP AGAIN: ");  
              byte* tmpRng = (byte*)calloc(sizeB,sizeof(byte));
              tmpRng = sdm.readFileRange32(_mFile,startBlock,sizeB-1,false);
              chk = calculateChecksum(tmpRng,0,sizeB-1);

              if (tmpRng!=NULL)
              {
                  free(tmpRng);
                  tmpRng=NULL;
              }
              
              //Serial.println("MARK CHK: ");
              
              blockChk = sdm.readFileRange32(_mFile,startBlock+sizeB-1,1,false)[0];
              //Serial.println("MARK BLOCK CHK: ");            

              if (blockChk == chk)
              {
                  
                  int flagByte = sdm.readFileRange32(_mFile,startBlock,1,false)[0];

                  int typeBlock = sdm.readFileRange32(_mFile,startBlock+1,1,false)[0];
                  
                  // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                  if (flagByte == 0)
                  {
                     
                      // Es una CABECERA
                      if (typeBlock==0)
                      {

                          //blockNameDetected = true;
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
                          //blockNameDetected = true;                                              

                          int tmpSizeBlock = (256*sdm.readFileRange32(_mFile,startBlock + sizeB+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock + sizeB,1,false)[0];

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
                  newSizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];

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
          
          if (_myTAP.descriptor != NULL)
          {
              free(_myTAP.descriptor);
              _myTAP.descriptor = NULL;
          }

          _myTAP.descriptor = (tBlockDescriptor*)calloc(numBlks+1,sizeof(tBlockDescriptor));
          // Guardamos el numero total de bloques
          
          char* nameTAP = (char*)calloc(10+1,sizeof(char));
          nameTAP = &INITCHAR[0];

          char* typeName = (char*)calloc(10+1,sizeof(char));
          typeName = &INITCHAR[0];

          int startBlock = 0;
          int lastStartBlock = 0;
          int sizeB = 0;
          int newSizeB = 0;
          int chk = 0;
          int blockChk = 0;
          int numBlocks = 0;
          char* blockName = (char*)calloc(10+1,sizeof(char));
          blockName = &INITCHAR[0];

          bool blockNameDetected = false;
          
          bool reachEndByte = false;

          int state = 0;    

          //Entonces recorremos el TAP. 
          // La primera cabecera SIEMPRE debe darse.
          Serial.println("");
          Serial.println("Analyzing TAP file. Please wait ...");
          
          // Los dos primeros bytes son el tamaño a contar
          sizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];
          startBlock = 2;

          while(reachEndByte==false && sizeB!=0)
          {
              // Inicializamos
              blockName = &INITCHAR[0];
              //bDscr[numBlocks].name = blockName;
              _myTAP.descriptor[numBlocks].name = blockName;

              blockNameDetected = false;
              
              //Serial.println("START LOOP AGAIN: ");  
              byte* tmpRng = (byte*)calloc(sizeB,sizeof(byte));
              tmpRng = sdm.readFileRange32(_mFile,startBlock,sizeB-1,false);
              chk = calculateChecksum(tmpRng,0,sizeB-1);

              if (tmpRng!=NULL)
              {
                  free(tmpRng);
                  tmpRng=NULL;
              }

              //Serial.println("MARK CHK: ");
              
              blockChk = sdm.readFileRange32(_mFile,startBlock+sizeB-1,1,false)[0];
              //Serial.println("MARK BLOCK CHK: ");            

              if (blockChk == chk)
              {
                  
                  _myTAP.descriptor[numBlocks].offset = startBlock;
                  _myTAP.descriptor[numBlocks].size = sizeB;
                  _myTAP.descriptor[numBlocks].chk = chk;        

                  // Cogemos info del bloque
                  
                  // Flagbyte
                  // 0x00 - HEADER
                  // 0xFF - DATA BLOCK
                  int flagByte = sdm.readFileRange32(_mFile,startBlock,1,false)[0];

                  // 0x00 - PROGRAM
                  // 0x01 - ARRAY NUM
                  // 0x02 - ARRAY CHAR
                  // 0x03 - CODE FILE
                  int typeBlock = sdm.readFileRange32(_mFile,startBlock+1,1,false)[0];
                  
                  // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                  if (flagByte == 0)
                  {

                      // Inicializamos                    
                      _myTAP.descriptor[numBlocks].type = 0;
                      _myTAP.descriptor[numBlocks].typeName = typeName;
                      
                      // Es una CABECERA
                      if (typeBlock==0)
                      {
                          // Es un header PROGRAM

                          _myTAP.descriptor[numBlocks].header = true;
                          _myTAP.descriptor[numBlocks].type = HPRGM;

                          blockNameDetected = true;

                          // Almacenamos el nombre
                          getBlockName(&_myTAP.descriptor[numBlocks].name,sdm.readFileRange32(_mFile,startBlock,19,false),0);


                          //Cogemos el nombre del TAP de la primera cabecera
                          if (startBlock < 23)
                          {
                              //nameTAP = (String)bDscr[numBlocks].name;
                              //nameTAP = bDscr[numBlocks].name;
                              nameTAP = _myTAP.descriptor[numBlocks].name;
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
                          getBlockName(&_myTAP.descriptor[numBlocks].name,sdm.readFileRange32(_mFile,startBlock,19,false),0);                          

                          int tmpSizeBlock = (256*sdm.readFileRange32(_mFile,startBlock + sizeB+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock + sizeB,1,false)[0];

                          if (tmpSizeBlock == 6914)
                          {
                              // Es una cabecera de un Screen
                              _myTAP.descriptor[numBlocks].screen = true;
                              _myTAP.descriptor[numBlocks].type = HSCRN;                              

                              state = 2;
                          }
                          else
                          {               
                              _myTAP.descriptor[numBlocks].type = HCODE;    
                          }
                   
                      }

                  }
                  else
                  {
                      if (state == 1)
                      {
                          state = 0;
                          // Es un bloque BASIC                         
                          _myTAP.descriptor[numBlocks].type = PRGM;
                      }
                      else if (state == 2)
                      {
                          state = 0;
                          // Es un bloque SCREEN                     
                          _myTAP.descriptor[numBlocks].type = SCRN;                         
                      }
                      else
                      {
                          // Es un bloque CM                        
                          _myTAP.descriptor[numBlocks].type = CODE;                         
                      }
                  }

                  _myTAP.descriptor[numBlocks].typeName = getTypeTAPBlock(_myTAP.descriptor[numBlocks].type);                         


                  if (blockNameDetected)
                  {                                     
                      _myTAP.descriptor[numBlocks].nameDetected = true;                                      
                  }
                  else
                  {
                      _myTAP.descriptor[numBlocks].nameDetected = false;
                  }

                  // Siguiente bloque
                  // Direcion de inicio (offset)
                  startBlock = startBlock + sizeB;
                  // Tamaño
                  newSizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];

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
          _myTAP.name = nameTAP;
          _myTAP.size = sizeTAP;
          _myTAP.numBlocks = numBlocks;
          

      }

      void showDescriptorTable()
      {
          Serial.println("");
          Serial.println("");
          Serial.println("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

          int totalBlocks = _myTAP.numBlocks;

          for (int n=0;n<totalBlocks;n++)
          {
              Serial.print("[" + String(n) + "]" + " - Offset: " + String(_myTAP.descriptor[n].offset) + " - Size: " + String(_myTAP.descriptor[n].size));
              char* strType = &INITCHAR[0];
              
              switch(_myTAP.descriptor[n].type)
              {
                  case 0: 
                    strType = &STRTYPE0[0];
                  break;

                  case 1:
                    strType = &STRTYPE1[0];
                  break;

                  case 7:
                    strType = &STRTYPE7[0];
                  break;

                  case 2:
                    strType = &STRTYPE2[0];
                  break;

                  case 3:
                    strType = &STRTYPE3[0];
                  break;

                  case 4:
                    strType = &STRTYPE4[0];
                  break;

                  default:
                    strType=&STRTYPEDEF[0];
              }

              if (_myTAP.descriptor[n].nameDetected)
              {
                  Serial.println("");
                  //Serial.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")");
                  Serial.print("[" + String(n) + "]" + " - Name: " + _myTAP.descriptor[n].name + " - (" + strType + ")");
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
                  #if LOG==3
                    Serial.println("> PROGRAM HEADER");
                  #endif
                  LAST_TYPE = &LASTYPE0[0];
                  break;

              case 1:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #if LOG==3
                    Serial.println("");
                    Serial.println("> BYTE HEADER");
                  #endif
                  LAST_TYPE = &LASTYPE1[0];
                  break;

              case 7:

                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #if LOG==3
                    Serial.println("");
                    Serial.println("> SCREEN HEADER");
                  #endif
                  LAST_TYPE = &LASTYPE7[0];
                  break;

              case 2:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #if LOG==3
                    Serial.println("");
                    Serial.println("> BASIC PROGRAM");
                  #endif
                  LAST_TYPE = &LASTYPE2[0];
                  break;

              case 3:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #if LOG==3
                    Serial.println("");
                    Serial.println("> SCREEN");
                  #endif
                  LAST_TYPE = &LASTYPE3[0];
                  break;

              case 4:
                  // Definimos el buffer del PLAYER igual al tamaño del bloque
                  #if LOG==3
                    Serial.println("");
                    Serial.println("> BYTE CODE");
                  #endif
                  if (LAST_SIZE != 6914)
                  {
                      LAST_TYPE = &LASTYPE4_1[0];
                  }
                  else
                  {
                      LAST_TYPE = &LASTYPE4_2[0];
                  }
                  break;
          }        
      }

      void sendStatus(int action, int value) {

        switch (action) {
          case PLAY_ST:
            //_hmi.writeString("");
            _hmi.writeString("PLAYst.val=" + String(value));
            break;

          case STOP_ST:
            //_hmi.writeString("");
            _hmi.writeString("STOPst.val=" + String(value));
            break;

          case PAUSE_ST:
            //_hmi.writeString("");
            _hmi.writeString("PAUSEst.val=" + String(value));
            break;

          case END_ST:
            //_hmi.writeString("");
            _hmi.writeString("ENDst.val=" + String(value));
            break;

          case READY_ST:
            //_hmi.writeString("");
            _hmi.writeString("READYst.val=" + String(value));
            break;

          case ACK_LCD:
            //_hmi.writeString("");
            _hmi.writeString("tape.LCDACK.val=" + String(value));
            //_hmi.writeString("");
            _hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");
            break;
          
          case RESET:
            //_hmi.writeString("");
            _hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
        }
      }

      public:    

      void set_SdFat32(SdFat32 sdf32)
      {
          _sdf32 = sdf32;
      }

      void set_file(File32 tapFileName, int sizeTAP)
      {
          // Pasamos los parametros a la clase
          _mFile = tapFileName;
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

      void set_SDM(SDmanager sdmTmp)
      {
          //_sdm = sdmTmp;
          //ptrSdm = &sdmTmp;
      }

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

      void proccess_tap(File32 tapFileName)
      {
          // Procesamos el fichero
          Serial.println("");
          Serial.println("Getting total blocks...");

          if (isFileTAP(tapFileName))
          {
              getBlockDescriptor(_mFile, _sizeTAP);
              showDescriptorTable();
              //myTAP = _myTAP;   
          }
      }

      void getInfoFileTAP(char* path) 
      {
      
        File32 tapFile;

        LAST_MESSAGE = "Analyzing file";
        _hmi.updateInformationMainPage();
      
        // Abrimos el fichero
        tapFile = sdm.openFile32(tapFile, path);
        
        // Obtenemos su tamaño total
        _mFile = tapFile;
        _rlen = tapFile.available();
      
        // creamos un objeto TAPproccesor
        //set_file(tapFileName, _rlen);
        proccess_tap(tapFile);
        
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
                  bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);

                  // Llamamos a la clase de reproducción
                  zxp.playHeaderProgram(bufferPlay, _myTAP.descriptor[i].size);

                } else if (_myTAP.descriptor[i].type == 1 || _myTAP.descriptor[i].type == 7) {
                  
                  // CABECERAS
                  if(bufferPlay!=NULL)
                  {
                      free(bufferPlay);
                      bufferPlay=NULL;
                  }      

                  bufferPlay = (byte*)calloc(_myTAP.descriptor[i].size, sizeof(byte));
                  bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);

                  zxp.playHeader(bufferPlay, _myTAP.descriptor[i].size);
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

                        bufferPlay = sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);
                        zxp.playDataBegin(bufferPlay, blockPlaySize);
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
                        bufferPlay = sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);

                        zxp.playDataEnd(bufferPlay, blockPlaySize);
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
                    bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, true);
                    
                    zxp.playData(bufferPlay, _myTAP.descriptor[i].size);
                  }
                }
              }

              Serial.println("");
              Serial.println("Playing was finish.");
              // En el caso de no haber parado manualmente, es por finalizar
              // la reproducción
              if (LOADING_STATE == 1) {
                PLAY = false;
                STOP = true;
                PAUSE = false;
                BLOCK_SELECTED = 0;
                LAST_MESSAGE = "Playing end. Automatic STOP.";

                _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

                _hmi.updateInformationMainPage();

                // Ahora ya podemos tocar el HMI panel otra vez
                sendStatus(END_ST, 1);
              }

              // Cerrando
              LOADING_STATE = 0;

              if(bufferPlay!=NULL)
              {
                free(bufferPlay);
                bufferPlay=NULL;
              }
              
              Serial.flush();

              // Ahora ya podemos tocar el HMI panel otra vez
              sendStatus(READY_ST, 1);

        }
        else
        {
            LAST_MESSAGE = "No file selected.";
            _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

            _hmi.updateInformationMainPage();
        }

      }


      // Constructor de la clase
      TAPproccesor(AudioKit kit)
      {
          _myTAP.name = (char*)calloc(10+1,sizeof(char));
          _myTAP.name = &INITCHAR[0];
          _myTAP.numBlocks = 0;
          _myTAP.descriptor = NULL;
          _myTAP.size = 0;

          //zxp.set_ESP32kit(kit);
      }      

};
