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
    
    // Estos valores definen las señales. Otros para el flanco negativo
    // provocan problemas de lectura en el Spectrum.
    const int maxAmplitude = 32767;
    const int minAmplitude = 0;

    float m_amplitude_L = maxAmplitude * MAIN_VOL_L / 100; 
    float m_amplitude_R = maxAmplitude * MAIN_VOL_R / 100; 

    // Al poner 2 canales - Falla. Solucionar
    const int channels = 2;  
    const float speakerOutPower = 0.002;

    public:
    // Parametrizado para el ZX Spectrum - Timming de la ROM
    // Esto es un factor para el calculo de la duración de onda
    float alpha = 4.0;
    // Este es un factor de división para la amplitud del flanco terminador
    float amplitudeFactor = 1.0;
    // T-states del ancho del terminador
    int maxTerminatorWidth = 25000; //200
    // otros parámetros
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

    size_t silenceWave(uint8_t *buffer, size_t samples, int amplitude)
    {
        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        
        for (int j=0;j<(samples/(2*chn));j++)
        {
            m_amplitude_R = MAIN_VOL_R * amplitude / 100;
            m_amplitude_L = MAIN_VOL_L * amplitude / 100;

            int16_t sample_R = m_amplitude_R;
            int16_t sample_L = m_amplitude_L;

            if (!SWAP_EAR_CHANNEL)
            {
              //L-OUT
              *ptr++ = sample_R;
              //R-OUT
              *ptr++ = sample_L * (EN_MUTE);
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

    size_t createWave(uint8_t *buffer, size_t bytes)
    {
        
        // Procedimiento para generar un tren de pulsos cuadrados completo
        // Antes de iniciar la reproducción ajustamos el volumen de carga.

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        int A = maxAmplitude;
     
        if (LAST_EDGE_IS==up)
        {
            A = minAmplitude;
        }
        else
        {
            A = maxAmplitude;
        }

        // Pulso alto (mitad del periodo)
        for (int j=0;j<bytes/(4*chn);j++){

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

        if (LAST_EDGE_IS==up)
        {
            A = maxAmplitude;
            LAST_EDGE_IS = up;
        }
        else
        {
            A = minAmplitude;
            LAST_EDGE_IS = down;
        }

        // Pulso bajo (la otra mitad)
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

    size_t makeWaveEdge(uint8_t *buffer, size_t bytes, edge lastSlope)
    {
        
        // Procedimiento para generar un tren de pulsos cuadrados completo
        // Antes de iniciar la reproducción ajustamos el volumen de carga.

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;
        int A = maxAmplitude;

        
        if (lastSlope==down)
        {
            // Si el ultimo edge ha quedado en bajo entonces genera un alto
            A=maxAmplitude;
        }
        else
        {
            // Lo contrario
            A=minAmplitude;
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

        // El siguiente semi-pulso es contrario al anterior
        if (lastSlope==down)
        {
            A=minAmplitude;
        }
        else
        {
            A=maxAmplitude;
        }

        // Pulso bajo (la otra mitad) es invertida
        for (int j=bytes/(4*chn);j<bytes/(2*chn);j++)
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

        // slope tomará los valores 1 (el ultimo flanco va de alto --> bajo) o distinto de 1 (el ultimo flanco va de bajo --> alto)
        LAST_EDGE_IS = lastSlope;

        return result;
    }

    size_t makeSampledSemiPulse(uint8_t *buffer, size_t bytes, edge thisSlopeIs)
    {

        // Procedimiento para genera un pulso 

        int chn = channels;
        size_t result = 0;
        int16_t *ptr = (int16_t*)buffer;

        int16_t sample_L = 0;
        int16_t sample_R = 0;

        if (thisSlopeIs==up)
        {
            // Hacemos el edge de down --> up
            m_amplitude_R = MAIN_VOL_R * maxAmplitude / 100;
            m_amplitude_L = MAIN_VOL_L * maxAmplitude / 100;
        }
        else
        {
            // Hacemos el edge de up --> down
            m_amplitude_R = MAIN_VOL_R * minAmplitude / 100;
            m_amplitude_L = MAIN_VOL_L * minAmplitude / 100;
        }

        //log("Sample: " + String(m_amplitude_L) + " - " + String(m_amplitude_R));

        sample_R = m_amplitude_R;
        sample_L = m_amplitude_L; 


        for (int j=0;j<bytes/(2*chn);j++)
        {

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

    void generatePulse(float freq, int samplingRate, edge lastSlope)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / alpha))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];

        // Escribimos el tren de pulsos en el procesador de Audio
        m_kit.write(buffer, makeSampledSemiPulse(buffer, bytes, lastSlope));
    }

    void generateOneWave(float freq, int samplingRate)
    {
        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate

        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / alpha))) / Tsr));
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

    void generateOneWaveEdge(float freq, int samplingRate, edge slope)
    {
        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate

        float Tsr = (1.0 / samplingRate);
        int bytes = int(round((1.0 / ((freq / alpha))) / Tsr));
        int chn = channels;

        uint8_t buffer[bytes*chn];

        m_kit.write(buffer, makeWaveEdge(buffer, bytes,slope));
                
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

    void generateWaveDuration(float freq, float duration, int samplingRate)
    {

        // Obtenemos el periodo de muestreo
        // Tsr = 1 / samplingRate
        float Tsr = (1.0 / samplingRate);
        int bytes = int((1.0 / ((freq / alpha))) / Tsr);
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

    void terminator(edge slope)
    {
        int width = maxTerminatorWidth;

        // Vemos como es el último bit MSB es la posición 0, el ultimo bit
        
        // Metemos un pulso de cambio de estado
        // para asegurar el cambio de flanco alto->bajo, del ultimo bit
        float freq = (1 / (width * tState));  
        generatePulse(freq, samplingRate,slope);
    }

    void silence(float duration)
    {
        // Paso la duración a T-States
        edge edgeSelected = down;
        if (duration>0)
        {
            int tStateSilence = (duration/1000) / (1/freqCPU);       
            int minSilenceFrame = 5000;

            int parts = 0;
            int lastPart = 0;

            // Esto lo hacemos para acabar bien un ultimo flanco en down.
            edgeSelected = down;
            if (LAST_EDGE_IS==down)
            {
                //terminator(up);
                edgeSelected = up;
            }
            else
            {
                edgeSelected = down;
            }

            if (tStateSilence > minSilenceFrame)
            {
                parts = tStateSilence / minSilenceFrame;
                lastPart = tStateSilence - (parts * minSilenceFrame);

                // Lo partimos
                for (int n=0;n<parts;n++)
                {
                    semiPulse(minSilenceFrame, edgeSelected);
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
                }
            }
            else
            {
                lastPart = tStateSilence;
            }

            semiPulse(lastPart, edgeSelected);

            LAST_EDGE_IS = edgeSelected;
        }
        else
        {
            if (LAST_EDGE_IS==down)
            {
                terminator(up);
                edgeSelected = down;
            }
            else
            {
                terminator(down);
                edgeSelected = up;
            }
            
            LAST_EDGE_IS = edgeSelected;
        }
       
    }

    void customPilotTone(int lenPulse, int numPulses)
    {
        //
        // Esto se usa para el ID 0x13 del TZX
        //
        
        // Iniciamos el flanco dependiendo de como fuera el ultimo
        // slope tomará los valores 1 (el ultimo flanco va de alto --> bajo) o distinto de 1 (el ultimo flanco va de bajo --> alto)        
        edge slope = up;

        // Cogemos el flanco del LAST_EDGE_IS para el primero
        if (LAST_EDGE_IS==up)
        {
            slope = down;
        }
        else
        {
            slope = up;            
        }

        // Repetimos el número de pulsos solicitados con un ancho "lenPulse"
        //log("Pure tone " + String(numPulses) + "/ len: " + String(lenPulse));

        for (int i = 0; i < numPulses;i++)
        {
            semiPulse(lenPulse,slope);

            //Vamos cambiando
            if (slope==up)
            {
                slope=down;
            }
            else
            {
                slope=up;
            }
        }

        // Guardamos el ultimo estado
        LAST_EDGE_IS = slope;
    }

    void semiPulse(int nTStates, edge slope)
    {
        // Procedimiento que genera un pulso de sincronismo, según los
        // T-States pasados por parámetro.
        //
        // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
        // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
        float freq = (1 / (nTStates * tState));   
        generatePulse(freq, samplingRate,slope);   
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
        //log("Valor de BIT0: " + String(BIT_0));
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

    void syncTone(int nTStates)
    {
        // Procedimiento que genera un pulso de sincronismo, según los
        // T-States pasados por parámetro.
        //
        // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
        // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
        float freq = (1 / (nTStates * tState));  
        
        edge slope = up;

        if (LAST_EDGE_IS==up)
        {
            slope=down;
        }
        else
        {
            slope=up;
        }

        generatePulse(freq, samplingRate,slope);        

        LAST_EDGE_IS = slope;
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

                        return;
                    }
                    else if (PAUSE==true)
                    {
                        LOADING_STATE = 3; // Parada del bloque actual
                        i=size;

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
            if (BYTES_LOADED > BYTES_TOBE_LOAD)
            {BYTES_LOADED = BYTES_TOBE_LOAD;}               


            // ********************* TERMINADOR *********************************

            //terminator();
        }
    }
    
    void sendDataArrayEdge(uint8_t* data, int size, bool isThelastBlock)
    {
        uint8_t _mask = 8;   // Para el last_byte
        uint8_t bRead = 0x00;
        int bytes_in_this_block = 0;        

        // Procedimiento para enviar datos desde un array.
        // si estamos reproduciendo, nos mantenemos.
        if (LOADING_STATE==1 || TEST_RUNNING)
        {

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
                    else
                    {
                        //log("Me lo he saltado 1");
                    }
                }


                // Vamos a ir leyendo los bytes y generando el sonido
                bRead = data[i];
                //log("Dato: " + String(i) + " - " + String(data[i]));

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
                        //log("Mascara: " + String(_mask) + " - Dato: [" + String(i) + "] - " + String(data[i]));
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
                    //log("He salido en: " + String(i));
                    //break // -> 27/12/2023
                    return;
                }
            }


            
            // Esto lo hacemos para asegurarnos que la barra se llena entera
            //_hmi.writeString("progression.val=100");

            if (BYTES_LOADED > BYTES_TOBE_LOAD)
            {BYTES_LOADED = BYTES_TOBE_LOAD;}

            //_hmi.writeString("progressTotal.val=" + String((int)((BYTES_LOADED*100)/(BYTES_TOBE_LOAD))));
            // //_hmi.updateInformationMainPage();                   

            if (isThelastBlock)
            {
                // ****************
                // ****************        TERMINADOR
                // ****************

                // Metemos un pulse de finalización, según acabe el ultimo flanco
                //uint8_t bit = data[size-1];
                //terminator();
            }
        }
        else
        {
            //log("Me lo he saltado 2");
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
            syncTone(SYNC1);
            //log("SYNC 1");
            if (LOADING_STATE == 2)
            {return;}

            syncTone(SYNC2);
            //log("SYNC 2");
            if (LOADING_STATE == 2)
            {return;}

            // Send data
            //sendDataArray(bBlock, lenBlock);
            sendDataArrayEdge(bBlock, lenBlock,true);
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

        void playCustomSequence(int* data, int numPulses)
        {
            //log("BYTE: " + String(BYTES_LOADED) + " - sample: 2SM - " + String(j) + " - R: " + String(sample_R) + " L: "+ String(sample_L));
            //
            // Esto lo usamos para el PULSE_SEQUENCE ID-13
            //

            // Iniciamos el flanco dependiendo de como fuera el ultimo
            // slope tomará los valores 1 (el ultimo flanco va de alto --> bajo) o distinto de 1 (el ultimo flanco va de bajo --> alto)        
            edge slope = up;

            if (LAST_EDGE_IS==up)
            {
                slope = down;
            }
            else
            {
                slope = up;
            }

            //log("------  Start PULSE SQZ");

            for (int i = 0; i < numPulses;i++)
            {
                // Vamos cambiando
                if (slope==up)
                {
                    slope=down;
                }
                else
                {
                    slope=up;
                }                

                // Generamos los semipulsos
                semiPulse(data[i],slope);
            }

            LAST_EDGE_IS = slope;
            
            //log("Flanco: " + String(LAST_EDGE_IS));     
            //log("++++++  End PULSE SQZ");  
        }   

        void playPureData(uint8_t* bBlock, int lenBlock)
        {
            // Se usa para reproducir los datos del ultimo bloque o bloque completo sin particionar, ID 0x14.
            // por tanto deben meter un terminador al final.
            sendDataArrayEdge(bBlock, lenBlock,true);

            if (LOADING_STATE == 2)
            {
                return;
            }

            // Ahora enviamos el silencio, si aplica.
            if (silent!=0)
            {
                // Silent tone
                silence(silent);

                if (LOADING_STATE == 2)
                {
                    return;
                }
            }
        }        

        void playPureDataPartition(uint8_t* bBlock, int lenBlock)
        {
            // Se envia a reproducir el bloque sin terminador ni silencio
            // ya que es parte de un bloque completo que ha sido partido en varios
            sendDataArrayEdge(bBlock, lenBlock, false);

            if (LOADING_STATE == 2)
            {
                return;
            }
        }        

        void playDataPartition(uint8_t* bBlock, int lenBlock)
        {
            // Se envia a reproducir el bloque sin terminador ni silencio
            // ya que es parte de un bloque completo que ha sido partido en varios
            sendDataArrayEdge(bBlock, lenBlock, false);

            if (LOADING_STATE == 2)
            {
                return;
            }
        } 

        void playDataBegin(uint8_t* bBlock, int lenBlock, int pulse_pilot_duration)
        {
            // PROGRAM
            float duration = tState * pulse_pilot_duration;
            // syncronize with short leader tone
            pilotTone(duration);
            // syncronization for end short leader tone
            syncTone(SYNC1);
            syncTone(SYNC2);

            // Send data
            sendDataArrayEdge(bBlock, lenBlock,false);
                   
        }

        void playDataEnd(uint8_t* bBlock, int lenBlock)
        {
            // Send data
            sendDataArrayEdge(bBlock, lenBlock,true);        
            
            // Silent tone
            silence(silent);
        }

        void playBlock(uint8_t* header, int len_header, uint8_t* data, int len_data, int pulse_pilot_duration_header, int pulse_pilot_duration_data)
        {           
            #if LOG==3
              //SerialHW.println("******* PROGRAM HEADER");
              //SerialHW.println("*******  - HEADER size " + String(len_header));
              //SerialHW.println("*******  - DATA   size " + String(len_data));
            #endif

            float durationHeader = tState * pulse_pilot_duration_header;
            float durationData = tState * pulse_pilot_duration_data;

            // PROGRAM
            //HEADER PILOT TONE
            pilotTone(durationHeader);
            // SYNC TONE
            syncTone(SYNC1);
            syncTone(SYNC2);

            sendDataArrayEdge(header, len_header,true);
            //sendDataArray(header, len_header);

            // Silent tone
            silence(silent);           

            #if LOG==3
              //SerialHW.println("******* PROGRAM DATA");
            #endif

            // Put now code block
            // syncronize with short leader tone
            pilotTone(durationData);
            // syncronization for end short leader tone
            syncTone(SYNC1);
            syncTone(SYNC2);

            // Send data
            sendDataArrayEdge(data, len_data, true);       
            
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
        ZXProcessor()
        {
          // Constructor de la clase
        }

};
