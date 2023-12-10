/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TAPproccesor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de recursos para la gestión de ficheros .TAP de ZX Spectrum

    Version: 0.2

    Historico de versiones
    ----------------------
    v.0.1 - Version inicial
    v.0.2 - Se han hecho modificaciones relativas al resto de clases para HMI y SDmanager. Se ha incluido la reproducción y analisis de bloques que estaba antes en el powadcr.ino
    v.0.3 - New update to develp branch

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

// TYPE block
#define HPRGM 0                     //Cabecera PROGRAM
#define HCODE 1                     //Cabecera BYTE  
#define HSCRN 7                     //Cabecera BYTE SCREEN$
#define HARRAYNUM 5                 //Cabecera ARRAY numerico
#define HARRAYCHR 6                 //Cabecera ARRAY char
#define PRGM 2                      //Bloque de datos BASIC
#define SCRN 3                      //Bloque de datos Screen$
#define CODE 4                      //Bloque de datps CM (BYTE)

class TAPproccesor
{

    public:

        // Estructura del descriptor de bloques
        struct tBlockDescriptor 
        {
            bool corrupted = false;
            int offset = 0;
            int size = 0;
            int chk = 0;
            char name[11];
            bool nameDetected = false;
            bool header = false;
            bool screen = false;
            int type = 0;
            char typeName[11];
        };    

        // Estructura tipo TAP
        struct tTAP 
        {
            char name[11];                                  // Nombre del TAP
            int size = 0;                                   // Tamaño
            int numBlocks = 0;                              // Numero de bloques
            tBlockDescriptor* descriptor;            // Descriptor
        };


    private:
        // Procesador de audio output
        ZXProccesor _zxp;     

        // Definicion de un TAP
        tTAP _myTAP;

        // Gestor de SD
        HMI _hmi;

        SdFat32 _sdf32;
        File32 _mFile;
        int _sizeTAP;
        int _rlen;

        int CURRENT_LOADING_BLOCK = 0;

        // Creamos el contenedor de bloques. Descriptor de bloques
        //tBlockDescriptor* bDscr = new tBlockDescriptor[255];
        // Gestión de bloques
        int _startBlock = 0;
        int _lastStartBlock = 0;

        bool isHeaderTAP(File32 tapFileName)
        {
            bool rtn = false;

            if (tapFileName != 0)
            {
                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("Begin isHeaderTAP");

                // La cabecera son 19 bytes
                uint8_t* bBlock = (uint8_t*)(malloc((20 * sizeof(uint8_t))));
                bBlock = sdm.readFileRange32(tapFileName,0,19,true);

                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("Got bBlock");

                // Obtenemos la firma del TAP
                char signTZXHeader[4];


                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("Initialized signTAP Header");

                // Analizamos la cabecera
                // Extraemos el nombre del programa
                for (int n=0;n<3;n++)
                {   
                    signTZXHeader[n] = (uint8_t)bBlock[n];
                    
                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println((int)signTZXHeader[n]);                  
                }

                if (signTZXHeader[0] == 19 && signTZXHeader[1] == 0 && signTZXHeader[2] == 0)
                {
                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println("is TAP ok");
                    rtn = true;
                }
                else
                {
                    rtn = false;
                }

                // Liberamos memoria
                free(bBlock);
            }
            else
            { 
            rtn = false;
            }

            log("isHeaderTAP OK");
            return rtn;
        }

        bool isFileTAP(File32 tapFileName)
        {
            bool rtn = false;
            char* szName = new char[254 + 1];

            tapFileName.getName(szName,254);
            String fileName = static_cast<String>(szName);

            if (fileName != "")
            {
                fileName.toUpperCase();
                if (fileName.indexOf("TAP") != -1)
                {
                    if (isHeaderTAP(tapFileName))
                    {
                    rtn = true;
                    }
                    else
                    {
                    rtn = false;
                    }
                }
                else
                {
                    rtn = false;
                }
            }
            else
            {
                rtn = false;
            }

            log("IsFileTAP OK");
            return rtn;
        }

        uint8_t calculateChecksum(uint8_t* bBlock, int startByte, int numBytes)
        {
            // Calculamos el checksum de un bloque de bytes
            uint8_t newChk = 0;

            #if LOG>3
            // SerialHW.println("");
            // SerialHW.println("Block len: ");
            // SerialHW.print(sizeof(bBlock)/sizeof(uint8_t*));
            #endif

            // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
            
            for (int n=startByte;n<(startByte+numBytes);n++)
            {
                newChk = newChk ^ bBlock[n];
            }

            #if LOG>3
            // SerialHW.println("");
            // SerialHW.println("Checksum: " + String(newChk));
            #endif

            return newChk;
        }


