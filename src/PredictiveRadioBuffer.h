// ✅ BUFFER CIRCULAR PREDICTIVO CON ML SIMPLE
class PredictiveRadioBuffer {
private:
    uint8_t* buffer;
    size_t bufferSize;
    size_t writePos;
    size_t readPos;
    size_t available;
    
    enum BufferState {
        LEARNING,    // Aprendiendo patrones (primeros 30s)
        PREDICTING,  // Usando predicciones
        ADAPTING,    // Adaptándose a cambios
        EMERGENCY    // Modo de emergencia
    };
    
    BufferState currentState;
    
    // ✅ MODELO PREDICTIVO SIMPLE
    struct ConnectionPattern {
        unsigned long timestamp;
        size_t bytesReceived;
        size_t bufferLevel;
        bool wasInterruption;
        unsigned long interruptionDuration;
    };
    
    static const int MAX_HISTORY = 50;
    ConnectionPattern history[MAX_HISTORY];
    int historyIndex;
    int historyCount;
    
    // ✅ MÉTRICAS APRENDIDAS
    struct LearnedMetrics {
        float avgBytesPerSecond;
        float avgInterruptionFrequency;    // Interrupciones por minuto
        float avgInterruptionDuration;     // Duración promedio en ms
        float connectionStabilityScore;    // 0.0 (muy inestable) a 1.0 (muy estable)
        float predictedNextInterruption;   // Tiempo estimado hasta próxima interrupción
        size_t optimalBufferLevel;         // Nivel óptimo predicho
    };
    
    LearnedMetrics metrics;
    
    // ✅ CONTROL ADAPTATIVO INTELIGENTE
    size_t dynamicLowWaterMark;
    size_t dynamicMediumWaterMark;
    size_t dynamicHighWaterMark;
    
    // ✅ ESTADÍSTICAS DE RENDIMIENTO
    unsigned long totalBytesWritten;
    unsigned long totalBytesRead;
    unsigned long lastWriteTime;
    unsigned long lastReadTime;
    unsigned long learningStartTime;
    
    // ✅ DETECCIÓN DE INTERRUPCIONES
    unsigned long lastSuccessfulWrite;
    unsigned long currentInterruptionStart;
    bool inInterruption;
    int consecutiveFailures;
    
    // ✅ CONFIGURACIÓN ADAPTATIVA
    struct AdaptiveConfig {
        size_t readChunkSize;
        unsigned long readInterval;
        float aggressivenessLevel;  // 0.0 conservador, 1.0 agresivo
        bool useEmergencyMode;
    };
    
    AdaptiveConfig config;

public:
    PredictiveRadioBuffer(size_t size) : 
        bufferSize(size), writePos(0), readPos(0), available(0),
        currentState(LEARNING), historyIndex(0), historyCount(0),
        totalBytesWritten(0), totalBytesRead(0), lastWriteTime(0), lastReadTime(0),
        inInterruption(false), consecutiveFailures(0) {
        
        buffer = (uint8_t*)ps_malloc(bufferSize);
        
        // ✅ INICIALIZAR MÉTRICAS
        resetMetrics();
        
        // ✅ MARCAS DE AGUA INICIALES CONSERVADORAS
        dynamicLowWaterMark = bufferSize / 4;      // 25%
        dynamicMediumWaterMark = bufferSize / 2;   // 50%  
        dynamicHighWaterMark = (bufferSize * 3) / 4; // 75%
        
        // ✅ CONFIGURACIÓN INICIAL ADAPTATIVA
        config.readChunkSize = 1024;
        config.readInterval = 8;
        config.aggressivenessLevel = 0.5;
        config.useEmergencyMode = false;
        
        learningStartTime = millis();
        
        logln("PredictiveRadioBuffer created: " + String(bufferSize) + " bytes (Learning mode)");
    }
    
