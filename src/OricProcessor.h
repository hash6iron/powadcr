/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: OricProcessor.h

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

    Descripción:
    Clase que implementa soporte para generación de señales ORIC TAP (Oric-1, Oric Atmos).
    
    Estrategia de integración: Usa ZXProcessor externamente para reutilizar infraestructura
    de generación de pulsos (createPulse, createSample). OricProcessor solo define timing ORIC.
    
    El formato ORIC TAP es más simple que ZX Spectrum:
    - Típicamente un solo bloque por archivo
    - Frecuencia estándar: 1200-2400 bps (SLOW mode)
    - Pulsos SLOW: 0-bit = 208µs LOW + 416µs HIGH (asimétrico), 1-bit = 208µs (simétrico)
    - Soporta modo turbo: 0-bit = 60µs LOW + 470µs HIGH (asimétrico), 1-bit = 60µs (simétrico)
    - Sincronización: bytes 0x16 repetidos + 0x24 (sync)

    Version: 1.0

    Historico de versiones
    ----------------------
    v.1.0 - Version inicial (ORIC TAP support) - Composición instead herencia

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
 * OricProcessor - Soporte para reproducción ORIC TAP
 * 
 * Clase ligera que define timing y parámetros ORIC.
 * NO hereda de ZXProcessor para evitar conflictos con métodos privados.
 * En cambio, se usa como utilidad de configuración y timing.
 * 
 * El playback real se maneja con ZXProcessor externo usando flags ORIC_MODE.
 * 
 * ============================================================================
 * CONTROL DE BAUDRATE
 * ============================================================================
 * Sistema automático de detección y ajuste de baudrate:
 * 
 * - TAPE_BAUDRATE es un multiplicador: [nuevo_baudrate]/1200
 * - ORIC estándar = 2400 bps => TAPE_BAUDRATE = 2.0
 * - ORIC turbo = 3600 bps => TAPE_BAUDRATE = 3.0 (aprox)
 * - Cuando se cambia TAPE_BAUDRATE, el sampling rate se recalcula automáticamente
 * - NO se necesita recalcular timing: todo se basa en el SAMPLING_RATE actual
 * 
 * Flujo:
 * 1. proccesingTAP() detecta ORIC (type == 0x20)
 * 2. Detecta turbo mode y establece TAPE_BAUDRATE (2.0 o 3.0)
 * 3. Antes de reproducir, ZXProcessor usa SAMPLING_RATE con TAPE_BAUDRATE
 * 4. Todos los cálculos de pulsos se hacen con el nuevo SAMPLING_RATE
 */
class OricProcessor {

private:
  // ============================================================================
  // ORIC TAP TIMING PARAMETERS
  // ============================================================================
  // Basados en especificación ORIC-1/Oric Atmos
  // Referencia: MaxDuino, Oric TAP specs PDF

 
  // Modo estándar (300 bps)
  const double oric_zeroLowPulseWidth = 208.0;   // MaxDuino Std 0 LOW
  const double oric_zeroHighPulseWidth = 416.0;  // MaxDuino Std 0 HIGH
  const double oric_onePulseWidth = 208.0;       // MaxDuino Std 1 (Symmetric)
  
  // Modo turbo (2400 bps)
  const double oric_turboZeroLowPulseWidth = 60.0;   // MaxDuino Turbo 0 LOW
  const double oric_turboZeroHighPulseWidth = 470.0; // MaxDuino Turbo 0 HIGH
  const double oric_turboOnePulseWidth = 60.0;       // MaxDuino Turbo 1
   
  // Frecuencia CPU Oric-1 / Oric Atmos
  const double oric_freqCPU = 1000000.0;  // 1 MHz (para cálculos de timing)
  const double oric_tState = (1.0 / oric_freqCPU);

  // Detección de modo turbo
  bool _isTurboMode = false;
  
  // Contador de bytes para detección dinámica de turbo
  int _byteCount = 0;
  static const int TURBO_DETECTION_THRESHOLD = 16; // bytes antes de decidir

public:

  OricProcessor() {
    _isTurboMode = false;
    _byteCount = 0;
  }