        bool isCorrectHeader(uint8_t* header, int startByte)
        {
            // Verifica que es una cabecera
            bool isHeader = false;
            uint8_t checksum = 0;
            uint8_t calcBlockChk = 0;

            if (header[startByte]==19 && header[startByte+1]==0 && header[startByte+2]==0)
            {
                checksum = header[startByte+20];
                calcBlockChk = calculateChecksum(header,startByte,18);

                if (checksum == calcBlockChk)
                {
                    isHeader = true;
                }
            }

            return isHeader;
        }

        bool isProgramHeader(uint8_t* header, int startByte)
        {
            // Verifica que es una cabecera
            //bool isHeaderPrg = false;
            uint8_t checksum = 0;
            uint8_t calcBlockChk = 0;

            if (isCorrectHeader(header, startByte))
            {
                if (header[startByte+3]==0)
                {
                    checksum = header[startByte+20];
                    calcBlockChk = calculateChecksum(header,startByte,18);

                    if (checksum == calcBlockChk)
                    {
                        //isHeaderPrg = true;
                        return true;
                    }
                }
                else
                //{isHeaderPrg = false;}
                {
                return false;
                }
            }

            //return isHeaderPrg;
            return false;
        }

        int getBlockLen(uint8_t* header, int startByte)
        {
            // Este procedimiento nos devuelve el tamaño definido un bloque de datos
            // analizando los 22 bytes de la cabecera pasada por valor.
            int blockLen = 0;
            if (isCorrectHeader(header,startByte))
            {
                blockLen = (256*header[startByte+22]) + header[startByte+21];
            }
            return blockLen;
        }

        char* getTypeTAPBlock(int nBlock)
        {
            String typeName;
            char* rtnStr = new char[20 + 1];

            switch(nBlock)
            {
                case 0:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    typeName = "PROGRAM.HEAD";
                    break;

                case 1:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    typeName = "BYTE.DATA";
                    break;

                case 7:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    typeName = "SCREEN.HEAD";
                    break;

                case 2:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    typeName = "BASIC BLOCK";
                    break;

                case 3:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    typeName = "SCREEN.DATA";
                    break;

                case 4:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    if (LAST_SIZE != 6914)
                    {
                        typeName = "BYTE.DATA";
                    }
                    else
                    {
                        typeName = "SCREEN.DATA";
                    }
                    break;
            }        

            // Transformamos en char*
            strcpy(rtnStr, typeName.c_str());
            return rtnStr;
        }
        
        char* getNameFromHeader(uint8_t* header, int startByte)
        {
            // Obtenemos el nombre del bloque cabecera      
            char* prgName = new char[10 + 1];

            if (isCorrectHeader(header,startByte))
            {
                // Es una cabecera PROGRAM 
                // el nombre está en los bytes del 4 al 13 (empezando en 0)
                // SerialHW.println("");
                // SerialHW.println("Name detected ");

                // Extraemos el nombre del programa
                for (int n=4;n<14;n++)
                {   
                    prgName[n-4] = (char)header[n];
                }
            }

            // Pasamos la cadena de caracteres
            return prgName;
        }

        char* getBlockName(char* prgName, uint8_t* header, int startByte)
        {
            // Obtenemos el nombre del bloque cabecera
            // Es una cabecera PROGRAM 
            // el nombre está en los bytes del 4 al 13 (empezando en 0)         
            header[startByte+12] = 0;
            return (char*)(header+2);             
        }
  
        // uint8_t* getBlockRange(uint8_t* bBlock, int byteStart, int byteEnd)
        // {

        //     // Extraemos un bloque de bytes indicando el uint8_t inicio y final
        //     //uint8_t* blockExtracted = new uint8_t[(byteEnd - byteStart)+1];
        //     uint8_t* blockExtracted = (uint8_t*)(malloc(((byteEnd - byteStart)+1) * sizeof(uint8_t)));

        //     int i=0;
        //     for (int n=byteStart;n<(byteStart + (byteEnd - byteStart)+1);n++)
        //     {
        //         blockExtracted[i] = bBlock[n];
        //     }

        //     return blockExtracted;
        // }

