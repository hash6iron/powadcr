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
 
 #pragma once

class TAPrecorder
{

    public:

      bool errorInDataRecording = false;
      bool wasFileNotCreated = false;
      bool stopRecordingProccess = false;
      //int errorsCountInRec = 0;

      bool wasRenamed = false;
      bool nameFileRead = false;
      bool notAudioInNotified = false;
      bool wasSelectedThreshold = false;
      int totalBlockTransfered = 0;
      
      
    private:

      AudioKit _kit;

      hw_timer_t *timer = NULL;

      HMI _hmi;
      SdFat32 _sdf32;
      tTZX _myTZX;
      File32 _mFile;

      const int BUFFER_SIZE_REC = 4;
      uint8_t* bufferRec;

      // Test Line in/out
      // const int BUFFER_SIZE_IN_OUT = 1024;
      // uint8_t bufferIn[1024];
      // uint8_t bufferOut[1024];

      // Comunes
      char* fileName;
      char* fileNameRename;      
      char* recDir;

      // Estamos suponiendo que el ZX Spectrum genera signal para grabar
      // con timming estandar entonces
      float tState = 1/3500000;
      int pilotPulse = 2168; // t-states
      int sync1 = 667;
      int sync2 = 735;
      int bit0 = 855;
      int bit1 = 1710;

      int pSync = 0;
      int pLead = 0;
      int state2 = 0;

      // Tiempos de señal
      // Tono guia
      const int timeForLead = 1714; //1714;   // Lapara buscar
      const int timeLeadSeparation = 585; 
      // Con samples
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
      //const float samplingRate = 48000.0;
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

      // Umbral para rechazo de ruido al 20%
      const int defaultThH = 6554;
      const int defaultThL = -6554;
      //
      int threshold_high = defaultThH; 
      int threshold_low = defaultThL; 
      //
      int totalSamplesToTest = 0;
      int countNoiseSamples = 0;
      bool resetSchmitt = false;
      int lastColorSchmittIndicator = 0;
      int nResponse=0;


      // Para los clásicos
      //int threshold_high = defaultThH; 
      //int threshold_low = defaultThL;       
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
      int pwLead1 = 0;
      int pwLead2 = 0;
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

      // Info block
      int headState = 0;
      int stateInfoBlock = 0;

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
          
          SerialHW.println("");
          SerialHW.println("State info block: " + String(stateInfoBlock));

