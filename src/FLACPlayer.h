/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: FLACPlayer.h
    
    Descripción:
    Reproductor optimizado de archivos FLAC diseñado para rendimiento máximo
    sin interrupción de audio. Elimina capas genéricas y se enfoca en fluidez
    de reproducción.
    
    Características de optimización:
    - Decodificador FLAC dedicado (sin capas genéricas)
    - Buffers circulares pre-dimensionados
    - Lectura de disco en tarea paralela (no bloquea)
    - Mínimas copias de datos
    - Stack adaptativo (ajusta refresh rate automáticamente)
    - DMA para audio output si disponible
    - Prioridad de tarea en tiempo real

    Versión: 1.0
    Creado por: Senior Dev Team
    
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "AudioTools/AudioCodecs/CodecFLACFoxen.h"

// Declaraciones extern para acceder al stream de audio desde powadcr.cpp
extern AudioBoardStream kitStream;
extern VolumeStream volumeStream;

// ============================================================================
// CONFIGURACIÓN OPTIMIZADA PARA FLAC
// ============================================================================

// Buffers circulares - FLAC necesita procesamiento predecible
#define FLAC_INPUT_BUFFER_SIZE      (32 * 1024)   // Buffer de entrada optimizado
#define FLAC_OUTPUT_BUFFER_SIZE     (16 * 1024)   // Buffer de salida
#define FLAC_PREBUFFER_THRESHOLD    (8 * 1024)    // Umbral mínimo para comenzar
#define FLAC_DECODER_STACK_SIZE     (12 * 1024)   // Stack dedicado para decodificador

// Parámetros de lectura de disco
#define FLAC_DISK_READ_SIZE         (8 * 1024)    // Lectura por chunk de disco
#define FLAC_DISK_READ_QUEUE_SIZE   4             // Queue de lectura
#define FLAC_DISK_PRIORITY          (tskIDLE_PRIORITY + 2)
#define FLAC_FAST_SEEK_STEP_KB      256           // Paso de avance/retroceso rapido dentro de pista

// Configuración de decodificador FLAC
// Nota: FLAC_MAX_BLOCK_SIZE y FLAC_MAX_CHANNELS están definidos en foxen-flac.h
#define FLAC_MAX_CHANNELS           2
#define FLAC_OUTPUT_BITS            16            // 16-bit output

// ============================================================================
// ESTRUCTURA DE CONTROL DE REPRODUCCIÓN FLAC
// ============================================================================

struct FLACPlayState {
    bool playing = false;
    bool paused = false;
    bool stop_requested = false;
    bool error_occurred = false;
    
    uint32_t file_position = 0;
    uint32_t file_size = 0;
    uint32_t bytes_decoded = 0;
    
    uint32_t sample_rate = 44100;
    uint8_t channels = 2;
    uint8_t bits_per_sample = 16;
    
    float playback_position_ms = 0.0f;
    float total_duration_ms = 0.0f;
    
    // Estad´ísticas de rendimiento
    uint32_t buffer_underruns = 0;
    uint32_t decoder_errors = 0;
    uint32_t avg_decoder_time_us = 0;
    
    String error_message = "";
    String current_file = "";
};

// ============================================================================
// GESTOR DE LISTA DE REPRODUCCIÓN FLAC
// ============================================================================

class FLACPlayList {
private:
    std::vector<String> tracks;
    
public:
    int current_track_index = -1;
    String playlist_path = "";
    
public:
    FLACPlayList() {}
    void openBlockMediaBrowser();

    // ============================================================================
    // FUNCIÓN DE CONVERSIÓN: FLACPlayList → tAudioList[]
    // ============================================================================
    // Convierte una playlist del FLACPlayer a un array de tAudioList para pasar
    // a openBlockMediaBrowser()
    
    // Cargar todos los ficheros .FLAC de una ruta
    bool loadPlaylist(const String& directory_path) {
        tracks.clear();
        current_track_index = -1;
        playlist_path = directory_path;
        
        File dir = SD_MMC.open(directory_path);
        if (!dir || !dir.isDirectory()) {
            log_error("FLAC","No se puede abrir directorio: " + directory_path);
            return false;
        }
        
        File file = dir.openNextFile();
        while (file) {
            String filename = file.name();
            
            // Buscar archivos .flac (case-insensitive)
            if (filename.endsWith(".flac") || filename.endsWith(".FLAC")) {
                String full_path = directory_path + "/" + filename;
                tracks.push_back(full_path);
                log_info("FLAC","Track added: " + full_path);
            }
            file = dir.openNextFile();
        }
        dir.close();
        
        log_info("FLAC","Playlist loaded: " + String(tracks.size()) + " tracks");
        return tracks.size() > 0;
    }
    
    // Establecer pista inicial (comparación case-insensitive)
    bool setCurrentTrack(const String& filepath) {
        String filepath_lower = filepath;
        filepath_lower.toLowerCase();
        
        for (int i = 0; i < tracks.size(); i++) {
            String track_lower = tracks[i];
            track_lower.toLowerCase();
            
            if (track_lower == filepath_lower) {
                current_track_index = i;
                logln("[FLAC] Track found at index " + String(i) + ": " + tracks[i]);
                return true;
            }
        }
        return false;
    }
    
    // Obtener pista actual
    String getCurrentTrack() {
        if (current_track_index >= 0 && current_track_index < tracks.size()) {
            return tracks[current_track_index];
        }
        return "";
    }
    
    // Siguiente pista
    String nextTrack() {
        if (tracks.size() == 0) return "";
        current_track_index = (current_track_index + 1) % tracks.size();
        return tracks[current_track_index];
    }
    
    // Pista anterior
    String previousTrack() {
        if (tracks.size() == 0) return "";
        current_track_index--;
        if (current_track_index < 0) {
            current_track_index = tracks.size() - 1;
        }
        return tracks[current_track_index];
    }
    
