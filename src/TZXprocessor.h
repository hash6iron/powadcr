/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TZXproccesor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de recursos para la gestión de ficheros .TZX del ZX Spectrum

    Version: 1.0

    Historico de versiones

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

class TZXprocessor
{

    private:

    const char ID10STR[35] = "ID 10 - Standard block            ";
    const char ID11STR[35] = "ID 11 - Speed block               ";
    const char ID12STR[35] = "ID 12 - Pure tone                 ";
    const char ID13STR[35] = "ID 13 - Pulse seq.                ";
    const char ID14STR[35] = "ID 14 - Pure data                 ";
    const char ID15STR[35] = "ID 15 - Direct recording          ";
    const char ID18STR[35] = "ID 18 - CSW recording block       ";
    const char ID19STR[35] = "ID 19 - Generalized data block    ";
    const char ID20STR[35] = "ID 20 - Pause or Stop             ";
    const char ID21STR[35] = "ID 21 - Group start               ";
    const char ID22STR[35] = "ID 22 - Group end                 ";
    const char ID23STR[35] = "ID 23 - Jump to block             ";
    const char ID24STR[35] = "ID 24 - Loop start                ";
    const char ID25STR[35] = "ID 25 - Loop end                  ";
    const char ID26STR[35] = "ID 26 - Call sequence             ";
    const char ID27STR[35] = "ID 27 - Return from sequence      ";
    const char ID28STR[35] = "ID 28 - Select block              ";
    const char ID2ASTR[35] = "ID 2A - Stop TAPE (48k mode)      ";
    const char ID2BSTR[35] = "ID 2B - Set signal level          ";
    const char ID4BSTR[35] = "ID 4B - TSX Block                 ";
    const char ID5ASTR[35] = "ID 5A - Glue block                ";
    const char IDXXSTR[35] = "Information block                 ";

    const int maxAllocationBlocks = 4000;

    // Procesador de audio output
    ZXProcessor _zxp;
    BlockProcessor _blDscTZX;

    HMI _hmi;

    // Definicion de un TZX
    //SdFat32 _sdf32;
    tTZX _myTZX;
    File _mFile;
    int _sizeTZX;
    int _rlen;

    // Audio HW
    AudioInfo new_sr;
    //AudioInfo new_sr2;   

    int CURRENT_LOADING_BLOCK = 0;

    bool stopOrPauseRequest()
    {
        // 
        
        if (LOADING_STATE == 1)
        {
            if (STOP==true)
            {
                LAST_MESSAGE = "Stop or pause requested";             
                LOADING_STATE = 2; // STOP del bloque actual
                return true;
            }
            else if (PAUSE==true)
            {
                LAST_MESSAGE = "Stop or pause requested";
                LOADING_STATE = 3; // PAUSE del bloque actual
                return true;
            }
          }      

        return false;   
    }

    uint8_t calculateChecksum(uint8_t* bBlock, int startByte, int numBytes)
    {
        // Calculamos el checksum de un bloque de bytes
        uint8_t newChk = 0;

        #if LOG>3
          ////SerialHW.println("");
          ////SerialHW.println("Block len: ");
          ////SerialHW.print(sizeof(bBlock)/sizeof(uint8_t*));
        #endif

        // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
        
        for (int n=startByte;n<(startByte+numBytes);n++)
        {
            newChk = newChk ^ bBlock[n];
        }

        #if LOG>3
          ////SerialHW.println("");
          ////SerialHW.println("Checksum: " + String(newChk));
        #endif

        return newChk;
    }      

    char* getNameFromStandardBlock(uint8_t* header)
    {
        // Obtenemos el nombre del bloque cabecera
        static char prgName[11];

        // Extraemos el nombre del programa desde un ID10 - Standard block
        for (int n=2;n<12;n++)
        {   
            if (header[n] <= 32)
            {
              // Los caracteres por debajo del espacio 0x20 se sustituyen
              // por " " ó podemos poner ? como hacía el Spectrum.
              prgName[n-2] = ' ';
            }
            if (header[n] == 96)
            {
              prgName[n-2] = (char)0xA3; //'£'; // El caracter 96 es la libra esterlina
            }
            if (header[n] == 126)
            {
              prgName[n-2] = (char)0x7E; //'~'; // El caracter 126 es la virgulilla
            }     
            if (header[n] == 127)
            {
              prgName[n-2] = (char)0xA9; //'©'; //  El caracter 127 es el copyright
            }   
            if (header[n] > 127)
            {
              prgName[n-2] = ' '; // Los caracteres por encima del espacio 0x7F se sustituyen
            }                               
            else
            {
              prgName[n-2] = (char)header[n];
            }
        }

        logln("Program name: " + String(prgName));

        // Pasamos la cadena de caracteres
        return prgName;

      }

    bool isHeaderTZX(File tzxFile)
    {
      if (tzxFile != 0)
      {

          // Capturamos la cabecera
          uint8_t* bBlock = (uint8_t*)ps_calloc(10+1,sizeof(uint8_t));
          readFileRange(tzxFile,bBlock,0,10,false); 

          // Obtenemos la firma del TZX
          char signTZXHeader[9];

          // Analizamos la cabecera
          // Extraemos el nombre del programa
          for (int n=0;n<7;n++)
          {   
              signTZXHeader[n] = (char)bBlock[n];
          }
          
          free(bBlock);

          //Aplicamos un terminador a la cadena de char
          signTZXHeader[7] = '\0';
          //Convertimos a String
          String signStr = String(signTZXHeader);
          
          if (signStr.indexOf("ZXTape!") != -1)
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

    bool isFileTZX(File mfile)
    {
        if (mfile)
        {
          // Capturamos el nombre del fichero en szName
          String szName = mfile.name();
          String fileName = String(szName);

          if (fileName != "")
          {
                //String fileExtension = szNameStr.substring(szNameStr.length()-3);
                fileName.toUpperCase();
                
                if (fileName.indexOf(".TZX") != -1 || fileName.indexOf(".TSX") != -1 || fileName.indexOf(".CDT") != -1 )
                {
                    //La extensión es TZX o CDT pero ahora vamos a comprobar si internamente también lo es
                    if (isHeaderTZX(mfile))
                    {
                      //Cerramos el fichero porque no se usará mas.
                      //mfile.close();
                      return true;
                    }
                    else
                    {
                      //Cerramos el fichero porque no se usará mas.
                      //mfile.close();
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
        else
        {
          return false;
        }

    }

    int getWORD(File mFile, int offset)
    {
        int sizeDW = 0;
        uint8_t* ptr1 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        uint8_t* ptr2 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        readFileRange(mFile,ptr1,offset+1,1,false);
        readFileRange(mFile,ptr2,offset,1,false);
        sizeDW = (256*ptr1[0]) + ptr2[0];
        free(ptr1);
        free(ptr2);

        return sizeDW;    
    }

    int getBYTE(File mFile, int offset)
    {
        int sizeB = 0;
        uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        readFileRange(mFile,ptr,offset,1,false);
        sizeB = ptr[0];
        free(ptr);

        return sizeB;      
    }

    int getNBYTE(File mFile, int offset, int n)
    {
        int sizeNB = 0;
        uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        
        for (int i = 0; i<n;i++)
        {
            readFileRange(mFile,ptr,offset+i,1,false);
            sizeNB += pow(2,(8*i)) * (ptr[0]);  
        }

        free(ptr);
        return sizeNB;             
    }

    int getID(File mFile, int offset)
    {
        // Obtenemos el ID
        int ID = 0;
        ID = getBYTE(mFile,offset);

        return ID;      
    }

    void getBlock(File mFile, uint8_t* &block, int offset, int size)
    {
        //Entonces recorremos el TZX. 
        // La primera cabecera SIEMPRE debe darse.
        readFileRange(mFile,block,offset,size,false);
    }

    bool verifyChecksum(File mFile, int offset, int size)
    {
        // Vamos a verificar que el bloque cumple con el checksum
        // Cogemos el checksum del bloque
        uint8_t chk = getBYTE(mFile,offset+size-1);

        uint8_t* block = (uint8_t*)ps_calloc(size+1,sizeof(uint8_t));
        getBlock(mFile,block,offset,size-1);
        uint8_t calcChk = calculateChecksum(block,0,size-1);
        free(block);

        if (chk == calcChk)
        {
            return true;
        }
        else
        {            
          return false;
        }
    }

    void analyzeID16(File mFile, int currentOffset, int currentBlock)
    {
      //Normal speed block

      //
      // TAP description block
      //
      
      //       |------ Spectrum-generated data -------|       |---------|

      // 13 00 00 03 52 4f 4d 7x20 02 00 00 00 00 80 f1 04 00 ff f3 af a3

      // ^^^^^...... first block is 19 bytes (17 bytes+flag+checksum)
      //       ^^... flag byte (A reg, 00 for headers, ff for data blocks)
      //          ^^ first byte of header, indicating a code block

      // file name ..^^^^^^^^^^^^^
      // header info ..............^^^^^^^^^^^^^^^^^
      // checksum of header .........................^^
      // length of second block ........................^^^^^
      // flag byte ...........................................^^
      // first two bytes of rom .................................^^^^^
      // checksum (checkbittoggle would be a better name!).............^^      
     
        int headerTAPsize = 21;   //Contando blocksize + flagbyte + etc hasta 
                                  //checksum
        int dataTAPsize = 0;
        char* gName;
        
        tTZXBlockDescriptor blockDescriptor; 
        _myTZX.descriptor[currentBlock].ID = 16;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // ////SerialHW.println("");
        // ////SerialHW.println("Offset begin: ");
        // ////SerialHW.print(currentOffset,HEX);

        //SYNC1
        // _zxp.SYNC1 = DSYNC1;
        // //SYNC2
        // _zxp.SYNC2 = DSYNC2;
        // //PULSE PILOT
        // _zxp.PILOT_PULSE_LEN = DPILOT_LEN;                              
        // // BTI 0
        // _zxp.BIT_0 = DBIT_0;
        // // BIT1                                          
        // _zxp.BIT_1 = DBIT_1;
        // No contamos el ID. Entonces:
        
        // Timming de la ROM
        _myTZX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
        _myTZX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
        _myTZX.descriptor[currentBlock].timming.sync_2 = DSYNC2;
        _myTZX.descriptor[currentBlock].timming.bit_0 = DBIT_0;
        _myTZX.descriptor[currentBlock].timming.bit_1 = DBIT_1;

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+1);

        // ////SerialHW.println("");
        // ////SerialHW.println("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getWORD(mFile,currentOffset+3);

        // ////SerialHW.println("");
        // ////SerialHW.println("Length of data: ");
        // ////SerialHW.print(_myTZX.descriptor[currentBlock].lengthOfData,HEX);

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 5;

        // ////SerialHW.println("");
        // ////SerialHW.println("Offset of data: ");
        // ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);

        // Vamos a verificar el flagByte
        int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        // Y cogemos para ver el tipo de cabecera
        int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);

        // Si el flagbyte es menor a 0x80
        if (flagByte < 128)
        {

            _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_HEADER;
            
            // Es una cabecera
            // 0 - program
            // 1 - Number array
            // 2 - Char array
            // 3 - Code file                     
            if (typeBlock == 0)
            {
                // Es una cabecera PROGRAM: BASIC
              _myTZX.descriptor[currentBlock].header = true;
              _myTZX.descriptor[currentBlock].type = 0;

              // if (!PROGRAM_NAME_DETECTED)
              // {
              uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
              getBlock(mFile,block,_myTZX.descriptor[currentBlock].offsetData,19);
              gName = getNameFromStandardBlock(block);
              PROGRAM_NAME = String(gName);
              LAST_PROGRAM_NAME = PROGRAM_NAME;
              PROGRAM_NAME_DETECTED = true;
              free(block);
              strncpy(_myTZX.descriptor[currentBlock].name,gName,14);
              // }
              // else
              // {
              //   strncpy(_myTZX.descriptor[currentBlock].name,_myTZX.name,14);
              // }

              // Almacenamos el nombre del bloque
              // 
            }
            else if (typeBlock == 1)
            {
                // Number ARRAY
                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 2;
            }
            else if (typeBlock == 2)
            {
                // Char ARRAY
                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 4;
            }
            else if (typeBlock == 3)
            {
              // BYTE o SCREEN (CODE)
              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {

                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 7;

                  // Almacenamos el nombre del bloque
                  uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
                  getBlock(mFile,block,_myTZX.descriptor[currentBlock].offsetData,19);
                  gName = getNameFromStandardBlock(block);
                  strncpy(_myTZX.descriptor[currentBlock].name,gName,14);
                  free(block);
              }
              else
              {
                // Es un bloque BYTE
                _myTZX.descriptor[currentBlock].type = 1;     

                // Almacenamos el nombre del bloque
                uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
                getBlock(mFile,block,_myTZX.descriptor[currentBlock].offsetData,19);
                gName = getNameFromStandardBlock(block);
                strncpy(_myTZX.descriptor[currentBlock].name,gName,14);                       
                free(block);
              }
            }
        }
        else
        {
            _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_DATA;

            if (typeBlock == 3)
            {
              // SCREEN
              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 7;
              }
              else
              {
                  // Es un bloque BYTE
                  _myTZX.descriptor[currentBlock].type = 1;                
              }
            }
            else
            {
                // Es un bloque BYTE
                _myTZX.descriptor[currentBlock].type = 1;
            }            
        } 

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData; 

        // Por defecto en ID10 no tiene MASK_LAST_BYTE pero lo inicializamos 
        // para evitar problemas con el ID11
        _myTZX.descriptor[currentBlock].hasMaskLastByte = false;
    }
        
    void analyzeID17(File mFile, int currentOffset, int currentBlock)
    {   
        char* gName;

        // Turbo data

        _myTZX.descriptor[currentBlock].ID = 17;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // Entonces
        // Timming de PULSE PILOT
        _myTZX.descriptor[currentBlock].timming.pilot_len = getWORD(mFile,currentOffset+1);

        ////SerialHW.println("");
        ////SerialHW.print("PULSE PILOT=");
        ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.pulse_pilot, HEX);

        // Timming de SYNC_1
        _myTZX.descriptor[currentBlock].timming.sync_1 = getWORD(mFile,currentOffset+3);

          //////SerialHW.println("");
          ////SerialHW.print(",SYNC1=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_1, HEX);

        // Timming de SYNC_2
        _myTZX.descriptor[currentBlock].timming.sync_2 = getWORD(mFile,currentOffset+5);

          //////SerialHW.println("");
          ////SerialHW.print(",SYNC2=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_2, HEX);

        // Timming de BIT 0
        _myTZX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+7);

          //////SerialHW.println("");
          ////SerialHW.print(",BIT_0=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_0, HEX);

        // Timming de BIT 1
        _myTZX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+9);        

          //////SerialHW.println("");
          ////SerialHW.print(",BIT_1=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_1, HEX);

        // Timming de PILOT TONE
        _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = getWORD(mFile,currentOffset+11);

          //////SerialHW.println("");
          ////SerialHW.print(",PILOT TONE=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses, HEX);

        // Cogemos el byte de bits of the last byte
        _myTZX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+13);
        _myTZX.descriptor[currentBlock].hasMaskLastByte = true;

