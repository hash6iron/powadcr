/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: ZXProccesor.h

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

    Descripción:
    Clase que implementa metodos y parametrizado para todas las señales que
 necesita el ZX Spectrum para la carga de programas.

    NOTA: Esta clase necesita de la libreria Audio-kit de Phil Schatzmann -
 https://github.com/pschatzmann/arduino-audiokit

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

// #include <stdint.h>
#include <math.h>

#pragma once

// Clase para generar todo el conjunto de señales que necesita el ZX Spectrum
class ZXProcessor {

private:
  // FIR Filter variables.
  int _x[3] = {0, 0, 0};

  // Butterworth filter
  const float cf[2] = {0.00707352221530138658, 0.98585295556939722683};
  float v[2]{};

  // lowPass and highpass butterworth filter 2º order
  float _yf[3] = {0, 0, 0};
  float _xf[3] = {0, 0, 0};

  HMI _hmi;

  // Definición de variables internas y constantes
  uint8_t buffer[0];

  // const double sampleDuration = (1.0 / SAMPLING_RATE); //0.0000002267; //
  //  segundos para 44.1HKz

  // Estos valores definen las señales. Otros para el flanco negativo
  // provocan problemas de lectura en el Spectrum.
  double maxAmplitude = LEVELUP;
  double minAmplitude = LEVELDOWN;

  // Al poner 2 canales - Falla. Solucionar
  const int channels = 2;
  // const double speakerOutPower = 0.002;

public:
  bool forzeLowLevel = false;
  bool forzeHighLevel = false;

  // Parametrizado para el ZX Spectrum - Timming de la ROM
  // Esto es un factor para el calculo de la duración de onda
  const double alpha = 4.0;
  // Este es un factor de división para la amplitud del flanco terminador
  const double amplitudeFactor = 1.0;
  // T-states del ancho del terminador

  // otros parámetros
  const double freqCPU = DfreqCPU;
  const double tState = (1.0 / freqCPU); // 0.00000028571 --> segundos Z80
                                         // T-State period (1 / 3.5MHz)
  int SYNC1 = DSYNC1;
  int SYNC2 = DSYNC2;
  int BIT_0 = DBIT_0;
  int BIT_1 = DBIT_1;

  //
  int PILOT_PULSE_LEN = DPILOT_LEN;
  // int PILOT_TONE = DPILOT_TONE;

  // int PULSE_PILOT_DURATION = PILOT_PULSE_LEN * PILOT_TONE;
  // int PULSE_PILOT_DURATION = PILOT_PULSE_LEN * DPILOT_DATA;

  double silent = DSILENT;
  double m_time = 0;

private:
  uint8_t _mask_last_byte = 8;

  // Error accumulator for precise sample width calculation
  // Distributes rounding error across multiple pulses
  double ERROR_ACCUMULATOR = 0.0;

  // AudioKitStream m_kit;
  void tapeAnimationON() {
    // Activamos animacion cinta
    _hmi.writeString("tape.tmAnimation.en=1");
    delay(50);
    _hmi.writeString("tape.tmAnimation.en=1");
  }

  void tapeAnimationOFF() {
    // Desactivamos animacion cinta
    _hmi.writeString("tape.tmAnimation.en=0");
    delay(50);
    _hmi.writeString("tape.tmAnimation.en=0");
  }

  bool stopOrPauseRequest() {

    remDetection();

    if (STOP) {
      LAST_MESSAGE = "Stop requested. Wait.";
      LOADING_STATE = 2; // Parada del bloque actual
      ACU_ERROR = 0;
      STOP_OR_PAUSE_REQUEST = true;
      return true;
    } else if (PAUSE || (!REM_DETECTED && STATUS_REM_ACTUATED)) {
      LAST_MESSAGE = "Pause requested. Wait.";
      LOADING_STATE = 3; // Pausa del bloque actual
      ACU_ERROR = 0;
      STOP_OR_PAUSE_REQUEST = true;

      if (STATUS_REM_ACTUATED) {
        STATUS_REM_ACTUATED = false;
        PLAY = false;
        PAUSE = true;
        STOP = false;
        REC = false;
        ABORT = true;
        EJECT = false;
      }

      return true;
    } else {
      return false;
    }
  }

