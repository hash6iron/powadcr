/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TZXproccesor.h

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
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
#pragma once
#include "HMI.h"
#include "ZXProcessor.h"
#include "globales.h"
#include <Arduino.h>
#include <FS.h>
#include <miniz.h>

class TZXprocessor {

private:
  const char ID10STR[35] = "ID 10 - Standard block            ";
  const char ID11STR[35] = "ID 11 - Speed block               ";
  const char ID12STR[35] = "ID 12 - Pure tone                 ";
  const char ID13STR[35] = "ID 13 - Pulse seq.                ";
  const char ID14STR[35] = "ID 14 - Pure data                 ";
  const char ID15STR[35] = "ID 15 - Direct recording          ";
  const char ID18STR[35] = "ID 18 - CSW recording             ";
  const char ID19STR[35] = "ID 19 - GDB                       ";
  const char ID20STR[35] = "ID 20 - Pause or Stop             ";
  const char ID21STR[35] = "ID 21 - Group start               ";
  const char ID22STR[35] = "ID 22 - Group end                 ";
  const char ID23STR[35] = "ID 23 - Jump to block             ";
  const char ID24STR[35] = "ID 24 - Loop start                ";
  const char ID25STR[35] = "ID 25 - Loop end                  ";
  const char ID26STR[35] = "ID 26 - Call sequence             ";
  const char ID27STR[35] = "ID 27 - Return from sequence      ";
  const char ID28STR[35] = "ID 28 - Select block              ";
  const char ID2ASTR[35] = "ID 2A - Stop TAPE (48k mode)      ";
  const char ID2BSTR[35] = "ID 2B - Set signal level          ";
  const char ID4BSTR[35] = "ID 4B - TSX Block                 ";
  const char ID5ASTR[35] = "ID 5A - Glue block                ";
  const char IDXXSTR[35] = "Information block                 ";

  const int maxAllocationBlocks = 4000;

  // Procesador de audio output
  ZXProcessor _zxp;
  // BlockProcessor _blDscTZX;

  HMI _hmi;

  // Definicion de un TZX
  // SdFat32 _sdf32;
  tTZX _myTZX;
  PZXprocessor _pzx;
  File _mFile;
  int _sizeTZX;
  int _rlen;

  // Audio HW
  AudioInfo new_sr;
  // AudioInfo new_sr2;

  int CURRENT_LOADING_BLOCK = 0;

  bool stopOrPauseRequest() {
    //

    if (LOADING_STATE == 1) {
      if (STOP == true) {
        LAST_MESSAGE = "Stop or pause requested";
        LOADING_STATE = 2; // STOP del bloque actual
        return true;
      } else if (PAUSE == true) {
        LAST_MESSAGE = "Stop or pause requested";
        LOADING_STATE = 3; // PAUSE del bloque actual
        return true;
      }
    }

    return false;
  }

  uint8_t calculateChecksum(uint8_t *bBlock, int startByte, int numBytes) {
    // Calculamos el checksum de un bloque de bytes
    uint8_t newChk = 0;

#if LOG > 3
    ////SerialHW.println("");
    ////SerialHW.println("Block len: ");
    ////SerialHW.print(sizeof(bBlock)/sizeof(uint8_t*));
#endif

    // Calculamos el checksum (no se contabiliza el ultimo numero que es
    // precisamente el checksum)

    for (int n = startByte; n < (startByte + numBytes); n++) {
      newChk = newChk ^ bBlock[n];
    }

#if LOG > 3
    ////SerialHW.println("");
    ////SerialHW.println("Checksum: " + String(newChk));
#endif

    return newChk;
  }

  char *getNameFromStandardBlock(uint8_t *header) {
    // Obtenemos el nombre del bloque cabecera
    static char prgName[11];

    // Extraemos el nombre del programa desde un ID10 - Standard block
    for (int n = 2; n < 12; n++) {
      if (header[n] <= 32) {
        // Los caracteres por debajo del espacio 0x20 se sustituyen
        // por " " ó podemos poner ? como hacía el Spectrum.
        prgName[n - 2] = ' ';
      }
      if (header[n] == 96) {
        prgName[n - 2] =
            (char)0xA3; //'£'; // El caracter 96 es la libra esterlina
      }
      if (header[n] == 126) {
        prgName[n - 2] = (char)0x7E; //'~'; // El caracter 126 es la virgulilla
      }
      if (header[n] == 127) {
        prgName[n - 2] = (char)0xA9; //'©'; //  El caracter 127 es el copyright
      }
      if (header[n] > 127) {
        prgName[n - 2] =
            ' '; // Los caracteres por encima del espacio 0x7F se sustituyen
      } else {
        prgName[n - 2] = (char)header[n];
      }
    }

    logln("Program name: " + String(prgName));

    // Pasamos la cadena de caracteres
    return prgName;
  }

  bool isHeaderTZX(File tzxFile) {
    if (tzxFile != 0) {

      // Capturamos la cabecera
      uint8_t *bBlock = (uint8_t *)ps_calloc(10 + 1, sizeof(uint8_t));
      readFileRange(tzxFile, bBlock, 0, 10, false);

      // Obtenemos la firma del TZX
      char signTZXHeader[9];

      // Analizamos la cabecera
      // Extraemos el nombre del programa
      for (int n = 0; n < 7; n++) {
        signTZXHeader[n] = (char)bBlock[n];
      }

      free(bBlock);

      // Aplicamos un terminador a la cadena de char
      signTZXHeader[7] = '\0';
      // Convertimos a String
      String signStr = String(signTZXHeader);

      if (signStr.indexOf("ZXTape!") != -1) {
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  bool isFileTZX(File mfile) {
    if (mfile) {
      // Capturamos el nombre del fichero en szName
      String szName = mfile.name();
      String fileName = String(szName);

      if (fileName != "") {
        // String fileExtension = szNameStr.substring(szNameStr.length()-3);
        fileName.toUpperCase();

        if (fileName.indexOf(".TZX") != -1 || fileName.indexOf(".TSX") != -1 ||
            fileName.indexOf(".CDT") != -1) {
          // La extensión es TZX o CDT pero ahora vamos a comprobar si
          // internamente también lo es
          if (isHeaderTZX(mfile)) {
            // Cerramos el fichero porque no se usará mas.
            // mfile.close();
            return true;
          } else {
            // Cerramos el fichero porque no se usará mas.
            // mfile.close();
            return false;
          }
        } else {
          return false;
        }
      } else {
        return false;
      }

    } else {
      return false;
    }
  }

  // int getWORD(File mFile, int offset)
  // {
  //     int sizeDW = 0;
  //     uint8_t* ptr1 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
  //     uint8_t* ptr2 = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
  //     readFileRange(mFile,ptr1,offset+1,1,false);
  //     readFileRange(mFile,ptr2,offset,1,false);
  //     sizeDW = (256*ptr1[0]) + ptr2[0];
  //     free(ptr1);
  //     free(ptr2);

  //     return sizeDW;
  // }

  // ✅ VERSIÓN OPTIMIZADA DE getWORD
  int getWORD(File mFile, int offset) {
    uint8_t buffer[2];

    mFile.seek(offset);
    mFile.read(buffer, 2);

    return (buffer[1] << 8) | buffer[0]; // Little-endian
  }

  // int getBYTE(File mFile, int offset)
  // {
  //     int sizeB = 0;
  //     uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));
  //     readFileRange(mFile,ptr,offset,1,false);
  //     sizeB = ptr[0];
  //     free(ptr);

  //     return sizeB;
  // }

  // ✅ VERSIÓN OPTIMIZADA - CON BUFFER ESTÁTICO
  int getBYTE(File mFile, int offset) {
    uint8_t buffer[1];

    mFile.seek(offset);
    mFile.read(buffer, 1);

    return buffer[0];
  }

  // ✅ AÑADIR getDWORD
  uint32_t getDWORD(File mFile, int offset) {
    uint8_t buffer[4];

    mFile.seek(offset);
    mFile.read(buffer, 4);

    return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) |
           buffer[0]; // Little-endian
  }

  // int getNBYTE(File mFile, int offset, int n)
  // {
  //     int sizeNB = 0;
  //     uint8_t* ptr = (uint8_t*)ps_calloc(1+1,sizeof(uint8_t));

  //     for (int i = 0; i<n;i++)
  //     {
  //         readFileRange(mFile,ptr,offset+i,1,false);
  //         sizeNB += pow(2,(8*i)) * (ptr[0]);
  //     }

  //     free(ptr);
  //     return sizeNB;
  // }

  // ✅ VERSIÓN OPTIMIZADA DE getNBYTE
  int getNBYTE(File mFile, int offset, int n) {
    if (n > 4)
      return 0; // Máximo 4 bytes para un int

    uint8_t buffer[4];

    mFile.seek(offset);
    mFile.read(buffer, n);

    int result = 0;
    for (int i = 0; i < n; i++) {
      result |= (buffer[i] << (8 * i));
    }

    return result;
  }

  int getID(File mFile, int offset) {
    // Obtenemos el ID
    int ID = 0;
    ID = getBYTE(mFile, offset);

    return ID;
  }

  void getBlock(File mFile, uint8_t *&block, int offset, int size) {
    // Entonces recorremos el TZX.
    //  La primera cabecera SIEMPRE debe darse.
    readFileRange(mFile, block, offset, size, false);
  }

  bool verifyChecksum(File mFile, int offset, int size) {
    // Vamos a verificar que el bloque cumple con el checksum
    // Cogemos el checksum del bloque
    uint8_t chk = getBYTE(mFile, offset + size - 1);

    uint8_t *block = (uint8_t *)ps_calloc(size + 1, sizeof(uint8_t));
    getBlock(mFile, block, offset, size - 1);
    uint8_t calcChk = calculateChecksum(block, 0, size - 1);
    free(block);

    if (chk == calcChk) {
      return true;
    } else {
      return false;
    }
  }

  void analyzeID16(File mFile, int currentOffset, int currentBlock) {
    // Normal speed block

    //
    // TAP description block
    //

    //       |------ Spectrum-generated data -------|       |---------|

    // 13 00 00 03 52 4f 4d 7x20 02 00 00 00 00 80 f1 04 00 ff f3 af a3

    // ^^^^^...... first block is 19 bytes (17 bytes+flag+checksum)
    //       ^^... flag byte (A reg, 00 for headers, ff for data blocks)
    //          ^^ first byte of header, indicating a code block

    // file name ..^^^^^^^^^^^^^
    // header info ..............^^^^^^^^^^^^^^^^^
    // checksum of header .........................^^
    // length of second block ........................^^^^^
    // flag byte ...........................................^^
    // first two bytes of rom .................................^^^^^
    // checksum (checkbittoggle would be a better name!).............^^

    int headerTAPsize = 21; // Contando blocksize + flagbyte + etc hasta
                            // checksum
    int dataTAPsize = 0;
    char *gName;

    tTZXBlockDescriptor blockDescriptor;
    _myTZX.descriptor[currentBlock].ID = 16;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // ////SerialHW.println("");
    // ////SerialHW.println("Offset begin: ");
    // ////SerialHW.print(currentOffset,HEX);

    // SYNC1
    //  _zxp.SYNC1 = DSYNC1;
    //  //SYNC2
    //  _zxp.SYNC2 = DSYNC2;
    //  //PULSE PILOT
    //  _zxp.PILOT_PULSE_LEN = DPILOT_LEN;
    //  // BTI 0
    //  _zxp.BIT_0 = DBIT_0;
    //  // BIT1
    //  _zxp.BIT_1 = DBIT_1;
    //  No contamos el ID. Entonces:

    // Timming de la ROM
    _myTZX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
    _myTZX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
    _myTZX.descriptor[currentBlock].timming.sync_2 = DSYNC2;
    _myTZX.descriptor[currentBlock].timming.bit_0 = DBIT_0;
    _myTZX.descriptor[currentBlock].timming.bit_1 = DBIT_1;

    // Obtenemos el "pause despues del bloque"
    // BYTE 0x00 y 0x01
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 1);

    // ////SerialHW.println("");
    // ////SerialHW.println("Pause after block: " +
    // String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)); Obtenemos
    // el "tamaño de los datos" BYTE 0x02 y 0x03
    _myTZX.descriptor[currentBlock].lengthOfData =
        getWORD(mFile, currentOffset + 3);

    // ////SerialHW.println("");
    // ////SerialHW.println("Length of data: ");
    // ////SerialHW.print(_myTZX.descriptor[currentBlock].lengthOfData,HEX);

    // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
    _myTZX.descriptor[currentBlock].offsetData = currentOffset + 5;

    // ////SerialHW.println("");
    // ////SerialHW.println("Offset of data: ");
    // ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);

    // Vamos a verificar el flagByte
    int flagByte = getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData);
    // Y cogemos para ver el tipo de cabecera
    int typeBlock =
        getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData + 1);

    // Si el flagbyte es menor a 0x80
    if (flagByte < 128) {

      _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_HEADER;

      // Es una cabecera
      // 0 - program
      // 1 - Number array
      // 2 - Char array
      // 3 - Code file
      if (typeBlock == 0) {
        // Es una cabecera PROGRAM: BASIC
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 0;

        // if (!PROGRAM_NAME_DETECTED)
        // {
        uint8_t *block = (uint8_t *)ps_calloc(19 + 1, sizeof(uint8_t));
        getBlock(mFile, block, _myTZX.descriptor[currentBlock].offsetData, 19);
        gName = getNameFromStandardBlock(block);
        PROGRAM_NAME = String(gName);
        LAST_PROGRAM_NAME = PROGRAM_NAME;
        PROGRAM_NAME_DETECTED = true;
        free(block);
        strncpy(_myTZX.descriptor[currentBlock].name, gName, 14);
        // }
        // else
        // {
        //   strncpy(_myTZX.descriptor[currentBlock].name,_myTZX.name,14);
        // }

        // Almacenamos el nombre del bloque
        //
      } else if (typeBlock == 1) {
        // Number ARRAY
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 2;
      } else if (typeBlock == 2) {
        // Char ARRAY
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 4;
      } else if (typeBlock == 3) {
        // BYTE o SCREEN (CODE)
        if (_myTZX.descriptor[currentBlock].lengthOfData == 6914) {

          _myTZX.descriptor[currentBlock].screen = true;
          _myTZX.descriptor[currentBlock].type = 7;

          // Almacenamos el nombre del bloque
          uint8_t *block = (uint8_t *)ps_calloc(19 + 1, sizeof(uint8_t));
          getBlock(mFile, block, _myTZX.descriptor[currentBlock].offsetData,
                   19);
          gName = getNameFromStandardBlock(block);
          strncpy(_myTZX.descriptor[currentBlock].name, gName, 14);
          free(block);
        } else {
          // Es un bloque BYTE
          _myTZX.descriptor[currentBlock].type = 1;

          // Almacenamos el nombre del bloque
          uint8_t *block = (uint8_t *)ps_calloc(19 + 1, sizeof(uint8_t));
          getBlock(mFile, block, _myTZX.descriptor[currentBlock].offsetData,
                   19);
          gName = getNameFromStandardBlock(block);
          strncpy(_myTZX.descriptor[currentBlock].name, gName, 14);
          free(block);
        }
      }
    } else {
      _myTZX.descriptor[currentBlock].timming.pilot_num_pulses = DPULSES_DATA;

      if (typeBlock == 3) {
        // SCREEN
        if (_myTZX.descriptor[currentBlock].lengthOfData == 6914) {
          _myTZX.descriptor[currentBlock].screen = true;
          _myTZX.descriptor[currentBlock].type = 7;
        } else {
          // Es un bloque BYTE
          _myTZX.descriptor[currentBlock].type = 1;
        }
      } else {
        // Es un bloque BYTE
        _myTZX.descriptor[currentBlock].type = 1;
      }
    }

    // No contamos el ID
    // La cabecera tiene 4 bytes de parametros y N bytes de datos
    // pero para saber el total de bytes de datos hay que analizar el TAP
    // int positionOfTAPblock = currentOffset + 4;
    // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);

    // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
    _myTZX.descriptor[currentBlock].size =
        _myTZX.descriptor[currentBlock].lengthOfData;

