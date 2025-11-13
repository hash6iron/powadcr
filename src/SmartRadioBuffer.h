// ✅ NUEVO BUFFER CIRCULAR INTELIGENTE PARA RADIO
class SmartRadioBuffer {
private:
    uint8_t* buffer;
    size_t bufferSize;
    size_t writePos;
    size_t readPos;
    size_t available;
    
    // ✅ GESTIÓN INTELIGENTE DEL ESTADO
    enum BufferState {
        FILLING,     // Llenando buffer inicial
        STABLE,      // Funcionamiento normal
        CRITICAL,    // Buffer bajo, modo defensivo
        OVERFLOW     // Buffer lleno, evitar pérdidas
    };
    
    BufferState currentState;
    
    // ✅ ESTADÍSTICAS PARA ADAPTACIÓN AUTOMÁTICA
    unsigned long totalBytesWritten;
    unsigned long totalBytesRead;
    unsigned long lastWriteTime;
    unsigned long lastReadTime;
    
    // ✅ CONTROL ADAPTATIVO
    size_t lowWaterMark;     // 25% del buffer
    size_t mediumWaterMark;  // 50% del buffer
    size_t highWaterMark;    // 75% del buffer
    
    // ✅ GESTIÓN DE INTERRUPCIONES
    unsigned long lastSuccessfulWrite;
    int consecutiveFailures;
    bool connectionStable;
    
    // Configuracion
    // const int lowWaterDivisor = 6;    // 5% para estado crítico
    // const int mediumWaterThreshold = 3;  // 90% para estado estable
    // const int highWaterThreshold = 3;    // 75% para estado de overflow
    //const int bufferSizeMultipler = 2;      // Multiplicador de tamaño de buffer respecto al original

public:
SmartRadioBuffer(size_t size) : 
    bufferSize(size), writePos(0), readPos(0), available(0),
    currentState(FILLING), totalBytesWritten(0), totalBytesRead(0),
    lastWriteTime(0), lastReadTime(0), consecutiveFailures(0), connectionStable(true) 
{
    
    buffer = (uint8_t*)ps_malloc(bufferSize);
    
    // ✅ MARCAS DE AGUA CORREGIDAS - MÁS LÓGICAS
    lowWaterMark = bufferSize / 8;        // 12.5% - crítico
    mediumWaterMark = bufferSize / 4;     // 25% - empezar estable  
    highWaterMark = (bufferSize * 3) / 4; // 75% - overflow
    
    // logln("SmartRadioBuffer created: " + String(bufferSize) + " bytes");
    // logln("Water marks - Low: " + String(lowWaterMark) + " (" + String((lowWaterMark*100)/bufferSize) + "%), Med: " + String(mediumWaterMark) + " (" + String((mediumWaterMark*100)/bufferSize) + "%), High: " + String(highWaterMark) + " (" + String((highWaterMark*100)/bufferSize) + "%)");
}
    
    ~SmartRadioBuffer() {
        if (buffer) {
            free(buffer);
        }
    }
    
    bool isReady() {
        return buffer != nullptr;
    }
    
    void clear() {
        writePos = 0;
        readPos = 0;
        available = 0;
        currentState = FILLING;
        consecutiveFailures = 0;
        connectionStable = true;
        lastSuccessfulWrite = millis();
    }
    
    // ✅ ESCRITURA INTELIGENTE CON GESTIÓN DE OVERFLOW
    size_t write(const uint8_t* data, size_t len) {
        if (!buffer || len == 0) return 0;
        
        unsigned long currentTime = millis();
        lastWriteTime = currentTime;
        
        size_t bytesWritten = 0;
        size_t freeSpace = bufferSize - available;
        
        if (freeSpace == 0) {
            // ✅ BUFFER LLENO - ESTRATEGIA INTELIGENTE
            handleBufferOverflow();
            freeSpace = bufferSize - available;
        }
        
        // ✅ ESCRIBIR DATOS DISPONIBLES
        size_t toWrite = min(len, freeSpace);
        
        for (size_t i = 0; i < toWrite; i++) {
            buffer[writePos] = data[i];
            writePos = (writePos + 1) % bufferSize;
            available++;
            bytesWritten++;
        }
        
        if (bytesWritten > 0) {
            totalBytesWritten += bytesWritten;
            lastSuccessfulWrite = currentTime;
            consecutiveFailures = 0;
            connectionStable = true;
        } else {
            consecutiveFailures++;
            if (consecutiveFailures > 5) {
                connectionStable = false;
            }
        }
        
        // ✅ ACTUALIZAR ESTADO DEL BUFFER
        updateBufferState();
        
        return bytesWritten;
    }
    
