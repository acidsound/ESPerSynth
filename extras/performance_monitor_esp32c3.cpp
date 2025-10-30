/*
 * ESP32C3 Mozzi Library 성능 모니터링 구현
 * 
 * 실시간 성능 측정, 지연 시간 분석, CPU 사용률 모니터링
 * ESP32C3의 RISC-V 아키텍처 특성을 고려한 최적화된 성능 추적
 */

#include "mozzi_config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// =============================================================================
// 성능 모니터링 전역 변수
// =============================================================================

static const char* TAG = "ESP32C3_Performance";

// 성능 측정 버퍼
#define PERFORMANCE_BUFFER_SIZE 1000
#define LATENCY_BUFFER_SIZE 100
#define CPU_MONITORING_PERIOD 1000  // 1초

// 오디오 성능 지표
volatile uint32_t audioSamplesProcessed = 0;
volatile uint32_t audioDroppedSamples = 0;
volatile uint32_t audioBufferOverflows = 0;

// 타이머 성능 지표
volatile uint32_t timerInterruptCount = 0;
volatile uint32_t timerInterruptMisses = 0;
volatile uint32_t maxTimerLatency = 0;
volatile uint32_t avgTimerLatency = 0;

// CPU 성능 지표
volatile uint32_t cpuUsagePercent = 0;
volatile uint32_t freeHeapBytes = 0;
volatile uint32_t minFreeHeapBytes = 0;

// 지연 시간 측정 버퍼
static uint32_t latencyBuffer[LATENCY_BUFFER_SIZE];
static size_t latencyIndex = 0;
static uint32_t lastInterruptTime = 0;

// 성능 통계 구조체
typedef struct {
    uint32_t min;
    uint32_t max;
    uint32_t avg;
    uint32_t count;
} PerformanceStats;

static PerformanceStats audioProcessingStats = {0};
static PerformanceStats bufferOperationStats = {0};
static PerformanceStats interruptServiceStats = {0};

// =============================================================================
// 성능 모니터링 초기화
// =============================================================================

void initializePerformanceMonitoring() {
    DEBUG_PRINTLN("Initializing ESP32C3 performance monitoring...");
    
    // 성능 버퍼 초기화
    memset(latencyBuffer, 0, sizeof(latencyBuffer));
    latencyIndex = 0;
    lastInterruptTime = 0;
    
    // 통계 구조체 초기화
    audioProcessingStats = (PerformanceStats){0, 0, 0, 0};
    bufferOperationStats = (PerformanceStats){0, 0, 0, 0};
    interruptServiceStats = (PerformanceStats){0, 0, 0, 0};
    
    // 성능 지표 초기화
    audioSamplesProcessed = 0;
    audioDroppedSamples = 0;
    audioBufferOverflows = 0;
    timerInterruptCount = 0;
    timerInterruptMisses = 0;
    maxTimerLatency = 0;
    avgTimerLatency = 0;
    cpuUsagePercent = 0;
    
    // 최소 힙 메모리 초기화
    freeHeapBytes = ESP.getFreeHeap();
    minFreeHeapBytes = freeHeapBytes;
    
    DEBUG_PRINTLN("Performance monitoring initialized");
    
#ifdef MONITOR_MEMORY_USAGE
    printMemoryUsage();
#endif
}

// =============================================================================
// 오디오 성능 측정
// =============================================================================

void startAudioProcessingTimer() {
#ifdef MEASURE_LATENCY
    lastInterruptTime = micros();
#endif
}

void endAudioProcessingTimer() {
#ifdef MEASURE_LATENCY
    uint32_t latency = micros() - lastInterruptTime;
    
    // 지연 시간 버퍼에 저장
    latencyBuffer[latencyIndex] = latency;
    latencyIndex = (latencyIndex + 1) % LATENCY_BUFFER_SIZE;
    
    // 통계 업데이트
    updatePerformanceStats(&audioProcessingStats, latency);
#endif
}