    // Por defecto en ID10 no tiene MASK_LAST_BYTE pero lo inicializamos
    // para evitar problemas con el ID11
    _myTZX.descriptor[currentBlock].hasMaskLastByte = false;
  }

  void analyzeID17(File mFile, int currentOffset, int currentBlock) {
    char *gName;

    // Turbo data

    _myTZX.descriptor[currentBlock].ID = 17;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // Entonces
    // Timming de PULSE PILOT
    _myTZX.descriptor[currentBlock].timming.pilot_len =
        getWORD(mFile, currentOffset + 1);

    ////SerialHW.println("");
    ////SerialHW.print("PULSE PILOT=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.pulse_pilot,
    /// HEX);

    // Timming de SYNC_1
    _myTZX.descriptor[currentBlock].timming.sync_1 =
        getWORD(mFile, currentOffset + 3);

    //////SerialHW.println("");
    ////SerialHW.print(",SYNC1=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_1, HEX);

    // Timming de SYNC_2
    _myTZX.descriptor[currentBlock].timming.sync_2 =
        getWORD(mFile, currentOffset + 5);

    //////SerialHW.println("");
    ////SerialHW.print(",SYNC2=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.sync_2, HEX);

    // Timming de BIT 0
    _myTZX.descriptor[currentBlock].timming.bit_0 =
        getWORD(mFile, currentOffset + 7);

    //////SerialHW.println("");
    ////SerialHW.print(",BIT_0=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_0, HEX);

    // Timming de BIT 1
    _myTZX.descriptor[currentBlock].timming.bit_1 =
        getWORD(mFile, currentOffset + 9);

    //////SerialHW.println("");
    ////SerialHW.print(",BIT_1=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_1, HEX);

    // Timming de PILOT TONE
    _myTZX.descriptor[currentBlock].timming.pilot_num_pulses =
        getWORD(mFile, currentOffset + 11);

    //////SerialHW.println("");
    ////SerialHW.print(",PILOT TONE=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses,
    /// HEX);

    // Cogemos el byte de bits of the last byte
    _myTZX.descriptor[currentBlock].maskLastByte =
        getBYTE(mFile, currentOffset + 13);
    _myTZX.descriptor[currentBlock].hasMaskLastByte = true;

    //////SerialHW.println("");
    ////SerialHW.print(",MASK LAST BYTE=");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].maskLastByte, HEX);

    // ********************************************************************
    //
    //
    //
    //
    //                 Ahora analizamos el BLOQUE DE DATOS
    //
    //
    //
    //
    // ********************************************************************

    // Obtenemos el "pause despues del bloque"
    // BYTE 0x00 y 0x01
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 14);

    ////SerialHW.println("");
    ////SerialHW.print("Pause after block: " +
    /// String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
    ////SerialHW.println("");
    // Obtenemos el "tamaño de los datos"
    // BYTE 0x02 y 0x03
    _myTZX.descriptor[currentBlock].lengthOfData =
        getNBYTE(mFile, currentOffset + 16, 3);

    ////SerialHW.println("");
    ////SerialHW.print("Length of data: ");
    ////SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));
    ////SerialHW.println("");

    // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
    _myTZX.descriptor[currentBlock].offsetData = currentOffset + 19;

    // Almacenamos el nombre del bloque
    //_myTZX.descriptor[currentBlock].name =
    // getNameFromStandardBlock(getBlock(mFile,_myTZX.descriptor[currentBlock].offsetData,19));

    ////SerialHW.println("");
    ////SerialHW.print("Offset of data: 0x");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
    ////SerialHW.println("");

    // Vamos a verificar el flagByte
    int flagByte = getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData);

    // ******************************************************************************
    // CORRECCIÓN SPEEDLOCK: Bloques cortos de protección
    // ******************************************************************************
    // Los bloques Speedlock y otras protecciones usan bloques ID 0x11 muy
    // cortos (típicamente 1-3 bytes con solo unos pocos bits usados).
    //
    // Si lengthOfData < 19 bytes, NO podemos asumir que es una cabecera
    // estándar ZX Spectrum porque:
    // 1. No hay suficientes bytes para formar una cabecera válida (19 bytes)
    // 2. El segundo byte (typeBlock) podría estar fuera del bloque de datos
    // 3. Leer 19 bytes corruniría datos de bloques siguientes
    //
    // En estos casos, tratamos el bloque como datos de protección (type=4)
    // independientemente del valor del flagByte.
    // ******************************************************************************

    if (_myTZX.descriptor[currentBlock].lengthOfData < 19) {
      // Bloque corto - típico de protecciones como Speedlock
      // No intentar interpretar como cabecera estándar
      strncpy(_myTZX.descriptor[currentBlock].name, "              ", 14);
      _myTZX.descriptor[currentBlock].type =
          4; // Tratar como datos de protección
      _myTZX.descriptor[currentBlock].header = false;
    } else if (flagByte < 128) {
      // Bloque largo con flagByte < 128: posible cabecera estándar
      int typeBlock =
          getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData + 1);

      // Es una cabecera
      uint8_t *block = (uint8_t *)ps_calloc(19 + 1, sizeof(uint8_t));
      getBlock(mFile, block, _myTZX.descriptor[currentBlock].offsetData, 19);
      gName = getNameFromStandardBlock(block);
      strncpy(_myTZX.descriptor[currentBlock].name, gName, 14);
      free(block);

      if (typeBlock == 0) {
        // Es una cabecera BASIC
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 0;

      } else if (typeBlock == 1) {
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 0;
      } else if (typeBlock == 2) {
        _myTZX.descriptor[currentBlock].header = true;
        _myTZX.descriptor[currentBlock].type = 0;
      } else if (typeBlock == 3) {
        if (_myTZX.descriptor[currentBlock].lengthOfData == 6914) {
          _myTZX.descriptor[currentBlock].screen = true;
          _myTZX.descriptor[currentBlock].type = 7;
        } else {
          // Es un bloque BYTE
          _myTZX.descriptor[currentBlock].type = 1;
        }
      } else {
        // typeBlock no reconocido, tratar como datos
        _myTZX.descriptor[currentBlock].type = 4;
      }
    } else {
      // flagByte >= 128: Es un bloque de datos
      strncpy(_myTZX.descriptor[currentBlock].name, "              ", 14);
      _myTZX.descriptor[currentBlock].type = 4;
    }

    // No contamos el ID
    // La cabecera tiene 4 bytes de parametros y N bytes de datos
    // pero para saber el total de bytes de datos hay que analizar el TAP
    // int positionOfTAPblock = currentOffset + 4;
    // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);

    // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
    _myTZX.descriptor[currentBlock].size =
        _myTZX.descriptor[currentBlock].lengthOfData;
  }

  void analyzeID18(File mFile, int currentOffset, int currentBlock) {
    // ID-12 - Pure tone

    _myTZX.descriptor[currentBlock].ID = 18;
    _myTZX.descriptor[currentBlock].playeable = true;
    //_myTZX.descriptor[currentBlock].forSetting = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;
    _myTZX.descriptor[currentBlock].timming.pure_tone_len =
        getWORD(mFile, currentOffset + 1);
    _myTZX.descriptor[currentBlock].timming.pure_tone_num_pulses =
        getWORD(mFile, currentOffset + 3);

#ifdef DEBUGMODE
    SerialHW.println("");
    SerialHW.print("Pure tone setting: Ts: ");
    SerialHW.print(_myTZX.descriptor[currentBlock].timming.pure_tone_len, HEX);
    SerialHW.print(" / Np: ");
    SerialHW.print(_myTZX.descriptor[currentBlock].timming.pure_tone_num_pulses,
                   HEX);
    SerialHW.println("");
#endif

    // Esto es para que tome los bloques como especiales
    _myTZX.descriptor[currentBlock].type = 99;
    // Guardamos el size
    _myTZX.descriptor[currentBlock].size = 4;
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock = 0.0;
    _myTZX.descriptor[currentBlock].silent = 0;
    //
  }

  void analyzeID19(File mFile, int currentOffset, int currentBlock) {
    // ID-13 - Pulse sequence

    _myTZX.descriptor[currentBlock].ID = 19;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    int num_pulses = getBYTE(mFile, currentOffset + 1);
    _myTZX.descriptor[currentBlock].timming.pulse_seq_num_pulses = num_pulses;

    // Reservamos memoria.
    _myTZX.descriptor[currentBlock].timming.pulse_seq_array =
        (int *)ps_calloc(num_pulses + 1, sizeof(int));

    // Tomamos ahora las longitudes
    int coff = currentOffset + 2;

#ifdef DEBUGMODE
    log("ID-13: Pulse sequence");
#endif

    for (int i = 0; i < num_pulses; i++) {
      int lenPulse = getWORD(mFile, coff);
      _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] = lenPulse;

#ifdef DEBUGMODE
      SerialHW.print("[" + String(i) + "]=0x");
      SerialHW.print(lenPulse, HEX);
      SerialHW.print("; ");
#endif

      // Adelantamos 2 bytes
      coff += 2;
    }

    ////SerialHW.println("");

    // Esto es para que tome los bloques como especiales
    _myTZX.descriptor[currentBlock].type = 99;
    // Guardamos el size (bytes)
    _myTZX.descriptor[currentBlock].size = (num_pulses * 2) + 1;
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock = 0;
    _myTZX.descriptor[currentBlock].silent = 0;
    //
  }

  void analyzeID20(File mFile, int currentOffset, int currentBlock) {
    // ID-14 - Pure Data Block

    _myTZX.descriptor[currentBlock].ID = 20;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // Timming de la ROM
    _myTZX.descriptor[currentBlock].timming.pilot_len = DPILOT_LEN;
    _myTZX.descriptor[currentBlock].timming.sync_1 = DSYNC1;
    _myTZX.descriptor[currentBlock].timming.sync_2 = DSYNC2;

    // Timming de BIT 0
    _myTZX.descriptor[currentBlock].timming.bit_0 =
        getWORD(mFile, currentOffset + 1);

    //////SerialHW.println("");
    // //SerialHW.print(",BIT_0=");
    // //SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_0, HEX);

    // Timming de BIT 1
    _myTZX.descriptor[currentBlock].timming.bit_1 =
        getWORD(mFile, currentOffset + 3);

    //////SerialHW.println("");
    // //SerialHW.print(",BIT_1=");
    // //SerialHW.print(_myTZX.descriptor[currentBlock].timming.bit_1, HEX);

    // Cogemos el byte de bits of the last byte
    _myTZX.descriptor[currentBlock].maskLastByte =
        getBYTE(mFile, currentOffset + 5);
    _myTZX.descriptor[currentBlock].hasMaskLastByte = true;
    //_zxp.set_maskLastByte(_myTZX.descriptor[currentBlock].maskLastByte);

    // //SerialHW.println("");
    // //SerialHW.print(",MASK LAST BYTE=");
    // //SerialHW.print(_myTZX.descriptor[currentBlock].maskLastByte, HEX);

    // ********************************************************************
    //
    //
    //
    //
    //                 Ahora analizamos el BLOQUE DE DATOS
    //
    //
    //
    //
    // ********************************************************************

    // Obtenemos el "pause despues del bloque"
    // BYTE 0x00 y 0x01
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 6);

    // //SerialHW.println("");
    // //SerialHW.print("Pause after block: " +
    // String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
    // //SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
    // //SerialHW.println("");

    // Obtenemos el "tamaño de los datos"
    // BYTE 0x02 y 0x03
    _myTZX.descriptor[currentBlock].lengthOfData =
        getNBYTE(mFile, currentOffset + 8, 3);

    // //SerialHW.println("");
    // //SerialHW.println("Length of data: ");
    // //SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));

    // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
    _myTZX.descriptor[currentBlock].offsetData = currentOffset + 11;

    ////SerialHW.println("");
    ////SerialHW.print("Offset of data: 0x");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
    ////SerialHW.println("");

    // Vamos a verificar el flagByte

    int flagByte = getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData);
    int typeBlock =
        getBYTE(mFile, _myTZX.descriptor[currentBlock].offsetData + 1);
    //
    //_myTZX.descriptor[currentBlock].name = &INITCHAR[0];
    strncpy(_myTZX.descriptor[currentBlock].name, "              ", 14);
    //_myTZX.descriptor[currentBlock].type = 4;
    _myTZX.descriptor[currentBlock].type = 99; // Importante
    _myTZX.descriptor[currentBlock].header = false;

    // No contamos el ID
    // La cabecera tiene 4 bytes de parametros y N bytes de datos
    // pero para saber el total de bytes de datos hay que analizar el TAP
    // int positionOfTAPblock = currentOffset + 4;
    // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);

    // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
    _myTZX.descriptor[currentBlock].size =
        _myTZX.descriptor[currentBlock].lengthOfData;
  }

  void analyzeID21(File mFile, int currentOffset, int currentBlock) {
    // ID-15 - Direct recording
    _myTZX.descriptor[currentBlock].ID = 21;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // Obtenemos el "pause despues del bloque"
    // BYTE 0x00 y 0x01
    _myTZX.descriptor[currentBlock].samplingRate =
        (double)getWORD(mFile, currentOffset + 1);
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        getWORD(mFile, currentOffset + 3);
    // Esto es muy importante para el ID 0x15
    // Used bits (samples) in last byte of data (1-8) (e.g. if this is 2, only
    // first two samples of the last byte will be played)
    _myTZX.descriptor[currentBlock].hasMaskLastByte = true;
    _myTZX.descriptor[currentBlock].maskLastByte =
        getBYTE(mFile, currentOffset + 5);

    // //SerialHW.println("");
    // //SerialHW.print("Pause after block: " +
    // String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock)+ " - 0x");
    // //SerialHW.print(_myTZX.descriptor[currentBlock].pauseAfterThisBlock,HEX);
    // //SerialHW.println("");

    // Obtenemos el "tamaño de los datos"
    // BYTE 0x02 y 0x03
    _myTZX.descriptor[currentBlock].lengthOfData =
        getNBYTE(mFile, currentOffset + 6, 3);

    // //SerialHW.println("");
    // //SerialHW.println("Length of data: ");
    // //SerialHW.print(String(_myTZX.descriptor[currentBlock].lengthOfData));

    // Los datos (TAP) empiezan en 0x04. Posición del bloque de datos.
    _myTZX.descriptor[currentBlock].offsetData = currentOffset + 9;

    ////SerialHW.println("");
    ////SerialHW.print("Offset of data: 0x");
    ////SerialHW.print(_myTZX.descriptor[currentBlock].offsetData,HEX);
    ////SerialHW.println("");

    // Vamos a verificar el flagByte

    // int flagByte = getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData);
    // int typeBlock =
    // getBYTE(mFile,_myTZX.descriptor[currentBlock].offsetData+1);
    //
    //_myTZX.descriptor[currentBlock].name = &INITCHAR[0];
    // strncpy(_myTZX.descriptor[currentBlock].name,"              ",14);
    //_myTZX.descriptor[currentBlock].type = 4;
    _myTZX.descriptor[currentBlock].type = 0; // Importante
    _myTZX.descriptor[currentBlock].header = false;

    // No contamos el ID
    // La cabecera tiene 4 bytes de parametros y N bytes de datos
    // pero para saber el total de bytes de datos hay que analizar el TAP
    // int positionOfTAPblock = currentOffset + 4;
    // dataTAPsize = getWORD(mFile,positionOfTAPblock + headerTAPsize + 1);

    // NOTA: Sumamos 2 bytes que son la DWORD que indica el dataTAPsize
    _myTZX.descriptor[currentBlock].size =
        _myTZX.descriptor[currentBlock].lengthOfData + 8 + 1;
  }

  void analyzeID24(File mFile, int currentOffset, int currentBlock) {
    logln("ID-18: CSW Recording Block v2");
    logln("------------------------------");
    _myTZX.descriptor[currentBlock].ID = 24;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;
    _myTZX.descriptor[currentBlock].type = 99; // Tipo especial

    // 1. LEER LA CABECERA DEL BLOQUE CSW
    _myTZX.descriptor[currentBlock].size =
        getNBYTE(mFile, currentOffset + 1, 4);
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 5);
    _myTZX.descriptor[currentBlock].timming.csw_sampling_rate =
        getNBYTE(mFile, currentOffset + 7, 3);
    _myTZX.descriptor[currentBlock].timming.csw_compression_type =
        getBYTE(mFile, currentOffset + 10);

    logln("  - Block Size: " + String(_myTZX.descriptor[currentBlock].size));
    logln("  - Pause After: " +
          String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock) + " ms");
    logln("  - Sampling Rate: " +
          String(_myTZX.descriptor[currentBlock].timming.csw_sampling_rate) +
          " Hz");
    logln(
        "  - Compression: " +
        String(_myTZX.descriptor[currentBlock].timming.csw_compression_type == 1
                   ? "RLE"
                   : "Z-RLE"));

    int dataOffset = currentOffset + 16;
    int compressedDataSize = _myTZX.descriptor[currentBlock].size - 15;
    uint8_t *rleData = nullptr;
    size_t rleDataSize = 0;
    bool zrle_fallback_needed = false; // ✅ Flag para controlar el fallback

    // 2. GESTIONAR LA COMPRESIÓN
    if (_myTZX.descriptor[currentBlock].timming.csw_compression_type ==
        2) // Z-RLE
    {
      logln("  - Z-RLE detected. Attempting decompression with miniz...");
      uint8_t *zrle_buffer = (uint8_t *)ps_malloc(compressedDataSize);
      if (!zrle_buffer) {
        logln("ERROR: Failed to alloc Z-RLE buffer");
        return;
      }
      mFile.seek(dataOffset);
      mFile.read(zrle_buffer, compressedDataSize);

      mz_ulong decompressed_size = compressedDataSize * 10; // Estimación segura
      rleData = (uint8_t *)ps_malloc(decompressed_size);

      if (rleData) {
        int result = mz_uncompress(rleData, &decompressed_size, zrle_buffer,
                                   compressedDataSize);

        if (result == MZ_OK) {
          rleDataSize = decompressed_size;
          logln("  - Decompressed to " + String(rleDataSize) +
                " bytes of RLE data.");
          uint8_t *final_rle_data = (uint8_t *)ps_realloc(rleData, rleDataSize);
          if (final_rle_data)
            rleData = final_rle_data;
        } else {
          logln("ERROR: miniz decompression failed with code: " +
                String(result));
          free(rleData);
          rleData = nullptr;
          // ✅ Si el error es de datos, activamos el fallback a RLE simple
          if (result == MZ_DATA_ERROR) {
            logln("WARNING: Data error suggests this is not a valid ZLIB "
                  "stream. Falling back to RLE mode.");
            zrle_fallback_needed = true;
          }
        }
      } else {
        logln("ERROR: Failed to alloc initial RLE buffer");
      }
      free(zrle_buffer);
    }

    // Si es RLE (Tipo 1) o si el Z-RLE falló y necesita fallback
    if (_myTZX.descriptor[currentBlock].timming.csw_compression_type == 1 ||
        zrle_fallback_needed) {
      if (zrle_fallback_needed) {
        logln("  - Executing fallback: Reading data as plain RLE.");
        _myTZX.descriptor[currentBlock].playeable = false;
      } else {
        logln("  - RLE detected. Reading directly.");
      }
      rleDataSize = compressedDataSize;
      rleData = (uint8_t *)ps_malloc(rleDataSize);
      if (!rleData) {
        logln("ERROR: Failed to alloc RLE buffer");
        return;
      }
      mFile.seek(dataOffset);
      mFile.read(rleData, rleDataSize);
    }

    if (!rleData || rleDataSize == 0) {
      logln("ERROR: No RLE data to process.");
      if (rleData)
        free(rleData);
      return;
    }

    // 3. PARSEAR LOS DATOS RLE Y CREAR LA SECUENCIA DE PULSOS
    // (El resto de la función no necesita cambios)
    // ... (código de parseo de RLE existente) ...
    int pulseCount = 0;
    uint8_t last_pulse = 0;
    for (size_t i = 0; i < rleDataSize;) {
      pulseCount++;
      uint8_t value = rleData[i];
      if (value == 0x00) {
        i += 2;
      } else {
        last_pulse = value;
        i += 1;
      }
    }
    logln("  - Found " + String(pulseCount) + " RLE pulse sequences.");

    if (pulseCount > 0) {
      _myTZX.descriptor[currentBlock].timming.csw_num_pulses = pulseCount;
      _myTZX.descriptor[currentBlock].timming.csw_pulse_data =
          (tRlePulse *)ps_calloc(pulseCount, sizeof(tRlePulse));

      if (!_myTZX.descriptor[currentBlock].timming.csw_pulse_data) {
        logln("ERROR: Failed to alloc RLE pulse array");
        free(rleData);
        return;
      }

      int currentPulse = 0;
      last_pulse = 0;
      for (size_t i = 0; i < rleDataSize && currentPulse < pulseCount;) {
        uint8_t value = rleData[i];
        if (value == 0x00) {
          uint8_t repeat = rleData[i + 1];
          _myTZX.descriptor[currentBlock]
              .timming.csw_pulse_data[currentPulse]
              .pulse_len = last_pulse;
          _myTZX.descriptor[currentBlock]
              .timming.csw_pulse_data[currentPulse]
              .repeat = repeat;
          i += 2;
        } else {
          last_pulse = value;
          _myTZX.descriptor[currentBlock]
              .timming.csw_pulse_data[currentPulse]
              .pulse_len = last_pulse;
          _myTZX.descriptor[currentBlock]
              .timming.csw_pulse_data[currentPulse]
              .repeat = 1;
          i += 1;
        }
        currentPulse++;
      }
    }

    free(rleData);
  }

  void analyzeID25(File mFile, int currentOffset, int currentBlock) {
    // ID 19 - Generalized Data Block
    // Estructura del bloque (offsets relativos al ID):
    // +0:     ID (0x19)
    // +1-4:   Block length (DWORD)
    // +5-6:   Pause (WORD)
    // +7-10:  TOTP (DWORD)
    // +11:    NPP (BYTE)
    // +12:    ASP (BYTE)
    // +13-16: TOTD (DWORD)
    // +17:    NPD (BYTE)
    // +18:    ASD (BYTE)
    // +19+:   SYMDEF[], PRLE[], SYMDEF[], Data stream

    _myTZX.descriptor[currentBlock].ID = 25; // 0x19 en decimal
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // Leer block length (DWORD) - está en currentOffset+1 (después del byte ID)
    uint32_t blockLength = getDWORD(mFile, currentOffset + 1);
    _myTZX.descriptor[currentBlock].size = blockLength;

    // Calcular límite máximo del offset para evitar lecturas fuera del bloque
    // El bloque termina en: currentOffset + 1 (ID) + 4 (length field) +
    // blockLength
    int maxOffset = currentOffset + 1 + 4 + blockLength;

    // Leer pause (WORD) - offset +5 desde el ID
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 5);

    // Leer TOTP (DWORD) - offset +7
    _myTZX.descriptor[currentBlock].symbol.TOTP =
        getDWORD(mFile, currentOffset + 7);

    // Leer NPP (BYTE) - offset +11
    _myTZX.descriptor[currentBlock].symbol.NPP =
        getBYTE(mFile, currentOffset + 11);

    // Leer ASP (BYTE) - offset +12
    _myTZX.descriptor[currentBlock].symbol.ASP =
        getBYTE(mFile, currentOffset + 12);
    if (_myTZX.descriptor[currentBlock].symbol.ASP == 0)
      _myTZX.descriptor[currentBlock].symbol.ASP = 256;

    // Leer TOTD (DWORD) - offset +13
    _myTZX.descriptor[currentBlock].symbol.TOTD =
        getDWORD(mFile, currentOffset + 13);

    // Leer NPD (BYTE) - offset +17
    _myTZX.descriptor[currentBlock].symbol.NPD =
        getBYTE(mFile, currentOffset + 17);

    // Leer ASD (BYTE) - offset +18
    _myTZX.descriptor[currentBlock].symbol.ASD =
        getBYTE(mFile, currentOffset + 18);
    if (_myTZX.descriptor[currentBlock].symbol.ASD == 0)
      _myTZX.descriptor[currentBlock].symbol.ASD = 256;

    // Los datos variables empiezan en offset +19
    int offset = currentOffset + 19;

    // Leer SYMDEF para pilot/sync si TOTP > 0
    if (_myTZX.descriptor[currentBlock].symbol.TOTP > 0) {
      _myTZX.descriptor[currentBlock].symbol.symDefPilot = (tSymDef *)ps_calloc(
          _myTZX.descriptor[currentBlock].symbol.ASP, sizeof(tSymDef));
      if (_myTZX.descriptor[currentBlock].symbol.symDefPilot == NULL) {
        SerialHW.println("Error: Failed to allocate symDefPilot");
        return;
      }
      for (int i = 0; i < _myTZX.descriptor[currentBlock].symbol.ASP; i++) {
        if (offset + 1 > maxOffset) {
          SerialHW.println(
              "Error: Offset out of bounds in symDefPilot symbolFlag");
          return;
        }
        _myTZX.descriptor[currentBlock].symbol.symDefPilot[i].symbolFlag =
            getBYTE(mFile, offset);
        offset += 1;
        _myTZX.descriptor[currentBlock].symbol.symDefPilot[i].pulse_array =
            (int *)ps_calloc(_myTZX.descriptor[currentBlock].symbol.NPP,
                             sizeof(int));
        if (_myTZX.descriptor[currentBlock].symbol.symDefPilot[i].pulse_array ==
            NULL) {
          SerialHW.println(
              "Error: Failed to allocate pulse_array for symDefPilot");
          return;
        }
        for (int j = 0; j < _myTZX.descriptor[currentBlock].symbol.NPP; j++) {
          if (offset + 2 > maxOffset) {
            SerialHW.println(
                "Error: Offset out of bounds in symDefPilot pulse_array");
            return;
          }
          _myTZX.descriptor[currentBlock].symbol.symDefPilot[i].pulse_array[j] =
              getWORD(mFile, offset);
          offset += 2;
        }
      }

      // Leer PRLE para pilot/sync
      _myTZX.descriptor[currentBlock].symbol.pilotStream = (tPrle *)ps_calloc(
          _myTZX.descriptor[currentBlock].symbol.TOTP, sizeof(tPrle));
      if (_myTZX.descriptor[currentBlock].symbol.pilotStream == NULL) {
        SerialHW.println("Error: Failed to allocate pilotStream");
        return;
      }
      for (int i = 0; i < _myTZX.descriptor[currentBlock].symbol.TOTP; i++) {
        if (offset + 1 > maxOffset) {
          SerialHW.println("Error: Offset out of bounds in pilotStream symbol");
          return;
        }
        _myTZX.descriptor[currentBlock].symbol.pilotStream[i].symbol =
            getBYTE(mFile, offset);
        offset += 1;
        if (offset + 2 > maxOffset) {
          SerialHW.println("Error: Offset out of bounds in pilotStream repeat");
          return;
        }
        _myTZX.descriptor[currentBlock].symbol.pilotStream[i].repeat =
            getWORD(mFile, offset);
        offset += 2;
      }
    }

    // Leer SYMDEF para data
    _myTZX.descriptor[currentBlock].symbol.symDefData = (tSymDef *)ps_calloc(
        _myTZX.descriptor[currentBlock].symbol.ASD, sizeof(tSymDef));
    if (_myTZX.descriptor[currentBlock].symbol.symDefData == NULL) {
      SerialHW.println("Error: Failed to allocate symDefData");
      return;
    }
    for (int i = 0; i < _myTZX.descriptor[currentBlock].symbol.ASD; i++) {
      if (offset + 1 > maxOffset) {
        SerialHW.println(
            "Error: Offset out of bounds in symDefData symbolFlag");
        return;
      }
      _myTZX.descriptor[currentBlock].symbol.symDefData[i].symbolFlag =
          getBYTE(mFile, offset);
      offset += 1;
      _myTZX.descriptor[currentBlock].symbol.symDefData[i].pulse_array =
          (int *)ps_calloc(_myTZX.descriptor[currentBlock].symbol.NPD,
                           sizeof(int));
      if (_myTZX.descriptor[currentBlock].symbol.symDefData[i].pulse_array ==
          NULL) {
        SerialHW.println(
            "Error: Failed to allocate pulse_array for symDefData");
        return;
      }
      for (int j = 0; j < _myTZX.descriptor[currentBlock].symbol.NPD; j++) {
        if (offset + 2 > maxOffset) {
          SerialHW.println(
              "Error: Offset out of bounds in symDefData pulse_array");
          return;
        }
        _myTZX.descriptor[currentBlock].symbol.symDefData[i].pulse_array[j] =
            getWORD(mFile, offset);
        offset += 2;
      }
    }

    // Calcular NB = ceil(Log2(ASD))
    int NB = 0;
    int temp = _myTZX.descriptor[currentBlock].symbol.ASD;
    while (temp > 1) {
      temp >>= 1;
      NB++;
    }
    if ((1 << NB) < _myTZX.descriptor[currentBlock].symbol.ASD)
      NB++;

    // Calcular DS = ceil(NB * TOTD / 8)
    int DS = ((NB * _myTZX.descriptor[currentBlock].symbol.TOTD) + 7) / 8;

    // Leer data stream
    _myTZX.descriptor[currentBlock].symbol.dataStream =
        (uint8_t *)ps_calloc(DS, sizeof(uint8_t));
    if (_myTZX.descriptor[currentBlock].symbol.dataStream == NULL) {
      SerialHW.println("Error: Failed to allocate dataStream");
      return;
    }
    // The data stream is at the end of the block
    // El bloque termina en: currentOffset + 1 + 4 + blockLength
    // El dataStream ocupa los últimos DS bytes del bloque
    int dataStreamOffset = currentOffset + 1 + 4 + blockLength - DS;
    for (int i = 0; i < DS; i++) {
      _myTZX.descriptor[currentBlock].symbol.dataStream[i] =
          getBYTE(mFile, dataStreamOffset + i);
    }

    // Guardar offsets
    // Los SYMDEF/PRLE empiezan en offset +19
    _myTZX.descriptor[currentBlock].symbol.offsetPilotDataStream =
        currentOffset + 19;
    _myTZX.descriptor[currentBlock].symbol.offsetDataStream = dataStreamOffset;

    // Esto es para que tome los bloques como especiales
    _myTZX.descriptor[currentBlock].type = 99;
    _myTZX.descriptor[currentBlock].silent = 0;
  }

  void analyzeID32(File mFile, int currentOffset, int currentBlock) {
    // Pause or STOP the TAPE

    _myTZX.descriptor[currentBlock].ID = 32;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // Obtenemos el valor de la pausa
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        getWORD(mFile, currentOffset + 1);
  }

  void analyzeID40(File mFile, int currentOffset, int currentBlock) {
    // Size block
    int sizeBlock = 0;
    int numSelections = 0;

    // Obtenemos la dirección del siguiente offset
    _myTZX.descriptor[currentBlock].ID = 40;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset + 2;

    sizeBlock = getWORD(mFile, currentOffset + 1);
    numSelections = getBYTE(mFile, currentOffset + 3);

    _myTZX.descriptor[currentBlock].size = sizeBlock + 2;
  }

  void analyzeID48(File mFile, int currentOffset, int currentBlock) {
    // 0x30 - Text Description
    int sizeTextInformation = 0;

    _myTZX.descriptor[currentBlock].ID = 48;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    sizeTextInformation = getBYTE(mFile, currentOffset + 1);

    ////SerialHW.println("");
    ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));

    // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
    // el bloque comienza en el offset del ID y acaba en
    // offset[ID] + tamaño_bloque
    _myTZX.descriptor[currentBlock].size = sizeTextInformation + 1;
  }

  void analyzeID49(File mFile, int currentOffset, int currentBlock) {
    // 0x31 - Message block
    int sizeTextInformation = 0;

    _myTZX.descriptor[currentBlock].ID = 49;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    sizeTextInformation = getBYTE(mFile, currentOffset + 2);

    ////SerialHW.println("");
    ////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));

    // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
    // el bloque comienza en el offset del ID y acaba en
    // offset[ID] + tamaño_bloque
    _myTZX.descriptor[currentBlock].size = sizeTextInformation + 1;
  }

  void analyzeID50(File mFile, int currentOffset, int currentBlock) {
    // ID 0x32 - Archive Info
    int sizeBlock = 0;

    _myTZX.descriptor[currentBlock].ID = 50;
    _myTZX.descriptor[currentBlock].playeable = false;
    // Los dos primeros bytes del bloque no se cuentan para
    // el tamaño total
    _myTZX.descriptor[currentBlock].offset = currentOffset + 3;

    sizeBlock = getWORD(mFile, currentOffset + 1);

    //////SerialHW.println("");
    //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));

    // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
    // el bloque comienza en el offset del ID y acaba en
    // offset[ID] + tamaño_bloque
    _myTZX.descriptor[currentBlock].size = sizeBlock;
  }

  void analyzeID51(File mFile, int currentOffset, int currentBlock) {
    // 0x33 - Hardware Information block
    int sizeBlock = 0;

    _myTZX.descriptor[currentBlock].ID = 51;
    _myTZX.descriptor[currentBlock].playeable = false;
    // Los dos primeros bytes del bloque no se cuentan para
    // el tamaño total
    _myTZX.descriptor[currentBlock].offset = currentOffset;

    // El bloque completo mide, el numero de maquinas a listar
    // multiplicado por 3 byte por maquina listada
    sizeBlock = getBYTE(mFile, currentOffset + 1) * 3;

    //////SerialHW.println("");
    //////SerialHW.println("ID48 - TextSize: " + String(sizeTextInformation));

    // El tamaño del bloque es "1 byte de longitud de texto + TAMAÑO_TEXTO"
    // el bloque comienza en el offset del ID y acaba en
    // offset[ID] + tamaño_bloque
    _myTZX.descriptor[currentBlock].size = sizeBlock - 1;
  }

  void analyzeID33(File mFile, int currentOffset, int currentBlock) {
    // Group start
    int sizeTextInformation = 0;

    _myTZX.descriptor[currentBlock].ID = 33;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset + 2;
    _myTZX.descriptor[currentBlock].group = MULTIGROUP_COUNT;
    MULTIGROUP_COUNT++;

    // Tomamos el tamaño del nombre del Grupo
    sizeTextInformation = getBYTE(mFile, currentOffset + 1);
    _myTZX.descriptor[currentBlock].size = sizeTextInformation;

    // Ahora cogemos el texto en el siguiente byte
    // #ifdef DEBUGMODE
    logln("ID33 - sizeTextInformation = " + String(sizeTextInformation) +
          " bytes");
    // #endif

    uint8_t *grpN =
        (uint8_t *)ps_calloc(sizeTextInformation + 5, sizeof(uint8_t));
    readFileRange(mFile, grpN, currentOffset + 2, sizeTextInformation, false);
    char groupName[32];
    // Limpiamos de basura todo el buffer
    strcpy(groupName, "                             ");
    strncpy(groupName, (char *)grpN, 30);

    // for(int i=0;i<sizeTextInformation-1;i++)
    // {
    //     groupName[i] = (char)grpN[i];
    // }

    // logln("Group name: " + String(groupName));

    // Cogemos solo 29 letras
    if (sizeTextInformation < 30) {
      strncpy(_myTZX.descriptor[currentBlock].name, groupName,
              sizeTextInformation);
    } else {
      strncpy(_myTZX.descriptor[currentBlock].name, groupName, 29);
    }

    logln("ID33 - Group start: " + String(groupName));
    free(grpN);
  }

  void analyzeID53(File mFile, int currentOffset, int currentBlock) {
    _myTZX.descriptor[currentBlock].ID = 53;
    _myTZX.descriptor[currentBlock].playeable = false;
    // Size block
    int sizeBlock = 0;
    sizeBlock = getWORD(mFile, currentOffset + 0x10 + 1);
    _myTZX.descriptor[currentBlock].size = sizeBlock;
    _myTZX.descriptor[currentBlock].offset = currentOffset + 0x14;
    SerialHW.print("Tamaño: 0x");
    SerialHW.println(sizeBlock, HEX);
  }

  void analyzeID75(File mFile, int currentOffset, int currentBlock) {
    // ID 0x4B

    _myTZX.descriptor[currentBlock].ID = 75;
    _myTZX.descriptor[currentBlock].playeable = true;
    _myTZX.descriptor[currentBlock].offset = currentOffset;
    _myTZX.descriptor[currentBlock].timming.sync_1 = 0;
    _myTZX.descriptor[currentBlock].timming.sync_2 = 0;

    // Tamaño de los datos
    _myTZX.descriptor[currentBlock].lengthOfData =
        getNBYTE(mFile, currentOffset + 1, 4) - 12;
    _myTZX.descriptor[currentBlock].size =
        _myTZX.descriptor[currentBlock].lengthOfData;
#ifdef DEBUGMODE
    logln("");
    logln("Tamaño datos: " +
          String(_myTZX.descriptor[currentBlock].lengthOfData) + " bytes");
#endif

    _myTZX.descriptor[currentBlock].offsetData = currentOffset + 17;

    // Pausa despues del bloque
    _myTZX.descriptor[currentBlock].pauseAfterThisBlock =
        (double)getWORD(mFile, currentOffset + 5);
#ifdef DEBUGMODE
    logln("Pause after block: " +
          String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
#endif

    // Timming de PULSE PILOT
    _myTZX.descriptor[currentBlock].timming.pilot_len =
        getWORD(mFile, currentOffset + 7);
#ifdef DEBUGMODE
    logln(",PULSE PILOT = " +
          String(_myTZX.descriptor[currentBlock].timming.pilot_len));
#endif

    // Timming de PILOT TONE
    _myTZX.descriptor[currentBlock].timming.pilot_num_pulses =
        getWORD(mFile, currentOffset + 9);
#ifdef DEBUGMODE
    logln(",PULSE TONE = " +
          String(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses));
#endif

    // Timming de ZERO
    _myTZX.descriptor[currentBlock].timming.bit_0 =
        getWORD(mFile, currentOffset + 11);
#ifdef DEBUGMODE
    logln("PULSE ZERO = " +
          String(_myTZX.descriptor[currentBlock].timming.bit_0));
#endif

    // Timming de ONE
    _myTZX.descriptor[currentBlock].timming.bit_1 =
        getWORD(mFile, currentOffset + 13);
#ifdef DEBUGMODE
    logln("PULSE ONE = " +
          String(_myTZX.descriptor[currentBlock].timming.bit_1));
#endif

    // Configuracion de los bits
    _myTZX.descriptor[currentBlock].timming.bitcfg =
        getBYTE(mFile, currentOffset + 15);

    _myTZX.descriptor[currentBlock].timming.bytecfg =
        getBYTE(mFile, currentOffset + 16);

#ifdef DEBUGMODE
    logln(",ENDIANNESS=");
    logln(String(_myTZX.descriptor[currentBlock].timming.bitcfg & 0b00000001));
#endif

    _myTZX.descriptor[currentBlock].type = 99;
  }

  void analyzeBlockUnknow(int id_num, int currentOffset, int currentBlock) {
    _myTZX.descriptor[currentBlock].ID = id_num;
    _myTZX.descriptor[currentBlock].playeable = false;
    _myTZX.descriptor[currentBlock].offset = currentOffset;
  }

  bool getTZXBlock(File mFile, int currentBlock, int currentID,
                   int currentOffset, int &nextIDoffset) {
    bool res = true;

    // Inicializamos el descriptor
    _myTZX.descriptor[currentBlock].group = 0;
    _myTZX.descriptor[currentBlock].chk = 0;
    _myTZX.descriptor[currentBlock].delay = 1000;
    _myTZX.descriptor[currentBlock].hasMaskLastByte = false;
    _myTZX.descriptor[currentBlock].header = false;
    _myTZX.descriptor[currentBlock].ID = 0;
    _myTZX.descriptor[currentBlock].lengthOfData = 0;
    _myTZX.descriptor[currentBlock].loop_count = 0;
    _myTZX.descriptor[currentBlock].maskLastByte = 8;
    _myTZX.descriptor[currentBlock].nameDetected = false;
    _myTZX.descriptor[currentBlock].offset = 0;
    _myTZX.descriptor[currentBlock].jump_this_ID = false;
    _myTZX.descriptor[currentBlock].signalLvl = false;

    switch (currentID) {
    // ID 10 - Standard Speed Data Block
    case 16:

      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID16(mFile, currentOffset, currentBlock);
        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 5;
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID10STR, 35);
      } else {
        res = false;
      }
      break;

    // ID 11- Turbo Speed Data Block
    case 17:

      // TIMMING_STABLISHED = true;

      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID17(mFile, currentOffset, currentBlock);
        nextIDoffset =
            currentOffset + _myTZX.descriptor[currentBlock].size + 19;
        //_myTZX.descriptor[currentBlock].typeName = "ID 11 - Speed block";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID11STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 12 - Pure Tone
    case 18:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID18(mFile, currentOffset, currentBlock);
        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
        //_myTZX.descriptor[currentBlock].typeName = "ID 12 - Pure tone";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID12STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 13 - Pulse sequence
    case 19:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID19(mFile, currentOffset, currentBlock);
        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
        //_myTZX.descriptor[currentBlock].typeName = "ID 13 - Pulse seq.";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID13STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 14 - Pure Data Block
    case 20:

      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID20(mFile, currentOffset, currentBlock);

        nextIDoffset =
            currentOffset + _myTZX.descriptor[currentBlock].size + 10 + 1;

        //"ID 14 - Pure Data block";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID14STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 15 - Direct Recording
    case 21:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID21(mFile, currentOffset, currentBlock);

        nextIDoffset = currentOffset +
                       _myTZX.descriptor[currentBlock].lengthOfData + 8 + 1;
        logln("Next ID offset: 0x" + String(nextIDoffset, HEX));
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID15STR, 35);

        // Informacion minima del fichero
        // PROGRAM_NAME = "Audio block (WAV)";
        LAST_SIZE = _myTZX.descriptor[currentBlock].size;