    // ✅ LECTURA INTELIGENTE CON CONTROL DE ESTADO
    size_t read(uint8_t* data, size_t len) {
        if (!buffer || len == 0 || !shouldRead()) return 0;
        
        unsigned long currentTime = millis();
        lastReadTime = currentTime;
        
        size_t bytesRead = 0;
        size_t toRead = min(len, available);
        
        // ✅ AJUSTAR LECTURA SEGÚN ESTADO DEL BUFFER
        toRead = adjustReadSize(toRead);
        
        for (size_t i = 0; i < toRead; i++) {
            data[i] = buffer[readPos];
            readPos = (readPos + 1) % bufferSize;
            available--;
            bytesRead++;
        }
        
        if (bytesRead > 0) {
            totalBytesRead += bytesRead;
        }
        
        // ✅ ACTUALIZAR ESTADO TRAS LECTURA
        updateBufferState();
        
        return bytesRead;
    }
    
private:
    // ✅ GESTIÓN INTELIGENTE DE OVERFLOW
    void handleBufferOverflow() {
        if (currentState != OVERFLOW) {
            //logln("Buffer overflow detected - entering overflow mode");
            currentState = OVERFLOW;
        }
        
        // ✅ ESTRATEGIA: DESCARTAR 25% MÁS ANTIGUO PARA HACER ESPACIO
        size_t bytesToDiscard = bufferSize / 4;
        
        readPos = (readPos + bytesToDiscard) % bufferSize;
        available -= bytesToDiscard;
        
        //logln("Discarded " + String(bytesToDiscard) + " bytes to prevent overflow");
    }
    
    // ✅ DECIDIR SI DEBEMOS LEER SEGÚN EL ESTADO
    bool shouldRead() {
        switch (currentState) {
            case FILLING:
                // Durante llenado inicial, solo leer si tenemos suficiente
                return available >= lowWaterMark / 2;
                
            case STABLE:
                // En modo estable, leer normalmente
                return available > 0;
                
            case CRITICAL:
                // En modo crítico, leer muy conservadoramente
                return available > 256;
                
            case OVERFLOW:
                // En overflow, leer agresivamente para hacer espacio
                return available > 0;
                
            default:
                return available > 0;
        }
    }

    // ✅ AJUSTAR TAMAÑO DE LECTURA SEGÚN ESTADO - MENOS RESTRICTIVO
    size_t adjustReadSize(size_t requestedSize) {
        switch (currentState) {
            case FILLING:
                // Durante llenado, leer en chunks pequeños
                return min(requestedSize, (size_t)512);  // Aumentado de 256
                
            case STABLE:
                // ✅ EN MODO ESTABLE, SER MUCHO MENOS RESTRICTIVO
                return requestedSize;  // Sin restricciones - era 768
                
            case CRITICAL:
                // En modo crítico, chunks pequeños
                return min(requestedSize, (size_t)256);  // Aumentado de 128
                
            case OVERFLOW:
                // En overflow, chunks grandes para vaciar rápido
                return requestedSize;
                
            default:
                return min(requestedSize, (size_t)1024);  // Aumentado de 512
        }
    }    
    
    // // ✅ AJUSTAR TAMAÑO DE LECTURA SEGÚN ESTADO
    // size_t adjustReadSize(size_t requestedSize) {
    //     switch (currentState) {
    //         case FILLING:
    //             // Durante llenado, leer en chunks pequeños
    //             return min(requestedSize, (size_t)256);
                
    //         case STABLE:
    //             // En modo estable, tamaño normal
    //             return min(requestedSize, (size_t)768);
                
    //         case CRITICAL:
    //             // En modo crítico, chunks muy pequeños
    //             return min(requestedSize, (size_t)128);
                
    //         case OVERFLOW:
    //             // En overflow, chunks grandes para vaciar rápido
    //             return requestedSize;
                