        int getNumBlocks(File32 mFile, int sizeTAP)
        {

            int startBlock = 0;
            int lastStartBlock = 0;
            int sizeB = 0;
            int newSizeB = 0;
            int chk = 0;
            int blockChk = 0;
            int numBlocks = 0;

            FILE_CORRUPTED = false;
            
            bool reachEndByte = false;

            int state = 0;    

            //Entonces recorremos el TAP. 
            // La primera cabecera SIEMPRE debe darse.
            // SerialHW.println("");
            // SerialHW.println("Analyzing TAP file. Please wait ...");
            
            // SerialHW.println("");
            // SerialHW.println("SIZE TAP: " + String(sizeTAP));

            // Los dos primeros bytes son el tamaño a contar
            sizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];
            startBlock = 2;

            while(reachEndByte==false)
            {

                uint8_t* tmpRng = (uint8_t*)(malloc(sizeB * sizeof(uint8_t)));

                tmpRng = sdm.readFileRange32(_mFile,startBlock,sizeB-1,false);
                chk = calculateChecksum(tmpRng,0,sizeB-1);

                free(tmpRng);                            
                blockChk = sdm.readFileRange32(_mFile,startBlock+sizeB-1,1,false)[0];
            

                if (blockChk == chk)
                {                                  
                    // Siguiente bloque
                    // Direcion de inicio (offset)
                    startBlock = startBlock + sizeB;
                    // Tamaño
                    newSizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];

                    numBlocks++;
                    sizeB = newSizeB;
                    startBlock = startBlock + 2;

                    // SerialHW.println("");
                    // SerialHW.print("OFFSET: 0x");
                    // SerialHW.print(startBlock,HEX);
                    // SerialHW.print(" / SIZE:   " + String(newSizeB));
                }
                else
                {
                    reachEndByte = true;
                    // SerialHW.println("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock));

                    // Abortamos
                    FILE_CORRUPTED = true;
                }