#ifdef DEBUGMODE
        log("ID 0x21 - DIRECT RECORDING");
#endif
      } else {
        res = false;
      }
      break;

    // ID 18 - CSW Recording
    case 24:
      if (_myTZX.descriptor != nullptr) {
        analyzeID24(mFile, currentOffset, currentBlock);
        // ✅ CÁLCULO DE OFFSET CORREGIDO
        // El siguiente bloque empieza después del ID (1), el campo de tamaño
        // (4) y el tamaño de los datos.
        nextIDoffset =
            currentOffset + 1 + 4 + _myTZX.descriptor[currentBlock].size;
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID18STR, 35);
      } else {
        res = false;
      }
      break;

    // ID 19 - Generalized Data Block
    case 25:
      // ID_NOT_IMPLEMENTED = true;
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID25(mFile, currentOffset, currentBlock);
        // Posición del siguiente bloque en el TZX
        nextIDoffset =
            currentOffset + _myTZX.descriptor[currentBlock].size + 4 + 1;
        logln("Next ID offset: 0x" + String(nextIDoffset, HEX));
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID19STR, 35);
      } else {
        res = false;
      }
      break;

    // ID 20 - Pause and Stop Tape
    case 32:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID32(mFile, currentOffset, currentBlock);

        nextIDoffset = currentOffset + 3;
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID20STR, 35);

