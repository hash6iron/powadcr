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

     bool isHeaderTZX(File32 tzxFileName)
     {
        if (tzxFileName != 0)
        {

           Serial.println("");
           Serial.println("");
           Serial.println("Begin isHeaderTZX");

           // Capturamos la cabecera

           //byte* bBlock = NULL; 
           byte* bBlock = (byte*)calloc(10+1,sizeof(byte));
           bBlock = sdm.readFileRange32(tzxFileName,0,10,true);

           Serial.println("");
           Serial.println("");
           Serial.println("Got bBlock");

           // Obtenemos la firma del TZX
           char* signTZXHeader = (char*)calloc(8+1,sizeof(char));
           signTZXHeader = &INITCHAR[0];

           Serial.println("");
           Serial.println("");
           Serial.println("Initialized signTZX Header");

           // Analizamos la cabecera
           // Extraemos el nombre del programa
           for (int n=0;n<6;n++)
           {   
               signTZXHeader[n] = (char)bBlock[n];
           }
           
           String sign = String(signTZXHeader);

           Serial.println("");
           Serial.println("");
           Serial.println("sign in TZX file detected.");
           Serial.println(sign);

           if (sign == "ZXTape!")
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

     bool isFileTZX(File32 tzxFileName)
     {
         if (tzxFileName != 0)
         {

            // Capturamos el nombre del fichero en szName
            char* szName = (char*)calloc(255,sizeof(char));
            tzxFileName.getName(szName,254);
            //String szNameStr = sdm.getFileName(tzxFileName);
            String fileName = String(szName);

            Serial.println("");
            Serial.println("");
            Serial.println("Name " + fileName);

            if (fileName != "")
            {
                  //String fileExtension = szNameStr.substring(szNameStr.length()-3);
                  fileName.toUpperCase();
                  
                  if (fileName.indexOf(".TZX") != -1)
                  {
                      // La extensión es TZX pero ahora vamos a comprobar si
                      // internamente también lo es
                      if (isHeaderTZX(tzxFileName))
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
         else
         {
            return false;
         }
  
     }

   
    void proccess_tzx(File32 tzxFileName)
    {
          // Procesamos el fichero
          Serial.println("");
          Serial.println("Getting total blocks...");     

        if (isFileTZX(tzxFileName))
        {
            Serial.println();
            Serial.println();
            Serial.println("Is TZX file");
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

      Serial.println();
      Serial.println();
      Serial.println("Size: " + String(_rlen));

      // creamos un objeto TZXproccesor
      //set_file(tzxFile, _rlen);
      proccess_tzx(tzxFile);
      
      Serial.println("");
      Serial.println("");
      Serial.println("END PROCCESING TZX: ");
    
    }

    void initialize()
    {
        // if (_myTAP.descriptor != NULL)
        // {
        //   free(_myTAP.descriptor);
        //   free(_myTAP.name);
        //   _myTAP.descriptor = NULL;
        //   _myTAP.name = "\0";
        //   _myTAP.numBlocks = 0;
        //   _myTAP.size = 0;
        // }          
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
