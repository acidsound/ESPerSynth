/*
 * ESP32C3 Mozzi Library 통합 헤더 파일
 * 
 * 모든 ESP32C3 + Mozzi Library 관련 기능을 포함하는 메인 헤더
 * 통합 설정, 초기화, 관리 기능을 제공
 */

#ifndef ESP32C3_MOZZI_INTEGRATION_H
#define ESP32C3_MOZZI_INTEGRATION_H

#include "mozzi_config.h"
#include <Arduino.h>

// =============================================================================
// 버전 정보
// =============================================================================

#define ESP32C3_MOZZI_VERSION "1.0.0"
#define ESP32C3_MOZZI_BUILD_DATE "2025-10-30"

// =============================================================================
// 전방 선언
// =============================================================================

// AudioOutput.cpp 함수들
void initializeAudioOutput();
void audioOutput(int16_t output);
void initializeAudioBuffers();
bool bufferAudioSample(int16_t sample);
bool getAudioSample(int16_t *sample);
bool isBufferFull();
bool isBufferEmpty();
void switchBuffer();
void testAudioOutput();
void printAudioOutputStatus();

// TimerInterrupt.cpp 함수들
void initializeTimerInterrupts();
void startAudioTimer();
void stopAudioTimer();
void startControlTimer();
void stopControlTimer();
bool isAudioTimerRunning();
bool isControlTimerRunning();
void restartAudioTimer();
void restartControlTimer();
void validateAudioFrequency();
void debugTimerConfiguration();
void printTimerPerformanceReport();

// BufferManager.cpp 함수들
void initializeBufferManager();
bool writeToAudioBuffer(int16_t sample);
bool readFromAudioBuffer(int16_t *sample);
bool isAudioBufferReady();
bool isAudioBufferFull();
void resetAudioBuffer();
bool pushCircularBuffer(int16_t sample);
bool popCircularBuffer(int16_t *sample);
bool isCircularBufferFull();
bool isCircularBufferEmpty();
void clearCircularBuffer();
void printBufferStatistics();

// PerformanceMonitor.cpp 함수들
void initializePerformanceMonitoring();
void startAudioProcessingTimer();
void endAudioProcessingTimer();
void incrementAudioSampleCount();
void startInterruptTimer();
void endInterruptTimer();
void printPerformanceReport();
void analyzeLatency();
void runPerformanceBenchmark();
void resetPerformanceCounters();
void startPerformanceMonitoring();
void stopPerformanceMonitoring();

// =============================================================================
// ESP32C3 Mozzi 통합 클래스
// =============================================================================

class ESP32C3Mozzi {
private:
    bool initialized;
    bool audioActive;
    bool performanceMonitoring;
    
public:
    ESP32C3Mozzi();
    
    // 초기화 함수들
    bool initialize();
    bool initializeAudio();
    bool initializeTimers();
    bool initializeBuffers();
    bool initializePerformanceMonitoring();
    
    // 실행 제어 함수들
    bool startAudio();
    bool stopAudio();
    bool restartAudio();
    
    // 상태 확인 함수들
    bool isInitialized() const { return initialized; }
    bool isAudioActive() const { return audioActive; }
    bool isPerformanceMonitoringActive() const { return performanceMonitoring; }
    
    // 정보 출력 함수들
    void printSystemStatus();
    void printPerformanceReport();
    void printBufferStatus();
    void printTimerStatus();
    void printConfiguration();
    
    // 테스트 함수들
    void runSelfTest();
    void runAudioTest();
    void runPerformanceBenchmark();
    void validateConfiguration();
    
    // 유틸리티 함수들
    void resetAllCounters();
    void emergencyStop();
    bool isSystemHealthy();
};

// =============================================================================
// 전역 인스턴스
// =============================================================================

extern ESP32C3Mozzi mozziSystem;

// =============================================================================
// 편의 매크로
// =============================================================================

