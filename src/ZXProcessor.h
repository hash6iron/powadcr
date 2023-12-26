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
class ZXProcessor 
{
   
    private:

    HMI _hmi;
    
    // Definición de variables internas y constantes
    uint8_t buffer[0];
    
    // Parametrizado para el ES8388 a 44.1KHz
    const float samplingRate = 44099.988;
    //const float samplingRate = 48000.0;
    const float sampleDuration = (1.0 / samplingRate); //0.0000002267; //
                                                              // segundos para 44.1HKz
    const float maxAmplitude = 32767.0;
    float m_amplitude_L = maxAmplitude; 
    float m_amplitude_R = maxAmplitude; 

    // Al poner 2 canales - Falla. Solucionar
    const int channels = 2;  
    const float speakerOutPower = 0.002;

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

    uint8_t _mask_last_byte = 8;

    AudioKit m_kit;

    size_t silenceWave(uint8_t *buffer, size_t samples)
    {
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        
        // Amplitud de silencio
        int A = 0;

        for (int j=0;j<(samples/2);j++)
        {
            m_amplitude_R = MAIN_VOL_R * A / 100;
            m_amplitude_L = MAIN_VOL_L * A / 100;
            int16_t sample_R = (m_amplitude_R / 2);
            int16_t sample_L = (m_amplitude_L / 2);

            if (!SWAP_EAR_CHANNEL)
            {
              //L-OUT
              *ptr++ = sample_R;
              //R-OUT
              *ptr++ = sample_L * (EN_MUTE) * (1-EN_MUTE_2);
            }
            else
            {
              //R-OUT
              *ptr++ = sample_L;
              //L-OUT
              *ptr++ = sample_R * (EN_MUTE);
            }


            result+=2*chn;
        }

        return result;
    }

    size_t silenceWaveEdge(uint8_t *buffer, size_t samples)
    {
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        int A = 0;

        for (int j=0;j<(samples/2);j++)
        {

            m_amplitude_R = MAIN_VOL_R * A / 100;
            m_amplitude_L = MAIN_VOL_L * A / 100;
            int16_t sample_R = (m_amplitude_R / 2);
            int16_t sample_L = (m_amplitude_L / 2);

            if (!SWAP_EAR_CHANNEL)
            {
              //L-OUT
              *ptr++ = sample_R;
              //R-OUT
              *ptr++ = sample_L * (EN_MUTE) * (1-EN_MUTE_2);
            }
            else
            {
              //R-OUT
              *ptr++ = sample_L;
              //L-OUT
              *ptr++ = sample_R * (EN_MUTE);
            }


            result+=2*chn;
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

            if (!SWAP_EAR_CHANNEL)
            {
              //L-OUT
              *ptr++ = sample * (1-EN_MUTE);
              //R-OUT
              *ptr++ = sample * EN_MUTE;
            }
            else
            {
              //R-OUT
              *ptr++ = sample * EN_MUTE;
              //L-OUT
              *ptr++ = sample * (1-EN_MUTE);
            }


            result+=2*chn;
        }

        return result;
    }

    size_t readSin(uint8_t *buffer, size_t bytes, float freq, bool stereo)
    {

        // Antes de iniciar la reproducción ajustamos el volumen de carga.
        m_amplitude_R = MAIN_VOL_R * 32767 / 100;
        m_amplitude_L = MAIN_VOL_L * 32767 / 100;

        float double_Pi = PI * 2.0;
        float angle = double_Pi * freq * m_time + 0;
        if (stereo)
        {
            int16_t result = m_amplitude_R * sin(angle);
            m_time += 1.0 / samplingRate; 
        }
        else
        {
            int16_t result = m_amplitude_L * sin(angle);
            m_time += 1.0 / samplingRate; 
        }

        return m_time;     
    }

