/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: ZXProccesor.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Clase que implementa metodos y parametrizado para todas las señales que necesita el ZX Spectrum para la carga de programas.

    NOTA: Esta clase necesita de la libreria Audio-kit de Phil Schatzmann - https://github.com/pschatzmann/arduino-audiokit
    
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

//#include <stdint.h>

#pragma once

// Clase para generar todo el conjunto de señales que necesita el ZX Spectrum
class ZXProccesor 
{
   
    private:

    HMI _hmi;
    
    // Definición de variables internas y constantes
    uint8_t buffer[0];
    
    // Parametrizado para el ES8388 a 44.1KHz
    const float samplingRate = 44099.988;
    const float sampleDuration = (1.0 / samplingRate); //0.0000002267; //
                                                              // segundos para 44.1HKz
    const float maxAmplitude = 32767.0;
    float m_amplitude = maxAmplitude; 
    const int channels = 1;  
    
    public:
    // Parametrizado para el ZX Spectrum - Timming de la ROM
    float freqCPU = DfreqCPU;
    float tState = (1.0 / freqCPU); //0.00000028571 --> segundos Z80 
                                          //T-State period (1 / 3.5MHz)
    int SYNC1 = DSYNC1;
    int SYNC2 = DSYNC2;
    int BIT_0 = DBIT_0;
    int BIT_1 = DBIT_1;
    int PULSE_PILOT = DPULSE_PILOT;
    int PILOT_TONE = DPILOT_TONE;

    int PULSE_PILOT_DURATION = PULSE_PILOT * PILOT_TONE;
    //int PULSE_PILOT_DURATION = PULSE_PILOT * DPILOT_DATA;

    float silent = DSILENT;
    float m_time = 0;

    private:

    byte _mask_last_byte = 8;

    AudioKit m_kit;

    // size_t createWave(uint8_t *buffer, size_t bytes){
        
    //     // Procedimiento para generar un tren de pulsos cuadrados completo

    //     int chn = channels;
    //     size_t result = 0;
    //     int16_t *ptr = (int16_t*)buffer;

    //     // Pulso alto (mitad del periodo)
    //     for (int j=0;j<bytes/2;j++){

    //         int16_t sample = m_amplitude;
    //         *ptr++ = sample;
    //         if (chn>1)
    //         {
    //           *ptr++ = sample;
    //         }
    //         result+=2*chn;
    //     }

    //     // Pulso bajo (la otra mitad)
    //     for (int j=bytes/2;j<bytes;j++){
            
    //         int16_t sample = 0;
            
    //         *ptr++ = sample;
    //         if (chn>1)
    //         {
    //           *ptr++ = sample;
    //         }
    //         result+=2*chn;
    //     }

    //     return result;
    // }

    // void get_amplitude()
    // {
    //     // Establecemos la amplitud máxima.
    //     // Leemos el valor del vol.val
      
    //     int cvalue = myNex.readNumber("menu.vol.val");

    //     if (cvalue != 777777)
    //     {
    //       if (cvalue == 0)
    //       {
    //           cvalue = 90;
    //           MAIN_VOL = cvalue;              
    //       }

    //       if (cvalue != LAST_MAIN_VOL)
    //       {
    //           MAIN_VOL = cvalue;

    //           SerialHW.println("");
    //           SerialHW.println("");
    //           SerialHW.println("Valor leido: " + String(cvalue));

    //           LAST_MAIN_VOL = MAIN_VOL;
    //       }
    //     }


    // }

    size_t silenceWave(uint8_t *buffer, size_t samples)
    {
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        for (int j=0;j<(samples/2);j++)
        {
            int16_t sample = (m_amplitude/2);
            //int16_t sample = 0;
            *ptr++ = sample;
            if (chn>1)
            {
              *ptr++ = sample;
            }            
            result+=2;
        }

        return result;      
    }

