class SimpleCircularBuffer {
private:
    uint8_t* buffer;
    size_t bufferSize;
    size_t writePos;
    size_t readPos;
    size_t available;

public:
    SimpleCircularBuffer(size_t size) : 
        bufferSize(size), writePos(0), readPos(0), available(0) {
        buffer = (uint8_t*)ps_malloc(bufferSize);
    }
    
    ~SimpleCircularBuffer() {
        if (buffer) free(buffer);
    }
    
    size_t write(const uint8_t* data, size_t len) {
        if (!buffer) return 0;
        
        size_t freeSpace = bufferSize - available;
        size_t toWrite = min(len, freeSpace);
        
        for (size_t i = 0; i < toWrite; i++) {
            buffer[writePos] = data[i];
            writePos = (writePos + 1) % bufferSize;
            available++;
        }
        
        return toWrite;
    }
    
    size_t read(uint8_t* data, size_t len) {
        if (!buffer) return 0;
        
        size_t toRead = min(len, available);
        
        for (size_t i = 0; i < toRead; i++) {
            data[i] = buffer[readPos];
            readPos = (readPos + 1) % bufferSize;
            available--;
        }
        
        return toRead;
    }
    
    void clear() {
        writePos = 0;
        readPos = 0;
        available = 0;
    }
    
    size_t getAvailable() const {
        return available;
    }
    
    size_t getFreeSpace() const {
        return bufferSize - available;
    }
    
    bool isReady() const {
        return buffer != nullptr;
    }
};