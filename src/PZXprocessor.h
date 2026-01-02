/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: PZXprocessor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
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

class PZXprocessor 
{
    private:

    tPZX _myPZX;
    File _mFile;
    int _sizePZX;
    int _rlen;

    // ✅ AÑADIR DEPENDENCIAS NECESARIAS (igual que en TZXprocessor)
    ZXProcessor _zxp;
    HMI _hmi;

    // --- Funciones de ayuda para leer datos Little-Endian ---
int getWORD(File mFile, int offset)
    {
        uint8_t buffer[2];
        
        mFile.seek(offset);
        mFile.read(buffer, 2);
        
        return (buffer[1] << 8) | buffer[0];  // Little-endian
    }

    // ✅ VERSIÓN OPTIMIZADA - CON BUFFER ESTÁTICO
    int getBYTE(File mFile, int offset)
    {
      uint8_t buffer[1];
      
      mFile.seek(offset);
      mFile.read(buffer, 1);
      
      return buffer[0];
    }

    // ✅ VERSIÓN OPTIMIZADA DE getNBYTE
    int getNBYTE(File mFile, int offset, int n)
    {
        if (n > 4) return 0;  // Máximo 4 bytes para un int
        
        uint8_t buffer[4];
        
        mFile.seek(offset);
        mFile.read(buffer, n);
        
        int result = 0;
        for (int i = 0; i < n; i++)
        {
            result |= (buffer[i] << (8 * i));
        }
        
        return result;
    }

    char* readTag(File &mFile, int offset) 
    {
        static char tag[5] = {0};

        mFile.seek(offset);
        mFile.read((uint8_t*)tag, 4);
        tag[4] = '\0';
        return tag;
    }

    // --- Analizadores para cada tipo de bloque PZX ---

    void analyzePZXT(File &mFile, tPZXBlockDescriptor &descriptor) {
        strncpy(descriptor.typeName, "PZX Header", 35);
        descriptor.playeable = false;
        // Aquí se podría leer la versión y la info del tape si se quisiera mostrar en la UI
    }

    void analyzeSTOP(File &mFile, tPZXBlockDescriptor &descriptor) {
        strncpy(descriptor.typeName, "PZX Stop Tape", 35);
        descriptor.playeable = true; // Es una acción reproducible
        // Leemos los 2 bytes de flags del bloque
        descriptor.stop_flags = getNBYTE(mFile, descriptor.offset + 8, 2);
        logln(" - STOP block found. Flags: " + String(descriptor.stop_flags));
    }    

    void analyzePAUS(File &mFile, tPZXBlockDescriptor &descriptor) {
        strncpy(descriptor.typeName, "PZX Pause", 35);
        descriptor.playeable = true;
        uint32_t duration_field = getNBYTE(mFile, descriptor.offset + 8, 4);
        descriptor.pause_duration = duration_field & 0x7FFFFFFF;
        descriptor.initial_level = (duration_field >> 31) & 1;

        logln(" - PAUS duration: " + String(descriptor.pause_duration) + " TStates");
    }

    void analyzePULS(File &mFile, tPZXBlockDescriptor &descriptor) {
        strncpy(descriptor.typeName, "PZX Pulse Sequence", 35);
        descriptor.playeable = true;
        descriptor.initial_level = 0; // Por defecto es LOW

        // 1. Contar cuántos pulsos hay para reservar memoria
        int pulse_count = 0;
        int current_pos = descriptor.offset + 8;
        while (current_pos < (descriptor.offset + 8 + descriptor.size)) {
            pulse_count++;
            uint16_t val1 = getNBYTE(mFile, current_pos, 2);
            current_pos += 2;
            if (val1 >= 0x8000) { // Tiene repeat count
                uint16_t val2 = getNBYTE(mFile, current_pos, 2);
                current_pos += 2;
                if (val2 >= 0x8000) { // Duración de 32 bits
                    current_pos += 2;
                }
            } else { // No tiene repeat, puede tener duración de 32 bits
                if (val1 >= 0x8000) { // Duración de 32 bits
                    current_pos += 2;
                }
            }
        }
        
        if (pulse_count == 0) return;

        descriptor.timming.pzx_num_pulses = pulse_count;
        descriptor.timming.pzx_pulse_data = (tRlePulse*)ps_calloc(pulse_count, sizeof(tRlePulse));
        if (!descriptor.timming.pzx_pulse_data) return;

        // 2. Llenar la estructura de pulsos
        current_pos = descriptor.offset + 8;
        for (int i = 0; i < pulse_count; ++i) {
            uint32_t duration;
            uint16_t count = 1;
            
            uint16_t val1 = getNBYTE(mFile, current_pos, 2);
            current_pos += 2;

            if (val1 >= 0x8000) {
                count = val1 & 0x7FFF;
                duration = getNBYTE(mFile, current_pos, 2);
                current_pos += 2;
            } else {
                duration = val1;
            }

            if (duration >= 0x8000) {
                duration &= 0x7FFF;
                duration <<= 16;
                duration |= getNBYTE(mFile, current_pos, 2);
                current_pos += 2;
            }
            
            descriptor.timming.pzx_pulse_data[i].pulse_len = duration;
            descriptor.timming.pzx_pulse_data[i].repeat = count;
        }
    }