          //////SerialHW.println("");
          ////SerialHW.print(",MASK LAST BYTE=");
          ////SerialHW.print(_myTZX.descriptor[currentBlock].maskLastByte, HEX);


        // ********************************************************************
        //
        //
        //
        //
        //                 Ahora analizamos el BLOQUE DE DATOS
        //
        //
        //
        //
        // ********************************************************************

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+14);

        ////SerialHW.println("");
        ////SerialHW.print("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
        ////SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
        ////SerialHW.println("");
        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+16,3);

        ////SerialHW.println("");
        ////SerialHW.print("Length of data: ");
        ////SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));
        ////SerialHW.println("");

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 19;

        // Almacenamos el nombre del bloque
        //_myTZX.descriptor[currentBlock].name = getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19));
        
        ////SerialHW.println("");
        ////SerialHW.print("Offset of data: 0x");
        ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
        ////SerialHW.println("");

        // Vamos a verificar el flagByte

        int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);

        if (flagByte < 128)
        {
            // Es una cabecera
            uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
            getBlock(mFile,block,_myTZX.descriptor[currentBlock].offsetData,19);
            gName = getNameFromStandardBlock(block);
            strncpy(_myTZX.descriptor[currentBlock].name,gName,14);                       
            free(block);  
                      
            if (typeBlock == 0)
            {
                // Es una cabecera BASIC
                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 0;

            }
            else if (typeBlock == 1)
            {
                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 2)
            {
                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 3)
            {
              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 7;
              }
              else
              {
                  // Es un bloque BYTE
                  _myTZX.descriptor[currentBlock].type = 1;                
              }
            }
        }
        else
        {
            // Es un bloque BYTE
            strncpy(_myTZX.descriptor[currentBlock].name,"              ",14);
            _myTZX.descriptor[currentBlock].type = 4;
        }

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData; 
    }

    void analyzeID18(File mFile, int currentOffset, int currentBlock)
    {
        // ID-12 - Pure tone

        _myTZX.descriptor[currentBlock].ID = 18;
        _myTZX.descriptor[currentBlock].playeable = true;
        //_myTZX.descriptor[currentBlock].forSetting = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset; 
        _myTZX.descriptor[currentBlock].timming.pure_tone_len = getWORD(mFile,currentOffset+1);
        _myTZX.descriptor[currentBlock].timming.pure_tone_num_pulses = getWORD(mFile,currentOffset+3); 

        #ifdef DEBUGMODE
            SerialHW.println("");
            SerialHW.print("Pure tone setting: Ts: ");
            SerialHW.print(_myTZX.descriptor[currentBlock].timming.pure_tone_len,HEX);
            SerialHW.print(" / Np: ");
            SerialHW.print(_myTZX.descriptor[currentBlock].timming.pure_tone_num_pulses,HEX);        
            SerialHW.println("");
        #endif

        // Esto es para que tome los bloques como especiales
        _myTZX.descriptor[currentBlock].type = 99;     
        // Guardamos el size
        _myTZX.descriptor[currentBlock].size = 4;
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = 0;        
        _myTZX.descriptor[currentBlock].silent = 0;        
        //     
    }

    void analyzeID19(File mFile, int currentOffset, int currentBlock)
    {
        // ID-13 - Pulse sequence

        _myTZX.descriptor[currentBlock].ID = 19;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        int num_pulses = getBYTE(mFile,currentOffset+1);
        _myTZX.descriptor[currentBlock].timming.pulse_seq_num_pulses = num_pulses;
        
        // Reservamos memoria.
        _myTZX.descriptor[currentBlock].timming.pulse_seq_array = (int*)ps_calloc(num_pulses + 1,sizeof(int));
        
        // Tomamos ahora las longitudes
        int coff = currentOffset+2;

        #ifdef DEBUGMODE
          log("ID-13: Pulse sequence");
        #endif

        for (int i=0;i<num_pulses;i++)
        {
          int lenPulse = getWORD(mFile,coff);
          _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;

          #ifdef DEBUGMODE
            SerialHW.print("[" +String(i) + "]=0x");
            SerialHW.print(lenPulse,HEX);
            SerialHW.print("; ");
          #endif

          // Adelantamos 2 bytes
          coff+=2;
        }

        ////SerialHW.println("");

        // Esto es para que tome los bloques como especiales
        _myTZX.descriptor[currentBlock].type = 99;     
        // Guardamos el size (bytes)
        _myTZX.descriptor[currentBlock].size = (num_pulses*2)+1; 
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = 0;        
        _myTZX.descriptor[currentBlock].silent = 0;        
        //
    }

    void analyzeID20(File mFile, int currentOffset, int currentBlock)
    {
        // ID-14 - Pure Data Block

        _myTZX.descriptor[currentBlock].ID = 20;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset; 


        // Timming de la ROM
        _myTZX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
        _myTZX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
        _myTZX.descriptor[currentBlock].timming.sync_2 = DSYNC2;

        // Timming de BIT 0
        _myTZX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+1);

          //////SerialHW.println("");
          // //SerialHW.print(",BIT_0=");
          // //SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_0, HEX);

        // Timming de BIT 1
        _myTZX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+3);        

          //////SerialHW.println("");
          // //SerialHW.print(",BIT_1=");
          // //SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_1, HEX);

        // Cogemos el byte de bits of the last byte
        _myTZX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+5);
        _myTZX.descriptor[currentBlock].hasMaskLastByte = true;        
        //_zxp.set_maskLastByte(_myTZX.descriptor[currentBlock].maskLastByte);

          // //SerialHW.println("");
          // //SerialHW.print(",MASK LAST BYTE=");
          // //SerialHW.print(_myTZX.descriptor[currentBlock].maskLastByte, HEX);


        // ********************************************************************
        //
        //
        //
        //
        //                 Ahora analizamos el BLOQUE DE DATOS
        //
        //
        //
        //
        // ********************************************************************

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+6);

        // //SerialHW.println("");
        // //SerialHW.print("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
        // //SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
        // //SerialHW.println("");

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+8,3);

        // //SerialHW.println("");
        // //SerialHW.println("Length of data: ");
        // //SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 11;

        ////SerialHW.println("");
        ////SerialHW.print("Offset of data: 0x");
        ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
        ////SerialHW.println("");

        // Vamos a verificar el flagByte

        int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);
        //
        //_myTZX.descriptor[currentBlock].name = &INITCHAR[0];
        strncpy(_myTZX.descriptor[currentBlock].name,"              ",14);        
        //_myTZX.descriptor[currentBlock].type = 4;
        _myTZX.descriptor[currentBlock].type = 99; // Importante
        _myTZX.descriptor[currentBlock].header = false;

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData;        
    }

    void analyzeID21(File mFile, int currentOffset, int currentBlock)
    {
      // ID-15 - Direct recording

        _myTZX.descriptor[currentBlock].ID = 21;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[currentBlock].samplingRate = getWORD(mFile,currentOffset+1);
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+3);
        // Esto es muy importante para el ID 0x15
        // Used bits (samples) in last byte of data (1-8) (e.g. if this is 2, only first two samples of the last byte will be played)
        _myTZX.descriptor[currentBlock].hasMaskLastByte = true;
        _myTZX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+5);

        // //SerialHW.println("");
        // //SerialHW.print("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
        // //SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
        // //SerialHW.println("");

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+6,3);

        // //SerialHW.println("");
        // //SerialHW.println("Length of data: ");
        // //SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 9;

        ////SerialHW.println("");
        ////SerialHW.print("Offset of data: 0x");
        ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
        ////SerialHW.println("");

        // Vamos a verificar el flagByte

        //int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        //int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);
        //
        //_myTZX.descriptor[currentBlock].name = &INITCHAR[0];
        //strncpy(_myTZX.descriptor[currentBlock].name,"              ",14);        
        //_myTZX.descriptor[currentBlock].type = 4;
        _myTZX.descriptor[currentBlock].type = 0; // Importante
        _myTZX.descriptor[currentBlock].header = false;

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData;         
    }

    void analyzeID32(File mFile, int currentOffset, int currentBlock)
    {
        // Pause or STOP the TAPE
        
        _myTZX.descriptor[currentBlock].ID = 32;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        // Obtenemos el valor de la pausa
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+1);          
    }

    void analyzeID40(File mFile, int currentOffset, int currentBlock)
    {
        // Size block
        int sizeBlock = 0;
        int numSelections = 0;

        // Obtenemos la dirección del siguiente offset
        _myTZX.descriptor[currentBlock].ID = 40;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset + 2;

        sizeBlock = getWORD(mFile,currentOffset+1);
        numSelections = getBYTE(mFile,currentOffset+3);

        _myTZX.descriptor[currentBlock].size = sizeBlock+2;
        
    }

    void analyzeID48(File mFile, int currentOffset, int currentBlock)
    {
        // 0x30 - Text Description
        int sizeTextInformation = 0;

        _myTZX.descriptor[currentBlock].ID = 48;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        ////SerialHW.println("");
        ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeTextInformation + 1;
    }

    void analyzeID49(File mFile, int currentOffset, int currentBlock)
        {
            // 0x31 - Message block
            int sizeTextInformation = 0;

            _myTZX.descriptor[currentBlock].ID = 49;
            _myTZX.descriptor[currentBlock].playeable = false;
            _myTZX.descriptor[currentBlock].offset = currentOffset;

            sizeTextInformation = getBYTE(mFile,currentOffset+2);

            ////SerialHW.println("");
            ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
            
            // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
            // el bloque comienza en el offset del ID y acaba en
            // offset[ID] + tamaño_bloque
            _myTZX.descriptor[currentBlock].size = sizeTextInformation + 1;
        }

    void analyzeID50(File mFile, int currentOffset, int currentBlock)
    {
        // ID 0x32 - Archive Info
        int sizeBlock = 0;

        _myTZX.descriptor[currentBlock].ID = 50;
        _myTZX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTZX.descriptor[currentBlock].offset = currentOffset+3;

        sizeBlock = getWORD(mFile,currentOffset+1);

        //////SerialHW.println("");
        //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeBlock;         
    }    

    void analyzeID51(File mFile, int currentOffset, int currentBlock)
    {
        // 0x33 - Hardware Information block
        int sizeBlock = 0;

        _myTZX.descriptor[currentBlock].ID = 51;
        _myTZX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // El bloque completo mide, el numero de maquinas a listar
        // multiplicado por 3 byte por maquina listada
        sizeBlock = getBYTE(mFile,currentOffset+1) * 3;

        //////SerialHW.println("");
        //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeBlock-1;         
    }

    void analyzeID33(File mFile, int currentOffset, int currentBlock)
    {
        // Group start
        int sizeTextInformation = 0;

        _myTZX.descriptor[currentBlock].ID = 33;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset+2;
        _myTZX.descriptor[currentBlock].group = MULTIGROUP_COUNT;
        MULTIGROUP_COUNT++;

        // Tomamos el tamaño del nombre del Grupo
        sizeTextInformation = getBYTE(mFile,currentOffset+1);
        _myTZX.descriptor[currentBlock].size = sizeTextInformation;

        // Ahora cogemos el texto en el siguiente byte
        //#ifdef DEBUGMODE
          logln("ID33 - sizeTextInformation = " + String(sizeTextInformation) + " bytes");
        //#endif

        uint8_t* grpN = (uint8_t*)ps_calloc(sizeTextInformation+5,sizeof(uint8_t));
        readFileRange(mFile,grpN,currentOffset+2,sizeTextInformation,false);
        char groupName[32];
        // Limpiamos de basura todo el buffer
        strcpy(groupName,"                             ");
        strncpy(groupName,(char*)grpN,30);

        // for(int i=0;i<sizeTextInformation-1;i++)
        // {
        //     groupName[i] = (char)grpN[i];
        // }

        // logln("Group name: " + String(groupName));

        // Cogemos solo 29 letras
        if (sizeTextInformation < 30)
        {
          strncpy(_myTZX.descriptor[currentBlock].name,groupName,sizeTextInformation);
        }
        else
        {
          strncpy(_myTZX.descriptor[currentBlock].name,groupName,29);
        }

        logln("ID33 - Group start: " + String(groupName));
        free(grpN);

    }

    void analyzeID53(File mFile, int currentOffset, int currentBlock)
    {
        _myTZX.descriptor[currentBlock].ID = 53;
        _myTZX.descriptor[currentBlock].playeable = false;
        // Size block
        int sizeBlock = 0;
        sizeBlock = getWORD(mFile,currentOffset+0x10+1);
        _myTZX.descriptor[currentBlock].size = sizeBlock;
        _myTZX.descriptor[currentBlock].offset = currentOffset+0x14;
        SerialHW.print("Tamaño: 0x");
        SerialHW.println(sizeBlock,HEX);
    }

    void analyzeID75(File mFile, int currentOffset, int currentBlock)
    {
        // ID 0x4B

        _myTZX.descriptor[currentBlock].ID = 75;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;
        _myTZX.descriptor[currentBlock].timming.sync_1 = 0;
        _myTZX.descriptor[currentBlock].timming.sync_2 = 0;
        
        //Tamaño de los datos
        _myTZX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+1,4)-12;
        _myTZX.descriptor[currentBlock].size=_myTZX.descriptor[currentBlock].lengthOfData;
        #ifdef DEBUGMODE
          logln("");
          logln("Tamaño datos: " + String(_myTZX.descriptor[currentBlock].lengthOfData) + " bytes");
        #endif

        _myTZX.descriptor[currentBlock].offsetData=currentOffset+17;

        //Pausa despues del bloque
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+5);
        #ifdef DEBUGMODE
          logln("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
        #endif
        
        // Timming de PULSE PILOT
        _myTZX.descriptor[currentBlock].timming.pilot_len = getWORD(mFile,currentOffset+7);
        #ifdef DEBUGMODE
          logln(",PULSE PILOT = " + String(_myTZX.descriptor[currentBlock].timming.pilot_len));
        #endif
        
        // Timming de PILOT TONE
        _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = getWORD(mFile,currentOffset+9);
        #ifdef DEBUGMODE
          logln(",PULSE TONE = " + String(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses));
        #endif

        // Timming de ZERO
        _myTZX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+11);
        #ifdef DEBUGMODE
          logln("PULSE ZERO = " + String(_myTZX.descriptor[currentBlock].timming.bit_0));
        #endif
        
        // Timming de ONE
        _myTZX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+13);
        #ifdef DEBUGMODE
          logln("PULSE ONE = " + String(_myTZX.descriptor[currentBlock].timming.bit_1));
        #endif
        
        // Configuracion de los bits
         _myTZX.descriptor[currentBlock].timming.bitcfg = getBYTE(mFile,currentOffset+15);     


        _myTZX.descriptor[currentBlock].timming.bytecfg = getBYTE(mFile,currentOffset+16);
               
        #ifdef DEBUGMODE
          logln(",ENDIANNESS=");
          logln(String(_myTZX.descriptor[currentBlock].timming.bitcfg & 0b00000001));
        #endif

        _myTZX.descriptor[currentBlock].type = 99;
    }
    
    void analyzeBlockUnknow(int id_num, int currentOffset, int currentBlock)
    {
        _myTZX.descriptor[currentBlock].ID = id_num;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;   
    }

    bool getTZXBlock(File mFile, int currentBlock, int currentID, int currentOffset, int &nextIDoffset)
    {
        bool res = true;
        
        // Inicializamos el descriptor
        _myTZX.descriptor[currentBlock].group = 0;
        _myTZX.descriptor[currentBlock].chk = 0;
        _myTZX.descriptor[currentBlock].delay = 1000;
        _myTZX.descriptor[currentBlock].hasMaskLastByte = false;
        _myTZX.descriptor[currentBlock].header = false;
        _myTZX.descriptor[currentBlock].ID = 0;
        _myTZX.descriptor[currentBlock].lengthOfData = 0;
        _myTZX.descriptor[currentBlock].loop_count = 0;
        _myTZX.descriptor[currentBlock].maskLastByte = 8;
        _myTZX.descriptor[currentBlock].nameDetected = false;
        _myTZX.descriptor[currentBlock].offset = 0;
        _myTZX.descriptor[currentBlock].jump_this_ID = false;
        _myTZX.descriptor[currentBlock].signalLvl = false;


        switch (currentID)
        {
          // ID 10 - Standard Speed Data Block
          case 16:

            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID16(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 5;
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID10STR,35);                  
            }
            else
            {
                res = false;
            }
            break;

          // ID 11- Turbo Speed Data Block
          case 17:
            
            //TIMMING_STABLISHED = true;
            
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID17(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 19;
                //_myTZX.descriptor[currentBlock].typeName = "ID 11 - Speed block";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID11STR,35);
                  
            }
            else
            {
                res = false;
            }
            break;

          // ID 12 - Pure Tone
          case 18:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID18(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
                //_myTZX.descriptor[currentBlock].typeName = "ID 12 - Pure tone";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID12STR,35);
                  
            }
            else
            {
                res = false;
            }
            break;

          // ID 13 - Pulse sequence
          case 19:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID19(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
                //_myTZX.descriptor[currentBlock].typeName = "ID 13 - Pulse seq.";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID13STR,35);
                  
            }
            else
            {
                res = false;
            }
            break;                  

          // ID 14 - Pure Data Block
          case 20:
                       
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID20(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 10 + 1;
                
                //"ID 14 - Pure Data block";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID14STR,35);

            }
            else
            {
                res = false;
            }
            break;                  

          // ID 15 - Direct Recording
          case 21:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID21(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 9 + 1;;  
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID15STR,35);

                // Informacion minima del fichero
                // PROGRAM_NAME = "Audio block (WAV)";
                LAST_SIZE = _myTZX.descriptor[currentBlock].size;
                
                #ifdef DEBUGMODE
                  log("ID 0x21 - DIRECT RECORDING");
                #endif
            }
            else
            {
                res = false;
            }             
            break;

          // ID 18 - CSW Recording
          case 24:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTZX.descriptor[currentBlock].typeName = "ID 18 - CSW Rec";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID18STR,35);
            res=false;              
            break;

          // ID 19 - Generalized Data Block
          case 25:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTZX.descriptor[currentBlock].typeName = "ID 19 - General Data block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID19STR,35);
            res=false;              
            break;

          // ID 20 - Pause and Stop Tape
          case 32:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID32(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + 3;  
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID20STR,35);
                
                #ifdef DEBUGMODE
                  log("ID 0x20 - PAUSE / STOP TAPE");
                  log("-- value: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
                #endif
            }
            else
            {
                res = false;
            }
            break;

          // ID 21 - Group start
          case 33:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID33(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + 2 + _myTZX.descriptor[currentBlock].size;
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID21STR,35);
                
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;

                //_myTZX.descriptor[currentBlock].typeName = "ID 21 - Group start";
            }
            else
            {
                res = false;
            }
            break;                

          // ID 22 - Group end
          case 34:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTZX.descriptor[currentBlock].ID = 34;
                _myTZX.descriptor[currentBlock].playeable = false;
                _myTZX.descriptor[currentBlock].offset = currentOffset;

                nextIDoffset = currentOffset + 1;                      
                //_myTZX.descriptor[currentBlock].typeName = "ID 22 - Group end";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID22STR,35);

                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;
                  
            }
            else
            {
                res = false;
            }
            break;

          // ID 23 - Jump to block
          case 35:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 23 - Jump to block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID23STR,35);
            res=false;
            break;

          // ID 24 - Loop start
          case 36:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTZX.descriptor[currentBlock].ID = 36;
                _myTZX.descriptor[currentBlock].playeable = false;
                _myTZX.descriptor[currentBlock].offset = currentOffset;
                _myTZX.descriptor[currentBlock].loop_count = getWORD(mFile,currentOffset+1);

                #ifdef DEBUGMODE
                    log("LOOP GET: " + String(_myTZX.descriptor[currentBlock].loop_count));
                #endif

                nextIDoffset = currentOffset + 3;                      
                //_myTZX.descriptor[currentBlock].typeName = "ID 24 - Loop start";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID24STR,35);
                  
            }
            else
            {
                res = false;
            }                
            break;


          // ID 25 - Loop end
          case 37:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTZX.descriptor[currentBlock].ID = 37;
                _myTZX.descriptor[currentBlock].playeable = false;
                _myTZX.descriptor[currentBlock].offset = currentOffset;
                LOOP_END = currentOffset;
                
                nextIDoffset = currentOffset + 1;                      
                //_myTZX.descriptor[currentBlock].typeName = "ID 25 - Loop end";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID25STR,35);
                  
            }
            else
            {
                 res = false;
            }                
            break;

          // ID 26 - Call sequence
          case 38:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 26 - Call seq.";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID26STR,35);
            res=false;
            break;

          // ID 27 - Return from sequence
          case 39:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 27 - Return from seq.";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID27STR,35);
            res=false;              
            break;

          // ID 28 - Select block
          case 40:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID40(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
                
                //"ID 28 -Select block";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID28STR,35);
                                        
            }
            else
            {
                res = false;
            }                
            break;

          // ID 2A - Stop the tape if in 48K mode
          case 42:
            analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            nextIDoffset = currentOffset + 1;            

            //_myTZX.descriptor[currentBlock].typeName = "ID 2A - Stop TAPE (48k mode)";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID2ASTR,35);
              
            break;

          // ID 2B - Set signal level
          case 43:
            if (_myTZX.descriptor != nullptr)
            {
              int signalLevel = getBYTE(mFile,currentOffset+5);
              _myTZX.descriptor[currentBlock].ID = 43;
              _myTZX.descriptor[currentBlock].size = 5;
              _myTZX.descriptor[currentBlock].offset = currentOffset + 6;
              _myTZX.descriptor[currentBlock].playeable = false;


              // Inversion de señal    
              INVERSETRAIN = signalLevel;
              hmi.writeString("menuAudio2.polValue.val=" + String(INVERSETRAIN));  
              _myTZX.descriptor[currentBlock].signalLvl = INVERSETRAIN;      

              _hmi.refreshPulseIcons(INVERSETRAIN,ZEROLEVEL);               

              nextIDoffset = currentOffset + 6;            

              //_myTZX.descriptor[currentBlock].typeName = "ID 2B - Set signal level";
              strncpy(_myTZX.descriptor[currentBlock].typeName,ID2BSTR,35);
            }
            else
            {
              res=false;              
            }
            break;

          // ID 30 - Text description
          case 48:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID48(mFile,currentOffset,currentBlock);
                // Siguiente ID
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
                //_myTZX.descriptor[currentBlock].typeName = "ID 30 - Information";
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 31 - Message block
          case 49:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID49(mFile,currentOffset,currentBlock);
                // Siguiente ID
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 2;
                //_myTZX.descriptor[currentBlock].typeName = "ID 30 - Information";
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }       
            break;

          // ID 32 - Archive info
          case 50:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID50(mFile,currentOffset,currentBlock);

                // Siguiente ID
                nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
                //_myTZX.descriptor[currentBlock].typeName = "ID 32 - Archive info";
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 33 - Hardware type
          case 51:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID51(mFile,currentOffset,currentBlock);

                // Siguiente ID
                nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
                //_myTZX.descriptor[currentBlock].typeName = "ID 33- Hardware type";
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 35 - Custom info block
          case 53:
            analyzeID53(mFile,currentOffset,currentBlock);
            nextIDoffset = currentOffset + 0x15 + _myTZX.descriptor[currentBlock].size;

            //_myTZX.descriptor[currentBlock].typeName = "ID 35 - Custom info block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);

            // Esto le indica a los bloque de control de flujo que puede saltarse
            _myTZX.descriptor[currentBlock].jump_this_ID = true;

            break;

          // ID 4B - Standard Speed Data Block
          case 75:

            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID75(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 17;
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID4BSTR,35);
            }   
            else
            {
                res = false;
            }
            break;

          // ID 5A - "Glue" block (90 dec, ASCII Letter 'Z')
          case 90:
            analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            nextIDoffset = currentOffset + 8;            

            //_myTZX.descriptor[currentBlock].typeName = "ID 5A - Glue block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,ID5ASTR,35);
            break;

          default:
            ////SerialHW.println("");
            ////SerialHW.println("ID unknow " + currentID);
            #ifdef DEBUGMODE
              Serial.print("ID unknow 0x");
              Serial.println(currentID,HEX);
            #endif
            // Si no está implementado, salimos
            ID_NOT_IMPLEMENTED = true;
            res = false;
            break;
      }

      return res;
    }

    void getBlockDescriptor(File mFile, File &dscFile, int sizeTZX, bool hasGroupBlocks)
    {
          // Para ello tenemos que ir leyendo el TZX poco a poco
          // Detectaremos los IDs a partir del byte 9 (empezando por offset = 0)
          // Cada bloque empieza por un ID menos el primero 
          // que empieza por ZXTape!
          // Ejemplo ID + bytes
          //
          int startOffset = 10;   // Posición del primero ID, pero el offset
                                  // 0x00 de cada ID, sería el siguiente
          int currentID = 0;
          int nextIDoffset = 0;
          int currentOffset = 0;
          int lenghtDataBlock = 0;
          int currentBlock = 1; 
          bool endTZX = false;
          bool forzeEnd = false;
          bool endWithErrors = false;

          currentOffset = startOffset;
                   
          // Inicializamos
          ID_NOT_IMPLEMENTED = false;

          // Le pasamos el path del fichero, la extension se le asigna despues en 
          // la funcion createBlockDescriptorFile
          // _blDscTZX.createBlockDescriptorFileTZX();
          #ifdef DEBUGMODE
            logln("Creating DSC file"); 
            logln("Status of endTZX: " + String(endTZX));
            logln("Status of forzeEnd: " + String(forzeEnd));
            logln("Status of ID_NOT_IMPLEMENTED: " + String(ID_NOT_IMPLEMENTED));
          #endif

          while (!endTZX && !forzeEnd && !ID_NOT_IMPLEMENTED)
          {
             
              if (ABORT==true)
              {
                  forzeEnd = true;
                  endWithErrors = true;
                  LAST_MESSAGE = "Aborting. No proccess complete.";

                  #ifdef DEBUGMODE
                    Serial.println("Aborting TZX reading");
                  #endif
              }

              // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque completo para el descriptor.
              currentID = getID(mFile, currentOffset);
              
              // Por defecto el bloque no es reproducible
              _myTZX.descriptor[currentBlock].playeable	= false;

              // Ahora dependiendo del ID analizamos. Los ID están en HEX
              // y la rutina devolverá la posición del siguiente ID, así hasta
              // completar todo el fichero
              if (getTZXBlock(mFile, currentBlock, currentID, currentOffset, nextIDoffset))
              {
                  // Copiamos la estructura del bloque, apuntando a ella
                  tTZXBlockDescriptor t = _myTZX.descriptor[currentBlock];
                  // Agregamos la informacion del bloque al fichero del descriptor .dsc
                  _blDscTZX.putBlocksDescriptorTZX(dscFile, currentBlock,t,sizeTZX,hasGroupBlocks);
                  // Incrementamos el bloque
                  currentBlock++;               
              }
              else
              {
                LAST_MESSAGE = "ID block not implemented. Aborting";
                forzeEnd = true;
                endWithErrors = true;
              }

              if(currentBlock > maxAllocationBlocks)
              {
                  #ifdef DEBUGMODE
                    SerialHW.println("Error. TZX not possible to allocate in memory");
                  #endif

                  LAST_MESSAGE = "Error. Not enough memory for TZX/TSX/CDT";
                  endTZX = true;
                  // Salimos
                  return;  
              }
              else
              {}



              if (nextIDoffset >= sizeTZX)
              {
                  // Finalizamos
                  endTZX = true;
                  logln("END TZX analying");

              }
              else
              {
                  currentOffset = nextIDoffset;
              }

              TOTAL_BLOCKS = currentBlock;
          }
          // Nos posicionamos en el bloque 1
          BLOCK_SELECTED = 0;
          _hmi.writeString("currentBlock.val=" + String(BLOCK_SELECTED));

          _myTZX.numBlocks = currentBlock;
          _myTZX.size = sizeTZX;
          
    }

    void process_tzx(File mfile, File &dscFile)
    {
        // Procesamos el fichero
        #ifdef DEBUGMODE
          logln("Processing TZX file");
        #endif

        if (isFileTZX(mfile))
        {
            logln("TZX file detected");
            // Esto lo hacemos para poder abortar
            ABORT=false;
            getBlockDescriptor(mfile,dscFile,_sizeTZX,_myTZX.hasGroupBlocks);
        }
        else
        {
            logln("Error: Not a TZX file");
        }
    }

    void showBufferPlay(uint8_t* buffer, int size, int offset)
    {

        char hexs[20];
        if (size > 10)
        {
          #ifdef DEBUGMODE
            log("");
            logln(" Listing bufferplay.");
            SerialHW.print(buffer[0],HEX);
            SerialHW.print(",");
            SerialHW.print(buffer[1],HEX);
            SerialHW.print(" ... ");
            SerialHW.print(buffer[size-2],HEX);
            SerialHW.print(",");
            SerialHW.print(buffer[size-1],HEX);
            log("");            
          #endif

          sprintf(hexs, "%X", buffer[size-1]);
          dataOffset4 = hexs;
          sprintf(hexs, "%X", buffer[size-2]);
          dataOffset3 = hexs;
          sprintf(hexs, "%X", buffer[1]);
          dataOffset2 = hexs;
          sprintf(hexs, "%X", buffer[0]);
          dataOffset1 = hexs;

          sprintf(hexs, "%X", offset+(size-1));
          Offset4 = hexs;
          sprintf(hexs, "%X", offset+(size-2));
          Offset3 = hexs;
          sprintf(hexs, "%X", offset+1);
          Offset2 = hexs;
          sprintf(hexs, "%X", offset);
          Offset1 = hexs;

        }
        else
        {
            #ifdef DEBUGMODE
              // Solo mostramos la ultima parte
              SerialHW.println("Listing bufferplay. SHORT");
              SerialHW.print(buffer[size-2],HEX);
              SerialHW.print(",");
              SerialHW.print(buffer[size-1],HEX);

              sprintf(hexs, "%X", buffer[size-1]);
              dataOffset4 = hexs;
              sprintf(hexs, "%X", buffer[size-2]);
              dataOffset3 = hexs;

              sprintf(hexs, "%X", offset+(size-1));
              Offset4 = hexs;
              sprintf(hexs, "%X", offset+(size-2));
              Offset3 = hexs;

              log("");
            #endif
        }
    }

    public:

    tTZXBlockDescriptor* getDescriptor()
    {
        return _myTZX.descriptor;
    }
    
    void setDescriptorNull()
    {
            _myTZX.descriptor = nullptr;
    }
    
    void setTZX(tTZX tzx)
    {
        _myTZX = tzx;
    }
    
    void showInfoBlockInProgress(int nBlock)
    {
        // switch(nBlock)
        // {
        //     case 0:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("> PROGRAM HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE0[0];
        //         break;

        //     case 1:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> BYTE HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE1[0];
        //         break;

        //     case 7:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> SCREEN HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE7[0];
        //         break;

        //     case 2:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> BASIC PROGRAM");
        //         #endif
        //         LAST_TYPE = &LASTYPE2[0];
        //         break;

        //     case 3:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> SCREEN");
        //         #endif
        //         LAST_TYPE = &LASTYPE3[0];
        //         break;

        //     case 4:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> BYTE CODE");
        //         #endif
        //         if (LAST_SIZE != 6914)
        //         {
        //             LAST_TYPE = &LASTYPE4_1[0];
        //         }
        //         else
        //         {
        //             LAST_TYPE = &LASTYPE4_2[0];
        //         }
        //         break;

        //     case 5:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> ARRAY.NUM");
        //         #endif
        //         LAST_TYPE = &LASTYPE5[0];
        //         break;

        //     case 6:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           ////SerialHW.println("");
        //           ////SerialHW.println("> ARRAY.CHR");
        //         #endif
        //         LAST_TYPE = &LASTYPE6[0];
        //         break;
        // }
    }

    // void set_SDM(SDmanager sdmTmp)
    // {
    //     //_sdm = sdmTmp;
    //     //ptrSdm = &sdmTmp;
    // }

    // void set_SdFat32(SdFat32 sdf32)
    // {
    //     _sdf32 = sdf32;
    // }

    void set_HMI(HMI hmi)
    {
        _hmi = hmi;
    }

    void set_file(File mFile, int sizeTZX)
    {
      // Pasamos los parametros a la clase
      _mFile = mFile;
      _sizeTZX = sizeTZX;
    }  

    bool getBlocksFromDescriptorFile(File mFileTZX, char* pathDSC, tTZX &myTZX)
    {
        File mFileDsc;
        uint32_t sizeTZX = 0;
        String strLine;
        String str;

        // #ifdef DEBUGMODE
        //   String strTmp;
        // #endif

        char sline[300];
        //int nlines = 0;
        int nparam = 0;
        // Estamos indicando con nblock = -1 que es la cabecera del .dsc
        // y no la queremos.
        int nblock = 0;
        // Agregamos la extension del fichero al path del .TZX

        logln("Trying open DSC file: " + String(pathDSC));

        // Ahora abrimos el fichero .dsc
        mFileDsc = SD_MMC.open(pathDSC,FILE_READ);

        //
        if (mFileDsc)
        {
            // Ahora vamos a ir leyendo linea a linea y metiendo la informacion de los bloques en el descriptor.

            // ------------------------------------------------------------------------------
            // OJO! Si strLine supera el numero de caracteres declarado no entra en el WHILE 
            // ------------------------------------------------------------------------------
            logln("DSC File open: " + String(pathDSC));

            mFileDsc.seek(0);

            while (mFileDsc.available())
            {
              // Capturamos la linea completa
              strLine = mFileDsc.readStringUntil('\n');
              // Eliminamos el retorno de carro
              strLine.replace("\r", "");
              // Convertimos la linea en un array de char
              strLine.toCharArray(sline, sizeof(sline));
              
              #ifdef DEBUGMODE
                logln("Getting line: " + String(strLine));
              #endif

              // Descomponemos la linea en tokens
              // Cogemos el primer token
              str = strtok(sline, ",");

              if (nblock == 0 && nparam == 0 && str != _blDscTZX.dscVersion)
              {
                  LAST_MESSAGE = "DSC file old version.";
                  mFileDsc.close();
                  delay(1500);
                  return false;
              }

              // Ahora cogemos todos los parametros de strLine. Son 33 parametros
              // los vamos leyendo uno a uno.
              nparam = 0;
              // Estamos indicando con nblock = -1 que es la cabecera del .dsc
              // y no la queremos.
              if (nblock > 0)
              {
                  // Ahora recorremos todos los parametros de la linea. El primero id = 0
                  // ya lo hemos leido, nos lo saltamos. Por eso el case 0: no contiene nada.
                  while (str != NULL)
                  {

                    #ifdef DEBUGMODE
                        if (nparam != 12)
                        {
                          // Otros
                          logln("[" + String(nblock) + "] - Param: [" + String(nparam) + "] - Value: " + String(str.toInt()));
                        }
                        else
                        {
                          // Name
                          logln("[" + String(nblock) + "] - Param: [" + String(nparam) + "] - Value: " + String(str));                    
                        }
                    #endif

                    int numPulses = 0;
                    int coff = 0;

                    #ifdef DEBUGMODE
                      logln("Parameter: " + String(nblock) + " - " + String(nparam) + " - " + str);
                    #endif

                    // Vemos en cada momento que posicion de parametro estamos leyendo y lo procesamos
                    switch (nparam)
                    {
                        case 0:
                          // Posicion en el fichero
                          break;

                        case 1:
                          // .ID (Block ID)
                          myTZX.descriptor[nblock].ID = str.toInt();
                          break;

                        case 2:
                          // .chk (Checksum)
                          myTZX.descriptor[nblock].chk = str.toInt();
                          break;

                        case 3:
                          // .delay
                          myTZX.descriptor[nblock].delay = str.toInt();
                          break;

                        case 4:
                          // .group
                          myTZX.descriptor[nblock].group = str.toInt();
                          break;

                        case 5:
                          // .hasMaskLastByte
                          myTZX.descriptor[nblock].hasMaskLastByte = (str.toInt() == 0) ? false : true;
                          break;

                        case 6:
                          // .header
                          myTZX.descriptor[nblock].header = (str.toInt() == 0) ? false : true;             
                          break;

                        case 7:
                          // .jump_this_ID
                          myTZX.descriptor[nblock].jump_this_ID = (str.toInt() == 0) ? false : true;
                          break;

                        case 8:
                          // .lengthOfData
                          myTZX.descriptor[nblock].lengthOfData = str.toInt();
                          break;

                        case 9:
                          // .loop_count
                          myTZX.descriptor[nblock].loop_count = str.toInt();
                          break;

                        case 10:
                          // .offset
                          myTZX.descriptor[nblock].offset = str.toInt();
                          break;

                        case 11:
                          // .offsetData
                          myTZX.descriptor[nblock].offsetData = str.toInt();
                          break;

                        case 12:
                          // .name
                          strcpy(myTZX.descriptor[nblock].name,str.c_str());
                          break;

                        case 13:
                          // .nameDetected
                          myTZX.descriptor[nblock].nameDetected = (str.toInt() == 0) ? false : true;
                          break;

                        case 14:
                          // .pauseAfterThisBlock
                          myTZX.descriptor[nblock].pauseAfterThisBlock = str.toInt();
                          break;

                        case 15:
                          // .playeable
                          myTZX.descriptor[nblock].playeable = (str.toInt() == 0) ? false : true;
                          break;

                        case 16:
                          // .samplingRate
                          myTZX.descriptor[nblock].samplingRate = str.toInt();
                          break;

                        case 17:
                          // .screen
                          myTZX.descriptor[nblock].screen = (str.toInt() == 0) ? false : true;
                          break;

                        case 18:
                          // .silent
                          myTZX.descriptor[nblock].silent = str.toInt();
                          break;

                        case 19:
                          // .size
                          myTZX.descriptor[nblock].size = str.toInt();
                          break;

                        case 20:
                          // .timming.bit_0
                          myTZX.descriptor[nblock].timming.bit_0 = str.toInt();
                          break;

                        case 21:
                          // .timming.bit_1
                          myTZX.descriptor[nblock].timming.bit_1 = str.toInt();
                          break;

                        case 22:
                          // .timming.pilot_len
                          myTZX.descriptor[nblock].timming.pilot_len = str.toInt();
                          break;

                        case 23:
                          // .timming.pilot_num_pulses
                          myTZX.descriptor[nblock].timming.pilot_num_pulses = str.toInt();
                          break;

                        case 24:
                          // .timming.pulse_seq_num_pulses
                          numPulses = str.toInt();
                          myTZX.descriptor[nblock].timming.pulse_seq_num_pulses = numPulses;

                          // En el caso de que el bloque sea 0x13 hago esto.
                          // para cargar el array con la secuencia.
                          if (myTZX.descriptor[nblock].ID == 19)
                          {
                              // Calculamos la posicion de la secuencia de pulsos
                              coff = myTZX.descriptor[nblock].offset + 2;
                              // Reservamos memoria.
                              _myTZX.descriptor[nblock].timming.pulse_seq_array = (int*)ps_calloc(numPulses + 1,sizeof(int));
                              
                              // Cogemos los pulsos
                              logln("ID13 - Num. pulses: " + String(numPulses));
                              
                              for (int i=0;i<numPulses;i++)
                              {
                                _myTZX.descriptor[nblock].timming.pulse_seq_array[i] = getWORD(mFileTZX,coff);
                                coff += 2;
                              }
                          }                          
                          break;

                        case 25:
                          // .timming.pure_tone_len
                          myTZX.descriptor[nblock].timming.pure_tone_len = str.toInt();
                          break;

                        case 26:
                          // .timming.pure_tone_num_pulses
                          myTZX.descriptor[nblock].timming.pure_tone_num_pulses = str.toInt();
                          break;

                        case 27:
                          // .timming.sync_1
                          myTZX.descriptor[nblock].timming.sync_1 = str.toInt();
                          break;

                        case 28:
                          // .timming.sync_2
                          myTZX.descriptor[nblock].timming.sync_2 = str.toInt();
                          break;

                        case 29:
                          // .typeName
                          strcpy(myTZX.descriptor[nblock].typeName,str.c_str());
                          break;

                        case 30:
                          // .type
                          myTZX.descriptor[nblock].type = str.toInt();
                          break;

                        case 31:
                          // .timming.bitcfg
                          myTZX.descriptor[nblock].timming.bitcfg = str.toInt();
                          break;

                        case 32:
                          // .timming.bytecfg
                          myTZX.descriptor[nblock].timming.bytecfg = str.toInt();
                          break;

                        case 33:
                          // .maskLastByte
                          myTZX.descriptor[nblock].maskLastByte = str.toInt();
                          break;

                        case 34:
                          // El TZX tiene bloques Group start y Group End
                          myTZX.hasGroupBlocks = str.toInt();
                          break;

                        case 35:
                          // sizeTZX actual
                          myTZX.size = str.toInt();
                          sizeTZX = myTZX.size;
                          break;

                        case 36:  // Ojo! Esto es la posicion del dato en la linea!!!!
                          // Signal level solo queda afectado si hay un ID43
                          if (myTZX.descriptor[nblock].ID == 43)
                          {
                              logln("");
                              log("Signal LEVEL");
                              myTZX.descriptor[nblock].signalLvl = str.toInt();
                
                              // Inversion de señal      
                              INVERSETRAIN = myTZX.descriptor[nblock].signalLvl;
                                  
                              _hmi.refreshPulseIcons(INVERSETRAIN,ZEROLEVEL);                          
                              
                          }
                          break;

                        default:
                          break;
                    }

                    // Cogemos el parametro siguiente
                    str = strtok(NULL, ",");
                    nparam++;
                  }

                  if (nblock > 0 && nparam==0)
                  {
                    logln("Error. No params were read.");
                    mFileDsc.close();
                    return false;
                  }
                  // 
              }
              nblock++;
              TOTAL_BLOCKS = nblock;
            }

            if (TOTAL_BLOCKS == 0)
            {
              logln("Error, any block captured.");
              mFileDsc.close();
              return false;
            }

            logln("Total blocks captured from DSC: " + String(TOTAL_BLOCKS));

            // Nos posicionamos en el bloque 1
            BLOCK_SELECTED = 0;
            _hmi.writeString("currentBlock.val=" + String(BLOCK_SELECTED));
            myTZX.numBlocks = TOTAL_BLOCKS;  

            // Listamos el contenido de myTZX
            #ifdef DEBUGMODE
              logln("Listing all blocks captured from DSC");
              for (int i=0;i<TOTAL_BLOCKS;i++)
              {
                  String strTmp = "Block " + String(i) + " - ID: " + String(myTZX.descriptor[i].ID) + " - Name: " + String(myTZX.descriptor[i].name) + " - Size: " + String(myTZX.descriptor[i].size) + " - Offset: " + String(myTZX.descriptor[i].offset) + " - DataOffset: " + String(myTZX.descriptor[i].offsetData);
                  logln(strTmp);
              }   
              logln("End listing all blocks captured from DSC");
            #endif


            mFileDsc.close();
            // Decimos que ha ido bien
            return true;
        }
        else
        {
          
          // Decimos que NO ha ido bien
          logln("Error opening DSC file.");
          mFileDsc.close();
          return false;
        }
    }
    
    void proccessingDescriptor(File &dscFile, File &tzxFile, char* pathDSC)
    {

        LAST_MESSAGE = "Analyzing file. Capturing blocks";
        
        if (SD_MMC.exists(pathDSC))
        {
          SD_MMC.remove(pathDSC);
        }
        
       _blDscTZX.createBlockDescriptorFileTZX(dscFile,pathDSC); 
        // creamos un objeto TZXproccesor
        set_file(tzxFile, _rlen);
        // Lo procesamos y creamos DSC
        process_tzx(tzxFile, dscFile);
        // Cerramos para guardar los cambios
        dscFile.close();

        if (_myTZX.descriptor != nullptr && !ID_NOT_IMPLEMENTED)
        {
            // Entregamos información por consola
            strcpy(LAST_NAME,"              ");
        }
        else
        {
          LAST_MESSAGE = "Error in TZX/TSX/CDT or ID not supported";
        }      
    }

    void process(char* path) 
    {
      
      File tzxFile;
      File dscFile;

      
      PROGRAM_NAME_DETECTED = false;
      PROGRAM_NAME = "";
      PROGRAM_NAME_2 = "";
    
      MULTIGROUP_COUNT = 1;

      // Abrimos el fichero
      tzxFile = SD_MMC.open(path, FILE_READ);
      char* pathDSC = path;

      // Pasamos el fichero abierto a la clase. Elemento global
      _mFile = tzxFile;
      // Obtenemos su tamaño total
      _rlen = tzxFile.available();
      _myTZX.size = tzxFile.size();

      if (_rlen != 0)
      {
          FILE_IS_OPEN = true;

          // Asignamos el path al objeto blockDescriptor
          strcat(pathDSC,".dsc");
          
          if (_blDscTZX.existBlockDescriptorFile(dscFile, pathDSC))
          {
              // No lo creamos mas, ahora cogemos todo el descriptor del fichero
              // y nos ahorramos el procesado
              LAST_MESSAGE = "Loading blocks from DSC";

              if (!getBlocksFromDescriptorFile(tzxFile, pathDSC, _myTZX))
              {
                // Lanzamos entonces la extraccion directa desde el .TZX
                proccessingDescriptor(dscFile,tzxFile,pathDSC);
                logln("All blocks captured from TZX file");
              }
              else
              {
                logln("All blocks captured from DSC file");
              }
          }
          else
          {
            // Lanzamos la extraccion directa desde el TZX
            // y creamos el nuevo .dsc
            proccessingDescriptor(dscFile,tzxFile,pathDSC);
            logln("All blocks captured from TZX file");
          }
      }
      else
      {
          FILE_IS_OPEN = false;
          LAST_MESSAGE = "Error in TZX/TSX/CDT file has 0 bytes";
      }              
    }

    void initialize()
    {
        // if (_myTZX.descriptor != nullptr)
        // {
        //   //free(_myTZX.descriptor);
        //   //free(_myTZX.name);
        //   //_myTZX.descriptor = nullptr;

        // }          

        strncpy(_myTZX.name,"          ",10);
        _myTZX.numBlocks = 0;
        _myTZX.size = 0;

        CURRENT_BLOCK_IN_PROGRESS = 1;
        BLOCK_SELECTED = 1;
        PROGRESS_BAR_BLOCK_VALUE = 0;
        PROGRESS_BAR_TOTAL_VALUE = 0;
        PROGRAM_NAME_DETECTED = false;
    }

    void terminate()
    {
      //free(_myTZX.descriptor);
      //_myTZX.descriptor = nullptr;
      //free(_myTZX.name);
      strncpy(_myTZX.name,"          ",10);
      _myTZX.numBlocks = 0;
      _myTZX.size = 0;      
      // free(_myTZX.descriptor);
      // _myTZX.descriptor = nullptr;            
    }

    void playBlock(tTZXBlockDescriptor descriptor)
    {

        #ifdef DEBUGMODE
          log("Pulse len: " + String(descriptor.timming.pilot_len));
          log("Pulse nums: " + String(descriptor.timming.pilot_num_pulses));
        #endif

        uint8_t* bufferPlay = nullptr;

        //int pulsePilotDuration = descriptor.timming.pulse_pilot * descriptor.timming.pilot_num_pulses;
        int blockSizeSplit = SIZE_FOR_SPLIT;

        if (descriptor.size > blockSizeSplit)
        {
            #ifdef DEBUGMODE
              logln("Partiendo la pana");
              logln("Partiendo la pana");
              logln("Partiendo la pana");
              logln("Partiendo la pana");
            #endif

            int totalSize = descriptor.size;
            PARTITION_SIZE = totalSize;
            
            int offsetBase = descriptor.offsetData;
            int newOffset = 0;
            int blocks = totalSize / blockSizeSplit;
            int lastBlockSize = totalSize - (blocks * blockSizeSplit);

            #ifdef DEBUGMODE
              logln("");
              log("> Offset DATA:         " + String(offsetBase));
              log("> Total size del DATA: " + String(totalSize));
              log("> Total blocks: " + String(blocks));
              log("> Last block size: " + String(lastBlockSize));
            #endif

            if(!DIRECT_RECORDING)
            {
              // BTI 0
              _zxp.BIT_0 = descriptor.timming.bit_0;
              // BIT1                                          
              _zxp.BIT_1 = descriptor.timming.bit_1;
            }

            TOTAL_PARTS = blocks;

            // Recorremos el vector de particiones del bloque.
            for (int n=0;n < blocks;n++)
            {
              PARTITION_BLOCK = n;
              
              #ifdef DEBUGMODE
                logln("");
                log("Sending partition");
              #endif

              // Calculamos el offset del bloque
              newOffset = offsetBase + (blockSizeSplit*n);
              PRG_BAR_OFFSET_INI = newOffset;

              // Accedemos a la SD y capturamos el bloque del fichero
              bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
              readFileRange(_mFile,bufferPlay,newOffset,blockSizeSplit, true);

              // Mostramos en la consola los primeros y últimos bytes
              showBufferPlay(bufferPlay,blockSizeSplit,newOffset);     
              
              // Reproducimos la partición n, del bloque.
              if (n==0)
              {
                  if(!DIRECT_RECORDING)
                  {
                    _zxp.playDataBegin(bufferPlay, blockSizeSplit,descriptor.timming.pilot_len,descriptor.timming.pilot_num_pulses);
                  }
                  else
                  {
                    #ifdef DEBUGMODE
                      logln("");
                      log("> Playing DR block - Splitted - begin");
                    #endif                    
                    _zxp.playDRBlock(bufferPlay,blockSizeSplit,false);


                  }
              }
              else
              {
                   if(!DIRECT_RECORDING)
                  {                
                    _zxp.playDataPartition(bufferPlay, blockSizeSplit);
                  }
                  else
                  {
                    #ifdef DEBUGMODE
                      logln("");
                      log("> Playing DR block - Splitted - middle");
                    #endif                    
                    _zxp.playDRBlock(bufferPlay,blockSizeSplit,false);
                  }
              }

              free(bufferPlay);
            }

            // Ultimo bloque
            //log("Particion [" + String(blocks) + "/" + String(blocks) + "]");

            // Calculamos el offset del último bloque
            newOffset = offsetBase + (blockSizeSplit*blocks);
            PRG_BAR_OFFSET_INI = newOffset;

            blockSizeSplit = lastBlockSize;
            // Accedemos a la SD y capturamos el bloque del fichero
            bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
            readFileRange(_mFile,bufferPlay, newOffset,blockSizeSplit, true);

            // Mostramos en la consola los primeros y últimos bytes
            showBufferPlay(bufferPlay,blockSizeSplit,newOffset);         
            
            // Reproducimos el ultimo bloque con su terminador y silencio si aplica              
            if(!DIRECT_RECORDING)
            {                
              _zxp.playDataEnd(bufferPlay, blockSizeSplit);
            }
            else
            {
              #ifdef DEBUGMODE
                logln("");
                log("> Playing DR block - Splitted - Last");
              #endif                    
             
              _zxp.playDRBlock(bufferPlay,blockSizeSplit,true);
            }                                              

            free(bufferPlay);       
        }
        else
        {
            PRG_BAR_OFFSET_INI = descriptor.offsetData;  

            // Si es mas pequeño que el SPLIT, se reproduce completo.
            bufferPlay = (uint8_t*)ps_calloc(descriptor.size,sizeof(uint8_t));
            readFileRange(_mFile,bufferPlay, descriptor.offsetData, descriptor.size, true);

            showBufferPlay(bufferPlay,descriptor.size,descriptor.offsetData);
            verifyChecksum(_mFile,descriptor.offsetData,descriptor.size);    

            // BTI 0
            _zxp.BIT_0 = descriptor.timming.bit_0;
            // BIT1                                          
            _zxp.BIT_1 = descriptor.timming.bit_1;
            //
            if(!DIRECT_RECORDING)
            {                
                _zxp.playData(bufferPlay, descriptor.size, descriptor.timming.pilot_len,descriptor.timming.pilot_num_pulses);
            }
            else
            {
                #ifdef DEBUGMODE
                  logln("> Playing DR block - Full");
                #endif                    

                _zxp.playDRBlock(bufferPlay,descriptor.size,true);
            }
            
            free(bufferPlay);           
        }
    }

    bool isPlayeable(int id)
    {
      // Definimos los ID playeables (en decimal)
      bool res = false;
      int playeableListID[] = {16, 17, 18, 19, 20, 21, 24, 25, 32, 35, 36, 37, 38, 39, 40, 42, 43, 75, 90};
      for (int i = 0; i < 19; i++) 
      {
        if (playeableListID[i] == id) 
        { 
          res = true; //Inidicamos que lo hemos encontrado
          break;
        }
      }

      return res;
    }

    void prepareID4B(int currentBlock, File mFile, int nlb, int vlb, int ntb, int vtb, int pzero, int pone, int offset, int ldatos, bool begin)
    {
        // Generamos la señal de salida
        int pulsosmaximos;
        int npulses[2];
        int vpulse[2];

        npulses[0] = pzero/2;
        npulses[1] = pone/2;
        vpulse[0] = (_myTZX.descriptor[currentBlock].timming.bit_0);
        vpulse[1] = (_myTZX.descriptor[currentBlock].timming.bit_1);
        pulsosmaximos = (_myTZX.descriptor[currentBlock].timming.pilot_num_pulses) + ((npulses[vlb] * nlb) + 128 + (npulses[vtb] * ntb)) * ldatos;


        // Reservamos memoria dinamica
        _myTZX.descriptor[currentBlock].timming.pulse_seq_array = (int*)ps_calloc(pulsosmaximos+1,sizeof(int));
        // _myTZX.descriptor[currentBlock].timming.pulse_seq_array = new int[pulsosmaximos + 1]; 

        #ifdef DEBUGMODE
          logln("");
          log(" - Numero de pulsos: " + String(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses));
          logln("");
          log(" - Pulsos maximos: "+String(pulsosmaximos));
          logln("");
          log(" - Longitud de los datos: " + String(ldatos));
        #endif

        // metemos el pilot SOLO AL PRINCIPIO
        int i;
        int p;

        if (begin)
        {
          for (p=0; p < (_myTZX.descriptor[currentBlock].timming.pilot_num_pulses); p++)
          {
              _myTZX.descriptor[currentBlock].timming.pulse_seq_array[p] = _myTZX.descriptor[currentBlock].timming.pilot_len;
          }

          i=p;
        }
        else
        {
          i=0;
        }   

        #ifdef DEBUGMODE
          log(">> Bucle del pilot - Iteraciones: " + String(p));
        #endif

        // metemos los datos
        uint8_t *bRead = (uint8_t*)ps_calloc(ldatos + 1,sizeof(uint8_t));
        int lenPulse;
        
        // Leemos el bloque definido por la particion (ldatos) del fichero
        getBlock(mFile, bRead, offset, ldatos);

        for (int i2=0;i2<ldatos;i2++) // por cada byte
        {

            for (int i3=0;i3<nlb;i3++)  // por cada bit de inicio
            {
                for (int i4=0;i4<npulses[vlb];i4++)
                { 
                    lenPulse=vpulse[vlb];
                    _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                  
                    i++;
                    _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                  
                    i++;
                }
            }

            //metemos el byte leido 
            for (int n=0;n < 8;n++)                  
            {
                // Obtenemos el bit a transmitir
                uint8_t bitMasked = bitRead(bRead[i2], 0 + n);

                // Si el bit leido del BYTE es un "1"
                if(bitMasked == 1)
                {
                    // Procesamos "1"
                    for (int b1=0;b1<npulses[1];b1++)
                    {
                        _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[1];
                        
                        i++;
                        _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[1];
                      
                        i++;
                    }                       
                }
                else
                {
                    // En otro caso
                    // procesamos "0"
                    for (int b0=0;b0<npulses[0];b0++)
                    { 
                      _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[0];
                      
                      i++;
                      _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[0];
                    
                      i++;
                    }
                }
            }

            for (int i3=0;i3<ntb;i3++)
            {
                for (int i4=0;i4<npulses[vtb];i4++)
                { lenPulse=vpulse[vtb];
                    _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                    
                    i++;
                    _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                    
                    i++;
                }
            }
        }

        _myTZX.descriptor[currentBlock].timming.pulse_seq_num_pulses=i;
        
        #ifdef DEBUGMODE
          logln("datos: "+String(ldatos));
          logln("pulsos: "+String(i));
        
          logln("---------------------------------------------------------------------");
        #endif    

        free(bRead);
    }
    
    int getIDAndPlay(int i)
    {
        // Inicializamos el buffer de reproducción. Memoria dinamica
        uint8_t* bufferPlay = nullptr;
        int dly = 0;
        int newPosition = -1;
        
        // Cogemos la mascara del ultimo byte
        if (_myTZX.descriptor[i].hasMaskLastByte)
        {
          _zxp.set_maskLastByte(_myTZX.descriptor[i].maskLastByte);
        }
        else
        {
          _zxp.set_maskLastByte(8);
        }
        

        // Ahora lo voy actualizando a medida que van avanzando los bloques.
        PROGRAM_NAME_2 = _myTZX.descriptor[BLOCK_SELECTED].name;

        // El offset final sera el offsetdata + size
        PRG_BAR_OFFSET_END = _myTZX.descriptor[i].offsetData + _myTZX.descriptor[i].size;
        
        DIRECT_RECORDING = false;

        uint32_t sr=0;
        double divd=0.0;
        

        switch (_myTZX.descriptor[i].ID)
        {
            case 21:
              DIRECT_RECORDING = true;
              // 
              // PROGRAM_NAME = "Audio block (WAV)";
              LAST_SIZE = _myTZX.descriptor[i].size;  
              //
              // Congemos la configuracion por defecto ya establecida
              // para despues cambiar los parametros que necesitemos
              // y el resto se conserven.
              new_sr = kitStream.audioInfo();
              // new_sr2 = kitStream.defaultConfig();              
              // Calculamos el sampling rate desde el bloque ID 0x15
              divd = double(_myTZX.descriptor[i].samplingRate) * (1.0/3500000.0);
              sr = divd > 0 ? round(1.0 / (divd)) : 44100;
              //
              logln("Value in TZX srate: " + String(_myTZX.descriptor[i].samplingRate));
              logln("Custom sampling rate: " + String(sr));

              // Si el sampling rate es menor de 8kHz, lo cambiamos a 22KHz
              if (sr < 8000)
              {
                sr = STANDARD_SR_8_BIT_MACHINE;
                logln("Error. Changing sampling rate: " + String(sr));
              }

              // Cambiamos el sampling rate en el HW
              new_sr.sample_rate = sr;
              kitStream.setAudioInfo(new_sr);      
   
              // Indicamos el sampling rate
              LAST_MESSAGE = "Direct recording at " + String(sr) + "Hz";
              _hmi.writeString("tape.lblFreq.txt=\"" + String(int(sr/1000)) + "KHz\"" );


              //
              SAMPLING_RATE = sr;
              BIT_DR_0 =_myTZX.descriptor[i].samplingRate;
              BIT_DR_1 = _myTZX.descriptor[i].samplingRate;

              playBlock(_myTZX.descriptor[i]);
              break;
            
            case 36:  
              //Loop start ID 0x24
              // El loop controla el cursor de bloque por tanto debe estar el primero
              LOOP_PLAYED = 0;
              // El primer bloque a repetir es el siguiente, pero ponemos "i" porque el FOR lo incrementa. 
              // Entonces este bloque no se lee mas, ya se continua repitiendo el bucle.
              BL_LOOP_START = i;
              LOOP_COUNT = _myTZX.descriptor[i].loop_count;
              break;

            case 37:
              //Loop end ID 0x25
              #ifdef DEBUGMODE
                  log("LOOP: " + String(LOOP_PLAYED) + " / " + String(LOOP_COUNT));
                  log("------------------------------------------------------");
              #endif

              if (LOOP_PLAYED < LOOP_COUNT)
              {
                // Volvemos al primner bloque dentro del loop
                newPosition = BL_LOOP_START;
                LOOP_PLAYED++;
                //return newPosition;        
              }     
              break;

            case 32:
              // ID 0x20 - Pause or STOP the TAP
              // Hacemos un delay
              dly = _myTZX.descriptor[i].pauseAfterThisBlock;

              // Vemos si es una PAUSA o una parada del TAPE
              if (dly == 0)
              {                       
                  // Finalizamos el ultimo bit
                  if(LAST_SILENCE_DURATION==0)
                  {
                    _zxp.silence(2000);
                  }

                  // Inicializamos la polarización de la señal
                  //EDGE_EAR_IS = POLARIZATION;   

                  //_zxp.closeBlock();

                  // PAUSE
                  // Pausamos la reproducción a través del HMI 

                  // hmi.writeString("click btnPause,1"); 
                  // LAST_MESSAGE = "Playing PAUSE.";
                  LAST_GROUP = "[STOP BLOCK]";

                  // MODEWAV = false;
                  // PLAY = false;
                  // STOP = false;
                  // PAUSE = true;

                  // Lanzamos una PAUSA automatica
                  AUTO_PAUSE = true;
                  _hmi.verifyCommand("PAUSE");

                  // Dejamos preparado el sieguiente bloque
                  CURRENT_BLOCK_IN_PROGRESS++;
                  BLOCK_SELECTED++;
                  //WAITING_FOR_USER_ACTION = true;

                  if (BLOCK_SELECTED >= _myTZX.numBlocks)
                  {
                      // Reiniciamos
                      CURRENT_BLOCK_IN_PROGRESS = 1;
                      BLOCK_SELECTED = 1;
                  }

                  // Enviamos info al HMI.
                  // _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size,_myTZX.descriptor[BLOCK_SELECTED].playeable);                        
                  //return newPosition;
              }
              else
              {
                  // En otro caso. delay
                  _zxp.silence(dly);                        
              }                    
              break;

            case 33:
              // Comienza multigrupo
              LAST_GROUP = "[META BLK: " + String(_myTZX.descriptor[i].group) + "]";
              LAST_BLOCK_WAS_GROUP_START = true;
              LAST_BLOCK_WAS_GROUP_END = false;
              break;

            case 34:
              //LAST_GROUP = &INITCHAR[0];
              // Finaliza el multigrupo
              LAST_GROUP = "...";
              LAST_BLOCK_WAS_GROUP_END = true;
              LAST_BLOCK_WAS_GROUP_START = false;
              break;

            case 43:
              // Inversion de señal            
              if(INVERSETRAIN)
              {
                  // Para que empiece en DOWN tiene que ser POLARIZATION = UP
                  // esto seria una señal invertida
                  // EDGE_EAR_IS = down;
                  // POLARIZATION = down;
                  //INVERSETRAIN = true;                
                  hmi.writeString("menuAudio2.polValue.val=1");
                  // FIRST_BLOCK_INVERTED = true;
              }
              else
              {
                hmi.writeString("menuAudio2.polValue.val=0");
              }
              _hmi.refreshPulseIcons(INVERSETRAIN,ZEROLEVEL);               
              break;

            default:
              // Cualquier otro bloque entra por aquí, pero
              // hay que comprobar que sea REPRODUCIBLE

              // Reseteamos el indicador de META BLOCK
              // (añadido el 26/03/2024)
              // LAST_GROUP = "...";

              if (_myTZX.descriptor[i].playeable)
              {
                

                    //Silent
                    _zxp.silent = _myTZX.descriptor[i].pauseAfterThisBlock;        
                    //SYNC1
                    _zxp.SYNC1 = _myTZX.descriptor[i].timming.sync_1;
                    //SYNC2
                    _zxp.SYNC2 = _myTZX.descriptor[i].timming.sync_2;
                    //PULSE PILOT (longitud del pulso)
                    _zxp.PILOT_PULSE_LEN = _myTZX.descriptor[i].timming.pilot_len;
                    // BTI 0
                    _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
                    // BIT1                                          
                    _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;



                    // Obtenemos el nombre del bloque
                    strncpy(LAST_NAME,_myTZX.descriptor[i].name,14);
                    LAST_SIZE = _myTZX.descriptor[i].size;
                    strncpy(LAST_TYPE,_myTZX.descriptor[i].typeName,35);

                    #ifdef DEBUGMODE
                      logln("Bl: " + String(i) + "Playeable block");
                      logln("Bl: " + String(i) + "Name: " + LAST_NAME);
                      logln("Bl: " + String(i) + "Type name: " + LAST_TYPE);
                      logln("Bl: " + String(i) + "pauseAfterThisBlock: " + String(_zxp.silent));
                      logln("Bl: " + String(i) + "Sync1: " + String(_zxp.SYNC1));
                      logln("Bl: " + String(i) + "Sync2: " + String(_zxp.SYNC2));
                      logln("Bl: " + String(i) + "Pilot pulse len: " + String(_zxp.PILOT_PULSE_LEN));
                      logln("Bl: " + String(i) + "Bit0: " + String(_zxp.BIT_0));
                      logln("Bl: " + String(i) + "Bit1: " + String(_zxp.BIT_1));
                    #endif

                    // Almacenmas el bloque en curso para un posible PAUSE
                    if (LOADING_STATE != 2) 
                    {
                        CURRENT_BLOCK_IN_PROGRESS = i;
                        BLOCK_SELECTED = i;
                        PROGRESS_BAR_BLOCK_VALUE = 0;
                    }
                    else
                    {
                        //Paramos la reproducción.

                        PAUSE = false;
                        STOP = true;
                        PLAY = false;

                        i = _myTZX.numBlocks+1;

                        LOOP_PLAYED = 0;
                        // EDGE_EAR_IS = down;
                        LOOP_START = 0;
                        LOOP_END = 0;
                        BL_LOOP_START = 0;
                        BL_LOOP_END = 0;

                        //return newPosition;
                    }

                    //Ahora vamos lanzando bloques dependiendo de su tipo
                    // Actualizamos HMI
                    // _hmi.setBasicFileInformation(_myTZX.descriptor[i].name,_myTZX.descriptor[i].typeName,_myTZX.descriptor[i].size,_myTZX.descriptor[i].playeable);

                    // Reproducimos el fichero
                    if (_myTZX.descriptor[i].type == 0) 
                    {
                        //
                        // LOS BLOQUES TYPE == 0 
                        //
                        // Llevan una cabecera y después un bloque de datos

                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            //PULSE PILOT
                            _myTZX.descriptor[i].timming.pilot_len = DPILOT_LEN;
                            
                            // Reproducimos el bloque - PLAY
                            playBlock(_myTZX.descriptor[i]);
                            //free(bufferPlay);
                            break;

                          case 17:
                            //PULSE PILOT
                            // Llamamos a la clase de reproducción
                            playBlock(_myTZX.descriptor[i]);
                            break;
                        }

                    } 
                    else if (_myTZX.descriptor[i].type == 1 || _myTZX.descriptor[i].type == 7) 
                    {

                        // Llamamos a la clase de reproducción
                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            //Standard data - ID-10                          
                            _myTZX.descriptor[i].timming.pilot_len = DPILOT_LEN;                                  
                            playBlock(_myTZX.descriptor[i]);
                            break;

                          case 17:
                            // Speed data - ID-11                            
                            playBlock(_myTZX.descriptor[i]);
                            break;
                        }
                                              
                    } 
                    else if (_myTZX.descriptor[i].type == 99)
                    {
                        //
                        // Son bloques especiales de TONO GUIA o SECUENCIAS para SYNC
                        //
                        //int num_pulses = 0;

                        // Variables para el ID 0x4B (75)
                        int nlb;
                        int vlb;
                        int ntb;
                        int vtb;
                        int pzero;
                        int pone;

                        int ldatos;
                        int offset;

                        int bufferD;
                        int partitions;
                        int lastPartitionSize;
                        int silence;

                        bool begin = false;

                        switch (_myTZX.descriptor[i].ID)
                        {

                            case 75:

                              BYTES_TOBE_LOAD = _myTZX.size;

                              //configuracion del byte
                              pzero=((_myTZX.descriptor[i].timming.bitcfg & 0b11110000)>>4);
                              pone=((_myTZX.descriptor[i].timming.bitcfg & 0b00001111));
                              nlb=((_myTZX.descriptor[i].timming.bytecfg & 0b11000000)>>6);
                              vlb=((_myTZX.descriptor[i].timming.bytecfg & 0b00100000)>>5);    
                              ntb=((_myTZX.descriptor[i].timming.bytecfg & 0b00011000)>>3);
                              vtb=((_myTZX.descriptor[i].timming.bitcfg & 0b00000100)>>2);

                              #ifdef DEBUGMODE
                                logln("");
                                log("ID 0x4B:");
                                log("PULSES FOR ZERO = " + String(pzero));
                                log(" - " + String(_myTZX.descriptor[i].timming.bit_0) + " - ");
                                log(",PULSES FOR ONE = " + String(pone));
                                log(" - " + String(_myTZX.descriptor[i].timming.bit_1) + " - ");
                                log(",NUMBERS OF LEADING BITS = "+String(nlb));
                                log(",VALUE OF LEADING BITS = "+String(vlb));
                                log(",NUMBER OF TRAILING BITS = "
                                +String(ntb));
                                log(",VALUE OF TRAILING BITS = "+String(vtb));
                              #endif


                              // Conocemos la longitud total del bloque de datos a reproducir
                              ldatos = _myTZX.descriptor[i].lengthOfData;
                              // Nos quedamos con el offset inicial
                              offset = _myTZX.descriptor[i].offsetData;
                              
                              // Informacion para la barra de progreso
                              PRG_BAR_OFFSET_INI = offset;  //offset del DATA
                              PRG_BAR_OFFSET_END = offset + _myTZX.descriptor[i].lengthOfData;

                              bufferD = 1024;  // Buffer de BYTES de datos convertidos a samples

                              partitions = ldatos / bufferD;
                              lastPartitionSize = ldatos - (partitions * bufferD);

                              silence = _myTZX.descriptor[i].pauseAfterThisBlock;
                              
                              #ifdef DEBUGMODE
                                log(",SILENCE = "+String(silence) + " ms");                              

                                logln("");
                                log("Total data parts: " + String(partitions));
                                logln("");
                                log("----------------------------------------");
                              #endif          

                              //TSX_PARTITIONED = false;
                              PROGRESS_BAR_BLOCK_VALUE = 0;

                              if (ldatos > bufferD)
                              {
                                  //TSX_PARTITIONED = true;
                                  for(int n=0;n<partitions && !STOP && !PAUSE;n++)
                                  {
                                      if (n==0)
                                      {
                                        begin = true;
                                      }
                                      else
                                      {
                                        begin = false;
                                      }

                                      #ifdef DEBUGMODE
                                        logln("");
                                        log("Part [" + String(n) + "] - offset: ");
                                        logHEX(offset);
                                      #endif

                                      prepareID4B(i,_mFile,nlb,vlb,ntb,vtb,pzero,pone, offset, bufferD, begin);
                                      // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición                                                                    
                                      
                                      PRG_BAR_OFFSET_INI = 0;
                                      PRG_BAR_OFFSET_END = ldatos;

                                      _zxp.playCustomSequence(_myTZX.descriptor[i].timming.pulse_seq_array,_myTZX.descriptor[i].timming.pulse_seq_num_pulses,0.0);                                                                           
                                      // Avanzamos el puntero por el fichero
                                      offset += bufferD;

                                      PROGRESS_BAR_BLOCK_VALUE = ((PRG_BAR_OFFSET_INI + (offset)) * 100 ) / PRG_BAR_OFFSET_END;

                                      // Liberamos el array
                                      free(_myTZX.descriptor[i].timming.pulse_seq_array);
                                      // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
                                  }


                                  if (!STOP && !PAUSE)
                                  {
                                      // Ultima particion
                                      PRG_BAR_OFFSET_INI = 0;
                                      PRG_BAR_OFFSET_END = ldatos;

                                      PROGRESS_BAR_BLOCK_VALUE = 0;

                                      prepareID4B(i,_mFile,nlb,vlb,ntb,vtb,pzero,pone, offset, lastPartitionSize,false);
                                      // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición                                                                    

                                      _zxp.playCustomSequence(_myTZX.descriptor[i].timming.pulse_seq_array,_myTZX.descriptor[i].timming.pulse_seq_num_pulses,0.0); 
                                      // Liberamos el array
                                      free(_myTZX.descriptor[i].timming.pulse_seq_array);
                                      // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
                                      PROGRESS_BAR_BLOCK_VALUE = ((PRG_BAR_OFFSET_INI + (ldatos)) * 100 ) / PRG_BAR_OFFSET_END;
                                      // Pausa despues de bloque                                  
                                      _zxp.silence(silence,0.0);
                                  }

                                  #ifdef DEBUGMODE
                                    logln("Finish");
                                  #endif

                              }
                              else
                              {
                                  #ifdef DEBUGMODE
                                    logln(" - Only one data part");
                                  #endif

                                  if (!STOP && !PAUSE)
                                  {                                      

                                      PRG_BAR_OFFSET_INI = 0;
                                      PRG_BAR_OFFSET_END = ldatos;
                                      PROGRESS_BAR_BLOCK_VALUE = 0;
                                      
                                      // Una sola partiase 43ion
                                      PRG_BAR_OFFSET_INI = _myTZX.descriptor[i].offsetData;
                                      
                                      prepareID4B(i,_mFile,nlb,vlb,ntb,vtb,pzero,pone, offset, ldatos,true);

                                      // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición                                                                    
                                      _zxp.playCustomSequence(_myTZX.descriptor[i].timming.pulse_seq_array,_myTZX.descriptor[i].timming.pulse_seq_num_pulses,0); 
                                      PROGRESS_BAR_BLOCK_VALUE = ((PRG_BAR_OFFSET_INI + (ldatos)) * 100 ) / PRG_BAR_OFFSET_END;

                                      // Liberamos el array
                                      free(_myTZX.descriptor[i].timming.pulse_seq_array); 
                                      // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
                                      // Pausa despues de bloque 
                                      _zxp.silence(silence,0.0); 
                                  }                              
                              }
                             
                              break; 

                            case 18:
                              // ID 0x12 - Reproducimos un tono puro. Pulso repetido n veces       
                              #ifdef DEBUGMODE
                                  log("ID 0x12:");
                              #endif                    
                              _zxp.playPureTone(_myTZX.descriptor[i].timming.pure_tone_len,_myTZX.descriptor[i].timming.pure_tone_num_pulses);
                              break;

                            case 19:
                              // ID 0x13 - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición 
                               #ifdef DEBUGMODE
                                  logln("ID 0x13:");
                                  logln("Num. pulses: " + String(_myTZX.descriptor[i].timming.pulse_seq_num_pulses));
                                  for(int j=0;j<_myTZX.descriptor[i].timming.pulse_seq_num_pulses;j++)
                                  {
                                    SerialHW.print(_myTZX.descriptor[i].timming.pulse_seq_array[j],HEX);
                                    SerialHW.print(",");
                                  }
                               #endif                                                                     
                              _zxp.playCustomSequence(_myTZX.descriptor[i].timming.pulse_seq_array,_myTZX.descriptor[i].timming.pulse_seq_num_pulses);                                                                           
                              break;                          

                            case 20:
                              // ID 0x14
                              int blockSizeSplit = SIZE_FOR_SPLIT;

                              if (_myTZX.descriptor[i].size > blockSizeSplit)
                              {

                                  int totalSize = _myTZX.descriptor[i].size;
                                  
                                  PARTITION_SIZE = totalSize;

                                  int offsetBase = _myTZX.descriptor[i].offsetData;
                                  int newOffset = 0;
                                  double blocks = totalSize / blockSizeSplit;
                                  int lastBlockSize = totalSize - (blocks * blockSizeSplit);

                                  // BTI 0
                                  _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
                                  // BIT1                                          
                                  _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;

                                  PRG_BAR_OFFSET_INI = _myTZX.descriptor[i].offsetData;
                                  //BYTES_BASE = _myTZX.descriptor[i].offsetData;

                                  // Recorremos el vector de particiones del bloque.
                                  for (int n=0;n < blocks;n++)
                                  {
                                    // Calculamos el offset del bloque
                                    newOffset = offsetBase + (blockSizeSplit*n);
                                    // Accedemos a la SD y capturamos el bloque del fichero
                                    bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
                                    readFileRange(_mFile,bufferPlay, newOffset, blockSizeSplit, true);
                                    // Mostramos en la consola los primeros y últimos bytes
                                    showBufferPlay(bufferPlay,blockSizeSplit,newOffset);     

                                    PRG_BAR_OFFSET_INI = newOffset;
                                    //BYTES_BASE = _myTZX.descriptor[i].offsetData;

                                    // PROGRESS_BAR_TOTAL_VALUE = (PRG_BAR_OFFSET_INI * 100 ) / BYTES_TOBE_LOAD ;
                                    // PROGRESS_BAR_BLOCK_VALUE = (PRG_BAR_OFFSET_INI * 100 ) / (BYTES_BASE + BYTES_IN_THIS_BLOCK);
  
                                    #ifdef DEBUGMODE                                            
                                      log("Block. " + String(n));
                                      SerialHW.print(newOffset,HEX);                                              
                                      SerialHW.print(" - ");                                              
                                      SerialHW.print(newOffset+blockSizeSplit,HEX);                                              
                                      log("");
                                      for (int j=0;j<blockSizeSplit;j++)
                                      {
                                          SerialHW.print(bufferPlay[j],HEX);
                                          SerialHW.print(",");                                              
                                      }
                                      // (añadido el 26/03/2024)
                                      // Reproducimos la partición n, del bloque.
                                      _zxp.playDataPartition(bufferPlay, blockSizeSplit);                                      

                                    #else
                                      // Reproducimos la partición n, del bloque.
                                      _zxp.playDataPartition(bufferPlay, blockSizeSplit);                                      
                                    #endif

                                    free(bufferPlay);

                                  }

                                  // Ultimo bloque
                                  // Calculamos el offset del último bloque
                                  newOffset = offsetBase + (blockSizeSplit*blocks);
                                  blockSizeSplit = lastBlockSize;
                                  PRG_BAR_OFFSET_INI = newOffset;
                                  //BYTES_BASE = _myTZX.descriptor[i].offsetData;

                                  // PROGRESS_BAR_TOTAL_VALUE = (PRG_BAR_OFFSET_INI * 100 ) / BYTES_TOBE_LOAD ;
                                  // PROGRESS_BAR_BLOCK_VALUE = (PRG_BAR_OFFSET_INI * 100 ) / (_myTZX.descriptor[i].offset + BYTES_IN_THIS_BLOCK);

                                  // Accedemos a la SD y capturamos el bloque del fichero
                                  bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
                                  readFileRange(_mFile,bufferPlay, newOffset,blockSizeSplit, true);
                                  // Mostramos en la consola los primeros y últimos bytes
                                  showBufferPlay(bufferPlay,blockSizeSplit,newOffset);         

                                  #ifdef DEBUGMODE                                            
                                    log("Block. LAST");
                                    SerialHW.print(newOffset,HEX);
                                    SerialHW.print(" - ");                                              
                                    SerialHW.print(newOffset+blockSizeSplit,HEX);                                              
                                    log("");
                                    for (int j=0;j<blockSizeSplit;j++)
                                    {
                                        SerialHW.print(bufferPlay[j],HEX);
                                        SerialHW.print(",");                                              
                                    }
                                  #else
                                    // Reproducimos el ultimo bloque con su terminador y silencio si aplica
                                    _zxp.playPureData(bufferPlay, blockSizeSplit);                                     
                                  #endif

                                  free(bufferPlay); 
                              }
                              else
                              {
                                  bufferPlay = (uint8_t*)ps_calloc(_myTZX.descriptor[i].size,sizeof(uint8_t));
                                  readFileRange(_mFile,bufferPlay, _myTZX.descriptor[i].offsetData, _myTZX.descriptor[i].size, true);

                                  showBufferPlay(bufferPlay,_myTZX.descriptor[i].size,_myTZX.descriptor[i].offsetData);
                                  verifyChecksum(_mFile,_myTZX.descriptor[i].offsetData,_myTZX.descriptor[i].size);    

                                  // BTI 0
                                  _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
                                  // BIT1                                          
                                  _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;
                                  //
                                  _zxp.playPureData(bufferPlay, _myTZX.descriptor[i].size);
                                  free(bufferPlay);                                  
                              }                               
                              break;                          
                        }
                    }
                    else 
                    {
                        //
                        // Otros bloques de datos SIN cabecera no contemplados anteriormente - DATA
                        //

                        int blockSize = _myTZX.descriptor[i].size;


                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            // ID 0x10

                            //PULSE PILOT
                            _myTZX.descriptor[i].timming.pilot_len = DPILOT_LEN;
                            playBlock(_myTZX.descriptor[i]);
                            break;

                          case 17:
                            // ID 0x11
                            //PULSE PILOT
                            playBlock(_myTZX.descriptor[i]);
                            // Bloque de datos BYTE
                            break;
                        }

                    }                  
            }
            else
            {
              #ifdef DEBUGMODE
                logln("");
                logln("Bl: " + String(i) + " - Not playeable block");
                logln("");
              #endif
            }

            break;

        }      

        return newPosition;
    }

    void play()
    {

        LOOP_PLAYED = 0;

        int firstBlockToBePlayed = 0;
        int dly = 0;

        // Inicializamos 02.03.2024
        LOOP_START = 0;
        LOOP_END = 0;
        BL_LOOP_START = 0;
        BL_LOOP_END = 0;

        if (_myTZX.descriptor != nullptr)
        {           
              // Entregamos información por consola
              TOTAL_BLOCKS = _myTZX.numBlocks;
              strcpy(LAST_NAME,"              ");

              // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
              firstBlockToBePlayed = BLOCK_SELECTED;

              // Reiniciamos
              BYTES_TOBE_LOAD = _rlen;
              BYTES_LOADED = 0;              

              // Recorremos ahora todos los bloques que hay en el descriptor
              //-------------------------------------------------------------
              #ifdef DEBUGMODE
                  logln("Total blocks " + String(_myTZX.numBlocks));
                  logln("First block " + String(firstBlockToBePlayed));
                  logln("");
              #endif

              for (int i = firstBlockToBePlayed; i < _myTZX.numBlocks; i++) 
              {               

                  // Lo guardo por si hago PAUSE y vuelvo a reproducir el bloque
                  _myTZX.descriptor[i].edge = EDGE_EAR_IS;

                  BLOCK_SELECTED = i;  

                  if (BLOCK_SELECTED == 0) 
                  {
                    PRG_BAR_OFFSET_INI = 0;
                  }
                  else
                  {
                    PRG_BAR_OFFSET_INI = _myTZX.descriptor[BLOCK_SELECTED].offset;
                  }

                  _hmi.setBasicFileInformation(_myTZX.descriptor[i].ID,_myTZX.descriptor[i].group,_myTZX.descriptor[i].name,_myTZX.descriptor[i].typeName,_myTZX.descriptor[i].size,_myTZX.descriptor[i].playeable);
                  int new_i = getIDAndPlay(i);
                  // Entonces viene cambiada de un loop
                  if (new_i != -1)
                  {
                    i = new_i;
                  }

                  if (LOADING_STATE == 2 || LOADING_STATE == 3)
                  {
                    break;
                  }

                  // #ifdef MSX_REMOTE_PAUSE
                  // if (digitalRead(GPIO_MSX_REMOTE_PAUSE) == LOW && !REM_DETECTED)
                  // {
                  //   LAST_MESSAGE = "Remote pause detected";
                  //   LOADING_STATE = 3; // REM PAUSE del bloque actual
                  //   REM_DETECTED = true;
                  //   AUTO_PAUSE = true;
                  //   PAUSE = true;
                  //   STOP = false;
                  //   PLAY = false;
                  //   ABORT = false;
                    
                  //   // Dejamos preparado el sieguiente bloque
                  //   CURRENT_BLOCK_IN_PROGRESS++;
                  //   BLOCK_SELECTED++;
                  //   //WAITING_FOR_USER_ACTION = true;

                  //   if (BLOCK_SELECTED >= _myTZX.numBlocks)
                  //   {
                  //       // Reiniciamos
                  //       CURRENT_BLOCK_IN_PROGRESS = 1;
                  //       BLOCK_SELECTED = 1;
                  //   }

                  //   #ifdef DEBUGMODE
                  //     logln("REM detected. Delay 5000");
                  //   #endif

                  //   break;
                  // }
                  // #endif

                  if (stopOrPauseRequest())
                  {
                    // Forzamos la salida
                    i = _myTZX.numBlocks + 1;
                    break;
                  }
              }
              //---------------------------------------------------------------

              // En el caso de no haber parado manualmente, es por finalizar
              // la reproducción
              if (LOADING_STATE == 1) 
              {

                  if(LAST_SILENCE_DURATION==0)
                  {_zxp.silence(2000);}

                  // Inicializamos la polarización de la señal
                  // EDGE_EAR_IS = POLARIZATION;       

                  // Paramos
                  #ifdef DEBUGMODE
                      logAlert("AUTO STOP launch.");
                  #endif

                  PLAY = false;
                  PAUSE = false;
                  STOP = true;
                  REC = false;
                  ABORT = true;
                  EJECT = false;

                  BLOCK_SELECTED = 0;
                  BYTES_LOADED = 0; 

                  AUTO_STOP = true;

                  _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].ID,_myTZX.descriptor[BLOCK_SELECTED].group,_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size,_myTZX.descriptor[BLOCK_SELECTED].playeable);
              }

              // Cerrando
              // reiniciamos el edge del ultimo pulse
              LOOP_PLAYED = 0;
              //EDGE_EAR_IS = down;
              LOOP_START = 0;
              LOOP_END = 0;
              LOOP_COUNT = 0;
              BL_LOOP_START = 0;
              BL_LOOP_END = 0;
              MULTIGROUP_COUNT = 1;
              WAITING_FOR_USER_ACTION = false;
        }
        else
        {
            LAST_MESSAGE = "No file selected.";
            _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].ID,_myTZX.descriptor[BLOCK_SELECTED].group,_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size,_myTZX.descriptor[BLOCK_SELECTED].playeable);
        }        

    }

    TZXprocessor()
    {
        // Constructor de la clase
        strncpy(_myTZX.name,"          ",10);
        _myTZX.numBlocks = 0;
        _myTZX.size = 0;
        _myTZX.descriptor = nullptr;
    } 
};