    ~PredictiveRadioBuffer() {
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
        currentState = LEARNING;
        historyIndex = 0;
        historyCount = 0;
        inInterruption = false;
        consecutiveFailures = 0;
        learningStartTime = millis();
        resetMetrics();
        lastSuccessfulWrite = millis();
    }
    
    // ✅ ESCRITURA CON APRENDIZAJE DE PATRONES
    size_t write(const uint8_t* data, size_t len) {
        if (!buffer || len == 0) return 0;
        
        unsigned long currentTime = millis();
        lastWriteTime = currentTime;
        
        // ✅ DETECTAR FIN DE INTERRUPCIÓN
        if (inInterruption && len > 0) {
            recordInterruptionEnd(currentTime);
        }
        
        size_t bytesWritten = 0;
        size_t freeSpace = bufferSize - available;
        
        if (freeSpace == 0) {
            handleBufferOverflow();
            freeSpace = bufferSize - available;
        }
        
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
            
            // ✅ REGISTRAR DATOS PARA APRENDIZAJE
            recordConnectionData(currentTime, bytesWritten);
        } else {
            // ✅ DETECTAR INICIO DE INTERRUPCIÓN
            consecutiveFailures++;
            if (!inInterruption && consecutiveFailures > 3) {
                recordInterruptionStart(currentTime);
            }
        }
        
        // ✅ ACTUALIZAR MODELO PREDICTIVO
        updatePredictiveModel();
        
        return bytesWritten;
    }
    
    // ✅ LECTURA PREDICTIVA INTELIGENTE
    size_t read(uint8_t* data, size_t len) {
        if (!buffer || len == 0 || !shouldReadPredictive()) return 0;
        
        unsigned long currentTime = millis();
        lastReadTime = currentTime;
        
        size_t bytesRead = 0;
        size_t toRead = min(len, available);
        
        // ✅ AJUSTAR LECTURA SEGÚN PREDICCIONES
        toRead = adjustReadSizePredictive(toRead);
        
        for (size_t i = 0; i < toRead; i++) {
            data[i] = buffer[readPos];
            readPos = (readPos + 1) % bufferSize;
            available--;
            bytesRead++;
        }
        
        if (bytesRead > 0) {
            totalBytesRead += bytesRead;
        }
        
        return bytesRead;
    }
    