    // Ir a pista específica - Valida índice y selecciona primera si está fuera de rango
    String goToTrack(int index) {
        // Validar que el índice esté dentro del rango
        if (index < 0 || index >= tracks.size()) {
            // Si está fuera de rango, seleccionar el primero
            logln("[FLAC] ⚠ Índice " + String(index) + " fuera de rango [0," + 
                  String(tracks.size() - 1) + "]. Seleccionando primera pista.");
            
            if (tracks.size() == 0) {
                return "";  // No hay tracks
            }
            current_track_index = 0;
            return tracks[0];
        }
        
        // Índice válido - seleccionar
        current_track_index = index;
        logln("[FLAC] Seleccionando track #" + String(index + 1) + ": " + tracks[index]);
        return tracks[current_track_index];
    }
    
    // Total de pistas
    int getTotalTracks() {
        return tracks.size();
    }
    
    // Índice de pista actual (1-based para mostrar al usuario)
    int getCurrentTrackIndex() {
        return current_track_index + 1;  // Mostrar 1-based
    }
    
    // Obtener nombre de archivo sin ruta
    String getCurrentTrackName() {
        String track = getCurrentTrack();
        int last_slash = track.lastIndexOf('/');
        if (last_slash >= 0) {
            return track.substring(last_slash + 1);
        }
        return track;
    }
    
    // Obtener track en posición específica (0-based)
    String getTrackAt(int index) {
        if (index < 0 || index >= tracks.size()) {
            return "";
        }
        return tracks[index];
    }
    
    // Obtener nombre de archivo de una ruta
    String getTrackName(const String& full_path) {
        int last_slash = full_path.lastIndexOf('/');
        if (last_slash >= 0) {
            return full_path.substring(last_slash + 1);
        }
        return full_path;
    }
};

// ============================================================================
// BUFFER CIRCULAR OPTIMIZADO
// ============================================================================

template<size_t SIZE>
class CircularBuffer {
private:
    uint8_t buffer[SIZE];
    volatile size_t read_pos = 0;
    volatile size_t write_pos = 0;
    SemaphoreHandle_t mutex;
    
public:
    CircularBuffer() {
        mutex = xSemaphoreCreateMutex();
    }
    
    ~CircularBuffer() {
        if (mutex) vSemaphoreDelete(mutex);
    }
    
    size_t available() const {
        size_t w = write_pos;
        size_t r = read_pos;
        return (w >= r) ? (w - r) : (SIZE - r + w);
    }
    
    size_t free() const {
        return SIZE - available() - 1;
    }
    
    size_t write(const uint8_t* data, size_t len) {
        if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) return 0;
        
        size_t space = free();
        size_t to_write = (len > space) ? space : len;
        
        size_t w = write_pos;
        size_t chunk1 = (w + to_write > SIZE) ? (SIZE - w) : to_write;
        
        memcpy(buffer + w, data, chunk1);
        if (chunk1 < to_write) {
            memcpy(buffer, data + chunk1, to_write - chunk1);
        }
        
        write_pos = (w + to_write) % SIZE;
        xSemaphoreGive(mutex);
        
        return to_write;
    }
    
    size_t read(uint8_t* data, size_t len) {
        if (!xSemaphoreTake(mutex, pdMS_TO_TICKS(100))) return 0;
        
        size_t avail = available();
        size_t to_read = (len > avail) ? avail : len;
        
        size_t r = read_pos;
        size_t chunk1 = (r + to_read > SIZE) ? (SIZE - r) : to_read;
        
        memcpy(data, buffer + r, chunk1);
        if (chunk1 < to_read) {
            memcpy(data + chunk1, buffer, to_read - chunk1);
        }
        
        read_pos = (r + to_read) % SIZE;
        xSemaphoreGive(mutex);
        
        return to_read;
    }
    
    void clear() {
        // Intentar limpiar con retry logic en caso de mutex problemático
        uint8_t retry_count = 0;
        while (retry_count < 3) {
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50))) {
                read_pos = write_pos = 0;
                xSemaphoreGive(mutex);
                return;  // Éxito
            }
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        // Si los 3 intentos fallan, hacer clear forzado (riesgoso pero mejor que nada)
        log_error("FLAC", "CircularBuffer.clear(): Mutex no disponible, haciendo clear forzado");
        read_pos = write_pos = 0;  // Clear sin mutex (último recurso)
    }
};

// ============================================================================
// TAREA DE LECTURA DE DISCO (Sin bloquear)
// ============================================================================

struct DiskReadTask {
    File* file = nullptr;
    CircularBuffer<FLAC_INPUT_BUFFER_SIZE>* input_buffer = nullptr;
    TaskHandle_t task_handle = nullptr;
    volatile bool running = false;
    volatile bool should_exit = false;  // Señal para salida segura
    
