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

// TYPE block
#define HPRGM 0                     //Cabecera PROGRAM
#define HCODE 1                     //Cabecera BYTE  
#define HSCRN 7                     //Cabecera BYTE SCREEN$
#define HARRAYNUM 5                 //Cabecera ARRAY numerico
#define HARRAYCHR 6                 //Cabecera ARRAY char
#define PRGM 2                      //Bloque de datos BASIC
#define SCRN 3                      //Bloque de datos Screen$
#define CODE 4                      //Bloque de datps CM (BYTE)



class TZXproccesor
{

    private:

    // Procesador de audio
    ZXProccesor _zxp;
    
    // Gestor de SD
    SDmanager _sdm;
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

     bool isHeaderTZX()
     {
        if (_mFile != 0)
        {

           Serial.println("");
           Serial.println("");
           Serial.println("Begin isHeaderTZX");

           // Capturamos la cabecera
           // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
           // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
           /*
            Guru Meditation Error: Core  1 panic'ed (LoadStoreAlignment). Exception was unhandled.
            04:08:11.907 -> 
            04:08:11.907 -> Core  1 register dump:
            04:08:11.907 -> PC      : 0x4008b794  PS      : 0x00060a33  A0      : 0x800898ed  A1      : 0x3ffb1bc0  
            04:08:11.907 -> A2      : 0x6f5320bf  A3      : 0xb33fffff  A4      : 0x0000cdcd  A5      : 0x00060a23  
            04:08:11.907 -> A6      : 0x00060a20  A7      : 0x0000abab  A8      : 0x0000abab  A9      : 0xffffffff  
            04:08:11.907 -> A10     : 0x00000003  A11     : 0x00060a23  A12     : 0x00060a20  A13     : 0x00000001  
            04:08:11.907 -> A14     : 0x2fd320bf  A15     : 0x003fffff  SAR     : 0x0000001a  EXCCAUSE: 0x00000009  
            04:08:11.907 -> EXCVADDR: 0x6f5320bf  LBEG    : 0x4008663c  LEND    : 0x40086647  LCOUNT  : 0xffffffff             
            */
           byte* bBlock = (byte*)calloc(11+1,sizeof(byte));
           bBlock = NULL;
           bBlock = _sdm.readFileRange32(_mFile,0,10,false);

           // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR
           // ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR

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
           for (int n=0;n<7;n++)
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

     bool isFileTZX()
     {
         if (_mFile != 0)
         {

            // Capturamos el nombre del fichero en szName
            String szNameStr = _sdm.getFileName(_mFile);

            Serial.println("");
            Serial.println("");
            Serial.println("Name " + szNameStr);

            if (szNameStr != "")
            {
                  //String fileExtension = szNameStr.substring(szNameStr.length()-3);
                  szNameStr.toUpperCase();
                  
                  if (szNameStr.indexOf(".TZX") != -1)
                  {
                      // La extensión es TZX pero ahora vamos a comprobar si
                      // internamente también lo es
                      if (isHeaderTZX())
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

   
    void proccess_tzx()
    {}

    
    
    public:

    void set_SdFat32(SdFat32 sdf32)
    {
        _sdf32 = sdf32;
    }

    void set_file(File32 mFile, int sizeTZX)
    {
      // Pasamos los parametros a la clase
      _mFile = mFile;
      _sizeTZX = sizeTZX;
    }  

    void getInfoFileTZX(char* path) 
    {
    
      LAST_MESSAGE = "Analyzing file";
      _hmi.updateInformationMainPage();
    
      // Abrimos el fichero
      _mFile = _sdm.openFile32(_mFile, path);
      // Obtenemos su tamaño total
      _rlen = _mFile.available();

      Serial.println();
      Serial.println();
      Serial.println("Size: " + String(_rlen));

      // creamos un objeto TZXproccesor
      //set_file(_mFile, _rlen);

      Serial.println();
      Serial.println();
      Serial.println("Set file");      

      if (isFileTZX())
      {
          Serial.println();
          Serial.println();
          Serial.println("Is TZX file");
      }

      proccess_tzx();
      
      Serial.println("");
      Serial.println("");
      Serial.println("END PROCCESING TZX: ");
    
      // if (_myTAP.descriptor != NULL)
      // {
      //     // Entregamos información por consola
      //     PROGRAM_NAME = get_tap_name();
      //     TOTAL_BLOCKS = get_tap_numBlocks();
      //     LAST_NAME = "..";
    
      //     Serial.println("");
      //     Serial.println("");
      //     Serial.println("PROGRAM_NAME: " + PROGRAM_NAME);
      //     Serial.println("TOTAL_BLOCKS: " + String(TOTAL_BLOCKS));
    
      //     // Pasamos el descriptor           
      //     _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

      //     _hmi.updateInformationMainPage();
      // }
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

      _zxp.set_ESP32kit(kit);      
  } 
};