    size_t clearBuffer(uint8_t *buffer, size_t bytes)
    {
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        for (int j=0;j<bytes/(2*chn);j++){

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

    size_t readSin(uint8_t *buffer, size_t bytes, float freq)
    {

        // Antes de iniciar la reproducción ajustamos el volumen de carga.
        //ESP32kit.setVolume(MAIN_VOL);
        //get_amplitude();
        m_amplitude = MAIN_VOL * 32767 / 100;

        float double_Pi = PI * 2.0;
        float angle = double_Pi * freq * m_time + 0;
        int16_t result = m_amplitude * sin(angle);
        m_time += 1.0 / samplingRate; 

        return m_time;     
    }

    size_t createWave(uint8_t *buffer, size_t bytes){
        
        // Procedimiento para generar un tren de pulsos cuadrados completo
        // Antes de iniciar la reproducción ajustamos el volumen de carga.
        //ESP32kit.setVolume(MAIN_VOL);


        //get_amplitude();

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        // Pulso alto (mitad del periodo)
        for (int j=0;j<bytes/(4*chn);j++){

            if (j % 32 == 0)
            {
              m_amplitude = MAIN_VOL * 32767 / 100;
            }

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

        // Metemos un cero al final
        // *ptr++ = 0;
        // if (chn>1)
        // {
        //   *ptr++ = 0;
        // }
        // result+=2*chn;

        return result;
    }

    size_t readPulse(uint8_t *buffer, size_t bytes, int slope, bool end){

        // Antes de iniciar la reproducción ajustamos el volumen de carga.
        //ESP32kit.setVolume(MAIN_VOL);

        //get_amplitude();            

        // Procedimiento para genera un pulso 
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        for (int j=0;j<bytes/(2*chn);j++){

            if (j % 32 ==0)
            {
              m_amplitude = MAIN_VOL * 32767 / 100;
            }

            int16_t sample = 0;
            sample = m_amplitude * slope;

            if (end)
            {
                sample = (m_amplitude/2) + (m_amplitude/4);
            }
            
            *ptr++ = sample;
            
            if (chn>1)
            {
              *ptr++ = sample;
            }
            result+=2*chn;
        }

        // Metemos un cero al final
        // *ptr++ = 0;
        // if (chn>1)
        // {
        //   *ptr++ = 0;
        // }
        // result+=2*chn;

        return result;          
    }

    void generatePulse(float freq, int samplingRate, int slope, bool end)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));

        uint8_t buffer[bytes+2];

        for (int m=0;m < 1;m++)
        {
          // Escribimos el tren de pulsos en el procesador de Audio
          m_kit.write(buffer, readPulse(buffer, bytes, slope, end));
        } 
    }

