//
// SDManager para TAP files
//

#include "SdFat.h"

// **************** SD CARD

// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 1
#define SPI_SPEED SD_SCK_MHZ(4)

#if SD_FAT_TYPE == 0
  SdFat sdf;
  File dir;
  File file;
#elif SD_FAT_TYPE == 1
  SdFat32 sdf;
  File32 dir;
  File32 file;
#elif SD_FAT_TYPE == 2
  SdExFat sdf;
  ExFile dir;
  ExFile file;
#elif SD_FAT_TYPE == 3
  SdFs sdf;
  FsFile dir;
  FsFile file;
#else  // SD_FAT_TYPE
#error invalid SD_FAT_TYPE
#endif  // SD_FAT_TYPE

// ****************************************************************************
//
//                            Variables globales
//
// ****************************************************************************

int _FILE_LENGTH = 0;    //Tamaño del fichero en bytes

// ****************************************************************************
//
//                             Procedimientos 
//
// ****************************************************************************



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

bool IsCorrectHeader(byte* header)
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

int getBlockLen(byte* header)
{
    // Este procedimiento nos devuelve el tamaño definido un bloque de datos
    // analizando los 22 bytes de la cabecera pasada por valor.
    int blockLen = 0;
    if (IsCorrectHeader(header))
    {
        blockLen = (256*header[22]) + header[21];
    }
    return blockLen;
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


byte* getOneByteBlock(byte* bBlock, int bBlockNumber)
{
    // Este procedimiento devuelve un bloque de bytes, indicando
    // cual de los bloques queremos obtener

    int indexBlock = 0;
    int* offsetBlock = new int[50];
    int status = 0;
    int nStart = 0;
    int nEnd = 0;
    int blockSize = 0;
    byte* blockSelected;

    for (int n=0;n < _FILE_LENGTH;n++)
    {
        if (status == 0)
        { 
            // Estado de localizacion de inicio de cabecera
            if (bBlock[n]==19 && bBlock[n+1]==0 && bBlock[n+2]==0)
            {
                status = 1;
                nStart = n;
                n=3;
            }
        }
        else if (status == 1)
        {
            if (bBlock[n]==255)
            {
                //Hemos encontrado el final de cabecera. Cojo el tamaño del bloque de datos
                blockSize = (256*bBlock[n-1]) + bBlock[n-2];
                status = 2;
                n=23;
            }
        }
        else if (status == 2)
        {
            //Ahora buscamos otro 0x13, 0x00 que será el siguiente bloque
            if (n == _FILE_LENGTH - 1)
            {
                // Se ha llegado al final
                nEnd = n;
            }
            else
            {
                // Estado de localizacion de inicio de cabecera
                if (bBlock[n]==19 && bBlock[n+1]==0 && bBlock[n+2]==0)
                {
                    status = 0;
                    if (indexBlock == bBlockNumber)
                    {
                        // Este era el bloque buscado
                        blockSelected = new byte[nEnd - nStart + 1];
                        blockSelected = getBlockRange(bBlock,nStart,nEnd);
                        break;
                    }
                    else
                    {   
                        // Seguimos buscando
                        indexBlock++;
                    }
                }
            }
        }
    }

    return blockSelected;
}

int getTotalBlocks(byte* bBlock)
{
    // Este procedimiento devuelve el total de bloques que contiene el fichero
    int nblocks = 0;
    return nblocks;
}

byte* getHeaderBlock(byte* bBlock)
{
    // Este procedimiento obtiene la parte de la cabacera de un bloque de bytes
    // obtenido con getOneByteBlock
    byte* headerBlkTmp = new byte[21];
    
    for (int n=0;n<22;n++)
    {
        headerBlkTmp[n]=bBlock[n];
    }
    return headerBlkTmp;
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

byte* getDataBlock(byte* bBlock, int lenBlock)
{
    // Este procedimiento obtiene la parte de datos de un bloque de bytes
    byte* dataBlkTmp = new byte[lenBlock];
    return dataBlkTmp;
}

char* getNameOfProgramFromHeader(byte* bBlock)
{
    // Obtenemos el nombre del bloque cabecera
    char* prgName = new char[10];

    if (bBlock[0] == 19 && bBlock[1] == 0 && bBlock[2] == 0 && bBlock[3] == 0)
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
            prgName[n-4] = (char)bBlock[n];
        }
    }
    else
    {
        prgName = "";
    }

    // Pasamos la cadena de caracteres
    return prgName;

}

