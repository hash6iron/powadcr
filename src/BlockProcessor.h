/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: BlockProcessor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Clase para el manejo de bloques
   
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

class blockProcessor
{

    public:

      struct tTimming
      {
        int bit_0 = 855;
        int bit_1 = 1710;
        int pilot_len = 2168;
        int pilot_num_pulses = 0;
        int sync_1 = 667;
        int sync_2 = 735;
        int pure_tone_len = 0;
        int pure_tone_num_pulses = 0;
        int pulse_seq_num_pulses = 0;
        int* pulse_seq_array = nullptr;
        int bitcfg;
        int bytecfg;
      };

      struct tBlockDescriptor 
      {
        int ID = 0;
        int offset = 0;
        int size = 0;
        int chk = 0;
        int pauseAfterThisBlock = 1000;   //ms
        int lengthOfData = 0;
        int offsetData = 0;
        char name[15];
        bool nameDetected = false;
        bool header = false;
        bool screen = false;
        int type = 0;
        bool playeable = false;
        int delay = 1000;
        int silent;
        int maskLastByte = 8;
        bool hasMaskLastByte = false;
        tTimming timming;
        char typeName[36];
        int group = 0;
        int loop_count = 0;
        bool jump_this_ID = false;
        int samplingRate = 79;
      };

      // Estructura tipo TSX
      struct tFile
      {
        char name[11];                                  // Nombre del programa
        uint32_t size = 0;                              // Tamaño
        int numBlocks = 0;                              // Numero de bloques
        tBlockDescriptor* descriptor = nullptr;         // Descriptor
      };


    private:

        SdFat32 _sdf32;
        File32 _mFile;
        TAPprocessor fTAP;
        TZXprocessor fTZX;
        TSXprocessor fTSX;


    public:
        void createBlockDescriptorFile(File32 mFile)
        {}

        void putBlocksDescriptor(File32 mFile, tBlockDescriptor* descriptor)
        {}

        tBlockDescriptor* getBlockDescriptor(File32 mFile)
        {}

        tBlockDescriptor getBlockInformation(File32 mFile, int blockPosition)
        {}

        TAPprocessor::tBlockDescriptor convertToTAPDescritor(File32 mFile, tBlockDescriptor descriptor)
        {}

        TSXprocessor::tTSXBlockDescriptor convertToTSXDescriptor(File32 mFile, tBlockDescriptor descriptor)
        {}

        TZXprocessor::tTZXBlockDescriptor convertToTZXDescriptor(File32 mFile, tBlockDescriptor descriptor)
        {}

};