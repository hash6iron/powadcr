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
    //const float samplingRate = 44099.988;
    //const float samplingRate = 44100.0;
    //const float samplingRate = 48000.0;
    const float samplingRate = 32000.0;
    //const float sampleDuration = (1.0 / samplingRate); //0.0000002267; //
                                                       // segundos para 44.1HKz
    
    // Estos valores definen las señales. Otros para el flanco negativo
    // provocan problemas de lectura en el Spectrum.
    float maxAmplitude = LEVELUP;
    float minAmplitude = LEVELDOWN;

    float m_amplitude_L = MAIN_VOL_L * maxAmplitude; 
    float m_amplitude_R = MAIN_VOL_R * maxAmplitude; 

    // Al poner 2 canales - Falla. Solucionar
    const int channels = 2;  
    //const float speakerOutPower = 0.002;

    public:
    // Parametrizado para el ZX Spectrum - Timming de la ROM
    // Esto es un factor para el calculo de la duración de onda
    float alpha = 4.0;
    // Este es un factor de división para la amplitud del flanco terminador
    float amplitudeFactor = 1.0;
    // T-states del ancho del terminador
    int maxTerminatorWidth = 3500; //Minimo debe ser 1ms
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

        // edge invPulse(edge currentEdge)
        // {
        //     if (currentEdge==up)
        //     {
        //         return down;
        //     }
        //     else
        //     {
        //         return up;
        //     }
        // }

        // edge getPolarizeEdge(edge currentEdge)
        // {
        //     if (currentEdge==up)
        //     {
        //         if(INVERSETRAIN)
        //         {
        //             return down;
        //         }
        //         else
        //         {
        //             return up;
        //         }
        //     }
        //     else
        //     {
        //         if(INVERSETRAIN)
        //         {
        //             return up;
        //         }
        //         else
        //         {
        //             return down;
        //         }
        //     }
        // }

        // void getLowLimitSigal()
        // {
        //     if (ZEROLEVEL)
        //     {
        //         minAmplitude = 0;
        //         maxAmplitude = LEVELUP;
        //     }
        //     else
        //     {
        //         minAmplitude = LEVELDOWN;
        //         maxAmplitude = LEVELUP;
        //     }
        // }

        // float getVolumenSwap(bool R_Channel)
        // {
        //     if (SWAP_EAR_CHANNEL)
        //     {
        //         if(R_Channel)
        //         {
        //             //Entonces ahora es el Left
        //             return (MAIN_VOL_L / MAIN_VOL_FACTOR);
        //         }
        //         else
        //         {
        //             return (MAIN_VOL_R / MAIN_VOL_FACTOR);
        //         }
        //     }
        //     else
        //     {
        //         if(R_Channel)
        //         {
        //             //Entonces es el suyo R
        //             return (MAIN_VOL_R / MAIN_VOL_FACTOR);
        //         }
        //         else
        //         {
        //             return (MAIN_VOL_L / MAIN_VOL_FACTOR);
        //         }
        //     }
        // }

        // size_t makeSemiPulse_ori(uint8_t *buffer, size_t bytes, edge thisSlopeIs, bool changeNextEdge)
        // {

        //     // Procedimiento para genera un pulso 

        //     int chn = channels;
        //     size_t result = 0;
        //     int16_t *ptr = (int16_t*)buffer;

        //     int16_t sample_L = 0;
        //     int16_t sample_R = 0;
            
        //     int A = 0;

        //     getLowLimitSigal();

        //     if (thisSlopeIs==getPolarizeEdge(down))
        //     {
        //         // Hacemos el edge de down --> up
        //         if (!INVERSETRAIN)
        //         {
        //             A=maxAmplitude;

        //             if (changeNextEdge)
        //             {
        //                 LAST_EDGE_IS = up;
        //             }
        //             else
        //             {
        //                 LAST_EDGE_IS = down;
        //             }

        //         }
        //         else
        //         {
        //             A=minAmplitude;

        //             if (changeNextEdge)
        //             {
        //                 LAST_EDGE_IS = down;
        //             }
        //             else
        //             {
        //                 LAST_EDGE_IS = up;
        //             }
        //         }
                
        //         m_amplitude_R = getVolumenSwap(true) * A;
        //         m_amplitude_L = getVolumenSwap(false) * A;
        //     }
        //     else
        //     {
        //         // Hacemos el edge de up --> down
        //         if (!INVERSETRAIN)
        //         {
        //             A=minAmplitude;
                    
        //             if (changeNextEdge)
        //             {
        //                 LAST_EDGE_IS = down;
        //             }
        //             else
        //             {
        //                 LAST_EDGE_IS = up;
        //             }
        //         }
        //         else
        //         {
        //             A=maxAmplitude;
                    
        //             if (changeNextEdge)
        //             {
        //                 LAST_EDGE_IS = up;
        //             }
        //             else
        //             {
        //                 LAST_EDGE_IS = down;
        //             }
        //         }            
        //         m_amplitude_R = getVolumenSwap(true) * A;
        //         m_amplitude_L = getVolumenSwap(false) * A;
        //     }

        //     //log("Sample: " + String(m_amplitude_L) + " - " + String(m_amplitude_R));

        //     sample_R = m_amplitude_R;
        //     sample_L = m_amplitude_L; 

        //     DEBUG_AMP_R = sample_R;
        //     DEBUG_AMP_L = sample_L;

        //     for (int j=0;j<bytes/(2*chn);j++)
        //     {

        //         //R-OUT
        //         *ptr++ = sample_R;
        //         //L-OUT
        //         *ptr++ = sample_L * EN_STEREO;

        //         result+=2*chn;
        //     }

        //     return result;          
        // }

        size_t makeSemiPulse(uint8_t *buffer, size_t bytes, edge thisSlopeIs, bool changeNextEdge)
        {

            // Procedimiento para genera un pulso 

            int chn = channels;
            size_t result = 0;
            int16_t *ptr = (int16_t*)buffer;

            int16_t sample_L = 0;
            int16_t sample_R = 0;
            
            int A = 0;

            //getLowLimitSigal();

            if (thisSlopeIs==down)
            {
                // Hacemos el edge de down --> up
                A=maxAmplitude;

                if (changeNextEdge)
                {
                    LAST_EDGE_IS = up;
                }
                else
                {
                    LAST_EDGE_IS = down;
                }
            }
            else
            {
                A=minAmplitude;

                if (changeNextEdge)
                {
                    LAST_EDGE_IS = down;
                }
                else
                {
                    LAST_EDGE_IS = up;
                }
            }

            // Asignamos el nivel de amplitud que le corresponde ahora                
            m_amplitude_R = A;
            m_amplitude_L = A;
            
            //log("Sample: " + String(m_amplitude_L) + " - " + String(m_amplitude_R));

            sample_R = m_amplitude_R;
            sample_L = m_amplitude_L; 

            DEBUG_AMP_R = sample_R;
            DEBUG_AMP_L = sample_L;

            for (int j=0;j<bytes/(2*chn);j++)
            {

                //R-OUT
                *ptr++ = sample_R;
                //L-OUT
                *ptr++ = sample_L * EN_STEREO;

                result+=2*chn;
            }

            return result;          
        }

        void semiPulse(int tStateWidth, edge lastSlope, bool changeNextEdge)
        {

            // Obtenemos el periodo de muestreo
            // Tsr = 1 / samplingRate
            float freq = 1/(tStateWidth * tState);
            float Tsr = (1.0 / samplingRate);

            int bytes = int(round((1.0 / ((freq / alpha))) / Tsr));
            int chn = channels;

            uint8_t buffer[bytes*chn];

            // Escribimos el tren de pulsos en el procesador de Audio
            m_kit.write(buffer, makeSemiPulse(buffer, bytes, lastSlope, changeNextEdge));

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

        // void semiPulse(int nTStates, edge slope, bool changeNextEdge)
        // {
        //     // Procedimiento que genera un pulso de sincronismo, según los
        //     // T-States pasados por parámetro.
        //     //
        //     // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
        //     // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente. 
        //     generateSemiPulse(nTStates, samplingRate,slope,changeNextEdge); 
        // }

        void terminator(edge slope)
        {
            int width = maxTerminatorWidth;

            // Vemos como es el último bit MSB es la posición 0, el ultimo bit
            
            // Metemos un pulso de cambio de estado
            // para asegurar el cambio de flanco alto->bajo, del ultimo bit
            semiPulse(width,slope,true);
        }

        void customPilotTone(int lenPulse, int numPulses)
        {
            //
            // Esto se usa para el ID 0x13 del TZX
            //
            
            for (int i = 0; i < numPulses;i++)
            {
                semiPulse(lenPulse,LAST_EDGE_IS,true);

                if (LOADING_STATE == 1)
                {
                    if (STOP==true)
                    {
                        LOADING_STATE = 2; // Parada del bloque actual
                        //log("Aqui he parado - STOP");
                        return;
                    }
                    else if (PAUSE==true)
                    {
                        LOADING_STATE = 3; // Parada del bloque actual
                        //log("Aqui he parado - PAUSA");
                        return;
                    }
                }
            }
        }

        void pilotTone(int lenpulse, int numpulses)
        {
            // Tono guía para bloque TZX ID 0x10 y TAP

            // Calculamos la frecuencia del tono guía.
            // Hay que tener en cuenta que los T-States dados son de un SEMI-PULSO
            // es decir de la mitad del periodo. Entonces hay que calcular
            // el periodo completo que es 2 * T
            // float freq = (1 / (PULSE_PILOT * tState)) / 2;   
            // generateWaveDuration(freq, duration, samplingRate);
            
            customPilotTone(lenpulse, numpulses);
        }

        void zeroTone()
        {
            // Procedimiento que genera un bit "0"        
            semiPulse(BIT_0, LAST_EDGE_IS,true);
            semiPulse(BIT_0, LAST_EDGE_IS,true);
        }

        void oneTone()
        {
            // Procedimiento que genera un bit "1"
            semiPulse(BIT_1, LAST_EDGE_IS,true);
            semiPulse(BIT_1, LAST_EDGE_IS,true);       
        }

        void syncTone(int nTStates)
        {
            // Procedimiento que genera un pulso de sincronismo, según los
            // T-States pasados por parámetro.
            //
            // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono piloto
            // y otro al final de la recepción de los datos, que serán SYNC1 y SYNC2 respectivamente.
            semiPulse(nTStates,LAST_EDGE_IS,true);        
        }
      
        void sendDataArray(uint8_t* data, int size, bool isThelastBlock)
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
                        if ((i == size-1) && isThelastBlock)
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
                    
                        // Calculamos el byte enmascarado
                        uint8_t powMask = (256 - pow(2,8 - _mask));
                        uint8_t masked = bRead & powMask;
                        //
                        //log("Mascara:     " + String(_mask) + " - Dato: " + String(bRead));
                        //log("Byte masked: " + String(masked));

                        // Ahora vamos a descomponer el byte leido
                        // si hay mascara entonces solo se leerán n bits de los 8 que componen un BYTE.
                        for (int n=0;n < 8;n++)
                        {
                            // Calculamos el bit resultante de aplicar la mascara                        
                            uint8_t bitMasked = bitRead(masked, 7-n);

                            // Si el bit leido del BYTE es un "1"
                            if(bitMasked == 1)
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
                        return;
                    }
                }
                
                // Esto lo hacemos para asegurarnos que la barra se llena entera
                if (BYTES_LOADED > BYTES_TOBE_LOAD)
                {BYTES_LOADED = BYTES_TOBE_LOAD;}

            }
       
        }


    public:

        void set_maskLastByte(uint8_t mask)
        {
            _mask_last_byte = mask;
        }

        // void silence_ori(float duration)
        // {
            
        //     // Paso la duración a T-States
        //     edge edgeSelected = down;
        //     int thereIsTerminator = 0;

        //     // El silencio siempre acaba en un pulso de nivel bajo
        //     // Si no hay silencio, se pasas tres kilos del silencio y salimos
        //     if (duration > 0)
        //     {
        //         int tStateSilence = (duration/1000) / (1/freqCPU);       
        //         int minSilenceFrame = 5000;

        //         LAST_MESSAGE = "Silence: " + String(duration/1000) + " s";

        //         int parts = 0;
        //         int lastPart = 0;

        //         // Esto lo hacemos para acabar bien un ultimo flanco en down.
        //         // Hay que tener en cuenta que el terminador se quita del tiempo de PAUSA
        //         // la pausa puede ir desde 0 ms a 0x9999 ms
        //         // 1ms = 3500 T-States
        //         //
        //         // Según la especificación al menos 1ms debe estar en un flanco contrario al ultimo flanco para reconocer este ultimo, y después aplicar
        //         // el resto, como pausa.
        //         //

        //         // Aplicamos un cambio a alto, para leer el ultimo bit
        //         if (LAST_EDGE_IS==getPolarizeEdge(down))
        //         {
        //             // El primer milisegundo es el contrario al ultimo flanco
        //             terminator(LAST_EDGE_IS);
        //             thereIsTerminator = 1;
        //         }
        //         else
        //         {
        //             // No se necesita terminador
        //             thereIsTerminator = 0;
        //         }
                
        //         // Le restamos al silencio el terminador, si es que hubo
        //         if (tStateSilence > maxTerminatorWidth)
        //         {
        //             // Ahora calculamos el resto del silencio
        //             tStateSilence = tStateSilence - (maxTerminatorWidth * thereIsTerminator);
        //         }

        //         // Vemos si hay que partirlo
        //         if (tStateSilence > minSilenceFrame)
        //         {
        //             // Calculamos los frames
        //             parts = tStateSilence / minSilenceFrame;
        //             // Calculamos el ultimo frame
        //             lastPart = tStateSilence - (parts * minSilenceFrame);

        //             // Generamos el silencio
        //             for (int n=0;n<parts;n++)
        //             {
        //                 // Generamos el silencio ya en down
        //                 // Indicamos que el pulso anterior fue UP, porque o bien el terminador
        //                 // o bien el ultimo pulso, así estuvo, y cuando se genere el silencio
        //                 // se generará un down en la funcion makeSemiPulse

        //                 semiPulse(minSilenceFrame, up, false);

        //                 if (LOADING_STATE == 1)
        //                 {
        //                     if (STOP==true)
        //                     {
        //                         LOADING_STATE = 2; // Parada del bloque actual
        //                         return;
        //                     }
        //                     else if (PAUSE==true)
        //                     {
        //                         LOADING_STATE = 3; // Parada del bloque actual
        //                         return;
        //                     }
        //                 }                    
        //             }
        //         }
        //         else
        //         {
        //             lastPart = tStateSilence;
        //         }

        //         if (lastPart != 0)
        //         {
        //             // ultimo trozo del silencio
                    
        //             // Generamos el silencio ya en down
        //             // Indicamos que el pulso anterior fue UP, porque o bien el terminador
        //             // o bien el ultimo pulso, así estuvo, y cuando se genere el silencio
        //             // se generará un down en la funcion makeSemiPulse
        //             semiPulse(lastPart,up,false);

        //             if (LOADING_STATE == 1)
        //             {
        //                 if (STOP==true)
        //                 {
        //                     LOADING_STATE = 2; // Parada del bloque actual
        //                     return;
        //                 }
        //                 else if (PAUSE==true)
        //                 {
        //                     LOADING_STATE = 3; // Parada del bloque actual
        //                     return;
        //                 }
        //             }                     
        //         }                   

        //     }       
        // }

        void silence(float duration)
        {
            
            // Paso la duración a T-States
            edge edgeSelected = down;
            int thereIsTerminator = 0;

            // El silencio siempre acaba en un pulso de nivel bajo
            // Si no hay silencio, se pasas tres kilos del silencio y salimos
            if (duration > 0)
            {
                int tStateSilence = (duration/1000) / (1/freqCPU);       
                int minSilenceFrame = 5000;

                LAST_MESSAGE = "Silence: " + String(duration/1000) + " s";

                int parts = 0;
                int lastPart = 0;

                // Esto lo hacemos para acabar bien un ultimo flanco en down.
                // Hay que tener en cuenta que el terminador se quita del tiempo de PAUSA
                // la pausa puede ir desde 0 ms a 0x9999 ms
                // 1ms = 3500 T-States
                //
                // Según la especificación al menos 1ms debe estar en un flanco contrario al ultimo flanco para reconocer este ultimo, y después aplicar
                // el resto, como pausa.
                //

                // Aplicamos un cambio a alto, para leer el ultimo bit
                if (LAST_EDGE_IS==down)
                {
                    // El primer milisegundo es el contrario al ultimo flanco
                    terminator(LAST_EDGE_IS);
                    thereIsTerminator = 1;
                }
                else
                {
                    // No se necesita terminador
                    thereIsTerminator = 0;
                }
                
                // Le restamos al silencio el terminador, si es que hubo
                if (tStateSilence > maxTerminatorWidth)
                {
                    // Ahora calculamos el resto del silencio
                    tStateSilence = tStateSilence - (maxTerminatorWidth * thereIsTerminator);
                }

                // Vemos si hay que partirlo
                if (tStateSilence > minSilenceFrame)
                {
                    // Calculamos los frames
                    parts = tStateSilence / minSilenceFrame;
                    // Calculamos el ultimo frame
                    lastPart = tStateSilence - (parts * minSilenceFrame);

                    // Generamos el silencio
                    for (int n=0;n<parts;n++)
                    {
                        // Generamos el silencio ya en down
                        // Indicamos que el pulso anterior fue UP, porque o bien el terminador
                        // o bien el ultimo pulso, así estuvo, y cuando se genere el silencio
                        // se generará un down en la funcion makeSemiPulse

                        semiPulse(minSilenceFrame, up, false);

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

                if (lastPart != 0)
                {
                    // ultimo trozo del silencio
                    
                    // Generamos el silencio ya en down
                    // Indicamos que el pulso anterior fue UP, porque o bien el terminador
                    // o bien el ultimo pulso, así estuvo, y cuando se genere el silencio
                    // se generará un down en la funcion makeSemiPulse
                    semiPulse(lastPart,up,false);

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
        }



        void playPureTone(int lenPulse, int numPulses)
        {
            // Put now code block
            // syncronize with short leader tone
            customPilotTone(lenPulse, numPulses);          
        }

        void playCustomSequence(int* data, int numPulses)
        {
            //
            // Esto lo usamos para el PULSE_SEQUENCE ID-13
            //

            for (int i = 0; i < numPulses;i++)
            {
                // Generamos los semipulsos
                semiPulse(data[i],LAST_EDGE_IS,true);
            }

        }   

        void playData(uint8_t* bBlock, int lenBlock, int pulse_len, int pulse_pilot_duration)
        {
            //
            // Este procedimiento es usado por TAP
            //

            // Inicializamos el estado de la progress bar de bloque.
            PROGRESS_BAR_BLOCK_VALUE = 0;

            // Put now code block
            // syncronize with short leader tone
            pilotTone(pulse_len, pulse_pilot_duration);
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
            sendDataArray(bBlock, lenBlock,true);
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

        void playPureData(uint8_t* bBlock, int lenBlock)
        {
            // Se usa para reproducir los datos del ultimo bloque o bloque completo sin particionar, ID 0x14.
            // por tanto deben meter un terminador al final.
            sendDataArray(bBlock, lenBlock,true);

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

        void playDataPartition(uint8_t* bBlock, int lenBlock)
        {
            // Se envia a reproducir el bloque sin terminador ni silencio
            // ya que es parte de un bloque completo que ha sido partido en varios
            sendDataArray(bBlock, lenBlock, false);

            if (LOADING_STATE == 2)
            {
                return;
            }
        } 

        void playDataBegin(uint8_t* bBlock, int lenBlock, int pulse_len ,int pulse_pilot_duration)
        {
            // PROGRAM
            //float duration = tState * pulse_pilot_duration;
            // syncronize with short leader tone

            pilotTone(pulse_len,pulse_pilot_duration);
            // syncronization for end short leader tone
            syncTone(SYNC1);
            syncTone(SYNC2);

            // Send data
            sendDataArray(bBlock, lenBlock,false);
                   
        }

        void playDataEnd(uint8_t* bBlock, int lenBlock)
        {
            // Send data
            sendDataArray(bBlock, lenBlock,true);        
            
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
