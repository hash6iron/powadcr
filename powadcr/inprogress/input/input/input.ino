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
#include "AudioKitHAL.h"


AudioKit kit;
const int BUFFER_SIZE = 2048;
uint8_t buffer[BUFFER_SIZE];

// Estamos suponiendo que el ZX Spectrum genera signal para grabar
// con timming estandar entonces
float tState = 1/3500000;
int pilotPulse = 2168; // t-states
int sync1 = 667;
int sync2 = 735;
int bit0 = 855;
int bit1 = 1710;

const int high = 32767;
const int low = -32767;
const int silence = 0;

// Controlamos el estado del TAP
int state = 0;
// Contamos los pulsos del tono piloto
int pilotPulseCount = 0;
// Contamos los pulsos del sync1
long silenceCount = 0;
long lastSilenceCount = 0;
int bitCount = 0;
String bitString = "";
char* bitChStr = (char*)buffer;

// Estado flanco de la senal, HIGH or LOW
bool pulseHigh = false;
// Contador de bloques
int bl = 0;
// Umbral para rechazo de ruido
int threshold = 20000; 
//
bool errorInSemiPulseDetection = false;
//
int pulseState = 0;
int samplesCount_H = 0;
int samplesCount_L = 0;

struct tPulse
{
  int high_edge;
  int low_edge;
};

tPulse pulse;


// Calculamos las muestras que tiene cada pulso
// segun el tiempo de muestreo
long getSamples(int signalTStates, float tState, int samplingRate)
{
    float tSampling = 1/samplingRate;
    float tPeriod = signalTStates * tState;
    long samples = tPeriod / tSampling;

    return samples; 
}

int16_t prepareSignal(int16_t value, int threshold)
{
    int16_t returnValue = 0;

    if (value > threshold)
    {
      returnValue = 32767;
    }
    else if (value < -threshold)
    {
        returnValue = -32767;
    }
    else
    {
      returnValue = 0;    
    }

  return returnValue;
}

bool detectedSilencePulse(int16_t value, int threshold)
{
    if (value == 0 && silenceCount <= threshold)
    {
      // Si el pulso aun no ha completado un ancho y no ha habido errores
      // vamos bien.
      silenceCount++;
      return false;
    }
    else if (value == silence && silenceCount > threshold)
    {
        // Silencio detectado
        silenceCount = 0;
        return true;
    }
    else if (value != silence && silenceCount <= threshold)
    {
        // Silencio corrupto o menor del threshold
        silenceCount = 0;
        return false;
    }
    else if (value != silence && silenceCount > threshold)
    {
        // Silencio detectado
        silenceCount = 0;
        return true;
    }
}

bool measurePulse(int16_t value)
{
    if (pulseState == 0 && value == high)
    {
        samplesCount_H=1;
        pulseState = 1;
        return false;
    }
    else if (pulseState == 0 && value == low)
    {
        // Espero al flanco positivo
        pulseState = 0;
        silenceCount = 0;
        return false;
    }
    else if (pulseState == 0 && value == 0)
    {
        // Espero al flanco positivo
        pulseState = 0;
        silenceCount++;
        return false;
    }
    else if (pulseState == 1 && value == high)
    {
        samplesCount_H++;
        // Guardo el silencio
        lastSilenceCount = silenceCount;
        silenceCount = 0;
        return false;
    }
    else if (pulseState == 1 && value == low)
    {
        samplesCount_L=1;
        pulseState = 2;
        return false;
    }
    else if (pulseState == 2 && value == low)
    {
        samplesCount_L++;
        return false;
    }
    else if (pulseState == 2 && value == high)
    {
        // Ha finalizado el pulso
        pulse.high_edge = samplesCount_H;
        pulse.low_edge = samplesCount_L;

        // Leemos esta muestra que es HIGH para no perderla
        samplesCount_H = 1;
        samplesCount_L = 0;
        // Comenzamos otra vez
        pulseState = 1;
        return true;
    }
}

bool isGuidePulse(tPulse p)
{
    if (p.high_edge >= 26 && p.high_edge <= 28)
    {
        if (p.low_edge >= 26 && p.low_edge <= 28)
        {
            //
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool isSync(tPulse p)
{
    if (p.high_edge >= 8 && p.high_edge <= 9)
    {
        if (p.low_edge >= 8 && p.low_edge <= 9)
        {
            //
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool isBit0(tPulse p)
{
    if (p.high_edge >= 10 && p.high_edge <= 11)
    {
        if (p.low_edge >= 10 && p.low_edge <= 11)
        {
            //
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool isBit1(tPulse p)
{
    if (p.high_edge >= 20 && p.high_edge <= 22)
    {
        if (p.low_edge >= 20 && p.low_edge <= 22)
        {
            //
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

void readBuffer(int len)
{

  int16_t *value_ptr = (int16_t*) buffer;
  int16_t oneValue = *value_ptr++;
  int16_t oneValue2 = *value_ptr++;
  int16_t finalValue = 0;

    for (int j=0;j<len/4;j++)
    {  
        finalValue = prepareSignal(oneValue,threshold);
        //
        
        if (finalValue != silence)
        {
            if (measurePulse(finalValue))
            {
                // Ya se ha medido
                if (state == 0  && pilotPulseCount <= 800)
                {
                    if (isGuidePulse(pulse))
                    {
                        pilotPulseCount++;
                    }
                    else
                    {
                        pilotPulseCount = 0;
                    }
                }
                else if (state == 0 && pilotPulseCount > 800)
                {
                    // Saltamos a la espera de SYNC1 y 2
                    state = 1;
                    pilotPulseCount = 0;
                    Serial.println("Wait for SYNC");
                }
                else if (state == 1 && isSync(pulse))
                {
                    state = 2;
                    Serial.println("Wait for DATA");
                    Serial.println("");
                }
                else if (state == 2 && isBit0(pulse))
                {
                    state = 2;
                    bitString += "0";
                    bitChStr[bitCount] = '0';
                    bitCount++;
                }
                else if (state == 2 && isBit1(pulse))
                {
                    state = 2;
                    bitString += "1";
                    bitChStr[bitCount] = '1';
                    bitCount++;
                }

                if (bitCount > 7)
                {
                  bitCount=0;
                  // Valor leido del DATA
                  int value = strtol(bitChStr, (char**) NULL, 2);
                  // Lo representamos
                  Serial.print(bitString + " - ");
                  Serial.print(value,HEX);
                  Serial.println("");
                  bitString = "";
                }
            }
        }
        // Ahora medimos el pulso detectado

        // Leemos el siguiente valor
        oneValue = *value_ptr++;
        // Este solo es para desecharlo
        oneValue2 = *value_ptr++;        
    }
}

void setup() {
  Serial.begin(115200);
  // open in read mode
  auto cfg = kit.defaultConfig(AudioInput);
  //cfg.codec_mode = AUDIO_HAL_CODEC_MODE_LINE_IN;
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2; // microphone?
  cfg.sample_rate = AUDIO_HAL_44K_SAMPLES;
  kit.begin(cfg);

  Serial.println("");
  Serial.println("Waiting for guide tone");
  Serial.println("");

  // Inicializo bit string
  bitChStr = (char*)calloc(8, sizeof(char));
}

void loop() 
{
  size_t len = kit.read(buffer, BUFFER_SIZE);
  readBuffer(len);
}