  void insertSamplesError(int samples, bool changeNextEARedge) {
    // Este procedimiento permite insertar en la señal
    // las muestras acumuladas por error generado en la conversión
    // de double a int
    bool forzeExit = false;
    // El buffer se dimensiona para 16 bits
    uint8_t buffer[samples * 2 * channels];
    uint16_t *ptr = (uint16_t *)buffer;

    size_t result = 0;
    uint16_t sample_R = 0;
    uint16_t sample_L = 0;
    size_t bytes_written = 0;
    double amplitude = 0.0;

    if (EDGE_EAR_IS == down) {
      amplitude = minAmplitude;
    } else {
      amplitude = maxAmplitude;
    }

    sample_R = amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL / 100);
    sample_L = amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL / 100);
    // }

    // Escribimos el tren de pulsos en el procesador de Audio
    // Generamos la señal en el buffer del chip de audio.
    for (int j = 0; j < samples; j++) {
      // R-OUT - Right channel output (Amplified output)
      *ptr++ = sample_R;
      // L-OUT - Left channel output (Speaker)
      if (ACTIVE_AMP) {
        *ptr++ = sample_L * EN_SPEAKER;
      } else {
        *ptr++ = sample_L;
      }

      result += 2 * channels;

      if (stopOrPauseRequest()) {
        // Salimos
        forzeExit = true;
        return;
      }
    }

    if (!forzeExit) {
      // btstream.write(buffer, result);
      if (OUT_TO_WAV) {
        encoderOutWAV.write(buffer, result);
      } else {
        kitStream.write(buffer, result);
      }
    }

    // Reiniciamos
    ACU_ERROR = 0;
    // EDGE_EAR_IS ^=1;
    // if (INVERSETRAIN) EDGE_EAR_IS ^=1;
  }

  double getChannelAmplitude() {

    // Cambiamos el edge
    if (!KEEP_CURRENT_EDGE) {
      EDGE_EAR_IS ^= 1;
    }

    double A = 0;

    if (EDGE_EAR_IS == down) {
      A = minAmplitude;
    } else {
      A = maxAmplitude;
    }

    return (A);
  }

  int firFilter(int data) {
    _x[0] = _x[1];
    long tmp = ((((data * 3269048L) >> 2)    //= (3.897009118e-1 * data)
                 + ((_x[0] * 3701023L) >> 3) //+(  0.2205981765*v[0])
                 ) +
                1048576) >>
               21; // round and downshift fixed point /2097152
    _x[1] = (int)tmp;
    return (int)(_x[0] + _x[1]); // 2^
  }

  int lowPass(int data) {
    _xf[0] = _xf[1];
    _xf[1] = data;
    _yf[0] = _yf[1];
    float gain = 10;

    return (int)(gain * (0.969 * _yf[0] + 0.0155 * _xf[1] + 0.0155 * _xf[0]));
  }

  int highPass(int data) {
    // Coeficientes de 2º orden. Butterworth
    // https://www.meme.net.au/butterworth.html

    float A = 1;
    float B = 2;
    float C = 1;
    float D = 0.681;
    float E = -0.703;

    float gain = 6000;

    // Calculos
    _xf[0] = _xf[1];
    _xf[1] = _xf[2];
    _xf[2] = data;

    _yf[0] = _yf[1];
    _yf[1] = _yf[2];

    _yf[2] = (int)(gain * ((A * _xf[2] - B * _xf[1] + C * _xf[0])) +
                   D * _yf[1] - E * _yf[0]);

    return _yf[2];
  }

  public: 

  void createPulse(int width, int bytes, uint16_t sample_R, uint16_t sample_L) {
    size_t result = 0;
    uint8_t buffer[bytes + 4];
    int16_t *ptr = (int16_t *)buffer;
    int chn = channels;
    size_t bytes_written = 0;

    LAST_PULSE_WIDTH = width;

    for (int j = 0; j < width; j++) {
      if (stopOrPauseRequest()) {
        // Salimos
        return;
      }

      // R-OUT - Right channel output (Amplified output) and current output.
      *ptr++ = sample_R;
      // L-OUT - Left channel output (Speaker)
      if (ACTIVE_AMP) {
        *ptr++ = sample_L * EN_SPEAKER;
      } else {
        *ptr++ = sample_L;
      }
      //

      result += 2 * chn;
    }

    // int sr_parts = (MOTOR_DELAY_MS * (SAMPLING_RATE / 1000) * 2 * channels);
    

    // if (REM_DETECTED && (firstBytes_REM < sr_parts))
    // {
    //   // Si se ha detectado el REM y no se han escrito los primeros 4 bytes, insertamos un pulso de error
    //   AudioInfo info = kitStream.audioInfo();
    //   info.sample_rate = (SAMPLING_RATE / sr_parts)*firstBytes_REM;
    //   kitStream.setAudioInfo(info);
    //   firstBytes_REM++;
    //   if (firstBytes_REM >= sr_parts) 
    //   {
    //     // Una vez insertados los primeros 4 bytes, restauramos el sample rate original
    //     AudioInfo info = kitStream.audioInfo();
    //     info.sample_rate = SAMPLING_RATE;
    //     kitStream.setAudioInfo(info);
    //   }
    // }

    // Volcamos en el buffer
    if (OUT_TO_WAV) {
      encoderOutWAV.write(buffer, result);
    } else {
      kitStream.write(buffer, result);
    }
  }

  private: 
  
  void sampleDR(int samples, int amp) {
    // Calculamos el tamaño del buffer
    int bytes = 0; // Cada muestra ocupa 2 bytes (16 bits)
    int chn = channels;
    int frames = 0;
    double frames_rest = 0;

    // El buffer se dimensiona para 16 bits
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;
    // Amplitud de la señal
    double amplitude = 0;

    // double rsamples = round((width / freqCPU) * SAMPLING_RATE);
    //  Ajustamos
    // int samples = 1;

    // Ajustamos el volumen

    sample_R = (uint16_t)(amp * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    sample_L = (uint16_t)(amp * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));
    // }

    bytes = (int)samples * 2 * channels;
    //

    // Generamos la onda
    createPulse(samples, bytes, sample_R, sample_L);

    if (stopOrPauseRequest()) {
      // Salimos
      return;
    }
  }

  void pulseSilencePZX(int samples, bool initialLevelLow) {
    // Calculamos el tamaño del buffer
    int bytes = 0; // Cada muestra ocupa 2 bytes (16 bits)
    int chn = channels;
    int frames = 0;
    double frames_rest = 0.0;

    // El buffer se dimensiona para 16 bits
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;
    // Amplitud de la señal
    double amplitude = 0.0;

    ACU_ERROR = 0.0;

    //
    // Generamos el semi-pulso
    //

    // Obtenemos la amplitud de la señal según la configuración de polarización,
    // nivel low, etc.
    // Esto es para PZX
    EDGE_EAR_IS = initialLevelLow ? down : up;
    amplitude = getChannelAmplitude();

    sample_R = (uint16_t)(amplitude * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    sample_L = (uint16_t)(amplitude * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));

    // Pasamos los datos para el modo DEBUG
    DEBUG_AMP_R = sample_R;
    DEBUG_AMP_L = sample_L;

    // Definiciones el número samples para el splitter
    int minFrame = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;

    // Si el semi-pulso tiene un numero
    // menor de muestras que el minFrame, entonces
    // el minFrame será ese numero de muestras
    if (samples <= minFrame) {

      #ifdef DEBUGMODE
            // log("SIMPLE PULSE");
            // log(" --> samp:  " + String(samples));
            // log(" --> bytes: " + String(bytes));
            // log(" --> Chns:  " + String(channels));
            // log(" --> T0:  " + String(T0));
            // log(" --> T1:  " + String(T1));
      #endif
      // Definimos el tamaño del buffer
      bytes = samples * 2 * channels;
      //

      // Generamos la onda
      createPulse(samples, bytes, sample_R, sample_L);
    } else {
      // Caso de un silencio o pulso muy largo
      // -------------------------------------
      int frameSlot = samples;
      bytes = minFrame * 2 * channels;
      int pass = 0;
      while (frameSlot >= minFrame) {
        createPulse(minFrame, bytes, sample_R, sample_L);
        frameSlot = frameSlot - minFrame;
        pass++;
        if (stopOrPauseRequest()) {
          return;
        }
      }

      // El ultimo trozo
      if (frameSlot > 0) {
        bytes = frameSlot * 2 * channels;
        createPulse(frameSlot, bytes, sample_R, sample_L);
      }

#ifdef DEBUGMODE
      // log("ACU_ERROR: "+ String(ACU_ERROR));
#endif
    }

    if (stopOrPauseRequest()) {
      return;
    }
  }

  void pulseSilence(int samples) {
    // Calculamos el tamaño del buffer
    int bytes = 0; // Cada muestra ocupa 2 bytes (16 bits)
    int chn = channels;
    int frames = 0;
    double frames_rest = 0.0;

    // El buffer se dimensiona para 16 bits
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;
    // Amplitud de la señal
    double amplitude = 0.0;

    ACU_ERROR = 0.0;
    //
    // Generamos el semi-pulso
    //

    // Obtenemos la amplitud de la señal según la configuración de polarización,
    // nivel low, etc.
    // EDGE_EAR_IS = down;
    amplitude = getChannelAmplitude();

    // if (SWAP_MIC_CHANNEL)
    // {
    //     sample_L = amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL / 100);
    //     sample_R = amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL / 100);
    // }
    // else
    // {
    sample_R =
        (uint16_t)(amplitude * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    sample_L =
        (uint16_t)(amplitude * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));
    // }

    // Pasamos los datos para el modo DEBUG
    DEBUG_AMP_R = sample_R;
    DEBUG_AMP_L = sample_L;

    // Definiciones el número samples para el splitter
    int minFrame = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;

    // Si el semi-pulso tiene un numero
    // menor de muestras que el minFrame, entonces
    // el minFrame será ese numero de muestras
    if (samples <= minFrame) {

#ifdef DEBUGMODE
      // log("SIMPLE PULSE");
      // log(" --> samp:  " + String(samples));
      // log(" --> bytes: " + String(bytes));
      // log(" --> Chns:  " + String(channels));
      // log(" --> T0:  " + String(T0));
      // log(" --> T1:  " + String(T1));
#endif
      // Definimos el tamaño del buffer
      bytes = samples * 2 * channels;
      // Generamos la onda
      createPulse(samples, bytes, sample_R, sample_L);
    } else {
      // Caso de un silencio o pulso muy largo
      // -------------------------------------
      int frameSlot = samples;
      bytes = minFrame * 2 * channels;
      int pass = 0;
      while (frameSlot >= minFrame) {
        createPulse(minFrame, bytes, sample_R, sample_L);
        frameSlot = frameSlot - minFrame;
        pass++;
        if (stopOrPauseRequest()) {
          return;
        }
      }

      // El ultimo trozo
      if (frameSlot > 0) {
        bytes = frameSlot * 2 * channels;
        createPulse(frameSlot, bytes, sample_R, sample_L);
      }

#ifdef DEBUGMODE
      // log("ACU_ERROR: "+ String(ACU_ERROR));
#endif
    }

    if (stopOrPauseRequest()) {
      return;
    }
  }

  void fullPulse(double dwidth, double calibrationValue = 0.0) {
    // Genera un pulso COMPLETO (dos semipulsos) con mayor precisión
    int chn = channels;
    int minFrame = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
    double tpulseWidth = ((dwidth / freqCPU) * SAMPLING_RATE) + calibrationValue;
    int s[2] = {(int)round(tpulseWidth),(int)round((2 * tpulseWidth) - round(tpulseWidth))};

    for (int i = 0; i < 2; ++i) {
      int samples = s[i];

      double amplitude = 0.0;
      amplitude = getChannelAmplitude();

      // double amplitude = getChannelAmplitude(true);
      uint16_t sample_R =
          (uint16_t)(amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL / 100));
      uint16_t sample_L =
          (uint16_t)(amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL / 100));
      DEBUG_AMP_R = sample_R;
      DEBUG_AMP_L = sample_L;

      // if (samples <= minFrame) {
      int bytes = samples * 2 * chn;
      createPulse(samples, bytes, sample_R, sample_L);
      // } else {
      //     int framesCounter = 0;
      //     int frameSlot = minFrame;
      //     int bytes = minFrame * 2 * chn;
      //     while (frameSlot <= samples) {
      //         createPulse(minFrame, bytes, sample_R, sample_L);
      //         frameSlot += minFrame;
      //         if (stopOrPauseRequest()) return;
      //         framesCounter++;
      //     }
      //     // Procesa el residuo final si existe
      //     int processed = framesCounter * minFrame;
      //     int remaining = samples - processed;
      //     if (remaining > 0) {
      //         bytes = remaining * 2 * chn;
      //         createPulse(remaining, bytes, sample_R, sample_L);
      //     }
      // }
      if (stopOrPauseRequest())
        return;
    }
  }

  // void fullPulse(double dwidth, double calibrationValue = 0.0)
  // {
  //     // **************************************************************
  //     //
  //     // Genera un pulso COMPLETO (dos semipulsos) con mayor precisión
  //     //
  //     // **************************************************************

  //     // Calculamos el tamaño del buffer
  //     int bytes = 0; //Cada muestra ocupa 2 bytes (16 bits)
  //     int chn = channels;
  //     int frames = 0;
  //     double frames_rest = 0;

  //     // El buffer se dimensiona para 16 bits
  //     uint16_t sample_L = 0;
  //     uint16_t sample_R = 0;
  //     // Amplitud de la señal
  //     double amplitude = 0;

  //     // Calculamos los semipulsos
  //     int pulseWidth = (int)round(((dwidth / freqCPU) * SAMPLING_RATE) +
  //     calibrationValue); int s1 = int(pulseWidth / 2); int s2 = pulseWidth -
  //     s1;

  //     amplitude = getChannelAmplitude(true);
  //     sample_R = (uint16_t)(amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL /
  //     100)); sample_L = (uint16_t)(amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL
  //     / 100));

  //     // Pasamos los datos para el modo DEBUG
  //     DEBUG_AMP_R = sample_R;
  //     DEBUG_AMP_L = sample_L;

  //     // Definiciones el número samples para el splitter
  //     int minFrame = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
  //     // Si el semi-pulso tiene un numero
  //     // menor de muestras que el minFrame, entonces
  //     // el minFrame será ese numero de muestras

  //     if (s1 <= minFrame)
  //     {
  //         // Definimos el tamaño del buffer
  //         bytes = s1 * 2 * channels;
  //         // Generamos la onda
  //         createPulse(s1,bytes,sample_R,sample_L);
  //     }
  //     else
  //     {
  //         // Caso de un silencio o pulso muy largo
  //         // -------------------------------------

  //         // Definimos los terminadores de cada frame para el splitter
  //         int frameSlot = minFrame;

  //         // Definimos el buffer
  //         bytes = minFrame * 2 * channels;

  //         // Dividimos la onda en trozos
  //         int framesCounter = 0;

  //         while(frameSlot <= s1)
  //         {
  //             createPulse(minFrame, bytes, sample_R, sample_L);
  //             frameSlot += minFrame;

  //             if (stopOrPauseRequest()) {return;}
  //             framesCounter++;
  //         }

  //             int processed = framesCounter * minFrame;
  //             int remaining = s1 - processed;
  //             if (remaining > 0) {
  //                 bytes = remaining * 2 * channels;
  //                 createPulse(remaining, bytes, sample_R, sample_L);
  //             }
  //     }

  //     amplitude = getChannelAmplitude(true);
  //     sample_R = (uint16_t)(amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL /
  //     100)); sample_L = (uint16_t)(amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL
  //     / 100));

  //     // Ahora la otra mitad del pulso
  //     if (s2 <= minFrame)
  //     {
  //         // Definimos el tamaño del buffer
  //         bytes = s2 * 2 * channels;
  //         // Generamos la onda
  //         createPulse(s2,bytes,sample_R,sample_L);
  //     }
  //     else
  //     {
  //         // Caso de un silencio o pulso muy largo
  //         // -------------------------------------

  //         // Definimos los terminadores de cada frame para el splitter
  //         int frameSlot = minFrame;

  //         // Definimos el buffer
  //         bytes = minFrame * 2 * channels;

  //         // Dividimos la onda en trozos
  //         int framesCounter = 0;

  //         while(frameSlot <= s2)
  //         {
  //             createPulse(minFrame, bytes, sample_R, sample_L);
  //             frameSlot += minFrame;

  //             if (stopOrPauseRequest()) {return;}
  //             framesCounter++;
  //         }
  //     }

  //     if (stopOrPauseRequest()) {return;}
  // }

  void semiPulsePZX(double dwidth, bool initialLevelLow) {
    // Amplitud de la señal
    double amplitude = 0;
    int bytes = 0; // Cada muestra ocupa 2 bytes (16 bits)
    int samples = 0;
    // El buffer se dimensiona para 16 bits
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;

    // Calculamos el numero de samples con alta precisión
    // Usamos acumulador de error para distribuir fracciones uniformemente
    double rsamples = (((dwidth / freqCPU) * SAMPLING_RATE));
    
    // Acumular el valor exacto (incluyendo la parte fraccionaria)
    ERROR_ACCUMULATOR += rsamples;
    
    // Tomar la parte entera del acumulador como samples
    samples = (int)ERROR_ACCUMULATOR;
    
    // Mantener solo la parte fraccionaria para el próximo pulso
    ERROR_ACCUMULATOR -= samples;
    
    #ifdef DEBUGMODE
      logln(String(dwidth) + " / " + String(freqCPU) + " * " + String(SAMPLING_RATE) + " = " +
        String(rsamples) + " (accumulated: " + String(ERROR_ACCUMULATOR + samples) + ") => " + String(samples));
    #endif

    // Obtenemos la amplitud de la señal según la configuración de polarización,
    // nivel low, etc.

    // Esto es para PZX
    if (!CHANGE_PZX_LEVEL) {
      // Establecemos el nivel inicial del bloque DATA
      EDGE_EAR_IS = initialLevelLow ? down : up;
      // Usamos KEEP_CURRENT_EDGE para que getChannelAmplitude() no haga toggle
      KEEP_CURRENT_EDGE = true;
      amplitude = getChannelAmplitude();
      KEEP_CURRENT_EDGE = false;
      CHANGE_PZX_LEVEL = true;
    } else {
      amplitude = getChannelAmplitude();
    }

    sample_R = (uint16_t)(amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL / 100));
    sample_L = (uint16_t)(amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL / 100));

    // Pasamos los datos para el modo DEBUG
    DEBUG_AMP_R = sample_R;
    DEBUG_AMP_L = sample_L;

    // Hacemos splitting del frame si es necesario
    if (samples <= MIN_FRAME_FOR_SILENCE_PULSE_GENERATION) {
      // Caso de pulso corto
      // -------------------
      // Definimos el tamaño del buffer
      bytes = samples * 2 * channels;
      // Generamos la onda
      createPulse(samples, bytes, sample_R, sample_L);
    } else {
      // Caso de un silencio o pulso muy largo
      // -------------------------------------
      // Definimos los terminadores de cada frame para el splitter
      int frameSlot = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
      bytes = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION * 2 * channels;
      int framesCounter = 0;

      while (frameSlot <= samples) {
        createPulse(MIN_FRAME_FOR_SILENCE_PULSE_GENERATION, bytes, sample_R,
                    sample_L);
        frameSlot += MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;

        if (stopOrPauseRequest()) {
          return;
        }
        framesCounter++;
      }

      // ultima parte
      int processed = framesCounter * MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
      int remaining = samples - processed;
      if (remaining > 0) {
        bytes = remaining * 2 * channels;
        createPulse(remaining, bytes, sample_R, sample_L);
      }
    }
    // double int_rsamples;
    // double rsamples_fl = modf(rsamples,&int_rsamples);
    // double errAcc = rsamples - int_rsamples;
    // ADD_ONE_SAMPLE_COMPENSATION = errAcc >= 0.5 ? 1:0;
    // ADD_ONE_SAMPLE_COMPENSATION = 1;
    if (stopOrPauseRequest()) {
      return;
    }
  }



  void semiPulse(double dwidth, double samples_compensation = 0) 
  {
    // Amplitud de la señal
    double amplitude = 0;
    int bytes = 0; // Cada muestra ocupa 2 bytes (16 bits)
    int samples = 0;
    // El buffer se dimensiona para 16 bits
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;

    // Calculamos el numero de samples con alta precisión
    // Usamos acumulador de error para distribuir fracciones uniformemente
    double rsamples = (((dwidth / freqCPU) * SAMPLING_RATE)) + samples_compensation;
    
    // Acumular el valor exacto (incluyendo la parte fraccionaria)
    ERROR_ACCUMULATOR += rsamples;
    
    // Tomar la parte entera del acumulador como samples
    samples = (int)ERROR_ACCUMULATOR;
    
    // Mantener solo la parte fraccionaria para el próximo pulso
    ERROR_ACCUMULATOR -= samples;

    #ifdef DEBUGMODE
      logln(String(dwidth) + " / " + String(freqCPU) + " * " + String(SAMPLING_RATE) + " = " +
        String(rsamples) + " (accumulated: " + String(ERROR_ACCUMULATOR + samples) + ") => " + String(samples));
    #endif

    // Obtenemos la amplitud de la señal según la configuración de polarización,
    // nivel low, etc.
    amplitude = getChannelAmplitude();

    sample_R = (uint16_t)(amplitude * (MAIN_VOL_R / 100) * (MAIN_VOL / 100));
    sample_L = (uint16_t)(amplitude * (MAIN_VOL_L / 100) * (MAIN_VOL / 100));

    // Pasamos los datos para el modo DEBUG
    DEBUG_AMP_R = sample_R;
    DEBUG_AMP_L = sample_L;

    // Hacemos splitting del frame si es necesario
    if (samples <= MIN_FRAME_FOR_SILENCE_PULSE_GENERATION) {
      // Caso de pulso corto
      // -------------------
      // Definimos el tamaño del buffer
      bytes = samples * 2 * channels;
      // Generamos la onda
      createPulse(samples, bytes, sample_R, sample_L);
    } else {
      // Caso de un silencio o pulso muy largo
      // -------------------------------------
      // Definimos los terminadores de cada frame para el splitter
      int frameSlot = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
      bytes = MIN_FRAME_FOR_SILENCE_PULSE_GENERATION * 2 * channels;
      int framesCounter = 0;

      while (frameSlot <= samples) {
        createPulse(MIN_FRAME_FOR_SILENCE_PULSE_GENERATION, bytes, sample_R,
                    sample_L);
        frameSlot += MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;

        if (stopOrPauseRequest()) {
          return;
        }
        framesCounter++;
      }

      // ultima parte
      int processed = framesCounter * MIN_FRAME_FOR_SILENCE_PULSE_GENERATION;
      int remaining = samples - processed;
      if (remaining > 0) {
        bytes = remaining * 2 * channels;
        createPulse(remaining, bytes, sample_R, sample_L);
      }
    }

    if (stopOrPauseRequest()) {
      return;
    }
  }

  void customPilotTone(int lenPulse, int numPulses) {
    //
    // Esto se usa para el ID 0x13 del TZX
    //

#ifdef DEBUGMODE
    log("..Custom pilot tone");
    log("  --> lenPulse:  " + String(lenPulse));
    log("  --> numPulses: " + String(numPulses));
#endif

    for (int i = 0; i < numPulses; i++) {
      // Enviamos semi-pulsos alternando el cambio de flanco
      ADD_ONE_SAMPLE_COMPENSATION = false;
      semiPulse((double)lenPulse);
      ACU_ERROR = 0;
      if (stopOrPauseRequest()) {
        return;
      }
    }
  }

  void pilotTone(int lenpulse, int numpulses) {
    // Tono guía para bloque TZX ID 0x10 y TAP
    // int npulses = (int)round(numpulses/2);

    for (int i = 0; i < numpulses; i++) {
      // Enviamos semi-pulsos alternando el cambio de flanco
      semiPulse((double)lenpulse);
      ACU_ERROR = 0;
      if (stopOrPauseRequest()) {
        return;
      }
    }
  }

  void zeroTone() {
    // Procedimiento que genera un bit "0"
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulse(BIT_0);
    semiPulse(BIT_0);
    ACU_ERROR = 0;
  }

  void oneTone() {
    // Procedimiento que genera un bit "1"
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulse(BIT_1);
    semiPulse(BIT_1);
    ACU_ERROR = 0;
  }

  void syncTone(int nTStates) {
    // Procedimiento que genera un pulso de sincronismo, según los
    // T-States pasados por parámetro.
    //
    // El ZX Spectrum tiene dos tipo de sincronismo, uno al finalizar el tono
    // piloto y otro al final de la recepción de los datos, que serán SYNC1 y
    // SYNC2 respectivamente.

    // El pulso de sincronismo se considera un semi-pulso entonces no tiene
    // compensación.
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulse((double)nTStates);
    ACU_ERROR = 0;
  }

  void sendDataArray(uint8_t *data, int size, bool isThelastDataPart) {
    uint8_t _mask = 8; // Para el last_byte
    uint8_t bRead = 0x00;
    int bytes_in_this_block = 0;
    int ptrOffset = 0;

    // Procedimiento para enviar datos desde un array.
    // si estamos reproduciendo, nos mantenemos.
    if (LOADING_STATE == 1 || TEST_RUNNING) {

      // Recorremos todo el vector de bytes leidos para reproducirlos
      for (int i = 0; i < size; i++) {

        if (!TEST_RUNNING) {
          // Informacion para la barra de progreso
          // PROGRESS_BAR_BLOCK_VALUE = (int)(((i+1)*100)/(size));

          // if (BYTES_LOADED > BYTES_TOBE_LOAD)
          // {BYTES_LOADED = BYTES_TOBE_LOAD;}
          // Informacion para la barra de progreso total

          if (stopOrPauseRequest()) {
            // Salimos
            i = size;
            return;
          }
        }

        // Vamos a ir leyendo los bytes y generando el sonido
        bRead = data[i];

        // log("Dato: " + String(i) + " - " + String(data[i]));

        // Para la protección con mascara ID 0x11 - 0x0C
        // ---------------------------------------------
        // "Used bits in the last uint8_t (other bits should be 0) {8}
        //(e.g. if this is 6, then the bits used (x) in the last uint8_t are:
        // xxxxxx00, wh///ere MSb is the leftmost bit, LSb is the rightmost
        // bit)"

        // ¿Es el ultimo BYTE?. Si se ha aplicado mascara entonces
        // se modifica el numero de bits a transmitir

        if (LOADING_STATE == 1 || TEST_RUNNING) {
          // Vemos si es el ultimo byte de la ultima partición de datos del
          // bloque
          if ((i == size - 1) && isThelastDataPart) {
            // Aplicamos la mascara
            _mask = _mask_last_byte;
            // log("Mascara: " + String(_mask) + " - Dato: [" + String(i) + "] -
            // " + String(data[i]));
          } else {
            // En otro caso todo el byte es valido.
            // se anula la mascara.
            _mask = 8;
          }

          // Ahora vamos a descomponer el byte leido
          // y le aplicamos la mascara. Es decir SOLO SE TRANSMITE el nº de bits
          // que indica la mascara, para el último byte del bloque

          for (int n = 0; n < _mask; n++) {
            // Obtenemos el bit a transmitir
            uint8_t bitMasked = bitRead(bRead, 7 - n);

            // Si el bit leido del BYTE es un "1"
            if (bitMasked == 1) {
              // Procesamos "1"
              oneTone();
            } else {
              // En otro caso
              // procesamos "0"
              zeroTone();
            }
          }

          // Hemos cargado +1 byte. Seguimos
          if (!TEST_RUNNING) {
            BYTES_LOADED++;
            bytes_in_this_block++;
            BYTES_LAST_BLOCK = bytes_in_this_block;
          }
        } else {
          return;
        }

        ptrOffset = i;

        if (BYTES_TOBE_LOAD > 0)
          PROGRESS_BAR_TOTAL_VALUE =
              ((PRG_BAR_OFFSET_INI + (ptrOffset + 1)) * 100) / BYTES_TOBE_LOAD;

        if (PRG_BAR_OFFSET_END > 0)
          PROGRESS_BAR_BLOCK_VALUE =
              ((PRG_BAR_OFFSET_INI + (ptrOffset + 1)) * 100) /
              PRG_BAR_OFFSET_END;
      }
    }
  }