    uint8_t getByte(File &mFile, int offset) 
    {
        uint8_t b;
        mFile.seek(offset);
        mFile.read(&b, 1);
        return b;
    }

    void analyzeDATA(File &mFile, tPZXBlockDescriptor &descriptor) 
    {
        strncpy(descriptor.typeName, "PZX Data Block", 35);
        descriptor.playeable = true;
        int data_offset = descriptor.offset + 8;

        uint32_t count_field = getNBYTE(mFile, data_offset, 4);
        descriptor.data_bit_count = count_field & 0x7FFFFFFF;
        descriptor.initial_level = (count_field >> 31) & 1;
        
        descriptor.data_tail_pulse = getNBYTE(mFile, data_offset + 4, 2);
        // ✅ CORRECCIÓN: Usar una función de ayuda para leer un solo byte
        descriptor.data_p0_count = getByte(mFile, data_offset + 6);
        descriptor.data_p1_count = getByte(mFile, data_offset + 7);

        int s0_offset = data_offset + 8;
        if (descriptor.data_p0_count > 0) {
            descriptor.data_s0_pulses = (uint16_t*)ps_malloc(descriptor.data_p0_count * sizeof(uint16_t));
            if (descriptor.data_s0_pulses) { // Comprobar si la asignación tuvo éxito
                for (int i = 0; i < descriptor.data_p0_count; ++i) {
                    descriptor.data_s0_pulses[i] = getNBYTE(mFile, s0_offset + i * 2, 2);
                }
            }
        }

        int s1_offset = s0_offset + descriptor.data_p0_count * 2;
        if (descriptor.data_p1_count > 0) {
            descriptor.data_s1_pulses = (uint16_t*)ps_malloc(descriptor.data_p1_count * sizeof(uint16_t));
            if (descriptor.data_s1_pulses) { // Comprobar si la asignación tuvo éxito
                for (int i = 0; i < descriptor.data_p1_count; ++i) {
                    descriptor.data_s1_pulses[i] = getNBYTE(mFile, s1_offset + i * 2, 2);
                }
            }
        }
        
        descriptor.data_stream_offset = s1_offset + descriptor.data_p1_count * 2;

        // Comprobar si este bloque de datos podría ser una cabecera estándar (17 bytes)
        if (descriptor.data_bit_count / 8 >= 17)
        {
            uint8_t header_data[17];
            mFile.seek(descriptor.data_stream_offset);
            mFile.read(header_data, 17);

            // Comprobar si es una cabecera (flag 0x00) de tipo PROGRAM (tipo 0x00)
            if (header_data[0] == 0x00 && header_data[1] == 0x00)
            {
                strncpy(descriptor.typeName, "PROGRAM Header", 35);
                // Copiar los 10 bytes del nombre
                memcpy(descriptor.name, &header_data[2], 10);
                descriptor.name[10] = '\0'; // Asegurar el terminador nulo
                logln("Found PROGRAM Header. Name: " + String(descriptor.name));
            }
            // Podrías añadir más 'else if' para otros tipos como BYTES, etc.
        }        
    }

public:
    void setPZX(tPZX pzx)
    {
        _myPZX = pzx;
    }
    // Función principal para analizar un bloque PZX

    void set_file(File mFile, int sizePZX)
    {
      // Pasamos los parametros a la clase
      _mFile = mFile;
      _sizePZX = sizePZX;
    }  
    