private:
    // ✅ RESETEAR MÉTRICAS APRENDIDAS
    void resetMetrics() {
        metrics.avgBytesPerSecond = 0;
        metrics.avgInterruptionFrequency = 0;
        metrics.avgInterruptionDuration = 0;
        metrics.connectionStabilityScore = 0.5;
        metrics.predictedNextInterruption = 0;
        metrics.optimalBufferLevel = bufferSize / 2;
    }
    
    // ✅ REGISTRAR DATOS DE CONEXIÓN PARA APRENDIZAJE
    void recordConnectionData(unsigned long timestamp, size_t bytes) {
        ConnectionPattern& pattern = history[historyIndex];
        pattern.timestamp = timestamp;
        pattern.bytesReceived = bytes;
        pattern.bufferLevel = available;
        pattern.wasInterruption = false;
        pattern.interruptionDuration = 0;
        
        historyIndex = (historyIndex + 1) % MAX_HISTORY;
        if (historyCount < MAX_HISTORY) historyCount++;
    }
    
    // ✅ REGISTRAR INICIO DE INTERRUPCIÓN
    void recordInterruptionStart(unsigned long timestamp) {
        inInterruption = true;
        currentInterruptionStart = timestamp;
        logln("Interruption detected at buffer level: " + String(getBufferUsage(), 1) + "%");
    }
    
    // ✅ REGISTRAR FIN DE INTERRUPCIÓN
    void recordInterruptionEnd(unsigned long timestamp) {
        if (!inInterruption) return;
        
        unsigned long duration = timestamp - currentInterruptionStart;
        
        // Registrar en historial
        if (historyCount > 0) {
            int prevIndex = (historyIndex - 1 + MAX_HISTORY) % MAX_HISTORY;
            history[prevIndex].wasInterruption = true;
            history[prevIndex].interruptionDuration = duration;
        }
        
        inInterruption = false;
        logln("Interruption ended. Duration: " + String(duration) + "ms");
    }
    
    // ✅ ACTUALIZAR MODELO PREDICTIVO
    void updatePredictiveModel() {
        if (historyCount < 5) return; // Necesitamos al menos 5 muestras
        
        unsigned long currentTime = millis();
        unsigned long learningTime = currentTime - learningStartTime;
        
        // ✅ CALCULAR MÉTRICAS BÁSICAS
        calculateAverageMetrics();
        
        // ✅ CALCULAR ESTABILIDAD DE CONEXIÓN
        calculateStabilityScore();
        
        // ✅ PREDECIR PRÓXIMA INTERRUPCIÓN
        predictNextInterruption();
        
        // ✅ AJUSTAR MARCAS DE AGUA DINÁMICAMENTE
        adjustWaterMarks();
        
        // ✅ CAMBIAR DE ESTADO SEGÚN APRENDIZAJE
        if (currentState == LEARNING && learningTime > 30000) { // 30 segundos
            currentState = PREDICTING;
            logln("Switching to PREDICTING mode. Stability: " + String(metrics.connectionStabilityScore, 2));
        }
    }
    
    // // ✅ CALCULAR MÉTRICAS PROMEDIO
    // void calculateAverageMetrics() {
    //     if (historyCount == 0) return;
        
    //     unsigned long totalBytes = 0;
    //     unsigned long timeSpan = 0;
    //     int interruptions = 0;
    //     unsigned long totalInterruptionTime = 0;
        
    //     for (int i = 0; i < historyCount; i++) {
    //         totalBytes += history[i].bytesReceived;
            
    //         if (history[i].wasInterruption) {
    //             interruptions++;
    //             totalInterruptionTime += history[i].interruptionDuration;
    //         }
    //     }
        
    //     if (historyCount > 1) {
    //         timeSpan = history[(historyIndex - 1 + MAX_HISTORY) % MAX_HISTORY].timestamp - 
    //                   history[0].timestamp;
    //     }
        
    //     // Calcular métricas
    //     if (timeSpan > 0) {
    //         metrics.avgBytesPerSecond = (totalBytes * 1000.0) / timeSpan;
    //         metrics.avgInterruptionFrequency = (interruptions * 60000.0) / timeSpan; // Por minuto
    //     }
        
    //     if (interruptions > 0) {
    //         metrics.avgInterruptionDuration = totalInterruptionTime / interruptions;
    //     }
    // }
    

    
    // ✅ CALCULAR CONSISTENCIA DE BYTES RECIBIDOS - CORREGIDO
    float calculateConsistencyScore() {
        if (historyCount < 3) return 0.5f;
        
        float variance = 0.0f;
        float mean = 0.0f;
        
        // Calcular media
        for (int i = 0; i < historyCount; i++) {
            mean += (float)history[i].bytesReceived;
        }
        mean /= (float)historyCount;
        
        // Calcular varianza
        for (int i = 0; i < historyCount; i++) {
            float diff = (float)history[i].bytesReceived - mean;
            variance += diff * diff;
        }
        variance /= (float)historyCount;
        
        // Coeficiente de variación normalizado
        float cv = (mean > 0.0f) ? sqrt(variance) / mean : 1.0f;
        return 1.0f - min(cv, 1.0f);  // ✅ Ahora ambos son float
    }
    
// ✅ CALCULAR PUNTUACIÓN DE ESTABILIDAD - CORREGIDO
void calculateStabilityScore() {
    if (historyCount == 0) return;
    
    // Factores que afectan la estabilidad:
    float frequencyScore = 1.0f - min(metrics.avgInterruptionFrequency / 10.0f, 1.0f);
    float durationScore = 1.0f - min(metrics.avgInterruptionDuration / 5000.0f, 1.0f);
    float consistencyScore = calculateConsistencyScore();
    
    metrics.connectionStabilityScore = (frequencyScore + durationScore + consistencyScore) / 3.0f;
}

