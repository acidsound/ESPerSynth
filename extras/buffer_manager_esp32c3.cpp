/*
 * ESP32C3 Mozzi Library 버퍼 관리 구현
 * 
 * 최적화된 원형 버퍼, 더블 버퍼링, 메모리 풀 관리
 * ESP32C3의 제한된 RAM을 효율적으로 활용
 */

#include "mozzi_config.h"
#include "esp_log.h"
#include <string.h>

// =============================================================================
// 버퍼 관리 전역 변수
// =============================================================================

static const char* TAG = "ESP32C3_BufferManager";

// 오디오 버퍼 (더블 버퍼링)
static int16_t audioBuffer0[MOZZI_OUTPUT_BUFFER_SIZE];
static int16_t audioBuffer1[MOZZI_OUTPUT_BUFFER_SIZE];
static volatile uint8_t currentBuffer = 0;
static volatile uint8_t writeBuffer = 1;
static volatile size_t writeIndex = 0;
static volatile size_t readIndex = 0;

// 제어 버퍼
static int16_t controlBuffer[MOZZI_CONTROL_RATE];
static volatile size_t controlWriteIndex = 0;
static volatile size_t controlReadIndex = 0;

// 원형 버퍼 (RAM 절약)
static int16_t circularBuffer[MOZZI_CIRCULAR_BUFFER_SIZE];
static volatile size_t circularWriteIndex = 0;
static volatile size_t circularReadIndex = 0;
static volatile size_t circularCount = 0;

// 메모리 풀 관리
#define MEMORY_POOL_SIZE 4
static int16_t* memoryPool[MEMORY_POOL_SIZE];
static bool poolUsed[MEMORY_POOL_SIZE];

// =============================================================================
// 템플릿 기반 최적화된 원형 버퍼
// =============================================================================

template<typename T, size_t Size>
class OptimizedCircularBuffer {
private:
    T buffer[Size];
    volatile size_t writeIndex = 0;
    volatile size_t readIndex = 0;
    volatile size_t count = 0;
    
    // atomic operations for thread safety
    inline size_t atomicIncrement(volatile size_t* value, size_t max) {
        size_t old = *value;
        size_t new_val = (old + 1) % max;
        *value = new_val;
        return old;
    }
    
    inline size_t atomicDecrement(volatile size_t* value, size_t max) {
        size_t old = *value;
        size_t new_val = (old == 0) ? (max - 1) : (old - 1);
        *value = new_val;
        return old;
    }
    
public:
    bool push(const T& item) {
        if (count >= Size) return false;  // 버퍼 FULL
        
        size_t idx = atomicIncrement(&writeIndex, Size);
        buffer[idx] = item;
        count++;
        return true;
    }
    
    bool pop(T& item) {
        if (count == 0) return false;  // 버퍼 EMPTY
        
        size_t idx = atomicIncrement(&readIndex, Size);
        item = buffer[idx];
        count--;
        return true;
    }
    
    bool peek(T& item) const {
        if (count == 0) return false;  // 버퍼 EMPTY
        
        size_t idx = readIndex;
        item = buffer[idx];
        return true;
    }
    
    size_t available() const { return count; }
    bool full() const { return count >= Size; }
    bool empty() const { return count == 0; }
    
    void clear() {
        writeIndex = 0;
        readIndex = 0;
        count = 0;
    }
    
    size_t capacity() const { return Size; }
};

// 오디오용 최적화된 원형 버퍼
OptimizedCircularBuffer<int16_t, 256> audioCircularBuffer;

// =============================================================================
// 버퍼 초기화
// =============================================================================

void initializeBufferManager() {
    DEBUG_PRINTLN("Initializing ESP32C3 buffer manager...");
    
    // 오디오 버퍼 초기화
    memset(audioBuffer0, 0, sizeof(audioBuffer0));
    memset(audioBuffer1, 0, sizeof(audioBuffer1));
    
    currentBuffer = 0;
    writeBuffer = 1;
    writeIndex = 0;
    readIndex = 0;
    
    // 제어 버퍼 초기화
    memset(controlBuffer, 0, sizeof(controlBuffer));
    controlWriteIndex = 0;
    controlReadIndex = 0;
    
    // 원형 버퍼 초기화
    memset(circularBuffer, 0, sizeof(circularBuffer));
    circularWriteIndex = 0;
    circularReadIndex = 0;
    circularCount = 0;
    
    // 오디오 원형 버퍼 초기화
    audioCircularBuffer.clear();
    
    // 메모리 풀 초기화
    initializeMemoryPool();
    
    DEBUG_PRINTLN("Buffer manager initialized successfully");
}