#ifdef DEBUGMODE
        log("ID 0x20 - PAUSE / STOP TAPE");
        log("-- value: " +
            String(_myTZX.descriptor[currentBlock].pauseAfterThisBlock));
#endif
      } else {
        res = false;
      }
      break;

    // ID 21 - Group start
    case 33:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID33(mFile, currentOffset, currentBlock);

        nextIDoffset = currentOffset + 2 + _myTZX.descriptor[currentBlock].size;
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID21STR, 35);

        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;

        //_myTZX.descriptor[currentBlock].typeName = "ID 21 - Group start";
      } else {
        res = false;
      }
      break;

    // ID 22 - Group end
    case 34:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        _myTZX.descriptor[currentBlock].ID = 34;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;

        nextIDoffset = currentOffset + 1;
        //_myTZX.descriptor[currentBlock].typeName = "ID 22 - Group end";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID22STR, 35);

        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;

      } else {
        res = false;
      }
      break;

    // ID 23 - Jump to block
    case 35:
      ID_NOT_IMPLEMENTED = true;
      // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
      nextIDoffset = currentOffset + 2 + 1;

      // _myTZX.descriptor[currentBlock].typeName = "ID 23 - Jump to block";
      strncpy(_myTZX.descriptor[currentBlock].typeName, ID23STR, 35);
      res = false;
      break;

    // ID 24 - Loop start
    case 36:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        _myTZX.descriptor[currentBlock].ID = 36;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;
        _myTZX.descriptor[currentBlock].loop_count =
            getWORD(mFile, currentOffset + 1);

#ifdef DEBUGMODE
        log("LOOP GET: " + String(_myTZX.descriptor[currentBlock].loop_count));
#endif

        nextIDoffset = currentOffset + 3;
        //_myTZX.descriptor[currentBlock].typeName = "ID 24 - Loop start";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID24STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 25 - Loop end
    case 37:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        _myTZX.descriptor[currentBlock].ID = 37;
        _myTZX.descriptor[currentBlock].playeable = false;
        _myTZX.descriptor[currentBlock].offset = currentOffset;
        LOOP_END = currentOffset;

        nextIDoffset = currentOffset + 1;
        //_myTZX.descriptor[currentBlock].typeName = "ID 25 - Loop end";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID25STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 26 - Call sequence
    case 38: {
      ID_NOT_IMPLEMENTED = true;

      _myTZX.descriptor[currentBlock].ID = 38; // 0x26
      _myTZX.descriptor[currentBlock].playeable = false;
      _myTZX.descriptor[currentBlock].offset = currentOffset;

      uint16_t num_calls = getWORD(mFile, currentOffset + 1);
      _myTZX.descriptor[currentBlock].call_sequence_count = num_calls;

      if (num_calls > 0) {
        _myTZX.descriptor[currentBlock].call_sequence_array =
            (uint16_t *)ps_calloc(num_calls, sizeof(uint16_t));
        if (_myTZX.descriptor[currentBlock].call_sequence_array) {
          int data_offset = currentOffset + 3;
          for (int i = 0; i < num_calls; i++) {
            _myTZX.descriptor[currentBlock].call_sequence_array[i] =
                getWORD(mFile, data_offset + (i * 2));
          }
        }
      }

      // El tamaño del bloque es 1 (ID) + 2 (count) + count * 2 (array)
      _myTZX.descriptor[currentBlock].size = 2 + (num_calls * 2);
      // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
      nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;

      // _myTZX.descriptor[currentBlock].typeName = "ID 26 - Call seq.";
      strncpy(_myTZX.descriptor[currentBlock].typeName, ID26STR, 35);
      res = false;
      break;
    }
    // ID 27 - Return from sequence
    case 39:
      ID_NOT_IMPLEMENTED = true;
      // analyzeBlockUnknow(currentID,currentOffset, currentBlock);
      nextIDoffset = currentOffset + 1;

      // _myTZX.descriptor[currentBlock].typeName = "ID 27 - Return from seq.";
      strncpy(_myTZX.descriptor[currentBlock].typeName, ID27STR, 35);
      res = false;
      break;

    // ID 28 - Select block
    case 40:
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID40(mFile, currentOffset, currentBlock);

        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;

        //"ID 28 -Select block";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID28STR, 35);

      } else {
        res = false;
      }
      break;

    // ID 2A - Stop the tape if in 48K mode
    case 42:
      analyzeBlockUnknow(currentID, currentOffset, currentBlock);
      nextIDoffset = currentOffset + 1;

      //_myTZX.descriptor[currentBlock].typeName = "ID 2A - Stop TAPE (48k
      // mode)";
      strncpy(_myTZX.descriptor[currentBlock].typeName, ID2ASTR, 35);

      break;

    // ID 2B - Set signal level
    case 43:
      if (_myTZX.descriptor != nullptr) {
        int signalLevel = getBYTE(mFile, currentOffset + 5);
        _myTZX.descriptor[currentBlock].ID = 43;
        _myTZX.descriptor[currentBlock].size = 5;
        _myTZX.descriptor[currentBlock].offset = currentOffset + 6;
        _myTZX.descriptor[currentBlock].playeable = false;

        // Inversion de señal
        INVERSETRAIN = signalLevel;
        hmi.writeString("menuAudio2.polValue.val=" + String(INVERSETRAIN));
        _myTZX.descriptor[currentBlock].signalLvl = INVERSETRAIN;

        _hmi.refreshPulseIcons(INVERSETRAIN, ZEROLEVEL);

        nextIDoffset = currentOffset + 6;

        //_myTZX.descriptor[currentBlock].typeName = "ID 2B - Set signal level";
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID2BSTR, 35);
      } else {
        res = false;
      }
      break;

    // ID 30 - Text description
    case 48:
      // No hacemos nada solamente coger el siguiente offset
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID48(mFile, currentOffset, currentBlock);
        // Siguiente ID
        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 1;
        //_myTZX.descriptor[currentBlock].typeName = "ID 30 - Information";
        strncpy(_myTZX.descriptor[currentBlock].typeName, IDXXSTR, 35);
        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;
      } else {
        res = false;
      }
      break;

    // ID 31 - Message block
    case 49:
      // No hacemos nada solamente coger el siguiente offset
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID49(mFile, currentOffset, currentBlock);
        // Siguiente ID
        nextIDoffset = currentOffset + _myTZX.descriptor[currentBlock].size + 2;
        //_myTZX.descriptor[currentBlock].typeName = "ID 30 - Information";
        strncpy(_myTZX.descriptor[currentBlock].typeName, IDXXSTR, 35);
        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;
      } else {
        res = false;
      }
      break;

    // ID 32 - Archive info
    case 50:
      // No hacemos nada solamente coger el siguiente offset
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID50(mFile, currentOffset, currentBlock);

        // Siguiente ID
        nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
        //_myTZX.descriptor[currentBlock].typeName = "ID 32 - Archive info";
        strncpy(_myTZX.descriptor[currentBlock].typeName, IDXXSTR, 35);
        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;
      } else {
        res = false;
      }
      break;

    // ID 33 - Hardware type
    case 51:
      // No hacemos nada solamente coger el siguiente offset
      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID51(mFile, currentOffset, currentBlock);

        // Siguiente ID
        nextIDoffset = currentOffset + 3 + _myTZX.descriptor[currentBlock].size;
        //_myTZX.descriptor[currentBlock].typeName = "ID 33- Hardware type";
        strncpy(_myTZX.descriptor[currentBlock].typeName, IDXXSTR, 35);
        // Esto le indica a los bloque de control de flujo que puede saltarse
        _myTZX.descriptor[currentBlock].jump_this_ID = true;
      } else {
        res = false;
      }
      break;

    // ID 35 - Custom info block
    case 53:
      analyzeID53(mFile, currentOffset, currentBlock);
      nextIDoffset =
          currentOffset + 0x15 + _myTZX.descriptor[currentBlock].size;

      //_myTZX.descriptor[currentBlock].typeName = "ID 35 - Custom info block";
      strncpy(_myTZX.descriptor[currentBlock].typeName, IDXXSTR, 35);

      // Esto le indica a los bloque de control de flujo que puede saltarse
      _myTZX.descriptor[currentBlock].jump_this_ID = true;

      break;

    // ID 4B - Standard Speed Data Block
    case 75:

      if (_myTZX.descriptor != nullptr) {
        // Obtenemos la dirección del siguiente offset
        analyzeID75(mFile, currentOffset, currentBlock);
        nextIDoffset =
            currentOffset + _myTZX.descriptor[currentBlock].size + 17;
        strncpy(_myTZX.descriptor[currentBlock].typeName, ID4BSTR, 35);
      } else {
        res = false;
      }
      break;

    // ID 5A - "Glue" block (90 dec, ASCII Letter 'Z')
    case 90:
      analyzeBlockUnknow(currentID, currentOffset, currentBlock);
      nextIDoffset = currentOffset + 8;

      //_myTZX.descriptor[currentBlock].typeName = "ID 5A - Glue block";
      strncpy(_myTZX.descriptor[currentBlock].typeName, ID5ASTR, 35);
      break;

    default:
////SerialHW.println("");
////SerialHW.println("ID unknow " + currentID);
#ifdef DEBUGMODE
      Serial.print("ID unknow 0x");
      Serial.println(currentID, HEX);