// ✅ PREDECIR PRÓXIMA INTERRUPCIÓN - CORREGIDO
void predictNextInterruption() {
    if (metrics.avgInterruptionFrequency <= 0.0f) {
        metrics.predictedNextInterruption = 0.0f;
        return;
    }
    
    // Tiempo promedio entre interrupciones
    float avgTimeBetweenInterruptions = 60000.0f / metrics.avgInterruptionFrequency;
    
    // Ajustar según patrones temporales (simplificado)
    unsigned long timeSinceLastInterruption = millis() - lastSuccessfulWrite;
    
    if ((float)timeSinceLastInterruption > avgTimeBetweenInterruptions * 0.8f) {
        metrics.predictedNextInterruption = avgTimeBetweenInterruptions * 0.2f; // Próxima pronto
    } else {
        metrics.predictedNextInterruption = avgTimeBetweenInterruptions - (float)timeSinceLastInterruption;
    }
}

// ✅ AJUSTAR MARCAS DE AGUA DINÁMICAMENTE - CORREGIDO
void adjustWaterMarks() {
    // Baseado en estabilidad y duración de interrupciones
    float stabilityFactor = metrics.connectionStabilityScore;
    float interruptionFactor = min(metrics.avgInterruptionDuration / 1000.0f, 5.0f); // Max 5 segundos
    
    // Más inestable = marcas de agua más altas
    size_t baseHigh = (bufferSize * 3) / 4; // 75%
    size_t baseMedium = bufferSize / 2;      // 50%
    size_t baseLow = bufferSize / 4;         // 25%
    
    // Ajustar según estabilidad (menos estable = más buffer)
    float adjustment = 1.0f + (1.0f - stabilityFactor) + (interruptionFactor * 0.2f);
    
    dynamicHighWaterMark = min((size_t)(baseHigh * adjustment), bufferSize - 1024);
    dynamicMediumWaterMark = min((size_t)(baseMedium * adjustment), dynamicHighWaterMark - 1024);
    dynamicLowWaterMark = min((size_t)(baseLow * adjustment), dynamicMediumWaterMark - 1024);
    
    // Actualizar nivel óptimo
    metrics.optimalBufferLevel = dynamicMediumWaterMark + 
                               (size_t)((1.0f - stabilityFactor) * (float)(dynamicHighWaterMark - dynamicMediumWaterMark));
}

// ✅ CALCULAR MÉTRICAS PROMEDIO - CORREGIDO
void calculateAverageMetrics() {
    if (historyCount == 0) return;
    
    unsigned long totalBytes = 0;
    unsigned long timeSpan = 0;
    int interruptions = 0;
    unsigned long totalInterruptionTime = 0;
    
    for (int i = 0; i < historyCount; i++) {
        totalBytes += history[i].bytesReceived;
        
        if (history[i].wasInterruption) {
            interruptions++;
            totalInterruptionTime += history[i].interruptionDuration;
        }
    }
    
    if (historyCount > 1) {
        timeSpan = history[(historyIndex - 1 + MAX_HISTORY) % MAX_HISTORY].timestamp - 
                  history[0].timestamp;
    }
    
    // Calcular métricas
    if (timeSpan > 0) {
        metrics.avgBytesPerSecond = ((float)totalBytes * 1000.0f) / (float)timeSpan;
        metrics.avgInterruptionFrequency = ((float)interruptions * 60000.0f) / (float)timeSpan; // Por minuto
    }
    
    if (interruptions > 0) {
        metrics.avgInterruptionDuration = (float)totalInterruptionTime / (float)interruptions;
    }
}

