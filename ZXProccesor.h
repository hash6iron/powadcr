/*
 ZXProccesor.h
 Antonio Tamairón. 2023

 Descripción:
 Clase que implementa metodos y parametrizado para todas las señales que necesita el ZX Spectrum para el proceso de carga de programas.

 Version: 0.1

 NOTA: Esta clase necesita de la libreria Audio-kit de Phil Schatzmann - https://github.com/pschatzmann/arduino-audiokit

*/


#include <stdint.h>
#include "AudioKitHAL.h"
 
AudioKit m_kit;

// Variables necesarias
const int BUFFER_SIZE = 0;
uint8_t buffer[BUFFER_SIZE];

// Parametrizado para el ES8388
const int samplingRate = 44100;
const float sampleDuration = 0.0000002267; //segundos para 44.1HKz
const float maxAmplitude = 32767.0;
const int channels = 2;
//const int turboMode = 1;

float m_amplitude = maxAmplitude; 

// Parametrizado para el ZX Spectrum
const float tState = 0.00000028571; //segundos Z80 T-State period (1 / 3.5MHz)
int SYNC1 = 667;
int SYNC2 = 735;
int BIT_0 = 855;
int BIT_1 = 1710;
int PULSE_PILOT = 2168;
int PULSE_PILOT_HEADER = PULSE_PILOT * 8063;
int PULSE_PILOT_DATA = PULSE_PILOT * 3223;

// Clase para generar todo el conjunto de señales que necesita el ZX Spectrum
class ZXProccesor 
{

    private:

    void stopTape()
    {
        BLOCK_SELECTED = 0;
        BYTES_LOADED = 0;
        writeString("");
        writeString("currentBlock.val=1");   
        writeString("");
        writeString("progressTotal.val=0");      
        writeString("");
        writeString("progression.val=0");      
    }

    void pauseTape()
    {
        BLOCK_SELECTED = CURRENT_BLOCK_IN_PROGRESS;

        //Serial.println("");
        //Serial.println("BYTES READ: " + String(BYTES_LAST_BLOCK) + "/" + String(BYTES_LOADED));
        //Serial.println("");

        BYTES_LOADED = BYTES_LOADED - BYTES_LAST_BLOCK;

        if (BYTES_LOADED > BYTES_TOBE_LOAD)
        {
            BYTES_LOADED = BYTES_TOBE_LOAD;
        }

        writeString("");
        writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
    }

    public:
        
        ZXProccesor(AudioKit kit)
        {
          // Constructor de la clase
          m_kit = kit;
        }

        size_t readWave(uint8_t *buffer, size_t bytes){
            
            // Procedimiento para generar un tren de pulsos cuadrados completo

            int chn = channels;
            size_t result = 0;
            int16_t *ptr = (int16_t*)buffer;

            // Pulso alto (mitad del periodo)
            for (int j=0;j<bytes/(4*chn);j++){

                int16_t sample = m_amplitude;
                *ptr++ = sample;
                if (chn>1)
                {
                  *ptr++ = sample;
                }
                result+=2*chn;
            }

            // Pulso bajo (la otra mitad)
            for (int j=bytes/(4*chn);j<bytes/(2*chn);j++){
                
                int16_t sample = 0;
                
                *ptr++ = sample;
                if (chn>1)
                {
                  *ptr++ = sample;
                }
                result+=2*chn;
            }

            return result;
        }

        size_t readPulse(uint8_t *buffer, size_t bytes, int slope){

            // Procedimiento para genera un pulso 
            int chn = channels;
            size_t result = 0;
            int16_t *ptr = (int16_t*)buffer;
            for (int j=0;j<bytes/(2*chn);j++){
                int16_t sample = m_amplitude * slope;
                
                *ptr++ = sample;
                
                if (chn>1)
                {
                  *ptr++ = sample;
                }
                result+=2*chn;
            }

            return result;          
        }