    void analyzeCSW(File &mFile, tPZXBlockDescriptor &descriptor)
    {
        strncpy(descriptor.typeName, "PZX CSW Block", 35);
        descriptor.playeable = true;
        logln("Analyzing PZX CSW Block...");

        int block_data_offset = descriptor.offset + 8;
        uint32_t flags = getNBYTE(mFile, block_data_offset, 4);
        descriptor.initial_level = (flags >> 3) & 1;
        descriptor.pause_duration = getNBYTE(mFile, block_data_offset + 4, 2);

        int compressed_data_offset = block_data_offset + 6;
        int compressed_data_size = descriptor.size - 6;

        if (compressed_data_size <= 0) {
            logln("ERROR: CSW block has no data.");
            descriptor.playeable = false;
            return;
        }

        // --- Descompresión Z-RLE ---
        uint8_t* compressed_buffer = (uint8_t*)ps_malloc(compressed_data_size);
        if (!compressed_buffer) { logln("ERROR: Failed to alloc Z-RLE buffer"); return; }
        
        mFile.seek(compressed_data_offset);
        mFile.read(compressed_buffer, compressed_data_size);

        mz_ulong decompressed_size = compressed_data_size * 15; // Estimación segura
        uint8_t* rle_data = (uint8_t*)ps_malloc(decompressed_size);
        int rle_data_size = 0;

        if (rle_data) {
            int result = mz_uncompress(rle_data, &decompressed_size, compressed_buffer, compressed_data_size);
            if (result == MZ_OK) {
                rle_data_size = decompressed_size;
                logln("  - Decompressed to " + String(rle_data_size) + " bytes of RLE data.");
            } else {
                logln("ERROR: miniz decompression failed with code: " + String(result));
                free(rle_data);
                rle_data = nullptr;
            }
        }
        free(compressed_buffer);

        if (!rle_data || rle_data_size == 0) {
            logln("ERROR: No RLE data to process after decompression.");
            if(rle_data) free(rle_data);
            descriptor.playeable = false;
            return;
        }

        // --- Parseo de datos RLE a pulsos (similar a TZX ID 0x18) ---
        int pulse_count = 0;
        for (int i = 0; i < rle_data_size; ) {
            pulse_count++;
            if (rle_data[i] == 0x00) i += 2; else i += 1;
        }

        if (pulse_count > 0) {
            descriptor.csw_num_pulses = pulse_count;
            descriptor.csw_pulse_data = (tRlePulse*)ps_calloc(pulse_count, sizeof(tRlePulse));
            if (descriptor.csw_pulse_data) {
                int current_pulse = 0;
                uint8_t last_pulse_len = 0;
                for (int i = 0; i < rle_data_size && current_pulse < pulse_count; ) {
                    uint8_t value = rle_data[i];
                    if (value == 0x00) {
                        descriptor.csw_pulse_data[current_pulse].pulse_len = last_pulse_len;
                        descriptor.csw_pulse_data[current_pulse].repeat = rle_data[i + 1];
                        i += 2;
                    } else {
                        last_pulse_len = value;
                        descriptor.csw_pulse_data[current_pulse].pulse_len = last_pulse_len;
                        descriptor.csw_pulse_data[current_pulse].repeat = 1;
                        i += 1;
                    }
                    current_pulse++;
                }
            }
        }
        free(rle_data);
    }    

    void analyzePZXBlock(File &mFile, int currentOffset, tPZXBlockDescriptor &descriptor) {
        
        // Inicializar typeName para evitar punteros nulos.
        descriptor.typeName[0] = '\0';
        // Leer el tag y el tamaño del bloque
        char tag[5] = {0};
        uint32_t size = 0;

        mFile.seek(currentOffset);
        mFile.read((uint8_t*)tag, 4);
        size = getNBYTE(mFile, currentOffset + 4, 4);

        strncpy(descriptor.tag, tag, 5);
        descriptor.offset = currentOffset;
        descriptor.size = size;

        if (strcmp(tag, "PZXT") == 0) analyzePZXT(mFile, descriptor);
        else if (strcmp(tag, "PULS") == 0) analyzePULS(mFile, descriptor);
        else if (strcmp(tag, "DATA") == 0) analyzeDATA(mFile, descriptor);
        else if (strcmp(tag, "PAUS") == 0) analyzePAUS(mFile, descriptor);
        else if (strcmp(tag, "CSW") == 0) analyzeCSW(mFile, descriptor); // ✅ AÑADIDO: Llamada a analyzeCSW        
        else if (strcmp(tag, "BRWS") == 0) { strncpy(descriptor.typeName, "Browse Point", 35); descriptor.playeable = false; }
        else if (strcmp(tag, "STOP") == 0) { analyzeSTOP(mFile, descriptor); }
        else { strncpy(descriptor.typeName, "Unknown PZX Block", 35); descriptor.playeable = false; }
    }
 
