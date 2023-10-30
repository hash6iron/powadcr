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

class TZXproccesor
{

    private:

    // Procesador de audio output
    ZXProccesor _zxp;

    HMI _hmi;

    // Definicion de un TZX
    SdFat32 _sdf32;
    tTZX _myTZX;
    File32 _mFile;
    int _sizeTZX;
    int _rlen;

    int CURRENT_LOADING_BLOCK = 0;

    static void tryAllocateSRamThenPSRam(tTZXBlockDescriptor*& ds,int size)
    {
        // Liberamos
        // if (ds != NULL)
        // {
        //   free(ds);
        // }

        // only try allocating in SRAM when there are 96K free at least
        if (ESP.getFreeHeap() >= 256*1024)
        {
            ds = (tTZXBlockDescriptor*)calloc(size, sizeof(tTZXBlockDescriptor));
            if (ds != NULL)
            {
                SerialHW.printf("Ds allocated into SRAM (fast)");
                return;
            }
        }
        
        ds = (tTZXBlockDescriptor*)ps_calloc(size, sizeof(tTZXBlockDescriptor));
        if (ds != NULL) 
        {
            SerialHW.printf("Ds allocated into PSRAM (slow)");
            return;
        }
        //
        SerialHW.printf("ERROR: unable to allocate ds");
    }


    byte calculateChecksum(byte* bBlock, int startByte, int numBytes)
    {
        // Calculamos el checksum de un bloque de bytes
        byte newChk = 0;

        #if LOG>3
          SerialHW.println("");
          SerialHW.println("Block len: ");
          SerialHW.print(sizeof(bBlock)/sizeof(byte*));
        #endif

        // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
        
        for (int n=startByte;n<(startByte+numBytes);n++)
        {
            newChk = newChk ^ bBlock[n];
        }

        #if LOG>3
          SerialHW.println("");
          SerialHW.println("Checksum: " + String(newChk));
        #endif

        return newChk;
    }      

    char* getNameFromStandardBlock(byte* header)
    {
        // Obtenemos el nombre del bloque cabecera
        char* prgName = (char*)ps_calloc(10+1,sizeof(char));

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
            

            SerialHW.println("");
            SerialHW.print(String(prgName[n-2]));
        }

