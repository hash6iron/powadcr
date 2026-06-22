# FLACPlayer() - Checklist de Verificación

## 📋 Pre-Compilación

- [ ] **Archivos en lugar correcto**
  - [ ] `src/FLACPlayer.h` existe
  - [ ] `src/powadcr.cpp` modificado
  - [ ] Headers de AudioTools disponibles

- [ ] **Dependencias disponibles**
  - [ ] `AudioTools` instalado en `lib/`
  - [ ] `libfoxenflac` en `lib/libfoxenflac/`
  - [ ] `CodecFLACFoxen.h` accesible
  - [ ] FreeRTOS headers disponibles

- [ ] **Memoria suficiente**
  - [ ] RAM disponible: > 50KB (buffers)
  - [ ] FLASH disponible: > 30KB (código)
  - [ ] PSRAM si es necesario

---

## 🔨 Compilación

- [ ] **Build sin errores**
  ```bash
  pio run -e esp32devCOM
  ```
  Resultado esperado: 
  ```
  ✓ Compiling .../src/powadcr.cpp
  ✓ Linking .../firmware.elf
  ✓ Building .../firmware.bin
  ```

- [ ] **Sin errores en FLACPlayer.h**
  - [ ] Syntax correcto (llaves balanceadas)
  - [ ] Includes resueltos
  - [ ] Template specializations ok

- [ ] **Sin errores en cambios powadcr.cpp**
  - [ ] `#include "FLACPlayer.h"` reconocido
  - [ ] case 'f' compilable
  - [ ] Variables globales accesibles

- [ ] **Warnings evaluados**
  - [ ] Variables sin usar: OK (normales)
  - [ ] Warnings críticos: 0
  - [ ] Errores de link: 0

---

## 📤 Upload

- [ ] **Conexión Serial**
  - [ ] COM port detectado (ej: COM11)
  - [ ] Baudrate: 115200
  - [ ] Conexión estable

- [ ] **Upload exitoso**
  ```
  ✓ Image successfully written to flash
  ✓ Resetting to run the new image
  ```

- [ ] **ESP32 reinicia**
  - [ ] Pantalla HMI se muestra
  - [ ] Sistema listo para usar
  - [ ] No reinicia indefinidamente

---

## 🎵 Reproducción de FLAC

### Preparación

- [ ] **Archivo FLAC disponible**
  - [ ] Ruta: `/musica/test.flac`
  - [ ] Formato: FLAC 16-bit 44.1kHz
  - [ ] Tamaño: > 1MB (prueba real)

- [ ] **Monitor serie activo**
  - [ ] Terminal abierto (115200 baud)
  - [ ] Logs visibles
  - [ ] Buffer limpio

### Reproducción

- [ ] **Selección de archivo**
  - [ ] HMI muestra archivo FLAC
  - [ ] Navegador reconoce extensión `.flac`
  - [ ] Path correcto en `PATH_FILE_TO_LOAD`

- [ ] **Inicialización**
  - [ ] Log: `[FLAC] Inicializando reproductor...`
  - [ ] Log: `[FLAC] Reproductor inicializado correctamente`
  - [ ] Log: `[FLAC] Iniciando reproducción`

- [ ] **Pre-buffer**
  - [ ] Log: `[FLAC] Pre-bufferizando datos...`
  - [ ] Log: `[FLAC] Pre-buffer completado: XXXX bytes`
  - [ ] Buffer ≥ 8KB antes de reproducción

- [ ] **Audio funciona**
  - [ ] Se escucha música
  - [ ] Audio continuo (sin cortes)
  - [ ] Volumen correcto (MAIN_VOL aplicado)
  - [ ] Calidad clara (sin distorsión)

### Control de Reproducción

- [ ] **PLAY**
  - [ ] Inicia reproducción
  - [ ] Audio audible
  - [ ] Barra progreso avanza

- [ ] **PAUSE**
  - [ ] Audio se detiene
  - [ ] Posición mantiene
  - [ ] PLAY resume en mismo punto

- [ ] **STOP**
  - [ ] Audio se detiene
  - [ ] Buffers limpios
  - [ ] Retorna a MEDIA_PLAYER_EN=false

- [ ] **EJECT**
  - [ ] Audio se detiene
  - [ ] Vuelve a menu principal
  - [ ] Sistema estable

- [ ] **VOL UP/DOWN**
  - [ ] Volumen responde inmediatamente
  - [ ] Cambios audibles
  - [ ] Sin clicks de audio

- [ ] **EQ ajustes**
  - [ ] LOW/MID/HIGH responden
  - [ ] Cambios audibles
  - [ ] Sin cortes durante cambio

---

## 📊 Estadísticas y Rendimiento

### Logs Automáticos

- [ ] **Estadísticas cada 10 segundos**
  ```
  [FLAC] ===== ESTADÍSTICAS =====
  Estado: Reproduciendo
  Archivo: /musica/test.flac
  Progreso: XX%
  Buffer disponible: XXXX bytes
  Bytes decodificados: XXXXX
  Iteraciones decodificador: XXXX
  Tiempo promedio decodificación: XXX µs
  Frecuencia de muestreo: 44100 Hz
  Canales: 2
  Buffer underruns: 0
  Errores: 0
  ============================
  ```

