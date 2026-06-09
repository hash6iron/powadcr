/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: ZX80Processor.h

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

    Descripción:
    Clase que implementa soporte para generación de señales ZX80/ZX81 cassette files (.O, .P, .80, .81).
    
    Estrategia de integración: Usa ZXProcessor externamente para reutilizar infraestructura
    de generación de pulsos (createPulse). ZX80Processor solo define timing y parámetros.
    
    El formato ZX80/ZX81 cassette:
    - ZX81 files (.81, .P): Se cargan desde 4009h, end address en 4014h
    - ZX80 files (.80, .O): Se cargan desde 4000h, end address en 400Ah
    - .P y .O files pueden contener basura (28-38 bytes o múltiplos de 128)
    - Señal: 0-bit = 4 pulsos (1200µs), 1-bit = 9 pulsos (2700µs)
    - Pulse: 150µs HIGH + 150µs LOW
    - Silencio entre bits: 1300µs
    - Baud rate: ~400 bps (0s) a 250 bps (1s), promedio ~307 bps

    Version: 1.0

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

#include <math.h>
#include "config.h"

#pragma once

/**
 * ZX80Processor - Soporte para reproducción ZX80/ZX81 cassette files
 * 
 * Clase ligera que define timing y parámetros ZX80/ZX81.
 * El playback real se maneja con ZXProcessor externo.
 */
class ZX80Processor {

private:
  // ============================================================================
  // ZX80/ZX81 CASSETTE TIMING PARAMETERS
  // ============================================================================
  
  // Pulse: 150µs HIGH + 150µs LOW (total 300µs per pulse)
  const double zx_pulseWidth = 150.0;          // µs per half-pulse
  const double zx_silenceWidth = 1300.0;       // µs silence between bits
  
  // Bit encoding
  const int zx_zeroPulses = 4;                 // 0-bit = 4 pulses (1200µs)
  const int zx_onePulses = 9;                  // 1-bit = 9 pulses (2700µs)
  
  // Memory addresses
  const uint16_t zx81_loadAddr = 0x4009;       // ZX81 load address
  const uint16_t zx81_endAddrPtr = 0x4014;     // ZX81 end address pointer
  const uint16_t zx80_loadAddr = 0x4000;       // ZX80 load address
  const uint16_t zx80_endAddrPtr = 0x400A;     // ZX80 end address pointer

public:

  ZX80Processor() { }

  /**
   * Detecta si el archivo es ZX81 (.P, .81) o ZX80 (.O, .80)
   * Por ahora asume ZX81 por defecto; podría mejorarse con heurística
   */
  bool isZX81(const char* filename = nullptr) {
    if (!filename) return true; // Default ZX81
    String fname(filename);
    fname.toLowerCase();
    return fname.endsWith(".p") || fname.endsWith(".81");
  }

  /**
   * Calcula el tamaño de datos válido eliminando basura
   * Lee el end address desde la memoria cargada
   */
  int calculateValidSize(uint8_t *buffer, int bufferSize, bool isZX81) {
    if (!buffer || bufferSize < 16) return 0;
    
    uint16_t endAddrPtr = isZX81 ? zx81_endAddrPtr : zx80_endAddrPtr;
    uint16_t loadAddr = isZX81 ? zx81_loadAddr : zx80_loadAddr;
    
    // Lee end address desde la ubicación correcta (little-endian)
    // Offset dentro del buffer: endAddrPtr - loadAddr
    int offset = (int)(endAddrPtr - loadAddr);
    
    if (offset + 2 > bufferSize) return bufferSize; // Fallback si no alcanza
    
    uint16_t endAddr = (uint16_t)(buffer[offset] | (buffer[offset + 1] << 8));
    
    // Calcula el tamaño válido
    int validSize = (int)(endAddr - loadAddr);
    
    // Limita a 16KB (máximo típico para ZX80/81)
    if (validSize > 16384) validSize = 16384;
    if (validSize <= 0) validSize = bufferSize; // Fallback
    
    return min(validSize, bufferSize);
  }

  /**
   * Obtiene número de pulsos para un bit específico
   */
  int getPulsesForBit(uint8_t bit) {
    return (bit == 0) ? zx_zeroPulses : zx_onePulses;
  }

  /**
   * Obtiene ancho de semi-pulso en microsegundos
   */
  double getSemiPulseWidth() {
    return zx_pulseWidth;
  }

  /**
   * Obtiene ancho de silencio entre bits en microsegundos
   */
  double getSilenceWidth() {
    return zx_silenceWidth;
  }

  /**
   * Convierte microsegundos a muestras de audio
   */
  int microsecondsToSamples(double microSeconds) {
    double seconds = microSeconds * 1e-6;
    return (int)round(seconds * SAMPLING_RATE);
  }

  /**
   * Obtiene información de timing ZX80/ZX81
   */
  String getZX80TimingInfo() {
    String info = "ZX80/ZX81 Cassette Timing:\n";
    info += "Pulse width: " + String(zx_pulseWidth, 0) + " µs\n";
    info += "Silence: " + String(zx_silenceWidth, 0) + " µs\n";
    info += "0-bit: " + String(zx_zeroPulses) + " pulses\n";
    info += "1-bit: " + String(zx_onePulses) + " pulses\n";
    return info;
  }
};