// 시스템 초기화
#define MOZZI_SYSTEM_INIT() mozziSystem.initialize()
#define MOZZI_AUDIO_INIT() mozziSystem.initializeAudio()
#define MOZZI_TIMER_INIT() mozziSystem.initializeTimers()
#define MOZZI_BUFFER_INIT() mozziSystem.initializeBuffers()
#define MOZZI_PERF_INIT() mozziSystem.initializePerformanceMonitoring()

// 시스템 시작/정지
#define MOZZI_START() mozziSystem.startAudio()
#define MOZZI_STOP() mozziSystem.stopAudio()
#define MOZZI_RESTART() mozziSystem.restartAudio()

// 상태 확인
#define MOZZI_IS_READY() mozziSystem.isInitialized()
#define MOZZI_IS_RUNNING() mozziSystem.isAudioActive()
#define MOZZI_IS_HEALTHY() mozziSystem.isSystemHealthy()

// 정보 출력
#define MOZZI_PRINT_STATUS() mozziSystem.printSystemStatus()
#define MOZZI_PRINT_PERF() mozziSystem.printPerformanceReport()
#define MOZZI_PRINT_BUFFERS() mozziSystem.printBufferStatus()
#define MOZZI_PRINT_TIMERS() mozziSystem.printTimerStatus()

// 테스트
#define MOZZI_SELF_TEST() mozziSystem.runSelfTest()
#define MOZZI_AUDIO_TEST() mozziSystem.runAudioTest()
#define MOZZI_BENCHMARK() mozziSystem.runPerformanceBenchmark()

// =============================================================================
// 기본 제공 콜백 함수
// =============================================================================

// Mozzi에서 호출되는 오디오 훅 (기본 구현)
void audioHook();

// Mozzi에서 호출되는 제어 업데이트 (빈 구현)
void updateControl();

// =============================================================================
// 예외 처리 및 디버깅
// =============================================================================

// 오류 코드 정의
#define MOZZI_ERROR_NONE 0
#define MOZZI_ERROR_INIT_FAILED 1
#define MOZZI_ERROR_TIMER_FAILED 2
#define MOZZI_ERROR_BUFFER_FAILED 3
#define MOZZI_ERROR_AUDIO_FAILED 4
#define MOZZI_ERROR_MEMORY_FAILED 5

// 오류 처리 매크로
#ifdef DEBUG_MOZZI_ESP32C3
    #define MOZZI_CHECK_ERROR(condition, error_code) \
        if (!(condition)) { \
            DEBUG_PRINTLN("ERROR: " #condition " failed, code: " error_code); \
            return false; \
        }
#else
    #define MOZZI_CHECK_ERROR(condition, error_code) \
        if (!(condition)) { \
            return false; \
        }
#endif

// =============================================================================
// 호환성 함수들
// =============================================================================

// Arduino Mozzi 호환성
void startMozzi() {
    mozziSystem.initialize();
    mozziSystem.startAudio();
}

// =============================================================================
// 설정 검증 및 자동 설정
// =============================================================================

// 컴파일 타임 설정 검증
static_assert(MOZZI_AUDIO_RATE <= 32768, "Audio rate too high for ESP32C3");
static_assert(MOZZI_OUTPUT_BUFFER_SIZE >= 64, "Buffer size too small");
static_assert(MOZZI_OUTPUT_BUFFER_SIZE <= 1024, "Buffer size too large");

// =============================================================================
// 사용 예시 템플릿
// =============================================================================

/*
// 기본 사용 예시:
#include "esp32c3_mozzi_integration.h"

void setup() {
    Serial.begin(115200);
    
    // 시스템 초기화
    if (!mozziSystem.initialize()) {
        Serial.println("Mozzi system initialization failed");
        return;
    }
    
    // 오디오 테스트
    mozziSystem.runAudioTest();
    
    Serial.println("ESP32C3 + Mozzi ready");
}

void loop() {
    // Mozzi 오디오 처리
    audioHook();
    
    // 주기적으로 상태 확인
    static uint32_t lastStatus = 0;
    if (millis() - lastStatus > 10000) { // 10초마다
        mozziSystem.printSystemStatus();
        lastStatus = millis();
    }
}
*/

#endif /* ESP32C3_MOZZI_INTEGRATION_H */