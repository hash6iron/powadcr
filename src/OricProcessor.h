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
    - Frecuencia estándar: 2400 bps
    - Pulsos: 0-bit = 274µs LOW + 274µs HIGH, 1-bit = 140µs
    - Soporta modo turbo: 0-bit = 182µs, 1-bit = 92µs
    - Sincronización: bytes 0x16 0x16 (sync)

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
  
  // Modo estándar (2400 bps)
  const double oric_zeroPulseWidth = 274.0;     // microsegundos para bit 0
  const double oric_onePulseWidth = 140.0;      // microsegundos para bit 1
  
  // Modo turbo (velocidad aumentada)
  const double oric_turboZeroPulseWidth = 182.0;  // Turbo bit 0
  const double oric_turboOnePulseWidth = 92.0;    // Turbo bit 1
  
  // Tono piloto/sync
  const double oric_syncPulseWidth = 200.0;     // Pulsos sincronización
  
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
  bool detectTurboMode() {
    // Heurística: si se detectan pulsos muy cortos de forma consistente,
    // probablemente sea modo turbo
    // Por ahora, suponemos modo estándar por defecto
    // En versiones futuras: analizar primeros N bytes para decidir
    return ORIC_TURBO_MODE;  // Usa flag global de config
  }

  /**
   * Obtiene duración de pulso para bit específico en microsegundos
   * 
   * @param bit Valor del bit (0 o 1)
   * @param isTurbo Usar timing turbo si es true
   * @return Duración en microsegundos
   */
  double getPulseWidthUs(uint8_t bit, bool isTurbo = false) {
    if (isTurbo) {
      return (bit == 0) ? oric_turboZeroPulseWidth : oric_turboOnePulseWidth;
    } else {
      return (bit == 0) ? oric_zeroPulseWidth : oric_onePulseWidth;
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
    info += "Std 0-bit: " + String(oric_zeroPulseWidth, 1) + " µs\n";
    info += "Std 1-bit: " + String(oric_onePulseWidth, 1) + " µs\n";
    info += "Turbo 0-bit: " + String(oric_turboZeroPulseWidth, 1) + " µs\n";
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
    
    #ifdef DEBUGMODE
      logln("ORIC turbo mode: " + String(turbo ? "ON" : "OFF"));
    #endif
  }

  /**
   * Obtiene modo turbo actual
   */
  bool getTurboMode() {
    return _isTurboMode;
  }

  /**
   * Inicializa procesador para bloque ORIC
   */
  void initialize() {
    _byteCount = 0;
    _isTurboMode = detectTurboMode();
    
    #ifdef DEBUGMODE
      logln("ORIC Processor initialized. Turbo: " + String(_isTurboMode ? "YES" : "NO"));
    #endif
  }
};

