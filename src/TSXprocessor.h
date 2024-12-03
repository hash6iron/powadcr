/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TSXproccesor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

      Colaboración: @imulilla - Israel Mula. 2024 / https://github.com/imulilla
          
    Descripción:
    Conjunto de recursos para la gestión de ficheros .TSX para MSX

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

class TSXprocessor
{

    private:

    const char ID10STR[35] = "ID 10 - Standard block            ";
    const char ID11STR[35] = "ID 11 - Speed block               ";
    const char ID12STR[35] = "ID 12 - Pure tone                 ";
    const char ID13STR[35] = "ID 13 - Pulse seq.                ";
    const char ID14STR[35] = "ID 14 - Pure data                 ";
    const char ID4BSTR[35] = "ID 4B - MSX                       ";
    const char IDXXSTR[35] = "ID                                ";

    const int maxAllocationBlocks = 4000;

    // Procesador de audio output
    ZXProcessor _zxp;

    HMI _hmi;

    // Definicion de un TSX
    SdFat32 _sdf32;
    tTSX _myTSX;
    File32 _mFile;
    int _sizeTSX;
    int _rlen;

    int CURRENT_LOADING_BLOCK = 0;

    bool stopOrPauseRequest()
    {
        if (LOADING_STATE == 1)
        {
            if (STOP==true)
            {
                LOADING_STATE = 2; // Parada del bloque actual
                return true;
            }
            else if (PAUSE==true)
            {
                LOADING_STATE = 3; // Parada del bloque actual
                return true;
            }
        }      

        return false;   
    }

    uint8_t calculateChecksum(uint8_t* bBlock, int startByte, int numBytes)
    {
        // Calculamos el checksum de un bloque de bytes
        uint8_t newChk = 0;

        // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
        
        for (int n=startByte;n<(startByte+numBytes);n++)
        {
            newChk = newChk ^ bBlock[n];
        }

        return newChk;
    }      

    char* getNameFromStandardBlock(uint8_t* header)
    {
        // Obtenemos el nombre del bloque cabecera
        static char prgName[11];

        // Extraemos el nombre del programa desde un ID10 - Standard block
        for (int n=2;n<12;n++)
        {   
            if (header[n] < 32)
            {
              // Los caracteres por debajo del espacio 0x20 se sustituyen
              // por " " ó podemos poner ? como hacía el Spectrum.
              prgName[n-2] = ' ';
            }
            else
            {
              prgName[n-2] = (char)header[n];
            }
        }


        // Pasamos la cadena de caracteres
        return prgName;

      }

    bool isHeaderTSX(File32 TSXFile)
    {
      if (TSXFile != 0)
      {

          // Capturamos la cabecera
          uint8_t* bBlock = (uint8_t*)ps_calloc(10+1,sizeof(uint8_t));
          sdm.readFileRange32(TSXFile,bBlock,0,10,false); 

          // Obtenemos la firma del TSX
          char signTSXHeader[9];

          // Analizamos la cabecera
          // Extraemos el nombre del programa
          for (int n=0;n<7;n++)
          {   
              signTSXHeader[n] = (char)bBlock[n];
          }
          
          free(bBlock);

          //Aplicamos un terminador a la cadena de char
          signTSXHeader[7] = '\0';
          //Convertimos a String
          String signStr = String(signTSXHeader);
          
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

    bool isFileTSX(File32 TSXFile)
    {
        if (TSXFile != 0)
        {
          // Capturamos el nombre del fichero en szName
          char szName[254];
          TSXFile.getName(szName,254);
          String fileName = String(szName);

          if (fileName != "")
          {
                //String fileExtension = szNameStr.substring(szNameStr.length()-3);
                fileName.toUpperCase();
                
                if (fileName.indexOf(".TSX") != -1)
                {
                    //La extensión es TSX pero ahora vamos a comprobar si internamente también lo es
                    if (isHeaderTSX(TSXFile))
                    { 
                      //Cerramos el fichero porque no se usará mas.
                      TSXFile.close();
                      return true;
                    }
                    else
                    {
                      //Cerramos el fichero porque no se usará mas.
                      TSXFile.close();
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

    int getWORD(File32 mFile, int offset)
    {
        int sizeDW = 0;
        uint8_t* ptr1 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        uint8_t* ptr2 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        sdm.readFileRange32(mFile,ptr1,offset+1,1,false);
        sdm.readFileRange32(mFile,ptr2,offset,1,false);
        sizeDW = (256*ptr1[0]) + ptr2[0];
        free(ptr1);
        free(ptr2);

        return sizeDW;    
    }

    int getBYTE(File32 mFile, int offset)
    {
        int sizeB = 0;
        uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));

        sdm.readFileRange32(mFile,ptr,offset,1,false);
        sizeB = ptr[0];
        free(ptr);

        return sizeB;      
    }

    int getNBYTE(File32 mFile, int offset, int n)
    {
        int sizeNB = 0;
        uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
        
        for (int i = 0; i<n;i++)
        {
            sdm.readFileRange32(mFile,ptr,offset+i,1,false);
            sizeNB += pow(2,(8*i)) * (ptr[0]);  
        }

        free(ptr);
        return sizeNB;           
    }

    int getID(File32 mFile, int offset)
    {
        // Obtenemos el ID
        return(getBYTE(mFile,offset));
    }

    void getBlock(File32 mFile, uint8_t* &block, int offset, int size)
    {
        //Entonces recorremos el TSX. 
        // La primera cabecera SIEMPRE debe darse.
        // Obtenemos el bloque
        // uint8_t* block = (uint8_t*)ps_calloc(size+1,sizeof(uint8_t));
        sdm.readFileRange32(mFile,block,offset,size,false);
    }

    bool verifyChecksum(File32 mFile, int offset, int size)
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

    void analyzeID16(File32 mFile, int currentOffset, int currentBlock)
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
        
        tTSXBlockDescriptor blockDescriptor; 
        _myTSX.descriptor[currentBlock].ID = 16;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset;
       
        // Timming de la ROM
        _myTSX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
        _myTSX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
        _myTSX.descriptor[currentBlock].timming.sync_2 = DSYNC2;
        _myTSX.descriptor[currentBlock].timming.bit_0 = DBIT_0;
        _myTSX.descriptor[currentBlock].timming.bit_1 = DBIT_1;

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+1);

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTSX.descriptor[currentBlock].lengthOfData = getWORD(mFile,currentOffset+3);

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTSX.descriptor[currentBlock].offsetData = currentOffset + 5;

        // Vamos a verificar el flagByte
        int flagByte = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData);
        // Y cogemos para ver el tipo de cabecera
        int typeBlock = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData+1);

        // Si el flagbyte es menor a 0x80
        if (flagByte < 128)
        {

            _myTSX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_HEADER;
            
            // Es una cabecera
            if (typeBlock == 0)
            {
                // Es una cabecera PROGRAM: BASIC
              _myTSX.descriptor[currentBlock].header = true;
              _myTSX.descriptor[currentBlock].type = 0;

              //_myTSX.name = "TSX";
              if (!PROGRAM_NAME_DETECTED)
              {
                  uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
                  getBlock(mFile,block,_myTSX.descriptor[currentBlock].offsetData,19);
                  gName = getNameFromStandardBlock(block);
                  PROGRAM_NAME = String(gName);
                  PROGRAM_NAME_DETECTED = true;
                  free(block);
              }

              // Almacenamos el nombre del bloque
              strncpy(_myTSX.descriptor[currentBlock].name,_myTSX.name,14);

            }
            else if (typeBlock == 1)
            {
                _myTSX.descriptor[currentBlock].header = true;
                _myTSX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 2)
            {
                _myTSX.descriptor[currentBlock].header = true;
                _myTSX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 3)
            {

              if (_myTSX.descriptor[currentBlock].lengthOfData == 6914)
              {

                  _myTSX.descriptor[currentBlock].screen = true;
                  _myTSX.descriptor[currentBlock].type = 7;

                  // Almacenamos el nombre del bloque
                  uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
                  getBlock(mFile,block,_myTSX.descriptor[currentBlock].offsetData,19);
                  gName = getNameFromStandardBlock(block);
                  strncpy(_myTSX.descriptor[currentBlock].name,gName,14);
                  free(block);
              }
              else
              {
                // Es un bloque BYTE
                _myTSX.descriptor[currentBlock].type = 1;     

                // Almacenamos el nombre del bloque
                uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
                getBlock(mFile,block,_myTSX.descriptor[currentBlock].offsetData,19);
                gName = getNameFromStandardBlock(block);
                strncpy(_myTSX.descriptor[currentBlock].name,gName,14);                       
                free(block);
              }
            }
        }
        else
        {
            _myTSX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_DATA;

            if (typeBlock == 3)
            {

              if (_myTSX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  _myTSX.descriptor[currentBlock].screen = true;
                  _myTSX.descriptor[currentBlock].type = 3;
              }
              else
              {
                  // Es un bloque BYTE
                  _myTSX.descriptor[currentBlock].type = 4;                
              }
            }
            else
            {
                // Es un bloque BYTE
                _myTSX.descriptor[currentBlock].type = 4;
            }            
        } 

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTSX.descriptor[currentBlock].size = _myTSX.descriptor[currentBlock].lengthOfData; 

        // Por defecto en ID10 no tiene MASK_LAST_BYTE pero lo inicializamos 
        // para evitar problemas con el ID11
        _myTSX.descriptor[currentBlock].hasMaskLastByte = false;
    }
        
    void analyzeID17(File32 mFile, int currentOffset, int currentBlock)
    {   
        char* gName;

        // Turbo data

        _myTSX.descriptor[currentBlock].ID = 17;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset;

        // Entonces
        // Timming de PULSE PILOT
        _myTSX.descriptor[currentBlock].timming.pilot_len = getWORD(mFile,currentOffset+1);

        // Timming de SYNC_1
        _myTSX.descriptor[currentBlock].timming.sync_1 = getWORD(mFile,currentOffset+3);

        // Timming de SYNC_2
        _myTSX.descriptor[currentBlock].timming.sync_2 = getWORD(mFile,currentOffset+5);

        // Timming de BIT 0
        _myTSX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+7);

        // Timming de BIT 1
        _myTSX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+9);        

