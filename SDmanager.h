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