                // ¿Hemos llegado al ultimo uint8_t
                if (startBlock > sizeTAP)                
                {
                    reachEndByte = true;
                    //break;
                    // SerialHW.println("");
                    // SerialHW.println("Success. End: ");
                }

            }

            return numBlocks;
        }

        bool getInformationOfHead(tBlockDescriptor &tB, int flagByte, int typeBlock, int startBlock, int sizeB, char (&nameTAP)[11])
        {
        
        // Obtenemos informacion de la cabecera

        bool blockNameDetected = false;
        int state = 0;

        if (flagByte < 128)
        {

            // Inicializamos                    
            tB.type = 0;
            strncpy(tB.typeName,"",10);
            
            // Es una CABECERA
            if (typeBlock==0)
            {
                // Es un header PROGRAM

                tB.header = true;
                tB.type = HPRGM;

                blockNameDetected = true;

                // Almacenamos el nombre
                //getBlockName(tB.name,sdm.readFileRange32(_mFile,startBlock,19,false),0);
                strncpy(tB.name,getBlockName(tB.name,sdm.readFileRange32(_mFile,startBlock,19,false),0),10);



                //Cogemos el nombre del TAP de la primera cabecera
                if (startBlock < 23)
                {
                    //nameTAP = (String)bDscr[numBlocks].name;
                    //nameTAP = bDscr[numBlocks].name;
                    strncpy(nameTAP,tB.name,10);
                }

                state = 1;
            }
            else if (typeBlock==1)
            {
                // Array num header
                // Almacenamos el nombre
                strncpy(tB.name,getBlockName(tB.name,sdm.readFileRange32(_mFile,startBlock,19,false),0),10);
                tB.type = HARRAYNUM;    

            }
            else if (typeBlock==2)
            {
                // Array char header
                // Almacenamos el nombre
                strncpy(tB.name,getBlockName(tB.name,sdm.readFileRange32(_mFile,startBlock,19,false),0),10);
                tB.type = HARRAYCHR;    

            }
            else if (typeBlock==3)
            {
                // Byte block

                // Vemos si es una cabecera de una SCREEN
                // para ello temporalmente vemos el tamaño del bloque de codigo. Si es 6914 bytes (incluido el checksum y el flag - 6912 + 2 bytes)
                // es una pantalla SCREEN
                blockNameDetected = true;
                
                // Almacenamos el nombre
                strncpy(tB.name,getBlockName(tB.name,sdm.readFileRange32(_mFile,startBlock,19,false),0),10);

                int tmpSizeBlock = (256*sdm.readFileRange32(_mFile,startBlock + sizeB+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock + sizeB,1,false)[0];

                if (tmpSizeBlock == 6914)
                {
                    // Es una cabecera de un Screen
                    tB.screen = true;
                    tB.type = HSCRN;                              

                    state = 2;
                }
                else
                {               
                    tB.type = HCODE;    
                }
        
            }

        }
        else
        {
            if (state == 1)
            {
                state = 0;
                // Es un bloque BASIC                         
                tB.type = PRGM;
            }
            else if (state == 2)
            {
                state = 0;
                // Es un bloque SCREEN                     
                tB.type = SCRN;                         
            }
            else
            {
                // Es un bloque CM                        
                tB.type = CODE;                         
            }
        }   

        return blockNameDetected;     
        }

        void getBlockDescriptor(File32 mFile, int sizeTAP)
        {
            // Este procedimiento permite analizar un fichero .TAP
            // obteniendo de este información relevante y los distintos bloques que lo
            // forman. De cada bloque almacenaremos la dirección de inicio ó 
            // posición en el fichero (offset) así como el tamaño.
            
            // Para ello tenemos que ir leyendo el TAP poco a poco
            // y llegando a los bytes que indican TAMAÑO(2 bytes) + 0xFF(1 uint8_t)

            // Este procedimiento recorre el .TAP y devuelve el número de bloques que contiene
            //int numBlks = getNumBlocks(mFile, sizeTAP);
            
            if (!FILE_CORRUPTED)
            {
                // La reserva de memoria para el descriptor de bloques del TAP
                // se hace en powadcr.ino

                //  Inicializamos variables
                char nameTAP[11];
                char typeName[11];
                char blockName[11];

                int startBlock = 0;
                int lastStartBlock = 0;
                int sizeB = 0;
                int newSizeB = 0;
                int chk = 0;
                int blockChk = 0;
                int numBlocks = 0;
                int state = 0;    

                bool blockNameDetected = false;                
                bool reachEndByte = false;

                //Entonces recorremos el TAP. 
                // SerialHW.println("");
                // SerialHW.println("Analyzing TAP file. Please wait ...");
                
                // La primera cabecera SIEMPRE debe darse.
                // Los dos primeros bytes son el tamaño a contar
                sizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];
                startBlock = 2;

                // Recorremos ahora el fichero
                while(reachEndByte==false && sizeB!=0)
                {
                    // Inicializamos
                    blockNameDetected = false;
                    
                    // Cogemos el bloque completo, para poder calcular su checksum
                    uint8_t* tmpRng = (uint8_t*)(malloc(sizeB * sizeof(uint8_t)));
                    tmpRng = sdm.readFileRange32(_mFile,startBlock,sizeB-1,false);
                    // Calculamos el checksum
                    chk = calculateChecksum(tmpRng,0,sizeB-1);
                    // Liberamos
                    free(tmpRng);
                    
                    // Obtenemos el valor de checksum de la cabecera del bloque
                    blockChk = sdm.readFileRange32(_mFile,startBlock+sizeB-1,1,false)[0];         

                    // Comparamos para asegurarnos que el bloque es correcto
                    if (blockChk == chk)
                    {
                        // Si es correcto entonces, prosigo obteniendo información
                        // Almaceno el inicio del bloque
                        _myTAP.descriptor[numBlocks].offset = startBlock;
                        // Almaceno el tamaño del bloque
                        _myTAP.descriptor[numBlocks].size = sizeB;
                        // Almaceno el checksum calculado
                        _myTAP.descriptor[numBlocks].chk = chk;        

                        // Cogemos mas info del bloque
                        
                        // Flagbyte
                        // 0x00 - HEADER
                        // 0xFF - DATA BLOCK
                        int flagByte = sdm.readFileRange32(_mFile,startBlock,1,false)[0];

                        // Typeblock
                        // 0x00 - PROGRAM
                        // 0x01 - ARRAY NUM
                        // 0x02 - ARRAY CHAR
                        // 0x03 - CODE FILE
                        int typeBlock = sdm.readFileRange32(_mFile,startBlock+1,1,false)[0];
                        
                        // Vemos si el bloque es una cabecera o un bloque de datos (bien BASIC o CM)
                        blockNameDetected = getInformationOfHead(_myTAP.descriptor[numBlocks],flagByte,typeBlock,startBlock,sizeB,nameTAP);

                        strncpy(_myTAP.descriptor[numBlocks].typeName,getTypeTAPBlock(_myTAP.descriptor[numBlocks].type),10)                       ;                      

                        // Si el bloque contenia nombre, se indica
                        if (blockNameDetected)
                        {                                     
                            _myTAP.descriptor[numBlocks].nameDetected = true;                                      
                        }
                        else
                        {
                            _myTAP.descriptor[numBlocks].nameDetected = false;
                        }

                        // Siguiente bloque
                        // Direcion de inicio (offset)
                        startBlock = startBlock + sizeB;
                        // Tamaño
                        newSizeB = (256*sdm.readFileRange32(_mFile,startBlock+1,1,false)[0]) + sdm.readFileRange32(_mFile,startBlock,1,false)[0];
                        // Pasamos al siguiente bloque
                        numBlocks++;
                        // Ahora el tamaño es el nuevo tamaño calculado
                        sizeB = newSizeB;
                        // El inicio de bloque comienza 2 bytes mas adelante (nos saltamos 13 00 ó FF 00)
                        startBlock = startBlock + 2;

                    }
                    else
                    {
                        // Si los checksum no coinciden. Indicamos que hemos acabado
                        // y decimos que hay un error (debug)
                        reachEndByte = true;
                        // SerialHW.println("Error in checksum. Block --> " + String(numBlocks) + " - offset: " + String(lastStartBlock));
                        
                        // Añadimos información importante
                        strncpy(_myTAP.name,"",1);
                        _myTAP.size = 0;
                        _myTAP.numBlocks = 0; 
                        // Salimos ya de la función.
                        return;
                    }

                    // ¿Hemos llegado al ultimo byte? Entonces lo indicamos reachEndByte = true
                    if (startBlock > sizeTAP)                
                    {
                        // Con esto en la siguiente vuelta, salimos del while
                        reachEndByte = true;
                        //break;
                        // SerialHW.println("");
                        // SerialHW.println("Success. End: ");
                    }

                    TOTAL_BLOCKS = numBlocks;
                }

                // Añadimos información importante
                strncpy(_myTAP.name,nameTAP,sizeof(nameTAP));
                _myTAP.size = sizeTAP;
                _myTAP.numBlocks = numBlocks;
            
            }
            else
            {
                // Ponemos la información a cero porque el
                // fichero estaba corrupto.
                strncpy(_myTAP.name,"",1);
                _myTAP.size = 0;
                _myTAP.numBlocks = 0;            
            }

            // Actualizamos el indicador de consumo de PS_RAM
            //_hmi.updateMem();
        }

        void showDescriptorTable()
        {
            // SerialHW.println("");
            // SerialHW.println("");
            // SerialHW.println("++++++++++++++++++++++++++++++ Block Descriptor +++++++++++++++++++++++++++++++++++++++");

            int totalBlocks = _myTAP.numBlocks;

            for (int n=0;n<totalBlocks;n++)
            {
                // SerialHW.print("[" + String(n) + "]" + " - Offset: " + String(_myTAP.descriptor[n].offset) + " - Size: " + String(_myTAP.descriptor[n].size));
                char* strType = &INITCHAR[0];
                
                switch(_myTAP.descriptor[n].type)
                {
                    case 0: 
                    strType = &STRTYPE0[0];
                    break;

                    case 1:
                    strType = &STRTYPE1[0];
                    break;

                    case 7:
                    strType = &STRTYPE7[0];
                    break;

                    case 2:
                    strType = &STRTYPE2[0];
                    break;

                    case 3:
                    strType = &STRTYPE3[0];
                    break;

                    case 4:
                    strType = &STRTYPE4[0];
                    break;

                    default:
                    strType=&STRTYPEDEF[0];
                }

                if (_myTAP.descriptor[n].nameDetected)
                {
                    // SerialHW.println("");
                    //// SerialHW.print("[" + String(n) + "]" + " - Name: " + (String(bDscr[n].name)).substring(0,10) + " - (" + strType + ")");
                    // SerialHW.print("[" + String(n) + "]" + " - Name: " + _myTAP.descriptor[n].name + " - (" + strType + ")");
                }
                else
                { 
                    // SerialHW.println("");
                    // SerialHW.print("[" + String(n) + "] - " + strType + " ");
                }

                // SerialHW.println("");
                // SerialHW.println("");

            }      
        }

        // int getTotalHeaders(uint8_t* fileTAP, int sizeTAP)
        // {
        //     // Este procedimiento devuelve el total de bloques que contiene el fichero
        //     int nblocks = 0;
        //     //uint8_t* bBlock = new uint8_t[sizeTAP];
        //     uint8_t* bBlock = (uint8_t*)(malloc(sizeTAP * sizeof(uint8_t)));
            
        //     bBlock = fileTAP; 
        //     // Para ello buscamos la secuencia "0x13 0x00 0x00"
        //     for (int n=0;n<sizeTAP;n++)
        //     {
        //         if (bBlock[n] == 19)
        //         {
        //             if ((n+1 < sizeTAP) && (bBlock[n+1] == 0))
        //             {
        //                 if ((n+2 < sizeTAP) && (bBlock[n+2] == 0))
        //                 {
        //                     nblocks++;
        //                     n = n + 3;
        //                 }
        //             }
        //         }
        //     }
            
        //     free(bBlock);
        //     return nblocks;
        // }

    public:
    
        void showInfoBlockInProgress(int nBlock)
        {
            switch(nBlock)
            {
                case 0:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("> PROGRAM HEADER");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE0[0],sizeof(&LASTYPE0[0]));
                    break;

                case 1:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> BYTE HEADER");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE1[0],sizeof(&LASTYPE1[0]));
                    break;

                case 7:

                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> SCREEN HEADER");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE7[0],sizeof(&LASTYPE7[0]));
                    break;

                case 2:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> BASIC PROGRAM");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE2[0],sizeof(&LASTYPE2[0]));
                    break;

                case 3:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> SCREEN");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE3[0],sizeof(&LASTYPE3[0]));
                    break;

                case 4:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> BYTE CODE");
                    #endif
                    if (LAST_SIZE != 6914)
                    {
                    strncpy(LAST_TYPE,&LASTYPE4_1[0],sizeof(&LASTYPE4_1[0]));
                    }
                    else
                    {
                    strncpy(LAST_TYPE,&LASTYPE4_2[0],sizeof(&LASTYPE4_2[0]));
                    }
                    break;

                case 5:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> ARRAY.NUM");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE5[0],sizeof(&LASTYPE5[0]));
                    break;

                case 6:
                    // Definimos el buffer del PLAYER igual al tamaño del bloque
                    #if LOG==3
                    // SerialHW.println("");
                    // SerialHW.println("> ARRAY.CHR");
                    #endif
                    strncpy(LAST_TYPE,&LASTYPE6[0],sizeof(&LASTYPE6[0]));
                    break;


            }        
        }

        // void deallocatingTAP()
        // {
        //     _//hmi.getMemFree();
        //     _hmi.updateMem();

        //     log("Deallocating TAP");
        //     log("--------------------------------------");
        //     // SerialHW.printf("Direccion de la copia %p", _myTAP.descriptor);
            
        //     free(_myTAP.descriptor);
            
        //     _//hmi.getMemFree();
        //     _hmi.updateMem();
        // }
        
        void set_SdFat32(SdFat32 sdf32)
        {
            _sdf32 = sdf32;
        }

        void set_file(File32 tapFileName, int sizeTAP)
        {
            // Pasamos los parametros a la clase
            _mFile = tapFileName;
            _sizeTAP = sizeTAP;
        }

        void set_SDM(SDmanager sdmTmp)
        {
            //_sdm = sdmTmp;
            //ptrSdm = &sdmTmp;
        }

        void set_HMI(HMI hmi)
        {
            _hmi = hmi;
        }

        void setTAP(tTAP tap)
        {
            _myTAP = tap;
        }

        tBlockDescriptor* getDescriptor()
        {
            return _myTAP.descriptor;
        }

        tTAP getTAP()
        {
            // Devolvemos el descriptor del TAP
            return _myTAP;
        }

        char* getTAPName()
        {
            // Devolvemos el nombre del TAP
            return _myTAP.name;
        }

        int geTAPNumBlocks()
        {
            // Devolvemos el numero de bloques del TAP
            return _myTAP.numBlocks;
        }

        void updateMemIndicator()
        {
            _hmi.updateMem();
        }

        void initialize()
        {
            strncpy(_myTAP.name,"",1);
            _myTAP.numBlocks = 0;
            _myTAP.size = 0;
            CURRENT_BLOCK_IN_PROGRESS = 0;
            BLOCK_SELECTED = 0;
            _hmi.writeString("currentBlock.val=" + String(BLOCK_SELECTED));
            _hmi.writeString("progression.val=" + String(0));     
        }

        void terminate()
        {
            strncpy(_myTAP.name,"",1);
            _myTAP.numBlocks = 0;
            _myTAP.size = 0;     
        }

        bool proccess_tap(File32 tapFileName)
        {
            // Procesamos el fichero .TAP
            
            // SerialHW.println("");
            // SerialHW.println("Getting total blocks...");

            // Verificamos que sea un TAP correcto.
            if (isFileTAP(tapFileName))
            {
                // Analizamos el .TAP y obtenemos el descriptor de bloques
                getBlockDescriptor(_mFile, _sizeTAP);

                // if (!FILE_CORRUPTED)
                // {
                //     // Mostramos información del descriptor (para debug)
                //     showDescriptorTable();
                // }
                return true;
            }
            else
            {
                return false;
            }
        }

        void getInfoFileTAP(char* path) 
        {
            
            // Comenzamos a trabajar con el fichero .TAP

            File32 tapFile;
            
            FILE_CORRUPTED = false;
            
            LAST_MESSAGE = "Analyzing file";
            delay(500);
            
            // Abrimos el fichero
            tapFile = sdm.openFile32(tapFile, path);
            
            // Obtenemos su tamaño total
            _mFile = tapFile;
            _rlen = tapFile.available();
            
            // creamos un objeto TAPproccesor
            set_file(tapFile, _rlen);
            
            // Actualizamos el indicador de memoria
            //_hmi.getMemFree();
           //_hmi.updateMem();

            if (proccess_tap(tapFile))
            {
                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("END PROCCESING TAP: ");

                if (_myTAP.descriptor != NULL)
                {
                    // Entregamos información por consola
                    PROGRAM_NAME = _myTAP.name;
                    TOTAL_BLOCKS = _myTAP.numBlocks;
                    strncpy(LAST_TYPE,&INITCHAR2[0],sizeof(&INITCHAR2[0]));
                
                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println("PROGRAM_NAME: " + PROGRAM_NAME);
                    // SerialHW.println("TOTAL_BLOCKS: " + String(TOTAL_BLOCKS));
                
                    // Pasamos informacion del descriptor al HMI         
                    _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);
                    // Actualizamos la pantalla
                    //
                }          
            }
            else
            {
                FILE_CORRUPTED = true;
            }
        }

        void play() 
        {

            if (_myTAP.descriptor != NULL)
            {         
            
                    // Inicializamos el buffer de reproducción. Memoria dinamica
                    uint8_t* bufferPlay;

                    // Entregamos información por consola
                    PROGRAM_NAME = _myTAP.name;
                    TOTAL_BLOCKS = _myTAP.numBlocks;
                    strncpy(LAST_NAME,&INITCHAR2[0],sizeof(&INITCHAR2[0]));

                    // Ahora reproducimos todos los bloques desde el seleccionado (para cuando se quiera uno concreto)
                    int m = BLOCK_SELECTED;
                    //BYTES_TOBE_LOAD = _rlen;

                    // Reiniciamos
                    if (BLOCK_SELECTED == 0) 
                    {
                        BYTES_LOADED = 0;
                        BYTES_TOBE_LOAD = _rlen;
                        //_hmi.writeString("");
                        _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED * 100) / (BYTES_TOBE_LOAD))));
                    } 
                    else 
                    {
                        BYTES_TOBE_LOAD = _rlen - _myTAP.descriptor[BLOCK_SELECTED - 1].offset;
                    }

                    for (int i = m; i < _myTAP.numBlocks; i++) 
                    {
                        BLOCK_PLAYED = false;

                        // Obtenemos el nombre del bloque
                        strncpy(LAST_NAME,_myTAP.descriptor[i].name,sizeof(_myTAP.descriptor[i].name));
                        LAST_SIZE = _myTAP.descriptor[i].size;

                        // LOADING_STATE se usa también para detener el procesador de audio (ZXProcessor.h).
                        //
                        // El estado LOADING_STATE=0 es un estado INICIAL.
                        // El estado LOADING_STATE=1 es un estado de PLAYING.
                        // El estado LOADING_STATE=2 es una MANUAL STOP.
                        //
                        if (LOADING_STATE==2)
                        {
                            LOADING_STATE = 0;
                            PAUSE = false;
                            STOP = true;
                            PLAY = false;

                            i = _myTAP.numBlocks+1;

                            log("LOADING_STATE 2");                           
                            return;
                        }
                        else if (LOADING_STATE==3)
                        {
                            LOADING_STATE = 0;
                            PAUSE = true;
                            STOP = false;
                            PLAY = false;

                            //i = _myTAP.numBlocks+1;

                            log("LOADING_STATE 3"); 
                            return; 
                        }
                        else
                        {
                            // Almacenmas el bloque en curso para un posible PAUSE

                            CURRENT_BLOCK_IN_PROGRESS = i;
                            BLOCK_SELECTED = i;

                            _hmi.writeString("currentBlock.val=" + String(i + 1));
                            _hmi.writeString("progression.val=" + String(0));
                        }

                        //Ahora vamos lanzando bloques dependiendo de su tipo
                        //Esto actualiza el LAST_TYPE
                        showInfoBlockInProgress(_myTAP.descriptor[i].type);

                        // Actualizamos HMI
                        _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);

                        //

                        // Reproducimos el fichero
                        if (_myTAP.descriptor[i].type == 0) 
                        {

                            // Reservamos memoria para el buffer de reproducción
                            bufferPlay = (uint8_t*)(malloc(_myTAP.descriptor[i].size * sizeof(uint8_t)));
                            bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, false);

                            // *** Cabecera PROGRAM
                            // Llamamos a la clase de reproducción
                            zxp.playData(bufferPlay, _myTAP.descriptor[i].size,DPILOT_HEADER * DPULSE_PILOT);

                            // Liberamos el buffer de reproducción
                            free(bufferPlay);
                    } 
                        else if (_myTAP.descriptor[i].type == 1 || _myTAP.descriptor[i].type == 7) 
                        {
                            
                            bufferPlay = (uint8_t*)(malloc(_myTAP.descriptor[i].size * sizeof(uint8_t)));
                            bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, false);

                            // *** Cabecera BYTE
                            // Llamamos a la clase de reproducción
                            zxp.playData(bufferPlay, _myTAP.descriptor[i].size,DPILOT_HEADER * DPULSE_PILOT);

                            // Liberamos el buffer de reproducción
                            free(bufferPlay);
                        } 
                        else 
                        {
                            // *** Bloque de DATA
                            int blockSize = _myTAP.descriptor[i].size;

                            // Si el SPLIT esta activado y el bloque es mayor de 20KB hacemos Split.
                            if ((SPLIT_ENABLED) && (blockSize > SIZE_TO_ACTIVATE_SPLIT)) 
                            {

                                // Lanzamos dos bloques
                                int bl1 = blockSize / 2;
                                int bl2 = blockSize - bl1;
                                int blockPlaySize = 0;
                                int offsetPlay = 0;

                                for (int j = 0; j < 2; j++) 
                                {

                                    if (j == 0) 
                                    {
                                        
                                        // Cortamos la primera mitad del bloque
                                        blockPlaySize = bl1;
                                        bufferPlay = (uint8_t*)(malloc(blockPlaySize * sizeof(uint8_t)));
                                        offsetPlay = _myTAP.descriptor[i].offset;
                                        bufferPlay = sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);
                                        
                                        // Reproducimos la primera mitad
                                        zxp.playDataBegin(bufferPlay, blockPlaySize,DPILOT_DATA * DPULSE_PILOT);
                                        
                                        // Liberamos el buffer de reproducción
                                        free(bufferPlay);                                      
                                    } 
                                    else 
                                    {
                                        // Cortamos el final del bloque
                                        blockPlaySize = bl2;
                                        offsetPlay = offsetPlay + bl1;
                                        bufferPlay = (uint8_t*)(malloc(blockPlaySize * sizeof(uint8_t)));
                                        bufferPlay = sdm.readFileRange32(_mFile, offsetPlay, blockPlaySize, true);

                                        // Reproducimos la ultima mitad
                                        zxp.playDataEnd(bufferPlay, blockPlaySize,DPILOT_DATA * DPULSE_PILOT);

                                        // Liberamos el buffer de reproducción
                                        free(bufferPlay);
                                    }
                                }
                            } 
                            else 
                            {
                                // En el caso de NO USAR SPLIT o el bloque es menor de 20K
                                bufferPlay = (uint8_t*)(malloc((_myTAP.descriptor[i].size) * sizeof(uint8_t)));
                                bufferPlay = sdm.readFileRange32(_mFile, _myTAP.descriptor[i].offset, _myTAP.descriptor[i].size, false);
                                
                                // Reproducimos el bloque de datos
                                zxp.playData(bufferPlay, _myTAP.descriptor[i].size,DPILOT_DATA * DPULSE_PILOT);

                                // Liberamos el buffer de reproducción
                                free(bufferPlay);
                            }
                        }

                        BLOCK_PLAYED = true;
                    }

                    // SerialHW.println("");
                    // SerialHW.println("Playing was finish.");

                    // En el caso de no haber parado manualmente, 
                    // Lanzamos el AUTO-STOP
                    if (LOADING_STATE == 1) 
                    {
                        PLAY = false;
                        STOP = true;
                        PAUSE = false;

                        LAST_MESSAGE = "Playing end. Automatic STOP.";

                        _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);
                        //

                        // SerialHW.println("");
                        // SerialHW.println("LOADING_STATE 1");
                    }              
            }
            else
            {
                // No se ha seleccionado ningún fichero
                LAST_MESSAGE = "No file selected.";
                _hmi.setBasicFileInformation(_myTAP.descriptor[BLOCK_SELECTED].name,_myTAP.descriptor[BLOCK_SELECTED].typeName,_myTAP.descriptor[BLOCK_SELECTED].size);
                //
            }

        }

        // Constructor de la clase
        TAPproccesor(AudioKit kit)
        {
            _myTAP.descriptor = NULL;
        }      

};