          // Si el conjunto leido es de 19 bytes. Es una cabecera.
          if (header.blockSize == 19 && stateInfoBlock==0)
          {
            stateInfoBlock=1;

            if (header.type == 0)
            {
              strncpy(LAST_TYPE,"PROGRAM.HEAD",sizeof("PROGRAM.HEAD"));
            }
            else if (header.type == 1)
            {
              strncpy(LAST_TYPE,"NUMBER_ARRAY.HEAD",sizeof("NUMBER_ARRAY.HEAD"));
            }
            else if (header.type == 2)
            {
              strncpy(LAST_TYPE,"NUMBER_ARRAY.HEAD",sizeof("NUMBER_ARRAY.HEAD"));
            }
            else if (header.type == 3)
            {
              strncpy(LAST_TYPE,"BYTE.HEAD",sizeof("BYTE.HEAD"));
            }            
          }
          else if (header.blockSize > 0 && stateInfoBlock==1)
          {
            
            stateInfoBlock=0;

            if (header.blockSize == 6912)
            {
              strncpy(LAST_TYPE,"SCREEN.DATA",sizeof("SCREEN.DATA"));
              SerialHW.println("SCREEN");
            }
            else
            {
              if (header.type != 0)
              {
                  // Es un bloque de datos BYTE
                  strncpy(LAST_TYPE,"BYTE.DATA",sizeof("BYTE.DATA"));
                  SerialHW.println("BYTE.DATA");
              }
              else if (header.type == 0)
              {
                  // Es un bloque BASIC
                  strncpy(LAST_TYPE,"BASIC.DATA",sizeof("BASIC.DATA"));
                  SerialHW.println("BASIC.DATA");
              }
            }
          }   

      }

      void renameFile()
      {
        // Reservamos memoria             
        char* cPath = (char*)ps_calloc(55,sizeof(char));
        String dirR = RECORDING_DIR + "/\0";
        cPath = strcpy(cPath, dirR.c_str());

        // Solo necesitamos el fichero final
        // porque mFile contiene el original
        strcat(cPath,fileNameRename);

        SerialHW.println("");
        SerialHW.println("File searching? " + String(cPath));
        SerialHW.println("");

        if (_sdf32.exists(cPath))
        {
          _sdf32.remove(cPath);
          SerialHW.println("");
          SerialHW.println("File removed --> " + String(cPath));
          SerialHW.println("");
        }

        if (_mFile.rename(cPath))
        {
          SerialHW.println("");
          SerialHW.println("File renamed --> " + String(cPath));
          SerialHW.println("");
          
          wasRenamed = true;
        }
      }

      void getFileName()
      {
        if (!nameFileRead)
        {
          // if (fileNameRename != NULL)
          // {
          //   free(fileNameRename);
          // }

          // Proporcionamos espacio en memoria para el
          // nuevo filename
          fileNameRename = (char*)ps_calloc(20,sizeof(char));

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

            // Si es una cabecera. Cogemos el tamaño del bloque data a
            // salvar.
            if (byteCount == 19 && headState==0)
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

                headState = 1;
            }
            else if (byteCount > 0 && headState==1)
            {
                // BLOQUE DATA

                // Guardamos ahora en fichero
                errorInDataRecording = false;
                waitingHead = true;
                stopRecordingProccess = false;

                // Inicializamos para el siguiente bloque
                header.blockSize = 19;  
                headState = 0;              
                
            }
            else
            {
                SerialHW.println("");
                SerialHW.println("Error in head information. Only " + String(byteCount) + " bytes were read.");
                SerialHW.println("");

                // Marcamos el error de cabecera
                errorInDataRecording = true;
                stopRecordingProccess = true;
                waitingHead = true;

                // Inicializamos para el siguiente bloque
                headState = 0;
                header.blockSize = 19;                
            }

            SerialHW.println("");
            SerialHW.println("Block data ------------------------");
            SerialHW.println("");
            SerialHW.println("Total bytes: " + String(byteCount));
            SerialHW.println("");

            //LAST_SIZE = header.blockSize;  
            //_hmi.updateInformationMainPage();
        }
        else
        {
          SerialHW.println("");
          SerialHW.println("Unknow error in proccess info block");
        }  
      }

      void analyzeSamplesForThreshold(int len)
      {

        int16_t *value_ptr = (int16_t*) bufferRec;
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
              //SerialHW.println("pw: " + String(samplesCrossing));                           
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
              // Nos aseguramos que el pulso es un pulso y no ruido
              // para eso exigimos que tenga al menos un mínimo de ancho
              detectState=1;
              zeroCrossing = true;   
              //SerialHW.println("pw: " + String(samplesCrossing));              
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
          pulseWidth = samplesCrossing;
          samplesCrossing=0;
        }

        // Devolvemos el estado de paso por cero  
        return zeroCrossing;    
      }

      // int16_t schmittDetector(int16_t value, int16_t thH, int16_t thL, bool reset)
      // {

      //   int16_t switchStatus = lastSwitchStatus;
      //   // Si hay un cambio, este no afecta hasta pasadas
      //   // las responseSamples
      //   const int responseSamples = 5;

      //   if (reset)
      //   {
      //     detectStateSchmitt = 0;
      //     switchStatus=0;
      //   }

      //   switch (detectStateSchmitt)
      //   {
      //     case 0:
      //       // Estado inicial
      //       nResponse=0;

      //       if (value > thH)
      //       {
      //         // HIGH
      //         detectStateSchmitt = 1;              
      //       }
      //       else if (value < thL)
      //       {
      //         // LOW
      //         detectStateSchmitt = 2;              
      //       }
      //       else
      //       {
      //         detectStateSchmitt = 0;
      //       }
      //       break;

      //     case 1:
      //       // Estado en HIGH
      //       if (value < thL)
      //       {
      //         if (nResponse>=responseSamples)
      //         {
      //           switchStatus = low;
      //           detectStateSchmitt = 2;
      //           nResponse=0;
      //         }
      //         else
      //         {
      //           nResponse++;
      //         }
      //       }
      //       else if (value > thH)
      //       {              
      //         detectStateSchmitt = 1;
      //         nResponse=0;
      //       }
      //       else
      //       {
      //         detectStateSchmitt = 1;
      //       }
      //       break;

      //     case 2:
      //       // Estado en LOW
      //       if (value < thL)
      //       {
      //         detectStateSchmitt = 2;              
      //       }
      //       else if (value > thH)
      //       {
      //         if (nResponse>=responseSamples)
      //         {
      //           switchStatus = high;
      //           detectStateSchmitt = 1;
      //           nResponse=0;
      //         }
      //         else
      //         {
      //           nResponse++;
      //         }
      //       }
      //       else
      //       {
      //         detectStateSchmitt = 2;
      //         nResponse=0;
      //       }            
      //       break;
      //   }
        
      //   lastSwitchStatus = switchStatus;
      //   return switchStatus;
      // }

      int16_t schmittDetector(int16_t value, int16_t thH, int16_t thL, bool reset)
      {

        int16_t switchStatus = lastSwitchStatus;

        if (reset)
        {
          detectStateSchmitt = 0;
          switchStatus=0;
        }

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



      void readBuffer(int len)
      {

        int16_t *value_ptr = (int16_t*) bufferRec;
        int16_t oneValue = *value_ptr++;
        int16_t oneValue2 = *value_ptr++;
        int16_t finalValue = 0;

        // Para modo debug.
        // Esto es para modo depuración. 
        //
        bool showDataDebug = SHOW_DATA_DEBUG;
        
        // Ajuste del detector de señal
        // Muestras del tono guia leidas
        int maxPilotPulseCount = MAX_PULSES_LEAD;//800;

        // Señal Lead
        int minLead = MIN_LEAD;//50;
        int maxLead = MAX_LEAD;//56;
        // Señal de SYNC
        int minSync = MIN_SYNC; //14
        int maxSync = MAX_SYNC; //20
        // Señal para BIT 0
        int minBit0 = MIN_BIT0;//1;
        int maxBit0 = MAX_BIT0;//39;
        // Señal para BIT 1
        int minBit1 = MIN_BIT1;//40;
        int maxBit1 = MAX_BIT1;//65;
        //
  
        if (!WasfirstStepInTheRecordingProccess)
        {
          LAST_MESSAGE = "Recorder ready. Play source data.";
          _hmi.updateInformationMainPage(); 
          WasfirstStepInTheRecordingProccess=true;         
        }

        for (int j=0;j<len/4;j++)
        {  
            if (EN_SCHMITT_CHANGE)
            {
              threshold_high = (SCHMITT_THR * 32767)/100;
              threshold_low = (-1)*((SCHMITT_THR * 32768)/100); 

              if (LAST_SCHMITT_THR != SCHMITT_THR)   
              {
                SerialHW.println("Thr modified: " + String(threshold_high));
              }
              // Actualizamos el ultimo cambio
              LAST_SCHMITT_THR = SCHMITT_THR;
            }
            else
            {
              // Fijamos threshold en 20%
              threshold_high = defaultThH;
              threshold_low = defaultThL;
            }

            // Elección del canal de escucha
            if (SWAP_MIC_CHANNEL)
            {
              finalValue = schmittDetector(oneValue,threshold_high,threshold_low,false);              
            }
            else
            {
              // Cogemos el canal LEFT
              finalValue = schmittDetector(oneValue2,threshold_high,threshold_low,false);              
            }

            if (true)
            {
                if (detectZeroCrossing(finalValue))
                {
                    //delay(1);
                    //
                    // Control del estado de señal
                    //
                    // Detección de silencio
                    if (pulseWidth > 100 && byteCount > 0)
                    {
                      // Si estamos capturando datos
                      // y el width es grande entonces es silencio
                      isSilence = true;
                      if (showDataDebug)
                      {
                        SerialHW.println("Silence");                            
                      }                      
                    }
                    else
                    {
                      isSilence = false;
                    }                    

                    switch (state)
                    {
                      case 0:
                        if (pulseWidth >= (minLead/2))
                        {
                          
                          if (showDataDebug)
                          {
                            SerialHW.println("Pilot count: " + String(pilotPulseCount));                            
                          }

                          pwLead1 = pulseWidth;
                          state = 40;                    
                        }
                        else
                        {
                          state = 0;
                        }
                        break;
                      case 40:
                        if (pulseWidth >= (minLead/2))
                        {
                            pwLead2 = pulseWidth;
                            int delta = pwLead1 + pwLead2;

                            if (delta >= minLead && delta <= maxLead)
                            {
                                if (pilotPulseCount <= maxPilotPulseCount)
                                {
                                  pilotPulseCount++;
                                }
                                else if (pilotPulseCount > maxPilotPulseCount)
                                {
                                    // Saltamos a la espera de SYNC1 y 2
                                    state = 1;
                                    pilotPulseCount = 0;
                               
                                    LAST_MESSAGE = "Recorder listening.";
                                    _hmi.updateInformationMainPage();  

                                    if (showDataDebug)
                                    {
                                      SerialHW.println("REC. Wait for SYNC1");
                                      SerialHW.println("");
                                    }

                                    // Escribimos los primeros bytes
                                    if (waitingHead)
                                    {
                                        uint8_t firstByte = 19;
                                        uint8_t secondByte = 0;
                                        _mFile.write(firstByte);
                                        _mFile.write(secondByte);

                                        waitingHead = false; 

                                        if (showDataDebug)
                                        {
                                          SerialHW.println("");
                                          SerialHW.println("Writting first bytes.");
                                          SerialHW.println("");
                                        }
                                    }
                                }                          
                            }
                            else
                            {
                              //pilotPulseCount=0;
                              //state=0;
                            }
                        }
                        else
                        {
                          state = 0;
                        }            
                        break;

                      case 1:
                        if (pulseWidth < (minLead/2))
                        {
                            pwSync1 = pulseWidth;
                            state = 20;

                            if (showDataDebug)
                            {
                              SerialHW.println("REC. Wait for SYNC2");
                              SerialHW.println("");
                            }
                        }                      
                        break;

                      case 20:
                        if ((pulseWidth < (minLead/2)))
                        {
                            pwSync2 = pulseWidth;
                            int delta = pwSync1 + pwSync2;
                            if (delta > minSync && delta < maxSync)
                            {
                              state = 21;

                              if (showDataDebug)
                              {
                                SerialHW.println("REC. Wait for DATA");
                                SerialHW.println("");
                              }
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
                        if ((pulseWidth < (minLead/2)))
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
                        if (delta >= minBit0 && delta <= maxBit0)
                        {
                            state = 21;
                            bitString += "0";
                            bitChStr[bitCount] = '0';
                            bitCount++;
                        }
                        else if (delta >= minBit1 && delta <= maxBit1)
                        {
                            state = 21;
                            bitString += "1";
                            bitChStr[bitCount] = '1';
                            bitCount++;
                        }
                        else
                        {
                          if (delta > maxBit1)
                          {
                            // Es silencio
                            if (showDataDebug)
                            {
                              SerialHW.println("");
                              SerialHW.println("-- Silence -- " + String(delta * 0.0224) + " ms");
                            }
                          }
                          else
                          {
                            if (showDataDebug)
                            { 
                              SerialHW.println("[ Bit, " + String(bitCount) + " from byte, " + String(byteCount) + " error. Delta: " + String(delta) + " ]");                            
                            }
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

                          // Lo representamos
                          //SerialHW.print(bitString + " - ");
                          if (showDataDebug)
                          {
                            if (byteRead < 16)
                            {
                              SerialHW.print("0");
                              SerialHW.print(byteRead,HEX);
                            }
                            else
                            {
                              SerialHW.print(byteRead,HEX);
                            }
                            SerialHW.print(" ");

                            bytesXlinea++;
                            if (bytesXlinea>15)
                            {
                              bytesXlinea =0;
                              SerialHW.println("");                    
                            }
                          }

                          if (showDataDebug)
                          {
                            SerialHW.print(" [");
                            SerialHW.print(checksum,HEX);
                            SerialHW.print("] ");
                          }

                          
                          // Mostramos información de la cabecera
                          proccesByteData();   

                          // Mostramos el progreso de grabación del bloque
                          if (!showDataDebug)
                          {
                            _hmi.writeString("progression.val=" + String((int)((byteCount*100)/(header.blockSize-1))));                        
                          }

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

                // ***********************************************************
                //
                //        Hay silencio, entonces ha finalizado un bloque.
                //
                // ***********************************************************

                if (isSilence == true)
                {
                  // Si lo es. Reiniciamos el flag de silencio
                  isSilence = false;

                  //
                  // Fin de bloque
                  //

                  int blkSize = header.blockSize;
                  
                  if (checksum==0 && byteCount==blkSize)
                  {
                    // Si el checksum final es 0, y contiene datos
                    // El bloque es correcto.

                    // NOTA: El ultimo byte es el checksum
                    // si se hace XOR sobre este mismo da 0
                    // e indica que todo es correcto

                    SerialHW.println("");
                    SerialHW.println("Block capture correctly.");

                    // Informamos de los bytes salvados
                    LAST_SIZE = blkSize; 
                    // Informamos del bloque en curso
                    BLOCK_SELECTED = blockCount;

                    // Procesamos información del bloque
                    proccessInfoBlock();
                    // Actualizamos el HMI.
                    _hmi.updateInformationMainPage(); 

                    // Incrementamos un bloque  
                    blockCount++;      
                  } 
                  else if ((byteCount!=0 && byteCount!=blkSize && checksum !=0) || (byteCount==blkSize && checksum !=0))
                  {
                    SerialHW.println("");
                    SerialHW.println("Corrupted data detected. Error.");
                    SerialHW.println("");

                    LAST_MESSAGE = "Error. Corrupted data.";
                    // Actualizamos el HMI.
                    _hmi.updateInformationMainPage();
                    delay(1000);

                    // Reiniciamos todo
                    state = 0;
                    byteCount = 0;      
                    bytesXlinea = 0;
                    checksum = 0; 
                    bitCount = 0;
                    bitString = "";
                    stateInfoBlock = 0;
                    // Cero bloques capturados correctos
                    blockCount = 0;
                    //waitingHead = true;
                    totalBlockTransfered = 0;

                    // Hay que reiniciar para el disparador de Schmitt
                    detectStateSchmitt=0;                                         
                    // Reiniciamos el detector de paso por cero
                    detectState=0;
                    samplesCrossing = 0;
                    // Otros que inicializamos
                    pulseWidth = 0;
                    
                    // Forzamos a parar. No seguimos.
                    // ------------------------------
                    errorInDataRecording = true;
                    stopRecordingProccess = true;
                    //
                    // Finalizamos
                    break;
                  } 
                  else
                  {
                    // Reiniciamos todo
                    state = 0;
                    byteCount = 0;      
                    bytesXlinea = 0;
                    checksum = 0; 
                    bitCount = 0;
                    bitString = "";
                    stateInfoBlock = 0;                   
                    // Cero bloques capturados correctos
                    blockCount = 0;
                    totalBlockTransfered = 0;
                    //waitingHead = true;

                    // Hay que reiniciar para el disparador de Schmitt
                    detectStateSchmitt=0;                                         
                    // Reiniciamos el detector de paso por cero
                    detectState=0;
                    samplesCrossing = 0;
                    // Otros que inicializamos
                    pulseWidth = 0;
                    
                    // Forzamos a parar. No seguimos.
                    // ------------------------------
                    errorInDataRecording = true;
                    stopRecordingProccess = true;

                    LAST_MESSAGE = "Unknow  error.";
                    // Actualizamos el HMI.
                    _hmi.updateInformationMainPage();  
                    delay(1000);

                    // Finalizamos
                    break;                  
                  }

                  SerialHW.println("");
                  SerialHW.println("Bytes transfered: " + String(byteCount));
                  SerialHW.println("");

                  // Hacemos una pausa para estabilizar buffer
                  delay(25);
                  
                  // --------------------------------------------------------
                  //
                  // TODO OK - Continuamos con mas bloques si no hay STOP
                  // --------------------------------------------------------
                  // Comenzamos otra vez
                  // --------------------------------------------------------
                  state = 0;
                  byteCount = 0;      
                  bytesXlinea = 0;
                  checksum = 0; 
                  bitCount = 0;
                  bitString = "";
                  //stateInfoBlock = 0;
                  // Guardamos los bloques transferidos
                  totalBlockTransfered = blockCount;

                  // Hay que reiniciar para el disparador de Schmitt
                  detectStateSchmitt=0;                                         
                  // Reiniciamos el detector de paso por cero
                  detectState=0;
                  samplesCrossing = 0;
                  // Otros que inicializamos
                  pulseWidth = 0;
                }
            }
            else
            {
              SerialHW.println(String(finalValue));              
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
        if (EN_SCHMITT_CHANGE)
        {
          if (!wasSelectedThreshold)
          {
            SerialHW.println("");
            SerialHW.println("Fixed threshold from HMI");
            
            threshold_high = (SCHMITT_THR * 32767)/100;
            threshold_low = (-1)*(SCHMITT_THR * 32768)/100;            

            SerialHW.println("TH+ = " + String(threshold_high));
            SerialHW.println("TH- = " + String(threshold_low));

            wasSelectedThreshold = true;                 
          }       
        }
        else
        {
          threshold_high = defaultThH;
          threshold_low = defaultThL; 
        }
      }

      void initializeBuffer()
      {
        for (int i=0;i<BUFFER_SIZE_REC;i++)
        {
          bufferRec[i]=0;
        }
      }

      bool recording()
      {
          
          size_t len = _kit.read(bufferRec, BUFFER_SIZE_REC);
          //readBuffer(len,true);
          readBuffer(len);
        
          // Despreciamos ahora algunas muestras para trabajar el buffer
          if (stopRecordingProccess)
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

      // void testOutput()
      // {
          
      //     size_t len = _kit.read(bufferIn, BUFFER_SIZE_IN_OUT);
      //     //SerialHW.println("BufferIn len: " + String(len));


      //     int16_t *value_ptr = (int16_t*)bufferIn;
      //     int16_t *ptrOut = (int16_t*)bufferOut;
      //     int16_t oneValue = *value_ptr++;
      //     int16_t oneValue2 = *value_ptr++;
         
      //     int16_t sample = 0;
      //     int16_t sample2 = 0;

      //     for (int j=0;j<len/4;j++)
      //     {
      //       // Leemos el siguiente valor del buffer canal R
      //       oneValue = *value_ptr++;
      //       // Leemos el siguiente valor del buffer canal L
      //       oneValue2 = *value_ptr++; 
      //       // Almacenamos en el buffer de salida
      //       sample = schmittDetector(oneValue,defaultThH,defaultThL,false);

      //       if (detectZeroCrossing(sample))
      //       {
      //         if (sample > defaultThH)
      //         {
      //           sample2 = 32767;
      //         }
      //         else if (sample < defaultThL)
      //         {
      //           sample2 = -32768;
      //         }
      //         else
      //         {
      //           sample2 = 0;
      //         }
      //       }

      //       *ptrOut++=sample2;
      //       *ptrOut++=oneValue2;
      //     }
      //     // Sacamos el buffer procesado
      //     _kit.write(bufferOut, len);                     

      // }

      void prepareHMI()
      {
        // Inicializamos variables del HMI
        BLOCK_SELECTED = 0;
        TOTAL_BLOCKS = 0;
        PROGRAM_NAME = "";
        PROGRAM_NAME_2 = "";
        TYPE_FILE_LOAD = "";
        LAST_SIZE = 0;
        strncpy(LAST_TYPE,"",1);
        strncpy(LAST_NAME,"",1);
      }

      void initialize()
      {
        prepareHMI();

        pLead = 0;
        pSync = 0;
        state2 = 0;

        // Reservamos memoria
        fileName = (char*)ps_calloc(20,sizeof(char));
        fileName = "_record\0";

        recDir = (char*)ps_calloc(55,sizeof(char));
        String dirR = RECORDING_DIR + "/\0";
        recDir = strcpy(recDir, dirR.c_str());
        
        if (!_sdf32.mkdir(RECORDING_DIR))
        {
          SerialHW.println("Error. Recording dir wasn't create or exist.");
        }

        // Se crea un nuevo fichero temporal con la ruta del REC
        strcat(recDir, fileName);
        SerialHW.println("Dir for REC: " + String(recDir));

        // Inicializo bit string
        bitChStr = (char*)ps_calloc(8, sizeof(char));
        datablock = (byte*)ps_calloc(1, sizeof(byte));
        
        // Inicializamos el array de nombre del header
        for (int i=0;i<10;i++)
        {
          header.name[i] = ' ';
        }

        bufferRec = (uint8_t*)ps_calloc(BUFFER_SIZE_REC,sizeof(uint8_t));
        initializeBuffer();

        errorInDataRecording = false;
        nameFileRead = false;
        wasRenamed = false;
        blockCount = 0;
        waitingHead = true;
        wasSelectedThreshold = false;
        stabilizationPhaseFinish = false;
        countStabilizationSteps=0;        
        state = 0;
        detectState = 0;
        detectStateSchmitt = 0;   
        samplesCrossing = 0;
        pulseWidth = 0;      
        stopRecordingProccess = false; 
        stateInfoBlock = 0;
        //
        headState = 0;
      }

      bool createTempTAPfile()
      {
        if (!_mFile.open(recDir, O_WRITE | O_CREAT | O_TRUNC)) 
        {
          if (!_mFile.isOpen())
          {
            // Pruebo otra vez. Pero con otros parámetros
            if (!_mFile.open(recDir, O_WRITE | O_CREAT)) 
            {
              // El fichero no fue creado. Entonces no está abierto
              SerialHW.println("Error in file for REC.");
              wasFileNotCreated = true;
              stopRecordingProccess = true;
              return false;
            }
            else
            {
              // El fichero fue creado. Entonces está abierto
              SerialHW.println("File for REC prepared.");
              wasFileNotCreated = false;
              stopRecordingProccess = false;
              return true;
            }             
          }
          else
          {
            // ESTE CASO ES RARO y NO DEBERÍA DARSE NUNCA
            // pero lo controlamos por si acaso.

            // El fichero fue creado. Entonces está abierto
            SerialHW.println("File for REC prepared.");
            wasFileNotCreated = false;            
            stopRecordingProccess = false;
            return true;
          }  
        }
        else
        {
          // El fichero fue creado. Entonces está abierto
          SerialHW.println("File for REC prepared.");
          wasFileNotCreated = false;
          stopRecordingProccess = false;
          return true;
        }        
      }

      void terminate(bool removeFile)
      {
          
          if (!wasFileNotCreated)
          {
              // El fichero inicialmente fue creado con exito
              // en la SD, pero ...
              //
              // 
              if (_mFile.size() !=0)
              {
                  if (!removeFile)
                  {
                    // Finalmente se graba el contenido  
                    if (_mFile.isOpen())
                    {
                      // Lo renombramos con el nombre del BASIC
                      renameFile();         
                    }          

                    // Lo cerramos
                    _mFile.close();
                    delay(125);

                  }
                  else
                  {
                    // SE ELIMINA
                    // No se renombra porque es erroneo
                    _mFile.close();
                    delay(125);
                    // Se elimina
                    _mFile.remove();
                  }
              }
              else
              {
                // Tiene 0 bytes. Meto algo y lo cierro
                // es un error
                SerialHW.println("");
                SerialHW.println("Error. 0 byte file");
                _mFile.write(0xFF);
                _mFile.close();
                _mFile.remove();
              }
          }
          else
          {
            // El fichero no fue creado
            _mFile.close();
            delay(125);
          }
          wasRenamed = false;
          nameFileRead = false;

          _hmi.writeString("progression.val=0");     
          _hmi.updateInformationMainPage();     
          
          // Reiniciamos flags
          // Hay que reiniciar para el disparador de Schmitt
          detectStateSchmitt=0;                                         
          // Reiniciamos el detector de paso por cero
          detectState=0;
          samplesCrossing = 0;
          // Otros que inicializamos
          pulseWidth = 0;

          // Para el mensaje de READY listening
          WasfirstStepInTheRecordingProccess = false;
          // Reiniciamos flags de recording errors
          errorInDataRecording = false;
          stopRecordingProccess = false;
          wasFileNotCreated = true;
      }

      TAPrecorder()
      {}
};
