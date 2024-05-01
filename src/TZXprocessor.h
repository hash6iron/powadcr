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

    public:

      struct tTimming
      {
        int bit_0 = 855;
        int bit_1 = 1710;
        int pilot_len = 2168;
        int pilot_num_pulses = 0;
        int sync_1 = 667;
        int sync_2 = 735;
        int pure_tone_len = 0;
        int pure_tone_num_pulses = 0;
        int pulse_seq_num_pulses = 0;
        int* pulse_seq_array;
      };

      // Estructura de un descriptor de TZX
      struct tTZXBlockDescriptor 
      {
        int ID = 0;
        int offset = 0;
        int size = 0;
        int chk = 0;
        int pauseAfterThisBlock = 1000;   //ms
        int lengthOfData = 0;
        int offsetData = 0;
        char name[15];
        bool nameDetected = false;
        bool header = false;
        bool screen = false;
        int type = 0;
        bool playeable = false;
        int delay = 1000;
        int silent;
        int maskLastByte = 8;
        bool hasMaskLastByte = false;
        tTimming timming;
        char typeName[36];
        int group = 0;
        int loop_count = 0;
        bool jump_this_ID = false;
      };

      // Estructura tipo TZX
      struct tTZX
      {
        char name[11];                               // Nombre del TZX
        int size = 0;                             // Tamaño
        int numBlocks = 0;                        // Numero de bloques
        tTZXBlockDescriptor* descriptor;          // Descriptor
      };

    private:

    const char ID10STR[35] = "ID 10 - Standard block            ";
    const char ID11STR[35] = "ID 11 - Speed block               ";
    const char ID12STR[35] = "ID 12 - Pure tone                 ";
    const char ID13STR[35] = "ID 13 - Pulse seq.                ";
    const char ID14STR[35] = "ID 14 - Pure data                 ";
    const char IDXXSTR[35] = "ID                                ";

    const int maxAllocationBlocks = 4000;

    // Procesador de audio output
    ZXProcessor _zxp;

    HMI _hmi;

    // Definicion de un TZX
    SdFat32 _sdf32;
    tTZX _myTZX;
    File32 _mFile;
    int _sizeTZX;
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

    bool isHeaderTZX(File32 tzxFile)
    {
      if (tzxFile != 0)
      {

          ////SerialHW.println("");
          ////SerialHW.println("Begin isHeaderTZX");

          // Capturamos la cabecera

          uint8_t* bBlock = sdm.readFileRange32(tzxFile,0,10,false); 

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
          
          ////SerialHW.println("");
          ////SerialHW.println("Sign: " + signStr);

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

    bool isFileTZX(File32 tzxFile)
    {
        if (tzxFile != 0)
        {
          // Capturamos el nombre del fichero en szName
          char* szName = (char*)ps_calloc(255,sizeof(char));
          tzxFile.getName(szName,254);
          String fileName = String(szName);
          free(szName);

          if (fileName != "")
          {
                //String fileExtension = szNameStr.substring(szNameStr.length()-3);
                fileName.toUpperCase();
                
                if (fileName.indexOf(".TZX") != -1)
                {
                    //La extensión es TZX pero ahora vamos a comprobar si internamente también lo es
                    if (isHeaderTZX(tzxFile))
                    { 
                      //Cerramos el fichero porque no se usará mas.
                      tzxFile.close();
                      return true;
                    }
                    else
                    {
                      //Cerramos el fichero porque no se usará mas.
                      tzxFile.close();
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
        uint8_t* ptr1 = sdm.readFileRange32(mFile,offset+1,1,false);
        uint8_t* ptr2 = sdm.readFileRange32(mFile,offset,1,false);
        sizeDW = (256*ptr1[0]) + ptr2[0];
        free(ptr1);
        free(ptr2);

        return sizeDW;    
    }

    int getBYTE(File32 mFile, int offset)
    {
        int sizeB = 0;
        uint8_t* ptr = sdm.readFileRange32(mFile,offset,1,false);
        sizeB = ptr[0];
        free(ptr);

        return sizeB;      
    }

    int getNBYTE(File32 mFile, int offset, int n)
    {
        int sizeNB = 0;

        for (int i = 0; i<n;i++)
        {
            sizeNB += pow(2,(8*i)) * (sdm.readFileRange32(mFile,offset+i,1,false)[0]);  
        }

        return sizeNB;           
    }

    int getID(File32 mFile, int offset)
    {
        // Obtenemos el ID
        int ID = 0;
        ID = getBYTE(mFile,offset);

        return ID;      
    }

    uint8_t* getBlock(File32 mFile, int offset, int size)
    {
        //Entonces recorremos el TZX. 
        // La primera cabecera SIEMPRE debe darse.
        // Obtenemos el bloque
        uint8_t* block = (uint8_t*)ps_calloc(size+1,sizeof(uint8_t));
        block = sdm.readFileRange32(mFile,offset,size,false);
        return(block);
    }

    bool verifyChecksum(File32 mFile, int offset, int size)
    {
        // Vamos a verificar que el bloque cumple con el checksum
        // Cogemos el checksum del bloque
        uint8_t chk = getBYTE(mFile,offset+size-1);

        uint8_t* block = (uint8_t*)ps_calloc(size+1,sizeof(uint8_t));
        block = getBlock(mFile,offset,size-1);
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
            if (typeBlock == 0)
            {
                // Es una cabecera PROGRAM: BASIC
              _myTZX.descriptor[currentBlock].header = true;
              _myTZX.descriptor[currentBlock].type = 0;

              //_myTZX.name = "TZX";
              if (!PROGRAM_NAME_DETECTED)
              {
                  uint8_t* block = getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19);
                  gName = getNameFromStandardBlock(block);
                  PROGRAM_NAME = String(gName);
                  PROGRAM_NAME_DETECTED = true;
                  free(block);
              }

              // Almacenamos el nombre del bloque
              strncpy(_myTZX.descriptor[currentBlock].name,_myTZX.name,14);

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

                  // Almacenamos el nombre del bloque
                  uint8_t* block =getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19);
                  gName = getNameFromStandardBlock(block);
                  strncpy(_myTZX.descriptor[currentBlock].name,gName,14);
                  free(block);
              }
              else
              {
                // Es un bloque BYTE
                _myTZX.descriptor[currentBlock].type = 1;     

                // Almacenamos el nombre del bloque
                uint8_t* block = getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19);
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

              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 3;
              }
              else
              {
                  // Es un bloque BYTE
                  _myTZX.descriptor[currentBlock].type = 4;                
              }
            }
            else
            {
                // Es un bloque BYTE
                _myTZX.descriptor[currentBlock].type = 4;
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
        
    void analyzeID17(File32 mFile, int currentOffset, int currentBlock)
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
            uint8_t* block = getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19);
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

    void analyzeID18(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID19(File32 mFile, int currentOffset, int currentBlock)
    {
        // ID-13 - Pulse sequence

        _myTZX.descriptor[currentBlock].ID = 19;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        int num_pulses = getBYTE(mFile,currentOffset+1);
        _myTZX.descriptor[currentBlock].timming.pulse_seq_num_pulses = num_pulses;
        
        // Reservamos memoria.
        _myTZX.descriptor[currentBlock].timming.pulse_seq_array = new int[num_pulses]; 
        
        // Tomamos ahora las longitudes
        int coff = currentOffset+2;

        log("ID-13: Pulse sequence");
        //SerialHW.println("");

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

    void analyzeID20(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID32(File32 mFile, int currentOffset, int currentBlock)
    {
        // Pause or STOP the TAPE
        
        _myTZX.descriptor[currentBlock].ID = 32;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        // Obtenemos el valor de la pausa
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getWORD(mFile,currentOffset+1);          
    }

    void analyzeID40(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID48(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID49(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID50(File32 mFile, int currentOffset, int currentBlock)
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

    void analyzeID51(File32 mFile, int currentOffset, int currentBlock)
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


    void analyzeID33(File32 mFile, int currentOffset, int currentBlock)
    {
        // Group start
        int sizeTextInformation = 0;

        _myTZX.descriptor[currentBlock].ID = 33;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset+2;
        _myTZX.descriptor[currentBlock].group = MULTIGROUP_COUNT;
        MULTIGROUP_COUNT++;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        ////SerialHW.println("");
        ////SerialHW.println("ID33 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeTextInformation;
    }

    void analyzeBlockUnknow(int id_num, int currentOffset, int currentBlock)
    {
        _myTZX.descriptor[currentBlock].ID = id_num;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;   
    }

    bool getTZXBlock(File32 mFile, int currentBlock, int currentID, int currentOffset, int &nextIDoffset)
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x10");
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x11");
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x12");
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
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTZX.descriptor[currentBlock].typeName = "ID 15 - Direct REC";
            res=false;              
            break;

          // ID 18 - CSW Recording
          case 24:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTZX.descriptor[currentBlock].typeName = "ID 18 - CSW Rec";
            res=false;              
            break;

          // ID 19 - Generalized Data Block
          case 25:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            
            // _myTZX.descriptor[currentBlock].typeName = "ID 19 - General Data block";
            res=false;              
            break;

          // ID 20 - Pause and Stop Tape
          case 32:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID32(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + 3;  
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                
                log("ID 0x20 - PAUSE / STOP TAPE");
                log("-- value: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x20");
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
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                
                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;

                //_myTZX.descriptor[currentBlock].typeName = "ID 21 - Group start";
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x21");
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
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);

                // Esto le indica a los bloque de control de flujo que puede saltarse
                _myTZX.descriptor[currentBlock].jump_this_ID = true;
                  
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x22");
                res = false;

            }
            break;

          // ID 23 - Jump to block
          case 35:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 23 - Jump to block";
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
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                  
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x22");
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
                strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
                  
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x22");
                res = false;

            }                
            break;

          // ID 26 - Call sequence
          case 38:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 26 - Call seq.";
            res=false;
            break;

          // ID 27 - Return from sequence
          case 39:
            ID_NOT_IMPLEMENTED = true;
            // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            // nextIDoffset = currentOffset + 1;            

            // _myTZX.descriptor[currentBlock].typeName = "ID 27 - Return from seq.";
            res=false;              
            break;

          // ID 28 - Select block
          case 40:
            if (_myTZX.descriptor != nullptr)
            {
                // Obtenemos la dirección del siguiente offset
                analyzeID40(mFile,currentOffset, currentBlock);

                nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
                
                //"ID 14 - Pure Data block";
                strncpy(_myTZX.descriptor[currentBlock].typeName,ID14STR,35);
                                        
            }
            else
            {
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x22");
                res = false;

            }                
            break;

          // ID 2A - Stop the tape if in 48K mode
          case 42:
            analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            nextIDoffset = currentOffset + 1;            

            //_myTZX.descriptor[currentBlock].typeName = "ID 2A - Stop TAPE (48k mode)";
            strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
              
            break;

          // ID 2B - Set signal level
          case 43:
            if (_myTZX.descriptor != nullptr)
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

              //_myTZX.descriptor[currentBlock].typeName = "ID 2B - Set signal level";
              strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x30");
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x30");
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x32");
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
                ////SerialHW.println("");
                ////SerialHW.println("Error: Not allocation memory for block ID 0x33");
                res = false;
            }                  
            break;

          // ID 35 - Custom info block
          case 53:
            analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            nextIDoffset = currentOffset + 1;            

            //_myTZX.descriptor[currentBlock].typeName = "ID 35 - Custom info block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);

            // Esto le indica a los bloque de control de flujo que puede saltarse
            _myTZX.descriptor[currentBlock].jump_this_ID = true;

            break;

          // ID 5A - "Glue" block (90 dec, ASCII Letter 'Z')
          case 90:
            analyzeBlockUnknow(currentID,currentOffset, currentBlock);
            nextIDoffset = currentOffset + 8;            

            //_myTZX.descriptor[currentBlock].typeName = "ID 5A - Glue block";
            strncpy(_myTZX.descriptor[currentBlock].typeName,IDXXSTR,35);
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

    void getBlockDescriptor(File32 mFile, int sizeTZX)
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

          while (!endTZX && !forzeEnd && !ID_NOT_IMPLEMENTED)
          {
             
              if (ABORT==true)
              {
                  forzeEnd = true;
                  endWithErrors = true;
                  LAST_MESSAGE = "Aborting. No proccess complete.";
              }

              // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque completo para el descriptor.
              currentID = getID(mFile, currentOffset);
              
              #ifdef DEBUGMODE
                SerialHW.println("");
                SerialHW.println("-----------------------------------------------");
                SerialHW.print("TZX ID: 0x");
                SerialHW.print(currentID, HEX);
                SerialHW.println("");
                SerialHW.println("block: " + String(currentBlock));
                SerialHW.println("");
                SerialHW.print("offset: 0x");
                SerialHW.print(currentOffset, HEX);
                SerialHW.println("");
              #endif

              // Por defecto el bloque no es reproducible
              _myTZX.descriptor[currentBlock].playeable	= false;

              // Ahora dependiendo del ID analizamos. Los ID están en HEX
              // y la rutina devolverá la posición del siguiente ID, así hasta
              // completar todo el fichero
              if (getTZXBlock(mFile, currentBlock, currentID, currentOffset, nextIDoffset))
              {
                  currentBlock++;               
              }
              else
              {
                LAST_MESSAGE = "ID block not implemented. Aborting";
                forzeEnd = true;
                endWithErrors = true;
              }

                //SerialHW.println("");
                //SerialHW.print("Next ID offset: 0x");
                //SerialHW.print(nextIDoffset, HEX);
                //SerialHW.println("");

                if(currentBlock > maxAllocationBlocks)
                {
                  #ifdef DEBUGMODE
                    SerialHW.println("Error. TZX not possible to allocate in memory");
                  #endif

                  LAST_MESSAGE = "Error. Not enough memory for TZX";
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
                }
                else
                {
                    currentOffset = nextIDoffset;
                }

                TOTAL_BLOCKS = currentBlock;

                //SerialHW.println("");
                //SerialHW.println("+++++++++++++++++++++++++++++++++++++++++++++++");
                //SerialHW.println("");

          }

          _myTZX.numBlocks = currentBlock;
          _myTZX.size = sizeTZX;
          
    }
   
    void proccess_tzx(File32 tzxFileName)
    {
          // Procesamos el fichero
          ////SerialHW.println("");
          ////SerialHW.println("Getting total blocks...");     

        if (isFileTZX(tzxFileName))
        {
            // Esto lo hacemos para poder abortar
            ABORT=false;
            getBlockDescriptor(_mFile,_sizeTZX);
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
            #endif

            sprintf(hexs, "%X", buffer[size-1]);
            dataOffset4 = hexs;
            sprintf(hexs, "%X", buffer[size-2]);
            dataOffset3 = hexs;

            sprintf(hexs, "%X", offset+(size-1));
            Offset4 = hexs;
            sprintf(hexs, "%X", offset+(size-2));
            Offset3 = hexs;

            log("");
        }
    }

    public:

    tTZXBlockDescriptor* getDescriptor()
    {
        return _myTZX.descriptor;
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

    void set_file(File32 mFile, int sizeTZX)
    {
      // Pasamos los parametros a la clase
      _mFile = mFile;
      _sizeTZX = sizeTZX;
    }  

    void getInfoFileTZX(char* path) 
    {
      
      File32 tzxFile;
      
      PROGRAM_NAME_DETECTED = false;
      PROGRAM_NAME = "";
      PROGRAM_NAME_2 = "";

      LAST_MESSAGE = "Analyzing file";
    
      MULTIGROUP_COUNT = 1;

      // Abrimos el fichero
      tzxFile = sdm.openFile32(tzxFile, path);

      // Obtenemos su tamaño total
      _mFile = tzxFile;
      _rlen = tzxFile.available();

      if (_rlen != 0)
      {
        FILE_IS_OPEN = true;

        // creamos un objeto TZXproccesor
        set_file(tzxFile, _rlen);
        proccess_tzx(tzxFile);
        
        ////SerialHW.println("");
        ////SerialHW.println("END PROCCESING TZX: ");

        if (_myTZX.descriptor != nullptr && !ID_NOT_IMPLEMENTED)
        {
            // Entregamos información por consola
            //PROGRAM_NAME = _myTZX.name;
            strcpy(LAST_NAME,"              ");
        }
        else
        {
          LAST_MESSAGE = "Error in TZX or ID not supported";
          //_hmi.updateInformationMainPage();
        }

      }
      else
      {
        FILE_IS_OPEN = false;
        LAST_MESSAGE = "Error in TZX file has 0 bytes";
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

        log("Pulse len: " + String(descriptor.timming.pilot_len));
        log("Pulse nums: " + String(descriptor.timming.pilot_num_pulses));

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

            #ifdef DEBUGMODE
              log("Offset DATA:         " + String(offsetBase));
              log("Total size del DATA: " + String(totalSize));
            #endif


            // log("Información: ");
            // log(" - Tamaño total del bloque entero: " + String(totalSize));
            // log(" - Numero de particiones: " + String(blocks));
            // log(" - Ultimo bloque (size): " + String(lastBlockSize));
            // log(" - Offset: " + String(offsetBase));


            // BTI 0
            _zxp.BIT_0 = descriptor.timming.bit_0;
            // BIT1                                          
            _zxp.BIT_1 = descriptor.timming.bit_1;

            TOTAL_PARTS = blocks;

            // Recorremos el vector de particiones del bloque.
            for (int n=0;n < blocks;n++)
            {
              PARTITION_BLOCK = n;
              log("Envio el partition");

              // Calculamos el offset del bloque
              newOffset = offsetBase + (blockSizeSplit*n);
              // Accedemos a la SD y capturamos el bloque del fichero
              bufferPlay = sdm.readFileRange32(_mFile, newOffset, blockSizeSplit, true);
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
            }

            // Ultimo bloque
            //log("Particion [" + String(blocks) + "/" + String(blocks) + "]");

            // Calculamos el offset del último bloque
            newOffset = offsetBase + (blockSizeSplit*blocks);
            blockSizeSplit = lastBlockSize;
            // Accedemos a la SD y capturamos el bloque del fichero
            bufferPlay = sdm.readFileRange32(_mFile, newOffset,blockSizeSplit, true);
            // Mostramos en la consola los primeros y últimos bytes
            showBufferPlay(bufferPlay,blockSizeSplit,newOffset);         
            
            // Reproducimos el ultimo bloque con su terminador y silencio si aplica
            _zxp.playDataEnd(bufferPlay, blockSizeSplit);                                    

            free(bufferPlay);       
        }
        else
        {
            // Si es mas pequeño que el SPLIT, se reproduce completo.
            bufferPlay = sdm.readFileRange32(_mFile, descriptor.offsetData, descriptor.size, true);

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
      int playeableListID[] = {16, 17, 18, 19, 20, 21, 24, 25, 32, 35, 36, 37, 38, 39, 40, 42, 43, 90};
      for (int i = 0; i < 18; i++) 
      {
        if (playeableListID[i] == id) 
        { 
          res = true; //Inidicamos que lo hemos encontrado
          break;
        }
      }

      return res;
    }

    int getIdAndPlay(int i)
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
        //PROGRAM_NAME_2 = _myTZX.descriptor[BLOCK_SELECTED].name;

        switch (_myTZX.descriptor[i].ID)
        {
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
                  LAST_EAR_IS = POLARIZATION;   

                  //_zxp.closeBlock();

                  // PAUSE
                  // Pausamos la reproducción a través del HMI 

                  hmi.writeString("click btnPause,1"); 

                  PLAY = false;
                  STOP = false;
                  PAUSE = true;

                  LAST_MESSAGE = "Playing PAUSE.";
                  LAST_TZX_GROUP = "[STOP BLOCK]";

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
                  _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);                        
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
              LAST_TZX_GROUP = "[META BLK: " + String(_myTZX.descriptor[i].group) + "]";
              break;

            case 34:
              //LAST_TZX_GROUP = &INITCHAR[0];
              LAST_TZX_GROUP = "[GROUP END]";
              break;

            default:
              // Cualquier otro bloque entra por aquí, pero
              // hay que comprobar que sea REPRODUCIBLE

              // Reseteamos el indicador de META BLOCK
              // (añadido el 26/03/2024)
              LAST_TZX_GROUP = "...";

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
                        LAST_EAR_IS = down;
                        LOOP_START = 0;
                        LOOP_END = 0;
                        BL_LOOP_START = 0;
                        BL_LOOP_END = 0;

                        //return newPosition;
                    }

                    //Ahora vamos lanzando bloques dependiendo de su tipo
                    // Actualizamos HMI
                    _hmi.setBasicFileInformation(_myTZX.descriptor[i].name,_myTZX.descriptor[i].typeName,_myTZX.descriptor[i].size);

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

                        switch (_myTZX.descriptor[i].ID)
                        {
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
                                  log("ID 0x13:");
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
                                  int offsetBase = _myTZX.descriptor[i].offsetData;
                                  int newOffset = 0;
                                  double blocks = totalSize / blockSizeSplit;
                                  int lastBlockSize = totalSize - (blocks * blockSizeSplit);

                                  // BTI 0
                                  _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
                                  // BIT1                                          
                                  _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;

                                  // Recorremos el vector de particiones del bloque.
                                  for (int n=0;n < blocks;n++)
                                  {
                                    // Calculamos el offset del bloque
                                    newOffset = offsetBase + (blockSizeSplit*n);
                                    // Accedemos a la SD y capturamos el bloque del fichero
                                    bufferPlay = sdm.readFileRange32(_mFile, newOffset, blockSizeSplit, true);
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

                                  }

                                  // Ultimo bloque
                                  // Calculamos el offset del último bloque
                                  newOffset = offsetBase + (blockSizeSplit*blocks);
                                  blockSizeSplit = lastBlockSize;
                                  // Accedemos a la SD y capturamos el bloque del fichero
                                  bufferPlay = sdm.readFileRange32(_mFile, newOffset,blockSizeSplit, true);
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
                                  bufferPlay = sdm.readFileRange32(_mFile, _myTZX.descriptor[i].offsetData, _myTZX.descriptor[i].size, true);

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
            break;

        }      

        return newPosition;
    }

    void play()
    {

        LOOP_PLAYED = 0;
        // Inicializamos el buffer de reproducción. Memoria dinamica
        //uint8_t* bufferPlay = nullptr;
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
              if (BLOCK_SELECTED == 0) 
              {
                BYTES_LOADED = 0;
                BYTES_TOBE_LOAD = _rlen;
                PROGRESS_BAR_TOTAL_VALUE = (BYTES_LOADED * 100) / (BYTES_TOBE_LOAD);
              } 
              else 
              {
                BYTES_TOBE_LOAD = _rlen - _myTZX.descriptor[BLOCK_SELECTED - 1].offset;
              }
            

              // Recorremos ahora todos los bloques que hay en el descriptor
              //-------------------------------------------------------------
              for (int i = firstBlockToBePlayed; i < _myTZX.numBlocks; i++) 
              {               
                  BLOCK_SELECTED = i;                
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
                LAST_EAR_IS = POLARIZATION;       

                // Paramos
                PLAY = false;
                STOP = true;
                PAUSE = false;

                LAST_MESSAGE = "Playing end. Automatic STOP.";

                _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);
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
            _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);
        }        

    }

    TZXprocessor(AudioKit kit)
    {
        // Constructor de la clase
        strncpy(_myTZX.name,"          ",10);
        _myTZX.numBlocks = 0;
        _myTZX.size = 0;
        _myTZX.descriptor = nullptr;

        _zxp.set_ESP32kit(kit);      
    } 
};