    size_t createWave(uint8_t *buffer, size_t bytes)
    {
        
        // Procedimiento para generar un tren de pulsos cuadrados completo
        // Antes de iniciar la reproducción ajustamos el volumen de carga.

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        // Pulso alto (mitad del periodo)
        for (int j=0;j<bytes/(4*chn);j++){

            m_amplitude_R = MAIN_VOL_R * 32767 / 100;
            m_amplitude_L = MAIN_VOL_L * 32767 / 100;

            int16_t sample_L = m_amplitude_L;
            int16_t sample_R = m_amplitude_R;

            if (!SWAP_EAR_CHANNEL)
            {
              //R-OUT
              *ptr++ = sample_R;
              //L-OUT
              *ptr++ = sample_L * EN_MUTE;
            }
            else
            {
              //L-OUT
              *ptr++ = sample_L;
              //R-OUT
              *ptr++ = sample_R * EN_MUTE;
            }

            result +=2*chn;
        }

        // Pulso bajo (la otra mitad)
        for (int j=bytes/(4*chn);j<bytes/(2*chn);j++){
            
            int16_t sample = 0;          

            if (!SWAP_EAR_CHANNEL)
            {
              //L-OUT
              *ptr++ = sample * (1-EN_MUTE);
              //R-OUT
              *ptr++ = sample* EN_MUTE;
            }
            else
            {
              //R-OUT
              *ptr++ = sample* EN_MUTE;
              //L-OUT
              *ptr++ = sample * (1-EN_MUTE);
            }


            result +=2*chn;
        }

        return result;
    }

    size_t createWaveEdge(uint8_t *buffer, size_t bytes, int edge)
    {
        
        // Procedimiento para generar un tren de pulsos cuadrados completo
        // Antes de iniciar la reproducción ajustamos el volumen de carga.

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        int A = 32767;

        if (edge==1)
        {
            A=-32768;
        }
        else
        {
            A=32767;
        }

        // Pulso alto (mitad del periodo)
        for (int j=0;j<bytes/(4*chn);j++)
        {
            m_amplitude_R = MAIN_VOL_R * A / 100;
            m_amplitude_L = MAIN_VOL_L * A / 100;

            int16_t sample_L = m_amplitude_L;
            int16_t sample_R = m_amplitude_R;

            if (!SWAP_EAR_CHANNEL)
            {
              //R-OUT
              *ptr++ = sample_R;
              //L-OUT
              *ptr++ = sample_L * EN_MUTE;
            }
            else
            {
              //L-OUT
              *ptr++ = sample_L;
              //R-OUT
              *ptr++ = sample_R * EN_MUTE;
            }

            result +=2*chn;
        }


        if (edge==1)
        {
            A=32767;
            LAST_EDGE_IS = 1;
        }
        else
        {
            A=-32768;
            LAST_EDGE_IS = -1;
        }

        // Pulso bajo (la otra mitad) es invertida
        for (int j=bytes/(4*chn);j<bytes/(2*chn);j++){
            
            m_amplitude_R = MAIN_VOL_R * A / 100;
            m_amplitude_L = MAIN_VOL_L * A / 100;

            int16_t sample_L = m_amplitude_L;
            int16_t sample_R = m_amplitude_R;

            if (!SWAP_EAR_CHANNEL)
            {
              //R-OUT
              *ptr++ = sample_R;
              //L-OUT
              *ptr++ = sample_L * EN_MUTE;
            }
            else
            {
              //L-OUT
              *ptr++ = sample_L;
              //R-OUT
              *ptr++ = sample_R * EN_MUTE;
            }

            result +=2*chn;
        }

        return result;
    }

    size_t readPulse(uint8_t *buffer, size_t bytes, int slope, bool end)
    {

        // Procedimiento para genera un pulso 

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        
        for (int j=0;j<bytes/(2*chn);j++)
        {

            // slope tomará los valores 1 o -1
            if (slope==1)
            {
                m_amplitude_R = MAIN_VOL_R * 32767 / 100;
                m_amplitude_L = MAIN_VOL_L * 32767 / 100;
            }
            else
            {
                m_amplitude_R = MAIN_VOL_R * 32768 / 100;
                m_amplitude_L = MAIN_VOL_L * 32768 / 100;
            }
 
            int16_t sample_L = 0;
            int16_t sample_R = 0;

            sample_R = m_amplitude_R * slope;
            sample_L = m_amplitude_L * slope;

            if (end)
            {
                sample_R = (m_amplitude_R/2) + (m_amplitude_R/4);
                sample_L = (m_amplitude_L/2) + (m_amplitude_L/4);
            }
                       
            if (!SWAP_EAR_CHANNEL)
            {
              //R-OUT
              *ptr++ = sample_R;
              //L-OUT
              *ptr++ = sample_L * EN_MUTE;
            }
            else
            {
              //L-OUT
              *ptr++ = sample_L;
              //R-OUT
              *ptr++ = sample_R * EN_MUTE;
            }

            result+=2*chn;
        }

        return result;          
    }

