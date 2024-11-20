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

    bool createEmptyFile32(char* path)
    {
        File32 fFile;
        
        if (fFile.open(path, O_CREAT|O_WRONLY|O_TRUNC))
        {
            fFile.close();
            return true;
        }
        else
        {
            return false;
        }
    }

    bool createFile32(char* path)
    {
        File32 fFile;
        
        if (!fFile.exists(path))
        {
            if (fFile.open(path, O_WRITE | O_CREAT))
            {
                fFile.close();
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

    void deleteFile32(char* path)
    {
        File32 fFile;
        
        if (!fFile.exists(path))
        {
            fFile.remove();
        }
    }

    File32 openDir(char* path)
    {
        File32 dfFile;
        
        if (!dfFile.open(path, O_RDWR)) 
        {
            #ifdef DEBUGMODE
                log("open DIR failed");
            #endif
        }
        else
        {
            #ifdef DEBUGMODE
                log("open DIR success");
            #endif
        }

        return dfFile;
    }

    File32 openFile32(char* path)
    {
        File32 fFile;
        
        if (!fFile.open(path, O_RDWR)) 
        {
            #ifdef DEBUGMODE
                log("open failed");
            #endif
        }
        else
        {
            #ifdef DEBUGMODE
                log("open success");
            #endif
        }

        return fFile;
    }

    File32 openFile32(File32 fFile, char *path) 
    { 
        //SerialHW.println();
        //SerialHW.println("Open file:");
        //SerialHW.println(path);
    
        if (fFile != 0)
        {
            fFile.close();
        }
    
        if (!fFile.open(path, FILE_READ)) 
        {
            #ifdef DEBUGMODE
                log("open failed");
            #endif
        }
        else
        {
            #ifdef DEBUGMODE
                log("open success");
            #endif
        }
    
        return fFile;
    }

    

    void closeFile32(File32 fFile)
    {
        if (fFile != 0)
        {
            fFile.close();
            #ifdef DEBUGMODE
                log("closing file");
            #endif
        } 
    }
    
    uint8_t* readFile32(File32 mFile)
    {
        uint8_t* bufferFile = NULL;
    
        mFile.rewind();
    
        if (mFile) 
        {
            int rlen = mFile.available();
            FILE_LENGTH = rlen;
    
            //Redimensionamos el buffer al tamaño acordado del fichero
            bufferFile = (uint8_t*)ps_calloc(rlen+1,sizeof(uint8_t));
            mFile.read(bufferFile,rlen);
        } 
    
        return bufferFile;
    }
    
    uint8_t* readFileRange32(File32 mFile, int startByte, int size, bool logOn)
    {
        //Redimensionamos el buffer al tamaño acordado del rango
        uint8_t* bufferFile = (uint8_t*)ps_calloc(size+1,sizeof(uint8_t));
            
        // Ponemos a cero el puntero de lectura del fichero
        mFile.rewind();
        // Nos posicionamos en el uint8_t definido
        mFile.seek(startByte);
    
        ////SerialHW.println("***** readFileRange32 *****");

        #ifdef DEBUGMODE
            // log("..SDM - Info");
            // SerialHW.print("   + Offset: ");
            // SerialHW.print(startByte,HEX);
            // SerialHW.print(" | Size: ");
            // SerialHW.print(String(size));
        #endif

        // Almacenamos el tamaño del bloque, para información
        //LAST_SIZE = size;
        // Actualizamos HMI
        //updateInformationMainPage();
    
        if (mFile) 
        {
            int rlen = mFile.available();
            FILE_LENGTH = rlen;

            #if LOG > 3
              //SerialHW.println("");
              //SerialHW.println("LENGTH: " + String(rlen));            
            #endif

            if (rlen != 0)
            {
                mFile.read(bufferFile,size);
                
                #if LOG > 3
                  //SerialHW.println(" - Block red");
                #endif
            }

        } 
        else 
        {
            //SerialHW.print(F("SD Card: error opening file. Please check SD frequency."));
        }
    
        return bufferFile;
    }

    // void createCfgFile(char* path)
    // {
    //     File32 fFile;
        
    //     if (!fFile.create(path, O_RDWR)) 
    //     {
    //         log("create failed");
    //         return false;
    //     }
    //     else
    //     {
    //         log("create success");
    //         return true;
    //     }
    // }

    void writeParamCfg(File32 mFile, String param, String value)
    {
        // Escribimos el parámetro en el fichero de configuración
        if (mFile)
        {
            mFile.println("<" + param + ">");
            mFile.print(value);
            mFile.print("</" + param + ">");
        }
        else
        {
            #ifdef DEBUGMODE
                log("Error writing cgf");
            #endif
        }
    }


    String getValueOfParam(String line, String param)
    {
        #ifdef DEBUGMODE
            logln("Cfg. line: " + line);
            log(" - Param: " + param);
        #endif

        int firstClosingBracket = line.indexOf('>');

        if( firstClosingBracket != -1)
        {
            // int paramLen = param.length();
            int firstOpeningEndLine = line.indexOf("</",firstClosingBracket + 1);  
            
            if (firstOpeningEndLine != -1)
            {
                String res = line.substring(firstClosingBracket+1, firstOpeningEndLine);
                #ifdef DEBUGMODE
                    log(" / Value = " + res);
                #endif
                return res;
            }
        }
        
        return "null";
    }

    tConfig* readAllParamCfg(File32 mFile, int maxParameters)
    {
        tConfig* cfgData = new tConfig[maxParameters];

        // Vamos a ir linea a linea obteniendo la información de cada parámetro.
        char line[256];
        //
        int n;
        int i=0;

        // Vemos si el fichero ya está abierto
        if (mFile.isOpen())
        {
            // read lines from the file
            while ((n = mFile.fgets(line, sizeof(line))) > 0) 
            {

                // remove '\n'
                line[n-1] = 0;
                String strRes = "";

                strRes = line;
                #ifdef DEBUGMODE
                    logln("[" + String(i) + "]" + strRes);
                #endif

                if (i<=maxParameters)
                {
                    cfgData[i].cfgLine = strRes;
                    cfgData[i].enable = true;
                }
                else
                {
                    log("Out of space for cfg parameters. Max. " + String(maxParameters));
                }

                i++;

            }
        }

        return cfgData;
    }

    // String getFileName(File32 f)
    // {
    //       char* szName[255] = (char*)ps_calloc(255,sizeof(char));
    //       szName = &INITCHAR[0];
    //       f.getName(szName,255);
    //       String name = String(szName);
    //       free(szName);

    //       return name;
    // }
    
    // Constructor de la clase
    SDmanager()
    {}
};
