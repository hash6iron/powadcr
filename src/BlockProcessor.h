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

#define TAPtype 0
#define TZXtype 1
#define TSXtype 2

class BlockProcessor
{

    public:



      struct tBlDscTZX
      {
        char* path;
        int numBlocks;
        tTZXBlockDescriptor descriptor;
      };

      struct tBlDscTSX
      {
        char* path;
        int numBlocks;
        tTSXBlockDescriptor descriptor;
      };      

      struct tBlDscTAP
      {
        char* path;
        int numBlocks;
        tTAPBlockDescriptor descriptor;
      };  

    private:

        SdFat32 _sdf32;
        File32 _mFile;

        tBlDscTZX _blTZX;
        tBlDscTAP _blTAP;
        tBlDscTSX _blTSX;


    public:
        void createBlockDescriptorFileTZX(File32 &mFile)
        {
          // Creamos un fichero con el descriptor de bloques para TZX
          strcat(_blTZX.path,".dsc");

          // el fichero sera, xxxx.dsc
          if(mFile.exists(_blTZX.path))
          {
            mFile.remove(_blTZX.path);
          }
          // Lo creamos otra vez
          mFile.open(_blTZX.path,O_WRITE | O_CREAT | O_TRUNC);
        }

        void createBlockDescriptorFileTSX(File32 &mFile)
        {
          // Creamos un fichero con el descriptor de bloques para TSX
          strcat(_blTSX.path,".dsc");

          // el fichero sera, xxxx.dsc
          if(mFile.exists(_blTSX.path))
          {
            mFile.remove(_blTSX.path);
          }
          // Lo creamos otra vez
          mFile.open(_blTSX.path,O_WRITE | O_CREAT | O_TRUNC);
        }

        void createBlockDescriptorFileTAP(File32 &mFile)
        {
          // Creamos un fichero con el descriptor de bloques para TAP
          strcat(_blTAP.path,".dsc");

          // el fichero sera, xxxx.dsc
          if(mFile.exists(_blTAP.path))
          {
            mFile.remove(_blTAP.path);
          }
          // Lo creamos otra vez
          mFile.open(_blTAP.path,O_WRITE | O_CREAT | O_TRUNC);
        }

        void putBlocksDescriptorTSX(File32 &mFile,int pos,tTSXBlockDescriptor &descriptor)
        {
            // Ahora vamos a pasarle todo el descriptor TSX completo
            mFile.println(String(pos) + "," + 
                          String(descriptor.ID) + "," + 
                          String(descriptor.chk) +
                          String(descriptor.delay)+
                          String(descriptor.group) +
                          String(descriptor.hasMaskLastByte) +
                          String(descriptor.header) +
                          String(descriptor.jump_this_ID) +
                          String(descriptor.lengthOfData) +
                          String(descriptor.loop_count) +
                          String(descriptor.offset) + "," + 
                          String(descriptor.offsetData) +
                          String(descriptor.lengthOfData) +
                          String(descriptor.name) +
                          String(descriptor.nameDetected) +
                          String(descriptor.pauseAfterThisBlock) +
                          String(descriptor.playeable) +
                          String(descriptor.samplingRate) +
                          String(descriptor.screen) +
                          String(descriptor.silent) +
                          String(descriptor.size) +
                          String(descriptor.timming.bit_0) +
                          String(descriptor.timming.bit_1) +
                          String(descriptor.timming.bitcfg) +
                          String(descriptor.timming.bytecfg) +
                          String(descriptor.timming.pilot_len) +
                          String(descriptor.timming.pilot_num_pulses) +
                          String(descriptor.timming.pulse_seq_num_pulses) +
                          String(descriptor.timming.pure_tone_len) +
                          String(descriptor.timming.pure_tone_num_pulses) +
                          String(descriptor.timming.sync_1) +
                          String(descriptor.timming.sync_2));

            mFile.close();

            // Ahora pasamos todo al descriptor global
            _blTSX.descriptor = descriptor;
        }

        void putBlocksDescriptorTZX(File32 &mFile,int pos, tTZXBlockDescriptor &descriptor)
        {
            // Ahora vamos a pasarle todo el descriptor TZX completo
            mFile.println(String(pos) + "," + 
                          String(descriptor.ID) + "," + 
                          String(descriptor.chk) +
                          String(descriptor.delay)+
                          String(descriptor.group) +
                          String(descriptor.hasMaskLastByte) +
                          String(descriptor.header) +
                          String(descriptor.jump_this_ID) +
                          String(descriptor.lengthOfData) +
                          String(descriptor.loop_count) +
                          String(descriptor.offset) + "," + 
                          String(descriptor.offsetData) +
                          String(descriptor.lengthOfData) +
                          String(descriptor.name) +
                          String(descriptor.nameDetected) +
                          String(descriptor.pauseAfterThisBlock) +
                          String(descriptor.playeable) +
                          String(descriptor.samplingRate) +
                          String(descriptor.screen) +
                          String(descriptor.silent) +
                          String(descriptor.size) +
                          String(descriptor.timming.bit_0) +
                          String(descriptor.timming.bit_1) +
                          String(descriptor.timming.pilot_len) +
                          String(descriptor.timming.pilot_num_pulses) +
                          String(descriptor.timming.pulse_seq_num_pulses) +
                          String(descriptor.timming.pure_tone_len) +
                          String(descriptor.timming.pure_tone_num_pulses) +
                          String(descriptor.timming.sync_1) +
                          String(descriptor.timming.sync_2));

            mFile.close();

            // Ahora pasamos todo al descriptor global
            _blTZX.descriptor = descriptor;
        }

        tTZXBlockDescriptor* getBlockDescriptorTZX(File32 mFile)
        {}

        tTZXBlockDescriptor getBlockInformationTZX(File32 mFile, int blockPosition)
        {}

        tTSXBlockDescriptor* getBlockDescriptorTSX(File32 mFile)
        {}

        tTSXBlockDescriptor getBlockInformationTSX(File32 mFile, int blockPosition)
        {}

        tTAPBlockDescriptor* getBlockDescriptorTAP(File32 mFile)
        {}

        tTAPBlockDescriptor getBlockInformationTAP(File32 mFile, int blockPosition)
        {}

        tTAPBlockDescriptor convertToTAPDescritor(File32 mFile, tTAPBlockDescriptor descriptor)
        {}

        tTSXBlockDescriptor convertToTSXDescriptor(File32 mFile, tTSXBlockDescriptor descriptor)
        {}

        tTZXBlockDescriptor convertToTZXDescriptor(File32 mFile, tTZXBlockDescriptor descriptor)
        {}

        void putPathTZX(char* path)
        {
          _blTZX.path = path;
        }

        void putPathTSX(char* path)
        {
          _blTSX.path = path;
        }

        void putPathTAP(char* path)
        {
          _blTAP.path = path;
        }

        BlockProcessor()
        {
            // switch (type)
            // {

            // case 0:
            //   _blTAP.path = {"/0"};
            //   _blTAP.numBlocks = 0;
            //   break;

            // case 1:
            //   _blTZX.path = {"/0"};
            //   _blTZX.numBlocks = 0;
            //   break;

            // case 2:
            //   _blTSX.path = {"/0"};
            //   _blTSX.numBlocks = 0;
            //   break;

            // default:
            //   break;
            // }
        }
       
};