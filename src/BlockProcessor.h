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

      String dscVersion = "v1.2";

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
    
    // Verifica que el fichero DSC tiene cabecera y al menos una línea de datos
    bool isValidDSCFile(const char* path) 
    {
        File dscFile = SD_MMC.open(path, FILE_READ);
        if (!dscFile) return false;

        // Lee la cabecera (primera línea)
        String header = dscFile.readStringUntil('\n');
        header.trim();
        if (header.length() == 0) {
            dscFile.close();
            return false; // No hay cabecera
        }

        // Lee la siguiente línea (primer dato)
        String firstData = dscFile.readStringUntil('\n');
        firstData.trim();
        dscFile.close();

        // Si hay al menos una línea de datos, es válido
        return firstData.length() > 0;
    }

        bool existBlockDescriptorFile(File mFile, char* path)
        {
            logln("Valid DSC file? " + String(path));

            if(isValidDSCFile(path))
            {
                logln("DSC file found");
                mFile.close();
                return true;
            }

            logln("DSC file not found");
            return false;
        }

        void createBlockDescriptorFileTZX(File &mFile, char* path)
        {
          // Creamos un fichero con el descriptor de bloques para TZX
          _blTZX.path = path;

          // Lo creamos otra vez
          mFile = SD_MMC.open(_blTZX.path, FILE_WRITE);
          if(mFile)
          {
            logln("DSC file created or overwrite");
            mFile.println(dscVersion + ",pos,ID,chk,delay,group,hasMaskLastByte,header,jump_this_ID,lenOfData,loopCount,offset,offsetData,name,nameDetected,pauseATB,playeable,samplingrate,screen,silent,size,t.Bit0,t.Bit1,t.PilotLen,t.PilotNPulses,t.PulseSeq,t.PureTone,t.PureToneNP,t.Sync1,t.Sync2,typeName,type,t.bitcfg,t.bytecfg,maskLastByte,hasGroupBlocks,SizeTZX,signalLvl");
          }
        }

        void putBlocksDescriptorTZX(File &mFile,int pos, tTZXBlockDescriptor &descriptor, uint32_t sizeTZX, bool hasGroupBlocks)
        {
            // Creamos un fichero con el descriptor de bloques para TZX
            char name[20];
            char typeName[36];

            strncpy(name,descriptor.name,14);
            strcpy(typeName,descriptor.typeName);

            #ifdef DEBUGMODE
              logln("Writing in: " + String(_blTZX.path));
            #endif

            if (mFile)
            {
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
                            String(descriptor.maskLastByte) + "," +
                            String(hasGroupBlocks) + "," +
                            String(sizeTZX) + "," +
                            String(descriptor.signalLvl));       
            }       

        }

        BlockProcessor()
        {}
       
};