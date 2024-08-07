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

      const int BUFFER_SIZE_REC = 256; //256 (09/07/2024)
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
      int errorDetected = 0;


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
      bool _semiPulseWasMeasured = false;
      int16_t lastValueRead = 0;


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
      int stateInfoBlock = 0;
      bool isHead = true;

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
          uint8_t sizeNextBlLSB;
          uint8_t sizeNextBlMSB;          
          int blockSize = 19;
          int nextBlockSize = 0;
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

      void showByteInLog()
      {}

      void proccesByteData()
      {

          // Flag para indicar DATA o HEAD
          if (byteCount == 0)
          {
            // if (byteRead == 255)
            // {
            //   isHead = true;
            // }
            // else
            // {
            //   isHead = false;
            // }
          }

          // Flag tipo de cabecera para el HEAD
          if (byteCount == 1)
          {
              // 0 - program
              // 1 - Number array
              // 2 - Char array
              // 3 - Code file              
              header.type = byteRead;
              proccesInfoBlockType();
          }

          // Si es una cabecera diferente a DATA, 
          // tomamos nombres y tamaño del DATA
          if (header.type < 128)
          {
              if (byteCount > 1 && byteCount < 12)
              {
                    // Almacenamos el nombre. Los caracteres raros los sustituimos por blancos
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
          }

          // Tamaño del siguiente bloque
          if (byteCount == 12)
          {
            header.sizeNextBlLSB = byteRead;
          }

          if (byteCount == 13)
          {
              header.sizeNextBlMSB = byteRead;
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
          switch (stateInfoBlock)
          {
          case 0:
            //
            // Bloque con cabecera
            //
            if (header.blockSize == 19)
            {
                // Entonces después vendrá un bloque DATA
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
            else
            {
              LAST_MESSAGE = "Error. No waited block type.";
              delay(3000);
            }
            break;
          
          case 1:
            //
            // Bloque DATA
            //
            if (header.blockSize > 0)
            {
              // Esperamos solo bloques con cabecera despues de este bloque DATA
              stateInfoBlock=0;

              if (header.blockSize == 6912)
              {
                strncpy(LAST_TYPE,"SCREEN.DATA",sizeof("SCREEN.DATA"));
              }
              else
              {
                if (header.type != 0)
                {
                    // Es un bloque de datos BYTE
                    strncpy(LAST_TYPE,"BYTE.DATA",sizeof("BYTE.DATA"));
                }
                else if (header.type == 0)
                {
                    // Es un bloque BASIC
                    strncpy(LAST_TYPE,"BASIC.DATA",sizeof("BASIC.DATA"));
                }
              }
            }          
            break;

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

      void getFileName(bool test)
      {
        if (!nameFileRead)
        {
          // Proporcionamos espacio en memoria para el
          // nuevo filename
          fileNameRename = (char*)ps_calloc(20,sizeof(char));

          if (test)
          {
            strcpy(fileNameRename,"TEST");
          }
          else
          {
              int i=0;

              for (int n=0;n<10;n++)
              {
                if (header.name[n] != ' ')
                {
                    fileNameRename[i] = header.name[n];
                    i++;
                }
                else
                {}
              }            

              // Esto lo hacemos por si no hay nombre
              // que no de fallos.
              fileNameRename[10] = '_';
              fileNameRename[11] = 'r';
              fileNameRename[12] = 'e';
              fileNameRename[13] = 'c';

          }

          //char* extFile = ".tap\0";
          char extFile[] = ".tap";
          strcat(fileNameRename,extFile);
          
          nameFileRead = true;                             
        }        
      }

      void measurePulseWidth(int16_t value)
      {
          // Establecemos el threshold de detección
          int delta = 0;

          // Buscamos cambios de flanco y medimos un pulso completo
          switch (_measureState)
          {
            case 0:
              // Estado inicial
              // establecemos un estado incial unicamente.
              if (value > delta)
              {
                // Cambia a UP
                _measureState = 1;
                _measuredWidth++;
              }
              else if (value < -delta)
              {
                // Cambia a LOW
                _measureState = 3;
                _measuredWidth++;
              }
              else
              {
                _measureState = 0;
              }
              break;

            case 1:
              // Estado UP
              // Ahora cambia a LOW - Entonces es un flanco
              if (value < -delta)
              {
                _measureState = 2;
                _measuredPulseUpWidth = _measuredWidth;
                _semiPulseWasMeasured = true;
                _pulseWasMeasured = false;
                _edgeDetected = true;
                // Ya cuento con esta muestra, pero aun no hemos acabado 
                // de medir el pulso completo.
                _measuredWidth++;
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
              if (value > delta)
              {
                _measureState = 1; // 1 - 20/07/2024
                // Devolvemos el tamaño del pulso
                _lastMeasuredWidth = _measuredWidth;
                // o del otro semi-pulso
                _measuredPulseDownWidth = _measuredWidth - _measuredPulseUpWidth;
                _pulseWasMeasured = true;
                _semiPulseWasMeasured = false;

                // Comenzamos otra vez
                _edgeDetected = true;
                // Ya cuento con esta muestra. Empezamos un nuevo pulso completo
                _measuredWidth=1;
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
              if (value > delta)
              {
                _measureState = 4;
                _measuredPulseDownWidth = _measuredWidth;
                _semiPulseWasMeasured = true;
                _pulseWasMeasured = false;
                _edgeDetected = true;              
              
                // Ya cuento con esta muestra, pero aun no hemos acabado 
                // de medir el pulso completo.
                _measuredWidth++;
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
              if (value < -delta)
              {
                _measureState = 3;  // 3 - 20/07/2024
                // devolmenos el tamaño del pulso
                _lastMeasuredWidth = _measuredWidth;
                // o del otro semi-pulso
                _measuredPulseUpWidth = _measuredWidth - _measuredPulseDownWidth;
                _pulseWasMeasured = true;
                _semiPulseWasMeasured = false;
                _edgeDetected = true;
                // Ya cuento con esta muestra. Empezamos un nuevo pulso completo
                _measuredWidth=1;                
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

      void resetMeasuredPulse()
      {
        // Reiniciamos estados
        _pulseWasMeasured = false;
        _measureState = 0;

        // Reiniciamos anchos
        _lastMeasuredWidth = 0;
        _lastMeasureSemiPulsedWidth = 0;
        _pulseWidth = 0;
        _measuredPulseDownWidth = 0;
        _measuredPulseUpWidth = 0;
      }

      void prepareNewBlock()
      {
        //stateRecording = 0;
        byteCount = 0;      
        bitCount = 0;        
        checksum = 0; 
        bitString = "";
        isSilence = false;

        // El nuevo bloque tiene que registrar su tamaño en el fichero .tap
        // depende si es HEAD or DATA
        if (isHead)
        {
          // Es una HEAD
          header.sizeLSB = 17;
          header.sizeMSB = 0;
          isHead = false;                      
        }
        else
        {
          // Es DATA
          header.sizeLSB = header.sizeNextBlLSB;
          header.sizeMSB = header.sizeNextBlMSB;
          isHead = true;                      
        }

        // Hacemos la conversión, calculamos el nuevo block size
        // y lo metemos en el fichero.

        int size2 = 0;
        int size = header.sizeLSB + (header.sizeMSB * 256);
        uint8_t MSB = 0;
        uint8_t LSB = 0;

        // Guardamos el valor del tamaño
        header.blockSize = size + 2;

        // Hay que añadir este valor sumando 2, en la cabecera
        // antes del siguiente bloque
        size2 = size + 2;
        LSB = size2;
        MSB = size2 >> 8;

        // // Antes del siguiente bloque metemos el size
        _mFile.write(LSB);
        _mFile.write(MSB);        

      }

      void writeTAPHeader()
      {
        // Escribimos los primeros bytes
        uint8_t firstByte = 19;
        uint8_t secondByte = 0;
        // _mFile.write(firstByte);
        // _mFile.write(secondByte);
      }

      void stopREC()
      {
        
        //
        // Aquí se entra cuando hay un error detectado en la grabación
        // si el error no es especifico, entonces es 0 (error genérico)
        //

        STOP = true;
        REC = false;
        totalBlockTransfered = 0;                                  
        stopRecordingProccess=true;
        RECORDING_ERROR = 1;  

        // Reset de medidas de pulso
        resetMeasuredPulse();
        // Reset estado del control de pulso
        stateRecording = 0;
        isSilence = false;
        //
        waitingNextBlock = false;
        //
        WasfirstStepInTheRecordingProccess = false;
        //waitingHead = true;

        switch (errorDetected)
        {
        case 1:
          LAST_MESSAGE = "Sync not detected. Fine volume source or filter.";
          break;
        
        case 2:
          LAST_MESSAGE = "Checksum error in byte [" + String(byteCount) + "] in bit [" + String(bitCount) + "] - CHK=" + String(checksum);
          break;

        case 3:
          if (byteCount > header.blockSize)
          {
            LAST_MESSAGE = "Error. Headerless block not supported";
          }
          else
          {
            LAST_MESSAGE = "Error, " + String(header.blockSize - byteCount) + " bytes lost - " + String(byteCount) + " / " + String(header.blockSize);
          }
          break;
        default:
          LAST_MESSAGE = "Error.";
          break;
        }
        //byteCount = 0;
        // Bajamos el scope
        SCOPE = down;      
        delay(2000);
      }

      bool checkDataBlock()
      {
          //header.blockSize = byteCount;
          // Una vez se ha capturado un bloque entero (hemos llegado a un silencio) comprobamos.
          //
          // 1. Checksum = 0 : Todo ok y coincide con el checksum entregado por la fuente de sonido
          //    NOTA: El ultimo uint8_t es el checksum si se hace XOR sobre este mismo da 0 e indica que todo es correcto          
          //
          // 2. Se han capturado bytes
          // 3. El total de bytes coincide con el esperado.
          //

          if (checksum==0 && byteCount !=0 && header.blockSize == byteCount)
          {
              // Indicamos que se ha capturado un bloque completo
              BLOCK_REC_COMPLETED = true;
              LAST_MESSAGE = "Block saved";
              waitingNextBlock = false;

              // Informamos del bloque en curso
              BLOCK_SELECTED = blockCount;
              
              // Procesamos información del bloque
              LAST_SIZE = byteCount;

              //
              showProgramName();

              // Obtenemos el nombre que se le pondrá al .TAP
              getFileName(false);                
              errorInDataRecording = false;

              // Incrementamos un bloque  
              blockCount++;   
              
              // --------------------------------------------------------
              //
              // TODO OK - Continuamos con mas bloques si no hay STOP
              // --------------------------------------------------------
              // Comenzamos otra vez
              // --------------------------------------------------------
              // Reiniciamos todo
              //prepareNewBlock();
              // Guardamos los bloques transferidos
              totalBlockTransfered = blockCount;  
              //
              return true;               
          }             
          else
          {
              if (checksum != 0)
              {
                // Error de checksum
                errorDetected = 2;
              }
              else
              {
                // Error de bloque incompleto
                errorDetected = 3;
              }

              // Paramos la grabación.
              stopREC();
              
              // Volvemos
              return false;
          }                
      }

      void countNewByte()
      {
          BLOCK_REC_COMPLETED = false;

          //
          // Conversión de bits a bytes
          //

          //
          // Ahora controlamos el byte leido y se hemos cumplido el bloque
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
              
              // Analizamos cada byte leido para coger información en tiempo real
              proccesByteData();   

              // Mostramos el progreso de grabación del bloque
              PROGRESS_BAR_BLOCK_VALUE = (byteCount*100) / (header.blockSize-1);                        

              // Contabilizamos bytes
              byteCount++;

              // Actualizamos indicador de BYTES capturados
              LAST_MESSAGE = "Getting data. " + String(byteCount) + " / " + String(header.blockSize) + " bytes";

              // Reiniciamos la cadena de bits
              bitString = "";
              byteRead = 0;  
          }            
      }

      int16_t applyStereoFilter(int16_t oneValue,int16_t oneValue2)
      {
            int16_t finalValue = 0;

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
            if((finalValue > (-SCHMITT_THR * 20)) && 
               (finalValue < (SCHMITT_THR * 20)))
            {
                // Filtramos
                finalValue = 0;
                SCOPE = down;
            }
            else
            {
                // Dejamos pasar
                SCOPE = up;
            }       

            return finalValue; 
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
  
        double currentTime = 0;
        // Samples de referencia para control 
        int samplesRefForCtrl = 0;

        if (!WasfirstStepInTheRecordingProccess)
        {    
          // Comenzamos con una cabecera PROGRAM.
          isHead = true;
          // Pulsos a reset
          resetMeasuredPulse();
          BLOCK_REC_COMPLETED = false;
          //
          LAST_MESSAGE = "Recorder ready. Play source data.";
          // Ya no pasamos por aquí hasta parar el recorder
          WasfirstStepInTheRecordingProccess=true;    
        }

        // Analizamos todas las muestras del buffer de entrada
        for (int j=0;j<(len/4);j++)
        {  
            // Leemos los samples del buffer
            // canal R
            oneValue = *value_ptr++;
            // canal L
            oneValue2 = *value_ptr++;              

            // Aplicamos filtrado para eliminar ruido            
            finalValue = applyStereoFilter(oneValue, oneValue2);

            // Trabajamos ahora las muestras
            // Medimos pulsos
            measurePulseWidth(finalValue);

            // Si la medida de pulso ha acabado, analizamos
            if (_pulseWasMeasured)
            {
              _pulseWasMeasured = false;

              switch (stateRecording)
              {
                
                // Detección de tono guía
                case 0:
                  if(_lastMeasuredWidth >= 48 && _lastMeasuredWidth <= 60)
                  {
                      // He encontrado 2 flancos en el tiempo de control   
                      // Contamos los pulsos de LEAD
                      pulseCount++;
                      //resetMeasuredPulse();

                      if (pulseCount >= 256)//maxPilotPulseCount)
                      {

                          // Saltamos a la espera de SYNC
                          stateRecording = 1;
                          pulseCount = 0;
                          LAST_MESSAGE = "Waiting for SYNC";
                      }
                  }
                  break;

                // Detección de SYNC
                case 1:                   
                  // Detección de la SYNC 
                  //-- S1: 190.6us + S2: 210us 
                  //-- S1+S2: 19 pulsos a 48KHz

                  // Medimos una SYNC1 + SYNC2. Buscamos dos pulsos muy cercanos (el valor optimo es 20)
                  if (_lastMeasuredWidth >= 8 && _lastMeasuredWidth < 25)
                  {
                    // Estamos midiendo el pulso completo
                    stateRecording = 2;
                    prepareNewBlock();
                    // El primer byte de todo el .TAP es este, pero ya no se
                    // dará mas hasta un nuevo recording.
                  }

                  if ((_measuredPulseUpWidth >= 512) || 
                  (_measuredPulseDownWidth >= 512))  // Medida del silencio
                  {   
                      // Error. Hemos encontrado el silencio pero no la SYNC                    
                      stateRecording = 0;
                      errorDetected = 1;
                      stopREC();
                      return;
                  }

                  break;

                // Detección de bits
                case 2:
                  //
                  // Verifico si es 0, 1 o silencio de fin de bloque.
                  // Los bits 0 / 1 deben ser un pulso completo en otro caso estaría mal 
                  // la composición de la señal.
                  //
                  
                  if (_lastMeasuredWidth >= 10 && _lastMeasuredWidth < 25)
                  {
                      // Estamos midiendo el pulso completo
                      // Es un 0
                      bitString += "0";
                      bitChStr[bitCount] = '0';
                      // Contabilizamos un bit
                      bitCount++;
                      countNewByte();
                  }
                  else if (_lastMeasuredWidth >= 25 && _lastMeasuredWidth < 60)
                  {
                      // Estamos midiendo el pulso completo
                      // Es un 1
                      bitString += "1";
                      bitChStr[bitCount] = '1';
                      // Contabilizamos un bit
                      bitCount++;
                      countNewByte();
                  }
                  else if ((_measuredPulseUpWidth >= 512) || 
                  (_measuredPulseDownWidth >= 512))  // Medida del silencio
                  {   
                      // Hemos encontrado el silencio, finalizamos el bloque y chequeamos checksum                    
                      stateRecording = 0;

                      if (!checkDataBlock())
                      {
                        // Si hay fallo salimos.
                        return;
                      }
                  }
                  else
                  {
                    // Error. Pulsos inesperados
                    LAST_MESSAGE = "Bad pulse width " + String(_lastMeasuredWidth);
                  }
                  break;

                default:
                  break;
              }

            }

            
            // Pasamos al output directamente los valores de señal
            // para que se oiga por la salida de linea (line-out)

            //R-OUT
            *ptrOut++ = finalValue * (MAIN_VOL_R / 100);;
            //L-OUT
            *ptrOut++ = finalValue * (MAIN_VOL_L / 100);;
            //                        
            resultOut+=2*chn; 
        }

        // Volcamos el output en el buffer de salida
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
            if (errorDetected != 0)
            {
              // Hubo errores
              errorDetected = 0;
              REC = true;
              terminate(true);
              REC = false;
            }
            
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

          resetMeasuredPulse();

          stopRecordingProccess = false; 
          stateInfoBlock = 0;
          //
          waitingNextBlock = false;
          isSilence = false;
          //
          //WasfirstStepInTheRecordingProccess = false;
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

        FIRSTBLOCKREAD = false;
        bool fileWasClosed = false;

        // Si REC no está activo, no podemos terminar
        if (REC)
        {
            if (!wasFileNotCreated)
            {
                // El fichero inicialmente fue creado con exito
                // en la SD, pero ...
                //
                //
                // LAST_MESSAGE="Preparando fichero.";
                // delay(3000);

                if (_mFile.size() !=0 && BLOCK_REC_COMPLETED)
                {

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
                      fileWasClosed = true;
 
                      delay(125);

                      LAST_MESSAGE = "File saved.";
                      LAST_MESSAGE += " Size: " + String(_mFile.size()/1024) + " KB";
                    }
                    else
                    {
                      // SE ELIMINA
                      // No se renombra porque es erroneo
                      _mFile.close();
                      fileWasClosed = true;                      
                      delay(125);
                      // Se elimina
                      _mFile.remove();
                      // LAST_MESSAGE="File removed!";
                      // delay(1500);                     
                      LAST_MESSAGE = "File not saved.";
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
                  fileWasClosed = true;
                  delay(250);
                  _mFile.remove();
                  delay(250);
                  
                  // LAST_MESSAGE="File filled and removed.";
                  // delay(1500);                  
                  LAST_MESSAGE = "File not saved.";     
                }
            }
            else
            {
              // El fichero no fue creado
              _mFile.close();
              fileWasClosed = true;
              LAST_MESSAGE = "File not saved.";
            }
        }

        wasRenamed = false;
        nameFileRead = false;
        WasfirstStepInTheRecordingProccess = false;
        
        // Ponemos a cero todos los indicadores
        _hmi.resetIndicators();   
      }

      TAPrecorder()
      {}
};
