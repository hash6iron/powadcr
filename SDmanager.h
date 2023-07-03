/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: SDmanager.h
    
    Creado por:
      Antonio Tamairón. 2023  
      @hash6iron / https://powagames.itch.io/


    Colaboradores en el proyecto:
      - Fernando Mosquera
      - Guillermo
      - Mario J
      - Pedro Pelaez

    
    Descripción:
    Funciones para la gestión de ficheros y lectura de bloques de bytes desde la SD

    Version: 0.1

    Historico de versiones
    v.0.1 - Version de pruebas. En desarrollo
    

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "SdFat.h"

SdFat32 sdf;
File32 dir;
File32 file;

// ****************************************************************************
//
//                             Procedimientos 
//
// ****************************************************************************


File32 openFile32(File32 fFile, char *path) 
{ 
    Serial.println();
    Serial.println("Open file:");
    Serial.println(path);

    if (fFile != NULL)
    {
        fFile.close();
        Serial.println("closing file");
    }

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

byte* readFile32(File32 mFile)
{
    byte* bufferFile = NULL;

    mFile.rewind();

    if (mFile) 
    {
        int rlen = mFile.available();
        FILE_LENGTH = rlen;

        //Serial.print("Len: ");
        //Serial.print(String(rlen));

        //Redimensionamos el buffer al tamaño acordado del fichero
        bufferFile = (byte*)malloc(rlen);

        int i=0;
        while(i < rlen)
        {
            bufferFile[i] = (byte)mFile.read();
            i++;
        }
    } 
    else 
    {
        Serial.print(F("SD Card: error opening file. Please check SD frequency."));
    }

    return bufferFile;
}

byte* readFileRange32(File32 mFile, int startByte, int size, bool logOn)
{
    //Redimensionamos el buffer al tamaño acordado del rango
    byte* bufferFile = (byte*)malloc(size);

    // Ponemos a cero el puntero de lectura del fichero
    mFile.rewind();
    // Nos posicionamos en el byte definido
    mFile.seek(startByte);

    //Serial.println("***** readFileRange32 *****");
    if (logOn)
    {
        //Serial.println("   + Offset: " + String(startByte));
        //Serial.println("   + Size: " + String(size));
        //Serial.println("");
    }

    // Almacenamos el tamaño del bloque, para información
    //LAST_SIZE = size;
    // Actualizamos HMI
    //updateInformationMainPage();

    if (mFile) 
    {
        int rlen = mFile.available();
        FILE_LENGTH = rlen;

        int i=0;

        while(i < size)
        {
            bufferFile[i] = (byte)mFile.read();;
            i++;
        }
    } 
    else 
    {
        Serial.print(F("SD Card: error opening file. Please check SD frequency."));
    }

    return bufferFile;
}