void incrementAudioSampleCount() {
    audioSamplesProcessed++;
    
#ifdef MONITOR_MEMORY_USAGE
    // 주기적으로 메모리 사용량 체크
    static uint32_t lastMemoryCheck = 0;
    if (millis() - lastMemoryCheck >= 5000) { // 5초마다
        uint32_t currentHeap = ESP.getFreeHeap();
        if (currentHeap < minFreeHeapBytes) {
            minFreeHeapBytes = currentHeap;
        }
        freeHeapBytes = currentHeap;
        lastMemoryCheck = millis();
    }
#endif
}

void recordAudioDrop() {
    audioDroppedSamples++;
}

void recordBufferOverflow() {
    audioBufferOverflows++;
}

// =============================================================================
// 인터럽트 성능 측정
// =============================================================================

void startInterruptTimer() {
#ifdef MEASURE_ISR_TIMING
    lastInterruptTime = micros();
#endif
}

void endInterruptTimer() {
#ifdef MEASURE_ISR_TIMING
    uint32_t isrTime = micros() - lastInterruptTime;
    
    timerInterruptCount++;
    
    // 최대 지연 시간 업데이트
    if (isrTime > maxTimerLatency) {
        maxTimerLatency = isrTime;
    }
    
    // 인터럽트 누락 감지 (지연이 2배 이상인 경우)
    if (isrTime > (TIMER_INTERVAL_US * 2)) {
        timerInterruptMisses++;
    }
    
    // 평균 지연 시간 계산 (移動平均)
    static uint32_t isrSamples = 0;
    static uint32_t isrSum = 0;
    
    isrSum += isrTime;
    isrSamples++;
    
    if (isrSamples >= 1000) { // 1000샘플마다 업데이트
        avgTimerLatency = isrSum / isrSamples;
        isrSum = 0;
        isrSamples = 0;
    }
    
    // 통계 업데이트
    updatePerformanceStats(&interruptServiceStats, isrTime);
#endif
}

// =============================================================================
// 통계 업데이트 함수
// =============================================================================

void updatePerformanceStats(PerformanceStats* stats, uint32_t value) {
    stats->count++;
    
    if (stats->min == 0 || value < stats->min) {
        stats->min = value;
    }
    
    if (value > stats->max) {
        stats->max = value;
    }
    
    // 평균 계산 (이동 평균)
    stats->avg = ((stats->avg * (stats->count - 1)) + value) / stats->count;
}

// =============================================================================
// CPU 사용률 모니터링
// =============================================================================

void updateCpuUsage() {
#ifdef MONITOR_CPU_USAGE
    static uint32_t lastIdleTime = 0;
    static uint32_t lastTotalTime = 0;
    
    uint32_t idleTime = xTaskGetIdleRunTimeCounter();
    uint32_t totalTime = xTaskGetTickCount();
    
    if (lastTotalTime > 0) {
        uint32_t idleDelta = idleTime - lastIdleTime;
        uint32_t totalDelta = totalTime - lastTotalTime;
        
        if (totalDelta > 0) {
            cpuUsagePercent = 100 - ((idleDelta * 100) / totalDelta);
        }
    }
    
    lastIdleTime = idleTime;
    lastTotalTime = totalTime;
#endif
}

// =============================================================================
// 성능 보고서 출력
// =============================================================================