    bool isFilePZX(File &mFile) 
    {
        char tag[5] = {0};
        mFile.seek(0);
        mFile.read((uint8_t*)tag, 4);
        return strcmp(tag, "PZXT") == 0;
    }
   
// ... (código existente) ...
    void getBlockDescriptor(File &mFile)
    {
        int startOffset = 0; // Por defecto, empezamos en el offset 0

        // 1. Comprobar que el fichero empieza con la cabecera obligatoria 'PZXT'
        char file_tag[5] = {0};
        mFile.seek(0);
        mFile.read((uint8_t*)file_tag, 4);
        
        if (strcmp(file_tag, "PZXT") != 0) 
        {
            // Según la especificación, si no empieza con PZXT, no es un fichero válido.
            logln("Error: Not a valid PZX file. Missing 'PZXT' header at offset 0.");
            _myPZX.numBlocks = 0;
            TOTAL_BLOCKS = 0;
            return;
        }
        
        // ✅ CORRECCIÓN: Leer el tamaño real del bloque PZXT para saber dónde empieza el siguiente.
        // El tamaño total del bloque PZXT es 8 (tag+size) + el valor del campo size.
        uint32_t pzxt_block_data_size = getNBYTE(mFile, 4, 4);
        startOffset = 8 + pzxt_block_data_size;
        
        logln("PZXT header found. Next block starts at offset: " + String(startOffset));

        int currentOffset = startOffset;
        int blockCount = 0;
        
        // 2. Primer paso: contar bloques de datos (empezando DESPUÉS del bloque PZXT)
        while(currentOffset < mFile.size()) {
            blockCount++;

            char* tag = readTag(mFile, currentOffset);
            uint32_t blockSize = getNBYTE(mFile, currentOffset + 4, 4);
        
            // logln("");
            // logln("");
            // log("PZX Block - " + String(tag) + " at offset: [");
            // logHEX(currentOffset);
            // log("] - Size: " + String(blockSize) + " bytes");

            currentOffset += 8 + blockSize;
        }
        
        TOTAL_BLOCKS = blockCount;
        logln("Total PZX Data Blocks Found: " + String(blockCount));

        if (blockCount == 0) {
            _myPZX.numBlocks = 0;
            return;
        }

        // 3. Reservar memoria para el número correcto de bloques
        _myPZX.numBlocks = blockCount;
        _myPZX.descriptor = (tPZXBlockDescriptor*)ps_calloc(blockCount, sizeof(tPZXBlockDescriptor));
        if (_myPZX.descriptor == nullptr) {
            logln("FATAL: Failed to allocate memory for PZX descriptors. ESP will likely restart.");
            return;
        }

        // 4. Segundo paso: analizar cada bloque y rellenar el descriptor
        currentOffset = startOffset; // Reiniciar el offset al primer bloque de datos
        for (int i = 0; i < blockCount; ++i) {
            //if (STOP || ABORT) break;
            
            analyzePZXBlock(mFile, currentOffset, _myPZX.descriptor[i]);
            
            logln("TAg analyzed: " + String(_myPZX.descriptor[i].tag));
            logln("Offset: "); 
            logHEX(_myPZX.descriptor[i].offset);
            logln("Size: " + String(_myPZX.descriptor[i].size));
            logln("Analyzed PZX Block " + String(i + 1) + "/" + String(blockCount) + ": " + String(_myPZX.descriptor[i].typeName));
            
            // Avanzar al siguiente bloque
            currentOffset += 8 + _myPZX.descriptor[i].size;
        }
    }
// ... (resto del código) ...
    

    // ✅ FUNCIÓN AÑADIDA: El bucle principal que recorre y analiza todos los bloques
    // void getBlockDescriptor(File &mFile)
    // {
    //     // 1. Validar la cabecera PZX y obtener el offset inicial
    //     char file_tag[5] = {0};
    //     mFile.seek(0);
    //     mFile.read((uint8_t*)file_tag, 4);
    //     if (strcmp(file_tag, "PZXT") != 0) {
    //         logln("Error: Not a valid PZX file. Missing 'PZXT' header.");
    //         return;
    //     }

    //     // La cabecera 'PZXT' tiene un tamaño de datos y un header.
    //     // El primer bloque real empieza después de ella.
    //     // uint32_t header_block_size = getLittleEndianDWord(mFile, 4);
    //     int startOffset = 10; // Offset para el primer bloque de datos
        
    //     int currentOffset = startOffset;
    //     int blockCount = 0;
        
    //     // 2. Primer paso: contar bloques de datos (empezando DESPUÉS de la cabecera)
    //     while(currentOffset < _myPZX.size) {
    //         blockCount++;