char* getNameOfProgram32(File32 mFile)
{
    char* prgName = new char[10];
    byte* blockTmp = new byte[21];

    // Cogemos 21 bytes la primera vez
    mFile.rewind();
    int16_t count = mFile.read(blockTmp,21);

    #ifdef LOG==3
      Serial.println("Block read: ");
      Serial.print(count,DEC);
      Serial.print(" bytes");
      Serial.println("");
      Serial.println("Header: ");
      for (int n=0;n<21;n++)
      {
          Serial.print(blockTmp[n],HEX);
          Serial.print(",");
      }
    #endif

    if (blockTmp[0] == 19 && blockTmp[1] == 0 && blockTmp[2] == 0 && blockTmp[3] == 0)
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
            prgName[n-4] = (char)blockTmp[n];
        }
    }
    else
    {
        prgName = "";
    }

    // Pasamos la cadena de caracteres
    return prgName;
}

char* getNameOfProgram(SdFile mFile)
{
    char* prgName = new char[10];
    byte* blockTmp = new byte[21];

    // Cogemos 21 bytes la primera vez
    mFile.rewind();
    int16_t count = mFile.read(blockTmp,21);

    #ifdef LOG==3
      Serial.println("Block red: ");
      Serial.print(count,DEC);
      Serial.println("");
      Serial.println("Header: ");
      for (int n=0;n<21;n++)
      {
          Serial.print(blockTmp[n]);
          Serial.print(",");
      }
    #endif

    if (blockTmp[0] == 19 && blockTmp[1] == 0 && blockTmp[2] == 0 && blockTmp[3] == 0)
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
            prgName[n-4] = (char)blockTmp[n];
        }
    }
    else
    {
        prgName = "";
    }

    // Pasamos la cadena de caracteres
    return prgName;
}


byte* readFile(SdFile mFile)
{
    byte* bufferFile;

    mFile.rewind();

    if (mFile) 
    {
        int rlen = mFile.available();
        _FILE_LENGTH = rlen;
        Serial.print("Len: ");
        Serial.print(String(rlen));

        //Redimensionamos el buffer al tamaño acordado del fichero
        bufferFile = resize(bufferFile,0,rlen);

        int i=0;
        while(i < rlen)
        {
            byte a = mFile.read();
            bufferFile[i] = a;
            i++;
        }
    } 
    else 
    {
        Serial.print(F("SD Card: error on opening file"));
    }

    return bufferFile;
}

byte* readFile32(File32 mFile)
{
    byte* bufferFile;

    mFile.rewind();

    if (mFile) 
    {
        int rlen = mFile.available();
        _FILE_LENGTH = rlen;

        Serial.print("Len: ");
        Serial.print(String(rlen));

        //Redimensionamos el buffer al tamaño acordado del fichero
        bufferFile = resize(bufferFile,0,rlen);

        int i=0;
        while(i < rlen)
        {
            byte a = mFile.read();
            bufferFile[i] = a;
            i++;
        }
    } 
    else 
    {
        Serial.print(F("SD Card: error on opening file"));
    }

    return bufferFile;

}

SdFile openFile(SdFile fFile, char *path) 
{ 
    if (!fFile.open(path, FILE_READ)) 
    {
        Serial.println("open failed");
    }
    else
    {
        Serial.println("open success");
    }

    return fFile;
}

File32 openFile32(File32 fFile, char *path) 
{ 
    if (!fFile.open(path, FILE_READ)) 
    {
        Serial.println("open failed");
    }
    else
    {
        Serial.println("open success");
    }

    return fFile;
}