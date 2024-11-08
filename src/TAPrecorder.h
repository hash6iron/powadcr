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
      bool fileWasNotCreated = false;
      bool stopRecordingProccess = false;
      //int errorsCountInRec = 0;

      bool wasRenamed = false;
      bool nameFileRead = false;
      bool notAudioInNotified = false;
      //bool wasSelectedThreshold = false;
      int totalBlockTransfered = 0;
      bool WasfirstStepInTheRecordingProccess = false;
      bool actuateAutoRECStop = false;
   
      
      
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
      //double tState = 1/3500000;
      // int pilotPulse = 2168; // t-states
      // int sync1 = 667;
      // int sync2 = 735;
      // int bit0 = 855;
      // int bit1 = 1710;

      // int pSync = 0;
      // int pLead = 0;
      // int state2 = 0;

      // Tiempos de señal
      // Tono guia
      // const int timeForLead = 1714; //1714;   // Lapara buscar
      // const int timeLeadSeparation = 585; 
      // Con samples
      // Señal de sincronismo
      // const int timeForSync = 400; //520
      // const int timeSyncSeparation = 190;
      // Umbrales de pulso.
      // const int high = 32767;
      // const int low = -32767;
      // const int silence = 0;

      // // Definimos el sampling rate real de la placa.
      // //const double samplingRate = 44099.988;
      // const double samplingRate = 48000.0;
      // //const double samplingRate = 22050.0;
      // // Calculamos el T sampling_rate en us
      // const double TsamplingRate = (1 / samplingRate) * 1000 * 1000;

      // Controlamos el estado del TAP
      int stateRecording = 0;
      int statusSchmitt = 0;
      // Contamos los pulsos del tono piloto
      int pulseCount = 0;
      int lostPulses = 0;
      // Contamos los pulsos del sync1
      long silenceCount = 0;
      long lastSilenceCount = 0;
      long silenceSample = 0;

      char* bitChStr;
      uint8_t* datablock;
      //uint8_t* fileData;
      int bitCount = 0;
      int byteCount = 0;
      int blockCount = 0;
      int ptrOffset = 0;
      //int badPulseW = 0;
      bool recWithoutHead = false;
      String bitString = "";

      uint8_t checksum = 0;
      uint8_t byteRead = 0;
      uint8_t lastByteRead = 0;


      // Umbral de histeresis para el filtro Schmitt al 10% de Vmax
      const int defaultThH = 3000;
      const int defaultThL = -3000;
      int threshold_high = defaultThH; 
      int threshold_low = defaultThL; 
      int errorDetected = 0;
     
     
      // Zero crossing routine
      int _measuredPulseUpWidth = 0;
      int _measuredPulseDownWidth = 0;
      bool _pulseUpWasMeasured = false;
      bool _pulseDownWasMeasured = false;

      int _measureState = 0;
      int _measureSilence = 0;
      bool _pulseInverted = false;
      bool _silenceTimeOverflow = false;
      bool _silenceDetected = false;
      //

      // Info block
      int stateInfoBlock = 0;
      bool isHead = true;

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

          // Flag para indicar DATA o HEAD
          if (byteCount == 0)
          {
              // Si esperabamos una cabecera PROGRAM ...
              if (isHead==false && header.sizeLSB == 17)
              {
                  // .. y vemos que el byte 1 es 0xFF
                  if (byteRead == 255)
                  {
                      // No hay cabecera. Bloque DATA sin cabecera program.
                      recWithoutHead = true;
                      return;
                  }
                  else
                  {
                      // Cabecera
                      recWithoutHead = false;              
                  }
              }
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

          if (sizeof(header.name) >= 10)
          {
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
                PROGRAM_NAME_2 = "";
            }
          }
          else
          {
              PROGRAM_NAME = "noname";
              PROGRAM_NAME_2 = "noname";
          }

          PROGRAM_NAME_ESTABLISHED = true;
      }

      void proccesInfoBlockType()
      {          

          if (recWithoutHead)
          {
            strncpy(LAST_TYPE,"BYTE.H-LESS",sizeof("BYTE.H-LESS"));
            return;
          }

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
          //Generamos un numero aleatorio para el final del nombre
          srand(time(0));
          delay(125);
          int rn = rand()%999999;
          //Le unimos la extensión .TAP
          String txtRn = "-" + String(rn) + ".tap";
          char const *extPath = txtRn.c_str();
          strcat(fileNameRename,extPath);
          //y unimos el fichero al path
          strcat(cPath,fileNameRename);

          #ifdef DEBUGMODE
            logln("");
            logln("Filename for rename: ");
            log(cPath);
            logln("");  
          #endif

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
          //strcpy(fileNameRename,"noname");

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
          nameFileRead = true;                             
        }        
      }

      void measurePulseWidth(int16_t finalValue)
      {
          // ------------------------------------------------------------------------------------------------------------------
          // Measure Pulse Width
          // ------------------------------------------------------------------------------------------------------------------

          // Establecemos el threshold de detección
          int deltaH = threshold_high;
          int deltaL = threshold_low;

          if (_pulseInverted)
          {
              if (finalValue == 32767)
              {
                  finalValue = -32768;
              }
              else
              {
                  finalValue = 32767;
              }
          }

          // Buscamos cambios de flanco y medimos un pulso completo
          switch (_measureState)
          {
            case 0:
              // Estado inicial
              // establecemos un estado incial unicamente. Estudiamos como empieza la señal
              if (finalValue == 32767)
              {
                // Empezamos con el pulso directo
                _pulseInverted = false;
                _measureState = 1;
                _measuredPulseDownWidth=0;
                _measuredPulseUpWidth=1;
              }
              else if (finalValue == -32768)
              {
                // Empezamos con el pulso invertido
                _pulseInverted = true;
                _measureState = 1;
                _measuredPulseUpWidth=1;
                _measuredPulseDownWidth=0;
              }
              else
              {
                // Aun no se ha detectado ningún pulso en el filtro Schmitt, entonces
                // no sabemos en este caso tampoco que es.
                _measureState = 0;                           
                _measuredPulseUpWidth=0;
                _measuredPulseDownWidth=0;
                _pulseInverted=false;
              }
              break;

            case 1:
              // Estado UP
              // Ahora cambia a LOW - Entonces es un flanco
              if (finalValue == -32768)
              {
                // Cambio de estado
                _measureState = 2;
                _pulseUpWasMeasured = true;
                _measuredPulseDownWidth++;
              }
              else if (finalValue == 32767)
              {
                // Calculo el ancho del semi-pulso alto
                _measuredPulseUpWidth++;
                _pulseUpWasMeasured = false;
                _measuredPulseDownWidth=0;
                _measureState = 1;
              }
              else
              {
                _measureState = 1;
              }                  
              break;

            case 2:
              // Estado LOW
              if (finalValue == 32767)
              {
                _measureState = 1; // 1 - 20/07/2024
                _pulseDownWasMeasured = true;
                _measuredPulseUpWidth++;
              }
              else if (finalValue == -32768)
              {
                _measuredPulseDownWidth++;
                _pulseDownWasMeasured = false;
                _measuredPulseUpWidth=0;
              }
              else
              {
                _measureState = 2;
              }                          
              break;
          }

          // Un pulso muy largo sin cambio entonces estamos ante un silencio
          if (_measuredPulseDownWidth < 512 && _measuredPulseUpWidth < 512)
          {
              _silenceDetected = false; 
              _silenceTimeOverflow = false;           
              _measureSilence = 0;
          }
          else
          {
              _silenceDetected = true;

              if (_measuredPulseDownWidth >= 512)
              {
                  _measureSilence = _measuredPulseDownWidth;
              }
              else
              {
                  _measureSilence = _measuredPulseUpWidth;
              }            

              // Evitamos desbordamiento
              if (_measureSilence > 2646000)
              {
                  _silenceTimeOverflow = true;
                  actuateAutoRECStop = true;
                  //Paramos la animación del indicador de recording
                  hmi.writeString("tape2.tmAnimation.en=0");    
                  //Paramos la animación del indicador de recording
                  hmi.writeString("tape.tmAnimation.en=0");          
              }
          }


          // ------------------------------------------------------------------------------------------------------------------
      }

      void resetMeasuredPulse()
      {
        // Reiniciamos estados
        _measureState = 0;
        // Reiniciamos anchos
        _measuredPulseDownWidth = 0;
        _measuredPulseUpWidth = 0;
        _measureSilence = 0;  //27102024
      }

      void prepareNewBlock()
      {
        //stateRecording = 0;
        byteCount = 0;      
        bitCount = 0;        
        checksum = 0; 
        bitString = "";

        PROGRAM_NAME_ESTABLISHED = false;

        //guardamos la posición del puntero del fichero en este momento
        //que es justo al final del ultimo bloque + 1 (inicio del siguiente)
        ptrOffset = _mFile.position();

        // El nuevo bloque tiene que registrar su tamaño en el fichero .tap
        // depende si es HEAD or DATA
        if (isHead)
        {
          // Es una HEAD
          header.sizeLSB = 17;
          header.sizeMSB = 0;

          // El siguiente ya no es cabecera PROGRAM
          // será cabecera BYTE (por eso isHead = false)
          if (recWithoutHead)
          {
              isHead = true;                      
          }
          else
          {
              isHead = false;                      
          }
        }
        else
        { 
          // Este ahora es DATA
          header.sizeLSB = header.sizeNextBlLSB;
          header.sizeMSB = header.sizeNextBlMSB;
          // El siguiente será con cabecera PROGRAM
          // por eso isHead = true
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
          //
          //showProgramName(); 

          bool res = false;

          if (!recWithoutHead)
          {
              if (byteCount !=0 && header.blockSize == byteCount && checksum==0)
              {
                  // Indicamos que se ha capturado un bloque completo
                  BLOCK_REC_COMPLETED = true;
                  LAST_MESSAGE = "Block saved";
                  //waitingNextBlock = false;

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
                  //Inicializamops la cadena de bits
                  strcpy(bitChStr,"");
                  
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
                  res = true;               
              }             
              else if (byteCount == 0)
              {
                  // No se han capturado datos.
                  // Paramos la grabación.
                  stopREC(3);
                  
                  // Volvemos
                  res = false;
              }   
              else if (checksum != 0)
              {
                  // Error de checksum
                  // Paramos la grabación.
                  stopREC(2);
                  
                  // Volvemos
                  res = false;            
              }             
              else if (byteCount < header.blockSize)
              {
                  // Error de bytes perdidos
                  // Paramos la grabación.
                  stopREC(5);
                  
                  // Volvemos
                  res = false;
              }
              else if (byteCount > header.blockSize)
              {
                  // Error de bytes excedidos
                  // Paramos la grabación.
                  stopREC(6);
                  
                  // Volvemos
                  res = false;
              }            
          }
          else
          {
              if (checksum == 0)
              {
                  // el bloque es OK
                  // Indicamos que se ha capturado un bloque completo
                  BLOCK_REC_COMPLETED = true;
                  //LAST_MESSAGE = "Block saved";
                  //waitingNextBlock = false;

                  // Informamos del bloque en curso
                  BLOCK_SELECTED = blockCount;
                  
                  // Procesamos información del bloque
                  LAST_SIZE = byteCount;

                  //
                  // Ahora añadimos el tamaño del bloque sin cabecera
                  // Hacemos la conversión, calculamos el nuevo block size
                  // y lo metemos en el fichero.

                  int size = LAST_SIZE;
                  uint8_t MSB = 0;
                  uint8_t LSB = 0;

                  LSB = size;
                  MSB = size >> 8;

                  // metemos el size
                  int currentPtrOffset = _mFile.position();
                                
                  _mFile.seek(ptrOffset);
                  // Añadimos el tamaño del bloque capturado al principio
                  // de la cabecera.
                  _mFile.write(LSB);
                  _mFile.write(MSB); 
                  _mFile.seek(currentPtrOffset);
                  //
                  showProgramName();

                  // Obtenemos el nombre que se le pondrá al .TAP
                  getFileName(false);                
                  errorInDataRecording = false;

                  // Incrementamos un bloque  
                  blockCount++;   
                  //Inicializamops la cadena de bits
                  strcpy(bitChStr,"");
                  
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
                  res = true;                      
              }
              else
              {
                // Error de checksum
                // Paramos la grabación.
                stopREC(2);
                
                // Volvemos
                res = false;                 
              }
          }

          return res;
      }

      void countNewByte()
      {
          // --------------------------------------------------------------------------------------------------------
          // Count NEW BYTE
          // --------------------------------------------------------------------------------------------------------
              BLOCK_REC_COMPLETED = false;

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
                  //lastChk = checksum;
                  // Calculamos el nuevo checksum
                  checksum = checksum ^ byteRead;
                  // Nos quedamos siempre con el ultimo para poder
                  // comparar con el checksum en los bloques sin cabecera.
                  lastByteRead = byteRead;
                  
                  // Mostramos el progreso de grabación del bloque
                  PROGRESS_BAR_BLOCK_VALUE = (byteCount*100) / (header.blockSize-1);    

                  // Analizamos cada byte leido para coger información en tiempo real
                  // y tomar decisiones.
                  proccesByteData();                       

                  // Incrementamos los bytes leidos.
                  byteCount++;

                  if (!recWithoutHead)
                  {
                      if (byteCount > header.blockSize)
                      {
                          // Por alguna razón se están leyendo mas bytes de los necesarios.
                          // quizás porque no se ha detectado el silencio
                          errorDetection(6);
                          // Reiniciamos la cadena de bits
                          bitString = "";
                          byteRead = 0; 
                          return;
                      }
                      else
                      {
                          //Actualizamos indicador de BYTES capturados
                          LAST_MESSAGE = "Data: " + String(byteCount) + " / " + String(header.blockSize) + " bytes";
                      }
                  }
                  else
                  {
                      //Actualizamos indicador de BYTES capturados
                      LAST_MESSAGE = "Data: " + String(byteCount) + " bytes - CHK: " + String(checksum);
                  }
                  // Reiniciamos la cadena de bits
                  bitString = "";
                  byteRead = 0;  
              }          

          // ---------------------------------------------------------------------------------------------------------       
      }

      int16_t applySchmittFilter(int16_t oneValue,int16_t oneValue2)
      {
            int16_t finalValue = 0;
            int16_t outFinalValue = 0;

            selectThreshold();

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

            switch (statusSchmitt)
            {
              case 0:
                if (finalValue > threshold_high)
                {
                  statusSchmitt = 1;
                  outFinalValue = 32767;
                }
                else if (finalValue < threshold_low)
                {
                  statusSchmitt = 2;
                  outFinalValue = -32768;
                }
                else
                {
                  statusSchmitt = 0;
                  outFinalValue = 0;
                }
                break;

              case 1:
                if (finalValue > threshold_high)
                {
                  statusSchmitt = 1;
                  outFinalValue = 32767;
                }
                else if (finalValue < threshold_low)
                {
                  statusSchmitt = 2;
                  outFinalValue = -32768;
                }
                else
                {
                  statusSchmitt = 1;
                  outFinalValue = 32767;
                }
                break;

              case 2:
                if (finalValue > threshold_high)
                {
                  statusSchmitt = 1;
                  outFinalValue = 32767;
                }
                else if (finalValue < threshold_low)
                {
                  statusSchmitt = 2;
                  outFinalValue = -32768;
                }
                else
                {
                  statusSchmitt = 2;
                  outFinalValue = -32768;
                }
                break;

              default:
              break;
            }

            return outFinalValue; 
      }

      void initializePulse()
      {
        _measuredPulseUpWidth = 0;
        _measuredPulseDownWidth = 0;
        //_measuredWidth = 0;
        //_edgeDetected = false;                                      
      }

      void errorDetection(int error)
      {                             
        errorDetected = error;
        stopREC(error);        
        initializePulse(); 
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
        
        // Anchos de los semi-pulsos para 44.1KHz
        // Tono guia
        const int16_t wToneMin = 22;
        const int16_t wToneMax = 40;
        // Silencio
        const int16_t wSilence = 512;
        // SYNC 1
        const int16_t wSync = 15;  //min value_default = 20
        // Bit 0
        const int16_t wBit0_1 = 2;  //min
        const int16_t wBit0_2 = 15;  //max
        // Bit 1
        const int16_t wBit1_1 = 15;    //17  02/11/2024
        const int16_t wBit1_2 = 35;    //30  02/11/2024
        
        //Maximo numero de pulsos a leer antes de esperar una SYNC
        const int maxPilotPulseCount = 256; 


        // Para modo debug.
        // Esto es para modo depuración. 
        //
        //bool showDataDebug = SHOW_DATA_DEBUG;
        
        // Ajuste del detector de señal
        // Muestras del tono guia leidas
  
        //double currentTime = 0;
        // Samples de referencia para control 
        //int samplesRefForCtrl = 0;

        if (!WasfirstStepInTheRecordingProccess)
        {    
          // Comenzamos con una cabecera PROGRAM.
          isHead = true; //*
          //resetMeasuredPulse();
          //badPulseW = 0; //*
          statusSchmitt = 0;
          _pulseInverted = false;
          _measureState = 0;
          BLOCK_REC_COMPLETED = false; //*
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
            finalValue = applySchmittFilter(oneValue, oneValue2);
            //finalValue = oneValue;

            // Trabajamos ahora las muestras
            // Medimos pulsos
            measurePulseWidth(finalValue);

            // Si la medida de pulso ha acabado, analizamos
            if (_silenceDetected)
            {
              
              if (_measureSilence > 512)
              {
                  if (!_silenceTimeOverflow)
                  {
                    LAST_MESSAGE = "Silence: " + String(_measureSilence/44100) + "s";
                  }
                  else
                  {
                    LAST_MESSAGE = "Silence: > 1m";
                  }

                  // Ahora vemos en que stateRecording estamos
                  switch (stateRecording)
                  {
                    case 0:
                      // Guide tone
                      stateRecording = 0;
                      pulseCount = 0;
                      break;

                    case 1:
                      // SYNC detection
                      // Error. Hemos encontrado el silencio pero no la SYNC                    
                      stateRecording = 0;
                      errorDetection(1);
                      return;                
                      break;

                    case 2:
                      // Capture DATA
                      // Hemos encontrado el silencio, finalizamos el bloque y chequeamos checksum                    
                      stateRecording = 0;
                      LAST_MESSAGE = "Data = " + String(byteCount) + " / " + String(header.blockSize) + " bytes";
                      
                      if (!checkDataBlock())
                      {
                        // Si hay fallo salimos.
                        //return;
                      }
                    default:
                      break;
                  }

              }
            }
            else
            {
                // statusPulse dependerá de si se ha medido un pulso alto, porque es lo unico que comparamos.
                // los pulsos bajos los obviamos.
                bool statusPulse = _pulseUpWasMeasured;

                // No es un silencio
                if (stateRecording < 2)
                {
                    if (statusPulse)
                    {
                        _pulseUpWasMeasured = false;
                        _pulseDownWasMeasured = false;                        

                        //LAST_MESSAGE = "Up: " + String(_measuredPulseUpWidth) + " - Down: " + String(_measuredPulseDownWidth);

                        switch (stateRecording)
                        {
                          
                          // Detección de tono guía
                          case 0:
                            //LAST_MESSAGE = "Up: " + String(_measuredPulseUpWidth) + " - Down: " + String(_measuredPulseDownWidth);
                            if ((_measuredPulseUpWidth >= wToneMin) && (_measuredPulseUpWidth < wToneMax))
                            {                              

                                  // Contamos los pulsos de LEAD
                                pulseCount++;
        
                                if (pulseCount >= maxPilotPulseCount)//maxPilotPulseCount)
                                {
                                    // Saltamos a la espera de SYNC
                                    stateRecording = 1;
                                    pulseCount = 0;
                                    LAST_MESSAGE = "Waiting for SYNC";
                                    //initializePulse();                       
                                }
                                else
                                {
                                  stateRecording = 0;
                                }
                            }
                            break;

                          // Detección de SYNC
                          case 1:                   
                              // Detección de la SYNC 1
                              //-- S1: 190.6us 

                              // Medimos una SYNC1. Buscamos dos pulsos muy cercanos
                              //
                              if (_measuredPulseUpWidth < wSync)
                              {
                                // Esperamos ahora SYNC 2 en DOWN
                                stateRecording = 2;                              

                                //Esperamos un bloque ahora PROGRAM o BYTE, entonces
                                //nos preparamos
                                prepareNewBlock();
                                //badPulseW = 0;                                                           
                                
                              }
                              else
                              {
                                // Reiniciamos los contadores mientras no sea                               
                                stateRecording = 1;
                              }
                              break;

                          default:
                            break;
                        }                    
                    }                  
                }
                else if (stateRecording == 2)
                {
                    if (statusPulse)
                    {
                        _pulseUpWasMeasured = false;
                        _pulseDownWasMeasured = false;

                        if (((_measuredPulseUpWidth ) >= wBit0_1) &&
                            ((_measuredPulseUpWidth ) <= wBit0_2))
                        {
                            // Es un 0

                            // Vuelta al ciclo de detección del primer semui-pulso de data
                            stateRecording = 2;

                            bitString += "0";
                            bitChStr[bitCount] = '0';
                            // Contabilizamos un bit
                            bitCount++;
                            countNewByte();

                            //badPulseW = 0;
                        }
                        else if (((_measuredPulseUpWidth ) >= wBit1_1) &&
                                ((_measuredPulseUpWidth ) <= wBit1_2))
                        {
                            // Es un 1
                            
                            // Vuelta al ciclo de detección del primer semui-pulso de data
                            stateRecording = 2;    

                            bitString += "1";
                            bitChStr[bitCount] = '1';
                            // Contabilizamos un bit
                            bitCount++;
                            countNewByte();

                            //badPulseW = 0;
                        }
                        else
                        {
                          //Bad pulse
                          errorDetection(4);
                        }                            
                    }
                }
            }
                        
            // Pasamos al output directamente los valores de señal
            // para que se oiga por la salida de linea (line-out)

            if (REC_AUDIO_LOOP)
            {
              //R-OUT
              *ptrOut++ = finalValue * (MAIN_VOL_R / 100);
              //L-OUT
              *ptrOut++ = finalValue * (MAIN_VOL_L / 100);
                                     
              resultOut+=2*chn; 
            }
        }

        // Volcamos el output en el buffer de salida
        _kit.write(bufferOut, resultOut);         
    }

    public:

void stopREC(int error)
      {
        
        //
        // Aquí se entra cuando hay un error detectado en la grabación
        // si el error no es especifico, entonces es 0 (error genérico)
        //
        errorDetected = error;

        // Eliminado el 02/11/2024
        STOP = true;
        REC = false;

        totalBlockTransfered = 0;                                  
        stopRecordingProccess=true;
        //RECORDING_ERROR = 1;  

        // Reset de medidas de pulso
        resetMeasuredPulse();
        // Reset estado del control de pulso
        stateRecording = 0;
        //isSilence = false;
        //
        //waitingNextBlock = false;
        //
        WasfirstStepInTheRecordingProccess = false;
        
        //waitingHead = true;

        switch (error)
        {
            case 0:
              // No error
              terminate(false);
              LAST_MESSAGE = "Auto REC stop.";
            case 1:
              LAST_MESSAGE = "Sync not detected. Fine volume source or filter.";
              break;
            
            case 2:
              LAST_MESSAGE = "Checksum error in byte [" + String(byteCount) + "] in bit [" + String(bitCount) + "] - CHK=" + String(checksum);
              break;

            case 3:
              LAST_MESSAGE = "Error. Any byte was captured.";
              break;

            case 4:
              LAST_MESSAGE = "Wrong pulse. " + String(_measuredPulseUpWidth) + " byte: " + String(byteCount+1);
              //LAST_MESSAGE = "Error. Wrong pulses detected.";
              break;

            case 5:
              LAST_MESSAGE = "Error, " + String(header.blockSize - byteCount) + " bytes lost - " + String(byteCount) + " / " + String(header.blockSize);
              break;

            case 6:
              //LAST_MESSAGE = "Error. Headerless block not supported";
              LAST_MESSAGE = "Error. Bytes was exceeded.";
              break;

            default:
              LAST_MESSAGE = "Error not defined.";
              break;
        }
        //byteCount = 0;
        // Bajamos el scope
        //SCOPE = down;      
        //Paramos la animación del indicador de recording
        hmi.writeString("tape2.tmAnimation.en=0");    
        //Paramos la animación del indicador de recording
        hmi.writeString("tape.tmAnimation.en=0");
        
        // Reiniciamos el estado de los botones
        hmi.writeString("tape.STOPst.val=1");
        hmi.writeString("tape.RECst.val=0");           

        delay(2000);
      }

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
          threshold_high = (SCHMITT_THR * 32767)/100;
          threshold_low = (-1)*(SCHMITT_THR * 32768)/100;            
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
              delay(2000);
              REC = true;
              terminate(true);
              errorDetected = 0;
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

          // pLead = 0;
          // pSync = 0;
          // state2 = 0;

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
          strcpy(header.name,"noname");

          bufferRec = (uint8_t*)ps_calloc(BUFFER_SIZE_REC,sizeof(uint8_t));
          initializeBuffer();

          errorInDataRecording = false;
          blockCount = 0;
          stateRecording = 0;
          statusSchmitt = 0;
          //
          resetMeasuredPulse();

          stopRecordingProccess = false; 
          stateInfoBlock = 0;
          wasRenamed = false;
          nameFileRead = false;
          WasfirstStepInTheRecordingProccess = false;
          statusSchmitt = 0;
          // Ponemos a cero todos los indicadores
          _hmi.resetIndicators();           
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
              fileWasNotCreated = true;
              stopRecordingProccess = true;
              return false;
            }
            else
            {
              // El fichero fue creado. Entonces está abierto
              fileWasNotCreated = false;
              return true;
            }             
          }
          else
          {
            // ESTE CASO ES RARO y NO DEBERÍA DARSE NUNCA
            // pero lo controlamos por si acaso.

            // El fichero fue creado. Entonces está abierto
            fileWasNotCreated = false;            
            return true;
          }  
        }
        else
        {
          // El fichero fue creado. Entonces está abierto
          fileWasNotCreated = false;
          return true;
        }        
      }

      bool fillAndCloseFile()
      {
          bool fileWasClosed = false;

          for(int i=0;i<256;i++)
          {
            _mFile.write(0x01);
          }

          _mFile.close();
          fileWasClosed = true;
          delay(250); 

          return fileWasClosed;    
      }

      bool saveBlocksOnFile(bool partially)
      {
          bool fileWasClosed = false;

          if (_mFile.isOpen())
          {
            // Lo renombramos con el nombre del BASIC
            renameFile();        
            delay(125);                       
          }  

          if (partially)
          {
              // Finalmente se graba el contenido menos el bloque erroneo
              int currentOffset = _mFile.position();
              _mFile.seek(ptrOffset);
              uint8_t MSB = 0;
              uint8_t LSB = 0;                      
              _mFile.write(LSB);
              _mFile.write(MSB);
              _mFile.seek(currentOffset);

              // Lo cerramos
              _mFile.close();
              fileWasClosed = true;

              delay(125);

              LAST_MESSAGE = "File partially saved.";

              if (_mFile.size() < 1024)
              {
                LAST_MESSAGE += " Size: " + String(_mFile.size()) + " bytes";
              }
              else
              {
                LAST_MESSAGE += " Size: " + String(_mFile.size()/1024) + " KB";
              }
          }
          else
          {
              // Finalmente se graba todo el contenido         
              // Lo cerramos
              _mFile.close();
              fileWasClosed = true;

              delay(125);

              LAST_MESSAGE = "File saved.";

              if (_mFile.size() < 1024)
              {
                LAST_MESSAGE += " Size: " + String(_mFile.size()) + " bytes";
              }
              else
              {
                LAST_MESSAGE += " Size: " + String(_mFile.size()/1024) + " KB";
              }

          }

          return fileWasClosed;

      }

      void terminate(bool removeFile)
      {

        // ************************************************************************
        //
        // En este procedimiento cerramos el fichero, para validarlo por el SO
        //
        // *************************************************************************

        //FIRSTBLOCKREAD = false;
        bool fileWasClosed = false;

        // Si REC no está activo, no podemos terminar.
        if (REC)
        {
            // Vemos si el fichero inicialmente fue creado.
            if (fileWasNotCreated == false)
            {
                // El fichero inicialmente fue creado con exito
                // en la SD, pero ...
                //

                // Si el bloque ha sido completado "tono guia + bytes + silencio"
                // se almacena
                if (BLOCK_REC_COMPLETED)
                {
                    // Se almacenó algo y hay bloques completos validados
                    if (_mFile.size() !=0)
                    {

                        if (!removeFile)
                        {
                          fileWasClosed = saveBlocksOnFile(false);
                        }
                        else
                        {
                          //
                          // El ultimo bloque es erroneo pero el resto no
                          //
                          fileWasClosed = saveBlocksOnFile(true);
                        }
                    }
                    else
                    {
                      // Tiene 0 bytes. Meto algo y lo cierro
                      // es un error
                      // Escribimos 256 bytes
                      fileWasClosed = fillAndCloseFile();
                    }
                }
                else
                {
                    // Aqui entra porque el BLOCK_REC_COMPLETED == false
                    // entonces el ultimo bloque no fue terminado.
                    //
                    // El ultimo bloque es erroneo pero el resto no

                    if (byteCount !=0 && errorDetected == 0)
                    {

                        // Finalmente se graba el contenido menos el bloque erroneo
                        if (_mFile.isOpen())
                        {                     
                            // Lo renombramos con el nombre del BASIC
                            renameFile();        
                            delay(125);                       
                        }          

                        if (ptrOffset != 0)
                        {
                          //
                          fileWasClosed = saveBlocksOnFile(true);  
                        }
                        else
                        {
                          // Lo cerramos
                          _mFile.close();
                          fileWasClosed = true;
                          delay(125);                          
                        }
                    }
                    else
                    {                  
                        // Tiene 0 bytes. Meto algo y lo cierro
                        // es un error
                        // Escribimos 256 bytes
                        fileWasClosed = fillAndCloseFile();
                    }        
                }
            }
            else
            {

              // El fichero no fue creado
              _mFile.close();
              fileWasClosed = true;
              delay(125); 
            }
        }

        wasRenamed = false;
        nameFileRead = false;
        WasfirstStepInTheRecordingProccess = false;
        statusSchmitt = 0;
        // Ponemos a cero todos los indicadores
        _hmi.resetIndicators();   
        
        // Nos aseguramos el cierre.  05/11/2024 - 02:17
        _mFile.close();
        fileWasClosed = true;
        delay(1000);
      }

      TAPrecorder()
      {}
};