    //         char* tag = readTag(mFile, currentOffset);

    //         uint32_t blockSize = getNBYTE(mFile, currentOffset + 4, 4);
        
    //         logln("");
    //         logln("");
    //         log("PZX Block - " + String(tag) + " at offset: [");
    //         logHEX(currentOffset);
    //         log("] - Size: " + String(blockSize) + " bytes");

    //         currentOffset += 8 + blockSize;

    //     }
        
    //     TOTAL_BLOCKS = blockCount+1;
    //     logln("Total PZX Data Blocks Found: " + String(blockCount+1));

    //     if (blockCount == 0) {
    //         _myPZX.numBlocks = 0;
    //         return;
    //     }

    //     // 3. Reservar memoria para el número correcto de bloques
    //     _myPZX.numBlocks = blockCount;
    //     _myPZX.descriptor = (tPZXBlockDescriptor*)ps_calloc(blockCount, sizeof(tPZXBlockDescriptor));
    //     if (_myPZX.descriptor == nullptr) {
    //         logln("FATAL: Failed to allocate memory for PZX descriptors. ESP will likely restart.");
    //         return;
    //     }

    //     // 4. Segundo paso: analizar cada bloque y rellenar el descriptor
    //     mFile.seek(0);
    //     currentOffset = startOffset; // Reiniciar el offset al primer bloque de datos
    //     for (int i = 0; i < blockCount; ++i) {
    //         //if (STOP || ABORT) break;
            
    //         analyzePZXBlock(mFile, currentOffset, _myPZX.descriptor[i]);
            
    //         logln("TAg analyzed: " + String(_myPZX.descriptor[i].tag));
    //         logln("Offset: "); 
    //         logHEX(_myPZX.descriptor[i].offset);
    //         logln("Size: " + String(_myPZX.descriptor[i].size));
    //         logln("Analyzed PZX Block " + String(i + 1) + "/" + String(blockCount) + ": " + String(_myPZX.descriptor[i].typeName));
            
    //         // Avanzar al siguiente bloque
    //         currentOffset += 8 + _myPZX.descriptor[i].size;
    //     }
    // }    

    void proccessingDescriptor(File &pzxFile)
    {

        LAST_MESSAGE = "Analyzing file. Capturing blocks";
        
        // if (SD_MMC.exists(pathDSC))
        // {
        //   SD_MMC.remove(pathDSC);
        // }
        
        //_blDscTZX.createBlockDescriptorFileTZX(dscFile,pathDSC); 
        // creamos un objeto TZXproccesor
        set_file(pzxFile, _rlen);
        // Lo procesamos y creamos DSC
        getBlockDescriptor(pzxFile);
        // Cerramos para guardar los cambios
        //dscFile.close();
        
        if (_myPZX.descriptor != nullptr)
        {
             // Entregamos información por consola
             //strcpy(LAST_NAME,"              ");
             LAST_MESSAGE = "PZX ready.";
        }
        else
        {
           LAST_MESSAGE = "Error in PZX not supported";
        }      
    }

    void process(char* path) 
    {
      
      File pzxFile;
      File dscFile;

      
      PROGRAM_NAME_DETECTED = false;
      PROGRAM_NAME = "";
      PROGRAM_NAME_2 = "";
    
      MULTIGROUP_COUNT = 1;

      // Abrimos el fichero
      pzxFile = SD_MMC.open(path, FILE_READ);
      //char* pathDSC = path;

      // Pasamos el fichero abierto a la clase. Elemento global
      _mFile = pzxFile;
      // Obtenemos su tamaño total
      _rlen = pzxFile.available();
      _myPZX.size = pzxFile.size();

      if (_rlen != 0)
      {
          FILE_IS_OPEN = true;

          proccessingDescriptor(pzxFile);
          logln("All blocks captured from PZX file");
      }
      else
      {
          FILE_IS_OPEN = false;
          LAST_MESSAGE = "Error in PZX file has 0 bytes";
      }              
    }

    void tapeAnimationON()
    {
        // Activamos animacion cinta
        _hmi.writeString("tape2.tmAnimation.en=1");
        _hmi.writeString("tape.tmAnimation.en=1");
        delay(250);
        _hmi.writeString("tape2.tmAnimation.en=1");
        _hmi.writeString("tape.tmAnimation.en=1");
    }

    void tapeAnimationOFF()
    {
        // Desactivamos animacion cinta
        _hmi.writeString("tape2.tmAnimation.en=0");
        _hmi.writeString("tape.tmAnimation.en=0");
        delay(250);
        _hmi.writeString("tape2.tmAnimation.en=0");
        _hmi.writeString("tape.tmAnimation.en=0");
    }

