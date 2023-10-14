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

      hw_timer_t *timer = NULL;

      HMI _hmi;
      SdFat32 _sdf32;
      tTZX _myTZX;
      File32 _mFile;

      const int BUFFER_SIZE = 4;
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

      // Tiempos de señal
      // Tono guia
      const int timeForLead = 1714; //1714;   // Lapara buscar
      const int timeLeadSeparation = 585; 
      // Con samples
      const int minLead = 25;
      const int maxLead = 28;
      // Señal de sincronismo
      const int timeForSync = 400; //520
      const int timeSyncSeparation = 190;
      // Con samples
      const int minSync1 = 7;
      const int maxSync1 = 7;
      //
      const int minSync2 = 8;
      const int maxSync2 = 8;

      // Señal de bit0
      const int timeForBit0 = 489;
      const int timeBit0Separation = 240;
      // Con samples
      const int minBit0 = 10;
      const int maxBit0 = 11;
      // Señal de bit1
      const int timeForBit1 = 977;
      const int timeBit1Separation = 480; 
      // Con samples
      const int minBit1 = 20;
      const int maxBit1 = 28;

      // Umbrales de pulso.
      const int high = 32767;
      const int low = -32768;
      const int silence = 0;

      // Definimos el sampling rate real de la placa.
      const float samplingRate = 44099.988;
      // Calculamos el T sampling_rate en us
      const float TsamplingRate = (1 / samplingRate) * 1000 * 1000;

      // Controlamos el estado del TAP
      int state = 0;
      // Contamos los pulsos del tono piloto
      int pilotPulseCount = 0;
      // Contamos los pulsos del sync1
      long silenceCount = 0;
      long lastSilenceCount = 0;
      bool isSilence = false;
      //bool blockEnd = false;
      long silenceSample = 0;

      char* bitChStr;
      byte* datablock;
      //byte* fileData;
      int bytesXlinea = 0;
      int bitCount = 0;
      int byteCount = 0;
      int blockCount = 0;
      int pulseCount = 0;
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

      // LOAD routine
      long averageThreshold = 0;
      bool errorInSemiPulseDetection = false;
      int edge1Counter = 0;
      int edge2Counter = 0;
      int statusEdgeOnDetection = 0;
      int statusEdgeOffDetection = 0;
      int saveStatus = 0;
      // Timming
      unsigned long startTime = 0;
      float timeBuffer = 0;
      bool countTimeEnable = false;
      bool startTimeWasLoaded = false;
      bool measureTimeWasLoaded = false;
      bool timeOutAlarm = false;
      
     
      // Porcentaje de error del comparador de tiempo (%)
      const int timeToEdgeThreshold = 2;
      // Tiempo que se ha tardado en encontrar el flanco
      float timeToEdge1 = 0;
      float timeToEdge2 = 0;
      // Crossing por cero
      int16_t lastFinalValue = 0;

      // Estabilizador del buffer del DSP
      bool stabilizationPhaseFinish = false;
      int countStabilizationSteps = 0;

      // Zero crossing routine
      int samplesCrossing=0;
      int detectState = 0;
      int pulseWidth = 0;
      int pwSync1 = 0;
      int pwSync2 = 0;
      int pwBit0_1 = 0;
      int pwBit0_2 = 0;
      //
      // Detector de Schmitt
      int16_t lastSwitchStatus = 0;
      int detectStateSchmitt = 0;

      //
      float TON = 0;
      float TOFF = 0;
      float TSI = 0;
      float TControl = 0;
      unsigned long TCycle = 0;

      // Notifications
      bool noSignalWasNotified = false;
      bool guideDetectionWasNotified = false;

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
                header.blockSize = size+2;
                
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

        //SerialHW.println("Media: ");
        //SerialHW.println(String(averageThreshold));

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

      void setTimer(int value)
      {
          if (!startTimeWasLoaded)
          {
              timeBuffer = value;
              startTimeWasLoaded = true;
              //SerialHW.println("Loading timer: " + String(value) + " us");
              startTimer();
          }
          else
          {}
      }

      void resetTimer()
      {
        startTimeWasLoaded = false;
      }

      void countTime()
      {
        // Contamos el tiempo de una muestra
        if (countTimeEnable)
        {
          timeBuffer -= TsamplingRate;
          //timeBuffer -= 22.4;
          if (timeBuffer <= 0)
          {
            timeBuffer = 0;
          }
        }
      }

      bool timeOut()
      {
        if (timeBuffer==0)
        {
          stopTimer();
          return true;
        }
        else
        {
          return false;
        }
      }

      float readTimer()
      {
        float ti = timeBuffer;
        return ti;
      }

      void stopTimer()
      {
        countTimeEnable = false;
      }

      void startTimer()
      {
        countTimeEnable = true;
      }

      float timeThreshold(int value, float per)
      {
        // Calcula el time menos un % de error
        return (value - (value*per));
      }

      bool detectZeroCrossing(int16_t value)
      {
        bool zeroCrossing = false;

        switch (detectState)
        {
          case 0:
            // Estado inicial transición            
            if (value==high)
            {
              // A high
              detectState=2;
              zeroCrossing = false;
            }
            else if (value==low)
            {
              // A low
              detectState=1;
              zeroCrossing = false;
            }
            else
            {
              detectState=0;
              zeroCrossing = false;
            }
           break;

          case 1:
            // Regimen estacionario en LOW
            if (value==high)
            {
              detectState=2;
              zeroCrossing = true;              
            }
            else if (value==low)
            {
              detectState=1;
              samplesCrossing++;              
              zeroCrossing = false;              
            }
            else
            {
              detectState=1;
              zeroCrossing = false;              
            }
            break;

          case 2:
            // Regimen estacionario en HIGH
            if (value==high)
            {
              detectState=2;
              samplesCrossing++;              
              zeroCrossing = false;              
            }
            else if (value==low)
            {
              detectState=1;
              zeroCrossing = true;              
            }
            else
            {
              detectState=2;
              zeroCrossing = false;              
            }
            break;

        }
        
        // NOTA: lastValue se actualiza en readBuffer()
        if (zeroCrossing)
        {
          //SerialHW.println("Measure: w: " + String(samplesCrossing));
          pulseWidth = samplesCrossing;
          
          samplesCrossing=0;
        }

        // Devolvemos el estado de paso por cero  
        return zeroCrossing;    
      }

      int16_t schmittDetector(int16_t value, int16_t thH, int16_t thL)
      {

        int16_t switchStatus = lastSwitchStatus;

        switch (detectStateSchmitt)
        {
          case 0:
            // Estado inicial
            if (value > thH)
            {
              // HIGH
              detectStateSchmitt = 1;              
            }
            else if (value < thL)
            {
              // LOW
              detectStateSchmitt = 2;              
            }
            else
            {
              detectStateSchmitt = 0;
            }
            break;

          case 1:
            // Estado en HIGH
            if (value < thL)
            {
              switchStatus = low;
              detectStateSchmitt = 2;              
            }
            else if (value > thH)
            {
              detectStateSchmitt = 1;              
            }
            else
            {
              detectStateSchmitt = 1;
            }
            break;

          case 2:
            // Estado en LOW
            if (value < thL)
            {
              detectStateSchmitt = 2;              
            }
            else if (value > thH)
            {
              switchStatus = high;
              detectStateSchmitt = 1;              
            }
            else
            {
              detectStateSchmitt = 2;
            }            
            break;

        }
        
        lastSwitchStatus = switchStatus;
        return switchStatus;
      }

      // void readBuffer(int len, bool getSamples)
      // {

      //   int16_t *value_ptr = (int16_t*) buffer;
      //   int16_t oneValue = *value_ptr++;
      //   int16_t oneValue2 = *value_ptr++;
      //   int16_t finalValue = 0;

      //   bool testRecorder = false;

      //   for (int j=0;j<len/4;j++)
      //   {  
      //       finalValue = prepareSignal(oneValue,threshold_high,threshold_low);
            
      //       if (testRecorder)
      //       {
      //         SerialHW.println(String(finalValue));
      //       }

      //       if (getSamples && !testRecorder)
      //       {
      //         switch (state)
      //         {
                
      //           case 0:
      //             // -------------------------------------------------
      //             // LD-START
      //             // -------------------------------------------------
      //             // Buscamos un pulso al menos en 14000 T-States (4000 us)
      //             // El timer una vez cargado no se vuelve a recargar
      //             // hasta que no se hace un resetTimer()
      //             setTimer(4000);
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                   stopTimer();
      //                   resetTimer();                      
      //                   // LD-WAIT
      //                   //SerialHW.println("LD-WAIT - phase 1");
      //                   state = 10;
      //               }
      //               else
      //               {
      //                 // Seguimos              
      //                 state = 0;
      //               }
      //             }
      //             else
      //             {
      //               // Time out
      //               stopTimer();
      //               resetTimer();
      //               // LD-START
      //               state = 0;
      //             }
      //             break;

      //           case 10:
      //             // -------------------------------------------------
      //             // LD-WAIT
      //             // -------------------------------------------------
      //             // Buscamos dos pulso al menos en 1s de tiempo (1000000us)
      //             // El timer una vez cargado no se vuelve a recargar
      //             // hasta que no se hace un resetTimer()                break;
      //             setTimer(1000000);
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                   // Buscamos ahora el segundo
      //                   //SerialHW.println(" --> LD-WAIT - phase 2");
      //                   state = 11;
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out              
      //                 state = 10;
      //               }
      //             }
      //             else
      //             {
      //               // Time out
      //               stopTimer();
      //               resetTimer();
      //               // LD-START
      //               state = 0;
      //             }
      //           case 11:
      //             // LD-WAIT - segundo edge
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                   // Si dos pulsos fueraon encontrados en el intervalo 
      //                   // de tiempo, indicará que posiblemente estemos en el
      //                   // tren de pulsos del tono guía.
      //                   stopTimer();
      //                   resetTimer();
      //                   // LD-LEADER
      //                   //SerialHW.println(" --> LD-LEADER - phase 1");
      //                   state = 1;
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out
      //                 state = 11;
      //               }
      //             }
      //             else
      //             {
      //               // Time out
      //               stopTimer();
      //               resetTimer();
      //               // LD-START
      //               state = 0;
      //             }
      //             break;

      //           case 1:
      //             //
      //             // -------------------------------------------------
      //             // LD-LEADER
      //             // -------------------------------------------------
      //             //
      //             // El timer una vez cargado no se vuelve a recargar
      //             // hasta que no se hace un resetTimer()              
      //             setTimer(timeForLead);
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                 timeToEdge1 = readTimer();
      //                 //SerialHW.println(" --> LD-LEADER - phase 2");
      //                 state = 2;
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out
      //                 state = 1;
      //               }
      //             }
      //             else
      //             {
      //                 // Time out
      //                 stopTimer();
      //                 resetTimer();
      //                 pulseCount = 0;                    
      //                 // LD-START
      //                 state = 0;                  
      //             }
      //             break;
                
      //           case 2:
      //             // LD-LEADER segundo edge
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                 timeToEdge2 = readTimer();
      //                 float delta = timeToEdge1 - timeToEdge2;
      //                 // Tenemos que encontrarlos con una distancia mínima
      //                 // de 3000 T-States
      //                 //SerialHW.println(" --T1: " + String(timeToEdge1));                    
      //                 //SerialHW.println(" --T2: " + String(timeToEdge2)); 
      //                 //SerialHW.println(" --delta: " + String(delta)); 

      //                 if (delta >= timeLeadSeparation)
      //                 {
      //                   // Si el pulso está dentro
      //                   pulseCount++;
      //                   //SerialHW.println(" --pulseCount: " + String(pulseCount));

      //                   stopTimer();
      //                   resetTimer();

      //                   // Contamos 256 pulsos seguidos, esto nos indica
      //                   // que estamos en medio del tono guía
      //                   if (pulseCount >= 256)
      //                   {
      //                     // paso al estado de espera de SYNC1
      //                     pulseCount=0;
      //                     //
      //                     //SerialHW.println("Wait for SYNC 1");
      //                     // LD-SYNC
      //                     state = 3;
      //                   }
      //                   else
      //                   {
      //                     // LD-LEADER
      //                     // Seguimos buscando pulsos
      //                     state = 1;
      //                   }
      //                 }
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out
      //                 state = 2;
      //               }
      //             }
      //             else
      //             {
      //                 // Time out
      //                 stopTimer();
      //                 resetTimer();
      //                 pulseCount = 0;                    
      //                 state = 0;
      //             }
      //             break;

      //           case 3:

      //             // ------------------------------------------------
      //             // LD-SYNC
      //             // ------------------------------------------------
      //             // El timer una vez cargado no se vuelve a recargar
      //             // hasta que no se hace un resetTimer()              
      //             setTimer(timeForSync);
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {
      //                 // Si lo encontramos antes de hacer time-out es SYNC1
      //                 // Ahora hay que encontrar otro que esté pegado a este
      //                 timeToEdge1 = readTimer();
      //                 //SerialHW.println("Wait for SYNC 1 - Phase 1 - Detected - " + String(timeToEdge1));                      
      //                 state = 4;
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out
      //                 state = 3;
      //               }
      //             }
      //             else
      //             {
      //                 // Time out
      //                 stopTimer();
      //                 resetTimer();
      //                 state = 3;                  
      //             }                
      //             break;

      //           case 4:            
      //             // LD-SYNC phase 2
      //             if (!timeOut())
      //             {
      //               if (detectZeroCrossing(finalValue))
      //               {                    
      //                 timeToEdge2 = readTimer();
      //                 float delta = timeToEdge1 - timeToEdge2;

      //                 //SerialHW.println("Delta: " + String(delta));

      //                 if (delta >= timeSyncSeparation)
      //                 {

      //                   //SerialHW.println("Delta: " + String(delta));
      //                   //SerialHW.println("Wait for DATA");

      //                   LAST_MESSAGE = "OK SYNC";                
      //                   _hmi.updateInformationMainPage();                        

      //                   // LD-BITS
      //                   stopTimer();
      //                   resetTimer();
      //                   state=5;
      //                 }
      //                 else
      //                 {
      //                   // Si es demasiado corto. No es SYNC1
      //                   // Sigo buscando antes de que se me acabe el tiempo
      //                   stopTimer();
      //                   resetTimer();
      //                   state=3;
      //                 }
      //               }
      //               else
      //               {
      //                 // Continuamos hasta el time-out
      //                 state = 4;
      //               }
      //             } 
      //             else
      //             {
      //               // Time out
      //               // Si es demasiado largo no es el SYNC
      //               // lo sigo buscando.
      //               stopTimer();
      //               resetTimer();
      //               //
      //               state=3;
      //             }             
      //             break;

      //           case 5:
      //             //
      //             // LD-BITS
      //             //
      //             // Detección de bit0 ó bit1
      //             // Buscamos un bit 0, pero si se excede el time out, 
      //             // es entonces un bit 1
      //             setTimer(timeForBit1);
      //             if (!timeOut())
      //             {
      //                 if (detectZeroCrossing(finalValue))
      //                 {                    
      //                   timeToEdge1 = readTimer();
      //                   // Es posiblemente un BIT0
      //                   state=6;
      //                 }
      //                 else
      //                 {
      //                   // Sigo buscando antes de que se me acabe el tiempo
      //                   state=5;
      //                 }
      //             }
      //             else
      //             {
      //               // Empezamos otra vez en el LOOP
      //               stopTimer();
      //               resetTimer();

      //               state = 5;
      //             }
      //            break;

      //           case 6:
      //             if (!timeOut())
      //             {
      //                 if (detectZeroCrossing(finalValue))
      //                 {                    
      //                   timeToEdge2 = readTimer();
      //                   float delta = timeToEdge1 - timeToEdge2;
      //                   // Es un BIT1
      //                   if (delta >= timeBit1Separation)
      //                   {
      //                       // Es un BIT1
      //                       bitString += "1";
      //                       bitChStr[bitCount] = '1';
      //                       bitCount++;
      //                   }
      //                   else if ((delta >= timeBit0Separation) && (delta < timeBit1Separation))
      //                   {
      //                       // Es un BIT0
      //                       bitString += "0";
      //                       bitChStr[bitCount] = '0';
      //                       bitCount++;
      //                   }

      //                   // ************************************************
      //                   // Conversión de bits a bytes
      //                   // ************************************************
      //                   if (bitCount > 7)
      //                   {
      //                     // Reiniciamos el contador de bits
      //                     bitCount=0;

      //                     // Convertimos a byte el valor leido del DATA
      //                     byte value = strtol(bitChStr, (char**) NULL, 2);
      //                     byteRead = value;
      //                     uint8_t valueToBeWritten = value;
                          
      //                     // Escribimos en fichero el dato
      //                     _mFile.write(valueToBeWritten);

      //                     // Guardamos el checksum generado
      //                     lastChk = checksum;
      //                     // Calculamos el nuevo checksum
      //                     checksum = checksum ^ value;

      //                     // Lo representamos
      //                     // SerialHW.print(bitString + " - ");
      //                     // SerialHW.print(value,HEX);
      //                     // SerialHW.print(" - str: ");
      //                     // SerialHW.print((char)value);
      //                     // SerialHW.print(" - chk: ");
      //                     // SerialHW.print(checksum,HEX);
      //                     // SerialHW.println("");                    
                        
      //                     proccesByteData();   

      //                     // Mostramos el progreso de grabación del bloque
      //                     //_hmi.writeString("progression.val=" + String((int)((byteCount*100)/(header.blockSize-1))));                        

      //                     // Contabilizamos bytes
      //                     byteCount++;
      //                     // Reiniciamos la cadena de bits
      //                     bitString = "";
      //                   }

      //                   //
      //                   // *****************************************************
      //                   //
      //                   stopTimer();
      //                   resetTimer();
      //                   state=5;
      //                 }
      //                 else
      //                 {
      //                   // Sigo buscando antes de que se me acabe el tiempo
      //                   state=6;
      //                 }
      //             }
      //             else
      //             {
      //               // Empezamos otra vez en el LOOP
      //               stopTimer();
      //               resetTimer();
      //               //
      //               state = 5;
      //             }
      //            break;

      //         }

      //         // Contamos el tiempo de una muestra que son aprox. 22.4us
      //         // ya que el contenido de las muestras se temporizan en diferido
      //         // al estar todas en un buffer
      //         countTime();
      //       }

      //       // Leemos el siguiente valor del buffer canal R
      //       oneValue = *value_ptr++;
      //       // Leemos el siguiente valor del buffer canal L
      //       oneValue2 = *value_ptr++; 
      //   }
      // }

      void readBuffer_old(int len)
      {

        int16_t *value_ptr = (int16_t*) buffer;
        int16_t oneValue = *value_ptr++;
        int16_t oneValue2 = *value_ptr++;
        int16_t finalValue = 0;

          for (int j=0;j<len/4;j++)
          {  
              // threshold_high = 20000;
              // threshold_low = -20000;

              finalValue = schmittDetector(oneValue,threshold_high,threshold_low);
              

              // Esto es para modo depuración. 
              // TRUE  -> No depura
              // FALSE -> Si depura. Para conectar el serial plotter de arduino.

              if (true)
              {
                if (detectZeroCrossing(finalValue))
                {

                    //
                    // Control del estado de señal
                    //
                    // Detección de silencio
                    if (pulseWidth > 100 && byteCount > 0)
                    {
                      // Si estamos capturando datos
                      // y el width es grande entonces es silencio
                      isSilence = true;
                    }
                    else
                    {
                      isSilence = false;
                    }                    

                    switch (state)
                    {
                      case 0:
                        if (pilotPulseCount <= 800)
                        {
                            if (pulseWidth >= minLead && pulseWidth <= maxLead)
                            {
                                //SerialHW.println("Pilot count: " + String(pilotPulseCount));
                                pilotPulseCount++;
                            }
                        }
                        else if (pilotPulseCount > 800)
                        {
                            // Saltamos a la espera de SYNC1 y 2
                            state = 1;
                            pilotPulseCount = 0;
                            SerialHW.println("Listening");                                  

                            LAST_MESSAGE = "Recorder listening.";
                            _hmi.updateInformationMainPage();  

                            // SerialHW.println("REC. Wait for SYNC1");
                            // SerialHW.println("");

                            // Limpiamos el buffer
                            //initializeBuffer(); 

                            // Escribimos los primeros bytes
                            if (waitingHead)
                            {
                                uint8_t firstByte = 19;
                                uint8_t secondByte = 0;
                                _mFile.write(firstByte);
                                _mFile.write(secondByte);

                                waitingHead = false; 

                                // SerialHW.println("");
                                // SerialHW.println("Writting first bytes.");
                                // SerialHW.println("");
                            }
                        }                      
                        break;

                      case 1:
                        if (pulseWidth < minLead)
                        {
                            pwSync1 = pulseWidth;
                            state = 20;
                            // SerialHW.println("REC. Wait for SYNC2");
                            // SerialHW.println("");
                        }                      
                      break;

                      case 20:
                        if ((pulseWidth < minLead))
                        {
                            pwSync2 = pulseWidth;
                            int delta = pwSync1 + pwSync2;
                            if (delta > 14 && delta < 20)
                            {
                                state = 21;
                                // SerialHW.println("REC. Wait for DATA");
                                //SerialHW.println("");
                            }
                            else
                            {
                              // Buscamos otro SYNC
                              state = 1;
                            }
                        }
                        break;
                      
                      case 21:
                        // Cojo el primer pulseWidth
                        if ((pulseWidth < minLead))
                        {
                            pwBit0_1 = pulseWidth;
                            state = 2;
                        }
                        else
                        {
                          state = 21;
                        }

                        break;    

                      case 2:
                        // Cojo el segundo pulseWidth
                        pwBit0_2 = pulseWidth;
                        int delta = pwBit0_1 + pwBit0_2;
                        if (delta > 0 && delta < 40)
                        {
                            state = 21;
                            bitString += "0";
                            bitChStr[bitCount] = '0';
                            bitCount++;
                        }
                        else if (delta >= 40 && delta < 65)
                        {
                            state = 21;
                            bitString += "1";
                            bitChStr[bitCount] = '1';
                            //byteRead += (2^(7-bitCount));
                            bitCount++;
                        }
                        else
                        {
                          if (delta > 100)
                          {
                            SerialHW.println("");
                            SerialHW.println("-- Silence -- " + String(delta * 0.0224) + " ms");
                          }
                          else
                          {
                            SerialHW.println("[ Bit error: " + String(delta) + " ]");                            
                          }
                          state = 21;
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
                          uint8_t valueToBeWritten = byteRead;
                          
                          // Escribimos en fichero el dato
                          _mFile.write(valueToBeWritten);

                          // Guardamos el checksum generado
                          lastChk = checksum;
                          // Calculamos el nuevo checksum
                          checksum = checksum ^ byteRead;

                          // // Lo representamos
                          // //SerialHW.print(bitString + " - ");
                          // if (byteRead < 16)
                          // {
                          //   SerialHW.print("0");
                          //   SerialHW.print(byteRead,HEX);
                          // }
                          // else
                          // {
                          //   SerialHW.print(byteRead,HEX);
                          // }
                          // SerialHW.print(" ");
                          // // SerialHW.print(" - str: ");
                          // // if (value > 32)
                          // // {
                          // //   SerialHW.print((char)value);
                          // // }
                          // // else
                          // // {
                          // //   SerialHW.print("?");
                          // // }
                          // //SerialHW.print(" - chk: ");
                          // //SerialHW.print(checksum,HEX);
                          // bytesXlinea++;
                          // if (bytesXlinea>15)
                          // {
                          //   bytesXlinea =0;
                          //   SerialHW.println("");                    
                          // }
                          
                          proccesByteData();   

                          // Mostramos el progreso de grabación del bloque
                          _hmi.writeString("progression.val=" + String((int)((byteCount*100)/(header.blockSize-1))));                        

                          // Contabilizamos bytes
                          byteCount++;
                          // Reiniciamos la cadena de bits
                          bitString = "";
                          byteRead = 0;

                        }
                        break;
                    }
                    //SerialHW.println("Pulse width: " + String(pulseWidth));
                }

                // Hay silencio, pero ¿Es silencio despues de bloque?
                if (isSilence == true)
                {
                  // Si lo es. Reiniciamos los marcadores
                  isSilence = false;


                  //blockEnd = false;


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
                    // Limpiamos el buffer
                    //initializeBuffer();                                
                    
                  } 
                  else if (byteCount != 0 && checksum !=0)
                  {
                    SerialHW.println("");
                    SerialHW.println("Corrupted data detected. Error.");
                    SerialHW.println("");

                    LAST_MESSAGE = "Error. Corrupted data.";

                    // Guardamos ahora en fichero
                    recordingFinish = true;
                    errorInDataRecording = true;
                    blockCount = 0;
                    errorsCountInRec++;
                    // Actualizamos el HMI.
                    _hmi.updateInformationMainPage(); 

                  } 
                  else
                  {}

                  SerialHW.println("");
                  SerialHW.println("Bytes transfered: " + String(byteCount));
                  SerialHW.println("");

                  delay(25);
                  
                  // Comenzamos otra vez
                  state = 0;
                  byteCount = 0;      
                  bytesXlinea = 0;
                  
                  // Hay que reiniciar para el disparador de Schmitt
                  detectStateSchmitt=0;                                         
                  // Reiniciamos el detector de paso por cero
                  detectState=0;
                  samplesCrossing = 0;
                  
                  pulseWidth = 0;
                  checksum = 0; 
                  bitCount = 0;
                  bitString = "";


                  // if (buffer != NULL)
                  // {free(buffer);}
                  // buffer = (uint8_t*)calloc(BUFFER_SIZE,sizeof(uint8_t));
                  // initializeBuffer();

                }
              }
              else
              {
                  bool st = detectZeroCrossing(finalValue);                
              }
              // Leemos el siguiente valor del buffer canal R
              oneValue = *value_ptr++;
              // Leemos el siguiente valor del buffer canal L
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
              int totalSamplesForTh = 15000;
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

              if (averageThreshold <= 700)
              {
                // N-Go
                SerialHW.println("");
                SerialHW.println("Threshold for N-Go");
                threshold_high = 20000;
                threshold_low = -20000;

                //LAST_MESSAGE = "Setting filter. Please wait.";                
                //LAST_MESSAGE = LAST_MESSAGE + " [ Next / N-Go ] " + String(averageThreshold);
              }
              else
              {
                // Classic versions
                SerialHW.println("");
                SerialHW.println("Threshold for classic versions");
                threshold_high = 6000;
                threshold_low = -6000;

                //LAST_MESSAGE = "Setting filter. Please wait.";                
                //LAST_MESSAGE = LAST_MESSAGE + " [ Classic ] " + String(averageThreshold);
              }

              LAST_MESSAGE = "Recorder ready. Play source data.";
              _hmi.updateInformationMainPage();  
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
          //readBuffer(len,true);
          readBuffer_old(len);
        
        // Despreciamos ahora algunas muestras para trabajar el buffer
          // if (!stabilizationPhaseFinish)
          // {
          //   if (countStabilizationSteps>=10)
          //   {
          //       stabilizationPhaseFinish = true;
          //       countStabilizationSteps = 0; 
          //       LAST_MESSAGE = "Recorder ready. Listening.";
          //       _hmi.updateInformationMainPage(); 
          //   }
          //   else
          //   {
          //     // Las muestras se descartan
          //     readBuffer(len,false);
          //     countStabilizationSteps++;
          //   }
          // }
          // else
          // {
          //   // Ahora las muestras pasan por el analizdor
          //   readBuffer(len,true);
          // }


          if (recordingFinish || wasFileNotCreated)
          {
            // Paramos el recorder
            return true;
          }
          else
          {
            // No paramos el recorder
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
          if (!_mFile.open(fileName, O_WRITE | O_CREAT)) 
          {
            SerialHW.println("File for REC failed!");
            wasFileNotCreated = true;
          }
          else
          {
            SerialHW.println("File for REC prepared.");
            wasFileNotCreated = false;
          }          
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
        stabilizationPhaseFinish = false;
        countStabilizationSteps=0;        
        state = 0;
        detectState = 0;
        detectStateSchmitt = 0;   
        samplesCrossing = 0;
        pulseWidth = 0;             
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

          if (!wasFileNotCreated)
          {
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
          }
          else
          {
            _mFile.close();
          }
          wasRenamed = false;
          nameFileRead = false;

          _hmi.writeString("progression.val=0");     
          _hmi.updateInformationMainPage();     
          
      }

      TAPrecorder()
      {}
};