public:
  void setEdge(uint8_t edge) { EDGE_EAR_IS = edge; }

  void set_maskLastByte(uint8_t mask) { _mask_last_byte = mask; }

  void silenceDR(double duration, double sr) {
    // la duracion se da en ms
    LAST_SILENCE_DURATION = duration;

    // Paso la duración a T-States
    double parts = 0, fractPart, intPart, terminator_samples = 0;
    int lastPart = 0;
    int tStateSilence = 0;
    int tStateSilenceOri = 0;
    double samples = 0;

    #ifdef DEBUGMODE
        log("Silencio: " + String(duration) + " ms");
    #endif

    // El silencio siempre acaba en un pulso de nivel bajo
    // Si no hay silencio, se pasas tres kilos del silencio y salimos
    if (duration > 0) 
    {

      samples = ((duration / 1000.0) * sr);

      if (duration < 500.0) {
        // Calculamos el error y agregamos una muestra si es necesario
        int samples_int = (int)samples;
        double duration_int = samples_int / sr * 1000.0;

        if ((duration_int - duration) > 0.01) {
          samples++;
        }
      }

      #ifdef DEBUGMODE
            log("Samples: " + String(samples));
      #endif

      // Generador de pulsos exclusivo para silencios
      int swidth = round(samples);
      if (swidth > 0) {

        // Esto es para máquinas como el 48K
        if (duration >= 1000) {
          // Añadimos ms extras para maquinas como el 48K
          int addsmp = (int)(SILENCE_COMPENSATION_48K * SAMPLING_RATE);
          swidth += addsmp;
        }
        //
        pulseSilence(swidth);
      }

    } else {
      EDGE_EAR_IS ^= 1; // Alternamos el nivel del EAR para el siguiente pulso
    }
  }

  void forceLevelLow() {
    // Fuerza el nivel a LOW sin generar audio
    EDGE_EAR_IS = down;
  }

  void silence(double duration, bool changeNextEARedge = true, bool pzxForzeLevel = false, double samplesCompensation = 0) 
  {
    // la duracion se da en ms
    LAST_SILENCE_DURATION = duration;
    ACU_ERROR = 0;
    double samples = 0;

    #ifdef DEBUGMODE
        logln("Silencio: " + String(duration) + " ms");
        logln(" - Current EAR level: " +
              String(EDGE_EAR_IS == down ? "LOW" : "HIGH"));
    #endif

    // Generar silencio adicional si duration > 0
    if (duration > 0.0) {
      samples = (duration / 1000.0) * SAMPLING_RATE;

      // 07/04/2026
      // Añadido ROUND() para evitar problemas de truncado en silencios largos.
      int swidth = (int)round(samples + samplesCompensation);

      // Esto es para máquinas como el 48K
      if (duration >= 1000) {
        // Añadimos ms extras para maquinas como el 48K
        int addsmp = (int)(SILENCE_COMPENSATION_48K * SAMPLING_RATE);
        swidth += addsmp;
      }

      pulseSilence(swidth);

    } else {
      // Si el silencio es 0 al menos tengo que replicar el ultimo semipulso
      // para que se note el cambio de nivel

      // Opcion 1: generar un pulso de silencio con la duración del último
      // pulso, para asegurar que se note el cambio de nivel en el EAR
      // pulseSilence(LAST_PULSE_WIDTH);

      // Opcion 2: simplemente alternamos el nivel del EAR para el siguiente
      // pulso, sin generar un pulso de silencio adicional
      // EDGE_EAR_IS ^= 1; // Alternamos el nivel del EAR para el siguiente
      // pulso
    }
  }

  void silencePZX(double duration, bool initialLevelLow) {
    // la duracion se da en ms
    LAST_SILENCE_DURATION = duration;
    // ADD_ONE_SAMPLE_COMPENSATION = false;
    ACU_ERROR = 0;

    double samples = 0;

    #ifdef DEBUGMODE
        log("Silencio: " + String(duration) + " ms");
    #endif

    // Generar silencio adicional si duration > 0
    if (duration > 0.0) {

      // Calculamos el numero de samples
      samples = (duration / 1000.0) * SAMPLING_RATE;

      // Generador de pulsos exclusivo para silencios
      int swidth = (int)round(samples + PZX_COMPENSATION_FACTOR);
      if (swidth > 0) {
        logln("Samples of the SILENCE: " + String(duration) + " is " + String(swidth));
        pulseSilencePZX(swidth, initialLevelLow);
      }
    }
  }

  void pulsePZX(int width, bool initialLevelLow) {
    // Para PZX
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulsePZX((double)width, initialLevelLow);
    ACU_ERROR = 0;
  }

  void pulse(int width) {
    // Para PZX
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulse((double)width, PZX_COMPENSATION_FACTOR);
    ACU_ERROR = 0;
  }

  void playPulse(int lenPulse) {
    // Para PZX
    ADD_ONE_SAMPLE_COMPENSATION = false;
    semiPulse((double)lenPulse, PZX_COMPENSATION_FACTOR);
    ACU_ERROR = 0;
  }

  void playPureTone(int lenPulse, int numPulses) {
    // syncronize with short leader tone
    customPilotTone(lenPulse, numPulses);
  }

  void playCustomSequence(int *data, int numPulses, long calibrationValue = 0) {
    //
    // Esto lo usamos para el PULSE_SEQUENCE ID-13
    //

    double samples = 0;
    double rsamples = 0;
    int ptrOffset = 0;

    for (int i = 0; i < numPulses; i++) {
      ADD_ONE_SAMPLE_COMPENSATION = false;
      semiPulse((double)data[i]);
      ACU_ERROR = 0;
      ptrOffset = i;
    }
  }

  void playGDB(tSymbol *symbol) {
    // Reproducir Generalized Data Block
    int pulseCount = 0;
    // Primero, pilot/sync stream
    if (symbol->TOTP > 0) {
      for (int i = 0; i < symbol->TOTP; i++) {
        int symIndex = symbol->pilotStream[i].symbol;
        int repeat = symbol->pilotStream[i].repeat;
        for (int r = 0; r < repeat; r++) {
          playSymbol(symbol->symDefPilot[symIndex], symbol->NPP, pulseCount);
          if (stopOrPauseRequest())
            return;
        }
      }
    }

    // Luego, data stream
    int NB = 0;
    int temp = symbol->ASD;
    while (temp > 1) {
      temp >>= 1;
      NB++;
    }
    if ((1 << NB) < symbol->ASD)
      NB++;

    int bitPos = 0;
    for (int i = 0; i < symbol->TOTD; i++) {
      // Extraer NB bits del data stream, LSB first in byte = LSB first in
      // symbol
      int symbolIndex = 0;
      for (int b = 0; b < NB; b++) {
        int byteIndex = bitPos / 8;
        int bitInByte = bitPos % 8; // LSB first in byte
        if (symbol->dataStream[byteIndex] & (1 << bitInByte)) {
          symbolIndex |= (1 << b); // LSB first in symbol
        }
        bitPos++;
      }

      playSymbol(symbol->symDefData[symbolIndex], symbol->NPD, pulseCount);
      if (stopOrPauseRequest())
        return;
    }
    SerialHW.println("Data stream done, total pulses: " + String(pulseCount));
  }

  void playSymbol(tSymDef symDef, int maxPulses, int &pulseCount) {
    // Reproducir un símbolo: aplicar flags y reproducir pulsos
    uint8_t flags = symDef.symbolFlag;
    bool polarityChange = true;
    if ((flags & 0x03) == 0x01)
      polarityChange = false; // same as current level
    else if ((flags & 0x03) == 0x02)
      forzeLowLevel = true; // force low
    else if ((flags & 0x03) == 0x03)
      forzeHighLevel = true; // force high

    for (int p = 0; p < maxPulses; p++) {
      int pulseLen = symDef.pulse_array[p];
      if (pulseLen == 0)
        break; // zero-length pulse terminates
      ADD_ONE_SAMPLE_COMPENSATION = false;
      semiPulse((double)pulseLen);
      // polarityChange = !polarityChange; // alternate for next pulse
      pulseCount++;
    }
  }

  void playCustomSymbol(int pulsewidth, int repeat) {
    //
    // Esto lo usamos para el GENERALIZA DATA BLOCK ID-19
    //
    for (int i = 0; i < repeat; i++) {
      // Generamos los semipulsos
      ADD_ONE_SAMPLE_COMPENSATION = false;
      semiPulse((double)pulsewidth);
      ACU_ERROR = 0;
    }
  }

  void playData(uint8_t *bBlock, int lenBlock, int pulse_len, int num_pulses) {
    //
    // Este procedimiento es usado por TAP
    //

    // Inicializamos el estado de la progress bar de bloque.
    PROGRESS_BAR_BLOCK_VALUE = 0;

    // Put now code block
    // syncronize with short leader tone
    // long t1=millis();
    ADD_ONE_SAMPLE_COMPENSATION = false;
    pilotTone(pulse_len, num_pulses);
    // long t2=millis();

    // logln("Pilot tone time: " + String(t2-t1));

    // log("Pilot tone");
    if (LOADING_STATE == 2) {
      return;
    }

    // syncronization for end short leader tone
    syncTone(SYNC1);
    // log("SYNC 1");
    if (LOADING_STATE == 2) {
      return;
    }

    syncTone(SYNC2);
    // log("SYNC 2");
    if (LOADING_STATE == 2) {
      return;
    }

    // Send data
    // sendDataArray(bBlock, lenBlock);
    sendDataArray(bBlock, lenBlock, true);
    // log("Send DATA");
    if (LOADING_STATE == 2) {
      return;
    }

    // Silent tone
    ACU_ERROR = 0;
    silence(silent);

    // log("Send SILENCE");
    if (LOADING_STATE == 2) {
      return;
    }

    // log("Fin del PLAY");
  }

  // ✅ FUNCIÓN playDRBlock MODIFICADA PARA MANEJAR EL ÚLTIMO BYTE
  void playDRBlock2(uint8_t *bBlock, int size, bool isThelastDataPart) {
    if (!bBlock || size == 0)
      return;

    // 1. Calcular cuántos bits totales procesar en este trozo
    int total_bits_to_process = size * 8;

    // 2. Si este es el último trozo Y la máscara es válida (1-7),
    //    recalculamos los bits a procesar.
    if (isThelastDataPart && _mask_last_byte > 0 && _mask_last_byte < 8) {
      // Bits a procesar = (todos los bytes menos el último) * 8 + los bits de
      // la máscara
      total_bits_to_process = ((size - 1) * 8) + _mask_last_byte;
    }

    // =====================================================
    // OPTIMIZACIÓN: Usar buffer para escribir en chunks
    // en lugar de llamar a sampleDR() para cada bit
    // =====================================================
    const int BUFFER_SAMPLES = 512; // Número de muestras por chunk
    const int BUFFER_BYTES =
        BUFFER_SAMPLES * 2 * channels; // 2 bytes por muestra, stereo
    uint8_t audio_buffer[BUFFER_BYTES];
    int16_t *ptr = (int16_t *)audio_buffer;
    int samples_in_buffer = 0;

    // Pre-calcular amplitudes ajustadas por volumen
    uint16_t sample_up_R =
        (uint16_t)(maxLevelUp * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    uint16_t sample_up_L =
        (uint16_t)(maxLevelUp * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));
    uint16_t sample_down_R =
        (uint16_t)(maxLevelDown * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    uint16_t sample_down_L =
        (uint16_t)(maxLevelDown * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));

    // Ajustar canal L si ACTIVE_AMP está activo
    if (ACTIVE_AMP) {
      sample_up_L = sample_up_L * EN_SPEAKER;
      sample_down_L = sample_down_L * EN_SPEAKER;
    }

    int current_bit_count = 0;

    // 3. Iterar por cada byte del trozo
    for (int byte_idx = 0; byte_idx < size; byte_idx++) {
      uint8_t current_byte = bBlock[byte_idx];

      // 4. Iterar por cada bit del byte (de MSB a LSB)
      for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
        // 5. Si ya hemos procesado todos los bits necesarios, salir del bucle.
        if (current_bit_count >= total_bits_to_process) {
          break;
        }

        // Obtenemos el bit actual
        uint8_t bit_value = (current_byte >> bit_idx) & 1;

        // Añadir muestra al buffer
        if (bit_value == 1) {
          *ptr++ = sample_up_R;
          *ptr++ = sample_up_L;
        } else {
          *ptr++ = sample_down_R;
          *ptr++ = sample_down_L;
        }

        samples_in_buffer++;
        current_bit_count++;

        // Cuando el buffer está lleno, escribir al stream
        if (samples_in_buffer >= BUFFER_SAMPLES) {
          int bytes_to_write = samples_in_buffer * 2 * channels;

          if (OUT_TO_WAV) {
            encoderOutWAV.write(audio_buffer, bytes_to_write);
          } else {
            kitStream.write(audio_buffer, bytes_to_write);
          }

          // Reset buffer
          ptr = (int16_t *)audio_buffer;
          samples_in_buffer = 0;

          // Check stop/pause menos frecuentemente
          if (stopOrPauseRequest())
            return;
        }
      }

      if (current_bit_count >= total_bits_to_process) {
        break;
      }
    }

    // Escribir muestras restantes en el buffer
    if (samples_in_buffer > 0) {
      int bytes_to_write = samples_in_buffer * 2 * channels;

      if (OUT_TO_WAV) {
        encoderOutWAV.write(audio_buffer, bytes_to_write);
      } else {
        kitStream.write(audio_buffer, bytes_to_write);
      }
    }
  }

  void playDRBlock(uint8_t *bBlock, int size, bool isThelastDataPart) {
    // Aqui ponemos el byte de mascara del bloque ID 0x15 -
    // importante porque esto dice cuantos bits de cada bytes se reproducen
    uint8_t _mask = 8;
    uint8_t bRead = 0x00;
    int bytes_in_this_block = 0;
    int ptrOffset = 0;

    for (int i = 0; i < size; i++) {
      // Por cada byte creamos un createPulse
      bRead = bBlock[i];
      // Descomponemos el byte

      // log("Dato: " + String(i) + " - " + String(data[i]));

      // Para la protección con mascara ID 0x11 - 0x0C
      // ---------------------------------------------
      // "Used bits in the last uint8_t (other bits should be 0) {8}
      //(e.g. if this is 6, then the bits used (x) in the last uint8_t are:
      // xxxxxx00, wh///ere MSb is the leftmost bit, LSb is the rightmost bit)"

      // ¿Es el ultimo BYTE?. Si se ha aplicado mascara entonces
      // se modifica el numero de bits a transmitir

      if (LOADING_STATE == 1 || TEST_RUNNING) {
        // Vemos si es el ultimo byte de la ultima partición de datos del bloque
        if ((i == size - 1) && isThelastDataPart) {
          // Aplicamos la mascara
          _mask = _mask_last_byte;
        } else {
          // En otro caso todo el byte es valido.
          // se anula la mascara.
          _mask = 8;
        }

#ifdef DEBUGMODE
        logln("Audio is Out");
        logln("Mask: " + String(_mask));
#endif

        // Ahora vamos a descomponer el byte leido
        // y le aplicamos la mascara. Es decir SOLO SE TRANSMITE el nº de bits
        // que indica la mascara, para el último byte del bloque

        for (int n = 0; n < _mask; n++) {
          // Obtenemos el bit a transmitir
          uint8_t bitMasked = bitRead(bRead, 7 - n);

          // Si el bit leido del BYTE es un "1"
          if (bitMasked == 1) {
            // bit "1"
            sampleDR(1, maxLevelUp);
          } else {
            // bit "0"
            sampleDR(1, maxLevelDown);
          }
        }

        // Hemos cargado +1 byte. Seguimos
        if (!TEST_RUNNING) {
          BYTES_LOADED++;
          bytes_in_this_block++;
          BYTES_LAST_BLOCK = bytes_in_this_block;
        }
      } else {
        return;
      }

      ptrOffset = i;
      // Esto lo hacemos para asegurarnos que la barra se llena entera
      PROGRESS_BAR_TOTAL_VALUE =
          ((PRG_BAR_OFFSET_INI + (ptrOffset + 1)) * 100) / BYTES_TOBE_LOAD;
      PROGRESS_BAR_BLOCK_VALUE =
          ((PRG_BAR_OFFSET_INI + (ptrOffset + 1)) * 100) / PRG_BAR_OFFSET_END;
    }
  }

  void playPureData(uint8_t *bBlock, int lenBlock) {
    // Se usa para reproducir los datos del ultimo bloque o bloque completo sin
    // particionar, ID 0x14. por tanto deben meter un terminador al final.
    sendDataArray(bBlock, lenBlock, true);

    if (LOADING_STATE == 2) {
      return;
    }

    // ******************************************************************************
    // CORRECCIÓN TZX v1.20: Manejo correcto de pausas para Pure Data (ID 0x14)
    // ******************************************************************************
    // Según la especificación, si hay pausa > 0:
    // 1. Generar 1ms del nivel OPUESTO al último para completar el último
    // flanco
    // 2. Luego el resto de la pausa en nivel LOW
    // Si pausa == 0, NO cambiar el nivel actual
    // ******************************************************************************

    if (silent > 0) {
      // Silent tone (silence() ya maneja el terminador y nivel LOW)
      ACU_ERROR = 0;
      silence(silent);

      if (LOADING_STATE == 2) {
        return;
      }
    }
  }

  void playDataPartition(uint8_t *bBlock, int lenBlock) {
    // Se envia a reproducir el bloque sin terminador ni silencio
    // ya que es parte de un bloque completo que ha sido partido en varios
    sendDataArray(bBlock, lenBlock, false);

    if (LOADING_STATE == 2) {
      return;
    }
  }

  void playDataBegin(uint8_t *bBlock, int lenBlock, int pulse_len,
                     int num_pulses) {
    // PROGRAM
    // double duration = tState * pulse_pilot_duration;
    // syncronize with short leader tone
    ADD_ONE_SAMPLE_COMPENSATION = false;
    pilotTone(pulse_len, num_pulses);
    // syncronization for end short leader tone
    syncTone(SYNC1);
    syncTone(SYNC2);

    // Send data
    sendDataArray(bBlock, lenBlock, false);
  }

  void playDataEnd(uint8_t *bBlock, int lenBlock) {
    // Send data
    sendDataArray(bBlock, lenBlock, true);

    // Silent tone
    ACU_ERROR = 0;
    silence(silent);
  }

  void set_HMI(HMI hmi) { _hmi = hmi; }

  void samplingtest(float samplingrate) {
    // Se genera un pulso de tono guia
    // kitStream.setPAPower(ACTIVE_AMP);
    // kitStream.setVolume(MAIN_VOL / 100);

    logln("Waiting for test");

    // volvemos a la pagina de menu
    _hmi.writeString("page tape0");

    STOP = false;
    _hmi.writeString("g0.txt=\"Wait ...\"");
    delay(5000);
    logln("Starting test");
    _hmi.writeString("g0.txt=\"Test signal playing. Press STOP for end\"");
    // Generamos una onda de 256 muestras
    int bytes = 0;
    int samples = 256;
    uint16_t sample_L = 0;
    uint16_t sample_R = 0;

    sample_R = (uint16_t)(32768 * (MAIN_VOL_R / 100.0) * (MAIN_VOL / 100.0));
    sample_L = (uint16_t)(32768 * (MAIN_VOL_L / 100.0) * (MAIN_VOL / 100.0));

    bytes = samples * 2 * channels;

    while (!STOP) {
      // Lanzamos 10 pulsos de 256 muestras
      // tono
      samples = 60;
      bytes = samples * 2 * channels;
      for (int i = 0; i < 8063; i++) {
        createPulse(samples, bytes, sample_R, sample_L);
        sample_R *= -1;
        sample_L *= -1;
      }

      // Silencio 3458ms
      samples = 256;
      bytes = samples * 2 * channels;
      for (int i = 0; i < 1296; i++) {
        createPulse(samples, bytes, sample_R, sample_L);
      }

      samples = 192;
      bytes = samples * 2 * channels;
      createPulse(samples, bytes, sample_R, sample_L);

      sample_R *= -1;
      sample_L *= -1;
    }
    _hmi.writeString("g0.txt=\"Press EJECT to select a file or REC\"");
    STOP = true;
    PLAY = false;
    PAUSE = false;
    STOP_OR_PAUSE_REQUEST = false;
  }

  // Constructor
  ZXProcessor() {
    // Constructor de la clase
    ACU_ERROR = 0;
  }
};