// ✅ AJUSTAR TAMAÑO DE LECTURA SEGÚN PREDICCIONES - CORREGIDO
size_t adjustReadSizePredictive(size_t requestedSize) {
    float aggressiveness = config.aggressivenessLevel;
    
    // ✅ AJUSTAR SEGÚN ESTABILIDAD Y PREDICCIONES
    if (currentState == PREDICTING) {
        // Más estable = más agresivo en lectura
        aggressiveness *= metrics.connectionStabilityScore;
        
        // Si se predice interrupción pronto, ser más conservador
        if (metrics.predictedNextInterruption > 0.0f && metrics.predictedNextInterruption < 3000.0f) {
            aggressiveness *= 0.5f;
        }
    }
    
    // Calcular tamaño de chunk dinámico
    size_t baseChunk = config.readChunkSize;
    size_t adjustedChunk = (size_t)((float)baseChunk * aggressiveness);
    
    return min(requestedSize, max(adjustedChunk, (size_t)256));
}
    
    // // ✅ PREDECIR PRÓXIMA INTERRUPCIÓN
    // void predictNextInterruption() {
    //     if (metrics.avgInterruptionFrequency <= 0) {
    //         metrics.predictedNextInterruption = 0;
    //         return;
    //     }
        
    //     // Tiempo promedio entre interrupciones
    //     float avgTimeBetweenInterruptions = 60000.0 / metrics.avgInterruptionFrequency;
        
    //     // Ajustar según patrones temporales (simplificado)
    //     unsigned long timeSinceLastInterruption = millis() - lastSuccessfulWrite;
        
    //     if (timeSinceLastInterruption > avgTimeBetweenInterruptions * 0.8) {
    //         metrics.predictedNextInterruption = avgTimeBetweenInterruptions * 0.2; // Próxima pronto
    //     } else {
    //         metrics.predictedNextInterruption = avgTimeBetweenInterruptions - timeSinceLastInterruption;
    //     }
    // }
    
    // // ✅ AJUSTAR MARCAS DE AGUA DINÁMICAMENTE
    // void adjustWaterMarks() {
    //     // Baseado en estabilidad y duración de interrupciones
    //     float stabilityFactor = metrics.connectionStabilityScore;
    //     float interruptionFactor = min(metrics.avgInterruptionDuration / 1000.0, 5.0); // Max 5 segundos
        
    //     // Más inestable = marcas de agua más altas
    //     size_t baseHigh = (bufferSize * 3) / 4; // 75%
    //     size_t baseMedium = bufferSize / 2;      // 50%
    //     size_t baseLow = bufferSize / 4;         // 25%
        
    //     // Ajustar según estabilidad (menos estable = más buffer)
    //     float adjustment = 1.0 + (1.0 - stabilityFactor) + (interruptionFactor * 0.2);
        
    //     dynamicHighWaterMark = min((size_t)(baseHigh * adjustment), bufferSize - 1024);
    //     dynamicMediumWaterMark = min((size_t)(baseMedium * adjustment), dynamicHighWaterMark - 1024);
    //     dynamicLowWaterMark = min((size_t)(baseLow * adjustment), dynamicMediumWaterMark - 1024);
        
    //     // Actualizar nivel óptimo
    //     metrics.optimalBufferLevel = dynamicMediumWaterMark + 
    //                                (size_t)((1.0 - stabilityFactor) * (dynamicHighWaterMark - dynamicMediumWaterMark));
    // }
    
    // ✅ DECIDIR SI LEER SEGÚN PREDICCIONES
    bool shouldReadPredictive() {
        if (available == 0) return false;
        
        switch (currentState) {
            case LEARNING:
                return available >= dynamicLowWaterMark / 2;
                
            case PREDICTING:
                // ✅ USAR PREDICCIONES PARA DECIDIR
                if (metrics.predictedNextInterruption > 0 && metrics.predictedNextInterruption < 2000) {
                    // Interrupción predicha pronto - ser más conservador
                    return available >= dynamicMediumWaterMark;
                } else {
                    return available >= dynamicLowWaterMark;
                }
                
            case ADAPTING:
                return available >= dynamicMediumWaterMark;
                
            case EMERGENCY:
                return available > 512; // Muy conservador
                
            default:
                return available > 0;
        }
    }
    
    // // ✅ AJUSTAR TAMAÑO DE LECTURA SEGÚN PREDICCIONES
    // size_t adjustReadSizePredictive(size_t requestedSize) {
    //     float aggressiveness = config.aggressivenessLevel;
        
    //     // ✅ AJUSTAR SEGÚN ESTABILIDAD Y PREDICCIONES
    //     if (currentState == PREDICTING) {
    //         // Más estable = más agresivo en lectura
    //         aggressiveness *= metrics.connectionStabilityScore;
            
    //         // Si se predice interrupción pronto, ser más conservador
    //         if (metrics.predictedNextInterruption > 0 && metrics.predictedNextInterruption < 3000) {
    //             aggressiveness *= 0.5;
    //         }
    //     }
        
    //     // Calcular tamaño de chunk dinámico
    //     size_t baseChunk = config.readChunkSize;
    //     size_t adjustedChunk = (size_t)(baseChunk * aggressiveness);
        
    //     return min(requestedSize, max(adjustedChunk, (size_t)256));
    // }
    
    void handleBufferOverflow() {
        logln("Predictive buffer overflow - discarding old data");
        
        size_t bytesToDiscard = bufferSize / 8; // Descartar menos que antes
        readPos = (readPos + bytesToDiscard) % bufferSize;
        available -= bytesToDiscard;
        
        // Cambiar a modo adaptativo si hay overflows frecuentes
        if (currentState != EMERGENCY) {
            currentState = ADAPTING;
        }
    }