void printPerformanceReport() {
    DEBUG_PRINTLN("=== ESP32C3 Performance Report ===");
    
    // 오디오 성능 정보
    DEBUG_PRINT("Audio Samples Processed: ");
    DEBUG_PRINTLN(audioSamplesProcessed);
    
    DEBUG_PRINT("Audio Dropped Samples: ");
    DEBUG_PRINTLN(audioDroppedSamples);
    
    DEBUG_PRINT("Buffer Overflows: ");
    DEBUG_PRINTLN(audioBufferOverflows);
    
    if (audioSamplesProcessed > 0) {
        float dropoutRate = (float)audioDroppedSamples / audioSamplesProcessed * 100.0f;
        DEBUG_PRINT("Dropout Rate: ");
        DEBUG_PRINT(dropoutRate, 3);
        DEBUG_PRINTLN("%");
    }
    
    // 타이머 성능 정보
    DEBUG_PRINT("Timer Interrupts: ");
    DEBUG_PRINTLN(timerInterruptCount);
    
    DEBUG_PRINT("Timer Interrupt Misses: ");
    DEBUG_PRINTLN(timerInterruptMisses);
    
    DEBUG_PRINT("Max Timer Latency: ");
    DEBUG_PRINT(maxTimerLatency);
    DEBUG_PRINTLN(" μs");
    
    DEBUG_PRINT("Avg Timer Latency: ");
    DEBUG_PRINT(avgTimerLatency);
    DEBUG_PRINTLN(" μs");
    
    // 오디오 처리 통계
    if (audioProcessingStats.count > 0) {
        DEBUG_PRINT("Audio Processing Stats (μs):");
        DEBUG_PRINT(" Min: ");
        DEBUG_PRINT(audioProcessingStats.min);
        DEBUG_PRINT(" Max: ");
        DEBUG_PRINT(audioProcessingStats.max);
        DEBUG_PRINT(" Avg: ");
        DEBUG_PRINT(audioProcessingStats.avg);
        DEBUG_PRINTLN();
    }
    
    // 인터럽트 서비스 통계
    if (interruptServiceStats.count > 0) {
        DEBUG_PRINT("ISR Stats (μs):");
        DEBUG_PRINT(" Min: ");
        DEBUG_PRINT(interruptServiceStats.min);
        DEBUG_PRINT(" Max: ");
        DEBUG_PRINT(interruptServiceStats.max);
        DEBUG_PRINT(" Avg: ");
        DEBUG_PRINT(interruptServiceStats.avg);
        DEBUG_PRINTLN();
    }
    
    // CPU 사용률 정보
    DEBUG_PRINT("CPU Usage: ");
    DEBUG_PRINT(cpuUsagePercent);
    DEBUG_PRINTLN("%");
    
    // 메모리 사용량 정보
    printMemoryUsage();
    
    // 시스템 정보
    printSystemInfo();
}

// =============================================================================
// 메모리 사용량 출력
// =============================================================================

void printMemoryUsage() {
    DEBUG_PRINTLN("=== Memory Usage ===");
    
    DEBUG_PRINT("Free Heap: ");
    DEBUG_PRINT(ESP.getFreeHeap());
    DEBUG_PRINTLN(" bytes");
    
    DEBUG_PRINT("Minimum Free Heap: ");
    DEBUG_PRINT(minFreeHeapBytes);
    DEBUG_PRINTLN(" bytes");
    
    DEBUG_PRINT("Heap Size: ");
    DEBUG_PRINT(ESP.getHeapSize());
    DEBUG_PRINTLN(" bytes");
    
    DEBUG_PRINT("Stack High Water Mark: ");
    DEBUG_PRINTLN(uxTaskGetStackHighWaterMark(NULL));
    
    // 힙 사용률 계산
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t usedHeap = totalHeap - freeHeap;
    float heapUsagePercent = (float)usedHeap / totalHeap * 100.0f;
    
    DEBUG_PRINT("Heap Usage: ");
    DEBUG_PRINT(usedHeap);
    DEBUG_PRINT("/");
    DEBUG_PRINT(totalHeap);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(heapUsagePercent, 1);
    DEBUG_PRINTLN("%)");
}

// =============================================================================
// 시스템 정보 출력
// =============================================================================

void printSystemInfo() {
    DEBUG_PRINTLN("=== System Information ===");
    
    DEBUG_PRINT("Chip: ");
    DEBUG_PRINTLN(ESP.getChipModel());
    
    DEBUG_PRINT("Chip Revision: ");
    DEBUG_PRINTLN(ESP.getChipRevision());
    
    DEBUG_PRINT("CPU Frequency: ");
    DEBUG_PRINT(ESP.getCpuFreqMHz());
    DEBUG_PRINTLN(" MHz");
    
    DEBUG_PRINT("Flash Size: ");
    DEBUG_PRINT(ESP.getFlashChipSize());
    DEBUG_PRINTLN(" bytes");
    
    DEBUG_PRINT("Flash Speed: ");
    DEBUG_PRINT(ESP.getFlashChipSpeed() / 1000000);
    DEBUG_PRINTLN(" MHz");
    
    DEBUG_PRINT("FreeRTOS Tick Rate: ");
    DEBUG_PRINT(configTICK_RATE_HZ);
    DEBUG_PRINTLN(" Hz");
}

// =============================================================================
// 지연 시간 분석
// =============================================================================

