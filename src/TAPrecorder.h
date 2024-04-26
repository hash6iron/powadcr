/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TAPrecorder.ino
    
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
 
 #include <cmath>

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
      bool WasfirstStepInTheRecordingProccess = false;      
      
      
    private:

      AudioKit _kit;

      hw_timer_t *timer = NULL;

      HMI _hmi;
      SdFat32 _sdf32;
      File32 _mFile;

      const int BUFFER_SIZE_REC = 256; //4 //2048
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
      double tState = 1/3500000;
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
      const int low = -32767;
      const int silence = 0;

      // Definimos el sampling rate real de la placa.
      //const double samplingRate = 44099.988;
      const double samplingRate = 48000.0;
      //const double samplingRate = 22050.0;
      // Calculamos el T sampling_rate en us
      const double TsamplingRate = (1 / samplingRate) * 1000 * 1000;

      // Controlamos el estado del TAP
      int stateRecording = 0;
      // Contamos los pulsos del tono piloto
      int pulseCount = 0;
      int lostPulses = 0;
      // Contamos los pulsos del sync1
      long silenceCount = 0;
      long lastSilenceCount = 0;
      bool isSilence = false;
      //bool blockEnd = false;
      long silenceSample = 0;

      char* bitChStr;
      uint8_t* datablock;
      //uint8_t* fileData;
      int bitCount = 0;
      int byteCount = 0;
      int blockCount = 0;
      String bitString = "";

      uint8_t checksum = 0;
      uint8_t lastChk = 0;
      uint8_t byteRead = 0;

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
      double timeBuffer = 0;
      bool countTimeEnable = false;
      bool startTimeWasLoaded = false;
      bool measureTimeWasLoaded = false;
      bool timeOutAlarm = false;
      
     
      // Porcentaje de error del comparador de tiempo (%)
      const int timeToEdgeThreshold = 2;
      // Tiempo que se ha tardado en encontrar el flanco
      double timeToEdge1 = 0;
      double timeToEdge2 = 0;
      // Crossing por cero
      int16_t lastFinalValue = 0;

      // Zero crossing routine
      int _samplesCount = 0;
      int _lastSamplesCount = 0;
      int _measuredWidth = 0;
      int _lastMeasuredWidth = 0;
      int _measuredPulseUpWidth = 0;
      int _measuredPulseDownWidth = 0;
      int _lastMeasureSemiPulsedWidth = 0;
      bool _pulseWasMeasured = false;
      bool _edgeDetected = false;

      int p1count = 0;
      int p2count = 0;
      int p3count = 0;
      int p4count = 0;
      int silencePulses = 0;

      // int _samplesCountCtrl = 0;
      // long _pulseTime = 0;
      // long _T0 = 0;
      // long _T1 = 0;
      // bool _controlTimer = 0;
      //int _detectState = 0;
      int _detectStateT = 0;
      // bool _lastPulseWasUp = false;
      // bool _lastPulseMeasuredWasUp = false;
      int _samplesDiscard = 0;
      int _measureState = 0;
      int _lastEdgeDetected = 0;
      int _totalSamplesRed = 0;
      int _pulseWidth = 0;
      int pwLead1 = 0;
      int pwLead2 = 0;
      int pwSync1 = 0;
      int pwSync2 = 0;
      int pwBit0_1 = 0;
      int pwBit0_2 = 0;
      //
      int _bitPulseWidth = 0;
      bool _readBitNow = false;
      int _bitState = 0;
      //
      // Detector de Schmitt
      int16_t lastSwitchStatus = 0;
      //int detectStateSchmitt = 0;

      //
      double TON = 0;
      double TOFF = 0;
      double TSI = 0;
      double TControl = 0;
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
          uint8_t sizeLSB;
          uint8_t sizeMSB;
          int blockSize = 19;
      };

      tHeader header;

      // Calculamos las muestras que tiene cada pulso
      // segun el tiempo de muestreo
      // long getSamples(int signalTStates, double tState, int samplingRate)
      // {
      //     double tSampling = 1/samplingRate;
      //     double tPeriod = signalTStates * tState;
      //     long samples = tPeriod / tSampling;

      //     return samples; 
      // }

      uint8_t calculateChecksum(uint8_t* bBlock, int startByte, int numBytes)
            {
                // Calculamos el checksum de un bloque de bytes
                uint8_t newChk = 0;

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
          //SerialHW.print(header.name[n]);
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
              //SerialHW.println("SCREEN");
            }
            else
            {
              if (header.type != 0)
              {
                  // Es un bloque de datos BYTE
                  strncpy(LAST_TYPE,"BYTE.DATA",sizeof("BYTE.DATA"));
                  //SerialHW.println("BYTE.DATA");
              }
              else if (header.type == 0)
              {
                  // Es un bloque BASIC
                  strncpy(LAST_TYPE,"BASIC.DATA",sizeof("BASIC.DATA"));
                  //SerialHW.println("BASIC.DATA");
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

        if (_sdf32.exists(cPath))
        {
          _sdf32.remove(cPath);
          log("File exists then was remove first.");
        }

        if (_mFile.rename(cPath))
        {         
          wasRenamed = true;
        }
      }

      void getFileName()
      {
        if (!nameFileRead)
        {
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

          //char* extFile = ".tap\0";
          char extFile[] = ".tap";
          strcat(fileNameRename,extFile);
          
          nameFileRead = true;                             
        }        
      }

      void proccessInfoBlock()
      {

        checksum = 0;

        if (byteCount !=0)
        {

            // Si es una cabecera. Cogemos el tamaño del bloque data a
            // salvar.
            switch (headState)
            {
            case 0:
              if (byteCount == 19)
              {                         
                  // BLOQUE CABECERA

                  // La información del tamaño del bloque siguiente
                  // se ha tomado en tiempo real y está almacenada
                  // en headar.sizeLSB y header.sizeMSB
                  int size = 0;
                  size = header.sizeLSB + (header.sizeMSB * 256);
                  
                  // Guardamos el valor ya convertido en int
                  header.blockSize = size + 2;
                  
                  // Hay que añadir este valor sumando 2, en la cabecera
                  // antes del siguiente bloque
                  int size2 = size + 2;
                  uint8_t LSB = size2;
                  uint8_t MSB = size2 >> 8;

                  // Antes del siguiente bloque metemos el size
                  _mFile.write(LSB);
                  _mFile.write(MSB);
                  // Mostramos el nombre del programa en el display
                  // ojo que el marcador "nameFileRead" debe estar
                  // despues de esta función, para indicar que el
                  // nombre del programa ya fue leido y mostrado
                  // que todo lo que venga despues es PROGRAM_NAME_2
                  showProgramName();

                  // Obtenemos el nombre que se le pondrá al .TAP
                  getFileName();
                  headState = 1;
              }    
              else
              {
                  // Marcamos el error de cabecera
                  errorInDataRecording = true;
                  stopRecordingProccess = true;
                  waitingHead = true;

                  // Inicializamos para el siguiente bloque
                  headState = 0;
                  header.blockSize = 19;                 
              }        
              break;

            case 1:
              if (byteCount > 0)
              {
                  // BLOQUE DATA
                  // Guardamos ahora en fichero
                  errorInDataRecording = false;
                  waitingHead = true;
                  //stopRecordingProccess = false;

                  // Inicializamos para el siguiente bloque
                  header.blockSize = 19;  
                  headState = 0;              
              } 
              else
              {
                  // Marcamos el error de cabecera
                  errorInDataRecording = true;
                  stopRecordingProccess = true;
                  waitingHead = true;

                  // Inicializamos para el siguiente bloque
                  headState = 0;
                  header.blockSize = 19;                 
              }           
              break;

            }


        } 
      }

      void measurePulseWidth(int16_t value)
      {
          // Buscamos cambios de flanco
          switch (_measureState)
          {
            case 0:
              // Estado inicial
              // establecemos un estado incial unicamente.
              if (value > 0)
              {
                // Cambia a UP
                _measureState = 1;
              }
              else if (value < 0)
              {
                // Cambia a LOW
                _measureState = 3;
              }
              else
              {
                _measureState = 0;
              }
              break;

            case 1:
              // Estado UP
              // Ahora cambia a LOW - Entonces es un flanco
              if (value < 0)
              {
                _measureState = 2;
                _measuredPulseUpWidth = _measuredWidth;
                _pulseWasMeasured = false;
                _edgeDetected = true;
              }
              else
              {
                // Calculo el ancho del semi-pulso alto
                _measuredWidth++;
                _edgeDetected = false;
                _pulseWasMeasured = false;
              }
              break;

            case 2:
              // Estado LOW
              if (value > 0)
              {
                _measureState = 1;
                // Devolvemos el tamaño
                _lastMeasuredWidth = _measuredWidth;
                _pulseWasMeasured = true;
                // Comenzamos otra vez
                _measuredWidth = 1;
                _edgeDetected = true;
              }
              else
              {
                _measuredWidth++;
                _edgeDetected = false;
                _pulseWasMeasured = false;
              }              
              break;

            case 3:
              // Estado LOW
              if (value > 0)
              {
                _measureState = 4;
                _measuredPulseDownWidth = _measuredWidth;
                _pulseWasMeasured = false;
                _edgeDetected = true;              
              }
              else
              {
                _measuredWidth++;
                _edgeDetected = false;
                _pulseWasMeasured = false;
              }              
              break;

            case 4:
              // Estado UP
              // Ahora cambia a LOW - Entonces es un flanco
              if (value < 0)
              {
                _measureState = 3;
                _lastMeasuredWidth = _measuredWidth;
                _measuredWidth = 1;
                _pulseWasMeasured = true;
                _edgeDetected = true;
              }
              else
              {
                // Calculo el ancho del semi-pulso alto
                _measuredWidth++;
                _edgeDetected = false;
                _pulseWasMeasured = false;
              }
              break;
          }
      }

      void prepareNewBlock()
      {
        stateRecording = 0;
        byteCount = 0;      
        bitCount = 0;        
        checksum = 0; 
        bitString = "";
      }

      void writeTAPHeader()
      {
        // Escribimos los primeros bytes
        uint8_t firstByte = 19;
        uint8_t secondByte = 0;
        _mFile.write(firstByte);
        _mFile.write(secondByte);
      }

      void readBuffer(int len)
      {
        int16_t *value_ptr = (int16_t*)bufferRec;
        int16_t oneValue = 0;
        int16_t oneValue2 = 0;
        int16_t finalValue = 0;


        size_t resultOut = 0;                
        uint8_t bufferOut[BUFFER_SIZE_REC];                
        int16_t *ptrOut = (int16_t*)bufferOut;
        int chn = 2;                  

        // Para modo debug.
        // Esto es para modo depuración. 
        //
        bool showDataDebug = SHOW_DATA_DEBUG;
        
        // Ajuste del detector de señal
        // Muestras del tono guia leidas
        int maxPilotPulseCount = MAX_PULSES_LEAD; //255;

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

        const double TSYNC1 = 190.6;
        const double TSYNC2 = 210.0;
        const double TBIT0 = 244.2;
        const double TBIT1 = 488.6;
        const double TLEAD = 619.4;
  
        double currentTime = 0;
        // Samples de referencia para control 
        int samplesRefForCtrl = 0;

        if (!WasfirstStepInTheRecordingProccess)
        {    
          prepareNewBlock();
          // Importante no perder esto
          // el primer bloque esperado es de 19 bytes
          header.blockSize = 19;
          headState = 0;
          _measureState = 0;
          _pulseWasMeasured = false;
          BLOCK_REC_COMPLETED = false;
          //
          //LAST_MESSAGE = "Recorder ready. Play source data.";
          // Ya no pasamos por aquí hasta parar el recorder
          WasfirstStepInTheRecordingProccess=true;    
        }

        // Analizamos todas las muestras del buffer de entrada
        for (int j=0;j<(len/4);j++)
        {  
            // Leemos el siguiente valor del buffer canal R
            oneValue = *value_ptr++;
            // Leemos el siguiente valor del buffer canal L
            oneValue2 = *value_ptr++;              
                           
            // Elección del canal de escucha
            if (SWAP_MIC_CHANNEL)
            {
              finalValue = oneValue;
            }
            else
            {
              // Cogemos el canal LEFT
              finalValue = oneValue2;
            }

            //Aplicamos filtrado y corrección de pulso para estudiar los cambios de flanco
            if((finalValue > (-SCHMITT_THR * 20)) && (finalValue < (SCHMITT_THR * 20)))
            {
                finalValue = 0;
                SCOPE = down;
            }
            else
            {
                SCOPE = up;
            }

            // Trabajamos ahora las muestras
            // Medimos el pulso
            measurePulseWidth(finalValue);

            switch (stateRecording)
            {
              
              // Detección de tono guía
              case 0:

                if (_pulseWasMeasured)
                {                 
                    if(_lastMeasuredWidth >= 48 && _lastMeasuredWidth <= 60)
                    {
                        // He encontrado 2 flancos en el tiempo de control   
                        // Contamos los pulsos de LEAD
                        pulseCount++;
                        //_pulseWasMeasured = false;

                        //log("Pulse width: " + String(_lastMeasuredWidth));

                        //LAST_MESSAGE = "[F:" + String(SCHMITT_THR) + "%] Recognizing LEAD. [" + String(pulseCount) + "]";                        

                        if (pulseCount >= 1024)//maxPilotPulseCount)
                        {
                            //LAST_MESSAGE = "Waiting for SYNC 1.";

                            if (waitingHead)
                            {
                              writeTAPHeader();
                              waitingHead = false;
                            }

                            // Saltamos a la espera de SYNC
                            stateRecording = 1;
                            pulseCount = 0;
                        }
                    }
                  }
                  break;

              // Detección de SYNC
              case 1:                   
                // Detección de la SYNC 
                //-- S1: 190.6us + S2: 210us 
                //-- S1+S2: 19 pulsos a 48KHz

                if (_pulseWasMeasured)
                {
                    //_pulseWasMeasured = false;

                    if ((_lastMeasuredWidth >= 6) && (_lastMeasuredWidth <= 20))
                    {
                      stateRecording = 2;
                      LAST_MESSAGE = "Waiting for DATA.";
                    }
                    else
                    {
                      LAST_MESSAGE = "Waiting for SYNC.";
                    }
                }
                break;

              // Detección de bits
              case 2:
                  //
                  // Verifico si es 0, 1 o silencio de fin de bloque
                  //

                  if (_pulseWasMeasured)
                  {
                      _pulseWasMeasured = false;

                      // if (_measuredWidth == 0)
                      // {
                      //   // Error.
                      //   // Paramos la grabación.
                      //   STOP = true;
                      //   REC = false;
                      //   totalBlockTransfered = 0;                                  
                      //   stopRecordingProccess=true;
                      //   RECORDING_ERROR = 1;  

                      //   // Damos info en HMI
                      //   LAST_MESSAGE = "Error: Signal capture problem.";
                      //   SCOPE = down;

                      //   // Reiniciamos todo
                      //   //prepareNewBlock();   
                      //   //

                      //   //
                      //   // Finalizamos
                      //   return;                        
                      // }

                      if ((_measuredPulseUpWidth > 2) && (_measuredPulseUpWidth <= 16))
                      {
                          // Es un 0
                          bitString += "0";
                          bitChStr[bitCount] = '0';
                          // Contabilizamos un bit
                          bitCount++;
                      }
                      else if ((_measuredPulseUpWidth >= 17) && (_measuredPulseUpWidth <= 256))
                      {
                          // Es un 1
                          bitString += "1";
                          bitChStr[bitCount] = '1';
                          // Contabilizamos un bit
                          bitCount++;
                      } 

                      //
                      // Conversión de bits a bytes
                      //

                      if (bitCount > 7)
                      {
                          // Reiniciamos el contador de bits
                          bitCount=0;

                          // Convertimos a uint8_t el valor leido del DATA
                          uint8_t value = strtol(bitChStr, (char**) NULL, 2);
                          byteRead = value;
                          uint8_t valueToBeWritten = byteRead;
                          
                          // Escribimos en fichero el dato
                          _mFile.write(valueToBeWritten);

                          // Guardamos el checksum generado
                          lastChk = checksum;
                          // Calculamos el nuevo checksum
                          checksum = checksum ^ byteRead;
                          
                          // Mostramos información de la cabecera
                          proccesByteData();   

                          // Mostramos el progreso de grabación del bloque
                          if (!showDataDebug)
                          {
                            #ifndef DEBUGMODE
                              PROGRESS_BAR_BLOCK_VALUE = (byteCount*100) / (header.blockSize-1);                        
                            #endif
                          }

                          // Contabilizamos bytes
                          byteCount++;
                          LAST_MESSAGE = "Getting data. " + String(byteCount) + " / " + String(header.blockSize) + " bytes";

                          // Reiniciamos la cadena de bits
                          bitString = "";
                          byteRead = 0;

                          int blkSize = header.blockSize;
                          // Detectamos que el bloque ha acabado
                          // ahora comprobamos si es correcto o no.
                          if (byteCount == blkSize && byteCount != 0 && blkSize != 0)
                          {
                              if (checksum==0)
                              {
                                  
                                  BLOCK_REC_COMPLETED = true;

                                  // NOTA: El ultimo uint8_t es el checksum
                                  // si se hace XOR sobre este mismo da 0
                                  // e indica que todo es correcto
                                  LAST_MESSAGE = "Block captured ok!";

                                  // Informamos de los bytes salvados
                                  LAST_SIZE = blkSize; 
                                  // Informamos del bloque en curso
                                  BLOCK_SELECTED = blockCount;
                                  // Procesamos información del bloque
                                  proccessInfoBlock();
                                  // Incrementamos un bloque  
                                  blockCount++;   

                                  // --------------------------------------------------------
                                  //
                                  // TODO OK - Continuamos con mas bloques si no hay STOP
                                  // --------------------------------------------------------
                                  // Comenzamos otra vez
                                  // --------------------------------------------------------
                                  // Reiniciamos todo
                                  prepareNewBlock();
                                  // Guardamos los bloques transferidos
                                  totalBlockTransfered = blockCount;                 
                              }             
                              else
                              {
                                  // Paramos la grabación.
                                  STOP = true;
                                  REC = false;
                                  totalBlockTransfered = 0;                                  
                                  stopRecordingProccess=true;
                                  RECORDING_ERROR = 1;  

                                  // Damos info en HMI
                                  LAST_MESSAGE = "Error: Bytes=" + String(byteCount) + ".[" + String(bitCount) + "] / " + String(blkSize) + " / CHK=" + String(checksum);
                                  SCOPE = down;

                                  // Reiniciamos todo
                                  //prepareNewBlock();   
                                  //

                                  //
                                  // Finalizamos
                                  return;
                              }        
                          } 
                          else
                          {
                              BLOCK_REC_COMPLETED = false;
                          }                              
                      }    
                }  
                break;

              default:
                break;
            }
        
        
            // Pasamos al output directamente los valores de señal
            //R-OUT
            *ptrOut++ = finalValue;
            //L-OUT
            *ptrOut++ = finalValue;
            //                        
            resultOut+=2*chn; 
        }

        // Volcamos el output
        // Volcamos en el buffer

        _kit.write(bufferOut, resultOut); 
        
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
            //SerialHW.println("");
            //SerialHW.println("Fixed threshold from HMI");
            
            threshold_high = (SCHMITT_THR * 32767)/100;
            threshold_low = (-1)*(SCHMITT_THR * 32768)/100;            

            //SerialHW.println("TH+ = " + String(threshold_high));
            //SerialHW.println("TH- = " + String(threshold_low));

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
          size_t len = 0;
          len = _kit.read(bufferRec, BUFFER_SIZE_REC);            
          readBuffer(len);

          if (!stopRecordingProccess)
          {
            return false;
          }
          else
          {
            return true;
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
        strncpy(LAST_TYPE,"",1);
        strncpy(LAST_NAME,"",1);

        // if (SHOW_DATA_DEBUG)
        // {
        //   // Info en pantalla
        //   if (state==20)                         
        //   {
        //     dbgSync1="WAIT";
        //   }

        //   if (state==21)
        //   {
        //     dbgSync1="WAIT";
        //     dbgSync2="WAIT";
        //   }

        //   if(state==1)
        //   {
        //     dbgRep="WAIT";
        //   }
        // }

      }

      void initialize()
      {
          prepareHMI();

          pLead = 0;
          pSync = 0;
          state2 = 0;

          // Reservamos memoria
          fileName = (char*)ps_calloc(20,sizeof(char));
          fileName = (char*) "_record\0";

          recDir = (char*)ps_calloc(55,sizeof(char));
          String dirR = RECORDING_DIR + "/\0";
          recDir = strcpy(recDir, dirR.c_str());
          
          if (!_sdf32.mkdir(RECORDING_DIR))
          {
            log("Error! Directory exists or wasn't created");
          }

          // Se crea un nuevo fichero temporal con la ruta del REC
          strcat(recDir, fileName);
          //SerialHW.println("Dir for REC: " + String(recDir));

          // Inicializo bit string
          bitChStr = (char*)ps_calloc(8, sizeof(char));
          datablock = (uint8_t*)ps_calloc(1, sizeof(uint8_t));
          
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
          stateRecording = 0;

          _pulseWidth = 0;      
          stopRecordingProccess = false; 
          stateInfoBlock = 0;
          //
          headState = 0;
      }

      bool createTempTAPfile()
      {
        // Abrimos el fichero en el directorio /REC
        if (!_mFile.open(recDir, O_WRITE | O_CREAT | O_TRUNC)) 
        {
          // Si está abierto, compruebo que lo está
          if (!_mFile.isOpen())
          {
            // Pruebo otra vez. Pero con otros parámetros
            if (!_mFile.open(recDir, O_WRITE | O_CREAT)) 
            {
              // El fichero no fue creado. Entonces no está abierto
              //SerialHW.println("Error in file for REC.");
              wasFileNotCreated = true;
              stopRecordingProccess = true;
              return false;
            }
            else
            {
              // El fichero fue creado. Entonces está abierto
              wasFileNotCreated = false;
              return true;
            }             
          }
          else
          {
            // ESTE CASO ES RARO y NO DEBERÍA DARSE NUNCA
            // pero lo controlamos por si acaso.

            // El fichero fue creado. Entonces está abierto
            wasFileNotCreated = false;            
            return true;
          }  
        }
        else
        {
          // El fichero fue creado. Entonces está abierto
          wasFileNotCreated = false;
          return true;
        }        
      }

      void terminate(bool removeFile)
      {

        // Si REC no está activo, no podemos terminar
        if (REC)
        {
            if (!wasFileNotCreated)
            {
                // El fichero inicialmente fue creado con exito
                // en la SD, pero ...
                //
                // 
                if (_mFile.size() !=0 && BLOCK_REC_COMPLETED)
                {
                    LAST_MESSAGE += " size: " + String(_mFile.size());

                    if (!removeFile)
                    {
                      // Finalmente se graba el contenido  
                      if (_mFile.isOpen())
                      {
                        // Lo renombramos con el nombre del BASIC
                        renameFile();        

                        // LAST_MESSAGE="File rename!";
                        delay(125);                       
                      }          

                      // Lo cerramos
                      _mFile.close();
                      
                      // LAST_MESSAGE="File closed!";
                      // delay(1500);

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
                      LAST_MESSAGE="File removed!";
                      delay(1500);                     
                    }
                }
                else
                {
                  // Tiene 0 bytes. Meto algo y lo cierro
                  // es un error
                  // Escribimos 256 bytes
                  for(int i=0;i<256;i++)
                  {
                    _mFile.write(0x01);
                  }

                  _mFile.close();
                  delay(1000);
                  _mFile.remove();
                  LAST_MESSAGE="File filled and removed.";
                  delay(1500);                       

                }
            }
            else
            {
              // El fichero no fue creado
              //_mFile.close();
              //delay(125);
            }
        }

        wasRenamed = false;
        nameFileRead = false;

        // Ponemos a cero todos los indicadores
        _hmi.resetIndicators();     
      }

      TAPrecorder()
      {}
};