        // Timming de PILOT TONE
        _myTSX.descriptor[currentBlock].timming.pilot_num_pulses = getWORD(mFile,currentOffset+11);

        // Cogemos el byte de bits of the last byte
        _myTSX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+13);
        _myTSX.descriptor[currentBlock].hasMaskLastByte = true;


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
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+14);

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTSX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+16,3);

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTSX.descriptor[currentBlock].offsetData = currentOffset + 19;

        // Almacenamos el nombre del bloque
        //_myTSX.descriptor[currentBlock].name = getNameFromStandardBlock(getBlock(mFile,_myTSX.descriptor[currentBlock].offsetData,19));
        
        // Vamos a verificar el flagByte

        int flagByte = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData);
        int typeBlock = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData+1);

        if (flagByte < 128)
        {
            // Es una cabecera
            uint8_t* block = (uint8_t*)ps_calloc(19+1,sizeof(uint8_t));
            getBlock(mFile,block,_myTSX.descriptor[currentBlock].offsetData,19);
            gName = getNameFromStandardBlock(block);
            strncpy(_myTSX.descriptor[currentBlock].name,gName,14);                       
            free(block);  
                      
            if (typeBlock == 0)
            {
                // Es una cabecera BASIC
                _myTSX.descriptor[currentBlock].header = true;
                _myTSX.descriptor[currentBlock].type = 0;

            }
            else if (typeBlock == 1)
            {
                _myTSX.descriptor[currentBlock].header = true;
                _myTSX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 2)
            {
                _myTSX.descriptor[currentBlock].header = true;
                _myTSX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 3)
            {
              if (_myTSX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  _myTSX.descriptor[currentBlock].screen = true;
                  _myTSX.descriptor[currentBlock].type = 7;
              }
              else
              {
                  // Es un bloque BYTE
                  _myTSX.descriptor[currentBlock].type = 1;                
              }
            }
        }
        else
        {
            // Es un bloque BYTE
            strncpy(_myTSX.descriptor[currentBlock].name,"              ",14);
            _myTSX.descriptor[currentBlock].type = 4;
        }

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTSX.descriptor[currentBlock].size = _myTSX.descriptor[currentBlock].lengthOfData; 
    }

    void analyzeID18(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID-12 - Pure tone

        _myTSX.descriptor[currentBlock].ID = 18;
        _myTSX.descriptor[currentBlock].playeable = true;
        //_myTSX.descriptor[currentBlock].forSetting = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset; 
        _myTSX.descriptor[currentBlock].timming.pure_tone_len = getWORD(mFile,currentOffset+1);
        _myTSX.descriptor[currentBlock].timming.pure_tone_num_pulses = getWORD(mFile,currentOffset+3); 

        #ifdef DEBUGMODE
            SerialHW.println("");
            SerialHW.print("Pure tone setting: Ts: ");
            SerialHW.print(_myTSX.descriptor[currentBlock].timming.pure_tone_len,HEX);
            SerialHW.print(" / Np: ");
            SerialHW.print(_myTSX.descriptor[currentBlock].timming.pure_tone_num_pulses,HEX);        
            SerialHW.println("");
        #endif

        // Esto es para que tome los bloques como especiales
        _myTSX.descriptor[currentBlock].type = 99;     
        // Guardamos el size
        _myTSX.descriptor[currentBlock].size = 4;
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = 0;        
        _myTSX.descriptor[currentBlock].silent = 0;        
        //     
    }

    void analyzeID19(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID-13 - Pulse sequence

        _myTSX.descriptor[currentBlock].ID = 19;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset;    

        int num_pulses = getBYTE(mFile,currentOffset+1);
        _myTSX.descriptor[currentBlock].timming.pulse_seq_num_pulses = num_pulses;
        
        // Reservamos memoria.
        _myTSX.descriptor[currentBlock].timming.pulse_seq_array = (int*)ps_calloc(num_pulses + 1,sizeof(int));
        // _myTSX.descriptor[currentBlock].timming.pulse_seq_array = new int[num_pulses]; 
        
        // Tomamos ahora las longitudes
        int coff = currentOffset+2;

        #ifdef DEBUGMODE
          log("ID-13: Pulse sequence");
        #endif

        for (int i=0;i<num_pulses;i++)
        {
          int lenPulse = getWORD(mFile,coff);
          _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;

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
        _myTSX.descriptor[currentBlock].type = 99;     
        // Guardamos el size (bytes)
        _myTSX.descriptor[currentBlock].size = (num_pulses*2)+1; 
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = 0;        
        _myTSX.descriptor[currentBlock].silent = 0;        
        //
    }

    void analyzeID20(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID-14 - Pure Data Block

        _myTSX.descriptor[currentBlock].ID = 20;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset; 


        // Timming de la ROM
        _myTSX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
        _myTSX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
        _myTSX.descriptor[currentBlock].timming.sync_2 = DSYNC2;

        // Timming de BIT 0
        _myTSX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+1);

          //////SerialHW.println("");
          // //SerialHW.print(",BIT_0=");
          // //SerialHW.print(_myTSX.descriptor[currentBlock].timming.bit_0, HEX);

        // Timming de BIT 1
        _myTSX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+3);        

          //////SerialHW.println("");
          // //SerialHW.print(",BIT_1=");
          // //SerialHW.print(_myTSX.descriptor[currentBlock].timming.bit_1, HEX);

        // Cogemos el byte de bits of the last byte
        _myTSX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+5);
        _myTSX.descriptor[currentBlock].hasMaskLastByte = true;        
        //_zxp.set_maskLastByte(_myTSX.descriptor[currentBlock].maskLastByte);

          // //SerialHW.println("");
          // //SerialHW.print(",MASK LAST BYTE=");
          // //SerialHW.print(_myTSX.descriptor[currentBlock].maskLastByte, HEX);


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
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+6);

        // //SerialHW.println("");
        // //SerialHW.print("Pause after block: " + String(_myTSX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
        // //SerialHW.print(_myTSX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
        // //SerialHW.println("");

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTSX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+8,3);

        // //SerialHW.println("");
        // //SerialHW.println("Length of data: ");
        // //SerialHW.print(String(_myTSX.descriptor[currentBlock].lengthOfData));

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTSX.descriptor[currentBlock].offsetData = currentOffset + 11;

        ////SerialHW.println("");
        ////SerialHW.print("Offset of data: 0x");
        ////SerialHW.print(_myTSX.descriptor[currentBlock].offsetData,HEX);
        ////SerialHW.println("");

        // Vamos a verificar el flagByte

        int flagByte = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData);
        int typeBlock = getBYTE(mFile,_myTSX.descriptor[currentBlock].offsetData+1);
        //
        //_myTSX.descriptor[currentBlock].name = &INITCHAR[0];
        strncpy(_myTSX.descriptor[currentBlock].name,"              ",14);        
        //_myTSX.descriptor[currentBlock].type = 4;
        _myTSX.descriptor[currentBlock].type = 99; // Importante
        _myTSX.descriptor[currentBlock].header = false;

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTSX.descriptor[currentBlock].size = _myTSX.descriptor[currentBlock].lengthOfData;        
    }

    void analyzeID21(File32 mFile, int currentOffset, int currentBlock)
    {
      // ID-15 - Direct recording

        _myTSX.descriptor[currentBlock].ID = 21;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset;

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTSX.descriptor[currentBlock].samplingRate = getWORD(mFile,currentOffset+1);
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+3);
        // Esto es muy importante para el ID 0x15
        // Used bits (samples) in last byte of data (1-8) (e.g. if this is 2, only first two samples of the last byte will be played)
        _myTSX.descriptor[currentBlock].hasMaskLastByte = true;
        _myTSX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+5);

        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTSX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+6,3);

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTSX.descriptor[currentBlock].offsetData = currentOffset + 9;

        // Vamos a verificar el flagByte

        _myTSX.descriptor[currentBlock].type = 0; // Importante
        _myTSX.descriptor[currentBlock].header = false;

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTSX.descriptor[currentBlock].size = _myTSX.descriptor[currentBlock].lengthOfData;         
    }

    void analyzeID32(File32 mFile, int currentOffset, int currentBlock)
    {
        // Pause or STOP the TAPE
        
        _myTSX.descriptor[currentBlock].ID = 32;
        _myTSX.descriptor[currentBlock].playeable = false;
        _myTSX.descriptor[currentBlock].offset = currentOffset;    

        // Obtenemos el valor de la pausa
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+1);          
    }

    void analyzeID40(File32 mFile, int currentOffset, int currentBlock)
    {
        // Size block
        int sizeBlock = 0;
        int numSelections = 0;

        // Obtenemos la dirección del siguiente offset
        _myTSX.descriptor[currentBlock].ID = 40;
        _myTSX.descriptor[currentBlock].playeable = false;
        _myTSX.descriptor[currentBlock].offset = currentOffset + 2;

        sizeBlock = getWORD(mFile,currentOffset+1);
        numSelections = getBYTE(mFile,currentOffset+3);

        _myTSX.descriptor[currentBlock].size = sizeBlock+2;
        
    }

    void analyzeID48(File32 mFile, int currentOffset, int currentBlock)
    {
        // 0x30 - Text Description
        int sizeTextInformation = 0;

        _myTSX.descriptor[currentBlock].ID = 48;
        _myTSX.descriptor[currentBlock].playeable = false;
        _myTSX.descriptor[currentBlock].offset = currentOffset;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        ////SerialHW.println("");
        ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTSX.descriptor[currentBlock].size = sizeTextInformation + 1;
    }

    void analyzeID49(File32 mFile, int currentOffset, int currentBlock)
        {
            // 0x31 - Message block
            int sizeTextInformation = 0;

            _myTSX.descriptor[currentBlock].ID = 49;
            _myTSX.descriptor[currentBlock].playeable = false;
            _myTSX.descriptor[currentBlock].offset = currentOffset;

            sizeTextInformation = getBYTE(mFile,currentOffset+2);

            ////SerialHW.println("");
            ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
            
            // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
            // el bloque comienza en el offset del ID y acaba en
            // offset[ID] + tamaño_bloque
            _myTSX.descriptor[currentBlock].size = sizeTextInformation + 1;
        }

    void analyzeID50(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID 0x32 - Archive Info
        int sizeBlock = 0;

        _myTSX.descriptor[currentBlock].ID = 50;
        _myTSX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTSX.descriptor[currentBlock].offset = currentOffset+3;

        sizeBlock = getWORD(mFile,currentOffset+1);

        //////SerialHW.println("");
        //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTSX.descriptor[currentBlock].size = sizeBlock;         
    }    

    void analyzeID51(File32 mFile, int currentOffset, int currentBlock)
    {
        // 0x33 - Hardware Information block
        int sizeBlock = 0;

        _myTSX.descriptor[currentBlock].ID = 51;
        _myTSX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTSX.descriptor[currentBlock].offset = currentOffset;

        // El bloque completo mide, el numero de maquinas a listar
        // multiplicado por 3 byte por maquina listada
        sizeBlock = getBYTE(mFile,currentOffset+1) * 3;

        //////SerialHW.println("");
        //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTSX.descriptor[currentBlock].size = sizeBlock-1;         
    }

    void analyzeID33(File32 mFile, int currentOffset, int currentBlock)
    {
        // Group start
        int sizeTextInformation = 0;

        _myTSX.descriptor[currentBlock].ID = 33;
        _myTSX.descriptor[currentBlock].playeable = false;
        _myTSX.descriptor[currentBlock].offset = currentOffset+2;
        _myTSX.descriptor[currentBlock].group = MULTIGROUP_COUNT;
        MULTIGROUP_COUNT++;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        ////SerialHW.println("");
        ////SerialHW.println("ID33 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTSX.descriptor[currentBlock].size = sizeTextInformation;
    }

    void analyzeID53(File32 mFile, int currentOffset, int currentBlock)
    {
        _myTSX.descriptor[currentBlock].ID = 53;
        _myTSX.descriptor[currentBlock].playeable = false;
        // Size block
        int sizeBlock = 0;
        sizeBlock = getWORD(mFile,currentOffset+0x10+1);
        _myTSX.descriptor[currentBlock].size = sizeBlock;
        _myTSX.descriptor[currentBlock].offset = currentOffset+0x14;
        SerialHW.print("Tamaño: 0x");
        SerialHW.println(sizeBlock,HEX);
    }

    void analyzeID75(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID 0x4B

        _myTSX.descriptor[currentBlock].ID = 75;
        _myTSX.descriptor[currentBlock].playeable = true;
        _myTSX.descriptor[currentBlock].offset = currentOffset;
        _myTSX.descriptor[currentBlock].timming.sync_1 = 0;
        _myTSX.descriptor[currentBlock].timming.sync_2 = 0;
        
        //Tamaño de los datos
        _myTSX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+1,4)-12;
        _myTSX.descriptor[currentBlock].size=_myTSX.descriptor[currentBlock].lengthOfData;
        #ifdef DEBUGMODE
          logln("");
          logln("Tamaño datos: " + String(_myTSX.descriptor[currentBlock].lengthOfData) + " bytes");
        #endif

        _myTSX.descriptor[currentBlock].offsetData=currentOffset+17;

        //Pausa despues del bloque
        _myTSX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+5);
        #ifdef DEBUGMODE
          logln("Pause after block: " + String(_myTSX.descriptor[currentBlock].pauseAfterThisBlock));
        #endif
        
        // Timming de PULSE PILOT
        _myTSX.descriptor[currentBlock].timming.pilot_len = getWORD(mFile,currentOffset+7);
        #ifdef DEBUGMODE
          logln(",PULSE PILOT = " + String(_myTSX.descriptor[currentBlock].timming.pilot_len));
        #endif
        
        // Timming de PILOT TONE
        _myTSX.descriptor[currentBlock].timming.pilot_num_pulses = getWORD(mFile,currentOffset+9);
        #ifdef DEBUGMODE
          logln(",PULSE TONE = " + String(_myTSX.descriptor[currentBlock].timming.pilot_num_pulses));
        #endif

        // Timming de ZERO
        _myTSX.descriptor[currentBlock].timming.bit_0 = getWORD(mFile,currentOffset+11);
        #ifdef DEBUGMODE
          logln("PULSE ZERO = " + String(_myTSX.descriptor[currentBlock].timming.bit_0));
        #endif
        
        // Timming de ONE
        _myTSX.descriptor[currentBlock].timming.bit_1 = getWORD(mFile,currentOffset+13);
        #ifdef DEBUGMODE
          logln("PULSE ONE = " + String(_myTSX.descriptor[currentBlock].timming.bit_1));
        #endif
        
        // Configuracion de los bits
         _myTSX.descriptor[currentBlock].timming.bitcfg = getBYTE(mFile,currentOffset+15);     


        _myTSX.descriptor[currentBlock].timming.bytecfg = getBYTE(mFile,currentOffset+16);
               
        #ifdef DEBUGMODE
          logln(",ENDIANNESS=");
          logln(String(_myTSX.descriptor[currentBlock].timming.bitcfg & 0b00000001));
        #endif

        _myTSX.descriptor[currentBlock].type = 99;
    }

    void analyzeBlockUnknow(int id_num, int currentOffset, int currentBlock)
    {
        _myTSX.descriptor[currentBlock].ID = id_num;
        _myTSX.descriptor[currentBlock].playeable = false;
        _myTSX.descriptor[currentBlock].offset = currentOffset;   
    }

    bool getTSXBlock(File32 mFile, int currentBlock, int currentID, int currentOffset, int &nextIDoffset)
    {
        bool res = true;
        
        // Inicializamos el descriptor
        _myTSX.descriptor[currentBlock].group = 0;
        _myTSX.descriptor[currentBlock].chk = 0;
        _myTSX.descriptor[currentBlock].delay = 1000;
        _myTSX.descriptor[currentBlock].hasMaskLastByte = false;
        _myTSX.descriptor[currentBlock].header = false;
        _myTSX.descriptor[currentBlock].ID = 0;
        _myTSX.descriptor[currentBlock].lengthOfData = 0;
        _myTSX.descriptor[currentBlock].loop_count = 0;
        _myTSX.descriptor[currentBlock].maskLastByte = 8;
        _myTSX.descriptor[currentBlock].nameDetected = false;
        _myTSX.descriptor[currentBlock].offset = 0;
        _myTSX.descriptor[currentBlock].jump_this_ID = false;

        switch (currentID)
        {
          // ID 10 - Standard Speed Data Block
          case 16:

            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID16(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 5;
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID10STR,35);                  
            }
            else
            {
                res = false;
            }
            break;

          // ID 11- Turbo Speed Data Block
          case 17:
            
            //TIMMING_STABLISHED = true;
            
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID17(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 19;
                //_myTSX.descriptor[currentBlock].typeName = "ID 11 - Speed block";
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID11STR,35);
                  
            }
            else
            {
                res = false;

            }
            break;

          // ID 12 - Pure Tone
          case 18:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID18(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 1;
                //_myTSX.descriptor[currentBlock].typeName = "ID 12 - Pure tone";
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID12STR,35);
                  
            }
            else
            {
                res = false;

            }
            break;

          // ID 13 - Pulse sequence
          case 19:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID19(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 1;
                //_myTSX.descriptor[currentBlock].typeName = "ID 13 - Pulse seq.";
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID13STR,35);
                  
            }
            else
            {
                res = false;
            }
            break;                  

          // ID 14 - Pure Data Block
          case 20:
                       
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID20(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 10 + 1;
                
                //"ID 14 - Pure Data block";
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID14STR,35);

            }
            else
            {
                res = false;
            }
            break;                  

          // ID 15 - Direct Recording
          case 21:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID21(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 9 + 1;;  
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);

                // Informacion minima del fichero
                PROGRAM_NAME = "Audio block (WAV)";
                LAST_SIZE = _myTSX.descriptor[currentBlock].size;
                
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
            // _myTSX.descriptor[currentBlock].typeName = "ID 18 - CSW Rec";
            res=false;              
            break;

          // ID 19 - Generalized Data Block
          case 25:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTSX.descriptor[currentBlock].typeName = "ID 19 - General Data block";
            res=false;              
            break;

          // ID 20 - Pause and Stop Tape
          case 32:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID32(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + 3;  
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                
                #ifdef DEBUGMODE
                  log("ID 0x20 - PAUSE / STOP TAPE");
                  log("-- value: " + String(_myTSX.descriptor[currentBlock].pauseAfterThisBlock));
                #endif
            }
            else
            {
                res = false;
            }
            break;

          // ID 21 - Group start
          case 33:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID33(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + 2 + _myTSX.descriptor[currentBlock].size;
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;

                //_myTSX.descriptor[currentBlock].typeName = "ID 21 - Group start";
            }
            else
            {
                res = false;
            }
            break;                

          // ID 22 - Group end
          case 34:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTSX.descriptor[currentBlock].ID = 34;
                _myTSX.descriptor[currentBlock].playeable = false;
                _myTSX.descriptor[currentBlock].offset = currentOffset;

                nextIDoffset = currentOffset + 1;                      
                //_myTSX.descriptor[currentBlock].typeName = "ID 22 - Group end";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);

                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;
                  
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

            // _myTSX.descriptor[currentBlock].typeName = "ID 23 - Jump to block";
            res=false;
            break;

          // ID 24 - Loop start
          case 36:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTSX.descriptor[currentBlock].ID = 36;
                _myTSX.descriptor[currentBlock].playeable = false;
                _myTSX.descriptor[currentBlock].offset = currentOffset;
                _myTSX.descriptor[currentBlock].loop_count = getWORD(mFile,currentOffset+1);

                #ifdef DEBUGMODE
                    log("LOOP GET: " + String(_myTSX.descriptor[currentBlock].loop_count));
                #endif

                nextIDoffset = currentOffset + 3;                      
                //_myTSX.descriptor[currentBlock].typeName = "ID 24 - Loop start";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                  
            }
            else
            {
                res = false;
            }                
            break;


          // ID 25 - Loop end
          case 37:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                _myTSX.descriptor[currentBlock].ID = 37;
                _myTSX.descriptor[currentBlock].playeable = false;
                _myTSX.descriptor[currentBlock].offset = currentOffset;
                LOOP_END = currentOffset;
                
                nextIDoffset = currentOffset + 1;                      
                //_myTSX.descriptor[currentBlock].typeName = "ID 25 - Loop end";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                  
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

            // _myTSX.descriptor[currentBlock].typeName = "ID 26 - Call seq.";
            res=false;
            break;

          // ID 27 - Return from sequence
          case 39:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTSX.descriptor[currentBlock].typeName = "ID 27 - Return from seq.";
            res=false;              
            break;

          // ID 28 - Select block
          case 40:
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID40(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 1;
                
                //"ID 14 - Pure Data block";
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID14STR,35);
                                        
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

            //_myTSX.descriptor[currentBlock].typeName = "ID 2A - Stop TAPE (48k mode)";
            strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
              
            break;

          // ID 2B - Set signal level
          case 43:
            if (_myTSX.descriptor != nullptr)
            {
              int signalLevel = getBYTE(mFile,currentOffset+4);
              if(signalLevel==0)
              {
                // Para que empiece en DOWN tiene que ser UP
                POLARIZATION = up;
              }
              else
              {
                POLARIZATION = down;
              }
              LAST_EAR_IS = POLARIZATION;
              nextIDoffset = currentOffset + 5;            

              //_myTSX.descriptor[currentBlock].typeName = "ID 2B - Set signal level";
              strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
            }
            else
            {
              res=false;              
            }
            break;

          // ID 30 - Text description
          case 48:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID48(mFile,currentOffset,currentBlock);
                // Siguiente ID
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 1;
                //_myTSX.descriptor[currentBlock].typeName = "ID 30 - Information";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 31 - Message block
          case 49:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID49(mFile,currentOffset,currentBlock);
                // Siguiente ID
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 2;
                //_myTSX.descriptor[currentBlock].typeName = "ID 30 - Information";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }       
            break;

          // ID 32 - Archive info
          case 50:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID50(mFile,currentOffset,currentBlock);

                // Siguiente ID
                nextIDoffset = currentOffset + 3 + _myTSX.descriptor[currentBlock].size;
                //_myTSX.descriptor[currentBlock].typeName = "ID 32 - Archive info";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 33 - Hardware type
          case 51:
            // No hacemos nada solamente coger el siguiente offset
            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID51(mFile,currentOffset,currentBlock);

                // Siguiente ID
                nextIDoffset = currentOffset + 3 + _myTSX.descriptor[currentBlock].size;
                //_myTSX.descriptor[currentBlock].typeName = "ID 33- Hardware type";
                strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTSX.descriptor[currentBlock].jump_this_ID = true;                  
            }
            else
            {
                res = false;
            }                  
            break;

          // ID 35 - Custom info block
          case 53:
            analyzeID53(mFile,currentOffset,currentBlock);
            nextIDoffset = currentOffset + 0x15 + _myTSX.descriptor[currentBlock].size;

            //_myTSX.descriptor[currentBlock].typeName = "ID 35 - Custom info block";
            strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);

            // Esto le indica a los bloque de control de flujo que puede saltarse
            _myTSX.descriptor[currentBlock].jump_this_ID = true;

            break;

          // ID 4B - Standard Speed Data Block
          case 75:

            if (_myTSX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID75(mFile,currentOffset, currentBlock);
                nextIDoffset = currentOffset + _myTSX.descriptor[currentBlock].size + 17;
                strncpy(_myTSX.descriptor[currentBlock].typeName,ID4BSTR,35);
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

            //_myTSX.descriptor[currentBlock].typeName = "ID 5A - Glue block";
            strncpy(_myTSX.descriptor[currentBlock].typeName,IDXXSTR,35);
            break;

          default:
            ////SerialHW.println("");
            ////SerialHW.println("ID unknow " + currentID);
            ID_NOT_IMPLEMENTED = true;
            res = false;
            break;
      }

      return res;
    }

    void getBlockDescriptor(File32 mFile, int sizeTSX)
    {
          // Para ello tenemos que ir leyendo el TSX poco a poco
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
          bool endTSX = false;
          bool forzeEnd = false;
          bool endWithErrors = false;

          currentOffset = startOffset;
                   
          // Inicializamos
          ID_NOT_IMPLEMENTED = false;

          while (!endTSX && !forzeEnd && !ID_NOT_IMPLEMENTED)
          {
             
              if (ABORT==true)
              {
                  forzeEnd = true;
                  endWithErrors = true;
                  LAST_MESSAGE = "Aborting. No proccess complete.";
              }

              // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque completo para el descriptor.
              currentID = getID(mFile, currentOffset);
              
              // Por defecto el bloque no es reproducible
              _myTSX.descriptor[currentBlock].playeable	= false;

              // Ahora dependiendo del ID analizamos. Los ID están en HEX
              // y la rutina devolverá la posición del siguiente ID, así hasta
              // completar todo el fichero
              if (getTSXBlock(mFile, currentBlock, currentID, currentOffset, nextIDoffset))
              {
                  currentBlock++;               
              }
              else
              {
                LAST_MESSAGE = "ID block not implemented. Aborting";
                forzeEnd = true;
                endWithErrors = true;
              }

              logln("");
              logln("-----------------------------------------------");
              log("TSX ID: ");
              logHEX(currentID);
              logln("");
              logln("block: " + String(currentBlock));
              logln("");
              log("offset: ");
              logHEX(currentOffset);
              logln("");
              log("next offset: ");
              logHEX(nextIDoffset);
              logln("");
              logln("");

              if(currentBlock > maxAllocationBlocks)
              {
                #ifdef DEBUGMODE
                  logln("Error. TSX not possible to allocate in memory");
                #endif

                LAST_MESSAGE = "Error. Not enough memory for TSX";
                endTSX = true;
                // Salimos
                return;  
              }
              else
              {}
 
 
              if (nextIDoffset >= sizeTSX)
              {
                  // Finalizamos
                  endTSX = true;
              }
              else
              {
                  currentOffset = nextIDoffset;
              }

              TOTAL_BLOCKS = currentBlock;
          }

         
          // Nos posicionamos en el bloque 1
          BLOCK_SELECTED = 0;
          _hmi.writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));

          _myTSX.numBlocks = currentBlock;
          _myTSX.size = sizeTSX;
          
    }
   
    void proccess_TSX(File32 TSXFileName)
    {
        //Procesamos el fichero
        SerialHW.println("");
        SerialHW.println("Getting total blocks...");     

        if (isFileTSX(TSXFileName))
        {
            // Esto lo hacemos para poder abortar
            ABORT=false;
            getBlockDescriptor(_mFile,_sizeTSX);
        }      
    }

    void showBufferPlay(uint8_t* buffer, int size, int offset)
    {

        char hexs[20];
        if (size > 10)
        {
          #ifdef DEBUGMODE
            log("");
            SerialHW.println("Listing bufferplay.");
            SerialHW.print(buffer[0],HEX);
            SerialHW.print(",");
            SerialHW.print(buffer[1],HEX);
            SerialHW.print(" ... ");
            SerialHW.print(buffer[size-2],HEX);
            SerialHW.print(",");
            SerialHW.print(buffer[size-1],HEX);
            log("");            

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
          #endif

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

    tTSXBlockDescriptor* getDescriptor()
    {
        return _myTSX.descriptor;
    }

    void setDescriptorNull()
    {
            _myTSX.descriptor = nullptr;
    }

    void setTSX(tTSX tsx)
    {
        _myTSX = tsx;
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

    void set_SDM(SDmanager sdmTmp)
    {
        //_sdm = sdmTmp;
        //ptrSdm = &sdmTmp;
    }

    void set_SdFat32(SdFat32 sdf32)
    {
        _sdf32 = sdf32;
    }

    void set_HMI(HMI hmi)
    {
        _hmi = hmi;
    }

    void set_file(File32 mFile, int sizeTSX)
    {
      // Pasamos los parametros a la clase
      _mFile = mFile;
      _sizeTSX = sizeTSX;
    }  

    void getInfoFileTSX(char* path) 
    {
      
      File32 TSXFile;
      
      PROGRAM_NAME_DETECTED = false;
      PROGRAM_NAME = "";
      PROGRAM_NAME_2 = "";

      LAST_MESSAGE = "Analyzing file";
    
      MULTIGROUP_COUNT = 1;

      // Abrimos el fichero
      TSXFile = sdm.openFile32(TSXFile, path);

      // Obtenemos su tamaño total
      _mFile = TSXFile;
      _rlen = TSXFile.available();
      _myTSX.size = TSXFile.size();

      if (_rlen != 0)
      {
        FILE_IS_OPEN = true;

        // creamos un objeto TSXproccesor
        set_file(TSXFile, _rlen);
        proccess_TSX(TSXFile);
        
        ////SerialHW.println("");
        ////SerialHW.println("END PROCCESING TSX: ");

        if (_myTSX.descriptor != nullptr && !ID_NOT_IMPLEMENTED)
        {
            // Entregamos información por consola
            //PROGRAM_NAME = _myTSX.name;
            strcpy(LAST_NAME,"              ");
        }
        else
        {
          LAST_MESSAGE = "Error in TSX or ID not supported";
          //_hmi.updateInformationMainPage();
        }

      }
      else
      {
        FILE_IS_OPEN = false;
        LAST_MESSAGE = "Error in TSX file has 0 bytes";
      }


    }

    void initialize()
    {
        // if (_myTSX.descriptor != nullptr)
        // {
        //   //free(_myTSX.descriptor);
        //   //free(_myTSX.name);
        //   //_myTSX.descriptor = nullptr;

        // }          

        strncpy(_myTSX.name,"          ",10);
        _myTSX.numBlocks = 0;
        _myTSX.size = 0;

        CURRENT_BLOCK_IN_PROGRESS = 1;
        BLOCK_SELECTED = 1;
        PROGRESS_BAR_BLOCK_VALUE = 0;
        PROGRESS_BAR_TOTAL_VALUE = 0;
    }

    void terminate()
    {
      //free(_myTSX.descriptor);
      //_myTSX.descriptor = nullptr;
      //free(_myTSX.name);
      strncpy(_myTSX.name,"          ",10);
      _myTSX.numBlocks = 0;
      _myTSX.size = 0;      
      // free(_myTSX.descriptor);
      // _myTSX.descriptor = nullptr;            
    }

    void playBlock(tTSXBlockDescriptor descriptor)
    {

        uint8_t* bufferPlay = nullptr;

        //int pulsePilotDuration = descriptor.timming.pulse_pilot * descriptor.timming.pilot_num_pulses;
        int blockSizeSplit = SIZE_FOR_SPLIT;

        if (descriptor.size > blockSizeSplit)
        {
            //log("Partiendo la pana");

            int totalSize = descriptor.size;
            int offsetBase = descriptor.offsetData;
            int newOffset = 0;
            int blocks = totalSize / blockSizeSplit;
            int lastBlockSize = totalSize - (blocks * blockSizeSplit);

            SerialHW.println("");
              SerialHW.println("Offset DATA:         " + String(offsetBase));
              SerialHW.println("Total size del DATA: " + String(totalSize));
            SerialHW.println("");

            SerialHW.println("");
            SerialHW.println("Información: ");
            SerialHW.println(" - Tamaño total del bloque entero: " + String(totalSize));
            SerialHW.println(" - Numero de particiones: " + String(blocks));
            SerialHW.println(" - Ultimo bloque (size): " + String(lastBlockSize));
            SerialHW.println(" - Offset: " + String(offsetBase));
            SerialHW.println("");

            // BIT 0
            _zxp.BIT_0 = descriptor.timming.bit_0;
            // BIT1                                          
            _zxp.BIT_1 = descriptor.timming.bit_1;

            TOTAL_PARTS = blocks;

            // Recorremos el vector de particiones del bloque.
            for (int n=0;n < blocks;n++)
            {
              PARTITION_BLOCK = n;
              
              #ifdef DEBUGMODE
                log("Envio el partition");
              #endif

              // Calculamos el offset del bloque
              newOffset = offsetBase + (blockSizeSplit*n);
              BYTES_INI = newOffset;

              // Accedemos a la SD y capturamos el bloque del fichero
              bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
              sdm.readFileRange32(_mFile,bufferPlay, newOffset, blockSizeSplit, true);
              // Mostramos en la consola los primeros y últimos bytes
              showBufferPlay(bufferPlay,blockSizeSplit,newOffset);     
              
              // Reproducimos la partición n, del bloque.
              if (n==0)
              {
                _zxp.playDataBegin(bufferPlay, blockSizeSplit,descriptor.timming.pilot_len,descriptor.timming.pilot_num_pulses);
              }
              else
              {
                _zxp.playDataPartition(bufferPlay, blockSizeSplit);
              }

              free(bufferPlay);
            }

            // Ultimo bloque
            //log("Particion [" + String(blocks) + "/" + String(blocks) + "]");

            // Calculamos el offset del último bloque
            newOffset = offsetBase + (blockSizeSplit*blocks);
            BYTES_INI = newOffset;

            blockSizeSplit = lastBlockSize;
            // Accedemos a la SD y capturamos el bloque del fichero
            bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
            sdm.readFileRange32(_mFile,bufferPlay, newOffset,blockSizeSplit, true);
            // Mostramos en la consola los primeros y últimos bytes
            showBufferPlay(bufferPlay,blockSizeSplit,newOffset);         
            
            // Reproducimos el ultimo bloque con su terminador y silencio si aplica
            _zxp.playDataEnd(bufferPlay, blockSizeSplit);                                    

            free(bufferPlay);       
        }
        else
        {
            // Si es mas pequeño que el SPLIT, se reproduce completo.
            bufferPlay = (uint8_t*)ps_calloc(descriptor.size,sizeof(uint8_t));
            sdm.readFileRange32(_mFile,bufferPlay, descriptor.offsetData, descriptor.size, true);
            showBufferPlay(bufferPlay,descriptor.size,descriptor.offsetData);
            verifyChecksum(_mFile,descriptor.offsetData,descriptor.size);    

            // BTI 0
            _zxp.BIT_0 = descriptor.timming.bit_0;
            // BIT1                                          
            _zxp.BIT_1 = descriptor.timming.bit_1;
            //
            _zxp.playData(bufferPlay, descriptor.size, descriptor.timming.pilot_len,descriptor.timming.pilot_num_pulses);
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
          res = true; //Indicamos que lo hemos encontrado
          break;
        }
      }

      return res;
    }

    void prepareID4B(int currentBlock, File32 mFile, int nlb, int vlb, int ntb, int vtb, int pzero, int pone, int offset, int ldatos, bool begin)
    {
        // Generamos la señal de salida
        int pulsosmaximos;
        int npulses[2];
        int vpulse[2];

        npulses[0] = pzero/2;
        npulses[1] = pone/2;
        vpulse[0] = (_myTSX.descriptor[currentBlock].timming.bit_0);
        vpulse[1] = (_myTSX.descriptor[currentBlock].timming.bit_1);
        pulsosmaximos = (_myTSX.descriptor[currentBlock].timming.pilot_num_pulses) + ((npulses[vlb] * nlb) + 128 + (npulses[vtb] * ntb)) * ldatos;


        // Reservamos memoria dinamica
        _myTSX.descriptor[currentBlock].timming.pulse_seq_array = (int*)ps_calloc(pulsosmaximos+1,sizeof(int));
        // _myTSX.descriptor[currentBlock].timming.pulse_seq_array = new int[pulsosmaximos + 1]; 

        #ifdef DEBUGMODE
          logln("");
          log(" - Numero de pulsos: " + String(_myTSX.descriptor[currentBlock].timming.pilot_num_pulses));
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
          for (p=0; p < (_myTSX.descriptor[currentBlock].timming.pilot_num_pulses); p++)
          {
              _myTSX.descriptor[currentBlock].timming.pulse_seq_array[p] = _myTSX.descriptor[currentBlock].timming.pilot_len;
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
                    _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                  
                    i++;
                    _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                  
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
                        _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[1];
                        
                        i++;
                        _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[1];
                      
                        i++;
                    }                       
                }
                else
                {
                    // En otro caso
                    // procesamos "0"
                    for (int b0=0;b0<npulses[0];b0++)
                    { 
                      _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[0];
                      
                      i++;
                      _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=vpulse[0];
                    
                      i++;
                    }
                }
            }

            for (int i3=0;i3<ntb;i3++)
            {
                for (int i4=0;i4<npulses[vtb];i4++)
                { lenPulse=vpulse[vtb];
                    _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                    
                    i++;
                    _myTSX.descriptor[currentBlock].timming.pulse_seq_array[i]=lenPulse;
                    
                    i++;
                }
            }
        }

        _myTSX.descriptor[currentBlock].timming.pulse_seq_num_pulses=i;
        
        #ifdef DEBUGMODE
          logln("datos: "+String(ldatos));
          logln("pulsos: "+String(i));
        
          logln("---------------------------------------------------------------------");
        #endif    

        free(bRead);
    }


    int getIdAndPlay(int i)
    {
        // Inicializamos el buffer de reproducción. Memoria dinamica
        uint8_t* bufferPlay = nullptr;
        int dly = 0;
        int newPosition = -1;
        
        // Cogemos la mascara del ultimo byte
        if (_myTSX.descriptor[i].hasMaskLastByte)
        {
          _zxp.set_maskLastByte(_myTSX.descriptor[i].maskLastByte);
        }
        else
        {
          _zxp.set_maskLastByte(8);
        }
        

        // Ahora lo voy actualizando a medida que van avanzando los bloques.
        //PROGRAM_NAME_2 = _myTSX.descriptor[BLOCK_SELECTED].name;

        DIRECT_RECORDING = false;

        switch (_myTSX.descriptor[i].ID)
        {
            case 21:
              DIRECT_RECORDING = true;
              // 
              PROGRAM_NAME = "Audio block (WAV)";
              LAST_SIZE = _myTSX.descriptor[i].size;          
              // 
              if (_myTSX.descriptor[i].samplingRate == 79)
              {
                  SAMPLING_RATE = 44100;
                  ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES);
                  LAST_MESSAGE = "Direct recording at 44.1KHz";
              }
              else if (_myTSX.descriptor[i].samplingRate == 158)
              {
                  SAMPLING_RATE = 22050;
                  ESP32kit.setSampleRate(AUDIO_HAL_22K_SAMPLES);
                  LAST_MESSAGE = "Direct recording at 22.05KHz";
              }
              else
              {
                LAST_MESSAGE = "Direct recording sampling rate unknow";
                delay(1500);
                //Paramos la reproducción.

                PAUSE = false;
                STOP = true;
                PLAY = false;

                i = _myTSX.numBlocks+1;

                LOOP_PLAYED = 0;
                LAST_EAR_IS = down;
                LOOP_START = 0;
                LOOP_END = 0;
                BL_LOOP_START = 0;
                BL_LOOP_END = 0;                
              }
              // Ahora reproducimos
              playBlock(_myTSX.descriptor[i]);
              break;


            case 36:  
              //Loop start ID 0x24
              // El loop controla el cursor de bloque por tanto debe estar el primero
              LOOP_PLAYED = 0;
              // El primer bloque a repetir es el siguiente, pero ponemos "i" porque el FOR lo incrementa. 
              // Entonces este bloque no se lee mas, ya se continua repitiendo el bucle.
              BL_LOOP_START = i;
              LOOP_COUNT = _myTSX.descriptor[i].loop_count;
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
              dly = _myTSX.descriptor[i].pauseAfterThisBlock;

              // Vemos si es una PAUSA o una parada del TAPE
              if (dly == 0)
              {                       
                  // Finalizamos el ultimo bit
                  if(LAST_SILENCE_DURATION==0)
                  {
                    _zxp.silence(2000);
                  }

                  // Inicializamos la polarización de la señal
                  LAST_EAR_IS = POLARIZATION;   

                  //_zxp.closeBlock();

                  // PAUSE
                  // Pausamos la reproducción a través del HMI 

                  hmi.writeString("click btnPause,1"); 

                  PLAY = false;
                  STOP = false;
                  PAUSE = true;

                  LAST_MESSAGE = "Playing PAUSE.";
                  LAST_GROUP = "[STOP BLOCK]";

                  // Dejamos preparado el sieguiente bloque
                  CURRENT_BLOCK_IN_PROGRESS++;
                  BLOCK_SELECTED++;
                  //WAITING_FOR_USER_ACTION = true;

                  if (BLOCK_SELECTED >= _myTSX.numBlocks)
                  {
                      // Reiniciamos
                      CURRENT_BLOCK_IN_PROGRESS = 1;
                      BLOCK_SELECTED = 1;
                  }

                  // Enviamos info al HMI.
                  _hmi.setBasicFileInformation(_myTSX.descriptor[BLOCK_SELECTED].name,_myTSX.descriptor[BLOCK_SELECTED].typeName,_myTSX.descriptor[BLOCK_SELECTED].size,_myTSX.descriptor[BLOCK_SELECTED].playeable);                        
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
              LAST_GROUP = "[META BLK: " + String(_myTSX.descriptor[i].group) + "]";
              break;

            case 34:
              //LAST_GROUP = &INITCHAR[0];
              LAST_GROUP = "[GROUP END]";
              break;

            default:
              // Cualquier otro bloque entra por aquí, pero
              // hay que comprobar que sea REPRODUCIBLE

              // Reseteamos el indicador de META BLOCK
              // (añadido el 26/03/2024)
              LAST_GROUP = "...";

              if (_myTSX.descriptor[i].playeable)
              {
                    //Silent
                    _zxp.silent = _myTSX.descriptor[i].pauseAfterThisBlock;        
                    //SYNC1
                    _zxp.SYNC1 = _myTSX.descriptor[i].timming.sync_1;
                    //SYNC2
                    _zxp.SYNC2 = _myTSX.descriptor[i].timming.sync_2;
                    //PULSE PILOT (longitud del pulso)
                    _zxp.PILOT_PULSE_LEN = _myTSX.descriptor[i].timming.pilot_len;
                    // BTI 0
                    _zxp.BIT_0 = _myTSX.descriptor[i].timming.bit_0;
                    // BIT1                                          
                    _zxp.BIT_1 = _myTSX.descriptor[i].timming.bit_1;

                    // Obtenemos el nombre del bloque
                    strncpy(LAST_NAME,_myTSX.descriptor[i].name,14);
                    LAST_SIZE = _myTSX.descriptor[i].size;
                    strncpy(LAST_TYPE,_myTSX.descriptor[i].typeName,35);

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

                        i = _myTSX.numBlocks+1;

                        LOOP_PLAYED = 0;
                        LAST_EAR_IS = down;
                        LOOP_START = 0;
                        LOOP_END = 0;
                        BL_LOOP_START = 0;
                        BL_LOOP_END = 0;

                        //return newPosition;
                    }

                    //Ahora vamos lanzando bloques dependiendo de su tipo
                    // Actualizamos HMI
                    _hmi.setBasicFileInformation(_myTSX.descriptor[i].name,_myTSX.descriptor[i].typeName,_myTSX.descriptor[i].size,_myTSX.descriptor[i].playeable);

                    // Reproducimos el fichero
                    if (_myTSX.descriptor[i].type == 0) 
                    {
                        //
                        // LOS BLOQUES TYPE == 0 
                        //
                        // Llevan una cabecera y después un bloque de datos
                        
                        switch (_myTSX.descriptor[i].ID)
                        {
                          case 16:
                            //PULSE PILOT
                            _myTSX.descriptor[i].timming.pilot_len = DPILOT_LEN;
                            
                            // Reproducimos el bloque - PLAY
                            playBlock(_myTSX.descriptor[i]);
                            //free(bufferPlay);
                            break;

                          case 17:
                            //PULSE PILOT
                            // Llamamos a la clase de reproducción
                            playBlock(_myTSX.descriptor[i]);
                            break;
                    
                        }

                    } 
                    else if (_myTSX.descriptor[i].type == 1 || _myTSX.descriptor[i].type == 7) 
                    {

                        // Llamamos a la clase de reproducción
                        switch (_myTSX.descriptor[i].ID)
                        {
                          case 16:
                            //Standard data - ID-10                          
                            _myTSX.descriptor[i].timming.pilot_len = DPILOT_LEN;                                  
                            playBlock(_myTSX.descriptor[i]);
                            break;

                          case 17:
                            // Speed data - ID-11                            
                            playBlock(_myTSX.descriptor[i]);
                            break;                     
                        }                                              
                    } 
                    else if (_myTSX.descriptor[i].type == 99)
                    {
                        //
                        // Son bloques especiales de TONO GUIA o SECUENCIAS para SYNC
                        //
                        //int num_pulses = 0;
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
                      

                        switch (_myTSX.descriptor[i].ID)
                        {
                            case 75:


                              //configuracion del byte
                              pzero=((_myTSX.descriptor[i].timming.bitcfg & 0b11110000)>>4);
                              pone=((_myTSX.descriptor[i].timming.bitcfg & 0b00001111));
                              nlb=((_myTSX.descriptor[i].timming.bytecfg & 0b11000000)>>6);
                              vlb=((_myTSX.descriptor[i].timming.bytecfg & 0b00100000)>>5);    
                              ntb=((_myTSX.descriptor[i].timming.bytecfg & 0b00011000)>>3);
                              vtb=((_myTSX.descriptor[i].timming.bitcfg & 0b00000100)>>2);

                              #ifdef DEBUGMODE
                                logln("");
                                log("ID 0x4B:");
                                log("PULSES FOR ZERO = " + String(pzero));
                                log(" - " + String(_myTSX.descriptor[i].timming.bit_0) + " - ");
                                log(",PULSES FOR ONE = " + String(pone));
                                log(" - " + String(_myTSX.descriptor[i].timming.bit_1) + " - ");
                                log(",NUMBERS OF LEADING BITS = "+String(nlb));
                                log(",VALUE OF LEADING BITS = "+String(vlb));
                                log(",NUMBER OF TRAILING BITS = "
                                +String(ntb));
                                log(",VALUE OF TRAILING BITS = "+String(vtb));
                              #endif


                              // Conocemos la longitud total del bloque de datos a reproducir
                              ldatos = _myTSX.descriptor[i].lengthOfData;
                              // Nos quedamos con el offset inicial
                              offset = _myTSX.descriptor[i].offsetData;
                              
                              BYTES_INI = offset;
                              PROGRESS_BAR_TOTAL_VALUE = (BYTES_INI * 100 ) / BYTES_TOBE_LOAD ;

                              bufferD = 1024;  // Buffer de BYTES de datos convertidos a samples
                              // Reservamos memoria dinamica


                              partitions = ldatos / bufferD;
                              lastPartitionSize = ldatos - (partitions * bufferD);

                              silence = _myTSX.descriptor[i].pauseAfterThisBlock;
                              
                              #ifdef DEBUGMODE
                                log(",SILENCE = "+String(silence) + " ms");                              

                                logln("");
                                log("Total data parts: " + String(partitions));
                                logln("");
                                log("----------------------------------------");
                              #endif          

                              TSX_PARTITIONED = false;

                              if (ldatos > bufferD)
                              {
                                  TSX_PARTITIONED = true;
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
                                      

                                      _zxp.playCustomSequence(_myTSX.descriptor[i].timming.pulse_seq_array,_myTSX.descriptor[i].timming.pulse_seq_num_pulses,0.0);                                                                           
                                      offset += bufferD;
                                      BYTES_INI += bufferD;
                                      PROGRESS_BAR_TOTAL_VALUE = (BYTES_INI * 100 ) / BYTES_TOBE_LOAD ;

                                      // Liberamos el array
                                      free(_myTSX.descriptor[i].timming.pulse_seq_array);
                                      // delete[] _myTSX.descriptor[i].timming.pulse_seq_array;
                                  }


                                  if (!STOP && !PAUSE)
                                  {
                                      // Ultima particion
                                      prepareID4B(i,_mFile,nlb,vlb,ntb,vtb,pzero,pone, offset, lastPartitionSize,false);
                                      // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición                                                                    
                                      BYTES_INI += lastPartitionSize;

                                      _zxp.playCustomSequence(_myTSX.descriptor[i].timming.pulse_seq_array,_myTSX.descriptor[i].timming.pulse_seq_num_pulses,0.0); 
                                      // Liberamos el array
                                      free(_myTSX.descriptor[i].timming.pulse_seq_array);
                                      // delete[] _myTSX.descriptor[i].timming.pulse_seq_array;
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
                                      // Una sola particion
                                      prepareID4B(i,_mFile,nlb,vlb,ntb,vtb,pzero,pone, offset, ldatos,true);

                                      // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición                                                                    
                                      _zxp.playCustomSequence(_myTSX.descriptor[i].timming.pulse_seq_array,_myTSX.descriptor[i].timming.pulse_seq_num_pulses,0); 

                                      // Liberamos el array
                                      free(_myTSX.descriptor[i].timming.pulse_seq_array); 
                                      // delete[] _myTSX.descriptor[i].timming.pulse_seq_array;
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
                              _zxp.playPureTone(_myTSX.descriptor[i].timming.pure_tone_len,_myTSX.descriptor[i].timming.pure_tone_num_pulses);
                              break;

                            case 19:
                              // ID 0x13 - Reproducimos una secuencia. Pulsos de longitud contenidos en un array y repetición 
                              #ifdef DEBUGMODE
                                  log("ID 0x13:");
                                  for(int j=0;j<_myTSX.descriptor[i].timming.pulse_seq_num_pulses;j++)
                                  {
                                    SerialHW.print(_myTSX.descriptor[i].timming.pulse_seq_array[j],HEX);
                                    SerialHW.print(",");
                                  }
                              #endif                                                                     
                              _zxp.playCustomSequence(_myTSX.descriptor[i].timming.pulse_seq_array,_myTSX.descriptor[i].timming.pulse_seq_num_pulses);                                                                           
                              break;                          

                            case 20:
                              // ID 0x14
                              int blockSizeSplit = SIZE_FOR_SPLIT;

                              if (_myTSX.descriptor[i].size > blockSizeSplit)
                              {

                                  int totalSize = _myTSX.descriptor[i].size;
                                  int offsetBase = _myTSX.descriptor[i].offsetData;
                                  int newOffset = 0;
                                  double blocks = totalSize / blockSizeSplit;
                                  int lastBlockSize = totalSize - (blocks * blockSizeSplit);

                                  // BTI 0
                                  _zxp.BIT_0 = _myTSX.descriptor[i].timming.bit_0;
                                  // BIT1                                          
                                  _zxp.BIT_1 = _myTSX.descriptor[i].timming.bit_1;

                                  // Recorremos el vector de particiones del bloque.
                                  for (int n=0;n < blocks && !STOP && !PAUSE;n++)
                                  {
                                    // Calculamos el offset del bloque
                                    newOffset = offsetBase + (blockSizeSplit*n);
                                    BYTES_INI = newOffset;

                                    // Accedemos a la SD y capturamos el bloque del fichero
                                    bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
                                    sdm.readFileRange32(_mFile,bufferPlay, newOffset, blockSizeSplit, true);
                                    // Mostramos en la consola los primeros y últimos bytes
                                    showBufferPlay(bufferPlay,blockSizeSplit,newOffset);     

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
                                  if (!STOP && !PAUSE)
                                  {
                                      // Calculamos el offset del último bloque
                                      newOffset = offsetBase + (blockSizeSplit*blocks);
                                      BYTES_INI = newOffset;

                                      blockSizeSplit = lastBlockSize;
                                      // Accedemos a la SD y capturamos el bloque del fichero
                                      bufferPlay = (uint8_t*)ps_calloc(blockSizeSplit,sizeof(uint8_t));
                                      sdm.readFileRange32(_mFile,bufferPlay, newOffset,blockSizeSplit, true);
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
                              }
                              else
                              {
                                      
                                  if (!STOP && !PAUSE)
                                  {
                                      bufferPlay = (uint8_t*)ps_calloc(_myTSX.descriptor[i].size,sizeof(uint8_t));
                                      sdm.readFileRange32(_mFile,bufferPlay, _myTSX.descriptor[i].offsetData, _myTSX.descriptor[i].size, true);
                                      BYTES_INI = _myTSX.descriptor[i].offsetData;

                                      showBufferPlay(bufferPlay,_myTSX.descriptor[i].size,_myTSX.descriptor[i].offsetData);
                                      verifyChecksum(_mFile,_myTSX.descriptor[i].offsetData,_myTSX.descriptor[i].size);    

                                      // BTI 0
                                      _zxp.BIT_0 = _myTSX.descriptor[i].timming.bit_0;
                                      // BIT1                                          
                                      _zxp.BIT_1 = _myTSX.descriptor[i].timming.bit_1;
                                      //
                                      _zxp.playPureData(bufferPlay, _myTSX.descriptor[i].size);
                                      free(bufferPlay); 
                                  }                                 
                              }                               
                              break;                          
                        }                      
                    }
                    else 
                    {
                        //
                        // Otros bloques de datos SIN cabecera no contemplados anteriormente - DATA
                        //

                        int blockSize = _myTSX.descriptor[i].size;


                        switch (_myTSX.descriptor[i].ID)
                        {
                          case 16:
                            // ID 0x10

                            //PULSE PILOT
                            _myTSX.descriptor[i].timming.pilot_len = DPILOT_LEN;
                            playBlock(_myTSX.descriptor[i]);
                            break;

                          case 17:
                            // ID 0x11
                            //PULSE PILOT
                            playBlock(_myTSX.descriptor[i]);
                            // Bloque de datos BYTE
                            break;
                        }

                    }                  
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

        if (_myTSX.descriptor != nullptr)
        {           
              // Entregamos información por consola
              TOTAL_BLOCKS = _myTSX.numBlocks;
              strcpy(LAST_NAME,"              ");

              // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
              firstBlockToBePlayed = BLOCK_SELECTED;

              // Reiniciamos
              BYTES_TOBE_LOAD = _rlen;
              BYTES_LOADED = 0;

            
              #ifdef DEBUGMODE
                logln("");
                log("Starting blocks loop");
              #endif

              // Recorremos ahora todos los bloques que hay en el descriptor
              //-------------------------------------------------------------
              for (int i = firstBlockToBePlayed; i < _myTSX.numBlocks; i++) 
              {               
                  #ifdef DEBUGMODE
                    logln("");
                    log("Playing block " + String(i));
                  #endif              

                  BLOCK_SELECTED = i; 

                  if (BLOCK_SELECTED == 0) 
                  {
                    BYTES_INI = 0;
                  }
                  else
                  {
                    BYTES_INI = _myTSX.descriptor[BLOCK_SELECTED].offset;
                  }
               
                  int new_i = getIdAndPlay(i);
                  // Entonces viene cambiada de un loop
                  if (new_i != -1)
                  {
                    i = new_i;
                  }

                  if (LOADING_STATE == 2 || LOADING_STATE == 3)
                  {
                    break;
                  }

                  if (stopOrPauseRequest())
                  {
                    // Forzamos la salida
                    i = _myTSX.numBlocks + 1;
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
                  LAST_EAR_IS = POLARIZATION;       

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

                  _hmi.setBasicFileInformation(_myTSX.descriptor[BLOCK_SELECTED].name,_myTSX.descriptor[BLOCK_SELECTED].typeName,_myTSX.descriptor[BLOCK_SELECTED].size,_myTSX.descriptor[BLOCK_SELECTED].playeable);
              }

              // Cerrando
              // reiniciamos el edge del ultimo pulse
              LOOP_PLAYED = 0;
              //LAST_EAR_IS = down;
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
            _hmi.setBasicFileInformation(_myTSX.descriptor[BLOCK_SELECTED].name,_myTSX.descriptor[BLOCK_SELECTED].typeName,_myTSX.descriptor[BLOCK_SELECTED].size,_myTSX.descriptor[BLOCK_SELECTED].playeable);
        }        

    }

    TSXprocessor(AudioKit kit)
    {
        // Constructor de la clase
        strncpy(_myTSX.name,"          ",10);
        _myTSX.numBlocks = 0;
        _myTSX.size = 0;
        _myTSX.descriptor = nullptr;

        _zxp.set_ESP32kit(kit);      
    } 
};