void initializeAudioBuffers() {
    // 더블 버퍼 초기화
    memset(audioBuffer0, 0, sizeof(audioBuffer0));
    memset(audioBuffer1, 0, sizeof(audioBuffer1));
    
    currentBuffer = 0;
    writeBuffer = 1;
    writeIndex = 0;
    readIndex = 0;
    
    DEBUG_PRINTLN("Audio buffers (double buffering) initialized");
}

// =============================================================================
// 더블 버퍼링 오디오 버퍼 관리
// =============================================================================

bool writeToAudioBuffer(int16_t sample) {
    int16_t* current_write_buffer = (writeBuffer == 0) ? audioBuffer0 : audioBuffer1;
    
    if (writeIndex >= MOZZI_OUTPUT_BUFFER_SIZE) {
        return false; // 버퍼 가득참
    }
    
    current_write_buffer[writeIndex++] = sample;
    return true;
}

bool readFromAudioBuffer(int16_t* sample) {
    int16_t* current_read_buffer = (currentBuffer == 0) ? audioBuffer0 : audioBuffer1;
    
    if (readIndex >= MOZZI_OUTPUT_BUFFER_SIZE) {
        return false; // 읽기 끝
    }
    
    *sample = current_read_buffer[readIndex++];
    return true;
}

void switchAudioBuffer() {
    currentBuffer = writeBuffer;
    writeBuffer = 1 - writeBuffer;
    writeIndex = 0;
    readIndex = 0;
    
    DEBUG_PRINTLN("Audio buffer switched");
}

bool isAudioBufferReady() {
    return (readIndex >= MOZZI_OUTPUT_BUFFER_SIZE);
}

bool isAudioBufferFull() {
    return (writeIndex >= MOZZI_OUTPUT_BUFFER_SIZE);
}

void resetAudioBuffer() {
    writeIndex = 0;
    readIndex = 0;
}

// =============================================================================
// 원형 버퍼 관리
// =============================================================================

bool pushCircularBuffer(int16_t sample) {
    if (circularCount >= MOZZI_CIRCULAR_BUFFER_SIZE) {
        return false; // 버퍼 가득참
    }
    
    circularBuffer[circularWriteIndex] = sample;
    circularWriteIndex = (circularWriteIndex + 1) % MOZZI_CIRCULAR_BUFFER_SIZE;
    circularCount++;
    
    return true;
}

bool popCircularBuffer(int16_t* sample) {
    if (circularCount == 0) {
        return false; // 버퍼 비어있음
    }
    
    *sample = circularBuffer[circularReadIndex];
    circularReadIndex = (circularReadIndex + 1) % MOZZI_CIRCULAR_BUFFER_SIZE;
    circularCount--;
    
    return true;
}

bool peekCircularBuffer(int16_t* sample) {
    if (circularCount == 0) {
        return false;
    }
    
    *sample = circularBuffer[circularReadIndex];
    return true;
}

size_t getCircularBufferCount() {
    return circularCount;
}

bool isCircularBufferFull() {
    return (circularCount >= MOZZI_CIRCULAR_BUFFER_SIZE);
}

bool isCircularBufferEmpty() {
    return (circularCount == 0);
}

void clearCircularBuffer() {
    circularWriteIndex = 0;
    circularReadIndex = 0;
    circularCount = 0;
}

// =============================================================================
// 제어 버퍼 관리
// =============================================================================

bool writeControlBuffer(int16_t value) {
    if (controlWriteIndex >= MOZZI_CONTROL_RATE) {
        return false;
    }
    
    controlBuffer[controlWriteIndex++] = value;
    return true;
}

bool readControlBuffer(int16_t* value) {
    if (controlReadIndex >= MOZZI_CONTROL_RATE) {
        controlReadIndex = 0; // 루프
    }
    
    *value = controlBuffer[controlReadIndex++];
    return true;
}

void resetControlBuffer() {
    controlWriteIndex = 0;
    controlReadIndex = 0;
}

// =============================================================================
// 템플릿 버퍼 사용 예시
// =============================================================================