  /**
   * Detecta si el bloque ORIC está en modo turbo
   * analizando los pulsos iniciales
   * 
   * @return true si se detecta modo turbo, false si es modo estándar
   */
  bool detectTurboMode(int fileSize, int syncCount) {
    // HEURÍSTICA DE DETECCIÓN:
    // 1. Si el archivo es grande (> 2KB), casi seguro es 2400 baudios (Fast).
    if (fileSize > 2048) {
        return true; 
    }

    // 2. Analizar el preámbulo de sincronismo (0x16).
    // Los archivos de 300 baudios (Slow) tienen preámbulos muy largos.
    // Si hay pocos bytes de sincronismo (< 30), es probable que sea Fast.
    if (syncCount > 0 && syncCount < 30) {
        return true;
    }

    // 3. Fallback: usar el estado actual o por defecto Standard (Slow) 
    // si el archivo es minúsculo y tiene mucho sync.
    return fileSize > 512; 
  }

  /**
   * Obtiene duración de pulso para bit específico en microsegundos
   * 
   * @param bit Valor del bit (0 o 1)
   * @param isTurbo Usar timing turbo si es true
   * @param isHighPhase Si es true devuelve el ancho de la fase HIGH (solo para bit 0)
   * @return Duración en microsegundos
   */
  double getPulseWidthUs(uint8_t bit, bool isTurbo = false, bool isHighPhase = false) {
    if (isTurbo) {
      // Modo FAST: '1' es 2400Hz, '0' es 1200Hz
      return (bit == 1) ? oric_turboOnePulseWidth : (isHighPhase ? oric_turboZeroHighPulseWidth : oric_turboZeroLowPulseWidth);
    } else {
      // Modo SLOW: Frecuencias base
      return (bit == 1) ? oric_onePulseWidth : (isHighPhase ? oric_zeroHighPulseWidth : oric_zeroLowPulseWidth);
    }
  }

  /**
   * Convierte microsegundos a número de muestras de audio
   * 
   * @param pulseWidthUs Ancho de pulso en microsegundos
   * @return Número de muestras de audio
   */
  int pulseUsToSamples(double pulseWidthUs) {
    double pulseSeconds = pulseWidthUs * 1e-6;  // Convertir µs a segundos
    return (int)round(pulseSeconds * SAMPLING_RATE);
  }

  /**
   * Obtiene información de timing ORIC
   */
  String getOricTimingInfo() {
    String info = "ORIC TAP Timing:\n";
    info += "Mode: " + String(_isTurboMode ? "TURBO" : "STANDARD") + "\n";
    info += "Std 0-bit: " + String(oric_zeroLowPulseWidth, 1) + " - " + String(oric_zeroHighPulseWidth, 1) + " µs\n";
    info += "Std 1-bit: " + String(oric_onePulseWidth, 1) + " µs\n";
    info += "Turbo 0-bit: " + String(oric_turboZeroLowPulseWidth, 1) + " - " + String(oric_turboZeroHighPulseWidth, 1) + " µs\n";
    info += "Turbo 1-bit: " + String(oric_turboOnePulseWidth, 1) + " µs\n";
    return info;
  }

  /**
   * Establece modo turbo manualmente
   * 
   * @param turbo true para activar turbo, false para estándar
   */
  void setTurboMode(bool turbo) {
    _isTurboMode = turbo;
    
    
    log_info("ORIC","ORIC turbo mode: " + String(turbo ? "ON" : "OFF"));
    
  }

  /**
   * Obtiene modo turbo actual
   */
  bool getTurboMode() 
  {
    
    return _isTurboMode;
  }

    
  /**
   * Inicializa procesador para bloque ORIC
   */
  void initialize(int fileSize = 0, int syncCount = 0) {
    _byteCount = 0;
    if (fileSize > 0) {
        _isTurboMode = detectTurboMode(fileSize, syncCount);
    }
    
    log_info("ORIC","ORIC Processor initialized. Turbo: " + String(_isTurboMode ? "YES" : "NO"));
  }
};