### Métricas Esperadas

- [ ] **CPU Utilization**
  - [ ] Core 1 (audio): 40-55%
  - [ ] Core 0 (lectura): 10-20%
  - [ ] Total: < 70%

- [ ] **Decodificación**
  - [ ] Tiempo promedio: 250-400µs
  - [ ] Picos: < 500µs
  - [ ] Consistente (varianza baja)

- [ ] **Buffer**
  - [ ] Disponible: > 8KB (pre-buffer threshold)
  - [ ] Nunca vacío durante reproducción
  - [ ] Utilización: 50-80% normal

- [ ] **Estabilidad**
  - [ ] Buffer underruns: 0-1 en 10 minutos
  - [ ] Errores: 0
  - [ ] Sin reiniciOS inesperados

---

## 🔍 Verificación Avanzada

### Cambio de Pista

- [ ] **Siguiente archivo**
  - [ ] Cargar segundo archivo FLAC
  - [ ] Presionar PLAY
  - [ ] Audio del nuevo archivo suena
  - [ ] Smooth transition (sin pops)

### Reproducción Larga

- [ ] **30+ minutos**
  - [ ] Sistema estable todo el tiempo
  - [ ] No reinicia
  - [ ] Memoria no se desborda
  - [ ] Calidad consistente

### Archivos Variados

- [ ] **Diferentes bitrates**
  - [ ] 128kbps @ 44.1kHz ✓
  - [ ] 320kbps @ 44.1kHz ✓
  - [ ] 192kbps @ 48kHz (si soportado) ✓

- [ ] **Diferentes duraciones**
  - [ ] 30 segundos ✓
  - [ ] 5 minutos ✓
  - [ ] 30+ minutos ✓

### Condiciones de Estrés

- [ ] **CPU alta (ejecutar otras tareas)**
  - [ ] Audio sigue fluido
  - [ ] Underruns < 3/minuto (tolerable)
  - [ ] Sin reiniciOS

- [ ] **Lectura lenta (SD_MMC ocupado)**
  - [ ] Buffer se reduce pero sigue siendo suficiente
  - [ ] Pre-buffer puede demorarse más
  - [ ] Sin cortes de audio

---

## 🐛 Debugging

### Logs en Console

- [ ] **[FLAC] markers visibles**
  - [ ] Todos los eventos importantes logeados
  - [ ] Timestamps coherentes
  - [ ] Información clara y útil

- [ ] **Mensajes de error**
  - [ ] Si hay error: mensaje claro
  - [ ] Línea de código referenciada
  - [ ] Acción recomendada si aplica

### Monitoreo de Memoria

- [ ] **Stack no se desborda**
  - [ ] Monitor heap > 50KB libre
  - [ ] PSRAM estable (si se usa)
  - [ ] Sin fragmentación visible

### Monitoreo de Señal

- [ ] **Audio quality**
  - [ ] Frecuencia correcta (44.1kHz)
  - [ ] Amplitud correcta (no distorsión)
  - [ ] Estéreo correcto (ambos canales)
  - [ ] SNR aceptable (bajo ruido)

---

## ✅ Criterios de Aceptación Final

Todos estos deben estar en ✓:

### Core Functionality
- [ ] FLAC se reproduce sin interrupciones
- [ ] Controles PLAY/PAUSE/STOP funcionan
- [ ] Volumen ajustable
- [ ] EQ funciona
- [ ] Progreso visible

### Performance
- [ ] CPU < 70%
- [ ] Buffer underruns ≈ 0
- [ ] Respuesta < 200ms
- [ ] Audio fluido 100% del tiempo

### Stability
- [ ] Uptime > 60 minutos
- [ ] Sin reiniciOS inesperados
- [ ] Memoria estable
- [ ] Recuperación de errores ok

### User Experience
- [ ] Integración transparente
- [ ] Sin cambios de código requeridos
- [ ] Documentación clara
- [ ] Fácil de usar

---

## 📝 Reporte Final

### Firmado por:
```
Developer: _________________________  Fecha: __________

Tester:    _________________________  Fecha: __________

Lead:      _________________________  Fecha: __________
```

### Estado Final
```
[ ] ✅ LISTO PARA PRODUCCIÓN
[ ] ⚠️  LISTO CON RESTRICCIONES
[ ] ❌ REQUIERE CORRECCIONES
```

### Restricciones (si aplica)
```
_________________________________________________________________

_________________________________________________________________

_________________________________________________________________
```

### Notas Adicionales
```
_________________________________________________________________

_________________________________________________________________

_________________________________________________________________
```

---

## 🔄 Re-testing Periódico

### Semanal
- [ ] Reproducir 10 archivos FLAC diferentes
- [ ] Verificar uptime > 2 horas
- [ ] Revisar logs por errors

### Mensual
- [ ] Testing exhaustivo de todas funciones
- [ ] Stress test de 24+ horas
- [ ] Análisis de performance

### Trimestral
- [ ] Revisión completa de código
- [ ] Optimizaciones si hay margen
- [ ] Actualizar documentación

---

**Versión Checklist**: 1.0  
**Última actualización**: 2026-06-16  
**Responsable**: QA Team