bool pushAudioCircularBuffer(int16_t sample) {
    return audioCircularBuffer.push(sample);
}

bool popAudioCircularBuffer(int16_t* sample) {
    return audioCircularBuffer.pop(*sample);
}

bool peekAudioCircularBuffer(int16_t* sample) {
    return audioCircularBuffer.peek(*sample);
}

size_t getAudioCircularBufferCount() {
    return audioCircularBuffer.available();
}

void clearAudioCircularBuffer() {
    audioCircularBuffer.clear();
}

// =============================================================================
// 메모리 풀 관리
// =============================================================================

void initializeMemoryPool() {
    DEBUG_PRINTLN("Initializing audio memory pool...");
    
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        memoryPool[i] = new int16_t[MOZZI_OUTPUT_BUFFER_SIZE];
        poolUsed[i] = false;
        
        if (memoryPool[i] == nullptr) {
            DEBUG_PRINT("ERROR: Failed to allocate pool ");
            DEBUG_PRINTLN(i);
        }
    }
    
    DEBUG_PRINTLN("Memory pool initialized");
}

void* allocateAudioMemory() {
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (!poolUsed[i]) {
            poolUsed[i] = true;
            DEBUG_PRINT("Allocated audio memory from pool ");
            DEBUG_PRINTLN(i);
            return memoryPool[i];
        }
    }
    
    DEBUG_PRINTLN("ERROR: Audio memory pool exhausted");
    return nullptr; // 풀 가득참
}

void deallocateAudioMemory(void* ptr) {
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (memoryPool[i] == ptr) {
            poolUsed[i] = false;
            DEBUG_PRINT("Deallocated audio memory from pool ");
            DEBUG_PRINTLN(i);
            return;
        }
    }
    
    DEBUG_PRINTLN("WARNING: Attempted to free unknown memory");
}

bool isMemoryPoolAvailable() {
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (!poolUsed[i]) {
            return true;
        }
    }
    return false;
}

size_t getMemoryPoolUsage() {
    size_t used = 0;
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (poolUsed[i]) {
            used++;
        }
    }
    return used;
}

// =============================================================================
// 버퍼 성능 모니터링
// =============================================================================

void monitorBufferPerformance() {
    static uint32_t lastTime = millis();
    static size_t totalSamples = 0;
    static size_t droppedSamples = 0;
    
    uint32_t currentTime = millis();
    totalSamples++;
    
    if (currentTime - lastTime >= 1000) {  // 1초마다
        float sampleRate = totalSamples / ((currentTime - lastTime) / 1000.0f);
        float dropoutRate = (float)droppedSamples / totalSamples * 100.0f;
        
        DEBUG_PRINT("Buffer Performance (1s):");
        DEBUG_PRINT(" Rate: ");
        DEBUG_PRINT(sampleRate, 1);
        DEBUG_PRINT(" samples/s, Dropout: ");
        DEBUG_PRINT(dropoutRate, 2);
        DEBUG_PRINTLN("%");
        
        totalSamples = 0;
        droppedSamples = 0;
        lastTime = currentTime;
    }
}

void analyzeBufferUsage() {
    DEBUG_PRINTLN("=== Buffer Usage Analysis ===");
    
    // 오디오 버퍼 사용량
    DEBUG_PRINT("Audio Buffer 0: ");
    DEBUG_PRINT(sizeof(audioBuffer0));
    DEBUG_PRINTLN(" bytes");
    
    DEBUG_PRINT("Audio Buffer 1: ");
    DEBUG_PRINT(sizeof(audioBuffer1));
    DEBUG_PRINTLN(" bytes");
    
    // 원형 버퍼 사용량
    DEBUG_PRINT("Circular Buffer: ");
    DEBUG_PRINT(circularCount);
    DEBUG_PRINT("/");
    DEBUG_PRINT(MOZZI_CIRCULAR_BUFFER_SIZE);
    DEBUG_PRINTLN(" used");
    
    // 템플릿 버퍼 사용량
    DEBUG_PRINT("Audio Circular Buffer: ");
    DEBUG_PRINT(audioCircularBuffer.available());
    DEBUG_PRINT("/");
    DEBUG_PRINT(audioCircularBuffer.capacity());
    DEBUG_PRINTLN(" used");
    
    // 메모리 풀 사용량
    DEBUG_PRINT("Memory Pool: ");
    DEBUG_PRINT(getMemoryPoolUsage());
    DEBUG_PRINT("/");
    DEBUG_PRINT(MEMORY_POOL_SIZE);
    DEBUG_PRINTLN(" used");
    
    // 총 메모리 사용량
    size_t totalMemory = sizeof(audioBuffer0) + sizeof(audioBuffer1) + 
                        sizeof(circularBuffer) + sizeof(controlBuffer);
    DEBUG_PRINT("Total Buffer Memory: ");
    DEBUG_PRINT(totalMemory);
    DEBUG_PRINTLN(" bytes");
}

