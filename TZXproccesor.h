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

#include "SdFat.h"

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

      SdFat fFile;
      SdFat32 fFile32;

      // Definicion de un TAP
      tTZX _myTZX;
      File32 _mFile;
      int _sizeTZX;

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
            // Capturamos la cabecera
            byte* bBlock = (byte*)malloc(10+1);
            bBlock = readFileRange32(tzxFileName,0,10,false);

            // Obtenemos la firma del TZX
            char* nameTZXHeader = (char*)calloc(8+1,sizeof(char));

            // Analizamos la cabecera
            // Extraemos el nombre del programa
            for (int n=0;n<7;n++)
            {   
                nameTZXHeader[n] = (char)bBlock[n];
            }
            
            String sign = String(nameTZXHeader);

            Serial.println("");
            Serial.println("");
            Serial.println("TZX file detected.");
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

      bool isFileTZX(File32 tzxFileName)
      {
          // Analizamos el fichero. Para ello analizamos la cabecera
          char* szName = (char*)calloc(255,sizeof(char));
          String fileName = tzxFileName.getName(szName,sizeof(char));
          
          if (fileName != "")
          {
               String fileExtension = (fileName.substring(fileName.length()-3)).toUpperCase()
               if (fileExtension = "TZX")
               {
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

      public:
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
}