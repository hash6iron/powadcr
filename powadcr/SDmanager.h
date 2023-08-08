/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: SDmanager.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Funciones para la gestión de ficheros y lectura de bloques de bytes desde la SD

    Version: 0.2

    Historico de versiones
    v.0.1 - Version de pruebas. En desarrollo
    v.0.2 - Se convierte en una clase

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

class SDmanager
{

    public:

    SdFat32 sdf;
    File32 dir;
    File32 file;

    File32 openFile32(File32 fFile, char *path) 
    { 
        SerialHW.println();
        SerialHW.println("Open file:");
        SerialHW.println(path);
    
        if (fFile != 0)
        {
            fFile.close();
            SerialHW.println("closing file");
        }
    
        if (!fFile.open(path, FILE_READ)) 
        {
            SerialHW.println("open failed");
        }
        else
        {
            SerialHW.println("open success");
        }
    
        return fFile;
    }

    void closeFile32(File32 fFile)
    {
        if (fFile != 0)
        {
            fFile.close();
            SerialHW.println("closing file");
        } 
    }
    
    byte* readFile32(File32 mFile)
    {
        byte* bufferFile = NULL;
    
        mFile.rewind();
    
        if (mFile) 
        {
            int rlen = mFile.available();
            FILE_LENGTH = rlen;
    
            //SerialHW.print("Len: ");
            //SerialHW.print(String(rlen));
    
            //Redimensionamos el buffer al tamaño acordado del fichero
            bufferFile = (byte*)calloc(rlen+1,sizeof(byte));
            mFile.read(bufferFile,rlen);
            
            // int i=0;
            // while(i < rlen)
            // {
            //     bufferFile[i] = (byte)mFile.read();
            //     i++;
            // }
        } 
        else 
        {
            SerialHW.print(F("SD Card: error opening file. Please check SD frequency."));
        }
    
        return bufferFile;
    }
    
    byte* readFileRange32(File32 mFile, int startByte, int size, bool logOn)
    {
        //Redimensionamos el buffer al tamaño acordado del rango
        byte* bufferFile = NULL;
        bufferFile = (byte*)calloc(size+1,sizeof(byte));
    
        // Ponemos a cero el puntero de lectura del fichero
        mFile.rewind();
        // Nos posicionamos en el byte definido
        mFile.seek(startByte);
    
        //SerialHW.println("***** readFileRange32 *****");
        if (logOn)
        {
            SerialHW.println("   + Offset: " + String(startByte));
            SerialHW.println("   + Size: " + String(size));
            SerialHW.println("");
        }
    
        // Almacenamos el tamaño del bloque, para información
        //LAST_SIZE = size;
        // Actualizamos HMI
        //updateInformationMainPage();
    
        if (mFile) 
        {
            int rlen = mFile.available();
            FILE_LENGTH = rlen;

            #if LOG > 3
              SerialHW.println("");
              SerialHW.println("LENGTH: " + String(rlen));            
            #endif

            if (rlen != 0)
            {
                mFile.read(bufferFile,size);
                
                #if LOG > 3
                  SerialHW.println(" - Block red");
                #endif
            }

        } 
        else 
        {
            SerialHW.print(F("SD Card: error opening file. Please check SD frequency."));
        }
    
        return bufferFile;
    }

    String getFileName(File32 f)
    {
          char* szName = (char*)calloc(255,sizeof(char));
          szName = &INITCHAR[0];
          f.getName(szName,255);

          return String(szName);
    }
    
    // Constructor de la clase
    SDmanager()
    {}
};
