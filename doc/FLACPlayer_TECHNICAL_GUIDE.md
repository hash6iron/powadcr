# FLACPlayer() - Reproductor FLAC Optimizado para Máximo Rendimiento

## 📋 Introducción

`FLACPlayer()` es una función especializada y altamente optimizada para la reproducción exclusiva de archivos FLAC en el ESP32. Fue diseñada desde cero para eliminar todas las capas genéricas que introduce `MediaPlayer()` y enfocarse únicamente en la **fluidez de reproducción sin interrupciones**.

### ¿Por qué un reproductor FLAC específico?

- **FLAC es CPU-intensivo**: La decodificación FLAC requiere significativamente más poder de procesamiento que MP3
- **MediaPlayer() es genérico**: Incluye lógica para WAV, MP3 y FLAC, lo que añade overhead innecesario
- **Latencia crítica**: Cualquier interrupción en el buffer causa "audio cuts" audibles
- **Optimización dedicada**: Un reproductor específico permite micro-optimizaciones imposibles en código genérico

---

## ⚙️ Arquitectura Técnica

### Componentes Principales

```
┌─────────────────────────────────────────────────────────────────┐
│                      FLACPlayer()                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────┐    ┌──────────────────────┐          │
│  │  Lectura de Disco    │    │  Decodificación      │          │
│  │  (Tarea FreeRTOS)    │    │  (Núcleo Principal)  │          │
│  │  DiskReadTask        │───▶│  OptimizedFLAC      │          │
│  │  Core 0              │    │  Player              │          │
│  │  Prioridad: IDLE+2   │    │  Core 1              │          │
│  │                      │    │  Prioridad: IDLE+3   │          │
│  └──────────────────────┘    └──────────────────────┘          │
│        │                            │                           │
│        ▼                            ▼                           │
│  ┌──────────────────────┐    ┌──────────────────────┐          │
│  │ CircularBuffer       │    │ CircularBuffer       │          │
│  │ (Input)              │    │ (Output)             │          │
│  │ 32KB                 │    │ 16KB                 │          │
│  └──────────────────────┘    └──────────────────────┘          │
│        │                            │                           │
│        ▼                            ▼                           │
│  ┌──────────────────────────────────────────────────┐          │
│  │     FLACDecoderFoxen (libfoxenflac)             │          │
│  │     Decodificador FLAC optimizado               │          │
│  └──────────────────────────────────────────────────┘          │
│                            │                                    │
│                            ▼                                    │
│  ┌──────────────────────────────────────────────────┐          │
│  │     Audio Output Stream (ES8388)                │          │
│  │     DMA + Ecualizador + Control de Volumen     │          │
│  └──────────────────────────────────────────────────┘          │
│                            │                                    │
│                            ▼                                    │
│                    🔊 ALTAVOZ / AURICULARES                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Flujo de Datos

1. **DiskReadTask** (Tarea FreeRTOS Core 0)
   - Lee `FLAC_DISK_READ_SIZE` (8KB) de disco cuando hay espacio en buffer
   - No bloquea el núcleo de audio (Core 1)
   - Usa semáforos para sincronización thread-safe

2. **CircularBuffer (Input)**
   - Buffer de 32KB pre-dimensionado
   - Almacena datos comprimidos FLAC del disco
   - Umbral de pre-buffer: 8KB mínimo antes de iniciar decodificación

3. **Decodificación**
   - `OptimizedFLACPlayer::decode()` procesa lotes de 512 bytes
   - Se llama lo más frecuentemente posible desde el bucle principal
   - FLACDecoderFoxen convierte datos comprimidos → PCM 16-bit

4. **CircularBuffer (Output)**
   - Buffer de 16KB para datos PCM decodificados
   - Alimenta directamente el stream de audio
   - Ecualizador de 3 bandas (LOW, MID, HIGH)

---

## 🚀 Optimizaciones de Rendimiento

### 1. Lectura Asíncrona de Disco
```cpp
DiskReadTask disk_reader;  // Tarea FreeRTOS independiente
```
- Lee del disco en **Core 0** mientras decodifica en **Core 1**
- No bloquea la cadena de audio crítica
- Usa `file.read()` no bloqueante cuando buffer tiene espacio

### 2. Buffers Circulares Thread-Safe
```cpp
template<size_t SIZE>
class CircularBuffer {
    SemaphoreHandle_t mutex;  // Protección con mutex
    // Lectura/escritura O(1) sin copias innecesarias
};
```
- Evita copias de datos múltiples
- Sincronización con mutexes FreeRTOS
- Timeout de 100ms para no bloquear indefinidamente

### 3. Decodificación Incremental
```cpp
void decode() {
    size_t bytes_to_decode = (bytes_available > 4096) ? 4096 : bytes_available;
    // Procesa en lotes pequeños = latencia predecible
    decoder.write(decode_chunk, bytes_to_decode);
}
```
- Procesa en lotes pequeños (~512 bytes)
- Evita picos de latencia por decodificación masiva
- Permite respuesta rápida a comandos de usuario

### 4. Prioridades en Tiempo Real
```cpp
vTaskPrioritySet(nullptr, tskIDLE_PRIORITY + 3);  // Decodificación: IDLE+3
DiskReadTask::DISK_PRIORITY = tskIDLE_PRIORITY + 2;  // Lectura: IDLE+2
```
- Audio siempre tiene prioridad sobre lectura de disco
- Lectura de disco no interrumpe decodificación
- Núcleos segregados: lectura en Core 0, audio en Core 1

### 5. Estadísticas en Tiempo Real
```cpp
state.avg_decoder_time_us = (state.avg_decoder_time_us * 9 + decode_time_us) / 10;
// Promedio móvil exponencial de tiempo de decodificación
```
- Detecta cuello de botella de rendimiento
- Permite ajuste dinámico en futuras versiones

---

## 📊 Parámetros Configurables

Ajusables en `FLACPlayer.h`:

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| `FLAC_INPUT_BUFFER_SIZE` | 32 KB | Buffer de datos comprimidos |
| `FLAC_OUTPUT_BUFFER_SIZE` | 16 KB | Buffer de PCM decodificado |
| `FLAC_PREBUFFER_THRESHOLD` | 8 KB | Mínimo antes de iniciar reproducción |
| `FLAC_DISK_READ_SIZE` | 8 KB | Tamaño de cada lectura de disco |
| `FLAC_MAX_BLOCK_SIZE` | 16384 | Máximo bloque FLAC |
| `FLAC_MAX_CHANNELS` | 2 | Máximo de canales (stereo) |
| `FLAC_OUTPUT_BITS` | 16 | Bits por muestra (16-bit) |

---

## 🔧 Integración en el Sistema

### 1. Include Automático
```cpp
// En powadcr.cpp (línea ~105)
#include "FLACPlayer.h"
```

### 2. Detección de Archivos FLAC
```cpp
// En MediaPlayer() - case 'f':
if (ext[0] == 'f') {
    FLAC_IS_PLAYING = true;
    FLACPlayer();  // Delegación directa
    FLAC_IS_PLAYING = false;
    return;
}
```

### 3. Variables Globales Requeridas
La función `FLACPlayer()` accede a variables globales definidas en `globales.h`:

```cpp
extern bool EJECT;              // Salida de reproducción
extern bool REC;                // Cambio a modo grabación
extern bool MEDIA_PLAYER_EN;    // Estado del reproductor
extern bool MUSIC_IS_PLAYING;   // Música en reproducción
extern bool EQ_CHANGE;          // Cambio en ecualizador
extern bool AMP_CHANGE;         // Cambio en amplificador
extern bool SPK_CHANGE;         // Cambio en altavoz
extern bool VOLUME_CHANGE;      // Cambio de volumen
extern bool PLAY;               // Play request
extern bool PAUSE;              // Pause request
extern bool STOP;               // Stop request
extern int MAIN_VOL;            // Volumen (0-100)
extern int EQ_LOW, EQ_MID, EQ_HIGH;  // Valores ecualizador
extern String PATH_FILE_TO_LOAD;    // Ruta del archivo FLAC
extern String LAST_MESSAGE;     // Mensaje estado
extern int PROGRESS_BAR_TOTAL_VALUE;  // Progreso (0-100)
```

---

## 📈 Monitoreo de Rendimiento

### Estadísticas Disponibles

La función reporta automáticamente cada 10 segundos:

```
[FLAC] ===== ESTADÍSTICAS =====
Estado: Reproduciendo
Archivo: /musica/song.flac
Progreso: 45%
Buffer disponible: 24576 bytes
Bytes decodificados: 102400
Iteraciones decodificador: 1248
Tiempo promedio decodificación: 320 µs
Frecuencia de muestreo: 44100 Hz
Canales: 2
Buffer underruns: 0
Errores: 0
============================
```

### Acceso Manual a Estadísticas

```cpp
FLACPlayState state = player.getState();
uint32_t buffer_avail = player.getBufferAvailable();
uint32_t progress = player.getProgress();
String report = player.getStatsReport();
```

### Diagnóstico de Problemas

| Síntoma | Causa Probable | Solución |
|---------|---|---|
| Buffer underruns > 0 | Disco lento o CPU saturada | Aumentar `FLAC_INPUT_BUFFER_SIZE` |
| Tiempo decodificación > 500µs | FLAC inusualmente complejo | Reducir bitrate o usar CPU más rápida |
| Audio cortado | Lectura de disco bloqueada | Verificar SD_MMC speed, reducir `FLAC_DISK_READ_SIZE` |
| Erro de inicialización | Memoria insuficiente | Reducir `FLAC_MAX_BLOCK_SIZE` |

---

## 🎯 Casos de Uso

### ✅ Ideal Para
- Reproducción de FLAC en tiempo real
- Audio sin interrupciones
- Sistemas embebidos con recursos limitados
- Aplicaciones que requieren fluidez garantizada

### ❌ No Recomendado Para
- Cambios frecuentes de playlist (usar `MediaPlayer()`)
- Archivos con bitrates extremadamente altos (>500kbps @ 48kHz)
- Decodificadores con configuración no estándar

---

## 🔍 Ejemplos de Uso

### Reproducción Básica

```cpp
void playFLAC() {
    PATH_FILE_TO_LOAD = "/music/song.flac";
    PLAY = true;
    FLACPlayer();  // Se ejecuta hasta que STOP=true
}
```

### Con Control de Usuario

```cpp
void mediaLoop() {
    while (true) {
        FLACPlayer();  // Se bloquea hasta EJECT o REC
        
        if (EJECT) {
            logln("Usuario eyectó");
        }
        if (REC) {
            logln("Cambio a grabación");
        }
        
        delay(100);
    }
}
```

### Acceso a Estadísticas

```cpp
void debugFLAC() {
    // Dentro de FLACPlayer()
    OptimizedFLACPlayer* player = /* get reference */;
    
    if (millis() % 10000 == 0) {
        logln(player->getStatsReport());
        logln("Buffer: " + String(player->getBufferAvailable()) + " bytes");
        logln("Progreso: " + String(player->getProgress()) + "%");
    }
}
```

---

## 🛠️ Troubleshooting

### "Error initializing FLAC decoder"
- **Causa**: Memoria insuficiente para buffers
- **Solución**: Reducir `FLAC_MAX_BLOCK_SIZE` a 8192

### "Buffer underruns detected"
- **Causa**: Lectura de disco lenta
- **Solución**: 
  - Verificar velocidad SD_MMC (idealmente ≥10 MHz)
  - Aumentar `FLAC_INPUT_BUFFER_SIZE` a 64KB
  - Reducir otras tareas activas

### "Audio cutting out randomly"
- **Causa**: Decodificador no puede mantener ritmo
- **Solución**:
  - Utilizar archivos FLAC con bitrate ≤ 500kbps
  - Aumentar `FLAC_DISK_PRIORITY` a `tskIDLE_PRIORITY + 3`
  - Reducir `EQ_LOW`, `EQ_MID`, `EQ_HIGH` (desactivar ecualizador si es posible)

### "File opens but no sound"
- **Causa**: Ruta inválida o audio board no inicializado
- **Solución**:
  - Verificar `PATH_FILE_TO_LOAD` está completo (ej: `/musica/song.flac`)
  - Verificar `kitStream` está inicializado
  - Comprobar conexiones de audio

---

## 📚 Referencias Técnicas

### FLACDecoderFoxen
- Librería: `libfoxenflac`
- Fuente: `lib/libfoxenflac/src/foxen-flac.{c,h}`
- Wrapper AudioTools: `AudioCodecs/CodecFLACFoxen.h`

### Configuración de Audio
- Board: ES8388 (Audiokit)
- Sample rate: 44100 Hz (configurable)
- Bits: 16-bit PCM
- Canales: Stereo (2)

### FreeRTOS
- Core 0: Lectura de disco (DiskReadTask)
- Core 1: Decodificación y reproducción
- Prioridades: IDLE+2 (lectura), IDLE+3 (audio)

---

## 📝 Changelog

### v1.0 (2026-06-16)
- ✅ Versión inicial
- ✅ Buffer circular thread-safe
- ✅ Lectura async de disco
- ✅ Estadísticas en tiempo real
- ✅ Control de ecualizador integrado

### Futuras Mejoras (v1.1+)
- [ ] Seek adelante/atrás (fast-forward/rewind)
- [ ] Crossfade entre canciones
- [ ] Ajuste dinámico de buffer
- [ ] Soporte para 24-bit audio
- [ ] Caché de metadatos FLAC

---

## 👨‍💻 Notas del Desarrollador

**Principio de Diseño**: "Low latency, predictable performance"

Esta función fue diseñada con estos principios:
1. **Eliminar capas genéricas** → Reducir overhead
2. **Buffer circular** → Evitar reallocaciones
3. **Lectura async** → No bloquear audio
4. **Estadísticas** → Diagnóstico automático
5. **Configuración simple** → Solo parámetros esenciales

El código está altamente optimizado pero sigue siendo legible. Los comentarios explican decisiones de diseño no obvias.

---

## 📄 Licencia

Same as PowaDCR project - GNU General Public License v3.0

---

**Versión**: 1.0  
**Autor**: Senior Dev Team  
**Fecha**: 2026-06-16  
**Estado**: Production Ready ✅