public:
    // ✅ MÉTODOS DE INFORMACIÓN PREDICTIVA
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
        String state = "";
        switch (currentState) {
            case LEARNING: state = "LEARNING"; break;
            case PREDICTING: state = "PREDICTING"; break;
            case ADAPTING: state = "ADAPTING"; break;
            case EMERGENCY: state = "EMERGENCY"; break;
        }
        
        return state + " (" + String(getBufferUsage(), 1) + "% | Stability: " + 
               String(metrics.connectionStabilityScore, 2) + ")";
    }
    
    // ✅ OBTENER MÉTRICAS PREDICTIVAS
    struct PredictionInfo {
        float stabilityScore;
        float avgBytesPerSec;
        float nextInterruptionMs;
        size_t optimalLevel;
        String recommendation;
    };
    
    PredictionInfo getPredictionInfo() const {
        PredictionInfo info;
        info.stabilityScore = metrics.connectionStabilityScore;
        info.avgBytesPerSec = metrics.avgBytesPerSecond;
        info.nextInterruptionMs = metrics.predictedNextInterruption;
        info.optimalLevel = metrics.optimalBufferLevel;
        
        // Generar recomendación
        if (info.stabilityScore > 0.8) {
            info.recommendation = "Excellent connection - optimized for speed";
        } else if (info.stabilityScore > 0.6) {
            info.recommendation = "Good connection - balanced approach";
        } else if (info.stabilityScore > 0.4) {
            info.recommendation = "Unstable connection - defensive buffering";
        } else {
            info.recommendation = "Poor connection - maximum buffering";
        }
        
        return info;
    }
    
    void logPredictiveStats() {
        PredictionInfo info = getPredictionInfo();
        
        logln("=== PREDICTIVE BUFFER STATS ===");
        logln("Status: " + getStatusString());
        logln("Stability Score: " + String(info.stabilityScore, 2));
        logln("Avg Speed: " + String(info.avgBytesPerSec / 1024.0, 1) + " KB/s");
        logln("Next Interruption: " + String(info.nextInterruptionMs / 1000.0, 1) + "s");
        logln("Optimal Level: " + String((info.optimalLevel * 100) / bufferSize) + "%");
        logln("Recommendation: " + info.recommendation);
        logln("==============================");
    }
};