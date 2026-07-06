#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/CoreAudio/ResampleStream.h"
#include "AudioTools/CoreAudio/AudioFilter/Equalizer3Bands.h"

// Extern streams from powadcr.cpp
extern AudioBoardStream kitStream;
extern VolumeStream volumeStream;

// ============================================================================
// MP3 optimized settings
// ============================================================================

#define MP3_INPUT_BUFFER_SIZE      (32 * 1024)
#define MP3_OUTPUT_BUFFER_SIZE     (16 * 1024)
#define MP3_PREBUFFER_THRESHOLD    (8 * 1024)
#define MP3_DECODER_STACK_SIZE     (12 * 1024)
#define MP3_CHUNK_BUFFER_SIZE      512

#define MP3_DISK_READ_SIZE         (8 * 1024)
#define MP3_DISK_PRIORITY          (tskIDLE_PRIORITY + 2)

#define MP3_MAX_CHANNELS           2
#define MP3_OUTPUT_BITS            16
#define MP3_DEFAULT_SAMPLE_RATE    44100
#define MP3_OUTPUT_SAMPLE_RATE     96000

struct MP3PlayState {
    bool playing = false;
    bool paused = false;
    bool stop_requested = false;
    bool error_occurred = false;

    uint32_t file_position = 0;
    uint32_t file_size = 0;
    uint32_t bytes_decoded = 0;

    uint32_t sample_rate = MP3_DEFAULT_SAMPLE_RATE;
    uint8_t channels = MP3_MAX_CHANNELS;
    uint8_t bits_per_sample = MP3_OUTPUT_BITS;

    uint32_t buffer_underruns = 0;
    uint32_t decoder_errors = 0;

    String error_message = "";
    String current_file = "";

    bool track_end_reached = false;
};

class MP3PlayList {
private:
    std::vector<String> tracks;

public:
    int current_track_index = -1;
    String playlist_path = "";

    bool loadPlaylist(const String& directory_path) {
        tracks.clear();
        current_track_index = -1;
        playlist_path = directory_path;

        log_info("MP3","Opening index file: " + directory_path + "/idx.txt");
        File fileIdx = SD_MMC.open(directory_path + "/idx.txt", FILE_READ);
        if (!fileIdx) {
            log_error("MP3","Cannot open index file: " + directory_path + "/idx.txt");
            return false;
        }

        while (fileIdx.available()) {
            String filename = fileIdx.readStringUntil('\n');
            filename.trim();

            String fTemp = filename;
            fTemp.toUpperCase();
            if (fTemp.indexOf(".MP3") != -1) {
                tracks.push_back(filename);
                log_info("MP3","Track added: " + filename);
            }
        }
        fileIdx.close();

        log_info("MP3","Playlist loaded: " + String(tracks.size()) + " tracks");
        return tracks.size() > 0;
    }

    bool setCurrentTrack(const String& filepath) {
        String filepath_lower = filepath;
        filepath_lower.toLowerCase();

        for (int i = 0; i < tracks.size(); i++) {
            String track_lower = tracks[i];
            track_lower.toLowerCase();

            if (track_lower == filepath_lower) {
                current_track_index = i;
                return true;
            }
        }
        return false;
    }

    String getCurrentTrack() {
        if (current_track_index >= 0 && current_track_index < tracks.size()) {
            return tracks[current_track_index];
        }
        return "";
    }

    String nextTrack() {
        if (tracks.size() == 0) return "";
        current_track_index = (current_track_index + 1) % tracks.size();
        return tracks[current_track_index];
    }

    String previousTrack() {
        if (tracks.size() == 0) return "";
        current_track_index--;
        if (current_track_index < 0) {
            current_track_index = tracks.size() - 1;
        }
        return tracks[current_track_index];
    }

    String goToTrack(int index) {
        if (index < 0 || index >= tracks.size()) {
            if (tracks.size() == 0) {
                return "";
            }
            current_track_index = 0;
            return tracks[0];
        }

        current_track_index = index;
        return tracks[current_track_index];
    }

    int getTotalTracks() {
        return tracks.size();
    }