    void generateWavePulses(float freq, int numPulses, int samplingRate)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));

        //SerialHW.println("****** BUFFER SIZE --> " + String(bytes));

        uint8_t buffer[bytes];


        for (int m=0;m < numPulses;m++)
        {

          // Escribimos el tren de pulsos en el procesador de Audio
          m_kit.write(buffer, createWave(buffer, bytes));
        } 
    }

    void generateOneWave(float freq, int samplingRate)
    {
        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate

        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        uint8_t buffer[bytes];

        m_kit.write(buffer, createWave(buffer, bytes));
                
        //_hmi.readUART();

        if (LOADING_STATE == 1)
        {
            if (STOP==true)
            {
                LOADING_STATE = 2; // Parada del bloque actual
                //stopTape();
            }
            else if (PAUSE==true)
            {
                LOADING_STATE = 2; // Parada del bloque actual
                //pauseTape();
            }
        }
    }

    void generateWaveDuration(float freq, float duration, int samplingRate)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int((1.0 / ((freq / 4.0))) / Tsr);
        int numPulses = 4 * int(duration / (bytes*Tsr));

        uint8_t buffer[bytes];      

        for (int m=0;m < numPulses;m++)
        {
            
            //buttonsControl();
            //_hmi.readUART();

            if (LOADING_STATE == 1)
            {
                if (STOP==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    //stopTape();
                    break;
                }
                else if (PAUSE==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    //pauseTape();
                    break;
                }
            }

            // Limpiamos el buffer
            //m_kit.write(buffer, clearBuffer(buffer,bytes));
            // Rellenamos
            m_kit.write(buffer, createWave(buffer, bytes));
        } 
    }

    public:

    void silence(float duration)
    {
        // Esta onda se genera como el resto sumando trozos de onda
        // esto es debido al limite del buffer
        // no podemos hacer un buffer muy grande, peta el ESP32

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Td = 4*(duration / 1000);
        float Tsr = (1.0 / samplingRate);
        int samples = int(Td / Tsr);

        // Inicializamos con un tamaño de bloque de 1024 muestras cada vez
        int bufferSize = 2048;
        // Calculamos cuantos bloques tenemos que concatenar. Si el valor de
        // samples es menor, saldrá 0
        int frames = samples / bufferSize;
        int delta = abs(samples - (bufferSize * frames));

        // Si es cero, entonces el buffer será igual de grande que el 
        // numero de samples a rellenar para formar la onda
        if (frames == 0)
        {
            bufferSize = samples;
            frames = 1;
        }


        // SerialHW.println("");
        // SerialHW.println("");
        // SerialHW.println("Silencio: " + String(duration));
        // SerialHW.println("Samples : " + String(samples));
        // SerialHW.println("bufferSize: " + String(bufferSize));
        // SerialHW.println("Frames   : " + String(frames));
        // SerialHW.println("");
        // SerialHW.println("");
    
        // Rellenamos repitiendo el patron varias veces
        // porque el buffer es limitado
        for (int n=0;n<frames;n++)
        {
            // El ultimo frame que compone la señal tendrá el restante
            // ya que el ancho de la señal no siempre será multiplo exacto
            // del bufferSize, por lo tanto el ultimo tendrá ese restante 
            // (delta)
            if (n == (frames-1))
            {
                bufferSize = bufferSize + delta;
                //SerialHW.println("bufferSize + delta: " + String(bufferSize));
            }
  
            // Aplicamos la reserva de buffer
            uint8_t buffer[bufferSize]; 
            m_kit.write(buffer, silenceWave(buffer, bufferSize));
        }
    }

    void pilotTone(float duration)
    {
        //SerialHW.println("****** BUFFER SIZE --> " + String(duration));
        float freq = (1 / (PULSE_PILOT * tState)) / 2;    
        //SerialHW.println("******* PILOT HEADER " + String(freq) + " Hz");

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

    void syncTone(int nTStates, int slope)
    {
        // Procedimiento que genera un pulso de sincronismo, según los
        // T-States pasados por parámetro.
        //
        // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
        // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
        float freq = (1 / (nTStates * tState));    
        generatePulse(freq, samplingRate,slope, false);

        //int width = 200;
        //// Metemos un pulso de cambio de estado
        //freq = (1 / (width * tState)) / 2;    
        //generateOneWave(freq,samplingRate);  
        
    }

    private:
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
        byte _mask = 8;   // Para el last_byte

        // Procedimiento para enviar datos desde un array
        if (LOADING_STATE==1 || TEST_RUNNING)
        {
            byte bRead = 0x00;
            int bytes_in_this_block = 0;

            for (int i = 0; i < size;i++)
            {
            
              if (!TEST_RUNNING)
              {

                    //_hmi.readUART();

                    if (i % 32==0)
                    {
                        // Progreso de cada bloque.
                        // Con este metodo reducimos el consumo de datos
                        //_hmi.writeString("");
                        _hmi.writeString("progression.val=" + String((int)((i*100)/(size-1))));

                        if (BYTES_LOADED > BYTES_TOBE_LOAD)
                        {
                            BYTES_LOADED = BYTES_TOBE_LOAD;
                        }

                        //_hmi.writeString("");
                        _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));

                        _hmi.updateInformationMainPage();                    

                        //buttonsControl();
                    }

                    if (i == (size-1))
                    {
                        // Esto lo hacemos para asegurarnos que la barra se llena entera
                        //_hmi.writeString("");
                        _hmi.writeString("progression.val=" + String((int)((i*100)/(size-1))));

                        if (BYTES_LOADED > BYTES_TOBE_LOAD)
                        {
                            BYTES_LOADED = BYTES_TOBE_LOAD;
                        }

                        //_hmi.writeString("");
                        _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
                    }



                    if (LOADING_STATE == 1)
                    {
                        if (STOP==true)
                        {
                            LOADING_STATE = 2; // Parada del bloque actual
                            //stopTape();
                            i=size;
                            //break;
                        }
                        else if (PAUSE==true)
                        {
                            LOADING_STATE = 2; // Parada del bloque actual
                            //pauseTape();
                            i=size;
                            //break;
                        }

                    }
              }


              if (LOADING_STATE == 1 || TEST_RUNNING)
              {
                  // Vamos a ir leyendo los bytes y generando el sonido
                  bRead = data[i];
                  
                  // Para la protección con mascara ID11 - 0x0C
                  // "Used bits in the last byte (other bits should be 0) {8}
                  //(e.g. if this is 6, then the bits used (x) in the last byte are: xxxxxx00, wh///ere MSb is the leftmost bit, LSb is the rightmost bit)"
                  
                  // ¿Es el ultimo BYTE?. Si se ha aplicado mascara entonces
                  // se modifica el numero de bits a transmitir
                  if (i == size-1)
                  {
                      _mask = _mask_last_byte;
                  }
                  else
                  {
                      _mask = 8;
                  }

                  for (int n=0;n < _mask;n++)
                  {
                      // Si el bit leido del BYTE es un "1"
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
                  if (!TEST_RUNNING)
                  {
                      BYTES_LOADED++;
                      bytes_in_this_block++;
                      BYTES_LAST_BLOCK = bytes_in_this_block;              
                  }
              }
              else
              {
                break;
              }
            }

            int width = 0;
            // Leemos el ultimo bit (del ultimo byte), y dependiendo de como sea
            // así cerramos el flanco.
            // Cogemos el ultimo byte
            bRead = data[size-1];

            // Vemos como es el último bit MSB es la posición 0, el ultimo bit
            if (bitRead(bRead, 0) == 1)
            {
                width = BIT_1 / 2;
            }
            else
            {
                width = BIT_0 / 2;
            }

            
            // Metemos un pulso de cambio de estado
            // para asegurar el cambio de flanco alto->bajo, del ultimo bit
            float freq = (1 / (width * tState))/2;    
            generatePulse(freq, samplingRate,1,true);

        }
    }
    
    public:

        void set_maskLastByte(byte mask)
        {
            _mask_last_byte = mask;
        }

        void playData(byte* bBlock, int lenBlock, int pulse_pilot_duration)
        {

            float duration = tState * pulse_pilot_duration;

            // Put now code block
            // syncronize with short leader tone
            //clear(duration);
            pilotTone(duration);

            // syncronization for end short leader tone
            syncTone(SYNC1,1);
            syncTone(SYNC2,0);

            // Send data
            sendDataArray(bBlock, lenBlock);
                        
            // Silent tone
            silence(silent);
        }

        void playDataBegin(byte* bBlock, int lenBlock, int pulse_pilot_duration)
        {
                    // PROGRAM
            float duration = tState * pulse_pilot_duration;
            // Put now code block
            // syncronize with short leader tone
            pilotTone(duration);
            // syncronization for end short leader tone
            syncTone(SYNC1,1);
            syncTone(SYNC2,0);

            // Send data
            sendDataArray(bBlock, lenBlock);
                   
        }

        void playDataEnd(byte* bBlock, int lenBlock, int pulse_pilot_duration)
        {

            float duration = tState * pulse_pilot_duration;
            // Send data
            sendDataArray(bBlock, lenBlock);
            
            // syncronization for end data block          
            //syncTone(SYNC2,0);
            
            // Silent tone
            silence(silent);
        }

        void playBlock(byte* header, int len_header, byte* data, int len_data, int pulse_pilot_duration_header, int pulse_pilot_duration_data)
        {           
            #if LOG==3
              SerialHW.println("******* PROGRAM HEADER");
              SerialHW.println("*******  - HEADER size " + String(len_header));
              SerialHW.println("*******  - DATA   size " + String(len_data));
            #endif

            float durationHeader = tState * pulse_pilot_duration_header;
            float durationData = tState * pulse_pilot_duration_data;


            // PROGRAM
            //HEADER PILOT TONE
            pilotTone(durationHeader);
            // SYNC TONE
            syncTone(SYNC1,1);
            // Launch header
            syncTone(SYNC2,0);

            sendDataArray(header, len_header);

            // Silent tone
            silence(silent);

            #if LOG==3
              SerialHW.println("******* PROGRAM DATA");
            #endif

            // Put now code block
            // syncronize with short leader tone
            pilotTone(durationData);
            // syncronization for end short leader tone
            syncTone(SYNC1,1);
            syncTone(SYNC2,0);

            // Send data
            sendDataArray(data, len_data);
            
            // syncronization for end data block          
            
            // Silent tone
            silence(silent);
        }

        void playHeaderOnly(byte* header, int len_header, int pulse_pilot_duration)
        {           

            float duration = tState * pulse_pilot_duration;

            // PROGRAM
            //HEADER PILOT TONE
            pilotTone(duration);
            // SYNC TONE
            syncTone(SYNC1,1);
            // Launch header
            syncTone(SYNC2,0);

            sendDataArray(header, len_header);

            // Silent tone
            silence(silent);
        }        

        void set_ESP32kit(AudioKit kit)
        { 
          m_kit = kit;
        }

        void set_HMI(HMI hmi)
        {
          _hmi = hmi;
        }

        // Constructor
        ZXProccesor()
        {
          // Constructor de la clase
          //m_kit = kit;
        }

};