void analyzeLatency() {
    if (latencyIndex == 0) {
        DEBUG_PRINTLN("No latency data available");
        return;
    }
    
    DEBUG_PRINTLN("=== Latency Analysis ===");
    
    uint32_t min = UINT32_MAX;
    uint32_t max = 0;
    uint32_t sum = 0;
    uint32_t count = 0;
    
    // 지연 시간 통계 계산
    for (size_t i = 0; i < LATENCY_BUFFER_SIZE; i++) {
        if (latencyBuffer[i] > 0) {
            uint32_t val = latencyBuffer[i];
            min = min(min, val);
            max = max(max, val);
            sum += val;
            count++;
        }
    }
    
    if (count > 0) {
        uint32_t avg = sum / count;
        
        DEBUG_PRINT("Latency Stats (μs):");
        DEBUG_PRINT(" Min: ");
        DEBUG_PRINT(min);
        DEBUG_PRINT(" Max: ");
        DEBUG_PRINT(max);
        DEBUG_PRINT(" Avg: ");
        DEBUG_PRINT(avg);
        DEBUG_PRINT(" Count: ");
        DEBUG_PRINTLN(count);
        
        // 지연 시간 등급 평가
        if (avg < 10) {
            DEBUG_PRINTLN("Latency Grade: EXCELLENT");
        } else if (avg < 20) {
            DEBUG_PRINTLN("Latency Grade: GOOD");
        } else if (avg < 50) {
            DEBUG_PRINTLN("Latency Grade: FAIR");
        } else {
            DEBUG_PRINTLN("Latency Grade: POOR - Consider optimization");
        }
        
        // 목표 지연 시간 대비 평가
        uint32_t targetLatency = TIMER_INTERVAL_US;
        float latencyRatio = (float)avg / targetLatency;
        
        DEBUG_PRINT("Latency vs Target: ");
        DEBUG_PRINT(latencyRatio * 100, 1);
        DEBUG_PRINTLN("%");
        
        if (latencyRatio > 0.8) {
            DEBUG_PRINTLN("WARNING: Latency approaching target interval");
        }
    }
}

// =============================================================================
// 실시간 성능 모니터링 태스크
// =============================================================================

void performanceMonitoringTask(void* parameters) {
    DEBUG_PRINTLN("Starting performance monitoring task...");
    
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t monitorPeriod = pdMS_TO_TICKS(1000); // 1초
    
    while (1) {
        // CPU 사용률 업데이트
        updateCpuUsage();
        
        // 주기적 성능 보고 (선택사항)
        static uint32_t reportCount = 0;
        reportCount++;
        
        if (reportCount >= 60) { // 1분마다 간단한 보고
            DEBUG_PRINT("Performance Summary - CPU: ");
            DEBUG_PRINT(cpuUsagePercent);
            DEBUG_PRINT("%, Heap: ");
            DEBUG_PRINT(ESP.getFreeHeap());
            DEBUG_PRINTLN(" bytes");
            reportCount = 0;
        }
        
        // 다음 실행 대기
        vTaskDelayUntil(&lastWakeTime, monitorPeriod);
    }
}

// =============================================================================
// 성능 경고 시스템
// =============================================================================

void checkPerformanceWarnings() {
    bool warning = false;
    
    // CPU 사용률 경고
    if (cpuUsagePercent > 80) {
        DEBUG_PRINT("WARNING: High CPU usage: ");
        DEBUG_PRINT(cpuUsagePercent);
        DEBUG_PRINTLN("%");
        warning = true;
    }
    
    // 메모리 부족 경고
    if (ESP.getFreeHeap() < 10000) { // 10KB 미만
        DEBUG_PRINT("WARNING: Low memory: ");
        DEBUG_PRINT(ESP.getFreeHeap());
        DEBUG_PRINTLN(" bytes");
        warning = true;
    }
    
    // 지연 시간 경고
    if (avgTimerLatency > TIMER_INTERVAL_US * 0.7) {
        DEBUG_PRINT("WARNING: High latency: ");
        DEBUG_PRINT(avgTimerLatency);
        DEBUG_PRINT(" μs (target: ");
        DEBUG_PRINT(TIMER_INTERVAL_US);
        DEBUG_PRINTLN(" μs)");
        warning = true;
    }
    
    // 버퍼 오버플로우 경고
    if (audioBufferOverflows > 10) {
        DEBUG_PRINT("WARNING: Multiple buffer overflows: ");
        DEBUG_PRINTLN(audioBufferOverflows);
        warning = true;
    }
    
    if (warning) {
        DEBUG_PRINTLN("Performance degradation detected - consider optimization");
    }
}

