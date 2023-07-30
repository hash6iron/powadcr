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

class TZXproccesor
{

    private:

    HMI _hmi;

    // Definicion de un TZX
    SdFat32 _sdf32;
    tTZX _myTZX;
    File32 _mFile;
    int _sizeTZX;
    int _rlen;

    int CURRENT_LOADING_BLOCK = 0;

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

     bool isHeaderTZX(File32 tzxFile)
     {
        if (tzxFile != 0)
        {

           Serial.println("");
           Serial.println("Begin isHeaderTZX");

           // Capturamos la cabecera

           //byte* bBlock = NULL; 
           byte* bBlock = (byte*)calloc(10+1,sizeof(byte));
           bBlock = sdm.readFileRange32(tzxFile,0,10,true);

          // Obtenemos la firma del TZX
          char* signTZXHeader = (char*)calloc(8+1,sizeof(char));

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
           
           Serial.println("");
           Serial.println("Sign: " + signStr);

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
            char* szName = (char*)calloc(255,sizeof(char));
            tzxFile.getName(szName,254);
            
            //String szNameStr = sdm.getFileName(tzxFileName);
            String fileName = String(szName);

            Serial.println("");
            Serial.println("Name " + fileName);

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
        sizeDW = (256*sdm.readFileRange32(_mFile,offset+1,1,false)[0]) + sdm.readFileRange32(_mFile,offset,1,false)[0];  

        return sizeDW;    
    }

    int getBYTE(File32 mFile, int offset)
    {
        int sizeB = 0;
        sizeB = sdm.readFileRange32(_mFile,offset,1,false)[0];  

        return sizeB;      
    }

    int getID(File32 mFile, int offset)
    {
        // Obtenemos el ID
        int ID = 0;
        ID = getBYTE(_mFile,offset);

        return ID;      
    }

    byte* getBlock(File32 mFile, int offset, int size)
    {
        //Entonces recorremos el TZX. 
        // La primera cabecera SIEMPRE debe darse.
        byte* bloque = (byte*)calloc(size,sizeof(byte));


        // Obtenemos el bloque

        return bloque;
    }

    tTZXBlockDescriptor analyzeID16(File32 mFile, int currentOffset)
    {
        // Normal speed block
        
        tTZXBlockDescriptor blockDescriptor; 
        _myTZX.descriptor[block].ID = 16;
        _myTZX.descriptor[block].offset = currentOffset;

        // No contamos el ID. Entonces:

        // Obtenemos el "pause despues del bloque"
        // BYTE 0x00 y 0x01
        _myTZX.descriptor[block].pauseAfterThisBlock = getDWORD(mFile,currentOffset+1);
        // Obtenemos el "tamaño de los datos"
        // BYTE 0x02 y 0x03
        _myTZX.descriptor[block].lengthOfData = getDWORD(mFile,currentOffset+3);
        // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
        _myTZX.descriptor[block].offsetData = currentOffset + 4;
        // No contamos el ID
        // La cabecera tiene 4 bytes de parametros y N bytes de datos
        // pero para saber el total de bytes de datos hay que analizar el TAP
        _myTZX.descriptor[block].size = 4 + _myTZX.descriptor[block].lengthOfData;

        return blockDescriptor;
    }

    tTZXBlockDescriptor analyzeID48(File32 mFile, int currentOffset)
    {
        // Information block
        int sizeTextInformation = 0;

        tTZXBlockDescriptor blockDescriptor; 
        _myTZX.descriptor[block].ID = 48;
        _myTZX.descriptor[block].offset = currentOffset;

        sizeTextInformation = getByte(mFile,currenOffset+1);
        
        // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
        // el bloque comienza en el offset del ID y acaba en
        // offset[ID] + tamaño_bloque
        _myTZX.descriptor[block].size = sizeTextInformation + 1;

        return blockDescriptor;
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
          bool endWithErrors = false;

          currentOffset = startOffset;

          // Comenzamos a rastrear el fichero
          
          if (_myTZX.descriptor != NULL)
          {
              free(_myTZX.descriptor);
              _myTZX.descriptor = NULL;
          }

          // Reservamos espacio para el primer bloque y después iremos extendiendo
          // Vamos a ir usando realloc() para ir incrementando el espacio en memoria de manera dinámica.

          _myTZX.descriptor = (tTZXBlockDescriptor*)calloc(currentblock+1,sizeof(tTZXBlockDescriptor));              

          while (!endTZX)
          {
              // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque completo para el descriptor.
              currentID = getID(mFile, currentOffset);
              
              Serial.println("");
              Serial.println("TZX ID " + String(currentID));

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
                      _myTZX.descriptor[currentblock] = analyzeID16(mFile,currentoffset);

                      nextIDoffset = currentOffset + _myTZX.descriptor[currentblock].size + 1;
                      currentblock++;
                  }
                  else
                  {
                      Serial.println("");
                      Serial.println("Error: Not allocation memory for block ID 0x10");
                      endTZX = true;
                      endWithErrors = true;
                  }



                break;

                // ID 11- Turbo Speed Data Block
                case 17:
                break;

                // ID 12 - Pure Tone
                case 18:
                break;

                // ID 13 - Pulse sequence
                case 19:
                break;

                // ID 14 - Pure Data Block
                case 20:
                break;

                // ID 15 - Direct Recording
                case 21:
                break;

                // ID 18 - CSW Recording
                case 24:
                break;

                // ID 19 - Generalized Data Block
                case 25:
                break;

                // ID 30 - Information
                case 48:
                  // No hacemos nada solamente coger el siguiente offset
                  if (_myTZX.descriptor != NULL)
                  {
                      // Obtenemos la dirección del siguiente offset
                      _myTZX.descriptor[currentblock] = analyzeID48(mFile,currentoffset);

                      // Siguiente ID
                      nextIDoffset = currentOffset + _myTZX.descriptor[currentblock].size + 1;

                      currentblock++;
                  }
                  else
                  {
                      Serial.println("");
                      Serial.println("Error: Not allocation memory for block ID 0x10");
                      endTZX = true;
                      endWithErrors = true;
                  }                  
                  break;

              }

              Serial.println("");
              Serial.println("Next ID offset: " + String(nextIDoffset));
              _myTZX.descriptor = (tTZXBlockDescriptor*)realloc(currentblock,sizeof(tTZXBlockDescriptor)); 

          }
          // El siguiente bloque será
          // currentOffset + blockSize (que obtenemos del descriptor)

    }
   
    void proccess_tzx(File32 tzxFileName)
    {
          // Procesamos el fichero
          Serial.println("");
          Serial.println("Getting total blocks...");     

        if (isFileTZX(tzxFileName))
        {
            Serial.println();
            Serial.println("Is TZX file");

            getBlockDescriptor(_mFile,_sizeTZX);
        }      
    }

    
    
    public:

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
      
      Serial.println("");
      Serial.println("END PROCCESING TZX: ");
    
    }

    void initialize()
    {
        if (_myTZX.descriptor != NULL)
        {
          free(_myTZX.descriptor);
          free(_myTZX.name);
          _myTZX.descriptor = NULL;
          _myTZX.name = "\0";
          _myTZX.numBlocks = 0;
          _myTZX.size = 0;
        }          
    }

    void play()
    {}

    TZXproccesor(AudioKit kit)
    {
        // Constructor de la clase
        _myTZX.name = (char*)calloc(10+1,sizeof(char));
        _myTZX.name = &INITCHAR[0];
        _myTZX.numBlocks = 0;
        _myTZX.descriptor = NULL;
        _myTZX.size = 0;

        //zxp.set_ESP32kit(kit);      
    } 


};