    static void diskReaderThread(void* param) {
        DiskReadTask* self = (DiskReadTask*)param;
        uint8_t read_buffer[FLAC_DISK_READ_SIZE];
        uint32_t last_position = 0;
        
        while (self->running && !self->should_exit) {
            // Lectura no bloqueante si el buffer tiene espacio
            if (self->file && self->input_buffer->free() > FLAC_DISK_READ_SIZE) {
                size_t bytes_read = self->file->read(read_buffer, FLAC_DISK_READ_SIZE);
                if (bytes_read > 0) {
                    self->input_buffer->write(read_buffer, bytes_read);
                    last_position += bytes_read;
                } else {
                    // EOF o error - esperar a que se procese el buffer
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }
        
        // Notificar que la tarea está terminando
        self->task_handle = nullptr;  // Indicar que terminó
        vTaskDelete(nullptr);
    }
};

// ============================================================================
// REPRODUCTOR FLAC OPTIMIZADO
// ============================================================================

class OptimizedFLACPlayer {
private:
    audio_tools::FLACDecoderFoxen decoder;
    CircularBuffer<FLAC_INPUT_BUFFER_SIZE>* input_buffer = nullptr;
    CircularBuffer<FLAC_OUTPUT_BUFFER_SIZE>* output_buffer = nullptr;
    
    DiskReadTask disk_reader;
    FLACPlayState state;
    
    FLACPlayList playlist;  // Lista de reproducción
    
    volatile bool seeking_in_progress = false;  // Lock para evitar seeks concurrentes
    
    uint32_t last_update_ms = 0;
    uint32_t decoder_iterations = 0;
    uint32_t last_decoder_time_us = 0;
    uint32_t last_rwd_ffwd_time = 0;  // Para detectar pulsación mantenida
    bool was_rwd_pressed = false;
    bool was_ffwd_pressed = false;
    
    // Estadísticas de velocidad
    struct {
        uint32_t samples_decoded = 0;
        uint32_t sample_rate = 44100;
        uint32_t timestamp_ms = 0;
    } speed_metrics;

public:
    File current_file;  // Accesible en toda la clase
    OptimizedFLACPlayer() {
        input_buffer = new CircularBuffer<FLAC_INPUT_BUFFER_SIZE>();
        output_buffer = new CircularBuffer<FLAC_OUTPUT_BUFFER_SIZE>();
    }
    
    ~OptimizedFLACPlayer() {
        stop();
        if (input_buffer) delete input_buffer;
        if (output_buffer) delete output_buffer;
    }
    
    // ========================================================================
    // Inicialización
    // ========================================================================
    
    bool begin() {
        log_info("FLAC","Inicializando reproductor optimizado...");
        
        // Configurar decodificador con parámetros optimizados
        decoder.setMaxBlockSize(FLAC_MAX_BLOCK_SIZE);
        decoder.setMaxChannels(FLAC_MAX_CHANNELS);
        decoder.setInBufferSize(FLAC_INPUT_BUFFER_SIZE / 2);
        decoder.setOutBufferSize(FLAC_OUTPUT_BUFFER_SIZE / 4);
        decoder.set32Bit(false);  // 16-bit output
        
        // CRÍTICO: Configurar el stream de salida para el decodificador
        decoder.setOutput(volumeStream);
        
        if (!decoder.begin()) {
            log_error("FLAC","No se pudo inicializar decodificador");
            state.error_message = "Decoder initialization failed";
            return false;
        }
        
        // Iniciar prioridad de tarea en tiempo real
        vTaskPrioritySet(nullptr, tskIDLE_PRIORITY + 3);
        
        log_info("FLAC","Reproductor inicializado correctamente");
        return true;
    }
    
    // ========================================================================
    // Reproducción
    // ========================================================================
    
    bool play(const String& filepath) {
        log_info("FLAC","Iniciando reproducción: " + filepath);
        
        // REINICIALIZAR decodificador para nueva reproducción
        decoder.end();
        delay(50);
        decoder.setMaxBlockSize(FLAC_MAX_BLOCK_SIZE);
        decoder.setMaxChannels(FLAC_MAX_CHANNELS);
        decoder.setInBufferSize(FLAC_INPUT_BUFFER_SIZE / 2);
        decoder.setOutBufferSize(FLAC_OUTPUT_BUFFER_SIZE / 4);
        decoder.set32Bit(false);
        decoder.setOutput(volumeStream);
        
        if (!decoder.begin()) {
            log_error("FLAC","No se pudo reinicializar decodificador");
            return false;
        }
        
        // Abrir archivo
        current_file = SD_MMC.open(filepath.c_str(), FILE_READ);
        if (!current_file) {
            log_error("FLAC","No se pudo abrir archivo: " + filepath);
            state.error_message = "Cannot open file: " + filepath;
            return false;
        }
        
        state.current_file = filepath;
        state.file_size = current_file.size();
        state.file_position = 0;
        state.bytes_decoded = 0;
        state.playing = true;
        state.paused = false;
        state.stop_requested = false;
        state.error_occurred = false;
        
        // Limpiar buffers
        input_buffer->clear();
        output_buffer->clear();
        decoder_iterations = 0;
        
        // Iniciar tarea de lectura de disco
        startDiskReader();
        
        // Pre-buffer antes de decodificar
        preBufferData();
        
        log_info("FLAC","Reproducción iniciada - Tamaño: " + String(state.file_size) + " bytes");
        
        return true;
    }
    
    // ========================================================================
    // Bucle de decodificación - Debe llamarse lo más frecuentemente posible
    // ========================================================================
    
    void decode() {
        if (!state.playing || state.error_occurred) return;
        
        // NO detener si estamos en medio de un seek
        if (seeking_in_progress) return;
        
        uint32_t decode_start_us = micros();
        
        // Decodificar en lotes pequeños para mantener fluidez y permitir que disk_reader reabastezca
        uint8_t decode_chunk[512];
        size_t bytes_available = input_buffer->available();
        
        if (bytes_available >= FLAC_PREBUFFER_THRESHOLD) {
            // Decodificar conservadoramente: máximo 512 bytes por llamada
            size_t bytes_to_decode = bytes_available > 512 ? 512 : bytes_available;
            
            size_t bytes_read = input_buffer->read(decode_chunk, bytes_to_decode);
            
            if (bytes_read > 0) {
                // Decodificar los datos - esto actualiza el output buffer internamente
                size_t decoded = decoder.write(decode_chunk, bytes_read);
                state.bytes_decoded += decoded;
                state.file_position += decoded;
                
                // Calcular estadísticas
                uint32_t decode_time_us = micros() - decode_start_us;
                last_decoder_time_us = decode_time_us;
                
                // Promedio móvil de tiempo de decodificación
                state.avg_decoder_time_us = 
                    (state.avg_decoder_time_us * 9 + decode_time_us) / 10;
                
                decoder_iterations++;
            }
        } else if (bytes_available == 0 && !disk_reader.running && state.playing) {
            // Buffer vacío Y disk_reader inactivo Y file size validado = EOF real
            if (state.file_position >= state.file_size - 1) {
                state.playing = false;
            }
            // Si no, solo es un buffer vacío temporal - continuar esperando
        }
    }
    
    // ========================================================================
    // Control
    // ========================================================================
    
    void pause() {
        state.paused = true;
        log_info("FLAC","Reproducción pausada");
    }
    
    void resume() {
        state.paused = false;
        log_info("FLAC","Reproducción reanudada");
    }
    
    void stop() {
        log_info("FLAC","Deteniendo reproducción");
        state.stop_requested = true;
        state.playing = false;
        
        // Detener lectura de disco
        stopDiskReader();
        
        // Finalizar decodificador
        decoder.end();
        
        // Cerrar archivo
        if (current_file) {
            current_file.close();
        }
        
        // Limpiar buffers
        input_buffer->clear();
        output_buffer->clear();
        
        log_info("FLAC","Reproducción detenida");
    }

    void openBlockMediaBrowser() 
    {

        int totalblocks = 0;
        
        totalblocks = playlist.getTotalTracks();

        int max = MAX_BLOCKS_IN_BROWSER;
        if (totalblocks > max) {
            max = MAX_BLOCKS_IN_BROWSER;
        } else {
            max = totalblocks - 1;
        }
        BB_BROWSER_MAX = max;

        // Paso 0: Información general
        BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;

        myNex.writeStr("mp3browser.path.txt",HMI_FNAME);
        myNex.writeStr("mp3browser.totalBl.txt",String(totalblocks - 1));
        myNex.writeStr("mp3browser.bbpag.txt",String(BB_PAGE_SELECTED));
        myNex.writeStr("mp3browser.size0.txt","SIZE[MB]");

        double ctpage = (double)totalblocks / (double)MAX_BLOCKS_IN_BROWSER;
        int totalPages = trunc(ctpage);
        if ((totalblocks % MAX_BLOCKS_IN_BROWSER != 0) && ctpage > 1) {
            totalPages += 1;
        }
        //
        myNex.writeStr("mp3browser.totalPag.txt",String(totalPages));

        int pos = 1;
        //
        for (int i = BB_PTR_ITEM; i <= BB_PTR_ITEM + MAX_BLOCKS_IN_BROWSER; i++)
        {
            if (i <= BB_BROWSER_MAX) 
            {
                String name = playlist.getTrackAt(i);
                myNex.writeStr("mp3browser.id" + String(pos) + ".txt",String(i));
                myNex.writeStr("mp3browser.name" + String(pos) + ".txt",playlist.getTrackName(name));

                pos++;
            }            
        }

        // Si hemos terminado, reseteamos flags
        BB_OPEN = false;
        BB_UPDATE = false;
    }    

    bool seekToPosition(uint32_t target_pos) {
        // APROXIMACIÓN SIMPLIFICADA: Usar libfoxenflac decoder.copy()
        // El decoder maneja sincronización automáticamente
        
        // Validaciones básicas
        if (!current_file || state.file_size == 0 || target_pos >= state.file_size) {
            return false;
        }
        
        // Evitar seeks concurrentes
        if (seeking_in_progress) {
            return false;
        }
        
        seeking_in_progress = true;
                    
        // ====================================================================
        // FASE 3: SEEK AL ARCHIVO OBJETIVO
        // ====================================================================
        if (!current_file.seek(target_pos)) {
            log_error("FLAC", "Seek falló");
            seeking_in_progress = false;
            return false;
        }
        
        // Actualizar posición actual
        state.file_position = target_pos;
        state.bytes_decoded = target_pos;
              
        seeking_in_progress = false;
                
        return true;
    }

    // ========================================================================
    // BÚSQUEDA RÁPIDA OPTIMIZADA PARA FLAC
    // ========================================================================
    
    // Configuración de búsqueda: 5 segundos de audio por salto
    // Ajustable según UX preferida (reducir para más precisión, aumentar para saltos mayores)
    static constexpr uint32_t SEEK_STEP_MS = 5000;  // 5 segundos
    
    // Estimar bitrate promedio del archivo (en bits/segundo)
    // Basado en: file_size [bytes] / duración [segundos] * 8
    uint32_t estimateBitrate() const {
        if (state.file_size == 0 || state.sample_rate == 0 || state.channels == 0) {
            return 128000;  // Default fallback: 128 kbps
        }
        
        // Bitrate teórico: sample_rate * channels * bits_per_sample
        uint32_t theoretical_bitrate = state.sample_rate * state.channels * state.bits_per_sample;
        
        // FLAC típicamente comprime 40-60%, ajustar según análisis real
        // Para precisión, usar: file_size / number_of_samples * sample_rate * 8
        // Aproximación pragmática:
        return (theoretical_bitrate * 50) / 100;  // Asumir 50% de compresión típica
    }
    
    // Convertir milisegundos de audio a bytes en el archivo
    // Fórmula: bytes = (bitrate_bps / 8) * (duration_ms / 1000)
    uint32_t durationToBytes(uint32_t duration_ms) const {
        if (duration_ms == 0) return 0;
        
        uint32_t bitrate = estimateBitrate();
        // bytes = (bitrate_bits_per_sec / 8) * (ms / 1000)
        // = (bitrate / 8000) * ms
        uint64_t bytes = ((uint64_t)bitrate * duration_ms) / 8000;
        
        return (uint32_t)bytes;
    }
    
    // ========================================================================
    // Estado y estadísticas
    // ========================================================================
    
    FLACPlayState getState() const {
        return state;
    }
    
    bool isPlaying() const {
        // Retorna true mientras esté reproduciendo o en pausa (decoder sigue activo)
        return state.playing && !state.stop_requested;
    }
    
    bool isPaused() const {
        return state.paused;
    }
    
    uint32_t getBufferAvailable() const {
        return input_buffer->available();
    }
    
    uint32_t getBufferFree() const {
        return input_buffer->free();
    }
    
    uint32_t getProgress() const {
        if (state.file_size == 0) return 0;
        return (state.file_position * 100) / state.file_size;
    }
    


    String getStatsReport() const {
        String report = "\n[FLAC] ===== ESTADÍSTICAS =====\n";
        report += "Estado: " + String(state.playing ? "Reproduciendo" : "Detenido") + "\n";
        report += "Archivo: " + state.current_file + "\n";
        report += "Progreso: " + String(getProgress()) + "%\n";
        report += "Buffer disponible: " + String(getBufferAvailable()) + " bytes\n";
        report += "Bytes decodificados: " + String(state.bytes_decoded) + "\n";
        report += "Iteraciones decodificador: " + String(decoder_iterations) + "\n";
        report += "Tiempo promedio decodificación: " + String(state.avg_decoder_time_us) + " µs\n";
        report += "Frecuencia de muestreo: " + String(state.sample_rate) + " Hz\n";
        report += "Canales: " + String(state.channels) + "\n";
        report += "Buffer underruns: " + String(state.buffer_underruns) + "\n";
        report += "Errores: " + String(state.decoder_errors) + "\n";
        report += "============================\n";
        
        return report;
    }
    
    // ========================================================================
    // CONTROL DE LISTA DE REPRODUCCIÓN
    // ========================================================================
    
    bool initializePlaylist(const String& filepath) {
        // Extraer directorio de la ruta del archivo
        int last_slash = filepath.lastIndexOf('/');
        String directory = (last_slash >= 0) ? filepath.substring(0, last_slash) : "/";
        
        log_info("FLAC","Cargando playlist de directorio: " + directory);
        
        // Cargar playlist desde ese directorio
        if (!playlist.loadPlaylist(directory)) {
            log_error("FLAC","No se pudo cargar playlist de: " + directory);
            return false;
        }
        
        log_info("FLAC","Intentando encontrar archivo: " + filepath);
        
        // Establecer pista actual - intenta match exacto primero
        if (playlist.setCurrentTrack(filepath)) {
            log_info("FLAC","✓ Archivo encontrado en playlist");
            return true;
        }
        
        // Si no encuentra, usar la primera pista por defecto
        log_info("FLAC","⚠ Archivo no encontrado, usando primera pista");
        playlist.current_track_index = 0;  // Forzar a la primera
        return true;
    }
    
    String getNextTrack() {
        return playlist.nextTrack();
    }
    
    String getPreviousTrack() {
        return playlist.previousTrack();
    }
    
    int getCurrentTrackNumber() {
        return playlist.getCurrentTrackIndex();
    }
    
    int getTotalTracks() {
        return playlist.getTotalTracks();
    }
    
    // ========================================================================
    // FUNCIÓN DE CONVERSIÓN: Playlist FLAC → array tAudioList[]
    // ========================================================================
    // Convierte la playlist actual del FLACPlayer a un array de tAudioList
    // para pasar a openBlockMediaBrowser()   
    String getCurrentTrackName() {
        return playlist.getCurrentTrackName();
    }
    
    // Método para seleccionar un track específico por índice (0-based)
    // Si el índice está fuera de rango, selecciona el primero automáticamente
    String selectTrack(int track_index_0based) {
        String track = playlist.goToTrack(track_index_0based);
        
        if (track.isEmpty()) {
            log_error("FLAC","No hay tracks disponibles en la playlist");
            return "";
        }
        
        log_info("FLAC","Track seleccionado: #" + String(playlist.getCurrentTrackIndex()) + 
              " - " + playlist.getCurrentTrackName());
        
        return track;
    }
    
    String selectTrackByFilepath(const String& filepath) {
        if (playlist.setCurrentTrack(filepath)) {
            log_info("FLAC","Track seleccionado por filepath: " + filepath);
            return playlist.getCurrentTrack();
        }
        
        log_error("FLAC","No se encontró el track para filepath: " + filepath);
        return "";
    }

    // Versión con índice 1-based (más intuitiva para el usuario)
    String selectTrack1Based(int track_number_1based) {
        return selectTrack(track_number_1based - 1);
    }
    
private:
    // ========================================================================
    // Métodos privados de optimización
    // ========================================================================
    
    void preBufferData() {
        // Pre-cargar datos en el buffer antes de comenzar la decodificación
        log_info("FLAC","Pre-bufferizando datos (objetivo: " + String(FLAC_PREBUFFER_THRESHOLD * 2) + " bytes)...");
        
        uint32_t target_buffer_size = FLAC_PREBUFFER_THRESHOLD * 2;  // 16KB objetivo
        uint32_t prebuffer_attempts = 0;
        
        // Llenar el buffer más agresivamente
        while (input_buffer->available() < target_buffer_size && prebuffer_attempts < 100) {
            uint8_t chunk[FLAC_DISK_READ_SIZE];
            size_t bytes = current_file.read(chunk, FLAC_DISK_READ_SIZE);
            
            if (bytes > 0) {
                input_buffer->write(chunk, bytes);
                log_info("FLAC","Pre-buffer: +" + String(bytes) + " bytes, total: " + String(input_buffer->available()));
            } else {
                log_info("FLAC","EOF alcanzado durante pre-buffer");
                break;
            }
            
            prebuffer_attempts++;
            delay(2);  // Pequeña pausa
        }
        
        log_info("FLAC","Pre-buffer completado: " + String(input_buffer->available()) + 
              " bytes disponibles en " + String(prebuffer_attempts) + " intentos");
    }
    
    void startDiskReader() {
        disk_reader.file = &current_file;
        disk_reader.input_buffer = input_buffer;
        disk_reader.running = true;
        
        // Crear tarea con prioridad alta (pero no interrumpe audio)
        xTaskCreatePinnedToCore(
            DiskReadTask::diskReaderThread,
            "FLACDiskReader",
            FLAC_DECODER_STACK_SIZE,
            &disk_reader,
            FLAC_DISK_PRIORITY,
            &disk_reader.task_handle,
            0  // Core 0 (dejar Core 1 para audio)
        );
    }
    
    void stopDiskReader() {
        // Señalizar salida segura sin usar vTaskDelete desde afuera
        disk_reader.running = false;
        disk_reader.should_exit = true;
        
        // Esperar a que la tarea se auto-termine (máximo 100ms)
        uint32_t timeout_ms = 100;
        uint32_t start_ms = millis();
        
        while (disk_reader.task_handle != nullptr && millis() - start_ms < timeout_ms) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
        
        // Si la tarea aún no terminó (muy raro), forzar delete como último recurso
        if (disk_reader.task_handle) {
            log_error("FLAC", "stopDiskReader: Forzando vTaskDelete() después de timeout");
            vTaskDelete(disk_reader.task_handle);
            disk_reader.task_handle = nullptr;
        }
        
        disk_reader.should_exit = false;  // Resetear para próximo start
    }
};

// ============================================================================
// FUNCIÓN PÚBLICA OPTIMIZADA
// ============================================================================

/**
 * @brief Reproductor FLAC optimizado para máximo rendimiento
 * 
 * Esta función reemplaza a MediaPlayer() para archivos FLAC exclusivamente,
 * eliminando capas genéricas y enfocándose en:
 * - Fluidez de audio (buffer circular, lectura async)
 * - Mínima latencia de decodificación
 * - CPU optimizado (evita cambios innecesarios de contexto)
 * - Estadísticas de rendimiento en tiempo real
 */
void FLACPlayer() {
    
    OptimizedFLACPlayer player;
    String current_playing_file = "";
    bool file_prepared = false;
    uint8_t stausFastFFWind = 0;  // 0=ninguno, 1=FFWD, 2=RWD
    uint8_t stausFastRWind = 0;  // 0=ninguno, 1=FFWD, 2=RWD
    bool lastWasFastFFWind = false;
    bool lastWasFastRWind = false;
    
    // Inicializar reproductor
    if (!player.begin()) {
        log_info("FLAC","ERROR: Fallo en inicialización");
        LAST_MESSAGE = "FLAC initialization error";
        STOP = true;
        PLAY = false;
        return;
    }
    
    MEDIA_PLAYER_EN = true;
    MUSIC_IS_PLAYING = true;

    PLAY = false;
    PAUSE = false;
    EJECT = false;
    STOP = true;  // Comenzar en STOP
    
    // BUCLE EXTERNO: Permite reiniciar con nuevo archivo sin salir de FLACPlayer()
    while (!EJECT && !REC && MEDIA_PLAYER_EN) {
        
        // Inicializar lista de reproducción desde el directorio del archivo seleccionado
        logln("[FLAC] Inicializando playlist para: " + PATH_FILE_TO_LOAD);
        if (!player.initializePlaylist(PATH_FILE_TO_LOAD)) {
            log_info("FLAC","ERROR: No se pudo crear playlist de: " + PATH_FILE_TO_LOAD);
            LAST_MESSAGE = "Cannot load FLAC playlist";
            break;
        }
        
        logln("[FLAC] Playlist cargada: " + String(player.getTotalTracks()) + " pistas");
        myNex.writeNum("tape.totalBlocks.val", player.getTotalTracks());
        myNex.writeNum("tape.currentBlock.val", player.getTotalTracks());

        // Preparar archivo pero NO reproducir aún
        current_playing_file = PATH_FILE_TO_LOAD;
        if (!player.play(PATH_FILE_TO_LOAD)) {
            log_info("FLAC","ERROR: No se pudo abrir archivo: " + PATH_FILE_TO_LOAD);
            LAST_MESSAGE = "Cannot open FLAC file";
            break;
        }
        
        file_prepared = true;
        
        // Mostrar información inicial
        File f = SD_MMC.open(PATH_FILE_TO_LOAD);
        uint32_t file_size = f.size();
        f.close();
        
        log_info("FLAC","Pista: " + String(player.getCurrentTrackNumber()) + "/" + 
              String(player.getTotalTracks()) + " - " + player.getCurrentTrackName());
        log_info("FLAC","Tamaño: " + String(file_size / 1024) + " KB");
        
        myNex.writeNum("tape.totalBlocks.val", player.getTotalTracks());
        myNex.writeNum("tape.currentBlock.val", player.getCurrentTrackNumber());
       
        // Actualizar HMI con información inicial
        myNex.writeStr("tape.name.txt", player.getCurrentTrackName());

        if (file_size < (1024 * 1024)) {
            myNex.writeStr("tape.size.txt", String(file_size / 1024) + " KB");
        } else { 
            myNex.writeStr("tape.size.txt", String(file_size / (1024 * 1024)) + " MB");
        }
                
        LAST_MESSAGE = "FLAC: Ready to play. Press PLAY to start.";
        
        // ====================================================================
        // BUCLE PRINCIPAL DE REPRODUCCIÓN PARA ESTA PISTA
        // ====================================================================
        
        uint32_t last_update_hmi = millis();
        uint32_t decoder_iterations = 0;
        bool track_changed = false;
        bool is_currently_playing = false;  // Controlar si está realmente reproduciendo
        uint32_t last_rwd_ffwd_time = 0;  // ✅ Contador para KEEP_RWIND/KEEP_FFWIND
        
        logln("[FLAC] Archivo preparado. Esperando PLAY. isPlaying=" + String(player.isPlaying()) + 
              " isPaused=" + String(player.isPaused()));
        
        uint8_t playerStatus = 0;

        while (!EJECT && !REC && MEDIA_PLAYER_EN && !track_changed && PATH_FILE_TO_LOAD == current_playing_file) {
            
            // ====================================================================
            // CONTROL DE REPRODUCCIÓN (PLAY, PAUSE, STOP)
            // ====================================================================
            switch (playerStatus) {
                case 0:  // STOPPED
                    if (PLAY) 
                    {
                        playerStatus = 1;  // Cambiar a PLAYING
                        player.resume();
                        is_currently_playing = true;
                        tapeAnimationON();  // ✅ Activar animación del cassette
                    }
                    break;
                case 1:  // PLAYING
                    if (PAUSE) {
                        playerStatus = 2;       // Cambiar a PAUSED
                        player.pause();
                        PAUSE = false;
                        tapeAnimationOFF();     // ✅ Detener animación
                    } else if (STOP) {
                        playerStatus = 0;  // Cambiar a STOPPED
                        player.stop();
                        tapeAnimationOFF();  // ✅ Detener animación
                        is_currently_playing = false;
                        decoder_iterations = 0;
                        PATH_FILE_TO_LOAD = player.selectTrack1Based(1);  // Volver al primer track;                        
                        track_changed = true;  // Forzar reinicio de bucle externo
                        break;
                    }
                    // ✅ DETECCIÓN DE FIN DE TRACK - AUTO-ADVANCE
                    else if (player.getProgress() >= 100) {
                        log_info("FLAC","Track finalizado - Progreso: " + String(player.getProgress()) + "%");
                        
                        int current_track_num = player.getCurrentTrackNumber();  // 1-based
                        int total_tracks = player.getTotalTracks();
                        
                        log_info("FLAC","Track " + String(current_track_num) + " de " + String(total_tracks) + " completado");
                        
                        // Si no es el último track, ir al siguiente
                        if (current_track_num < total_tracks) {
                            log_info("FLAC","Auto-advance: Pasando al siguiente track");
                            String next_track = player.getNextTrack();
                            PATH_FILE_TO_LOAD = next_track;
                            track_changed = true;
                            break;  // Reiniciar bucle externo con nuevo track
                        }
                        // Es el último track
                        else {
                            if (disable_auto_media_stop) {
                                // Loop: reiniciar desde el principio
                                log_info("FLAC","Último track finalizado - Mode LOOP activado. Reiniciando playlist...");
                                String first_track = player.selectTrack1Based(1);  // Volver al primer track
                                PATH_FILE_TO_LOAD = first_track;
                                track_changed = true;
                                break;  // Reiniciar bucle externo
                            } else {
                                // Parar: fin de playlist
                                log_info("FLAC","Último track finalizado - Mode STOP. Deteniendo reproducción...");
                                player.stop();
                                tapeAnimationOFF();
                                playerStatus = 0;
                                is_currently_playing = false;
                                STOP = true;
                                PLAY = false;
                                LAST_MESSAGE = "Playlist finished";
                                // Dejamos preparado en el primer track de la playlist
                                String first_track = player.selectTrack1Based(1);  // Volver al primer track                                
                                PATH_FILE_TO_LOAD = first_track;
                                track_changed = true;
                                break;
                                // Quedamos en STOPPED esperando nuevo PLAY
                            }
                        }
                    }
                    
                    player.decode();  // Continuar decodificando mientras se reproduce
                    decoder_iterations++;
                    break;
                case 2:  // PAUSED
                    if (PLAY || PAUSE) {
                        PAUSE = false;          // Resetear bandera de pausa
                        playerStatus = 1;       // Cambiar a PLAYING
                        player.resume();
                        tapeAnimationON();      // ✅ Activar animación del cassette
                    } else if (STOP) {
                        playerStatus = 0;  // Cambiar a STOPPED
                        player.stop();
                        tapeAnimationOFF();  // ✅ Detener animación
                        is_currently_playing = false;
                        decoder_iterations = 0;
                        player.selectTrack(0);  // Reiniciar a la primera pista
                        break;
                    }
                    break;
            }
          
                        
            // ====================================================================
            // CONTROL DE NAVEGACIÓN (FFWIND, RWIND, KEEP_FFWIND, KEEP_RWIND)
            // ====================================================================
            
            // RWIND - Pista anterior (presión simple de RWD)
            if ((RWIND && !FFWIND) || (KEEP_RWIND && !KEEP_FFWIND))
            {
                if (RWIND && !lastWasFastRWind) 
                {
                    String prev_track = player.getPreviousTrack();
                    if (!prev_track.isEmpty()) {
                        logln("[FLAC] RWD: Cambiando a pista anterior");
                        //player.stop();
                        PATH_FILE_TO_LOAD = prev_track;
                        RWIND = false;
                        track_changed = true;
                        break;  // Salir para reiniciar reproducción
                    }
                    RWIND = false;
                    KEEP_RWIND = false;  // Resetear bandera de KEEP_RWIND si no hay pista anterior
                    KEEP_FFWIND = false;  // Resetear bandera de KEEP_FFWIND si no hay pista anterior
                }
                else {
                    // Entramos en modo RWIND - Tecla mantenida
                    switch (stausFastRWind) {
                        case 0:  // Primer ciclo de KEEP_FFWIND
                        {
                            if (KEEP_RWIND)
                            {
                                stausFastRWind = 1;
                                log_info("FLAC","RWD: Modo fast rewind activado");
                                // Borramos este flag para evitar cambio de track al soltar el boton.
                            }
                        }
                        break;
                        case 1:  // Segundo ciclo de KEEP_FFWIND
                        {
                            // Salimos cuando se manda un RWIND que es que se ha soltado la tecla                        
                            if (RWIND) {
                                stausFastRWind = 0;
                                RWIND = false;
                                KEEP_RWIND = false;  // Resetear bandera de KEEP_RWIND al soltar la tecla
                                KEEP_FFWIND = false;  // Resetear bandera de KEEP_FFWIND al soltar la tecla
                                FFWIND = false;  // Asegurar que no estamos en fast forward
                                lastWasFastRWind = false;   // Marcar que ya no estamos en fast rewind
                                lastWasFastFFWind = false;   // Asegurar que no estamos en fast forward
                                log_info("FLAC","RWD: Tecla soltada, saliendo de modo fast rewind");
                            }
                            else if (KEEP_RWIND)
                            {
                                // KEEP_RWIND - Avance rápido dentro de la pista actual
                                uint32_t now_ffwd = millis();
                                lastWasFastRWind = true;    // Marcar que estamos en fast rewind

                                if (now_ffwd - last_rwd_ffwd_time > 50) {
                                    last_rwd_ffwd_time = now_ffwd;


                                    if (player.getProgress() > 0) {
            
                                        size_t p_file_seek_pos = player.getState().file_position;
                                        if (p_file_seek_pos >= 0) {
                                            player.seekToPosition(p_file_seek_pos - 1024 * 128);  // Sincronizar el reproductor con el nuevo position del archivo
                                        }
                                    }

                                    PROGRESS_BAR_TOTAL_VALUE = player.getProgress();                            
                                }
                                
                            }
                        }
                        break;
                        case 3:  // Exit KEEP_RWIND
                        {
                            if (RWIND)
                            {
                                stausFastRWind = 0;
                                RWIND = false;
                            }
                        }
                        break;
                    }
                }
                            
            }
            else if ((FFWIND && !RWIND) || (KEEP_FFWIND && !KEEP_RWIND))
            {
                // FFWIND - Siguiente pista (presión simple de FFWD)
                if (FFWIND && !lastWasFastFFWind) {
                    String next_track = player.getNextTrack();
                    if (!next_track.isEmpty()) {
                        log_info("FLAC","FFWD: Cambiando a siguiente pista");
                        //player.stop();
                        PATH_FILE_TO_LOAD = next_track;
                        FFWIND = false;
                        track_changed = true;
                        break;  // Salir para reiniciar reproducción
                    }
                    FFWIND = false;
                    KEEP_RWIND = false;  // Resetear bandera de KEEP_RWIND si no hay pista anterior
                    KEEP_FFWIND = false;  // Resetear bandera de KEEP_FFWIND si no hay pista anterior
                }            
                else {
                    // Entramos en modo fastwind - Tecla mantenida
                    switch (stausFastFFWind) {
                        case 0:  // Primer ciclo de KEEP_FFWIND
                        {
                            if (KEEP_FFWIND)
                            {
                                stausFastFFWind = 1;
                                // Borramos este flag para evitar cambio de track al soltar el boton.                            
                            }
                        }
                        break;
                        case 1:  // Segundo ciclo de KEEP_FFWIND
                        {
                            // Salimos cuando se manda un FWIND que es que se ha soltado la tecla
                            if (FFWIND) {
                                stausFastFFWind = 0;
                                FFWIND = false;
                                KEEP_FFWIND = false;  // Resetear bandera de KEEP_FFWIND al soltar la tecla
                                lastWasFastFFWind = false;  // Marcar que ya no estamos en fast forward

                            }
                            else if (KEEP_FFWIND)
                            {
                                // KEEP_FFWIND - Avance rápido dentro de la pista actual
                                uint32_t now_ffwd = millis();
                                lastWasFastFFWind = true;   // Marcar que estamos en fast forward

                                if (now_ffwd - last_rwd_ffwd_time > 50) {
                                    last_rwd_ffwd_time = now_ffwd;

                                    // Controlamos los limites (alcanzar 90%)
                                    if (player.getProgress() <= 90) 
                                    {
                                        size_t p_file_seek_pos = player.getState().file_position;
                                        if (p_file_seek_pos <= player.getState().file_size) {
                                            player.seekToPosition(p_file_seek_pos + 1024 * 128);
                                        }  
                                    }

                                    PROGRESS_BAR_TOTAL_VALUE = player.getProgress();                            
                                }                            
                            }
                        }
                        break;
                        case 3:  // Exit KEEP_FFWIND
                        {
                            if (FFWIND)
                            {
                                stausFastFFWind = 0;
                                FFWIND = false;
                            }
                        }
                        break;  
                    }
                }
                
            }

            // Seleccion de pista con Block Browser
            if (BB_OPEN || BB_UPDATE) {
                log_info("FLAC","Abriendo Block Media Browser - BB_OPEN=" + String(BB_OPEN) + " BB_UPDATE=" + String(BB_UPDATE));
                player.openBlockMediaBrowser();
            }
            else if (UPDATE_HMI || UPDATE)
            {
                log_info("FLAC","Actualizando HMI - UPDATE_HMI=" + String(UPDATE_HMI) + " UPDATE=" + String(UPDATE));
                UPDATE_HMI = false;
                UPDATE = false;
                log_info("FLAC","Seleccionando pista desde Block Media Browser - BLOCK_SELECTED=" + String(BLOCK_SELECTED));
                player.selectTrack1Based(BLOCK_SELECTED);
                
                //track_changed = true;
            }

            // ====================================================================
            // ACTUALIZAR HMI (cada 2 segundos)
            // ====================================================================
            
            uint32_t now = millis();
            if (now - last_update_hmi > 2000) {
                last_update_hmi = now;
                
                if (is_currently_playing) {
                    // Actualizar progreso
                    PROGRESS_BAR_TOTAL_VALUE = player.getProgress();

                    // Para no repetirlo indefinidamente, solo actualizar si hay cambios significativos
                    if (PROGRESS_BAR_TOTAL_VALUE < 10) 
                    {
                        // Mostrar pista actual y total (BLOCK_SELECTED es 1-based)
                        myNex.writeNum("tape.totalBlocks.val", player.getTotalTracks());
                        myNex.writeNum("tape.currentBlock.val", player.getCurrentTrackNumber());

                        
                        uint32_t size_kb = player.getState().file_size / 1024;
                        myNex.writeStr("tape2.size.txt", String(size_kb) + " KB");
                        
                        // Nombre de la pista
                        myNex.writeStr("tape2.name.txt", player.getCurrentTrackName());
                        
                        // Estado de animación (1=reproduciendo, 0=parado)
                        //myNex.writeNum("tape2.play.val", 1);
                    }
                } else {
                    // Archivo preparado pero no reproduciendo
                    PROGRESS_BAR_TOTAL_VALUE = 0;
                    myNex.writeNum("tape.totalBlocks.val", player.getTotalTracks());
                    myNex.writeNum("tape.currentBlock.val", player.getCurrentTrackNumber());
                                        
                    // Mostrar información del archivo parado
                    uint32_t size_kb = player.getState().file_size / 1024;
                    myNex.writeStr("tape2.size.txt", String(size_kb) + " KB");
                    
                    myNex.writeStr("tape2.name.txt", player.getCurrentTrackName());
                    //myNex.writeNum("tape2.play.val", 0);  // 0=parado
                }
            }
                        
            // Yield para otras tareas (pero minimamente)
            if (decoder_iterations % 10 == 0) {
                vTaskDelay(1);  // Yield al sistema
            }
        }
        
        // Si se cambió de pista, reiniciar el loop externo
        if (track_changed && PATH_FILE_TO_LOAD != current_playing_file) {
            log_info("FLAC","Reabriendo nuevo archivo...");
            player.stop();
            continue;  // Volver al inicio del bucle externo
        }
    }
    
    // Limpieza final
    player.stop();
    MEDIA_PLAYER_EN = false;
    MUSIC_IS_PLAYING = false;
    
    log_info("FLAC","Reproductor finalizado");
    log_info("FLAC", player.getStatsReport());
}

// ============================================================================
// VARIABLES GLOBALES AUXILIARES (para ser accesibles desde powadcr.cpp)
// ============================================================================

extern bool PLAY;
extern bool PAUSE;
extern bool STOP;
extern bool EJECT;
extern bool REC;
extern bool FFWIND;
extern bool RWIND;
extern bool KEEP_FFWIND;
extern bool KEEP_RWIND;
extern bool PLAY;
extern bool PAUSE;
extern bool STOP;
extern bool EJECT;
extern bool REC;
extern bool MEDIA_PLAYER_EN;
extern bool MUSIC_IS_PLAYING;
extern bool EQ_CHANGE;
extern bool AMP_CHANGE;
extern bool SPK_CHANGE;
extern bool VOL_CHANGE;
extern float MAIN_VOL;
extern float EQ_LOW, EQ_MID, EQ_HIGH;
extern bool ACTIVE_AMP;
extern bool EN_SPEAKER;
extern bool disable_auto_media_stop;  // 
extern String PATH_FILE_TO_LOAD;
extern String LAST_MESSAGE;
extern int PROGRESS_BAR_TOTAL_VALUE;
