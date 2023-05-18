#include "SdFat.h"


class TAPproccesor
{
    private:
      struct tBlock
      {
          int index;
          int offset = 0;
          byte* header;
          byte* data;
      };

      struct tTAP
      {
          char* nameTAP;
          int size;
          int numBlocks;
          tBlock* container;
      };

      SdFat fFile;
      SdFat32 fFile32;

      // Definicion de un TAP
      tTAP myTAP;

      byte* resize(byte* buffer, size_t items, size_t newCapacity)
      {
        size_t count = items;
        byte* newBuffer = new byte[newCapacity];
        memmove(newBuffer, buffer, count * sizeof(byte));
        buffer = newBuffer;

        return buffer;
      }

      byte calculateChecksum(byte* bBlock)
      {
          // Calculamos el checksum de un bloque de bytes
          byte newChk = 0;

          #ifdef LOG==3
            Serial.println("");
            Serial.println("Block len: ");
            Serial.print(sizeof(bBlock));
          #endif

          // Calculamos el checksum
          int blockLen = sizeof(bBlock);
          for (int n=0;n<blockLen;n++)
          {
              newChk = newChk ^ bBlock[n];
          }

          return newChk;
      }

      bool isCorrectHeader(byte* header)
      {
          // Verifica que es una cabecera
          bool isHeader = false;
          byte checksum = 0;
          byte calcBlockChk = 0;

          if (header[0]==19 && header[1]==0 && header[2]==0)
          {
              checksum = header[20];
              calcBlockChk = calculateChecksum(header);

              if (checksum == calcBlockChk)
              {
                  isHeader = true;
              }
          }

          return isHeader;
      }

      bool isProgramHeader(byte* header)
      {
          // Verifica que es una cabecera
          bool isHeaderPrg = false;
          byte checksum = 0;
          byte calcBlockChk = 0;

          if (isCorrectHeader(header))
          {
              if (header[3]==0)
              {
                  checksum = header[20];
                  calcBlockChk = calculateChecksum(header);

                  if (checksum == calcBlockChk)
                  {
                      isHeaderPrg = true;
                  }
              }
              else
              {isHeaderPrg = false;}
          }

          return isHeaderPrg;
      }

      int getBlockLen(byte* header)
      {
          // Este procedimiento nos devuelve el tamaño definido un bloque de datos
          // analizando los 22 bytes de la cabecera pasada por valor.
          int blockLen = 0;
          if (isCorrectHeader(header))
          {
              blockLen = (256*header[22]) + header[21];
          }
          return blockLen;
      }


      byte* getHeaderBlockForOut(byte* bBlock)
      {
          // Este procedimiento obtiene la parte de la cabacera de un bloque de bytes
          // obtenido con getOneByteBlock, pero listo para sacar por la salida de audio.
          byte* headerBlkTmp = new byte[19];
          
          int i=0;
          // NOTA: Se elimina de la cabecera el 0x13,0x00, para la reproducción (los dos primeros bytes)
          for (int n=2;n<21;n++)
          {
              headerBlkTmp[i]=bBlock[n];
              i++;
          }
          return headerBlkTmp;  
      }
      
      byte* getHeaderBlock(byte* bBlock, int offset)
      {
          // Este procedimiento obtiene la parte de la cabacera de un bloque de bytes
          // obtenido con getOneByteBlock
          byte* headerBlkTmp = new byte[21];
          
          for (int n=offset;n<(offset + 22);n++)
          {
              headerBlkTmp[n]=bBlock[n];
          }
          return headerBlkTmp;
      }

      char* getNameFromHeader(byte* header)
      {
          // Obtenemos el nombre del bloque cabecera
          char* prgName = new char[10];

          if (isCorrectHeader(header))
          {
              // Es una cabecera PROGRAM 
              // el nombre está en los bytes del 4 al 13 (empezando en 0)
              #ifdef LOG==3
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
              prgName = "";
          }

          // Pasamos la cadena de caracteres
          return prgName;

      }

      byte* getBlockRange(byte* bBlock, int byteStart, int byteEnd)
      {

          // Extraemos un bloque de bytes indicando el byte inicio y final
          byte* blockExtracted = new byte[(byteEnd - byteStart)+1];

          int i=0;
          for (int n=byteStart;n<(byteStart + (byteEnd - byteStart)+1);n++)
          {
              blockExtracted[i] = bBlock[n];
          }

          return blockExtracted;
      }

      int getTotalBlocks(byte* bBlock)
      {
          // Este procedimiento devuelve el total de bloques que contiene el fichero
          int nblocks = 0;

          // Para ello buscamos la secuencia "0x13 0x00 0x00"

          return nblocks;
      }

      void analyzeTAPfile(byte* fileTAP, int sizeTAP)
      {
          // Analizamos el TAP
          //1. Obtenemos el numero de bloques
          //2. Extraemos cabeceras y bloques
          //3. Finalizamos
          int offset = 0;
          int offsetInitial = 0;
          int bLen = 0;
          byte* header;
          byte* data;

          for (int n=0;n<sizeTAP;n++)
          {
              header = new byte[21];
              header = getHeaderBlock(fileTAP, offset);
              
              if (isCorrectHeader(header))
              {
                // Cogemos la longitud del bloque de datos
                // esta información nos dará el offset desde el flag FF
                // para la siguiente cabecera
                bLen = getBlockLen(header);
                
                // aumentamos el total de bloques
                myTAP.numBlocks++;
                
                // Si es el primer bloque
                if (offset == 0)
                {
                    // Si es la primera cabecera y es PROGRAM
                    if (isProgramHeader(header))
                    {
                        myTAP.nameTAP = getNameFromHeader(header);
                    }
                }

                // Cogemos ahora el bloque de datos.
                // la cabecera en un bloque TAP completo acaba con CHECKSUM 
                // Después se indica el TAMAÑO DEL BLOQUE DE DATOS + 0xFF
                // el bloque de datos empieza después del 0xFF
                // entonces, son 22 bytes, el 23 ya es el bloque
                
                offsetInitial = offset;
                offset = offset + 23;

                // cogemos el bloque de datos
                data = new byte[bLen];
                data = getBlockRange(fileTAP,offset,offset+bLen);

                // offset para la nueva cabecera
                offset = offset + bLen + 1;

                // Almacenamos
                myTAP.container[myTAP.numBlocks-1].header = header;
                myTAP.container[myTAP.numBlocks-1].data = data;
                myTAP.container[myTAP.numBlocks-1].offset = offsetInitial;
                myTAP.container[myTAP.numBlocks-1].index = myTAP.numBlocks;
              }
              
          }
      }

    public:


      TAPproccesor(byte* fileTAP, int sizeTAP)
      {
          // El constructor se crea a partir de pasarle el TAP completo a la clase TAP proccesor.
          // entonces se analiza y se construye el buffer, etc para poder manejarlo.
          int totalBlocks = getTotalBlocks(fileTAP);
          myTAP.container = new tBlock[totalBlocks];

          analyzeTAPfile(fileTAP, sizeTAP);
      }
};