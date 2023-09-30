/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: input.ino
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Signal analyzer for REC function on powaDCR

    Version: 0.1

    Historico de versiones
    ----------------------
    v.0.1 - Version inicial
    
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
 

class TAPrecorder
{
    public:

      bool recordingFinish = false;
      bool errorInDataRecording = true;
      int errorsCountInRec = 0;
      bool wasRenamed = false;
      bool nameFileRead = false;
      bool notAudioInNotified = false;
      bool wasSelectedThreshold = false;
      bool wasFileNotCreated = true;
      
      
    private:

      AudioKit _kit;

      HMI _hmi;
      SdFat32 _sdf32;
      tTZX _myTZX;
      File32 _mFile;

      const int BUFFER_SIZE = 2048;
      uint8_t* buffer;

      // Comunes
      char* fileName;
      char* fileNameRename;      

      // Estamos suponiendo que el ZX Spectrum genera signal para grabar
      // con timming estandar entonces
      float tState = 1/3500000;
      int pilotPulse = 2168; // t-states
      int sync1 = 667;
      int sync2 = 735;
      int bit0 = 855;
      int bit1 = 1710;

      const int high = 32767;
      const int low = -32768;
      const int silence = 0;

      // Controlamos el estado del TAP
      int state = 0;
      // Contamos los pulsos del tono piloto
      int pilotPulseCount = 0;
      // Contamos los pulsos del sync1
      long silenceCount = 0;
      long lastSilenceCount = 0;
      bool isSilence = false;
      bool blockEnd = false;
      long silenceSample = 0;

      char* bitChStr;
      byte* datablock;
      //byte* fileData;

      int bitCount = 0;
      int byteCount = 0;
      int blockCount = 0;
      String bitString = "";

      byte checksum = 0;
      byte lastChk = 0;
      byte byteRead = 0;

      // Estado flanco de la senal, HIGH or LOW
      bool pulseHigh = false;
      bool waitingHead = true;

      // Contador de bloques
      int bl = 0;

      // Umbral para rechazo de ruido
      int threshold_high = 20000; 
      int threshold_low = -20000; 

      // Para los clásicos
      //int threshold_high = 6000; 
      //int threshold_low = -6000;       
      //

      int averageThreshold = 0;

      bool errorInSemiPulseDetection = false;
      //
      int pulseState = 0;
      int samplesCount_H = 0;
      int samplesCount_L = 0;

      struct tPulse
      {
        int high_edge;
        int low_edge;
      };

      tPulse pulse;

      struct tHeader
      {
          int type;
          char name[11];
          byte sizeLSB;
          byte sizeMSB;
          int blockSize = 19;
      };

      tHeader header;



      // Calculamos las muestras que tiene cada pulso
      // segun el tiempo de muestreo
      // long getSamples(int signalTStates, float tState, int samplingRate)
      // {
      //     float tSampling = 1/samplingRate;
      //     float tPeriod = signalTStates * tState;
      //     long samples = tPeriod / tSampling;

      //     return samples; 
      // }

      byte calculateChecksum(byte* bBlock, int startByte, int numBytes)
            {
                // Calculamos el checksum de un bloque de bytes
                byte newChk = 0;

                // Calculamos el checksum (no se contabiliza el ultimo numero que es precisamente el checksum)
                
                for (int n=startByte;n<(startByte+numBytes);n++)
                {
                    newChk = newChk ^ bBlock[n];
                }

                return newChk;
            }

      int16_t prepareSignal(int16_t value, int thresholdH, int thresholdL)
      {
          int16_t returnValue = 0;

          if (value >= thresholdH)
          {
            returnValue = high;
          }
          else if (value <= thresholdL)
          {
              returnValue = low;
          }
          else
          {
            returnValue = 0;    
          }

        return returnValue;
      }