#endif
      // Si no está implementado, salimos
      ID_NOT_IMPLEMENTED = true;
      res = false;
      break;
    }

    return res;
  }

  uint8_t *decompressCSW(uint8_t *compressedData, int compressedSize,
                         int compressionType, int &decompressedSize) {
    // Inicializar el tamaño de salida a 0
    decompressedSize = 0;

    // Validar entradas
    if (!compressedData || compressedSize == 0) {
      logln("ERROR (decompressCSW): Datos de entrada nulos o de tamaño cero.");
      return nullptr;
    }
    if (compressionType != 1 && compressionType != 2) {
      logln("ERROR (decompressCSW): Tipo de compresión no soportado: " +
            String(compressionType));
      return nullptr;
    }

    // 1. Preparar el buffer de destino con un tamaño inicial
    int capacity = compressedSize * 2; // Una estimación inicial razonable
    uint8_t *decompressedBuffer = (uint8_t *)ps_malloc(capacity);
    if (!decompressedBuffer) {
      logln("ERROR (decompressCSW): Fallo al alocar memoria inicial.");
      return nullptr;
    }

    int readPtr = 0;  // Puntero de lectura en el buffer comprimido
    int writePtr = 0; // Puntero de escritura en el buffer descomprimido

    // 2. Bucle de descompresión
    while (readPtr < compressedSize) {
      // Re-alocar más memoria si el buffer de destino se está llenando
      if (writePtr >= capacity - 256) { // Dejar un margen de seguridad
        capacity *= 2;
        uint8_t *newBuffer =
            (uint8_t *)ps_realloc(decompressedBuffer, capacity);
        if (!newBuffer) {
          logln("ERROR (decompressCSW): Fallo al re-alocar memoria.");
          free(decompressedBuffer);
          return nullptr;
        }
        decompressedBuffer = newBuffer;
      }

      uint8_t value = compressedData[readPtr++];

      // Lógica específica para Z-RLE (Tipo 2)
      if (compressionType == 2 && value == 0) {
        // Si el siguiente byte no existe, es un error en el fichero
        if (readPtr >= compressedSize)
          break;

        uint8_t repeatCount = compressedData[readPtr++];

        if (repeatCount == 0) {
          // Marcador de fin de stream en Z-RLE
          break;
        } else {
          // Secuencia de 'repeatCount' pulsos de 1 sample.
          // En un contexto de contenedor, esto se traduce a 'repeatCount' bytes
          // con valor 1.
          for (int k = 0; k < repeatCount; k++) {
            decompressedBuffer[writePtr++] = 1;
          }
        }
      }
      // Lógica para RLE (Tipo 1) y para pulsos normales de Z-RLE
      else {
        decompressedBuffer[writePtr++] = value;
      }
    }

    // 3. Ajustar el tamaño final del buffer y devolverlo
    decompressedSize = writePtr;
    uint8_t *finalBuffer =
        (uint8_t *)ps_realloc(decompressedBuffer, decompressedSize);
    if (!finalBuffer && decompressedSize > 0) {
      logln("WARNING (decompressCSW): Fallo al ajustar el buffer final. Puede "
            "haber memoria extra alocada.");
      return decompressedBuffer; // Devolver el buffer original si el realloc
                                 // falla
    }

    logln("CSW descompresión completa: " + String(compressedSize) +
          " bytes -> " + String(decompressedSize) + " bytes.");
    return finalBuffer;
  }

  // ... (después de la función decompressCSW) ...
  /**
   * @brief Descomprime un fichero CSW (RLE o Z-RLE) a otro fichero.
   *
   * @param compressedFile Fichero de entrada (comprimido), abierto en modo
   * lectura.
   * @param compressionType Tipo de compresión (1 para RLE, 2 para Z-RLE).
   * @param outFile Fichero de salida (descomprimido), abierto en modo
   * escritura.
   * @return bool Devuelve true si la descompresión fue exitosa.
   */
  bool decompressCSWFile(File &compressedFile, int compressionType,
                         File &outFile) {
    if (!compressedFile || !outFile) {
      logln("ERROR (decompressCSWFile): Ficheros de entrada o salida no "
            "válidos.");
      return false;
    }

    uint8_t read_buffer[512];
    int bytes_read;

    while ((bytes_read =
                compressedFile.read(read_buffer, sizeof(read_buffer))) > 0) {
      int readPtr = 0;
      while (readPtr < bytes_read) {
        uint8_t value = read_buffer[readPtr++];

        if (compressionType == 2 && value == 0) {
          if (readPtr >= bytes_read) { // Necesitamos leer el siguiente byte
            if ((bytes_read = compressedFile.read(read_buffer,
                                                  sizeof(read_buffer))) > 0) {
              readPtr = 0;
            } else {
              break; // Fin del fichero
            }
          }
          uint8_t repeatCount = read_buffer[readPtr++];

          if (repeatCount == 0) { // Fin del stream Z-RLE
            return true;
          } else {
            for (int k = 0; k < repeatCount; k++) {
              uint8_t one = 1;
              outFile.write(&one, 1);
            }
          }
        } else {
          outFile.write(&value, 1);
        }
      }
    }
    return true;
  }

  void getBlockDescriptor(File mFile, int sizeTZX, bool hasGroupBlocks) {
    // Para ello tenemos que ir leyendo el TZX poco a poco
    // Detectaremos los IDs a partir del byte 9 (empezando por offset = 0)
    // Cada bloque empieza por un ID menos el primero
    // que empieza por ZXTape!
    // Ejemplo ID + bytes
    //
    int startOffset = 10; // Posición del primero ID, pero el offset
                          // 0x00 de cada ID, sería el siguiente
    int currentID = 0;
    int nextIDoffset = 0;
    int currentOffset = 0;
    int lenghtDataBlock = 0;
    int currentBlock = 1;
    bool endTZX = false;
    bool forzeEnd = false;
    bool endWithErrors = false;

    currentOffset = startOffset;

    // Inicializamos
    ID_NOT_IMPLEMENTED = false;

// Le pasamos el path del fichero, la extension se le asigna despues en
// la funcion createBlockDescriptorFile
// _blDscTZX.createBlockDescriptorFileTZX();
#ifdef DEBUGMODE
    logln("Creating DSC file");
    logln("Status of endTZX: " + String(endTZX));
    logln("Status of forzeEnd: " + String(forzeEnd));
    logln("Status of ID_NOT_IMPLEMENTED: " + String(ID_NOT_IMPLEMENTED));
#endif

    while (!endTZX && !forzeEnd && !ID_NOT_IMPLEMENTED) {

      if (ABORT == true) {
        forzeEnd = true;
        endWithErrors = true;
        LAST_MESSAGE = "Aborting. No proccess complete.";

#ifdef DEBUGMODE
        Serial.println("Aborting TZX reading");
#endif
      }

      // El objetivo es ENCONTRAR IDs y ultimo byte, y analizar el bloque
      // completo para el descriptor.
      currentID = getID(mFile, currentOffset);

      // Por defecto el bloque no es reproducible
      _myTZX.descriptor[currentBlock].playeable = false;

      // Ahora dependiendo del ID analizamos. Los ID están en HEX
      // y la rutina devolverá la posición del siguiente ID, así hasta
      // completar todo el fichero

      if (getTZXBlock(mFile, currentBlock, currentID, currentOffset,
                      nextIDoffset)) {
        // Copiamos la estructura del bloque, apuntando a ella
        tTZXBlockDescriptor t = _myTZX.descriptor[currentBlock];
        // Agregamos la informacion del bloque al fichero del descriptor .dsc
        //_blDscTZX.putBlocksDescriptorTZX(dscFile,
        // currentBlock,t,sizeTZX,hasGroupBlocks);
        // Incrementamos el bloque
        currentBlock++;
      } else {
        LAST_MESSAGE = "ID block not implemented. Aborting";
        forzeEnd = true;
        endWithErrors = true;
      }

      if (currentBlock > maxAllocationBlocks) {
#ifdef DEBUGMODE
        SerialHW.println("Error. TZX not possible to allocate in memory");
#endif

        LAST_MESSAGE = "Error. Not enough memory for TZX/TSX/CDT";
        endTZX = true;
        // Salimos
        return;
      } else {
      }

      if (nextIDoffset >= sizeTZX) {
        // Finalizamos
        endTZX = true;
        logln("END TZX analying");

      } else {
        currentOffset = nextIDoffset;
      }

      TOTAL_BLOCKS = currentBlock;
    }
    // Nos posicionamos en el bloque 1
    BLOCK_SELECTED = 0;
    _hmi.writeString("currentBlock.val=" + String(BLOCK_SELECTED));

    _myTZX.numBlocks = currentBlock;
    _myTZX.size = sizeTZX;
  }

  void process_tzx(File mfile) //, File &dscFile)
  {
// Procesamos el fichero
#ifdef DEBUGMODE
    logln("Processing TZX file");
#endif

    if (isFileTZX(mfile)) {
      logln("TZX file detected");
      // Esto lo hacemos para poder abortar
      ABORT = false;
      getBlockDescriptor(mfile, _sizeTZX, _myTZX.hasGroupBlocks);
    } else {
      logln("Error: Not a TZX file");
    }
  }

  void showBufferPlay(uint8_t *buffer, int size, int offset) {

    char hexs[20];
    if (size > 10) {
#ifdef DEBUGMODE
      log("");
      logln(" Listing bufferplay.");
      SerialHW.print(buffer[0], HEX);
      SerialHW.print(",");
      SerialHW.print(buffer[1], HEX);
      SerialHW.print(" ... ");
      SerialHW.print(buffer[size - 2], HEX);
      SerialHW.print(",");
      SerialHW.print(buffer[size - 1], HEX);
      log("");
#endif

      sprintf(hexs, "%X", buffer[size - 1]);
      dataOffset4 = hexs;
      sprintf(hexs, "%X", buffer[size - 2]);
      dataOffset3 = hexs;
      sprintf(hexs, "%X", buffer[1]);
      dataOffset2 = hexs;
      sprintf(hexs, "%X", buffer[0]);
      dataOffset1 = hexs;

      sprintf(hexs, "%X", offset + (size - 1));
      Offset4 = hexs;
      sprintf(hexs, "%X", offset + (size - 2));
      Offset3 = hexs;
      sprintf(hexs, "%X", offset + 1);
      Offset2 = hexs;
      sprintf(hexs, "%X", offset);
      Offset1 = hexs;

    } else {
#ifdef DEBUGMODE
      // Solo mostramos la ultima parte
      SerialHW.println("Listing bufferplay. SHORT");
      SerialHW.print(buffer[size - 2], HEX);
      SerialHW.print(",");
      SerialHW.print(buffer[size - 1], HEX);

      sprintf(hexs, "%X", buffer[size - 1]);
      dataOffset4 = hexs;
      sprintf(hexs, "%X", buffer[size - 2]);
      dataOffset3 = hexs;

      sprintf(hexs, "%X", offset + (size - 1));
      Offset4 = hexs;
      sprintf(hexs, "%X", offset + (size - 2));
      Offset3 = hexs;

      log("");
#endif
    }
  }

public:
  tTZXBlockDescriptor *getDescriptor() { return _myTZX.descriptor; }

  bool getPZXInfo(File mFile) {
    // (Lógica similar a getTZXInfo, pero para PZX)
    // ...
    return true;
  }

  // ✅ NUEVA FUNCIÓN PARA OBTENER UN BLOQUE PZX
  int getPZXBlock(File mFile, int currentOffset,
                  tPZXBlockDescriptor &descriptor) {
    _pzx.analyzePZXBlock(mFile, currentOffset, descriptor);
    // El siguiente offset es el actual + cabecera (8) + tamaño de datos
    return currentOffset + 8 + descriptor.size;
  }

  void setDescriptorNull() { _myTZX.descriptor = nullptr; }

  void setTZX(tTZX tzx) { _myTZX = tzx; }

  void set_HMI(HMI hmi) { _hmi = hmi; }

  void set_file(File mFile, int sizeTZX) {
    // Pasamos los parametros a la clase
    _mFile = mFile;
    _sizeTZX = sizeTZX;
  }

  void proccessingDescriptor(File &tzxFile) {

    LAST_MESSAGE = "Analyzing file. Capturing blocks";

    // if (SD_MMC.exists(pathDSC))
    // {
    //   SD_MMC.remove(pathDSC);
    // }

    //_blDscTZX.createBlockDescriptorFileTZX(dscFile,pathDSC);
    // creamos un objeto TZXproccesor
    set_file(tzxFile, _rlen);
    // Lo procesamos y creamos DSC
    process_tzx(tzxFile);
    // Cerramos para guardar los cambios
    // dscFile.close();

    if (_myTZX.descriptor != nullptr && !ID_NOT_IMPLEMENTED) {
      // Entregamos información por consola
      strcpy(LAST_NAME, "              ");
    } else {
      LAST_MESSAGE = "Error in TZX/TSX/CDT or ID not supported";
    }
  }

  void process(char *path) {

    File tzxFile;
    File dscFile;

    PROGRAM_NAME_DETECTED = false;
    PROGRAM_NAME = "";
    PROGRAM_NAME_2 = "";

    MULTIGROUP_COUNT = 1;

    // Abrimos el fichero
    tzxFile = SD_MMC.open(path, FILE_READ);
    // char* pathDSC = path;

    // Pasamos el fichero abierto a la clase. Elemento global
    _mFile = tzxFile;
    // Obtenemos su tamaño total
    _rlen = tzxFile.available();
    _myTZX.size = tzxFile.size();

    if (_rlen != 0) {
      FILE_IS_OPEN = true;

      proccessingDescriptor(tzxFile);
      logln("All blocks captured from TZX file");
    } else {
      FILE_IS_OPEN = false;
      LAST_MESSAGE = "Error in TZX/TSX/CDT file has 0 bytes";
    }
  }

  void initialize() {

    strncpy(_myTZX.name, "          ", 10);
    _myTZX.numBlocks = 0;
    _myTZX.size = 0;

    CURRENT_BLOCK_IN_PROGRESS = 1;
    BLOCK_SELECTED = 1;
    PROGRESS_BAR_BLOCK_VALUE = 0;
    PROGRESS_BAR_TOTAL_VALUE = 0;
    PROGRAM_NAME_DETECTED = false;
  }

  void terminate() {
    // free(_myTZX.descriptor);
    //_myTZX.descriptor = nullptr;
    // free(_myTZX.name);
    strncpy(_myTZX.name, "          ", 10);
    _myTZX.numBlocks = 0;
    _myTZX.size = 0;
    // free(_myTZX.descriptor);
    // _myTZX.descriptor = nullptr;
  }

  void playBlock(tTZXBlockDescriptor descriptor) {

#ifdef DEBUGMODE
    log("Pulse len: " + String(descriptor.timming.pilot_len));
    log("Pulse nums: " + String(descriptor.timming.pilot_num_pulses));
#endif

    uint8_t *bufferPlay = nullptr;

    // int pulsePilotDuration = descriptor.timming.pulse_pilot *
    // descriptor.timming.pilot_num_pulses;
    int blockSizeSplit = SIZE_FOR_SPLIT;

    if (descriptor.size > blockSizeSplit) {
      int totalSize = descriptor.size;
      PARTITION_SIZE = totalSize;

      int offsetBase = descriptor.offsetData;
      int newOffset = 0;
      int blocks = totalSize / blockSizeSplit;
      int lastBlockSize = totalSize - (blocks * blockSizeSplit);

#ifdef DEBUGMODE
      logln("Partiendo la pana");
      logln("");
      log("> Offset DATA:         " + String(offsetBase));
      log("> Total size del DATA: " + String(totalSize));
      log("> Total blocks: " + String(blocks));
      log("> Last block size: " + String(lastBlockSize));
#endif

      if (!DIRECT_RECORDING) {
        // BTI 0
        _zxp.BIT_0 = descriptor.timming.bit_0;
        // BIT1
        _zxp.BIT_1 = descriptor.timming.bit_1;
      }

      TOTAL_PARTS = blocks;

      // Recorremos el vector de particiones del bloque.
      for (int n = 0; n < blocks; n++) {
        PARTITION_BLOCK = n;

#ifdef DEBUGMODE
        logln("");
        log("Sending partition");
#endif

        // Calculamos el offset del bloque
        newOffset = offsetBase + (blockSizeSplit * n);
        PRG_BAR_OFFSET_INI = newOffset;

        // Accedemos a la SD y capturamos el bloque del fichero
        bufferPlay = (uint8_t *)ps_calloc(blockSizeSplit, sizeof(uint8_t));
        readFileRange(_mFile, bufferPlay, newOffset, blockSizeSplit, true);

        // Mostramos en la consola los primeros y últimos bytes
        // showBufferPlay(bufferPlay,blockSizeSplit,newOffset);

        // Reproducimos la partición n, del bloque.
        if (n == 0) {
          if (!DIRECT_RECORDING) {
            _zxp.playDataBegin(bufferPlay, blockSizeSplit,
                               descriptor.timming.pilot_len,
                               descriptor.timming.pilot_num_pulses);
          } else {
#ifdef DEBUGMODE
            logln("");
            log("> Playing DR block - Splitted - begin");
#endif
            _zxp.playDRBlock(bufferPlay, blockSizeSplit, false);
          }
        } else {
          if (!DIRECT_RECORDING) {
            _zxp.playDataPartition(bufferPlay, blockSizeSplit);
          } else {
#ifdef DEBUGMODE
            logln("");
            log("> Playing DR block - Splitted - middle");
#endif
            _zxp.playDRBlock(bufferPlay, blockSizeSplit, false);
          }
        }

        free(bufferPlay);
      }

      // Ultimo bloque
      // log("Particion [" + String(blocks) + "/" + String(blocks) + "]");

      // Calculamos el offset del último bloque
      newOffset = offsetBase + (blockSizeSplit * blocks);
      PRG_BAR_OFFSET_INI = newOffset;

      blockSizeSplit = lastBlockSize;
      // Accedemos a la SD y capturamos el bloque del fichero
      bufferPlay = (uint8_t *)ps_calloc(blockSizeSplit, sizeof(uint8_t));
      readFileRange(_mFile, bufferPlay, newOffset, blockSizeSplit, true);

      // Mostramos en la consola los primeros y últimos bytes
      // showBufferPlay(bufferPlay,blockSizeSplit,newOffset);

      // Reproducimos el ultimo bloque con su terminador y silencio si aplica
      if (!DIRECT_RECORDING) {
        _zxp.playDataEnd(bufferPlay, blockSizeSplit);
      } else {
#ifdef DEBUGMODE
        logln("");
        log("> Playing DR block - Splitted - Last");
#endif

        _zxp.playDRBlock(bufferPlay, blockSizeSplit, true);
      }

      free(bufferPlay);
    } else {
      PRG_BAR_OFFSET_INI = descriptor.offsetData;

      // Si es mas pequeño que el SPLIT, se reproduce completo.
      bufferPlay = (uint8_t *)ps_calloc(descriptor.size, sizeof(uint8_t));
      readFileRange(_mFile, bufferPlay, descriptor.offsetData, descriptor.size,
                    true);

      // showBufferPlay(bufferPlay,descriptor.size,descriptor.offsetData);
      // verifyChecksum(_mFile,descriptor.offsetData,descriptor.size);

      // BTI 0
      _zxp.BIT_0 = descriptor.timming.bit_0;
      // BIT1
      _zxp.BIT_1 = descriptor.timming.bit_1;
      //
      if (!DIRECT_RECORDING) {
        _zxp.playData(bufferPlay, descriptor.size, descriptor.timming.pilot_len,
                      descriptor.timming.pilot_num_pulses);
      } else {
#ifdef DEBUGMODE
        logln("> Playing DR block - Full");
#endif

        _zxp.playDRBlock(bufferPlay, descriptor.size, true);
      }

      free(bufferPlay);
    }
  }

  bool isPlayeable(int id) {
    // Definimos los ID playeables (en decimal)
    bool res = false;
    int playeableListID[] = {16, 17, 18, 19, 20, 21, 24, 25, 32, 35,
                             36, 37, 38, 39, 40, 42, 43, 75, 90};
    for (int i = 0; i < 19; i++) {
      if (playeableListID[i] == id) {
        res = true; // Inidicamos que lo hemos encontrado
        break;
      }
    }

    return res;
  }

  void prepareID4B(int currentBlock, File mFile, int nlb, int vlb, int ntb,
                   int vtb, int pzero, int pone, int offset, int ldatos,
                   bool begin) {
    // Generamos la señal de salida
    int pulsosmaximos;
    int npulses[2];
    int vpulse[2];

    npulses[0] = pzero / 2;
    npulses[1] = pone / 2;
    vpulse[0] = (_myTZX.descriptor[currentBlock].timming.bit_0);
    vpulse[1] = (_myTZX.descriptor[currentBlock].timming.bit_1);
    pulsosmaximos =
        (_myTZX.descriptor[currentBlock].timming.pilot_num_pulses) +
        ((npulses[vlb] * nlb) + 128 + (npulses[vtb] * ntb)) * ldatos;

    // Reservamos memoria dinamica
    _myTZX.descriptor[currentBlock].timming.pulse_seq_array =
        (int *)ps_calloc(pulsosmaximos + 1, sizeof(int));
    // _myTZX.descriptor[currentBlock].timming.pulse_seq_array = new
    // int[pulsosmaximos + 1];

#ifdef DEBUGMODE
    logln("");
    log(" - Numero de pulsos: " +
        String(_myTZX.descriptor[currentBlock].timming.pilot_num_pulses));
    logln("");
    log(" - Pulsos maximos: " + String(pulsosmaximos));
    logln("");
    log(" - Longitud de los datos: " + String(ldatos));
#endif

    // metemos el pilot SOLO AL PRINCIPIO
    int i;
    int p;

    if (begin) {
      for (p = 0;
           p < (_myTZX.descriptor[currentBlock].timming.pilot_num_pulses);
           p++) {
        _myTZX.descriptor[currentBlock].timming.pulse_seq_array[p] =
            _myTZX.descriptor[currentBlock].timming.pilot_len;
      }

      i = p;
    } else {
      i = 0;
    }

#ifdef DEBUGMODE
    log(">> Bucle del pilot - Iteraciones: " + String(p));
#endif

    // metemos los datos
    uint8_t *bRead = (uint8_t *)ps_calloc(ldatos + 1, sizeof(uint8_t));
    int lenPulse;

    // Leemos el bloque definido por la particion (ldatos) del fichero
    getBlock(mFile, bRead, offset, ldatos);

    for (int i2 = 0; i2 < ldatos; i2++) // por cada byte
    {

      for (int i3 = 0; i3 < nlb; i3++) // por cada bit de inicio
      {
        for (int i4 = 0; i4 < npulses[vlb]; i4++) {
          lenPulse = vpulse[vlb];
          _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] = lenPulse;

          i++;
          _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] = lenPulse;

          i++;
        }
      }

      // metemos el byte leido
      for (int n = 0; n < 8; n++) {
        // Obtenemos el bit a transmitir
        uint8_t bitMasked = bitRead(bRead[i2], 0 + n);

        // Si el bit leido del BYTE es un "1"
        if (bitMasked == 1) {
          // Procesamos "1"
          for (int b1 = 0; b1 < npulses[1]; b1++) {
            _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] =
                vpulse[1];

            i++;
            _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] =
                vpulse[1];

            i++;
          }
        } else {
          // En otro caso
          // procesamos "0"
          for (int b0 = 0; b0 < npulses[0]; b0++) {
            _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] =
                vpulse[0];

            i++;
            _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] =
                vpulse[0];

            i++;
          }
        }
      }

      for (int i3 = 0; i3 < ntb; i3++) {
        for (int i4 = 0; i4 < npulses[vtb]; i4++) {
          lenPulse = vpulse[vtb];
          _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] = lenPulse;

          i++;
          _myTZX.descriptor[currentBlock].timming.pulse_seq_array[i] = lenPulse;

          i++;
        }
      }
    }

    _myTZX.descriptor[currentBlock].timming.pulse_seq_num_pulses = i;