        void generatePulse(float freq, int samplingRate, int slope)
        {

            if (TURBOMODE)
            {
                freq=freq*2;
            }

            // Obtenemos el periodo de muestreo
            // Tsr = 1 / samplingRate
            float Tsr = (1.0 / samplingRate);
            int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));

            uint8_t buffer[bytes];

            for (int m=0;m < 1;m++)
            {
              // Escribimos el tren de pulsos en el procesador de Audio
              m_kit.write(buffer, readPulse(buffer, bytes, slope));
            } 
        }

        void generateWavePulses(float freq, int numPulses, int samplingRate)
        {

            if (TURBOMODE)
            {
                freq=freq*2;
            }

            // Obtenemos el periodo de muestreo
            // Tsr = 1 / samplingRate
            float Tsr = (1.0 / samplingRate);
            int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));

            //Serial.println("****** BUFFER SIZE --> " + String(bytes));

            uint8_t buffer[bytes];

            for (int m=0;m < numPulses;m++)
            {
              // Escribimos el tren de pulsos en el procesador de Audio
              m_kit.write(buffer, readWave(buffer, bytes));
            } 
        }

        void generateOneWave(float freq, int samplingRate)
        {
            // Obtenemos el periodo de muestreo
            // Tsr = 1 / samplingRate
            if (TURBOMODE)
            {
                freq=freq*2;
            }

            float Tsr = (1.0 / samplingRate);
            int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
            uint8_t buffer[bytes];
            m_kit.write(buffer, readWave(buffer, bytes));
            
            //buttonsControl();
            readUART();

            if (LOADING_STATE == 1)
            {
                if (STOP==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    stopTape();
                }
                else if (PAUSE==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    pauseTape();
                }
            }
        }

        void generateWaveDuration(float freq, float duration, int samplingRate)
        {

            // if (turboMode==1)
            // {
            //     freq=freq*2;
            // }
            // Obtenemos el periodo de muestreo
            // Tsr = 1 / samplingRate
            float Tsr = (1.0 / samplingRate);
            int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
            int numPulses = 4 * int(duration / (bytes*Tsr));

            //Serial.println("****** BUFFER SIZE --> " + String(bytes));
            //Serial.println("****** Tsr         --> " + String(Tsr));
            //Serial.println("****** NUM PULSES  --> " + String(numPulses));

            uint8_t buffer[bytes];

            for (int m=0;m < numPulses;m++)
            {
                
                //buttonsControl();
                readUART();

                if (LOADING_STATE == 1)
                {
                    if (STOP==true)
                    {
                        LOADING_STATE = 2; // Parada del bloque actual
                        stopTape();
                        break;
                    }
                    else if (PAUSE==true)
                    {
                        LOADING_STATE = 2; // Parada del bloque actual
                        pauseTape();
                        break;
                    }
                }
                
                m_kit.write(buffer, readWave(buffer, bytes));
            } 
        }

        void pilotToneHeader()
        {
            float duration = tState * PULSE_PILOT_HEADER;
            //Serial.println("****** BUFFER SIZE --> " + String(duration));
            float freq = (1 / (PULSE_PILOT * tState)) / 2;    
            //Serial.println("******* PILOT HEADER " + String(freq) + " Hz");

            generateWaveDuration(freq, duration, samplingRate);
        }

        void pilotToneHeader(float duration)
        {
            //Serial.println("****** BUFFER SIZE --> " + String(duration));
            float freq = (1 / (PULSE_PILOT * tState)) / 2;    
            //Serial.println("******* PILOT HEADER " + String(freq) + " Hz");

            generateWaveDuration(freq, duration, samplingRate);
        }


        void pilotToneData()
        {
            float duration = tState * PULSE_PILOT_DATA;
            float freq = (1 / (PULSE_PILOT * tState)) / 2;    
            //Serial.println("******* PILOT DATA " + String(freq) + " Hz");
            generateWaveDuration(freq, duration, samplingRate);
        }


        void pilotToneData(float duration)
        {
            float freq = (1 / (PULSE_PILOT * tState)) / 2;    
            //Serial.println("******* PILOT DATA " + String(freq) + " Hz");
            generateWaveDuration(freq, duration, samplingRate);
        }

        void zeroTone()
        {
            // Procedimiento que genera un bit "0"
            float freq = (1 / (BIT_0 * tState)) / 2;
            generateOneWave(freq, samplingRate);
        }

        void oneTone()
        {
            // Procedimiento que genera un bit "1"
            float freq = (1 / (BIT_1 * tState)) / 2;
            generateOneWave(freq, samplingRate);
        }

        void syncTone(int nTStates)
        {
            // Procedimiento que genera un pulso de sincronismo, según los
            // T-States pasados por parámetro.
            //
            // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
            // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
            float freq = (1 / (nTStates * tState)) / 2;    
            generateOneWave(freq, samplingRate);
        }

        void sendDataStr(String data)
        {
          //
          // Procedimiento para enviar datos binarios desde una cadena de caracteres
          //
          for (int n=0;n<data.length();n++)
          {
            char c = data[n];

            if (c == '0')
            {
              zeroTone();
            }
            else
            {
              oneTone();
            }
          }
        }

        void sendDataArray(byte* data, int size)
        {

            // Procedimiento para enviar datos desde un array
            if (LOADING_STATE==1)
            {
                byte bRead = 0x00;
                int bytes_in_this_block = 0;

                for (int i = 0; i < size;i++)
                {


                
                  if (i % 8==0)
                  {
                      // Progreso de cada bloque.
                      // Con este metodo reducimos el consumo de datos
                      writeString("");
                      writeString("progression.val=" + String((int)((i*100)/(size-1))));

                      if (BYTES_LOADED > BYTES_TOBE_LOAD)
                      {
                          BYTES_LOADED = BYTES_TOBE_LOAD;
                      }

                      writeString("");
                      writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
                  }

                  if (i % 32==0)
                  {
                      updateInformationMainPage();                    
                  }

                  if (i == (size-1))
                  {
                      // Esto lo hacemos para asegurarnos que la barra se llena entera
                      writeString("");
                      writeString("progression.val=" + String((int)((i*100)/(size-1))));

                      if (BYTES_LOADED > BYTES_TOBE_LOAD)
                      {
                          BYTES_LOADED = BYTES_TOBE_LOAD;
                      }

                      writeString("");
                      writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
                  }

                  //buttonsControl();
                  readUART();

                  if (LOADING_STATE == 1)
                  {
                      if (STOP==true)
                      {
                          LOADING_STATE = 2; // Parada del bloque actual
                          stopTape();
                          i=size;
                          //break;
                      }
                      else if (PAUSE==true)
                      {
                          LOADING_STATE = 2; // Parada del bloque actual
                          pauseTape();
                          i=size;
                          //break;
                      }

                  }

                  if (LOADING_STATE == 1)
                  {
                      // Vamos a ir leyendo los bytes y generando el sonido
                      bRead = data[i];
                      //Serial.println("******* Byte " + String(bRead));
                      for (int n=0;n<8;n++)
                      {
                          // Si el bit leido del BYTE es un "1"
                          //Serial.println("*******---- bit " + String(bitRead(bRead,8-n)));
                          if(bitRead(bRead, 7-n) == 1)
                          {
                              // Procesamos "1"
                              oneTone();
                          }
                          else
                          {
                              // En otro caso
                              // procesamos "0"
                              zeroTone();
                          }
                      }

                      // Hemos cargado 1 bytes. Seguimos
                      BYTES_LOADED++;
                      bytes_in_this_block++;
                      BYTES_LAST_BLOCK = bytes_in_this_block;              
                  }
                  else
                  {
                    break;
                  }



                }

            }

        }

        void playHeader(byte* bBlock, int lenBlock)
        {
            // PROGRAM
            //HEADER PILOT TONE
            float duration = tState * PULSE_PILOT_HEADER/3;
            pilotToneHeader(duration);
            // SYNC TONE
            syncTone(SYNC1);
            // Launch header

            sendDataArray(bBlock, lenBlock);
            syncTone(SYNC2);

            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif          
        }

        void playHeaderProgram(byte* bBlock, int lenBlock)
        {
            // PROGRAM
            //HEADER PILOT TONE
            pilotToneHeader();
            // SYNC TONE
            syncTone(SYNC1);
            // Launch header

            sendDataArray(bBlock, lenBlock);
            syncTone(SYNC2);

            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif          
        }

        void playData(byte* bBlock, int lenBlock)
        {
            // Put now code block
            // syncronize with short leader tone
            //float duration = tState * PULSE_PILOT_DATA/2;
            pilotToneData();
            // syncronization for end short leader tone
            syncTone(SYNC1);

            // Send data
            sendDataArray(bBlock, lenBlock);
            
            // syncronization for end data block          
            syncTone(SYNC2);
            
            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif          
        }

        void playDataBegin(byte* bBlock, int lenBlock)
        {
            // Put now code block
            // syncronize with short leader tone
            pilotToneData();
            // syncronization for end short leader tone
            syncTone(SYNC1);

            // Send data
            sendDataArray(bBlock, lenBlock);
                   
        }

        void playDataEnd(byte* bBlock, int lenBlock)
        {

            // Send data
            sendDataArray(bBlock, lenBlock);
            
            // syncronization for end data block          
            syncTone(SYNC2);
            
            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif          
        }

        void playBlock(byte* header, int len_header, byte* data, int len_data)
        {           
            #ifdef LOG=3
              //Serial.println("******* PROGRAM HEADER");
              //Serial.println("*******  - HEADER size " + String(len_header));
              //Serial.println("*******  - DATA   size " + String(len_data));
            #endif

            // PROGRAM
            //HEADER PILOT TONE
            pilotToneHeader();
            // SYNC TONE
            syncTone(SYNC1);
            // Launch header

            sendDataArray(header, len_header);
            syncTone(SYNC2);

            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif

            #ifdef LOG=3
              //Serial.println("******* PROGRAM DATA");
            #endif

            // Put now code block
            // syncronize with short leader tone
            pilotToneData();
            // syncronization for end short leader tone
            syncTone(SYNC1);

            // Send data
            sendDataArray(data, len_data);
            
            // syncronization for end data block          
            syncTone(SYNC2);
            
            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif
        }

        void playHeaderOnly(byte* header, int len_header)
        {           
            #ifdef LOG=3
              //Serial.println("");
              //Serial.println("******* PROGRAM HEADER");
              //Serial.println("*******  - HEADER size " + String(len_header) + " bytes");
              //Serial.println("");
              //Serial.println("Header to send:");
              //Serial.println("");
              // for (int n=0;n<19;n++)
              // {
              //     Serial.print(header[n],HEX);
              //     Serial.print(",");
              // }
            #endif

            // PROGRAM
            //HEADER PILOT TONE
            pilotToneHeader();
            // SYNC TONE
            syncTone(SYNC1);
            // Launch header

            sendDataArray(header, len_header);
            syncTone(SYNC2);

            // Silent tone
            #ifdef SILENT
              sleep(SILENT);
            #endif
        }        

};
