/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TZXproccesor.h
    
    Creado por:
      Antonio Tamairón. 2023  
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de recursos para la gestión de ficheros .TZX del ZX Spectrum

    Version: 1.0

    Historico de versiones

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
        if (_mFile != NULL)
        {
           // Capturamos la cabecera
           byte* bBlock = (byte*)malloc(10+1);
           bBlock = _sdm.readFileRange32(_mFile,0,10,false);

           // Obtenemos la firma del TZX
           char* signTZXHeader = (char*)calloc(8+1,sizeof(char));
           signTZXHeader = "\0";

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
         if (_mFile != NULL)
         {
            // Analizamos el fichero. Para ello analizamos la cabecera
            char* szName = (char*)calloc(254+1,sizeof(char));
            szName = "\0";

            Serial.println();
            Serial.println();
            Serial.println("Preparing calloc memory");

            // Capturamos el nombre del fichero en szName
            _mFile.getName(szName,254);
            String szNameStr = String(szName);

            Serial.println();
            Serial.println();
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

  TZXproccesor()
  {
      // Constructor de la clase
      if ( _myTZX.name != "\0")
      {
          free( _myTZX.name);
      }
      _myTZX.name = (char*)calloc(10+1,sizeof(char));
      _myTZX.name != "\0";
      _myTZX.numBlocks = 0;
      _myTZX.descriptor = NULL;
      _myTZX.size = 0;
  } 
};