#ifdef DEBUGMODE
    logln("datos: " + String(ldatos));
    logln("pulsos: " + String(i));

    logln("--------------------------------------------------------------------"
          "-");
#endif

    free(bRead);
  }

  int getIDAndPlay(int i, bool is_pzx = false) {
    // Inicializamos el buffer de reproducción. Memoria dinamica
    uint8_t *bufferPlay = nullptr;
    double dly = 0;
    int newPosition = -1;

    // Por defecto usamos este sampling rate
    // (si lo cambia DR lo recuperamos en el siguinente bloque)

    // ******************************************************************************************
    //
    //
    // --- LÓGICA DE REPRODUCCIÓN TZX ---
    //
    //
    // ******************************************************************************************

    // Cogemos la mascara del ultimo byte
    if (_myTZX.descriptor[i].hasMaskLastByte) {
      _zxp.set_maskLastByte(_myTZX.descriptor[i].maskLastByte);
    } else {
      _zxp.set_maskLastByte(8);
    }

    // Ahora lo voy actualizando a medida que van avanzando los bloques.
    PROGRAM_NAME_2 = _myTZX.descriptor[BLOCK_SELECTED].name;

    // El offset final sera el offsetdata + size
    PRG_BAR_OFFSET_END =
        _myTZX.descriptor[i].offsetData + _myTZX.descriptor[i].size;

    DIRECT_RECORDING = false;

    double sr = 0.0;
    double divd = 0.0;

    // EDGE_EAR_IS ^= 1; // Alternamos la señal EAR

    switch (_myTZX.descriptor[i].ID) {

    case 21: // ID 0x15 - Direct Recording
    {
      DIRECT_RECORDING = true;

      // 1. Calcula el SR de entrada como ya lo haces
      // double divd = double(_myTZX.descriptor[i].samplingRate) * (1.0 /
      // DfreqCPU); int input_sr = divd > 0 ? round(1.0 / divd) : BASE_SR;

      // 2. NO cambies la configuración de kitStream. Déjala fija en 32150Hz.
      new_sr = kitStream.audioInfo();
      // Calculamos el sampling rate desde el bloque ID 0x15
      divd = double(_myTZX.descriptor[i].samplingRate) * (1.0 / DfreqCPU);

      sr = divd > 0 ? 1.0 / divd : 0; // antes 44.1KHz (20/12/2025)

      if (sr <= 1.0) {
        LAST_MESSAGE = "Error in sampling rate. Abort.";
        return 0;
      }

      //
      // logln("Value in TZX srate: " +
      // String(_myTZX.descriptor[i].samplingRate)); logln("Custom sampling
      // rate: " + String(sr));

      // Si el sampling rate es menor de 8kHz, lo cambiamos a default
      // if (sr < 8000)
      // {
      //   sr = BASE_SR;
      //   logln("Error. Changing sampling rate: " + String(sr));
      // }
      // BIT_DR_0 =_myTZX.descriptor[i].samplingRate;
      // BIT_DR_1 = _myTZX.descriptor[i].samplingRate;

      // Cambiamos el sampling rate en el HW
      new_sr.sample_rate = (float)sr;
      kitStream.setAudioInfo(new_sr);

      // 3. Lee los datos del bloque en un buffer
      int data_size = _myTZX.descriptor[i].lengthOfData;

      // Usamos un buffer temporal en el stack si el tamaño es razonable,
      // o alocación dinámica si es muy grande.
      if (data_size > 0 &&
          data_size < 16384) { // Límite de seguridad para el stack
        uint8_t bufferPlay[data_size];
        readFileRange(_mFile, bufferPlay, _myTZX.descriptor[i].offsetData,
                      data_size, true);

        // 4. Llama a la nueva función de reproducción con remuestreo
        _zxp.playDRBlock2(bufferPlay, data_size, true);

      } else if (data_size >= 16384) {
        // Para bloques muy grandes, procesar en trozos para no agotar la RAM
        logln("DR Block is very large. Processing in chunks.");
        const int chunk_size = 4096;
        uint8_t chunk_buffer[chunk_size];
        int bytes_left = data_size;
        int current_offset = _myTZX.descriptor[i].offsetData;

        while (bytes_left > 0) {
          int bytes_to_read = min(bytes_left, chunk_size);
          readFileRange(_mFile, chunk_buffer, current_offset, bytes_to_read,
                        true);

          bool is_last_chunk = (bytes_to_read == bytes_left);

          _zxp.playDRBlock2(chunk_buffer, bytes_to_read, is_last_chunk);

          bytes_left -= bytes_to_read;
          current_offset += bytes_to_read;

          if (stopOrPauseRequest())
            break;
        }
      }

      // Pausa después del bloque
      _zxp.silenceDR(_myTZX.descriptor[i].pauseAfterThisBlock, sr);

      // ✅ RESTAURAR el sampling rate original después del bloque Direct
      // Recording
      new_sr.sample_rate = (float)SAMPLING_RATE;
      kitStream.setAudioInfo(new_sr);

      DIRECT_RECORDING = false;

      break;
    }

      // **********************************************************

    case 36: { // Loop start ID 0x24
      // El loop controla el cursor de bloque por tanto debe estar el primero
      LOOP_PLAYED = 0;
      // El primer bloque a repetir es el siguiente, pero ponemos "i" porque el
      // FOR lo incrementa. Entonces este bloque no se lee mas, ya se continua
      // repitiendo el bucle.
      BL_LOOP_START = i;
      LOOP_COUNT = _myTZX.descriptor[i].loop_count;
      break;
    }
    case 37: {
      // Loop end ID 0x25
#ifdef DEBUGMODE
      log("LOOP: " + String(LOOP_PLAYED) + " / " + String(LOOP_COUNT));
      log("------------------------------------------------------");
#endif

      if (LOOP_PLAYED < LOOP_COUNT) {
        // Volvemos al primner bloque dentro del loop
        newPosition = BL_LOOP_START;
        LOOP_PLAYED++;
        // return newPosition;
      }
      break;
    }
    case 32: {
      // ******************************************************************************
      // ID 0x20 - Pause (silence) or 'Stop the Tape' command
      // ******************************************************************************
      // Especificación TZX v1.20:
      // - Si duración == 0: STOP TAPE (el emulador no debe continuar)
      // - Si duración > 0: Generar silencio de la duración especificada
      //
      // "A 'Pause' block consists of a 'low' pulse level of some duration."
      // "At the end of a 'Pause' block the 'current pulse level' is low"
      // ******************************************************************************

      dly = _myTZX.descriptor[i].pauseAfterThisBlock;

      if (dly == 0) {
        // STOP THE TAPE - Duración 0 significa parar
        // Según especificación: "the emulator or utility should (in effect)
        // STOP THE TAPE, i.e. should not continue loading until the user
        // or emulator requests it."

        // Finalizamos el ultimo bit con un silencio de seguridad
        if (LAST_SILENCE_DURATION == 0) {
          // Ponemos el tail de duración 1s - 3500000 TStates
          _zxp.silence(round((PAUSE_TAIL_TSTATES / DfreqCPU)) * 1000);
        }

        LAST_GROUP = "[STOP BLOCK]";

        // Lanzamos una PAUSA automatica
        AUTO_PAUSE = true;
        _hmi.verifyCommand("PAUSE");

        // Dejamos preparado el siguiente bloque
        CURRENT_BLOCK_IN_PROGRESS++;
        BLOCK_SELECTED++;

        if (BLOCK_SELECTED >= _myTZX.numBlocks) {
          // Reiniciamos
          CURRENT_BLOCK_IN_PROGRESS = 1;
          BLOCK_SELECTED = 1;
        }
      } else {
        // Pausa normal con duración > 0
        // silence() ya maneja el terminador de 1ms y fuerza nivel LOW
        _zxp.silence(dly);

        // Aseguramos que el nivel quede en LOW después de la pausa
        // (ya lo hace silence(), pero lo dejamos explícito por seguridad)
        // EDGE_EAR_IS = down;
      }
      break;
    }
    case 33: {
      // Comienza multigrupo
      LAST_GROUP = "[META BLK: " + String(_myTZX.descriptor[i].group) + "]";
      LAST_BLOCK_WAS_GROUP_START = true;
      LAST_BLOCK_WAS_GROUP_END = false;
      break;
    }
    case 34: {
      // LAST_GROUP = &INITCHAR[0];
      //  Finaliza el multigrupo
      LAST_GROUP = "...";
      LAST_BLOCK_WAS_GROUP_END = true;
      LAST_BLOCK_WAS_GROUP_START = false;
      break;
    }
    case 43: {
      // Inversion de señal
      if (INVERSETRAIN) {
        // Para que empiece en DOWN tiene que ser POLARIZATION = UP
        // esto seria una señal invertida
        // EDGE_EAR_IS = down;
        // POLARIZATION = down;
        // INVERSETRAIN = true;
        hmi.writeString("menuAudio2.polValue.val=1");
        // FIRST_BLOCK_INVERTED = true;
      } else {
        hmi.writeString("menuAudio2.polValue.val=0");
      }
      _hmi.refreshPulseIcons(INVERSETRAIN, ZEROLEVEL);
      break;
    }
    default:
      // Cualquier otro bloque entra por aquí, pero
      // hay que comprobar que sea REPRODUCIBLE

      // Reseteamos el indicador de META BLOCK
      // (añadido el 26/03/2024)
      // LAST_GROUP = "...";

      if (_myTZX.descriptor[i].playeable) {
        // Silent
        _zxp.silent = _myTZX.descriptor[i].pauseAfterThisBlock;
        logln("Bloque: " + String(i));
        logln("Silencio=" + String(_zxp.silent));
        logln("");

        // SYNC1
        _zxp.SYNC1 = _myTZX.descriptor[i].timming.sync_1;
        // SYNC2
        _zxp.SYNC2 = _myTZX.descriptor[i].timming.sync_2;
        // PULSE PILOT (longitud del pulso)
        _zxp.PILOT_PULSE_LEN = _myTZX.descriptor[i].timming.pilot_len;
        // BTI 0
        _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
        // BIT1
        _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;

        // Obtenemos el nombre del bloque
        strncpy(LAST_NAME, _myTZX.descriptor[i].name, 14);
        LAST_SIZE = _myTZX.descriptor[i].size;
        strncpy(LAST_TYPE, _myTZX.descriptor[i].typeName, 35);

#ifdef DEBUGMODE
        logln("Bl: " + String(i) + "Playeable block");
        logln("Bl: " + String(i) + "Name: " + LAST_NAME);
        logln("Bl: " + String(i) + "Type name: " + LAST_TYPE);
        logln("Bl: " + String(i) +
              "pauseAfterThisBlock: " + String(_zxp.silent));
        logln("Bl: " + String(i) + "Sync1: " + String(_zxp.SYNC1));
        logln("Bl: " + String(i) + "Sync2: " + String(_zxp.SYNC2));
        logln("Bl: " + String(i) +
              "Pilot pulse len: " + String(_zxp.PILOT_PULSE_LEN));
        logln("Bl: " + String(i) + "Bit0: " + String(_zxp.BIT_0));
        logln("Bl: " + String(i) + "Bit1: " + String(_zxp.BIT_1));
#endif

        // Almacenmas el bloque en curso para un posible PAUSE
        if (LOADING_STATE != 2) {
          CURRENT_BLOCK_IN_PROGRESS = i;
          BLOCK_SELECTED = i;
          PROGRESS_BAR_BLOCK_VALUE = 0;
        } else {
          // Paramos la reproducción.

          PAUSE = false;
          STOP = true;
          PLAY = false;

          i = _myTZX.numBlocks + 1;

          LOOP_PLAYED = 0;
          // EDGE_EAR_IS = down;
          LOOP_START = 0;
          LOOP_END = 0;
          BL_LOOP_START = 0;
          BL_LOOP_END = 0;

          // return newPosition;
        }

        // Ahora vamos lanzando bloques dependiendo de su tipo
        // Reproducimos el fichero
        // ******************************************************************************
        // CORRECCIÓN: Añadido type == 4 para bloques de datos con flagByte >=
        // 128 Esto es crítico para Speedlock y otras protecciones que usan
        // bloques Turbo con datos que no siguen la estructura estándar de
        // cabecera ZX Spectrum
        // ******************************************************************************
        if (_myTZX.descriptor[i].type == 0 || _myTZX.descriptor[i].type == 1 ||
            _myTZX.descriptor[i].type == 7 || _myTZX.descriptor[i].type == 4) {
          //
          switch (_myTZX.descriptor[i].ID) {
          case 16: {
            // Standard data - ID-10
            _myTZX.descriptor[i].timming.pilot_len = DPILOT_LEN;
            playBlock(_myTZX.descriptor[i]);
            break;
          }
          case 17: {
            // Speed data - ID-11
            playBlock(_myTZX.descriptor[i]);
            break;
          }
          }
        } else if (_myTZX.descriptor[i].type == 99) {
          //
          // Son bloques especiales de TONO GUIA o SECUENCIAS para SYNC
          //
          // int num_pulses = 0;

          // Variables para el ID 0x4B (75)
          int nlb;
          int vlb;
          int ntb;
          int vtb;
          int pzero;
          int pone;

          int ldatos;
          int offset;

          int bufferD;
          int partitions;
          int lastPartitionSize;
          double silence;

          bool begin = false;

          switch (_myTZX.descriptor[i].ID) {

          // Bloque 0x4B - MSX
          case 75: {
            BYTES_TOBE_LOAD = _myTZX.size;

            // configuracion del byte
            pzero = ((_myTZX.descriptor[i].timming.bitcfg & 0b11110000) >> 4);
            pone = ((_myTZX.descriptor[i].timming.bitcfg & 0b00001111));
            nlb = ((_myTZX.descriptor[i].timming.bytecfg & 0b11000000) >> 6);
            vlb = ((_myTZX.descriptor[i].timming.bytecfg & 0b00100000) >> 5);
            ntb = ((_myTZX.descriptor[i].timming.bytecfg & 0b00011000) >> 3);
            vtb = ((_myTZX.descriptor[i].timming.bitcfg & 0b00000100) >> 2);

#ifdef DEBUGMODE
            logln("");
            log("ID 0x4B:");
            log("PULSES FOR ZERO = " + String(pzero));
            log(" - " + String(_myTZX.descriptor[i].timming.bit_0) + " - ");
            log(",PULSES FOR ONE = " + String(pone));
            log(" - " + String(_myTZX.descriptor[i].timming.bit_1) + " - ");
            log(",NUMBERS OF LEADING BITS = " + String(nlb));
            log(",VALUE OF LEADING BITS = " + String(vlb));
            log(",NUMBER OF TRAILING BITS = " + String(ntb));
            log(",VALUE OF TRAILING BITS = " + String(vtb));
#endif

            // Conocemos la longitud total del bloque de datos a reproducir
            ldatos = _myTZX.descriptor[i].lengthOfData;
            // Nos quedamos con el offset inicial
            offset = _myTZX.descriptor[i].offsetData;

            // Informacion para la barra de progreso
            PRG_BAR_OFFSET_INI = offset; // offset del DATA
            PRG_BAR_OFFSET_END = offset + _myTZX.descriptor[i].lengthOfData;

            bufferD = 1024; // Buffer de BYTES de datos convertidos a samples

            partitions = ldatos / bufferD;
            lastPartitionSize = ldatos - (partitions * bufferD);

            silence = _myTZX.descriptor[i].pauseAfterThisBlock;

#ifdef DEBUGMODE
            log(",SILENCE = " + String(silence) + " ms");

            logln("");
            log("Total data parts: " + String(partitions));
            logln("");
            log("----------------------------------------");
#endif

            // TSX_PARTITIONED = false;
            PROGRESS_BAR_BLOCK_VALUE = 0;

            if (ldatos > bufferD) {
              // TSX_PARTITIONED = true;
              for (int n = 0; n < partitions && !STOP && !PAUSE; n++) {
                if (n == 0) {
                  begin = true;
                } else {
                  begin = false;
                }

#ifdef DEBUGMODE
                logln("");
                log("Part [" + String(n) + "] - offset: ");
                logHEX(offset);
#endif

                prepareID4B(i, _mFile, nlb, vlb, ntb, vtb, pzero, pone, offset,
                            bufferD, begin);
                // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud
                // contenidos en un array y repetición

                PRG_BAR_OFFSET_INI = 0;
                PRG_BAR_OFFSET_END = ldatos;

                _zxp.playCustomSequence(
                    _myTZX.descriptor[i].timming.pulse_seq_array,
                    _myTZX.descriptor[i].timming.pulse_seq_num_pulses, 0.0);
                // Avanzamos el puntero por el fichero
                offset += bufferD;

                PROGRESS_BAR_BLOCK_VALUE =
                    ((PRG_BAR_OFFSET_INI + (offset)) * 100) /
                    PRG_BAR_OFFSET_END;

                // Liberamos el array
                free(_myTZX.descriptor[i].timming.pulse_seq_array);
                // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
              }

              if (!STOP && !PAUSE) {
                // Ultima particion
                PRG_BAR_OFFSET_INI = 0;
                PRG_BAR_OFFSET_END = ldatos;

                PROGRESS_BAR_BLOCK_VALUE = 0;

                prepareID4B(i, _mFile, nlb, vlb, ntb, vtb, pzero, pone, offset,
                            lastPartitionSize, false);
                // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud
                // contenidos en un array y repetición

                _zxp.playCustomSequence(
                    _myTZX.descriptor[i].timming.pulse_seq_array,
                    _myTZX.descriptor[i].timming.pulse_seq_num_pulses, 0.0);
                // Liberamos el array
                free(_myTZX.descriptor[i].timming.pulse_seq_array);
                // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
                PROGRESS_BAR_BLOCK_VALUE =
                    ((PRG_BAR_OFFSET_INI + (ldatos)) * 100) /
                    PRG_BAR_OFFSET_END;
                // Pausa despues de bloque
                _zxp.silence(silence);
              }

#ifdef DEBUGMODE
              logln("Finish");
#endif

            } else {
#ifdef DEBUGMODE
              logln(" - Only one data part");
#endif

              if (!STOP && !PAUSE) {

                PRG_BAR_OFFSET_INI = 0;
                PRG_BAR_OFFSET_END = ldatos;
                PROGRESS_BAR_BLOCK_VALUE = 0;

                // Una sola partiase 43ion
                PRG_BAR_OFFSET_INI = _myTZX.descriptor[i].offsetData;

                prepareID4B(i, _mFile, nlb, vlb, ntb, vtb, pzero, pone, offset,
                            ldatos, true);

                // ID 0x4B - Reproducimos una secuencia. Pulsos de longitud
                // contenidos en un array y repetición
                _zxp.playCustomSequence(
                    _myTZX.descriptor[i].timming.pulse_seq_array,
                    _myTZX.descriptor[i].timming.pulse_seq_num_pulses, 0);
                PROGRESS_BAR_BLOCK_VALUE =
                    ((PRG_BAR_OFFSET_INI + (ldatos)) * 100) /
                    PRG_BAR_OFFSET_END;

                // Liberamos el array
                free(_myTZX.descriptor[i].timming.pulse_seq_array);
                // delete[] _myTZX.descriptor[i].timming.pulse_seq_array;
                // Pausa despues de bloque
                _zxp.silence(silence);
              }
            }
            break;
          }

          case 18: {
// ID 0x12 - Reproducimos un tono puro. Pulso repetido n veces
#ifdef DEBUGMODE
            log("ID 0x12:");
#endif
            _zxp.playPureTone(
                _myTZX.descriptor[i].timming.pure_tone_len,
                _myTZX.descriptor[i].timming.pure_tone_num_pulses);
            break;
          }

          case 19: {
            // ID 0x13 - Reproducimos una secuencia. Pulsos de longitud
            // contenidos en un array y repetición
#ifdef DEBUGMODE
            logln("ID 0x13:");
            logln("Num. pulses: " +
                  String(_myTZX.descriptor[i].timming.pulse_seq_num_pulses));
            for (int j = 0;
                 j < _myTZX.descriptor[i].timming.pulse_seq_num_pulses; j++) {
              SerialHW.print(_myTZX.descriptor[i].timming.pulse_seq_array[j],
                             HEX);
              SerialHW.print(",");
            }
#endif
            _zxp.playCustomSequence(
                _myTZX.descriptor[i].timming.pulse_seq_array,
                _myTZX.descriptor[i].timming.pulse_seq_num_pulses);
            break;
          }
            // case 19:
            // {
            //   // ID 0x19 - Generalized Data Block
            //   #ifdef DEBUGMODE
            //       logln("ID 0x19: Generalized Data Block");
            //       logln("TOTP: " + String(_myTZX.descriptor[i].symbol.TOTP));
            //       logln("TOTD: " + String(_myTZX.descriptor[i].symbol.TOTD));
            //   #endif
            //   _zxp.playGDB(&_myTZX.descriptor[i].symbol);
            //   _zxp.silence(_myTZX.descriptor[i].pauseAfterThisBlock);
            //   break;
            // }

          case 20: {
            // ID 0x14
            int blockSizeSplit = SIZE_FOR_SPLIT;

            if (_myTZX.descriptor[i].size > blockSizeSplit) {

              int totalSize = _myTZX.descriptor[i].size;

              PARTITION_SIZE = totalSize;

              int offsetBase = _myTZX.descriptor[i].offsetData;
              int newOffset = 0;
              double blocks = totalSize / blockSizeSplit;
              int lastBlockSize = totalSize - (blocks * blockSizeSplit);

              // BTI 0
              _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
              // BIT1
              _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;

              PRG_BAR_OFFSET_INI = _myTZX.descriptor[i].offsetData;
              // BYTES_BASE = _myTZX.descriptor[i].offsetData;

              // Recorremos el vector de particiones del bloque.
              for (int n = 0; n < blocks; n++) {
                // Calculamos el offset del bloque
                newOffset = offsetBase + (blockSizeSplit * n);
                // Accedemos a la SD y capturamos el bloque del fichero
                bufferPlay =
                    (uint8_t *)ps_calloc(blockSizeSplit, sizeof(uint8_t));
                readFileRange(_mFile, bufferPlay, newOffset, blockSizeSplit,
                              true);
                // Mostramos en la consola los primeros y últimos bytes
                // showBufferPlay(bufferPlay,blockSizeSplit,newOffset);

                PRG_BAR_OFFSET_INI = newOffset;
                // BYTES_BASE = _myTZX.descriptor[i].offsetData;

                // PROGRESS_BAR_TOTAL_VALUE = (PRG_BAR_OFFSET_INI * 100 ) /
                // BYTES_TOBE_LOAD ; PROGRESS_BAR_BLOCK_VALUE =
                // (PRG_BAR_OFFSET_INI * 100 ) / (BYTES_BASE +
                // BYTES_IN_THIS_BLOCK);

#ifdef DEBUGMODE
                log("Block. " + String(n));
                SerialHW.print(newOffset, HEX);
                SerialHW.print(" - ");
                SerialHW.print(newOffset + blockSizeSplit, HEX);
                log("");
                for (int j = 0; j < blockSizeSplit; j++) {
                  SerialHW.print(bufferPlay[j], HEX);
                  SerialHW.print(",");
                }
                // (añadido el 26/03/2024)
                // Reproducimos la partición n, del bloque.
                _zxp.playDataPartition(bufferPlay, blockSizeSplit);

#else
                // Reproducimos la partición n, del bloque.
                _zxp.playDataPartition(bufferPlay, blockSizeSplit);
#endif

                free(bufferPlay);
              }

              // Ultimo bloque
              // Calculamos el offset del último bloque
              newOffset = offsetBase + (blockSizeSplit * blocks);
              blockSizeSplit = lastBlockSize;
              PRG_BAR_OFFSET_INI = newOffset;
              // BYTES_BASE = _myTZX.descriptor[i].offsetData;

              // PROGRESS_BAR_TOTAL_VALUE = (PRG_BAR_OFFSET_INI * 100 ) /
              // BYTES_TOBE_LOAD ; PROGRESS_BAR_BLOCK_VALUE =
              // (PRG_BAR_OFFSET_INI * 100 ) / (_myTZX.descriptor[i].offset +
              // BYTES_IN_THIS_BLOCK);

              // Accedemos a la SD y capturamos el bloque del fichero
              bufferPlay =
                  (uint8_t *)ps_calloc(blockSizeSplit, sizeof(uint8_t));
              readFileRange(_mFile, bufferPlay, newOffset, blockSizeSplit,
                            true);
              // Mostramos en la consola los primeros y últimos bytes
              // showBufferPlay(bufferPlay,blockSizeSplit,newOffset);

#ifdef DEBUGMODE
              log("Block. LAST");
              SerialHW.print(newOffset, HEX);
              SerialHW.print(" - ");
              SerialHW.print(newOffset + blockSizeSplit, HEX);
              log("");
              for (int j = 0; j < blockSizeSplit; j++) {
                SerialHW.print(bufferPlay[j], HEX);
                SerialHW.print(",");
              }
#else
              // Reproducimos el ultimo bloque con su terminador y silencio si
              // aplica
              _zxp.playPureData(bufferPlay, blockSizeSplit);
#endif

              free(bufferPlay);
            } else {
              bufferPlay = (uint8_t *)ps_calloc(_myTZX.descriptor[i].size,
                                                sizeof(uint8_t));
              readFileRange(_mFile, bufferPlay, _myTZX.descriptor[i].offsetData,
                            _myTZX.descriptor[i].size, true);

              // showBufferPlay(bufferPlay,_myTZX.descriptor[i].size,_myTZX.descriptor[i].offsetData);
              // verifyChecksum(_mFile,_myTZX.descriptor[i].offsetData,_myTZX.descriptor[i].size);

              // BTI 0
              _zxp.BIT_0 = _myTZX.descriptor[i].timming.bit_0;
              // BIT1
              _zxp.BIT_1 = _myTZX.descriptor[i].timming.bit_1;
              //
              _zxp.playPureData(bufferPlay, _myTZX.descriptor[i].size);
              free(bufferPlay);
            }
            break;
          }

          case 24: {
            logln("Playing CSW Block (ID 0x18)");
            int num_pulses = _myTZX.descriptor[i].timming.csw_num_pulses;
            tRlePulse *pulse_data = _myTZX.descriptor[i].timming.csw_pulse_data;
            float csw_sampling_rate =
                (float)_myTZX.descriptor[i].timming.csw_sampling_rate;

            if (csw_sampling_rate == 0) {
              logln("ERROR: CSW block has a sampling rate of 0.");
              break;
            }

            // FÓRMULA DE CONVERSIÓN UNIVERSAL
            // Calcula el factor para convertir la unidad de duración del CSW a
            // T-States de 3.5MHz.
            float conversion_factor = (float)DfreqCPU / csw_sampling_rate;
            logln("  - CSW Conversion Factor to T-States: " +
                  String(conversion_factor));

            if (pulse_data && num_pulses > 0) {

              ADD_ONE_SAMPLE_COMPENSATION = false;

              for (int p = 0; p < num_pulses; p++) {
                if (stopOrPauseRequest())
                  break;

                uint16_t repeat = pulse_data[p].repeat;

                // Aplica el factor para obtener los T-States normalizados.
                uint32_t pulse_len_tstates = (uint32_t)round(
                    (float)pulse_data[p].pulse_len * conversion_factor);

                if (pulse_len_tstates == 0)
                  continue;

                // Llama a playCustomSymbol con el valor en T-States.
                // ZXProcessor se encargará de la conversión final a muestras de
                // audio para 32150Hz.
                _zxp.playCustomSymbol(pulse_len_tstates, repeat);
              }
            }

            _zxp.silence(_myTZX.descriptor[i].pauseAfterThisBlock);
            break;
          }

          case 25: {
            // ID 0x19 - Generalized data block
            logln("");
            logln("Playing generalized data block - ID 0x19");
            logln("Size: " + String(_myTZX.descriptor[i].size) + " bytes");

            // Para ID 0x19
            int TOTP = 0;
            int NPP = 0;
            int ASP = 0;
            // Para data
            int TOTD = 0;
            int NPD = 0;
            int ASD = 0;

            int symbolID = 0;
            int polarity = 0;
            int pulseLength = 0;
            int repeat = 0;

            // Para pilot y sync
            TOTP = _myTZX.descriptor[i].symbol.TOTP;
            NPP = _myTZX.descriptor[i].symbol.NPP;
            ASP = _myTZX.descriptor[i].symbol.ASP;

            int totalSteps =
                TOTP + TOTD; // El trabajo total es la suma de ambas fases
            int completedSteps = 0;
            PROGRESS_BAR_BLOCK_VALUE = 0;

            // Inicializar offset para barra de progreso total
            PRG_BAR_OFFSET_INI = _myTZX.descriptor[i].offsetData;
            int gdbBlockSize = _myTZX.descriptor[i].size;

#ifdef DEBUGMODE
            logln("");
            logln("Symbol definitions:");
            logln("TOTP: " + String(TOTP));
            logln("NPP: " + String(NPP));
            logln("ASP: " + String(ASP));
#endif

            // Para data
            TOTD = _myTZX.descriptor[i].symbol.TOTD;
            NPD = _myTZX.descriptor[i].symbol.NPD;
            ASD = _myTZX.descriptor[i].symbol.ASD;

            // #ifdef DEBUGMODE
            logln("");
            logln("Data definitions:");
            logln("TOTD: " + String(TOTD));
            logln("NPD: " + String(NPD));
            logln("ASD: " + String(ASD));
            logln("");
// #endif

// Mostramos las definiciones de simbolos
#ifdef DEBUGMODE
            logln("Symbol definitions table:");
            for (int k1 = 0; k1 < ASP; k1++) {
              logln(
                  "Symbol Def [" + String(k1) + "]: Polarity: " +
                  String(
                      _myTZX.descriptor[i].symbol.symDefPilot[k1].symbolFlag));
              for (int s1 = 0; s1 < NPP; s1++) {
                logln("  Pulse [" + String(s1) + "]: Length: " +
                      String(_myTZX.descriptor[i]
                                 .symbol.symDefPilot[k1]
                                 .pulse_array[s1]));
              }
            }

            for (int m1 = 0; m1 < TOTP; m1++) {
              log(String(_myTZX.descriptor[i].symbol.pilotStream[m1].symbol) +
                  "," +
                  String(_myTZX.descriptor[i].symbol.pilotStream[m1].repeat) +
                  "; ");
            }
            logln("");
#endif

            // PILOT / SYNC
            // Recorremos el array de pulsos para
            // -------------------------------------------------------------

            ADD_ONE_SAMPLE_COMPENSATION = false;

            // El nivel inicial del bloque GDB depende del nivel final del
            // bloque anterior. Solo se fuerza si el símbolo tiene polarity=2
            // (force LOW) o polarity=3 (force HIGH). Con polarity=0 (opposite
            // to current), simplemente alterna desde el nivel actual.

            for (int j1 = 0; j1 < TOTP; j1++) {
              // Cojo el ID del simbolo que necesito
              symbolID = _myTZX.descriptor[i].symbol.pilotStream[j1].symbol;
              // y las veces que se repite el SIMBOLO COMPLETO
              repeat = _myTZX.descriptor[i].symbol.pilotStream[j1].repeat;

              if (STOP || PAUSE)
                break; // Comprobar parada/pausa antes de cada byte

              // Verificamos que el simbolo es correcto
              if (symbolID < ASP) {
                // Ahora leo el simbolo de la tabla de definiciones
                // para obtener sus características

                // Obtenemos el tipo de polarización (solo bits 0-1 son válidos)
                polarity = _myTZX.descriptor[i]
                               .symbol.symDefPilot[symbolID]
                               .symbolFlag &
                           0x03;
#ifdef DEBUGMODE
                logln("[" + String(j1) + "] Symbol ID: " + String(symbolID) +
                      " - Repeat: " + String(repeat) +
                      " - Polarity: " + String(polarity));
#endif

                // ******************************************************************************
                // CORRECCIÓN GDB: El símbolo COMPLETO se repite 'repeat' veces
                // No cada pulso individual
                // ******************************************************************************
                for (int rep = 0; rep < repeat; rep++) {
                  // CORRECCIÓN: La polaridad solo se aplica al PRIMER pulso de
                  // cada repetición
                  bool firstPulseOfSymbol = true;

                  // Cada definición puede tener varios semi-pulsos
                  // Leemos cada semi-pulso del simbolo
                  for (int r = 0; r < NPP; r++) {
                    // Cogemos la definición del ancho del pulso identificado en
                    // el array
                    pulseLength = _myTZX.descriptor[i]
                                      .symbol.symDefPilot[symbolID]
                                      .pulse_array[r];

                    // Si el pulso es 0, indica fin del símbolo (spec TZX)
                    if (pulseLength == 0)
                      break;

#ifdef DEBUGMODE
                    logln("Pulse Length: " + String(pulseLength));
#endif

                    // La polaridad solo afecta al primer pulso de cada
                    // repetición del símbolo
                    if (firstPulseOfSymbol) {
                      switch (polarity) {
                      case 0: {
                        // Toggle: cambiar nivel
                        KEEP_CURRENT_EDGE = false;
                        _zxp.playCustomSymbol(pulseLength, 1);
                      } break;

                      case 1: {
                        // Same: mantener nivel actual
                        KEEP_CURRENT_EDGE = true;
                        _zxp.playCustomSymbol(pulseLength, 1);
                      } break;

                      case 2: {
                        // Force low
                        KEEP_CURRENT_EDGE = true;
                        EDGE_EAR_IS = down;
                        _zxp.playCustomSymbol(pulseLength, 1);
                      } break;

                      case 3: {
                        // Force high
                        KEEP_CURRENT_EDGE = true;
                        EDGE_EAR_IS = up;
                        _zxp.playCustomSymbol(pulseLength, 1);
                      } break;
                      }
                      firstPulseOfSymbol = false;
                    } else {
                      // Pulsos siguientes: siempre alternar
                      KEEP_CURRENT_EDGE = false;
                      _zxp.playCustomSymbol(pulseLength, 1);
                    }
                  }
                }
              }
            }

            // ------------------------------------------------------------------
            // DATA STREAM
            //
            // Recorremos el data stream bit a bit según NB (bits por símbolo)
            // -------------------------------------------------------------------

            // Calculamos cuántos bits necesita cada símbolo (mínimo NB tal que
            // 2^NB >= ASD)
            int NB = 0;
            int tmpASD = ASD - 1;
            while (tmpASD > 0) {
              NB++;
              tmpASD >>= 1;
            }
            int DS = ceil((float)(NB * TOTD) / 8.0);

#ifdef DEBUGMODE
            logln("Data stream:");
            logln("  NB (bits per symbol): " + String(NB));
            logln("  DS (stream bytes): " + String(DS));
            logln("  TOTD (total symbols): " + String(TOTD));
#endif

            // Variables para lectura de bitstream
            int current_byte_idx = 0;
            int current_bit_pos = 7; // Comenzamos desde el MSB
            int symbols_read = 0;
            int max_bits = NB * TOTD; // Total bits a leer
            int bit_index = 0;        // Índice global de bit

            // Iteramos hasta leer TOTD símbolos (NO todos los bits del stream)

            ADD_ONE_SAMPLE_COMPENSATION = false;

            while (symbols_read < TOTD && bit_index < max_bits) {
              if (STOP || PAUSE)
                break;

#ifdef DEBUGMODE
              logln("Leyendo símbolo #" + String(symbols_read) + ":");
#endif
              symbolID = 0;
              for (int bit = 0; bit < NB && bit_index < max_bits; bit++) {
                int byte_idx = bit_index / 8;
                int bit_pos = 7 - (bit_index % 8); // MSB first from byte
                uint8_t current_byte =
                    _myTZX.descriptor[i].symbol.dataStream[byte_idx];
                uint8_t bit_value = (current_byte >> bit_pos) & 1;
#ifdef DEBUGMODE
                logln("  Byte[" + String(byte_idx) + "] = 0x" +
                      String(current_byte, HEX) + ", bit " + String(bit_pos) +
                      " = " + String(bit_value));
#endif
                // CORRECCIÓN: El primer bit leído es el MSB del símbolo
                // Desplazar symbolID a la izquierda y añadir el nuevo bit como
                // LSB
                symbolID = (symbolID << 1) | bit_value;
                bit_index++;
              }
#ifdef DEBUGMODE
              logln("  symbolID = " + String(symbolID));
#endif

// DEBUG: Mostrar bytes del datastream según se procesan
// -------------------------------------------------------------
#ifdef DEBUGMODE
              if (bit_index % 8 == 0 && bit_index > 0) {
                int completed_byte_idx = (bit_index / 8) - 1;
                uint8_t original_byte =
                    _myTZX.descriptor[i].symbol.dataStream[completed_byte_idx];
                String hex = String(original_byte, HEX);
                if (hex.length() == 1)
                  hex = "0" + hex;
                log("0x" + hex + ",");
              }
              // -------------------------------------------------------------
#endif

              // Verificamos que el símbolo es válido
              if (symbolID < ASD) {
                // Obtenemos la polaridad del símbolo
                polarity = _myTZX.descriptor[i]
                               .symbol.symDefData[symbolID]
                               .symbolFlag &
                           0x03;

#ifdef DEBUGMODE
                logln("Symbol #" + String(symbols_read) + ": ID=" +
                      String(symbolID) + ", Polarity=" + String(polarity));
#endif

                // CORRECCIÓN: La polaridad solo se aplica al PRIMER pulso del
                // símbolo Los pulsos siguientes alternan automáticamente (como
                // polarity=0) Spec TZX: "The polarity of the first pulse of the
                // first symbol of a block
                //           is the polarity of the last pulse of the previous
                //           block"

                // Leemos cada semi-pulso que define el símbolo
                for (int r1 = 0; r1 < NPD; r1++) {
                  if (STOP || PAUSE)
                    break;

                  bool firstPulseOfSymbol = true;
                  // Cogemos la longitud del pulso
                  pulseLength = _myTZX.descriptor[i]
                                    .symbol.symDefData[symbolID]
                                    .pulse_array[r1];

                  // Si el pulso es 0, indica fin del símbolo (spec TZX)
                  if (pulseLength == 0)
                    break;

#ifdef DEBUGMODE
                  log("  Pulse[" + String(r1) + "]: " + String(pulseLength) +
                      "T");
#endif

                  // Reproducimos el pulso
                  // La polaridad solo afecta al primer pulso del símbolo
                  if (firstPulseOfSymbol) {
                    switch (polarity) {
                    case 0: {
                      // Toggle: cambiar nivel respecto al actual
                      KEEP_CURRENT_EDGE = false;
                      _zxp.playCustomSymbol(pulseLength, 1);
                    } break;

                    case 1: {
                      // Same: mantener el nivel actual (sin cambio)
                      KEEP_CURRENT_EDGE = true;
                      _zxp.playCustomSymbol(pulseLength, 1);
                    } break;

                    case 2: {
                      // Force low: forzar nivel bajo para el primer pulso
                      KEEP_CURRENT_EDGE = true;
                      EDGE_EAR_IS = down;
                      _zxp.playCustomSymbol(pulseLength, 1);
                    } break;

                    case 3: {
                      // Force high: forzar nivel alto para el primer pulso
                      KEEP_CURRENT_EDGE = true;
                      EDGE_EAR_IS = up;
                      _zxp.playCustomSymbol(pulseLength, 1);
                    } break;
                    }
                    firstPulseOfSymbol = false;
                  } else {
                    // Pulsos siguientes: siempre alternar (toggle)
                    KEEP_CURRENT_EDGE = false;
                    _zxp.playCustomSymbol(pulseLength, 1);
                  }
                }
              } else {
#ifdef DEBUGMODE
                logln("WARNING: Invalid symbolID " + String(symbolID) +
                      " (ASD=" + String(ASD) + ")");
#endif
              }

              symbols_read++;

              // Actualizamos la barra de progreso
              if (TOTD > 0) {
                PROGRESS_BAR_BLOCK_VALUE = (symbols_read * 100) / TOTD;
                // Calcular bytes equivalentes procesados dentro del bloque GDB
                int block_bytes_processed =
                    (gdbBlockSize * symbols_read) / TOTD;
                PROGRESS_BAR_TOTAL_VALUE =
                    ((PRG_BAR_OFFSET_INI + block_bytes_processed) * 100) /
                    BYTES_TOBE_LOAD;
              }
            }

            // ✅ ASEGURAR QUE LA BARRA LLEGUE AL 100% AL FINALIZAR
            PROGRESS_BAR_BLOCK_VALUE = 100;
            // Actualizar progreso total al finalizar el bloque GDB
            PROGRESS_BAR_TOTAL_VALUE =
                ((PRG_BAR_OFFSET_INI + gdbBlockSize) * 100) / BYTES_TOBE_LOAD;

            // Pausa despues de bloque (el silencio mantiene el nivel actual)
            _zxp.silence(_myTZX.descriptor[i].pauseAfterThisBlock);
            break;
          }
          }
        } else {
          //
          // Otros bloques de datos SIN cabecera no contemplados anteriormente -
          // DATA
          //

          int blockSize = _myTZX.descriptor[i].size;

          switch (_myTZX.descriptor[i].ID) {
          case 16: {
            // BASE_SR = STANDARD_SR_8_BIT_MACHINE;
            //  ID 0x10
            _myTZX.descriptor[i].timming.pilot_len = DPILOT_LEN;
            playBlock(_myTZX.descriptor[i]);
            break;
          }
          case 17: {
            // ID 0x11
            playBlock(_myTZX.descriptor[i]);
            break;
          }
          }
        }
      } else {
#ifdef DEBUGMODE
        logln("");
        logln("Bl: " + String(i) + " - Not playeable block");
        logln("");
#endif
      }

      break;
    }

    return newPosition;
  }

  void play() {
    LOOP_PLAYED = 0;

    int firstBlockToBePlayed = 0;
    int dly = 0;

    // Inicializamos 02.03.2024
    LOOP_START = 0;
    LOOP_END = 0;
    BL_LOOP_START = 0;
    BL_LOOP_END = 0;

    if (_myTZX.descriptor != nullptr) {
      // Entregamos información por consola
      TOTAL_BLOCKS = _myTZX.numBlocks;
      strcpy(LAST_NAME, "              ");

      // Ahora reproducimos todos los bloques desde el seleccionado (para cuando
      // se quiera uno concreto)
      firstBlockToBePlayed = BLOCK_SELECTED;

      // Reiniciamos
      BYTES_TOBE_LOAD = _rlen;
      BYTES_LOADED = 0;

// Recorremos ahora todos los bloques que hay en el descriptor
//-------------------------------------------------------------
#ifdef DEBUGMODE
      logln("Total blocks " + String(_myTZX.numBlocks));
      logln("First block " + String(firstBlockToBePlayed));
      logln("");
#endif

      // Inicializamos el nivel de la señal según la polarización seleccionada
      // EDGE_EAR_IS = INVERSETRAIN ? POLARIZATION ^ 1: POLARIZATION;

      for (int i = firstBlockToBePlayed; i < _myTZX.numBlocks; i++) {

        KEEP_CURRENT_EDGE =
            false; // Por defecto, cada bloque puede cambiar el nivel de la
                   // señal. Solo se mantiene si el bloque lo indica
                   // expresamente (polarity 1 en GDB o flag correspondiente en
                   // otros bloques)

        ADD_ONE_SAMPLE_COMPENSATION = false;

        // Lo guardo por si hago PAUSE y vuelvo a reproducir el bloque
        _myTZX.descriptor[i].edge = EDGE_EAR_IS;

        BLOCK_SELECTED = i;

        if (BLOCK_SELECTED == 0) {
          PRG_BAR_OFFSET_INI = 0;
        } else {
          PRG_BAR_OFFSET_INI = _myTZX.descriptor[BLOCK_SELECTED].offset;
        }

        _hmi.setBasicFileInformation(
            _myTZX.descriptor[i].ID, _myTZX.descriptor[i].group,
            _myTZX.descriptor[i].name, _myTZX.descriptor[i].typeName,
            _myTZX.descriptor[i].size, _myTZX.descriptor[i].playeable);
        int new_i = getIDAndPlay(i);
        // Entonces viene cambiada de un loop
        if (new_i != -1) {
          i = new_i;
        }

        if (LOADING_STATE == 2 || LOADING_STATE == 3) {
          break;
        }

        if (stopOrPauseRequest()) {
          // Forzamos la salida
          i = _myTZX.numBlocks + 1;
          break;
        }
      }
      //---------------------------------------------------------------

      // En el caso de no haber parado manualmente, es por finalizar
      // la reproducción
      if (LOADING_STATE == 1) {
        // ******************************************************************************
        // HACK TZX: Tail pulse para el último bloque de la cinta
        // "Sometimes a tape might have its last tail pulse missing.
        // In case it's the last block in the tape, it's best to flip the tape
        // bit a last time to ensure that the process is terminated properly."
        // ******************************************************************************
        // if (silent == 0)
        // {
        //     _zxp.finalTailPulse();
        // }

        if (LAST_SILENCE_DURATION == 0) {
          // Ponemos el tail de duración 1s - 3500000 TStates
          logln("Appliying final SILENCE TAIL");
          _zxp.silence((int)round((PAUSE_TAIL_TSTATES / DfreqCPU)) * 1000);
        }

// Inicializamos la polarización de la señal
// EDGE_EAR_IS = POLARIZATION;

// Paramos
#ifdef DEBUGMODE
        logAlert("AUTO STOP launch.");
#endif

        PLAY = false;
        PAUSE = false;
        STOP = true;
        REC = false;
        ABORT = true;
        EJECT = false;

        BLOCK_SELECTED = 0;
        BYTES_LOADED = 0;

        AUTO_STOP = true;

        _hmi.setBasicFileInformation(
            _myTZX.descriptor[BLOCK_SELECTED].ID,
            _myTZX.descriptor[BLOCK_SELECTED].group,
            _myTZX.descriptor[BLOCK_SELECTED].name,
            _myTZX.descriptor[BLOCK_SELECTED].typeName,
            _myTZX.descriptor[BLOCK_SELECTED].size,
            _myTZX.descriptor[BLOCK_SELECTED].playeable);
      }

      // Cerrando
      // reiniciamos el edge del ultimo pulse
      LOOP_PLAYED = 0;
      // EDGE_EAR_IS = down;
      LOOP_START = 0;
      LOOP_END = 0;
      LOOP_COUNT = 0;
      BL_LOOP_START = 0;
      BL_LOOP_END = 0;
      MULTIGROUP_COUNT = 1;
      WAITING_FOR_USER_ACTION = false;
    } else {
      LAST_MESSAGE = "No file selected.";
      _hmi.setBasicFileInformation(_myTZX.descriptor[BLOCK_SELECTED].ID,
                                   _myTZX.descriptor[BLOCK_SELECTED].group,
                                   _myTZX.descriptor[BLOCK_SELECTED].name,
                                   _myTZX.descriptor[BLOCK_SELECTED].typeName,
                                   _myTZX.descriptor[BLOCK_SELECTED].size,
                                   _myTZX.descriptor[BLOCK_SELECTED].playeable);
    }
  }

  TZXprocessor() {
    // Constructor de la clase
    strncpy(_myTZX.name, "          ", 10);
    _myTZX.numBlocks = 0;
    _myTZX.size = 0;
    _myTZX.descriptor = nullptr;
  }
};