      bool detectedSilencePulse(int16_t value, int threshold)
      {
          if (value == 0 && silenceCount <= threshold)
          {
            // Si el pulso aun no ha completado un ancho y no ha habido errores
            // vamos bien.
            silenceCount++;
            return false;
          }
          else if (value == silence && silenceCount > threshold)
          {
              // Silencio detectado
              silenceCount = 0;
              return true;
          }
          else if (value != silence && silenceCount <= threshold)
          {
              // Silencio corrupto o menor del threshold
              silenceCount = 0;
              return false;
          }
          else if (value != silence && silenceCount > threshold)
          {
              // Silencio detectado
              silenceCount = 0;
              return true;
          }
      }

      bool measurePulse(int16_t value)
      {
          // Comenzamos por detectar un cambio de XX --> HIGH
          if (pulseState == 0 && value == high)
          {
              // Pasamos a modo medidas en HIGH
              // contamos ya esta muestra
              isSilence = false;
              silenceSample = 0;
              samplesCount_H=1;
              // Cambiamos de estado
              pulseState = 1;
              return false;
          }
          else if (pulseState == 0 && value == low)
          {
              // Error. Tengo que esperar al flanco positivo        
              pulseState = 0;
              silenceCount = 0;
              //SerialHW.println("Error. Waiting HIGH EDGE");
              return false;
          }
          else if (pulseState == 0 && value == 0)
          {
              // Error. Finalizo
              pulseState = 0;
              //silenceCount++;
              //SerialHW.println("Sequency cut. Error.");
              return false;
          }
          else if (pulseState == 1 && value == high)
          {
              // Estado de medida en HIGH
              silenceSample = 0;
              blockEnd = false;
              samplesCount_H++;
              return false;
          }
          else if (pulseState == 1 && value == low && samplesCount_H >= 7)
          {
              // Cambio de HIGH --> LOW
              // finaliza el pulso HIGH y comienza el LOW
              // cuento esta muestra
              samplesCount_L=1;
              // Paso al estado de medida en LOW
              pulseState = 2;
              return false;
          }
          // else if (pulseState == 1 && value == 0 && samplesCount_H < 7)
          // {
          //     // Error pico de señal sin sentido
          //     silenceSample = 0;
          //     blockEnd = false;
          //     samplesCount_H=0;
          //     samplesCount_L=0;
          //     pulseState = 0;
          //     return false;
          // }          
          else if (pulseState == 2 && value == low)
          {
              // Estado de medida en LOW
              samplesCount_L++;
              return false;
          }
          else if (pulseState == 2 && (value == high || value == 0) && samplesCount_L >= 7)
          {
              // Ha finalizado el pulso. Pasa de LOW --> HIGH
              pulse.high_edge = samplesCount_H;
              pulse.low_edge = samplesCount_L;

              // Correcciones
              // if (pulse.high_edge >= 9 && pulse.high_edge <= 11 && pulse.low_edge <= 5)
              // {   
              //   pulse.high_edge = 10;
              //   pulse.low_edge = 10;
              // }
              // else if (pulse.low_edge >= 9 && pulse.low_edge <= 11 && pulse.high_edge <= 5)
              // {        
              //     pulse.high_edge = 10;
              //     pulse.low_edge = 10;
              // }
              // else if (pulse.high_edge >= 10 && pulse.high_edge <= 11)
              // {
              //   if (pulse.low_edge < 10)
              //   {
              //       pulse.high_edge = 10;
              //       pulse.low_edge = 10;
              //   }
              //   else if (pulse.low_edge > 20)
              //   {
              //       pulse.high_edge = 10;
              //       pulse.low_edge = 10;
              //   }
              // }
              // else if (pulse.low_edge >= 10 && pulse.low_edge <= 11)
              // {
              //   if (pulse.high_edge < 10)
              //   {
              //       pulse.high_edge = 10;
              //       pulse.low_edge = 10;
              //   }
              //   else if (pulse.high_edge > 20)
              //   {
              //       pulse.high_edge = 10;
              //       pulse.low_edge = 10;
              //   }
              // }
              // else if (pulse.high_edge >= 13 && pulse.high_edge <= 22)
              // {
              //   if (pulse.low_edge < 20)
              //   {
              //       pulse.high_edge = 20;
              //       pulse.low_edge = 20;
              //   }
              //   else if (pulse.low_edge > 25)
              //   {
              //       pulse.high_edge = 20;
              //       pulse.low_edge = 20;
              //   }
              // }
              // else if (pulse.low_edge >= 13 && pulse.low_edge <= 22)
              // {
              //   if (pulse.high_edge < 20)
              //   {
              //       pulse.high_edge = 20;
              //       pulse.low_edge = 20;
              //   }
              //   else if (pulse.high_edge > 25)
              //   {
              //       pulse.high_edge = 20;
              //       pulse.low_edge = 20;
              //   }
              // }

              //SerialHW.println("PW: " + String(pulse.high_edge) + " - " + String(pulse.low_edge));

              if (value == 0)
              {
                blockEnd = true;
                // Reiniciamos HIGH
                samplesCount_H = 0;
                // Pongo a cero las muestras en LOW
                samplesCount_L = 0;
              }
              else
              {
                // Leemos esta muestra que es HIGH para no perderla
                samplesCount_H = 1;
                // Pongo a cero las muestras en LOW
                samplesCount_L = 0;
              }
              // Comenzamos otra vez
              pulseState = 1;        
              return true;
          }

          // Si llegamos aquí por un silencio es entonces fin de bloque
          if (value == 0 && blockEnd == true && silenceSample > 500 && isSilence != true)
          {
            isSilence = true;
            // SerialHW.println("");
            // SerialHW.println("** Silence active **");          
            // SerialHW.println("");
            
            //notAudioInNotified = false;
            
            return true;
          }
          else
          {
            silenceSample++;
            return false;
          }
      }

