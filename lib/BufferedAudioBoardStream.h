#pragma once
#include <Arduino.h>
#include <AudioTools.h>

// Wrapper que añade buffering y corrige available/readBytes para AudioBoardStream
class BufferedAudioBoardStream : public AudioStream {
public:
    BufferedAudioBoardStream(AudioStream &source, size_t bufferSize = 4096)
        : _source(source), _buffer(bufferSize), _bufferSize(bufferSize) {}

    bool begin() override {
        _buffer.clear();
        return _source.begin();
    }

    void end() override {
        _source.end();
        _buffer.clear();
    }

    int available() override {
        refillBuffer();
        return _buffer.available();
    }

    size_t readBytes(uint8_t *data, size_t len) override {
        refillBuffer();
        size_t toRead = std::min(len, static_cast<size_t>(_buffer.available()));
        size_t read = _buffer.readArray(data, toRead);
        return read;
    }

    size_t write(const uint8_t *data, size_t len) override {
        return 0; // Solo lectura
    }

    size_t write(uint8_t c) override { return 0; }

    int availableForWrite() override { return 0; }

    void flush() override { _buffer.clear(); }

private:
    AudioStream &_source;
    RingBuffer<uint8_t> _buffer;
    size_t _bufferSize;

    void refillBuffer() {
        // Si el buffer está vacío, intenta rellenar
        if (_buffer.available() == 0) {
            uint8_t temp[256];
            size_t n = _source.readBytes(temp, sizeof(temp));
            if (n > 0) {
                _buffer.writeArray(temp, n);
            }
        }
    }
};