    int getCurrentTrackIndex() {
        return current_track_index + 1;
    }

    String getCurrentTrackName() {
        String track = getCurrentTrack();
        int last_slash = track.lastIndexOf('/');
        if (last_slash >= 0) {
            return track.substring(last_slash + 1);
        }
        return track;
    }

    String getTrackAt(int index) {
        if (index < 0 || index >= tracks.size()) {
            return "";
        }
        return tracks[index];
    }

    String getTrackName(const String& full_path) {
        int last_slash = full_path.lastIndexOf('/');
        if (last_slash >= 0) {
            return full_path.substring(last_slash + 1);
        }
        return full_path;
    }
};

template<size_t SIZE>
class MP3CircularBuffer {
private:
    uint8_t buffer[SIZE];
    volatile size_t read_pos = 0;
    volatile size_t write_pos = 0;
    SemaphoreHandle_t mutex;

public:
    MP3CircularBuffer() {
        mutex = xSemaphoreCreateMutex();
    }

    ~MP3CircularBuffer() {
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
        uint8_t retry_count = 0;
        while (retry_count < 3) {
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50))) {
                read_pos = write_pos = 0;
                xSemaphoreGive(mutex);
                return;
            }
            retry_count++;
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        log_error("MP3", "MP3CircularBuffer.clear(): mutex not available, forced clear");
        read_pos = write_pos = 0;
    }
};

struct MP3DiskReadTask {
    File* file = nullptr;
    MP3CircularBuffer<MP3_INPUT_BUFFER_SIZE>* input_buffer = nullptr;
    TaskHandle_t task_handle = nullptr;
    volatile bool running = false;
    volatile bool should_exit = false;