      bool isGuidePulse(tPulse p)
      {
          if (p.high_edge >= 26 && p.high_edge <= 28)
          {
              if (p.low_edge >= 26 && p.low_edge <= 28)
              {
                  //
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

      bool isSync(tPulse p)
      {
          if (p.high_edge >= 7 && p.high_edge <= 9)
          {
              if (p.low_edge >= 7 && p.low_edge <= 9)
              {
                  //
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

      bool isSync2B(tPulse p)
      {
          if (p.high_edge >= 26 && p.high_edge <= 27)
          {
              if (p.low_edge >= 7 && p.low_edge <= 9)
              {
                  //
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

      bool isBit0(tPulse p)
      {
          if (p.high_edge >= 10 && p.high_edge <= 11)
          {
              if (p.low_edge >= 10 && p.low_edge <= 11)
              {
                  //
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

      bool isBit1(tPulse p)
      {
          if (p.high_edge >= 20 && p.high_edge <= 22)
          {
              if (p.low_edge >= 20 && p.low_edge <= 22)
              {
                  //
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

      void proccesByteData()
      {
          // Almacenamos el tipo de cabecera
          if (byteCount == 1)
          {
              // 0 - program
              // 1 - Number array
              // 2 - Char array
              // 3 - Code file
              header.type = byteRead;
              
              SerialHW.println("");              
              SerialHW.println("HEADER TYPE");              
              SerialHW.println(String(header.type));
              SerialHW.println("");      

              proccesInfoBlockType();

          }

          if (byteCount > 1 && byteCount < 12)
          {
              // Almacenamos el nombre
              if (byteRead > 20)
              {
                header.name[byteCount-2] = (char)byteRead;
              }
              else
              {
                int valueTmp = 32;
                header.name[byteCount-2] = (char)valueTmp;
              }
          }

          if (byteCount == 12)
          {
              // Almacenamos el LSB del tamaño
              header.sizeLSB = byteRead;
          }

          if (byteCount == 13)
          {
              // Almacenamos el MSB del tamaño
              header.sizeMSB = byteRead;
          }

      }

      void showProgramName()
      {
        String prgName = "";
        for (int n=0;n<10;n++)
        {
          SerialHW.print(header.name[n]);
          prgName += header.name[n];
        }

        if (!nameFileRead)
        {
          PROGRAM_NAME = prgName;
        }
        else
        {
          PROGRAM_NAME_2 = prgName;
        }
        
      }

      void proccesInfoBlockType()
      {

          // Si el conjunto leido es de 19 bytes. Es una cabecera.
          if (header.blockSize == 19)
          {
            if (header.type == 0)
            {
              LAST_TYPE = "PROGRAM.HEAD";
            }
            else if (header.type == 1)
            {
              LAST_TYPE = "NUMBER_ARRAY.HEAD";
            }
            else if (header.type == 2)
            {
              LAST_TYPE = "CHAR_ARRAY.HEAD";
            }
            else if (header.type == 3)
            {
              LAST_TYPE = "BYTE.HEAD";
            }            
          }
          else if (header.blockSize > 19)
          {

            if (header.blockSize ==  6912)
            {
              LAST_TYPE = "SCREEN.DATA";
              SerialHW.println("SCREEN");
            }
            else
            {
              if (header.type != 0)
              {
                  // Es un bloque de datos BYTE
                  LAST_TYPE = "BYTE.DATA";
                  SerialHW.println("BYTE.DATA");
              }
              else if (header.type == 0)
              {
                  // Es un bloque BASIC
                  LAST_TYPE = "BASIC.DATA";
                  SerialHW.println("BASIC.DATA");
              }
            }
          }        
      }

      void renameFile()
      {
        char cPath[] = "/";
        char f[25];
        strcpy(f,fileNameRename);
        strcat(cPath,f);

        SerialHW.println("");
        SerialHW.println("File searching? " + String(cPath));
        SerialHW.println("");

        if (_sdf32.exists(fileNameRename))
        {
          _sdf32.remove(fileNameRename);
          SerialHW.println("");
          SerialHW.println("File removed --> " + String(cPath));
          SerialHW.println("");
        }

        if (_mFile.rename(fileNameRename))
        {
          SerialHW.println("");
          SerialHW.println("File renamed --> " + String(fileNameRename));
          SerialHW.println("");
          
          wasRenamed = true;
        }
      }

      void getFileName()
      {
        if (!nameFileRead)
        {
          if (fileNameRename != NULL)
          {
            free(fileNameRename);
          }

          // Proporcionamos espacio en memoria para el
          // nuevo filename
          fileNameRename = (char*)calloc(20,sizeof(char));

          int i=0;
          for (int n=0;n<10;n++)
          {
            if (header.name[n] != ' ')
            {
                fileNameRename[i] = header.name[n];
                i++;
            }
          }

          char* extFile = ".tap\0";
          strcat(fileNameRename,extFile);
          
          nameFileRead = true;                             
        }        
      }

      void proccessInfoBlock()
      {

        SerialHW.println("");
        SerialHW.print("Checksum: ");
        SerialHW.print(lastChk,HEX);
        SerialHW.println("");
        SerialHW.println("Block read: " + String(blockCount));

        checksum = 0;

        if (byteCount !=0)
        {
            SerialHW.println("");
            SerialHW.println("Block data was recorded successful");
            SerialHW.println("");
            SerialHW.println("Total bytes: " + String(byteCount));
            SerialHW.println("");

            // Inicializamos para el siguiente bloque
            header.blockSize = 19;

            // Si es una cabecera. Cogemos el tamaño del bloque data a
            // salvar.
            if (byteCount == 19)
            {                         
                // BLOQUE CABECERA

                // La información del tamaño del bloque siguiente
                // se ha tomado en tiempo real y está almacenada
                // en headar.sizeLSB y header.sizeMSB
                int size = 0;
                size = header.sizeLSB + header.sizeMSB*256;
                
                // Guardamos el valor ya convertido en int
                header.blockSize = size;
                
                // Hay que añadir este valor sumando 2, en la cabecera
                // antes del siguiente bloque
                int size2 = size + 2;
                uint8_t LSB = size2;
                uint8_t MSB = size2 >> 8;

                // Antes del siguiente bloque metemos el size
                _mFile.write(LSB);
                _mFile.write(MSB);

                SerialHW.print("PROGRAM: ");

                // Mostramos el nombre del programa en el display
                // ojo que el marcador "nameFileRead" debe estar
                // despues de esta función, para indicar que el
                // nombre del programa ya fue leido y mostrado
                // que todo lo que venga despues es PROGRAM_NAME_2
                showProgramName();

                // Obtenemos el nombre que se le pondrá al .TAP
                getFileName();

                SerialHW.println("");
                SerialHW.println("Block size: " + String(size) + " bytes");
                SerialHW.println("");
                SerialHW.println("");
            }
            else if (byteCount > 19)
            {
                // BLOQUE DATA

                // Guardamos ahora en fichero
                recordingFinish = true;
                errorInDataRecording = false;
                waitingHead = true;
                
            }

            //LAST_SIZE = header.blockSize;  
            //_hmi.updateInformationMainPage();
        }
        

      }

      void analyzeSamplesForThreshold(int len)
      {

        int16_t *value_ptr = (int16_t*) buffer;
        int16_t oneValue = *value_ptr++;
        int16_t oneValue2 = *value_ptr++;
        long average = 0;
        int averageSamples = 0;

        for (int j=0;j<len/4;j++)
        {  
            if (oneValue >= 0 && oneValue <= 20000)
            {
              // Lo contabilizo para la media
              average = average + oneValue;
              averageSamples++;
            }

            // Leemos el siguiente valor
            oneValue = *value_ptr++;
            // Este solo es para desecharlo
            oneValue2 = *value_ptr++;  
        }

        // Calculamos la media
        if (averageSamples==0)
        {averageSamples = 1;}

        averageThreshold += (average / averageSamples);

        SerialHW.println("Media: ");
        SerialHW.println(String(averageThreshold));

      }

      void readBuffer(int len)
      {

        int16_t *value_ptr = (int16_t*) buffer;
        int16_t oneValue = *value_ptr++;
        int16_t oneValue2 = *value_ptr++;
        int16_t finalValue = 0;

          for (int j=0;j<len/4;j++)
          {  
              finalValue = prepareSignal(oneValue,threshold_high,threshold_low);

              //SerialHW.println(String(oneValue));
              //SerialHW.println(String(finalValue));

              if (measurePulse(finalValue))
              {

                  //
                  // Control del estado de señal
                  //
                  if (state == 0  && pilotPulseCount <= 800)
                  {
                      if (isGuidePulse(pulse))
                      {
                          pilotPulseCount++;
                      }
                      else
                      {
                          pilotPulseCount = 0;
                      }
                  }
                  else if (state == 0 && pilotPulseCount > 800)
                  {
                      // Saltamos a la espera de SYNC1 y 2
                      state = 1;
                      pilotPulseCount = 0;
                      SerialHW.println("");                                  
                      SerialHW.println("REC. Wait for SYNC");
                      SerialHW.println("");

                      // Escribimos los primeros bytes
                      if (waitingHead)
                      {
                          uint8_t firstByte = 19;
                          uint8_t secondByte = 0;
                          _mFile.write(firstByte);
                          _mFile.write(secondByte);

                          waitingHead = false; 

                          SerialHW.println("");
                          SerialHW.println("Writting first bytes.");
                          SerialHW.println("");
                      }


                  }
                  else if (state == 1 && (isSync(pulse) || isSync2B(pulse)))
                  {
                      state = 2;
                      SerialHW.println("REC. Wait for DATA");
                      SerialHW.println("");
                  }
                  else if (state == 2 && isBit0(pulse))
                  {
                      state = 2;
                      bitString += "0";
                      bitChStr[bitCount] = '0';
                      bitCount++;
                  }
                  else if (state == 2 && isBit1(pulse))
                  {
                      state = 2;
                      bitString += "1";
                      bitChStr[bitCount] = '1';
                      bitCount++;
                  }

                  //
                  // Conversión de bits a bytes
                  //
                  if (bitCount > 7)
                  {
                    // Reiniciamos el contador de bits
                    bitCount=0;

                    // Convertimos a byte el valor leido del DATA
                    byte value = strtol(bitChStr, (char**) NULL, 2);
                    byteRead = value;
                    uint8_t valueToBeWritten = value;
                    
                    // Escribimos en fichero el dato
                    _mFile.write(valueToBeWritten);

                    // Guardamos el checksum generado
                    lastChk = checksum;
                    // Calculamos el nuevo checksum
                    checksum = checksum ^ value;

                    // Lo representamos
                    // SerialHW.print(bitString + " - ");
                    // SerialHW.print(value,HEX);
                    // SerialHW.print(" - str: ");
                    // SerialHW.print((char)value);
                    // SerialHW.print(" - chk: ");
                    // SerialHW.print(checksum,HEX);
                    // SerialHW.println("");                    
                    
                    proccesByteData();   

                    // Mostramos el progreso de grabación del bloque
                    _hmi.writeString("progression.val=" + String((int)((byteCount*100)/(header.blockSize-1))));                        

                    // Contabilizamos bytes
                    byteCount++;
                    // Reiniciamos la cadena de bits
                    bitString = "";

                  }
              }

              // Hay silencio, pero ¿Es silencio despues de bloque?
              if (isSilence == true && blockEnd == true)
              {
                // Si lo es. Reiniciamos los marcadores
                isSilence = false;
                blockEnd = false;


                if (checksum == 0 && byteCount !=0)
                {
                  // Si el checksum final es 0, y contiene
                  // El bloque es correcto.

                  // NOTA: El ultimo byte es el checksum
                  // si se hace XOR sobre este mismo da 0
                  // e indica que todo es correcto

                  SerialHW.println("");
                  SerialHW.println("Block red correctly.");

                  // Informamos de los bytes salvados
                  LAST_SIZE = header.blockSize; 
                  // Informamos del bloque en curso
                  BLOCK_SELECTED = blockCount;

                  // Procesamos información del bloque
                  proccessInfoBlock();
                  // Actualizamos el HMI.
                  _hmi.updateInformationMainPage(); 

                  // Incrementamos un bloque  
                  blockCount++;               
                  
                } 
                else if (byteCount != 0 && checksum !=0)
                {
                  SerialHW.println("");
                  SerialHW.println("Corrupt data detected. Error.");
                  SerialHW.println("");

                  LAST_MESSAGE = "Corrupt data detected. Error.";

                  // Guardamos ahora en fichero
                  recordingFinish = true;
                  errorInDataRecording = true;
                  blockCount = 0;
                  errorsCountInRec++;
                } 
                else
                {}

                // Comenzamos otra vez
                state = 0;
                byteCount = 0;                              
              }

              // Ahora medimos el pulso detectado

              // Leemos el siguiente valor
              oneValue = *value_ptr++;
              // Este solo es para desecharlo
              oneValue2 = *value_ptr++;        
          }
      }

    public:

      void set_SdFat32(SdFat32 sdf32)
      {
          _sdf32 = sdf32;
      }

      void set_HMI(HMI hmi)
      {
          _hmi = hmi;
      }

      void set_kit(AudioKit kit)
      {
        _kit = kit;
      }

      void selectThreshold()
      {
          if (!wasSelectedThreshold)
          {
              int totalSamplesForTh = 50;
              averageThreshold = 0;

              for (int i = 0;i<totalSamplesForTh;i++)
              {
                size_t len = _kit.read(buffer, BUFFER_SIZE);
                analyzeSamplesForThreshold(len);
              }

              // Entregamos la media del ruido
              averageThreshold = averageThreshold / totalSamplesForTh;

              //
              SerialHW.println("Average threshold: ");
              SerialHW.println(String(averageThreshold));

              if (averageThreshold <= 1000)
              {
                // N-Go
                SerialHW.println("");
                SerialHW.println("Threshold for N-Go");
                threshold_high = 20000;
                threshold_low = -20000;

                LAST_MESSAGE = "Recording:";                
                LAST_MESSAGE = LAST_MESSAGE + " [ Next / N-Go ] " + String(averageThreshold);
                _hmi.updateInformationMainPage();  
              }
              else
              {
                // Classic versions
                SerialHW.println("");
                SerialHW.println("Threshold for classic versions");
                threshold_high = 6000;
                threshold_low = -6000;

                LAST_MESSAGE = "Recording:";                
                LAST_MESSAGE = LAST_MESSAGE + " [ Classic ] " + String(averageThreshold);
                _hmi.updateInformationMainPage();  

              }

              wasSelectedThreshold = true;              
          }        
      }

      void initializeBuffer()
      {
        for (int i=0;i<BUFFER_SIZE;i++)
        {
          buffer[i]=0;
        }
      }

      bool recording()
      {
          
          size_t len = _kit.read(buffer, BUFFER_SIZE);
          readBuffer(len);   

          if (recordingFinish || wasFileNotCreated)
          {
            return true;
          }
          else
          {
            return false;
          }
      }

      void prepareHMI()
      {
        // Inicializamos variables del HMI
        BLOCK_SELECTED = 0;
        TOTAL_BLOCKS = 0;
        PROGRAM_NAME = "";
        PROGRAM_NAME_2 = "";
        TYPE_FILE_LOAD = "";
        LAST_SIZE = 0;
        LAST_NAME = "";
        LAST_TYPE = "";    
      }

      void initialize()
      {

        prepareHMI();

        // Reservamos memoria
        fileName = (char*)calloc(20,sizeof(char));
        fileName = "_record\0";

        // Se crea un nuevo fichero temporal
        if (!_mFile.open(fileName, O_WRITE | O_CREAT | O_TRUNC)) 
        {
          SerialHW.println("File for REC failed!");
          wasFileNotCreated = true;
        }
        else
        {
          SerialHW.println("File for REC prepared.");
          wasFileNotCreated = false;
        }

        // Inicializo bit string
        bitChStr = (char*)calloc(8, sizeof(char));
        datablock = (byte*)calloc(1, sizeof(byte));
        
        // Inicializamos el array de nombre del header
        for (int i=0;i<10;i++)
        {
          header.name[i] = ' ';
        }

        buffer = (uint8_t*)calloc(BUFFER_SIZE,sizeof(uint8_t));
        initializeBuffer();

        recordingFinish = false;
        // Suponemos que hay error
        errorInDataRecording = true;
        errorsCountInRec = 0;
        nameFileRead = false;
        wasRenamed = false;
        blockCount = 0;
        wasSelectedThreshold = false;
      }

      void terminate(bool removeFile)
      {
          
          if (buffer != NULL)
          {free(buffer);}

          SerialHW.println("");
          SerialHW.println("Ok free - buffer.");
          SerialHW.println("");          
          
          if (datablock != NULL)
          {free(datablock);}

          SerialHW.println("");
          SerialHW.println("Ok free - datablock.");
          SerialHW.println("");          


          if (bitChStr != NULL)
          {free(bitChStr);}

          SerialHW.println("");
          SerialHW.println("Ok free - bitChSrt.");
          SerialHW.println("");               

          if (!removeFile)
          {
            
            if (_mFile.isOpen())
            {
              renameFile();

              SerialHW.println("");
              SerialHW.println("File rename.");
              SerialHW.println("");              
            }          

            _mFile.close();

            SerialHW.println("");
            SerialHW.println("File closed and saved.");
            SerialHW.println("");     
          }
          else
          {
            _mFile.close();
            _mFile.remove();

          SerialHW.println("");
          SerialHW.println("File closed and removed.");
          SerialHW.println("");          

          }

          wasRenamed = false;
          nameFileRead = false;

          _hmi.writeString("progression.val=0");     
          _hmi.updateInformationMainPage();     
          
      }

      TAPrecorder()
      {}
};
