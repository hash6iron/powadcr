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

      String dscVersion = "v3.0";

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
        tTZXBlockDescriptor descriptor;
      };      

      struct tBlDscTAP
      {
        char* path;
        int numBlocks;
        tTAPBlockDescriptor descriptor;
      };  

    private:

        tBlDscTZX _blTZX;
        tBlDscTAP _blTAP;
        tBlDscTSX _blTSX;


    public:
        
        bool existBlockDescriptorFile(File32 mFile, char* path)
        {
            logln("Existe? " + String(path));

            if(mFile.open(path,O_READ))
            {
              mFile.close();
              logln("SI");
              return true;
            }
            else
            {
              logln("NO");
              return false;
            }
        }

        void createBlockDescriptorFileTZX(File32 &mFile, char* path)
        {
          // Creamos un fichero con el descriptor de bloques para TZX
          _blTZX.path = path;

          // Lo creamos otra vez
          if(mFile.open(_blTZX.path, O_RDWR | O_CREAT))
          {
            logln("DSC file created or overwrite");
            mFile.println(dscVersion + ",pos,ID,chk,delay,group,hasMaskLastByte,header,jump_this_ID,lenOfData,loopCount,offset,offsetData,name,nameDetected,pauseATB,playeable,samplingrate,screen,silent,size,t.Bit0,t.Bit1,t.PilotLen,t.PilotNPulses,t.PulseSeq,t.PureTone,t.PureToneNP,t.Sync1,t.Sync2,typeName,type,t.bitcfg,t.bytecfg,SizeTZX");
          }
        }

        void createBlockDescriptorFileTAP(File32 &mFile, char* path)
        {
          // Creamos un fichero con el descriptor de bloques para TAP
          _blTAP.path = path;

          // Lo creamos otra vez
          if(mFile.open(_blTAP.path,O_WRITE))
          {
            logln("DSC file created or overwrite");
            mFile.println(dscVersion + ",pos,ID,chk,delay,group,hasMaskLastByte,header,jump_this_ID,lenOfData,loopCount,offset,offsetData,name,nameDetected,pauseATB,playeable,samplingrate,screen,silent,size,t.Bit0,t.Bit1,t.PilotLen,t.PilotNPulses,t.PulseSeq,t.PureTone,t.PureToneNP,t.Sync1,t.Sync2,typeName,type,t.bitcfg,t.bytecfg,SizeTZX");
          }

        }

        void putBlocksDescriptorTZX(File32 &mFile,int pos, tTZXBlockDescriptor &descriptor, uint32_t sizeTZX)
        {
            // Creamos un fichero con el descriptor de bloques para TZX
            char name[20];
            char typeName[36];

            strncpy(name,descriptor.name,14);
            strcpy(typeName,descriptor.typeName);

            #ifdef DEBUGMODE
              logln("Writing in: " + String(_blTZX.path));
            #endif

            // Ahora vamos a pasarle todo el descriptor TZX completo
            mFile.println(String(pos) + "," + 
                          String(descriptor.ID) + "," + 
                          String(descriptor.chk) + "," +
                          String(descriptor.delay)+ "," +
                          String(descriptor.group) + "," +
                          String(descriptor.hasMaskLastByte) + "," +
                          String(descriptor.header) + "," +
                          String(descriptor.jump_this_ID) + "," +
                          String(descriptor.lengthOfData) + "," +
                          String(descriptor.loop_count) + "," +
                          String(descriptor.offset) + "," +
                          String(descriptor.offsetData) + "," +     
                          name + " ," +
                          String(descriptor.nameDetected) + "," +
                          String(descriptor.pauseAfterThisBlock) + "," +
                          String(descriptor.playeable) + "," +
                          String(descriptor.samplingRate) + "," +
                          String(descriptor.screen) + "," +
                          String(descriptor.silent) + "," +
                          String(descriptor.size) + "," +
                          String(descriptor.timming.bit_0) + "," +
                          String(descriptor.timming.bit_1) + "," +
                          String(descriptor.timming.pilot_len) + "," +
                          String(descriptor.timming.pilot_num_pulses) + "," +
                          String(descriptor.timming.pulse_seq_num_pulses) + "," +
                          String(descriptor.timming.pure_tone_len) + "," +
                          String(descriptor.timming.pure_tone_num_pulses) + "," +
                          String(descriptor.timming.sync_1) + "," +
                          String(descriptor.timming.sync_2) + "," +
                          typeName + " ," +
                          String(descriptor.type) + "," +
                          String(descriptor.timming.bitcfg) + "," +
                          String(descriptor.timming.bytecfg) + "," +
                          String(sizeTZX));              

        }

        // Para TAP

        tTAPBlockDescriptor* getBlockDescriptorTAP(File32 mFile)
        {}

        tTAPBlockDescriptor getBlockInformationTAP(File32 mFile, int blockPosition)
        {}

        tTAPBlockDescriptor convertToTAPDescritor(File32 mFile, tTAPBlockDescriptor descriptor)
        {}


        // Para TZX
        tTZXBlockDescriptor convertToTZXDescriptor(File32 mFile, tTZXBlockDescriptor descriptor)
        {}

        tTZXBlockDescriptor* getBlockDescriptorTZX(File32 mFile)
        {}

        tTZXBlockDescriptor getBlockInformationTZX(File32 mFile, int blockPosition)
        {}        

        // void putPathTZX(char* path)
        // {
        //   _blTZX.path = path;
        // }

        // void putPathTSX(char* path)
        // {
        //   _blTSX.path = path;
        // }

        // void putPathTAP(char* path)
        // {
        //   _blTAP.path = path;
        // }

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