    //         default:
    //             return min(requestedSize, (size_t)512);
    //     }
    // }

    // ✅ ACTUALIZAR ESTADO SEGÚN NIVEL DE LLENADO - SIMPLIFICADO
    void updateBufferState() {
        BufferState newState = currentState;
        
        // ✅ LÓGICA MÁS SIMPLE Y CLARA
        if (available < lowWaterMark) {           // < 12.5%
            newState = CRITICAL;
        } else if (available > highWaterMark) {   // > 75%
            newState = OVERFLOW;  
        } else if (available >= mediumWaterMark) { // >= 25%
            newState = STABLE;
        } else {
            // Entre 12.5% y 25% - mantener estado actual o ir a STABLE
            if (currentState == FILLING) {
                newState = STABLE;  // Salir de FILLING más temprano
            } else {
                newState = STABLE;  // Por defecto, ir a STABLE
            }
        }
        
        // if (newState != currentState) {
        //     logln("Buffer state: " + getStateString(currentState) + " -> " + getStateString(newState) + " (" + String(getBufferUsage(), 1) + "%)");
        //     currentState = newState;
        // }
    }

    
    // // ✅ ACTUALIZAR ESTADO SEGÚN NIVEL DE LLENADO
    // void updateBufferState() {
    //     BufferState newState = currentState;
        
    //     // ✅ TRANSICIONES MÁS TEMPRANAS A ESTADOS DEFENSIVOS
    //     if (available < lowWaterMark) {
    //         newState = CRITICAL;
    //     } else if (available > highWaterMark) {
    //         newState = OVERFLOW;
    //     } else if (available > mediumWaterMark) {
    //         newState = STABLE;
    //     } else {
    //         // ✅ ENTRE LOW Y MEDIUM - SER MÁS CONSERVADOR
    //         if (currentState == FILLING && available >= (lowWaterMark / 2)) {  // Cambio aquí
    //             newState = STABLE;
    //         } else if (currentState != FILLING) {
    //             // ✅ SI NO ESTAMOS LLENANDO Y TENEMOS POCO BUFFER, IR A CRÍTICO MÁS RÁPIDO
    //             if (available < (lowWaterMark * 1.5)) {  // Nuevo: 1.5x lowWaterMark = crítico antes
    //                 newState = CRITICAL;
    //             } else {
    //                 newState = STABLE;
    //             }
    //         }
    //     }
        
    //     if (newState != currentState) {
    //         ////logln("Buffer state change: " + getStateString(currentState) + " -> " + getStateString(newState) + " (available: " + String(available) + ")");
    //         currentState = newState;
    //     }
    // }
    
    String getStateString(BufferState state) const {
        switch (state) {
            case FILLING: return "FILLING";
            case STABLE: return "STABLE";
            case CRITICAL: return "CRITICAL";
            case OVERFLOW: return "OVERFLOW";
            default: return "UNKNOWN";
        }
    }

public:
    // ✅ MÉTODOS DE INFORMACIÓN Y ESTADÍSTICAS
    size_t getAvailable() const {
        return available;
    }
    
    size_t getFreeSpace() const {
        return bufferSize - available;
    }
    
    float getBufferUsage() const {
        return (available * 100.0f) / bufferSize;
    }
    
    String getStatusString() const {
        return getStateString(currentState) + " (" + String(getBufferUsage(), 1) + "%)";
    }
    
    bool isConnectionStable() const {
        return connectionStable && (millis() - lastSuccessfulWrite) < 5000;
    }
    
    // ✅ INFORMACIÓN DETALLADA PARA DEBUGGING
    void logStatistics() {
        unsigned long currentTime = millis();
        float avgWriteSpeed = (totalBytesWritten * 1000.0) / max(1UL, currentTime - 1000); // Aproximado
        
        //logln("=== SMART BUFFER STATS ===");
        //logln("State: " + getStatusString());
        //logln("Usage: " + String(available) + "/" + String(bufferSize) + " bytes");
        //logln("Total written: " + String(totalBytesWritten / 1024) + " KB");
        //logln("Total read: " + String(totalBytesRead / 1024) + " KB");
        //logln("Consecutive failures: " + String(consecutiveFailures));
        //logln("Connection stable: " + String(connectionStable));
        //logln("==========================");
    }
};