    void generatePulse(float freq, int samplingRate, int slope, bool end)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];

        for (int m=0;m < 1;m++)
        {
          // Escribimos el tren de pulsos en el procesador de Audio
          m_kit.write(buffer, readPulse(buffer, bytes, slope, end));
        } 
    }

    void generateOneWave(float freq, int samplingRate)
    {
        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate

        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];

        m_kit.write(buffer, createWave(buffer, bytes));
                
        if (LOADING_STATE == 1)
        {
            if (STOP==true)
            {
                LOADING_STATE = 2; // Parada del bloque actual
                return;
            }
            else if (PAUSE==true)
            {
                LOADING_STATE = 3; // Pausa del bloque actual
                return;
            }
        }
    }

    void generateOneWaveEdge(float freq, int samplingRate, int edge)
    {
        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate

        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];

        m_kit.write(buffer, createWaveEdge(buffer, bytes,edge));
                
        if (LOADING_STATE == 1)
        {
            if (STOP==true)
            {
                LOADING_STATE = 2; // Parada del bloque actual
                return;
            }
            else if (PAUSE==true)
            {
                LOADING_STATE = 3; // Pausa del bloque actual
                return;
            }
        }
    }

    void generateWavePulses(float freq, int numPulses, int samplingRate)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];


        for (int m=0;m < numPulses;m++)
        {
            if (LOADING_STATE == 1)
            {
                if (STOP==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    return;
                }
                else if (PAUSE==true)
                {
                    LOADING_STATE = 3; // Parada del bloque actual
                    return;
                }
            }            

          // Escribimos el tren de pulsos en el procesador de Audio
          m_kit.write(buffer, createWave(buffer, bytes));
        } 
    }

    void generateWavePulsesEdge(float freq, int numPulses, int samplingRate,int edge)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / 4.0))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];


        for (int m=0;m < numPulses;m++)
        {
            if (LOADING_STATE == 1)
            {
                if (STOP==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    return;
                }
                else if (PAUSE==true)
                {
                    LOADING_STATE = 3; // Parada del bloque actual
                    return;
                }
            }            

          // Escribimos el tren de pulsos en el procesador de Audio
          m_kit.write(buffer, createWaveEdge(buffer, bytes,edge));
        } 
    }    

    void generateWaveDuration(float freq, float duration, int samplingRate)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int((1.0 / ((freq / 4.0))) / Tsr);
        int numPulses = 4 * int(duration / (bytes*Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];      

        for (int m=0;m < numPulses;m++)
        {
            
            if (LOADING_STATE == 1)
            {
                if (STOP==true)
                {
                    LOADING_STATE = 2; // Parada del bloque actual
                    return;
                }
                else if (PAUSE==true)
                {
                    LOADING_STATE = 3; // Parada del bloque actual
                    return;
                }
            }

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
        //float Td = 4 * (duration / 1000);
        float Td = 4 * (duration / 1000);
        float Tsr = (1.0 / samplingRate);
        int samples = int(Td / Tsr);
        int chn = channels;

        // Inicializamos con un tamaño de bloque de 256 muestras cada vez
        // NOTA: Si esto es muy grande PETA EL ESP32
        int bufferSize = 256;

        // Calculamos cuantos bloques tenemos que concatenar. Si el valor de
        // samples es menor, saldrá 0
        int frames = samples / (bufferSize * chn);
        int delta = abs(samples - (bufferSize * frames * chn));

        // Si es cero, entonces el buffer será igual de grande que el 
        // numero de samples a rellenar para formar la onda
        if (frames == 0)
        {
            bufferSize = samples;
            frames = 1;
        }
    
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
            }
  
            // Aplicamos la reserva de buffer
            uint8_t buffer[bufferSize*chn]; 
            m_kit.write(buffer, silenceWave(buffer, bufferSize));
        }
    }

    void silenceEdge(float duration)
    {
        // Esta onda se genera como el resto sumando trozos de onda
        // esto es debido al limite del buffer
        // no podemos hacer un buffer muy grande, peta el ESP32

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        //float Td = 4 * (duration / 1000);
        float Td = 4 * (duration / 1000);
        float Tsr = (1.0 / samplingRate);
        int samples = int(Td / Tsr);
        int chn = channels;

        // Inicializamos con un tamaño de bloque de 256 muestras cada vez
        // NOTA: Si esto es muy grande PETA EL ESP32
        int bufferSize = 256;

        // Calculamos cuantos bloques tenemos que concatenar. Si el valor de
        // samples es menor, saldrá 0
        int frames = samples / (bufferSize * chn);
        int delta = abs(samples - (bufferSize * frames * chn));

        // Si es cero, entonces el buffer será igual de grande que el 
        // numero de samples a rellenar para formar la onda
        if (frames == 0)
        {
            bufferSize = samples;
            frames = 1;
        }
    
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
            }
  
            // Aplicamos la reserva de buffer
            uint8_t buffer[bufferSize*chn]; 
            m_kit.write(buffer, silenceWaveEdge(buffer, bufferSize));
        }
    }

    void customPilotTone_old(int lenPulse, int numPulses)
    {
        // Calculamos la frecuencia del tono guía.
        // Hay que tener en cuenta que los T-States dados son de un SEMI-PULSO
        // es decir de la mitad del periodo. Entonces hay que calcular
        // el periodo completo que es 2 * T
        float freq = (1 / (lenPulse * tState)) / 2;   
        generateWavePulses(freq, numPulses, samplingRate);
    }

    void customPilotTone(int lenPulse, int numPulses)
    {
        //
        // Esto se usa para el ID 0x13 del TZX
        //
        
        // Iniciamos el flanco dependiendo de como fuera el ultimo
        int slope = LAST_EDGE_IS;

        // Repetimos el número de pulsos solicitados con un ancho "lenPulse"
        for (int i = 0; i < numPulses;i++)
        {
            // Cambiamos slope de 0 a 1, para indicar si es 
            // pulso alto o bajo
            if (LAST_EDGE_IS!=1)
            {
                slope=1;
            }
            else
            {
                slope=-1;
            }

            semiPulse(lenPulse,slope);
            LAST_EDGE_IS = slope;          
        }
        //log("Flanco: " + String(LAST_EDGE_IS));       

    }

    void playCustomSequence(int* data, int numPulses)
    {
        //
        // Esto lo usamos para el PULSE_SEQUENCE ID-13
        //

        // Reproduce una secuencia de pulsos totalmente customizada
        // cada pulso tiene su timming y viene dado en un array (data)
        int slope = LAST_EDGE_IS;

        for (int i = 0; i < numPulses;i++)
        {
            // Cambiamos slope de 0 a 1, para indicar si es 
            // pulso alto o bajo
            if (LAST_EDGE_IS!=1)
            {
                slope=1;
            }
            else
            {
                slope=-1;
            }

            semiPulse(data[i],slope);
            LAST_EDGE_IS = slope;   
        }
        //log("Flanco: " + String(LAST_EDGE_IS));       

    }

    // void playCustomSequence_old(int* data, int numPulses)
    // {
    //     // Reproduce una secuencia de pulsos totalmente customizada
    //     // cada pulso tiene su timming y viene dado en un array (data)
    //     int slope = 0;

    //     for (int i = 0; i < numPulses;i++)
    //     {
    //         // Cambiamos slope de 0 a 1, para indicar si es 
    //         // pulso alto o bajo
    //         slope = 1 - (i % 2);
    //         syncTone(data[i],slope);            
    //     }

    //     // Metemos un pulso de cambio de estado
    //     // para asegurar el cambio de flanco alto->bajo, del elemento de la secuencia

    //     // if (slope == 1)
    //     // {
    //     //     SerialHW.println("");
    //     //     SerialHW.println("End edge: HIGH");
    //     //     LAST_EDGE_IS = 1;
    //     // }
    //     // else
    //     // {
    //     //     SerialHW.println("");
    //     //     SerialHW.println("End edge: LOW");
    //     //     LAST_EDGE_IS = 0;
    //     // }
    // }

    void semiPulse(int nTStates, int slope)
    {
        // Procedimiento que genera un pulso de sincronismo, según los
        // T-States pasados por parámetro.
        //
        // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
        // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
        float freq = (1 / (nTStates * tState));   
        
        generatePulse(freq, samplingRate,slope, false);        
    }

    void pilotTone(float duration)
    {
        // Calculamos la frecuencia del tono guía.
        // Hay que tener en cuenta que los T-States dados son de un SEMI-PULSO
        // es decir de la mitad del periodo. Entonces hay que calcular
        // el periodo completo que es 2 * T
        float freq = (1 / (PULSE_PILOT * tState)) / 2;   
        generateWaveDuration(freq, duration, samplingRate);
    }

    void zeroToneEdge()
    {
        // Procedimiento que genera un bit "0"
        float freq = (1 / (BIT_0 * tState)) / 2;        
        generateOneWaveEdge(freq, samplingRate,LAST_EDGE_IS);
    }

    void oneToneEdge()
    {
        // Procedimiento que genera un bit "1"
        float freq = (1 / (BIT_1 * tState)) / 2;        
        generateOneWaveEdge(freq, samplingRate,LAST_EDGE_IS);
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

    void sendDataArray(uint8_t* data, int size)
    {
        uint8_t _mask = 8;   // Para el last_byte

        // Procedimiento para enviar datos desde un array
        if (LOADING_STATE==1 || TEST_RUNNING)
        {
            uint8_t bRead = 0x00;
            int bytes_in_this_block = 0;

            for (int i = 0; i < size;i++)
            {
            
              if (!TEST_RUNNING)
              {
                // Informacion para la barra de progreso
                PROGRESS_BAR_BLOCK_VALUE = (int)(((i+1)*100)/(size));

                if (BYTES_LOADED > BYTES_TOBE_LOAD)
                {BYTES_LOADED = BYTES_TOBE_LOAD;}
                // Informacion para la barra de progreso total
                PROGRESS_BAR_TOTAL_VALUE = (int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD));
                
                if (LOADING_STATE == 1)
                {
                    if (STOP==true)
                    {
                        LOADING_STATE = 2; // Parada del bloque actual
                        i=size;
                        //log("Aqui he parado - STOP");
                        return;
                    }
                    else if (PAUSE==true)
                    {
                        LOADING_STATE = 3; // Parada del bloque actual
                        i=size;
                        //log("Aqui he parado - PAUSA");
                        return;
                    }

                }
              }


              if (LOADING_STATE == 1 || TEST_RUNNING)
              {
                  // Vamos a ir leyendo los bytes y generando el sonido
                  bRead = data[i];
                  
                  // Para la protección con mascara ID11 - 0x0C
                  // "Used bits in the last uint8_t (other bits should be 0) {8}
                  //(e.g. if this is 6, then the bits used (x) in the last uint8_t are: xxxxxx00, wh///ere MSb is the leftmost bit, LSb is the rightmost bit)"
                  
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

            // Esto lo hacemos para asegurarnos que la barra se llena entera
            // _hmi.writeString("progression.val=100");

            // if (BYTES_LOADED > BYTES_TOBE_LOAD)
            // {BYTES_LOADED = BYTES_TOBE_LOAD;}

            // _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
            // //_hmi.updateInformationMainPage();                   

            int width = 0;
            // Leemos el ultimo bit (del ultimo uint8_t), y dependiendo de como sea
            // así cerramos el flanco.
            // Cogemos el ultimo uint8_t
            bRead = data[size-1];

            // Vemos como es el último bit MSB es la posición 0, el ultimo bit
            if (bitRead(bRead, 0) == 1)
            {
                width = BIT_1;
            }
            else
            {
                width = BIT_0;
            }
            
            // Metemos un pulso de cambio de estado
            // para asegurar el cambio de flanco alto->bajo, del ultimo bit
            float freq = (1 / (width * tState));    
            generatePulse(freq, samplingRate,1,true);

            // El ultimo flanco siempre es HIGH
            LAST_EDGE_IS = 1;
        }
    }
    
    void sendDataArrayEdge(uint8_t* data, int size)
    {
        uint8_t _mask = 8;   // Para el last_byte

        // Procedimiento para enviar datos desde un array.
        // si estamos reproduciendo, nos mantenemos.
        if (LOADING_STATE==1 || TEST_RUNNING)
        {
            uint8_t bRead = 0x00;
            int bytes_in_this_block = 0;

            // Recorremos todo el vector de bytes leidos para reproducirlos
            for (int i = 0; i < size;i++)
            {
            
                if (!TEST_RUNNING)
                {
                    // Informacion para la barra de progreso
                    PROGRESS_BAR_BLOCK_VALUE = (int)(((i+1)*100)/(size));

                    if (BYTES_LOADED > BYTES_TOBE_LOAD)
                    {BYTES_LOADED = BYTES_TOBE_LOAD;}
                    // Informacion para la barra de progreso total
                    PROGRESS_BAR_TOTAL_VALUE = (int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD));
                    
                    if (LOADING_STATE == 1)
                    {
                        if (STOP==true)
                        {
                            LOADING_STATE = 2; // Parada del bloque actual
                            i=size;
                            //log("Aqui he parado - STOP");
                            return;
                        }
                        else if (PAUSE==true)
                        {
                            LOADING_STATE = 3; // Parada del bloque actual
                            i=size;
                            //log("Aqui he parado - PAUSA");
                            return;
                        }

                    }
                }


                // Vamos a ir leyendo los bytes y generando el sonido
                bRead = data[i];
                
                // Para la protección con mascara ID 0x11 - 0x0C
                // ---------------------------------------------
                // "Used bits in the last uint8_t (other bits should be 0) {8}
                //(e.g. if this is 6, then the bits used (x) in the last uint8_t are: xxxxxx00, wh///ere MSb is the leftmost bit, LSb is the rightmost bit)"
                
                // ¿Es el ultimo BYTE?. Si se ha aplicado mascara entonces
                // se modifica el numero de bits a transmitir

                if (LOADING_STATE==1 || TEST_RUNNING)
                {
                    if (i == size-1)
                    {
                        // Aplicamos la mascara
                        _mask = _mask_last_byte;
                    }
                    else
                    {
                        // En otro caso todo el byte es valido.
                        // se anula la mascara.
                        _mask = 8;
                    }
                
                    // Ahora vamos a descomponer el byte leido
                    // si hay mascara entonces solo se leerán n bits de los 8 que componen un BYTE.
                    for (int n=0;n < _mask;n++)
                    {

                        // Si el bit leido del BYTE es un "1"
                        if(bitRead(bRead, 7-n) == 1)
                        {
                            // Procesamos "1"
                            oneToneEdge();
                        }
                        else
                        {
                            // En otro caso
                            // procesamos "0"
                            zeroToneEdge();
                        }
                    }

                    // Hemos cargado +1 byte. Seguimos
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

            
            // Esto lo hacemos para asegurarnos que la barra se llena entera
            // _hmi.writeString("progression.val=100");

            // if (BYTES_LOADED > BYTES_TOBE_LOAD)
            // {BYTES_LOADED = BYTES_TOBE_LOAD;}

            // _hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
            // //_hmi.updateInformationMainPage();                   

            // ****************
            // ****************        TERMINADOR
            // ****************

            int width = 0;
            // Leemos el ultimo bit (del ultimo uint8_t), y dependiendo de como sea
            // así cerramos el flanco.
            // Cogemos el ultimo uint8_t
            bRead = data[size-1];

            // Vemos como es el último bit MSB es la posición 0, el ultimo bit
            if (bitRead(bRead, 0) == 1)
            {
                width = BIT_1;
            }
            else
            {
                width = BIT_0;
            }
            
            // Metemos un pulso de cambio de estado
            // para asegurar el cambio de flanco alto->bajo, del ultimo bit
            float freq = (1 / (width * tState));    
            generatePulse(freq, samplingRate,1,true);

            // El ultimo flanco siempre es HIGH
            LAST_EDGE_IS = 1;    

        }
    }
   

    public:

        void set_maskLastByte(uint8_t mask)
        {
            _mask_last_byte = mask;
        }

        void playData(uint8_t* bBlock, int lenBlock, int pulse_pilot_duration)
        {
            // Inicializamos el estado de la progress bar de bloque.
            PROGRESS_BAR_BLOCK_VALUE = 0;

            float duration = tState * pulse_pilot_duration;

            // Put now code block
            // syncronize with short leader tone
            pilotTone(duration);
            //log("Pilot tone");
            if (LOADING_STATE == 2)
            {return;}

            // syncronization for end short leader tone
            syncTone(SYNC1,1);
            //log("SYNC 1");
            if (LOADING_STATE == 2)
            {return;}

            syncTone(SYNC2,0);
            //log("SYNC 2");
            if (LOADING_STATE == 2)
            {return;}

            // Send data
            sendDataArray(bBlock, lenBlock);
            //log("Send DATA");
            if (LOADING_STATE == 2)
            {return;}
                        
            // Silent tone
            silence(silent);

            //log("Send SILENCE");
            if (LOADING_STATE == 2)
            {return;}
            
            //log("Fin del PLAY");
        }

        void playPureTone(int lenPulse, int numPulses)
        {
            // Put now code block
            // syncronize with short leader tone
            customPilotTone(lenPulse, numPulses);          
        }

        void playPureData(uint8_t* bBlock, int lenBlock)
        {

            // Send data
            sendDataArrayEdge(bBlock, lenBlock);

            //log("Send DATA");
            if (LOADING_STATE == 2)
            {
                return;
            }
                        
            // Silent tone
            silenceEdge(silent);

            //log("Send SILENCE");
            if (LOADING_STATE == 2)
            {
                return;
            }
            
            //log("Fin del PLAY");
        }        

        void playDataBegin(uint8_t* bBlock, int lenBlock, int pulse_pilot_duration)
        {
            // PROGRAM
            float duration = tState * pulse_pilot_duration;
            // syncronize with short leader tone
            pilotTone(duration);
            // syncronization for end short leader tone
            syncTone(SYNC1,1);
            syncTone(SYNC2,0);

            // Send data
            sendDataArray(bBlock, lenBlock);
                   
        }

        void playDataEnd(uint8_t* bBlock, int lenBlock, int pulse_pilot_duration)
        {

            float duration = tState * pulse_pilot_duration;
            // Send data
            sendDataArray(bBlock, lenBlock);        
            
            // Silent tone
            // if (silent!=0)
            // {
                silence(silent);
            // }
            
        }

        void playBlock(uint8_t* header, int len_header, uint8_t* data, int len_data, int pulse_pilot_duration_header, int pulse_pilot_duration_data)
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
            syncTone(SYNC2,0);

            sendDataArray(header, len_header);

            // Silent tone
            // if (silent!=0)
            // {
                silence(silent);
            // }
            

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
            
            // Silent tone
            // if (silent!=0)
            // {
                silence(silent);
            // }
            
        }

        void playHeaderOnly(uint8_t* header, int len_header, int pulse_pilot_duration)
        {           

            float duration = tState * pulse_pilot_duration;
            //
            // PROGRAM
            //HEADER PILOT TONE
            pilotTone(duration);
            // SYNC TONE
            syncTone(SYNC1,1);
            syncTone(SYNC2,0);

            sendDataArray(header, len_header);

            // Silent tone
            // if (silent!=0)
            // {
                silence(silent);
            // }
            
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
        ZXProcessor()
        {
          // Constructor de la clase
        }

};