    static void diskReaderThread(void* param) {
        MP3DiskReadTask* self = (MP3DiskReadTask*)param;
        uint8_t read_buffer[MP3_DISK_READ_SIZE];

        while (self->running && !self->should_exit) {
            if (self->file && self->input_buffer->free() > MP3_DISK_READ_SIZE) {
                size_t bytes_read = self->file->read(read_buffer, MP3_DISK_READ_SIZE);
                if (bytes_read > 0) {
                    self->input_buffer->write(read_buffer, bytes_read);
                } else {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            } else {
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }

        self->task_handle = nullptr;
        vTaskDelete(nullptr);
    }
};

class OptimizedMP3Player {
private:
    audio_tools::MP3DecoderHelix decoder;
    audio_tools::ResampleStream resampler;
    audio_tools::Equalizer3Bands eq;
    audio_tools::ConfigEqualizer3Bands cfg_eq;
    MP3CircularBuffer<MP3_INPUT_BUFFER_SIZE>* input_buffer = nullptr;
    MP3CircularBuffer<MP3_OUTPUT_BUFFER_SIZE>* output_buffer = nullptr;

    MP3DiskReadTask disk_reader;
    MP3PlayState state;
    MP3PlayList playlist;

    volatile bool seeking_in_progress = false;
    uint32_t decoder_iterations = 0;
    uint32_t target_output_sample_rate = MP3_OUTPUT_SAMPLE_RATE;
    uint32_t last_input_sample_rate = 0;
    uint8_t last_input_channels = 0;
    uint8_t last_input_bits = 0;
    float last_eq_low = 1.0f;
    float last_eq_mid = 1.0f;
    float last_eq_high = 1.0f;
    bool eq_enabled = false;

    bool isEQNeutral(float low, float mid, float high) const {
        return low == 1.0f && mid == 1.0f && high == 1.0f;
    }

    void updateAudioPathForEQ() {
        if (eq_enabled) {
            resampler.setOutput(eq);
        } else {
            resampler.setOutput(volumeStream);
        }
    }

public:
    File current_file;

    OptimizedMP3Player() : eq(volumeStream) {
        input_buffer = new MP3CircularBuffer<MP3_INPUT_BUFFER_SIZE>();
        output_buffer = new MP3CircularBuffer<MP3_OUTPUT_BUFFER_SIZE>();
    }

    ~OptimizedMP3Player() {
        stop();
        if (input_buffer) delete input_buffer;
        if (output_buffer) delete output_buffer;
    }

    bool begin() {
        log_info("MP3","Initializing optimized player...");

        // Keep DAC output fixed at 96kHz and resample decoded MP3 on-the-fly.
        target_output_sample_rate = MP3_OUTPUT_SAMPLE_RATE;
        AudioInfo i2s_cfg = kitStream.audioInfo();
        if (i2s_cfg.sample_rate > 0) {
            target_output_sample_rate = i2s_cfg.sample_rate;
        }

        AudioInfo mp3_input_cfg;
        mp3_input_cfg.sample_rate = MP3_DEFAULT_SAMPLE_RATE;
        mp3_input_cfg.bits_per_sample = MP3_OUTPUT_BITS;
        mp3_input_cfg.channels = MP3_MAX_CHANNELS;

        AudioInfo eq_info = mp3_input_cfg;
        eq_info.sample_rate = target_output_sample_rate;
        cfg_eq = eq.defaultConfig();
        cfg_eq.setAudioInfo(eq_info);
        cfg_eq.gain_low = EQ_LOW;
        cfg_eq.gain_medium = EQ_MID;
        cfg_eq.gain_high = EQ_HIGH;
        last_eq_low = cfg_eq.gain_low;
        last_eq_mid = cfg_eq.gain_medium;
        last_eq_high = cfg_eq.gain_high;

        eq_enabled = !isEQNeutral(cfg_eq.gain_low, cfg_eq.gain_medium, cfg_eq.gain_high);
        if (eq_enabled) {
            if (!eq.begin(cfg_eq)) {
                log_error("MP3","Cannot initialize equalizer");
                state.error_message = "EQ initialization failed";
                return false;
            }
        }

        // Pipeline: MP3 decoder -> resampler -> EQ (only if active) -> volumeStream
        updateAudioPathForEQ();
        if (!resampler.begin(mp3_input_cfg, (int)target_output_sample_rate)) {
            log_error("MP3","Cannot initialize resampler");
            state.error_message = "Resampler initialization failed";
            return false;
        }

        decoder.setMaxFrameSize(2 * 1024);
        decoder.setMaxPCMSize(8 * 1024);
        decoder.setOutput(resampler);
        decoder.addNotifyAudioChange(resampler);

        if (!decoder.begin()) {
            log_error("MP3","Cannot initialize decoder");
            state.error_message = "Decoder initialization failed";
            return false;
        }

        vTaskPrioritySet(nullptr, tskIDLE_PRIORITY + 3);
        log_info("MP3","Player initialized");
        return true;
    }

    bool play(const String& filepath) {
        log_info("MP3","Starting playback: " + filepath);

        decoder.end();
        delay(30);
        decoder.setOutput(resampler);
        if (!decoder.begin()) {
            log_error("MP3","Cannot restart decoder");
            return false;
        }

        current_file = SD_MMC.open(filepath.c_str(), FILE_READ);
        if (!current_file) {
            log_error("MP3","Cannot open file: " + filepath);
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
        state.track_end_reached = false;

        input_buffer->clear();
        output_buffer->clear();
        decoder_iterations = 0;
        last_input_sample_rate = 0;
        last_input_channels = 0;
        last_input_bits = 0;

        startDiskReader();
        preBufferData();

        return true;
    }

    void decode() {
        if (!state.playing || state.error_occurred) return;
        if (seeking_in_progress) return;

        uint8_t decode_chunk[MP3_CHUNK_BUFFER_SIZE];
        size_t bytes_available = input_buffer->available();

        if (bytes_available >= MP3_CHUNK_BUFFER_SIZE) {
            size_t bytes_read = input_buffer->read(decode_chunk, MP3_CHUNK_BUFFER_SIZE);
            if (bytes_read > 0) {
                decoder.write(decode_chunk, bytes_read);
                state.bytes_decoded += bytes_read;
                state.file_position += bytes_read;
                decoder_iterations++;
            }
        } else {
            size_t bytes_read = input_buffer->read(decode_chunk, bytes_available);
            if (bytes_read > 0) {
                decoder.write(decode_chunk, bytes_read);
                state.bytes_decoded += bytes_read;
                state.file_position += bytes_read;
                decoder_iterations++;
            }

            if (state.file_position >= state.file_size - 1) {
                trackEndReached();
            }
        }

        AudioInfo info = decoder.audioInfo();
        if (info.sample_rate > 0) state.sample_rate = info.sample_rate;
        if (info.channels > 0) state.channels = info.channels;
        if (info.bits_per_sample > 0) state.bits_per_sample = info.bits_per_sample;

        // Keep output fixed at 96kHz and only adapt resampling ratio from input format.
        if (state.sample_rate > 0) {
            if (state.sample_rate != last_input_sample_rate ||
                state.channels != last_input_channels ||
                state.bits_per_sample != last_input_bits) {
                AudioInfo track_info;
                track_info.sample_rate = state.sample_rate;
                track_info.channels = state.channels;
                track_info.bits_per_sample = state.bits_per_sample;
                resampler.setAudioInfo(track_info);

                last_input_sample_rate = state.sample_rate;
                last_input_channels = state.channels;
                last_input_bits = state.bits_per_sample;
            }
        }
    }

    void applyEQFromGlobals() {
        float new_low = EQ_LOW;
        float new_mid = EQ_MID;
        float new_high = EQ_HIGH;

        if (new_low == last_eq_low && new_mid == last_eq_mid && new_high == last_eq_high) {
            return;
        }

        bool new_eq_enabled = !isEQNeutral(new_low, new_mid, new_high);

        cfg_eq.gain_low = new_low;
        cfg_eq.gain_medium = new_mid;
        cfg_eq.gain_high = new_high;

        if (new_eq_enabled != eq_enabled) {
            eq_enabled = new_eq_enabled;
            if (eq_enabled) {
                if (!eq.begin(cfg_eq)) {
                    log_error("MP3","Cannot enable equalizer");
                    eq_enabled = false;
                    updateAudioPathForEQ();
                    return;
                }
            } else {
                eq.end();
            }
            updateAudioPathForEQ();
        }

        last_eq_low = new_low;
        last_eq_mid = new_mid;
        last_eq_high = new_high;
    }

    void pause() {
        state.paused = true;
        log_info("MP3","Playback paused");
    }

    void resume() {
        state.paused = false;
        log_info("MP3","Playback resumed");
    }

    void stop() {
        state.stop_requested = true;
        state.playing = false;

        stopDiskReader();
        decoder.end();

        if (current_file) {
            current_file.close();
        }

        input_buffer->clear();
        output_buffer->clear();
    }

    void openBlockMediaBrowser() {
        BB_BROWSER_MAX = TOTAL_BLOCKS - 1;
        BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;

        myNex.writeStr("mp3browser.path.txt", HMI_FNAME);
        myNex.writeStr("mp3browser.totalBl.txt", String(TOTAL_BLOCKS));
        myNex.writeStr("mp3browser.bbpag.txt", String(BB_PAGE_SELECTED));
        myNex.writeStr("mp3browser.size0.txt", "SIZE[MB]");

        double ctpage = (double)TOTAL_BLOCKS / (double)MAX_BLOCKS_IN_BROWSER;
        int totalPages = trunc(ctpage);
        if ((TOTAL_BLOCKS % MAX_BLOCKS_IN_BROWSER != 0) && ctpage > 1) {
            totalPages += 1;
        } else {
            totalPages = 1;
        }

        myNex.writeStr("mp3browser.totalPag.txt", String(totalPages));

        int pos = 1;
        for (int i = BB_PTR_ITEM; i <= BB_PTR_ITEM + MAX_BLOCKS_IN_BROWSER; i++) {
            if (i <= TOTAL_BLOCKS - 1) {
                String name = playlist.getTrackAt(i);
                myNex.writeStr("mp3browser.id" + String(pos) + ".txt", String(i));
                myNex.writeStr("mp3browser.name" + String(pos) + ".txt", playlist.getTrackName(name));
            } else {
                myNex.writeStr("mp3browser.id" + String(pos) + ".txt", "");
                myNex.writeStr("mp3browser.name" + String(pos) + ".txt", "");
            }
            pos++;
        }

        BB_OPEN = false;
        BB_UPDATE = false;
    }

    bool seekToPosition(uint32_t target_pos) {
        if (!current_file || state.file_size == 0 || target_pos >= state.file_size) {
            return false;
        }
        if (seeking_in_progress) {
            return false;
        }

        seeking_in_progress = true;
        bool ok = current_file.seek(target_pos);
        if (ok) {
            state.file_position = target_pos;
            state.bytes_decoded = target_pos;
        }
        seeking_in_progress = false;
        return ok;
    }

    MP3PlayState getState() const {
        return state;
    }

    bool isPlaying() const {
        return state.playing && !state.stop_requested;
    }

    bool isPaused() const {
        return state.paused;
    }

    void resetTrackEndFlag() {
        state.track_end_reached = false;
    }

    void trackEndReached() {
        state.track_end_reached = true;
    }

    uint32_t getProgress() const {
        if (state.file_size == 0) return 0;
        return (state.file_position * 100) / state.file_size;
    }

    String getStatsReport() const {
        String report = "\n[MP3] ===== STATS =====\n";
        report += "State: " + String(state.playing ? "Playing" : "Stopped") + "\n";
        report += "File: " + state.current_file + "\n";
        report += "Progress: " + String(getProgress()) + "%\n";
        report += "Decoded bytes: " + String(state.bytes_decoded) + "\n";
        report += "Iterations: " + String(decoder_iterations) + "\n";
        report += "Sample rate: " + String(state.sample_rate) + " Hz\n";
        report += "Channels: " + String(state.channels) + "\n";
        report += "======================\n";
        return report;
    }

    bool initializePlaylist(const String& filepath) {
        int last_slash = filepath.lastIndexOf('/');
        String directory = (last_slash >= 0) ? filepath.substring(0, last_slash) : "/";

        if (!playlist.loadPlaylist(directory)) {
            log_error("MP3","Cannot load playlist from: " + directory);
            return false;
        }

        if (playlist.setCurrentTrack(filepath)) {
            return true;
        }

        playlist.current_track_index = 0;
        return true;
    }

    bool syncCurrentTrackByPath(const String& filepath) {
        if (playlist.setCurrentTrack(filepath)) {
            return true;
        }

        // Fallback robusto: comparar por nombre de fichero sin ruta
        int last_slash = filepath.lastIndexOf('/');
        String wanted = (last_slash >= 0) ? filepath.substring(last_slash + 1) : filepath;
        wanted.toLowerCase();

        for (int i = 0; i < playlist.getTotalTracks(); i++) {
            String track = playlist.getTrackAt(i);
            int slash = track.lastIndexOf('/');
            String only_name = (slash >= 0) ? track.substring(slash + 1) : track;
            only_name.toLowerCase();
            if (only_name == wanted) {
                playlist.current_track_index = i;
                return true;
            }
        }

        return false;
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

    String getCurrentTrackName() {
        return playlist.getCurrentTrackName();
    }

    String selectTrack(int track_index_0based) {
        return playlist.goToTrack(track_index_0based);
    }

    String selectTrack1Based(int track_number_1based) {
        return selectTrack(track_number_1based - 1);
    }

private:
    void preBufferData() {
        uint32_t target_buffer_size = MP3_PREBUFFER_THRESHOLD * 2;
        uint32_t prebuffer_attempts = 0;

        while (input_buffer->available() < target_buffer_size && prebuffer_attempts < 100) {
            uint8_t chunk[MP3_DISK_READ_SIZE];
            size_t bytes = current_file.read(chunk, MP3_DISK_READ_SIZE);

            if (bytes > 0) {
                input_buffer->write(chunk, bytes);
            } else {
                break;
            }

            prebuffer_attempts++;
            delay(2);
        }
    }

    void startDiskReader() {
        disk_reader.file = &current_file;
        disk_reader.input_buffer = input_buffer;
        disk_reader.running = true;

        xTaskCreatePinnedToCore(
            MP3DiskReadTask::diskReaderThread,
            "MP3DiskReader",
            MP3_DECODER_STACK_SIZE,
            &disk_reader,
            MP3_DISK_PRIORITY,
            &disk_reader.task_handle,
            0
        );
    }

    void stopDiskReader() {
        disk_reader.running = false;
        disk_reader.should_exit = true;

        uint32_t timeout_ms = 100;
        uint32_t start_ms = millis();

        while (disk_reader.task_handle != nullptr && millis() - start_ms < timeout_ms) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }

        if (disk_reader.task_handle) {
            log_error("MP3", "stopDiskReader: forcing vTaskDelete after timeout");
            vTaskDelete(disk_reader.task_handle);
            disk_reader.task_handle = nullptr;
        }

        disk_reader.should_exit = false;
    }
};

static inline void updateInformationMP3(OptimizedMP3Player &player)
{
    myNex.writeNum("tape.totalBlocks.val", player.getTotalTracks());
    myNex.writeNum("tape.currentBlock.val", player.getCurrentTrackNumber());

    uint32_t size_kb = player.getState().file_size / 1024;
    // myNex.writeStr("tape2.size.txt", String(size_kb) + " KB");
    // myNex.writeStr("tape2.name.txt", player.getCurrentTrackName());
    //
    if (size_kb < 1024) {
        myNex.writeStr("tape.size.txt", String(size_kb) + " KB");
    } else {
        myNex.writeStr("tape.size.txt", String(size_kb / 1024) + " MB");
    }
    myNex.writeStr("tape.name.txt", player.getCurrentTrackName());


    // Actualizamos información del fichero en curso.
    log_info("MP3","Current track: " + player.getCurrentTrackName() + " (" + String(player.getCurrentTrackNumber()) + "/" + String(player.getTotalTracks()) + ")");
}

static inline void MP3Player() {
    OptimizedMP3Player player;
    String current_playing_file = "";
    bool file_prepared = false;
    uint8_t stausFastFFWind = 0;
    uint8_t stausFastRWind = 0;
    bool lastWasFastFFWind = false;
    bool lastWasFastRWind = false;

    if (!player.begin()) {
        log_error("MP3","ERROR: initialization failed");
        LAST_MESSAGE = "MP3 initialization error";
        STOP = true;
        PLAY = false;
        return;
    }

    MEDIA_PLAYER_EN = true;
    MUSIC_IS_PLAYING = true;

    PLAY = false;
    PAUSE = false;
    EJECT = false;
    STOP = true;

    if (!player.initializePlaylist(PATH_FILE_TO_LOAD)) {
        log_error("MP3","ERROR: cannot create playlist from: " + PATH_FILE_TO_LOAD);
        LAST_MESSAGE = "Cannot load MP3 playlist";
        TOTAL_BLOCKS = 0;
        return;
    }
    TOTAL_BLOCKS = player.getTotalTracks();

    myNex.writeNum("tape.totalBlocks.val", TOTAL_BLOCKS);
    myNex.writeNum("tape.currentBlock.val", TOTAL_BLOCKS);

    while (!EJECT && !REC && MEDIA_PLAYER_EN) {
        current_playing_file = PATH_FILE_TO_LOAD;

        if (!player.play(PATH_FILE_TO_LOAD)) {
            log_error("MP3","ERROR: cannot open file: " + PATH_FILE_TO_LOAD);
            LAST_MESSAGE = "Cannot open MP3 file";
            return;
        }

        file_prepared = true;

        File f = SD_MMC.open(PATH_FILE_TO_LOAD);
        uint32_t file_size = f.size();
        f.close();

        myNex.writeNum("tape.totalBlocks.val", TOTAL_BLOCKS);
        myNex.writeNum("tape.currentBlock.val", player.getCurrentTrackNumber());
        //
        myNex.writeStr("tape.name.txt", player.getCurrentTrackName());
        
        if (file_size < (1024 * 1024)) {
            myNex.writeStr("tape.size.txt", String(file_size / 1024) + " KB");
        } else {
            myNex.writeStr("tape.size.txt", String(file_size / (1024 * 1024)) + " MB");
        }

        LAST_MESSAGE = "MP3: Ready to play. Press PLAY to start.";

        uint32_t last_update_hmi = millis();
        uint32_t decoder_iterations = 0;
        bool track_changed = false;
        bool is_currently_playing = false;
        uint32_t last_rwd_ffwd_time = 0;

        uint8_t playerStatus = 0;

        if ((RWIND || FFWIND) && !is_currently_playing) {
            if (RWIND) {
                PATH_FILE_TO_LOAD = player.getPreviousTrack();
            } else if (FFWIND) {
                PATH_FILE_TO_LOAD = player.getNextTrack();
            }
        }

        while (!EJECT && !REC && MEDIA_PLAYER_EN && !track_changed && PATH_FILE_TO_LOAD == current_playing_file) {
            if (EQ_CHANGE) {
                EQ_CHANGE = false;
                player.applyEQFromGlobals();
            }

            switch (playerStatus) {
                case 0:
                {
                    if (PLAY) {
                        playerStatus = 1;
                        player.resume();
                        is_currently_playing = true;
                        tapeAnimationON();
                    }
                }
                break;

                case 1:
                {
                    if (PAUSE) {
                        playerStatus = 2;
                        player.pause();
                        PAUSE = false;
                        tapeAnimationOFF();
                    } else if (STOP || EJECT) {
                        playerStatus = 0;
                        player.stop();
                        tapeAnimationOFF();
                        is_currently_playing = false;
                        decoder_iterations = 0;
                        PATH_FILE_TO_LOAD = player.selectTrack1Based(1);
                        track_changed = true;
                        break;
                    } else if (player.getState().track_end_reached) {
                        player.resetTrackEndFlag();
                        int current_track_num = player.getCurrentTrackNumber();
                        int total_tracks = TOTAL_BLOCKS;

                        if (current_track_num < total_tracks) {
                            PATH_FILE_TO_LOAD = player.getNextTrack();
                            track_changed = true;
                            break;
                        } else {
                            if (disable_auto_media_stop) {
                                PATH_FILE_TO_LOAD = player.selectTrack1Based(1);
                                track_changed = true;
                                break;
                            } else {
                                player.stop();
                                tapeAnimationOFF();
                                playerStatus = 0;
                                is_currently_playing = false;
                                STOP = true;
                                PLAY = false;
                                LAST_MESSAGE = "Playlist finished";
                                PATH_FILE_TO_LOAD = player.selectTrack1Based(1);
                                track_changed = true;
                                break;
                            }
                        }
                    }

                    player.decode();
                    decoder_iterations++;
                }
                break;

                case 2:
                {
                    if (PLAY || PAUSE) {
                        PAUSE = false;
                        playerStatus = 1;
                        player.resume();
                        tapeAnimationON();
                    } else if (STOP) {
                        playerStatus = 0;
                        player.stop();
                        tapeAnimationOFF();
                        is_currently_playing = false;
                        decoder_iterations = 0;
                        player.selectTrack(0);
                    }
                }
                break;
            }

            if ((RWIND && !FFWIND) || (KEEP_RWIND && !KEEP_FFWIND)) {
                rewindAnimation(-1);

                if (RWIND && !lastWasFastRWind && player.getProgress() > 10) {
                    RWIND = false;
                    player.pause();
                    player.seekToPosition(0);
                    player.resume();
                } else if (RWIND && !lastWasFastRWind && player.getProgress() <= 10) {
                    String prev_track = player.getPreviousTrack();
                    if (!prev_track.isEmpty()) {
                        PATH_FILE_TO_LOAD = prev_track;
                        RWIND = false;
                        track_changed = true;
                        break;
                    }
                    RWIND = false;
                    KEEP_RWIND = false;
                    KEEP_FFWIND = false;
                } else {
                    switch (stausFastRWind) {
                        case 0:
                        {
                            if (KEEP_RWIND) {
                                stausFastRWind = 1;
                            }
                        }
                        break;

                        case 1:
                        {
                            if (RWIND) {
                                stausFastRWind = 0;
                                RWIND = false;
                                KEEP_RWIND = false;
                                KEEP_FFWIND = false;
                                FFWIND = false;
                                lastWasFastRWind = false;
                                lastWasFastFFWind = false;
                            } else if (KEEP_RWIND) {
                                uint32_t now_ffwd = millis();
                                lastWasFastRWind = true;

                                if (now_ffwd - last_rwd_ffwd_time > 50) {
                                    last_rwd_ffwd_time = now_ffwd;
                                    if (player.getProgress() > 0) {
                                        size_t p_file_seek_pos = player.getState().file_position;
                                        if (p_file_seek_pos >= 0) {
                                            player.seekToPosition(p_file_seek_pos - 1024 * 128);
                                        }
                                    }
                                    PROGRESS_BAR_TOTAL_VALUE = player.getProgress();
                                }
                            }
                        }
                        break;
                    }
                }
            }
            else if ((FFWIND && !RWIND) || (KEEP_FFWIND && !KEEP_RWIND)) {
                rewindAnimation(1);

                if (FFWIND && !lastWasFastFFWind) {
                    String next_track = player.getNextTrack();
                    if (!next_track.isEmpty()) {
                        PATH_FILE_TO_LOAD = next_track;
                        FFWIND = false;
                        track_changed = true;
                        
                        break;
                    }
                    FFWIND = false;
                    KEEP_RWIND = false;
                    KEEP_FFWIND = false;
                } else {
                    switch (stausFastFFWind) {
                        case 0:
                        {
                            if (KEEP_FFWIND) {
                                stausFastFFWind = 1;
                            }
                        }
                        break;

                        case 1:
                        {
                            if (FFWIND) {
                                stausFastFFWind = 0;
                                FFWIND = false;
                                KEEP_FFWIND = false;
                                lastWasFastFFWind = false;
                            } else if (KEEP_FFWIND) {
                                uint32_t now_ffwd = millis();
                                lastWasFastFFWind = true;

                                if (now_ffwd - last_rwd_ffwd_time > 50) {
                                    last_rwd_ffwd_time = now_ffwd;

                                    if (player.getProgress() <= 90) {
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
                    }
                }

                
            }

            if (BB_OPEN) {
                player.openBlockMediaBrowser();
            } else if (BB_UPDATE) {
                player.openBlockMediaBrowser();
            } else if (UPDATE_HMI || UPDATE) {
                UPDATE_HMI = false;
                if (BLOCK_SELECTED >= 0 && BLOCK_SELECTED <= TOTAL_BLOCKS) {
                    String current_path = PATH_FILE_TO_LOAD;
                    PATH_FILE_TO_LOAD = player.selectTrack(BLOCK_SELECTED - 1);
                    if (!PATH_FILE_TO_LOAD.isEmpty() && PATH_FILE_TO_LOAD != current_path) {
                        bool playerIsPlaying = player.isPlaying();
                        player.stop();

                        UPDATE_HMI = false;
                        UPDATE = false;
                        updateInformationMP3(player);
                        track_changed = true;
                        if (playerIsPlaying) {
                            player.play(PATH_FILE_TO_LOAD);
                        }
                        break;
                    }
                }
            }

            uint32_t now = millis();
            if (now - last_update_hmi > 2000) {
                last_update_hmi = now;

                if (is_currently_playing) {
                    PROGRESS_BAR_TOTAL_VALUE = player.getProgress();
                } else {
                    PROGRESS_BAR_TOTAL_VALUE = 0;
                }
                updateInformationMP3(player);
            }

            if (decoder_iterations % 10 == 0) {
                vTaskDelay(1);
            }
        }

        if (track_changed && PATH_FILE_TO_LOAD != current_playing_file) {
            player.stop();
            tapeAnimationOFF();
            continue;
        }
    }

    player.stop();
    tapeAnimationOFF();

    MEDIA_PLAYER_EN = false;
    MUSIC_IS_PLAYING = false;

    log_info("MP3", "Player finished");
    log_info("MP3", player.getStatsReport());
}