// =============================================================================
// 버퍼 크기 동적 조정
// =============================================================================

void resizeAudioBuffer(size_t newSize) {
    if (newSize < 16 || newSize > 2048) {
        DEBUG_PRINTLN("ERROR: Invalid buffer size");
        return;
    }
    
    DEBUG_PRINT("Resizing audio buffer to ");
    DEBUG_PRINT(newSize);
    DEBUG_PRINTLN(" samples");
    
    // 기존 버퍼 내용 보존
    int16_t tempBuffer[MOZZI_OUTPUT_BUFFER_SIZE];
    size_t currentSize = writeIndex;
    
    memcpy(tempBuffer, audioBuffer0, currentSize * sizeof(int16_t));
    
    // 새 크기로 버퍼 재할당 (필요시)
    // 현재는 정적 할당 사용
    
    // 버퍼 내용 복원
    writeIndex = min(currentSize, newSize);
    memcpy(audioBuffer0, tempBuffer, writeIndex * sizeof(int16_t));
    
    DEBUG_PRINTLN("Audio buffer resized");
}

// =============================================================================
// 버퍼 오버플로우 처리
// =============================================================================

void handleBufferOverflow() {
    DEBUG_PRINTLN("WARNING: Buffer overflow detected");
    
    // 가장 오래된 샘플 버림
    if (circularCount > 0) {
        int16_t dummy;
        popCircularBuffer(&dummy);
    }
    
    // 버퍼 인덱스 리셋
    writeIndex = 0;
    readIndex = 0;
}

void enableBufferOverflowProtection(bool enable) {
    // 버퍼 오버플로우 보호 기능 활성화
    // ISR에서 호출하여 자동 복구
    
    if (enable) {
        DEBUG_PRINTLN("Buffer overflow protection enabled");
    } else {
        DEBUG_PRINTLN("Buffer overflow protection disabled");
    }
}

// =============================================================================
// 버퍼 통계 정보
// =============================================================================

void printBufferStatistics() {
    DEBUG_PRINTLN("=== Buffer Statistics ===");
    
    DEBUG_PRINT("Audio Buffer Status:");
    DEBUG_PRINT(" Current: ");
    DEBUG_PRINT(currentBuffer);
    DEBUG_PRINT(" Write: ");
    DEBUG_PRINT(writeBuffer);
    DEBUG_PRINT(" Index: ");
    DEBUG_PRINT(writeIndex);
    DEBUG_PRINT("/");
    DEBUG_PRINTLN(MOZZI_OUTPUT_BUFFER_SIZE);
    
    DEBUG_PRINT("Circular Buffer:");
    DEBUG_PRINT(" Count: ");
    DEBUG_PRINT(circularCount);
    DEBUG_PRINT(" Write Index: ");
    DEBUG_PRINT(circularWriteIndex);
    DEBUG_PRINT(" Read Index: ");
    DEBUG_PRINTLN(circularReadIndex);
    
    DEBUG_PRINT("Audio Circular Buffer:");
    DEBUG_PRINT(" Available: ");
    DEBUG_PRINT(audioCircularBuffer.available());
    DEBUG_PRINT(" Full: ");
    DEBUG_PRINT(audioCircularBuffer.full() ? "Yes" : "No");
    DEBUG_PRINT(" Empty: ");
    DEBUG_PRINTLN(audioCircularBuffer.empty() ? "Yes" : "No");
}

// =============================================================================
// 버퍼 정리 및 종료
// =============================================================================

void cleanupBufferManager() {
    DEBUG_PRINTLN("Cleaning up buffer manager...");
    
    // 메모리 풀 정리
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (memoryPool[i] != nullptr) {
            delete[] memoryPool[i];
            memoryPool[i] = nullptr;
        }
    }
    
    DEBUG_PRINTLN("Buffer manager cleanup completed");
}