    void rewindAnimation(int direction)
    {
    int p = 0;
    int frames = 19;
    int fdelay = 5;

    while (p < frames)
    {

        POS_ROTATE_CASSETTE += direction;

        if (POS_ROTATE_CASSETTE > 23)
        {
        POS_ROTATE_CASSETTE = 4;
        }

        if (POS_ROTATE_CASSETTE < 4)
        {
        POS_ROTATE_CASSETTE = 23;
        }

        hmi.writeString("tape.animation.pic=" + String(POS_ROTATE_CASSETTE));
        delay(20);

        p++;
    }
    }

    void play()
    {
        int firstBlockToBePlayed = BLOCK_SELECTED-1;
        if (firstBlockToBePlayed < 0) firstBlockToBePlayed = 0;

        BYTES_TOBE_LOAD = _myPZX.size;
        BYTES_LOADED = 0;
        PROGRESS_BAR_BLOCK_VALUE = 0;
        STOP_OR_PAUSE_REQUEST = false;
        
        // Reiniciamos variables de control
        STOP = false;
        PAUSE = false;
        
        logln("Starting PZX playback...");
        logln("Total PZX Blocks: " + String(_myPZX.numBlocks));

        // Ahora lo voy actualizando a medida que van avanzando los bloques.
        PROGRAM_NAME_2 = "...";
        
        bool pause_from_stop_block = false;

        for (int i = firstBlockToBePlayed; i < _myPZX.numBlocks; ++i)
        {
            if (STOP || EJECT) break;

            if (PAUSE || pause_from_stop_block) 
            {
                tapeAnimationOFF();
                LAST_MESSAGE = "Tape paused. Press Play to continue.";
                while (!PLAY && !STOP && !EJECT)
                {
                    delay(100);

                    if (FFWIND)
                    {
                        FFWIND = false;
                        LAST_MESSAGE = "Fast Forwarding to next block";
                        BLOCK_SELECTED++;
                        if (i >= _myPZX.numBlocks)
                        {
                            BLOCK_SELECTED = _myPZX.numBlocks;
                            break;
                        }   
                        rewindAnimation(1);
                        // Forzamos un refresco de los indicadores
                        hmi.setBasicFileInformation(0, 0, myPZX.descriptor[BLOCK_SELECTED].name,
                                                    myPZX.descriptor[BLOCK_SELECTED].typeName,
                                                    myPZX.descriptor[BLOCK_SELECTED].size,
                                                    myPZX.descriptor[BLOCK_SELECTED].playeable);                       
                    }
                    else if (RWIND)
                    {
                        RWIND = false;
                        LAST_MESSAGE = "Rewinding to previous block";
                        BLOCK_SELECTED--;
                        if (BLOCK_SELECTED < 1)
                        {
                            BLOCK_SELECTED = 1;
                            break;
                        }   
                        rewindAnimation(-1);
                        // Forzamos un refresco de los indicadores
                        hmi.setBasicFileInformation(0, 0, myPZX.descriptor[BLOCK_SELECTED].name,
                                                    myPZX.descriptor[BLOCK_SELECTED].typeName,
                                                    myPZX.descriptor[BLOCK_SELECTED].size,
                                                    myPZX.descriptor[BLOCK_SELECTED].playeable);                          
                    }

                    if (BB_OPEN || BB_UPDATE)
                    {
                    // Ojo! no meter delay! que esta en una reproduccion
                    logln("Solicito abrir el BLOCKBROWSER");

                    hmi.openBlocksBrowser(myTZX,myTAP,myPZX);

                    BB_UPDATE = false;
                    BB_OPEN = false;
                    }                    
                }
                                
                if (PLAY)
                {
                    PAUSE = false;
                    tapeAnimationON();
                    LAST_MESSAGE = "Resuming playback";
                }
                else if (STOP)
                {
                    LAST_MESSAGE = "Stopped by user";
                    //LOADING_STATE = 2;
                    break;
                }
                else if (EJECT)
                {
                    //PZX_EJECT_RQT = true;
                    break;
                }

                // Si se paro por un bloque STOP, continuamos al siguiente
                // en otro caso permanecemos en el mismo bloque.
                if (BLOCK_SELECTED > 0 && !pause_from_stop_block)
                {
                    i = BLOCK_SELECTED - 1;
                }

                pause_from_stop_block = false;
            }

            BLOCK_SELECTED = i+1;

            if (!_myPZX.descriptor[i].playeable) continue;

            if (BLOCK_SELECTED == 0) 
            {
                PRG_BAR_OFFSET_INI = 0;
            }
            else
            {
                PRG_BAR_OFFSET_INI = _myPZX.descriptor[BLOCK_SELECTED].offset;
            }            

            CURRENT_BLOCK_IN_PROGRESS = i + 1;
            _hmi.setBasicFileInformation(0,0,_myPZX.descriptor[i].name,_myPZX.descriptor[i].typeName,_myPZX.descriptor[i].size,_myPZX.descriptor[i].playeable);

            if (strcmp(_myPZX.descriptor[i].tag, "PULS") == 0)
            {
                //logln(" - Playing PZX PULS Block");

                for (int p = 0; p < _myPZX.descriptor[i].timming.pzx_num_pulses; ++p)
                {
                    if (STOP || EJECT || PAUSE) break;

                    // ✅ CORRECCIÓN: Añadir un bucle para manejar las repeticiones de cada pulso
                    
                    for (int r = 0; r < _myPZX.descriptor[i].timming.pzx_pulse_data[p].repeat; ++r)
                    {
                        if (STOP || EJECT || PAUSE) break;
                        // Convertir T-States a milisegundos
                        _zxp.pulse(_myPZX.descriptor[i].timming.pzx_pulse_data[p].pulse_len, true);
                    }
                }
            }
            else if (strcmp(_myPZX.descriptor[i].tag, "DATA") == 0)
            {
                //logln(" - Playing PZX DATA Block");
                int data_byte_len = (_myPZX.descriptor[i].data_bit_count + 7) / 8;
                uint8_t* data_buffer = (uint8_t*)ps_malloc(data_byte_len);

                if (data_buffer)
                {
                    _mFile.seek(_myPZX.descriptor[i].data_stream_offset);
                    _mFile.read(data_buffer, data_byte_len);

                    for (int bit_num = 0; bit_num < _myPZX.descriptor[i].data_bit_count; ++bit_num)
                    {
                        if (STOP || EJECT || PAUSE) break;
                        int byte_idx = bit_num / 8;
                        int bit_idx = 7 - (bit_num % 8);
                        uint8_t bit_value = (data_buffer[byte_idx] >> bit_idx) & 1;

                        if (bit_value == 0) // Tocar pulsos para '0'
                        {
                            // ✅ CORRECCIÓN: Iterar por el array de pulsos y llamar a pulse para cada uno
                            for (int p = 0; p < _myPZX.descriptor[i].data_p0_count; ++p)
                            {
                                _zxp.pulse(_myPZX.descriptor[i].data_s0_pulses[p], true);
                            }
                        }
                        else // Tocar pulsos para '1'
                        {
                            // ✅ CORRECCIÓN: Iterar por el array de pulsos y llamar a pulse para cada uno
                            for (int p = 0; p < _myPZX.descriptor[i].data_p1_count; ++p)
                            {
                                _zxp.pulse(_myPZX.descriptor[i].data_s1_pulses[p], true);
                            }
                        }

                        //BYTES_LOADED = BYTES_LOADED + (bit_num / 8);
                        PROGRESS_BAR_BLOCK_VALUE = (int)(((float)(bit_num + 1) / _myPZX.descriptor[i].data_bit_count) * 100.0);
                    }
                    free(data_buffer);

                    if (_myPZX.descriptor[i].data_tail_pulse > 0)
                    {
                        // ✅ CORRECCIÓN: Usar semiPulse para el pulso de cola
                        _zxp.pulse(_myPZX.descriptor[i].data_tail_pulse, true);
                    }
                }
            }
            else if (strcmp(_myPZX.descriptor[i].tag, "PAUS") == 0)
            {
                //logln(" - Playing PZX PAUS Block");
                // El silencio está en TStates
                double duration = (_myPZX.descriptor[i].pause_duration / DfreqCPU) * 1000.0;
                _zxp.silence(duration);
            }
            else if (strcmp(_myPZX.descriptor[i].tag, "CSW") == 0)
            {

                logln("Playing PZX CSW Block");
                if (_myPZX.csw_sampling_rate > 0 && _myPZX.descriptor[i].csw_num_pulses > 0)
                {
                    // Factor para convertir la duración del pulso CSW a T-States de 3.5MHz
                    float conversion_factor = (float)DfreqCPU / (float)_myPZX.csw_sampling_rate;
                    
                    // Usar _zxp.pulse() en un bucle
                    
                    // Opcional: Si el nivel inicial es alto, generamos un pulso de duración 0 para establecerlo.
                    // La implementación de _zxp.pulse() podría manejar esto de otra forma.
                    // Si el audio empieza invertido, esta línea puede ser necesaria.
                    if (_myPZX.descriptor[i].initial_level == 1) {
                        _zxp.pulse(0, true); 
                    }

                    // 1. Iterar a través de los pulsos decodificados del RLE
                    for (int p = 0; p < _myPZX.descriptor[i].csw_num_pulses; ++p)
                    {
                        if (STOP) break;
                        
                        tRlePulse &pulse = _myPZX.descriptor[i].csw_pulse_data[p];
                        
                        // 2. Convertir la duración del pulso a T-States
                        uint16_t pulse_len_tstates = round((float)pulse.pulse_len * conversion_factor);

                        // 3. Repetir la llamada a _zxp.pulse() para cada repetición
                        for (int r = 0; r < pulse.repeat; ++r)
                        {
                            if (STOP) break;
                            _zxp.pulse(pulse_len_tstates, true);
                        }
                    }
                    // ✅ FIN DE LA CORRECCIÓN
                }
                _zxp.silence(_myPZX.descriptor[i].pause_duration);              

                // logln("Playing PZX CSW Block");
                // if (_myPZX.csw_sampling_rate > 0)
                // {
                //     // Factor para convertir la duración del pulso CSW a T-States de 3.5MHz
                //     float conversion_factor = (float)DfreqCPU / (float)_myPZX.csw_sampling_rate;
                    
                //     for (int p = 0; p < _myPZX.descriptor[i].csw_num_pulses; ++p)
                //     {
                //         if (STOP || EJECT || PAUSE) break;
                        
                //         tRlePulse &pulse = _myPZX.descriptor[i].csw_pulse_data[p];
                        
                //         // Convertimos la duración del pulso a T-States
                //         uint32_t pulse_len_tstates = round((float)pulse.pulse_len * conversion_factor);
                        
                //         if (pulse_len_tstates > 0) {
                //             // playCustomSymbol espera T-States y se encarga de la conversión final a audio
                //             _zxp.playCustomSymbol(pulse_len_tstates, pulse.repeat, true);
                //         }
                //     }
                // }
                // _zxp.silence(_myPZX.descriptor[i].pause_duration);
            }
            else if (strcmp(_myPZX.descriptor[i].tag, "STOP") == 0)
            {
                logln(" - Executing PZX STOP Block. Pausing tape.");
                pause_from_stop_block = true;
                PLAY = false;
                //PAUSE = true;
            }
            
            // Actualizamos el progreso total
            PROGRESS_BAR_TOTAL_VALUE = (int)(((float)(i + 1) / _myPZX.numBlocks) * 100.0);

        }

        // En el caso de no haber parado manualmente, es por finalizar
        // la reproducción
        if (LOADING_STATE == 1) 
        {
            logln("PZX playback finished.");

            if(LAST_SILENCE_DURATION==0)
            {_zxp.silence(2000);}

            // Paramos
            #ifdef DEBUGMODE
                logln("AUTO STOP launch.");
            #endif

            PLAY = false;
            PAUSE = false;
            STOP = true;
            REC = false;
            ABORT = true;
            EJECT = false;

            BLOCK_SELECTED = 1;
            CURRENT_BLOCK_IN_PROGRESS = 1;
            BYTES_LOADED = 0; 

            AUTO_STOP = true;

            _hmi.setBasicFileInformation(0,0,_myPZX.descriptor[BLOCK_SELECTED].name,_myPZX.descriptor[BLOCK_SELECTED].typeName,_myPZX.descriptor[BLOCK_SELECTED].size,_myPZX.descriptor[BLOCK_SELECTED].playeable);
        }
        // else
        // {
        //   PLAY = false;
        //   PAUSE = false;
        //   STOP = true;
        //   REC = false;
        //   ABORT = true;
        //   EJECT = false;

        //   BLOCK_SELECTED = 0;
        //   BYTES_LOADED = 0;            
        // }

        // Cerrando
    }    

    tPZX getDescriptor()
    {
        return _myPZX;
    }

    void initialize()
    {
        // Inicialización si es necesaria

        strncpy(_myPZX.name,"          ",10);
        _myPZX.numBlocks = 0;
        _myPZX.size = 0;

        CURRENT_BLOCK_IN_PROGRESS = 1;
        BLOCK_SELECTED = 1;
        PROGRESS_BAR_BLOCK_VALUE = 0;
        PROGRESS_BAR_TOTAL_VALUE = 0;
        PROGRAM_NAME_DETECTED = false;
    }        

};