        // Pasamos la cadena de caracteres
        return prgName;

      }

     byte* getBlockRange(byte* bBlock, int byteStart, int byteEnd)
     {

         // Extraemos un bloque de bytes indicando el byte inicio y final
         //byte* blockExtracted = new byte[(byteEnd - byteStart)+1];
         byte* blockExtracted = (byte*)ps_calloc((byteEnd - byteStart)+1, sizeof(byte));

         int i=0;
         for (int n=byteStart;n<(byteStart + (byteEnd - byteStart)+1);n++)
         {
             blockExtracted[i] = bBlock[n];
         }

         return blockExtracted;
     }

     bool isHeaderTZX(File32 tzxFile)
     {
        if (tzxFile != 0)
        {

           SerialHW.println("");
           SerialHW.println("Begin isHeaderTZX");

           // Capturamos la cabecera

           //byte* bBlock = NULL; 
           byte* bBlock = (byte*)ps_calloc(10+1,sizeof(byte));
           bBlock = sdm.readFileRange32(tzxFile,0,10,false);

          // Obtenemos la firma del TZX
          char* signTZXHeader = (char*)ps_calloc(8+1,sizeof(char));

          // Analizamos la cabecera
          // Extraemos el nombre del programa
           for (int n=0;n<7;n++)
           {   
               signTZXHeader[n] = (char)bBlock[n];
           }
           
           //Aplicamos un terminador a la cadena de char
           signTZXHeader[7] = '\0';
           //Convertimos a String
           String signStr = String(signTZXHeader);
           
           SerialHW.println("");
           SerialHW.println("Sign: " + signStr);

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
            
            //String szNameStr = sdm.getFileName(tzxFileName);
            String fileName = String(szName);

            SerialHW.println("");
            SerialHW.println("Name " + fileName);

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

    int getDWORD(File32 mFile, int offset)
    {
        int sizeDW = 0;
        sizeDW = (256*sdm.readFileRange32(mFile,offset+1,1,false)[0]) + sdm.readFileRange32(mFile,offset,1,false)[0];  

        return sizeDW;    
    }

    int getBYTE(File32 mFile, int offset)
    {
        int sizeB = 0;
        sizeB = sdm.readFileRange32(mFile,offset,1,false)[0];  

        return sizeB;      
    }

    int getNBYTE(File32 mFile, int offset, int n)
    {
        int sizeNB = 0;

        SerialHW.println("");
        SerialHW.println("HEX VALUE n bytes");

        for (int i = 0; i<n;i++)
        {
            sizeNB += pow(2,(8*i)) * (sdm.readFileRange32(mFile,offset+i,1,false)[0]);  
        }

        SerialHW.println("");
        SerialHW.println("DECIMAL");
        SerialHW.println(String(sizeNB));

        return sizeNB;           
    }

    int getID(File32 mFile, int offset)
    {
        // Obtenemos el ID
        int ID = 0;
        ID = getBYTE(mFile,offset);

        return ID;      
    }


    byte* getBlock(File32 mFile, int offset, int size)
    {
        //Entonces recorremos el TZX. 
        // La primera cabecera SIEMPRE debe darse.
        byte* bloque = (byte*)ps_calloc(size+1,sizeof(byte));
        // Obtenemos el bloque
        if (bloque != NULL)
        {
            bloque = sdm.readFileRange32(mFile,offset,size,false);          
        }

        return bloque;
    }

    bool verifyChecksum(File32 mFile, int offset, int size)
    {
        // Vamos a verificar que el bloque cumple con el checksum
        // Cogemos el checksum del bloque
        byte chk = getBYTE(mFile,offset+size-1);

         SerialHW.println("");
         SerialHW.println("Original checksum:");
         SerialHW.print(chk,HEX);

        byte* block = getBlock(mFile,offset,size-1);
        byte calcChk = calculateChecksum(block,0,size-1);

         SerialHW.println("");
         SerialHW.println("Calculated checksum:");
         SerialHW.print(calcChk,HEX);
         SerialHW.println("");

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
        
        tTZXBlockDescriptor blockDescriptor; 
        _myTZX.descriptor[currentBlock].ID = 16;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        SerialHW.println("");
        SerialHW.println("Offset begin: ");
        SerialHW.print(currentOffset,HEX);

        //SYNC1
        // zxp.SYNC1 = DSYNC1;
        // //SYNC2
        // zxp.SYNC2 = DSYNC2;
        // //PULSE PILOT
        // zxp.PULSE_PILOT = DPULSE_PILOT;                              
        // // BTI 0
        // zxp.BIT_0 = DBIT_0;
        // // BIT1                                          
        // zxp.BIT_1 = DBIT_1;
        // No contamos el ID. Entonces:
        
        // Timming de la ROM
        _myTZX.descriptor[currentBlock].timming.pulse_pilot = DPULSE_PILOT;
        _myTZX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
        _myTZX.descriptor[currentBlock].timming.sync_2 = DSYNC2;
        _myTZX.descriptor[currentBlock].timming.bit_0 = DBIT_0;
        _myTZX.descriptor[currentBlock].timming.bit_1 = DBIT_1;

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getDWORD(mFile,currentOffset+1);

        SerialHW.println("");
        SerialHW.println("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getDWORD(mFile,currentOffset+3);

        SerialHW.println("");
        SerialHW.println("Length of data: ");
        SerialHW.print(_myTZX.descriptor[currentBlock].lengthOfData,HEX);

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 5;

        SerialHW.println("");
        SerialHW.println("Offset of data: ");
        SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);

        // Vamos a verificar el flagByte
        int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        // Y cogemos para ver el tipo de cabecera
        int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);

        // Si el flagbyte es menor a 0x80
        if (flagByte < 128)
        {
            // Es una cabecera
            if (typeBlock == 0)
            {
                // Es una cabecera PROGRAM: BASIC
              SerialHW.println("");
              SerialHW.println("TYPEBLOCK: 0 - Header");
              SerialHW.println("");

              SerialHW.println("");
              SerialHW.println("TYPE: 0");
              SerialHW.println("");

              _myTZX.descriptor[currentBlock].header = true;
              _myTZX.descriptor[currentBlock].type = 0;

              //_myTZX.name = "TZX";
              if (!PROGRAM_NAME_DETECTED)
              {
                  PROGRAM_NAME = String(getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19)));

                  SerialHW.println("");
                  SerialHW.println("");
                  SerialHW.println("PROGRAM NAME: " + PROGRAM_NAME);

                  PROGRAM_NAME_DETECTED = true;  
              }


              // Almacenamos el nombre del bloque
              _myTZX.descriptor[currentBlock].name = _myTZX.name;

              // SerialHW.println("");
              // SerialHW.println("Nombre detectado: " + String(_myTZX.name));

            }
            else if (typeBlock == 1)
            {
                SerialHW.println("");
                SerialHW.println("TYPEBLOCK: 1 - Header");
                SerialHW.println("");

                SerialHW.println("");
                SerialHW.println("TYPE: 0");
                SerialHW.println("");

                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 2)
            {
                SerialHW.println("");
                SerialHW.println("TYPEBLOCK: 2 - Header");
                SerialHW.println("");

                SerialHW.println("");
                SerialHW.println("TYPE: 0");
                SerialHW.println("");

                _myTZX.descriptor[currentBlock].header = true;
                _myTZX.descriptor[currentBlock].type = 0;
            }
            else if (typeBlock == 3)
            {

              SerialHW.println("");
              SerialHW.println("TYPEBLOCK: 3 - Header");
              SerialHW.println("");

              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  SerialHW.println("");
                  SerialHW.println("TYPE: 7");
                  SerialHW.println("");

                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 7;

                  // Almacenamos el nombre del bloque
                  _myTZX.descriptor[currentBlock].name = getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19));
              }
              else
              {
                // Es un bloque BYTE
                SerialHW.println("");
                SerialHW.println("TYPE: 1");
                SerialHW.println("");
                _myTZX.descriptor[currentBlock].type = 1;     

                // Almacenamos el nombre del bloque
                _myTZX.descriptor[currentBlock].name = getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19));                         
              }
            }
        }
        else
        {
            if (typeBlock == 3)
            {
              SerialHW.println("");
              SerialHW.println("TYPEBLOCK: 3");
              SerialHW.println("");

              if (_myTZX.descriptor[currentBlock].lengthOfData == 6914)
              {
                  SerialHW.println("");
                  SerialHW.println("TYPE: 3");
                  SerialHW.println("");
                  _myTZX.descriptor[currentBlock].screen = true;
                  _myTZX.descriptor[currentBlock].type = 3;
              }
              else
              {
                  // Es un bloque BYTE
                  SerialHW.println("");
                  SerialHW.println("TYPE: 4");
                  SerialHW.println("");
                  _myTZX.descriptor[currentBlock].type = 4;                
              }
            }
            else
            {
                // Es un bloque BYTE
                SerialHW.println("");
                SerialHW.println("TYPE: 4");
                SerialHW.println("");
                _myTZX.descriptor[currentBlock].type = 4;
            }            
        } 

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getDWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData; 

        // Por defecto en ID10 no afecta, pero lo inicializamos 
        // para evitar problemas con el ID11
        zxp.set_maskLastByte(8);
    }
    
    void analyzeID17(File32 mFile, int currentOffset, int currentBlock)
    {
        // Turbo data

        _myTZX.descriptor[currentBlock].ID = 17;
        _myTZX.descriptor[currentBlock].playeable = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // Entonces
        // Timming de PULSE PILOT
        _myTZX.descriptor[currentBlock].timming.pulse_pilot = getDWORD(mFile,currentOffset+1);

        SerialHW.println("");
        SerialHW.print("PULSE PILOT=");
        SerialHW.print(_myTZX.descriptor[currentBlock].timming.pulse_pilot, HEX);

        // Timming de SYNC_1
        _myTZX.descriptor[currentBlock].timming.sync_1 = getDWORD(mFile,currentOffset+3);

          //SerialHW.println("");
          SerialHW.print(",SYNC1=");
          SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_1, HEX);

        // Timming de SYNC_2
        _myTZX.descriptor[currentBlock].timming.sync_2 = getDWORD(mFile,currentOffset+5);

          //SerialHW.println("");
          SerialHW.print(",SYNC2=");
          SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_2, HEX);

        // Timming de BIT 0
        _myTZX.descriptor[currentBlock].timming.bit_0 = getDWORD(mFile,currentOffset+7);

          //SerialHW.println("");
          SerialHW.print(",BIT_0=");
          SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_0, HEX);

        // Timming de BIT 1
        _myTZX.descriptor[currentBlock].timming.bit_1 = getDWORD(mFile,currentOffset+9);        

          //SerialHW.println("");
          SerialHW.print(",BIT_1=");
          SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_1, HEX);

        // Timming de PILOT TONE
        _myTZX.descriptor[currentBlock].timming.pilot_tone = getDWORD(mFile,currentOffset+11);

          //SerialHW.println("");
          SerialHW.print(",PILOT TONE=");
          SerialHW.print(_myTZX.descriptor[currentBlock].timming.pilot_tone, HEX);

        // Cogemos el byte de bits of the last byte
        _myTZX.descriptor[currentBlock].maskLastByte = getBYTE(mFile,currentOffset+13);
        zxp.set_maskLastByte(_myTZX.descriptor[currentBlock].maskLastByte);

          //SerialHW.println("");
          SerialHW.print(",MASK LAST BYTE=");
          SerialHW.print(_myTZX.descriptor[currentBlock].maskLastByte, HEX);


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
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getDWORD(mFile,currentOffset+14);

        SerialHW.println("");
        SerialHW.println("Pause after block: " + String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[currentBlock].lengthOfData = getNBYTE(mFile,currentOffset+16,3);

        SerialHW.println("");
        SerialHW.println("Length of data: ");
        SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));

        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[currentBlock].offsetData = currentOffset + 19;

        // Almacenamos el nombre del bloque
        _myTZX.descriptor[currentBlock].name = getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19));

        SerialHW.println("");
        SerialHW.println("Offset of data: ");
        SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);

        // Vamos a verificar el flagByte

        int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
        int typeBlock = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);

        if (flagByte < 128)
        {
            // Es una cabecera
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
            _myTZX.descriptor[currentBlock].type = 4;
        }

        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        // int positionOfTAPblock = currentOffset + 4;
        // dataTAPsize = getDWORD(mFile,positionOfTAPblock + headerTAPsize + 1);
        
        // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
        _myTZX.descriptor[currentBlock].size = _myTZX.descriptor[currentBlock].lengthOfData; 


    }

    void analyzeID18(File32 mFile, int currentOffset, int currentBlock)
    {
        // pure tone

        _myTZX.descriptor[currentBlock].ID = 18;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].forSetting = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        // Timming de PILOT TONE
        _myTZX.descriptor[currentBlock].timming.pilot_tone = getDWORD(mFile,currentOffset+1) * getDWORD(mFile,currentOffset+3);          
    }

    void analyzeID32(File32 mFile, int currentOffset, int currentBlock)
    {
        // Con este ID analizamos la parte solo del timming ya que el resto
        // información del bloque se puede analizar con "analyzeID16"

        _myTZX.descriptor[currentBlock].ID = 32;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].forSetting = true;
        _myTZX.descriptor[currentBlock].offset = currentOffset;    

        // Timming de PILOT TONE
        _myTZX.descriptor[currentBlock].pauseAfterThisBlock = getDWORD(mFile,currentOffset+1);          
    }

    void analyzeID50(File32 mFile, int currentOffset, int currentBlock)
    {
        // Information block
        int sizeBlock = 0;

        _myTZX.descriptor[currentBlock].ID = 50;
        _myTZX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTZX.descriptor[currentBlock].offset = currentOffset+3;

        sizeBlock = getDWORD(mFile,currentOffset+1);

        //SerialHW.println("");
        //SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeBlock;         
    }    

    void analyzeID51(File32 mFile, int currentOffset, int currentBlock)
    {
        // Hardware Information block
        int sizeBlock = 0;

        _myTZX.descriptor[currentBlock].ID = 51;
        _myTZX.descriptor[currentBlock].playeable = false;
        // Los dos primeros bytes del bloque no se cuentan para
        // el tamaño total
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        // El bloque completo mide, el numero de maquinas a listar
        // multiplicado por 3 byte por maquina listada
        sizeBlock = getBYTE(mFile,currentOffset+1) * 3;

        //SerialHW.println("");
        //SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeBlock-1;         
    }

    void analyzeID48(File32 mFile, int currentOffset, int currentBlock)
    {
        // Information block
        int sizeTextInformation = 0;

        _myTZX.descriptor[currentBlock].ID = 48;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        SerialHW.println("");
        SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[currentBlock].size = sizeTextInformation + 1;
    }

    void analyzeID33(File32 mFile, int currentOffset, int currentBlock)
    {
        // Group start
        int sizeTextInformation = 0;

        _myTZX.descriptor[currentBlock].ID = 33;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset+2;

        sizeTextInformation = getBYTE(mFile,currentOffset+1);

        SerialHW.println("");
        SerialHW.println("ID33 - TextSize: " + String(sizeTextInformation));
        
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

          const int maxAllocationBlocks = 4000;

          currentOffset = startOffset;

          // Hacemos allocation in memory

          tryAllocateSRamThenPSRam(_myTZX.descriptor,maxAllocationBlocks);

          TIMMING_STABLISHED = false;

          while (!endTZX && !forzeEnd)
          {
             
              if (ABORT==true)
              {
                  forzeEnd = true;
                  endWithErrors = true;
                  LAST_MESSAGE = "Aborting. No proccess complete.";
                  _hmi.updateInformationMainPage();
              }

              // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque completo para el descriptor.
              currentID = getID(mFile, currentOffset);
              
              SerialHW.println("");
              SerialHW.println("-----------------------------------------------");
              SerialHW.println("TZX ID ");
              SerialHW.print(currentID, HEX);
              SerialHW.println("");
              SerialHW.println("block: " + String(currentBlock));
              SerialHW.println("");

              // Ahora dependiendo del ID analizamos. Los ID están en HEX
              // y la rutina devolverá la posición del siguiente ID, así hasta
              // completar todo el fichero
              switch (currentID)
              {
                // ID 10 - Standard Speed Data Block
                case 16:

                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID16(mFile,currentOffset, currentBlock);

                      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 5;
                      
                      _myTZX.descriptor[currentBlock].typeName = "ID 10 - Standard block";

                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x10");
                      endTZX = true;
                      endWithErrors = true;
                  }
                break;

                // ID 11- Turbo Speed Data Block
                case 17:
                  
                  TIMMING_STABLISHED = true;
                  
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID17(mFile,currentOffset, currentBlock);

                      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 19;
                      
                      _myTZX.descriptor[currentBlock].typeName = "ID 11 - Speed block";

                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x11");
                      endTZX = true;
                      endWithErrors = true;
                  }
                break;

                // ID 12 - Pure Tone
                case 18:
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID16(mFile,currentOffset, currentBlock);

                      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 4;
                      
                      _myTZX.descriptor[currentBlock].typeName = "ID 12 - Pure tone";

                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x12");
                      endTZX = true;
                      endWithErrors = true;
                  }
                break;

                // ID 13 - Pulse sequence
                case 19:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 13 - Pulse seq.";
                  currentBlock++;
                  break;

                // ID 14 - Pure Data Block
                case 20:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 14 - Pure Data block";
                  currentBlock++;
                  break;

                // ID 15 - Direct Recording
                case 21:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 15 - Direct REC";
                  currentBlock++;
                  break;

                // ID 18 - CSW Recording
                case 24:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 18 - CSW Rec";currentBlock++;
                  break;

                // ID 19 - Generalized Data Block
                case 25:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 19 - General Data block";
                  currentBlock++;
                  break;

                // ID 20 - Pause and Stop Tape
                case 32:
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID32(mFile,currentOffset, currentBlock);

                      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 3;
                      _myTZX.descriptor[currentBlock].typeName = "ID 20 - Pause TAPE";    
                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x20");
                      endTZX = true;
                      endWithErrors = true;
                  }
                  break;

                // ID 21 - Group start
                case 33:
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID33(mFile,currentOffset, currentBlock);

                      nextIDoffset = currentOffset + 2 + _myTZX.descriptor[currentBlock].size;
                      _myTZX.descriptor[currentBlock].typeName = "ID 21 - Group start";
                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x21");
                      endTZX = true;
                      endWithErrors = true;
                  }
                  break;                

                // ID 22 - Group end
                case 34:
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      _myTZX.descriptor[currentBlock].ID = 34;
                      _myTZX.descriptor[currentBlock].playeable = false;
                      _myTZX.descriptor[currentBlock].offset = currentOffset;

                      nextIDoffset = currentOffset + 1;                      
                      _myTZX.descriptor[currentBlock].typeName = "ID 22 - Group end";
                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x22");
                      endTZX = true;
                      endWithErrors = true;
                  }
                  break;

                // ID 23 - Jump to block
                case 35:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 23 - Jump to block";
                  currentBlock++;
                  break;

                // ID 24 - Loop start
                case 36:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 24 - Loop start";
                  currentBlock++;
                  break;

                // ID 25 - Loop end
                case 37:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 25 - Loop end";
                  currentBlock++;
                  break;

                // ID 26 - Call sequence
                case 38:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 26 - Call seq.";
                  currentBlock++;
                  break;

                // ID 27 - Return from sequence
                case 39:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 27 - Return from seq.";
                  currentBlock++;
                  break;

                // ID 28 - Select block
                case 40:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 28 - Select block";
                  currentBlock++;
                  break;

                // ID 2A - Stop the tape if in 48K mode
                case 42:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 2A - Stop TAPE (48k mode)";
                  currentBlock++;
                  break;

                // ID 2B - Set signal level
                case 43:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 2B - Set signal level";
                  currentBlock++;
                  break;

                // ID 30 - Information
                case 48:
                  // No hacemos nada solamente coger el siguiente offset
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID48(mFile,currentOffset,currentBlock);

                      // Siguiente ID
                      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;

                      _myTZX.descriptor[currentBlock].typeName = "ID 30 - Information";

                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x30");
                      endTZX = true;
                      endWithErrors = true;
                  }                  
                  break;

                // ID 31 - Message block
                case 49:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;                  

                  _myTZX.descriptor[currentBlock].typeName = "ID 31 - Message block";
                  currentBlock++;
                  break;

                // ID 32 - Archive info
                case 50:
                  // No hacemos nada solamente coger el siguiente offset
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID50(mFile,currentOffset,currentBlock);

                      // Siguiente ID
                      nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
                      _myTZX.descriptor[currentBlock].typeName = "ID 32 - Archive info";
                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x32");
                      endTZX = true;
                      endWithErrors = true;
                  }                  
                  break;

                // ID 33 - Hardware type
                case 51:
                  // No hacemos nada solamente coger el siguiente offset
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      analyzeID51(mFile,currentOffset,currentBlock);

                      // Siguiente ID
                      nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
                      _myTZX.descriptor[currentBlock].typeName = "ID 33- Hardware type";
                      currentBlock++;
                  }
                  else
                  {
                      SerialHW.println("");
                      SerialHW.println("Error: Not allocation memory for block ID 0x33");
                      endTZX = true;
                      endWithErrors = true;
                  }                  
                  break;

                // ID 35 - Custom info block
                case 53:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 35 - Custom info block";
                  currentBlock++;
                  break;

                // ID 5A - "Glue" block (90 dec, ASCII Letter 'Z')
                case 90:
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "ID 5A - Glue block";
                  currentBlock++;
                  break;

                default:
                  SerialHW.println("");
                  SerialHW.println("ID unknow " + currentID);
                  analyzeBlockUnknow(currentID,currentOffset, currentBlock);
                  nextIDoffset = currentOffset + 1;            

                  _myTZX.descriptor[currentBlock].typeName = "Unknown block";
                  break;
                }

                SerialHW.println("");
                SerialHW.println("Next ID offset: ");
                SerialHW.print(nextIDoffset, HEX);
                
                if(currentBlock > maxAllocationBlocks)
                {
                  SerialHW.println("Error. TZX not possible to allocate in memory");  
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

                //delay(2000);
                TOTAL_BLOCKS = currentBlock;
                _hmi.updateInformationMainPage();

                SerialHW.println("");
                SerialHW.println("+++++++++++++++++++++++++++++++++++++++++++++++");
                SerialHW.println("");

          }
          // El siguiente bloque será
          // currentOffset + blockSize (que obtenemos del descriptor)
          SerialHW.println("");
          SerialHW.println("End: Total blocks " + String(currentBlock));

          _myTZX.numBlocks = currentBlock;
          _myTZX.size = sizeTZX;
          //_myTZX.name = "TZX";
          
          // Actualizamos una vez mas el HMI
          if (currentBlock % 100 == 0)
          {
              _hmi.updateInformationMainPage();
          }
    }
   
    void proccess_tzx(File32 tzxFileName)
    {
          // Procesamos el fichero
          SerialHW.println("");
          SerialHW.println("Getting total blocks...");     

        if (isFileTZX(tzxFileName))
        {
            SerialHW.println();
            SerialHW.println("Is TZX file");

            SerialHW.println();
            SerialHW.println("Size: " + String(_sizeTZX));

            // Esto lo hacemos para poder abortar
            ABORT=false;

            getBlockDescriptor(_mFile,_sizeTZX);
        }      
    }

    void sendStatus(int action, int value) 
    {

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

    void showBufferPlay(byte* buffer, int size)
    {
        SerialHW.println("Listing bufferplay.");
        for (int n=0;n<size;n++)
        {
            SerialHW.print(buffer[n],HEX);
            SerialHW.print(",");
        }
        SerialHW.println("");
    }

    public:

    void showInfoBlockInProgress(int nBlock)
    {
        // switch(nBlock)
        // {
        //     case 0:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("> PROGRAM HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE0[0];
        //         break;

        //     case 1:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> BYTE HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE1[0];
        //         break;

        //     case 7:

        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> SCREEN HEADER");
        //         #endif
        //         LAST_TYPE = &LASTYPE7[0];
        //         break;

        //     case 2:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> BASIC PROGRAM");
        //         #endif
        //         LAST_TYPE = &LASTYPE2[0];
        //         break;

        //     case 3:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> SCREEN");
        //         #endif
        //         LAST_TYPE = &LASTYPE3[0];
        //         break;

        //     case 4:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> BYTE CODE");
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
        //           SerialHW.println("");
        //           SerialHW.println("> ARRAY.NUM");
        //         #endif
        //         LAST_TYPE = &LASTYPE5[0];
        //         break;

        //     case 6:
        //         // Definimos el buffer del PLAYER igual al tamaño del bloque
        //         #if LOG==3
        //           SerialHW.println("");
        //           SerialHW.println("> ARRAY.CHR");
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
      _hmi.updateInformationMainPage();
    
      // Abrimos el fichero
      tzxFile = sdm.openFile32(tzxFile, path);

      // Obtenemos su tamaño total
      _mFile = tzxFile;
      _rlen = tzxFile.available();

      // creamos un objeto TZXproccesor
      set_file(tzxFile, _rlen);
      proccess_tzx(tzxFile);
      
      SerialHW.println("");
      SerialHW.println("END PROCCESING TZX: ");

      if (_myTZX.descriptor != NULL)
      {
          // Entregamos información por consola
          //PROGRAM_NAME = _myTZX.name;
          LAST_NAME = &INITCHAR2[0];
    
          SerialHW.println("");
          SerialHW.println("TZX");
          SerialHW.println("PROGRAM_NAME: " + PROGRAM_NAME);
          SerialHW.println("TOTAL_BLOCKS: " + String(TOTAL_BLOCKS));
    
          // Pasamos el descriptor           
          _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);

          _hmi.updateInformationMainPage();
      }
    
    }

    void initialize()
    {
        // if (_myTZX.descriptor != NULL)
        // {
        //   //free(_myTZX.descriptor);
        //   //free(_myTZX.name);
        //   //_myTZX.descriptor = NULL;

        // }          

        _myTZX.name = "\0";
        _myTZX.numBlocks = 0;
        _myTZX.size = 0;
    }

    void terminate()
    {
      free(_myTZX.descriptor);
      _myTZX.descriptor = NULL;
    }

    void play()
    {
        //_hmi.writeString("");
        _hmi.writeString("READYst.val=0");

        //_hmi.writeString("");
        _hmi.writeString("ENDst.val=0");

        if (_myTZX.descriptor != NULL)
        {         
              // Inicializamos el buffer de reproducción. Memoria dinamica
              byte* bufferPlay = NULL;

              // Entregamos información por consola
              //PROGRAM_NAME = _myTZX.name;
              TOTAL_BLOCKS = _myTZX.numBlocks;
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
                BYTES_TOBE_LOAD = _rlen - _myTZX.descriptor[BLOCK_SELECTED - 1].offset;
              }

              for (int i = m; i < _myTZX.numBlocks; i++) 
              {
                
                // Ahora lo voy actualizando a medida que van avanzando los bloques.
                PROGRAM_NAME_2 = _myTZX.descriptor[BLOCK_SELECTED].name;

                if (_myTZX.descriptor[i].ID == 18)
                {
                    // Reproducimos un tono puro
                    zxp.pilotTone(_myTZX.descriptor[i].timming.pilot_tone);
                }

                if (_myTZX.descriptor[i].ID == 32)
                {
                    // Hacemos un delay
                    int dly = _myTZX.descriptor[i].pauseAfterThisBlock;
                    if (dly == 0)
                    {
                        SerialHW.println("");
                        SerialHW.println("Esperando a STOP AUTO");
                        
                        // En el caso de cero. Paramos el TAPE (PAUSA, porque esto es para poder cargar bloques después)
                        //i = _myTZX.numBlocks;
                        // PAUSE
                        //Pausamos la reproducción a través
                        //del HMI 
                        hmi.writeString("click btnPause,1"); 
                        //break;
                    }
                    else
                    {
                        // En otro caso. delay
                        zxp.silence(dly);                        
                    }                    
                }

                if (_myTZX.descriptor[i].ID == 33)
                {
                  // Comienza multigrupo
                  
                  LAST_TZX_GROUP = "[G: " + String(i) + "]";
                }
                
                if (_myTZX.descriptor[i].ID == 34)
                {
                  LAST_TZX_GROUP = &INITCHAR[0];
                }

                if (_myTZX.descriptor[i].playeable)
                {
                      //Silent
                      zxp.silent = _myTZX.descriptor[i].pauseAfterThisBlock;        

                      //SYNC1
                      zxp.SYNC1 = _myTZX.descriptor[i].timming.sync_1;
                      //SYNC2
                      zxp.SYNC2 = _myTZX.descriptor[i].timming.sync_2;
                      //PULSE PILOT
                      zxp.PULSE_PILOT = _myTZX.descriptor[i].timming.pulse_pilot;
                      // BTI 0
                      zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
                      // BIT1                                          
                      zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;

                      //LAST_NAME = bDscr[i].name;
                      // Obtenemos el nombre del bloque
                      LAST_NAME = _myTZX.descriptor[i].name;
                      LAST_SIZE = _myTZX.descriptor[i].size;
                      LAST_TYPE = _myTZX.descriptor[i].typeName;

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
                      showInfoBlockInProgress(_myTZX.descriptor[i].type);

                      // Actualizamos HMI
                      _hmi.setBasicFileInformation(_myTZX.descriptor[i].name,_myTZX.descriptor[i].typeName,_myTZX.descriptor[i].size);

                      _hmi.updateInformationMainPage();

                      // Reproducimos el fichero
                      if (_myTZX.descriptor[i].type == 0) 
                      {

                        SerialHW.println("");
                        SerialHW.println("Head 1" + _myTZX.descriptor[i].ID);
                        SerialHW.println("");

                        bufferPlay = (byte*)ps_calloc(_myTZX.descriptor[i].size, sizeof(byte));

                        bufferPlay = sdm.readFileRange32(_mFile, _myTZX.descriptor[i].offsetData, _myTZX.descriptor[i].size, false);

                        showBufferPlay(bufferPlay,_myTZX.descriptor[i].size);

                        verifyChecksum(_mFile,_myTZX.descriptor[i].offsetData,_myTZX.descriptor[i].size);


                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            //PULSE PILOT
                            _myTZX.descriptor[i].timming.pilot_tone = DPILOT_HEADER;
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;

                          case 17:
                            //PULSE PILOT
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;
                        }

                        // Llamamos a la clase de reproducción
                        // Cabecera PROGRAM
                        int pt = _myTZX.descriptor[i].timming.pilot_tone;
                        int pp = _myTZX.descriptor[i].timming.pulse_pilot;
                        int si = _myTZX.descriptor[i].size;
                        
                        // Reproducimos el bloque - PLAY
                        zxp.playData(bufferPlay, si, pt * pp);

                      } 
                      else if (_myTZX.descriptor[i].type == 1 || _myTZX.descriptor[i].type == 7) 
                      {

                        bufferPlay = (byte*)ps_calloc(_myTZX.descriptor[i].size, sizeof(byte));

                        bufferPlay = sdm.readFileRange32(_mFile, _myTZX.descriptor[i].offsetData, _myTZX.descriptor[i].size, false);

                        //showBufferPlay(bufferPlay,_myTZX.descriptor[i].size);
                        verifyChecksum(_mFile,_myTZX.descriptor[i].offsetData,_myTZX.descriptor[i].size);


                        // Llamamos a la clase de reproducción
                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            //PULSE PILOT
                            _myTZX.descriptor[i].timming.pilot_tone = DPILOT_HEADER;
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;

                          case 17:
                            //PULSE PILOT
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;
                        }
                        
                        // Cabecera BYTE
                        zxp.playData(bufferPlay, _myTZX.descriptor[i].size,_myTZX.descriptor[i].timming.pilot_tone * _myTZX.descriptor[i].timming.pulse_pilot);
                      } 
                      else 
                      {
                        // DATA
                        int blockSize = _myTZX.descriptor[i].size;

                        bufferPlay = (byte*)ps_calloc(_myTZX.descriptor[i].size, sizeof(byte));

                        bufferPlay = sdm.readFileRange32(_mFile, _myTZX.descriptor[i].offsetData, _myTZX.descriptor[i].size, true);

                        //showBufferPlay(bufferPlay,_myTZX.descriptor[i].size);
                        verifyChecksum(_mFile,_myTZX.descriptor[i].offsetData,_myTZX.descriptor[i].size);


                        switch (_myTZX.descriptor[i].ID)
                        {
                          case 16:
                            //PULSE PILOT
                            _myTZX.descriptor[i].timming.pilot_tone = DPILOT_DATA;
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;

                          case 17:
                            //PULSE PILOT
                            zxp.PILOT_TONE = _myTZX.descriptor[i].timming.pilot_tone;
                            break;
                        }

                        // Bloque de datos BYTE
                        zxp.playData(bufferPlay, _myTZX.descriptor[i].size,_myTZX.descriptor[i].timming.pilot_tone * _myTZX.descriptor[i].timming.pulse_pilot);
                      }                  
                }
                
              }

              // En el caso de no haber parado manualmente, es por finalizar
              // la reproducción
              if (LOADING_STATE == 1) {
                PLAY = false;
                STOP = true;
                PAUSE = false;
                BLOCK_SELECTED = 0;
                LAST_MESSAGE = "Playing end. Automatic STOP.";

                _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);

                _hmi.updateInformationMainPage();

                // Ahora ya podemos tocar el HMI panel otra vez
                sendStatus(END_ST, 1);
              }

              // Cerrando
              LOADING_STATE = 0;
             
              //SerialHW.flush();

              // Ahora ya podemos tocar el HMI panel otra vez
              sendStatus(READY_ST, 1);

              // Liberamos el descriptor
              //free(_myTZX.descriptor);
        }
        else
        {
            LAST_MESSAGE = "No file selected.";
            _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].name,_myTZX.descriptor[BLOCK_SELECTED].typeName,_myTZX.descriptor[BLOCK_SELECTED].size);

            _hmi.updateInformationMainPage();
        }        

    }

    TZXproccesor(AudioKit kit)
    {
        // Constructor de la clase
        _myTZX.name = (char*)ps_calloc(10+1,sizeof(char));
        _myTZX.name = &INITCHAR[0];
        _myTZX.numBlocks = 0;
        _myTZX.descriptor = NULL;
        _myTZX.size = 0;

        //zxp.set_ESP32kit(kit);      
    } 
};