// =============================================================================
// 성능 벤치마크 테스트
// =============================================================================

void runPerformanceBenchmark() {
    DEBUG_PRINTLN("=== Performance Benchmark ===");
    
    // 오디오 처리 벤치마크
    uint32_t startTime = micros();
    const uint32_t testSamples = 10000;
    
    for (uint32_t i = 0; i < testSamples; i++) {
        // 오디오 처리 시뮬레이션
        int16_t sample = (int16_t)(32767 * sin(i * 2.0 * PI * 440.0 / MOZZI_AUDIO_RATE));
        audioOutput(sample);
    }
    
    uint32_t endTime = micros();
    uint32_t processingTime = endTime - startTime;
    float samplesPerSecond = (float)testSamples / (processingTime / 1000000.0f);
    
    DEBUG_PRINT("Audio Processing Benchmark:");
    DEBUG_PRINT(" Samples: ");
    DEBUG_PRINTLN(testSamples);
    DEBUG_PRINT(" Total Time: ");
    DEBUG_PRINT(processingTime);
    DEBUG_PRINTLN(" μs");
    DEBUG_PRINT(" Processing Rate: ");
    DEBUG_PRINT(samplesPerSecond, 1);
    DEBUG_PRINTLN(" samples/sec");
    DEBUG_PRINT(" Per Sample: ");
    DEBUG_PRINT(processingTime / testSamples);
    DEBUG_PRINTLN(" μs");
    
    // 성능 등급 평가
    float targetTimePerSample = (float)TIMER_INTERVAL_US;
    float actualTimePerSample = (float)processingTime / testSamples;
    
    if (actualTimePerSample < targetTimePerSample * 0.5) {
        DEBUG_PRINTLN("Performance Grade: EXCELLENT");
    } else if (actualTimePerSample < targetTimePerSample * 0.8) {
        DEBUG_PRINTLN("Performance Grade: GOOD");
    } else if (actualTimePerSample < targetTimePerSample) {
        DEBUG_PRINTLN("Performance Grade: FAIR");
    } else {
        DEBUG_PRINTLN("Performance Grade: POOR - Optimization needed");
    }
}

// =============================================================================
// 성능 데이터 초기화
// =============================================================================

void resetPerformanceCounters() {
    DEBUG_PRINTLN("Resetting performance counters...");
    
    // 오디오 성능 지표 초기화
    audioSamplesProcessed = 0;
    audioDroppedSamples = 0;
    audioBufferOverflows = 0;
    
    // 타이머 성능 지표 초기화
    timerInterruptCount = 0;
    timerInterruptMisses = 0;
    maxTimerLatency = 0;
    avgTimerLatency = 0;
    
    // 통계 구조체 초기화
    audioProcessingStats = (PerformanceStats){0, 0, 0, 0};
    bufferOperationStats = (PerformanceStats){0, 0, 0, 0};
    interruptServiceStats = (PerformanceStats){0, 0, 0, 0};
    
    // 지연 시간 버퍼 초기화
    memset(latencyBuffer, 0, sizeof(latencyBuffer));
    latencyIndex = 0;
    
    DEBUG_PRINTLN("Performance counters reset completed");
}

// =============================================================================
// 성능 모니터링 시작
// =============================================================================

void startPerformanceMonitoring() {
    // 성능 모니터링 태스크 시작
    xTaskCreate(performanceMonitoringTask, 
                "PerfMonitor", 
                2048,      // 스택 크기
                NULL,      // 파라미터
                2,         // 우선순위
                NULL);     // 태스크 핸들러
    
    DEBUG_PRINTLN("Performance monitoring started");
}

void stopPerformanceMonitoring() {
    // 성능 모니터링 중지
    // (실제로는 태스크 핸들러가 필요함)
    DEBUG_PRINTLN("Performance monitoring stopped");
}