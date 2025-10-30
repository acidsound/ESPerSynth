/*
 * ESP32C3 Mozzi Library 통합 구현
 * 
 * ESP32C3와 Mozzi Library의 완전한 통합 구현
 * 모든 기능을 통합하여 쉽게 사용할 수 있는 단일 클래스 제공
 */

#include "esp32c3_mozzi_integration.h"
#include "esp_log.h"

// =============================================================================
// 전역 변수 및 상수
// =============================================================================

static const char* TAG = "ESP32C3_Mozzi";

// 전역 시스템 인스턴스
ESP32C3Mozzi mozziSystem;

// =============================================================================
// ESP32C3Mozzi 클래스 구현
// =============================================================================

ESP32C3Mozzi::ESP32C3Mozzi() 
    : initialized(false), audioActive(false), performanceMonitoring(false) {
    DEBUG_PRINTLN("ESP32C3Mozzi system created");
}

bool ESP32C3Mozzi::initialize() {
    DEBUG_PRINTLN("Initializing ESP32C3 Mozzi System...");
    
    // 각子系统 순차 초기화
    if (!initializeAudio()) {
        DEBUG_PRINTLN("Failed to initialize audio system");
        return false;
    }
    
    if (!initializeTimers()) {
        DEBUG_PRINTLN("Failed to initialize timer system");
        return false;
    }
    
    if (!initializeBuffers()) {
        DEBUG_PRINTLN("Failed to initialize buffer system");
        return false;
    }
    
    if (!initializePerformanceMonitoring()) {
        DEBUG_PRINTLN("Failed to initialize performance monitoring");
        // 성능 모니터링 실패는 치명적이지 않음
    }
    
    initialized = true;
    DEBUG_PRINTLN("ESP32C3 Mozzi System initialized successfully");
    
    // 시스템 상태 출력
    printConfiguration();
    
    return true;
}

bool ESP32C3Mozzi::initializeAudio() {
    DEBUG_PRINTLN("Initializing audio system...");
    
    try {
        // 오디오 출력 초기화
        initializeAudioOutput();
        initializeAudioBuffers();
        
        DEBUG_PRINTLN("Audio system initialized");
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Audio system initialization failed");
        return false;
    }
}

bool ESP32C3Mozzi::initializeTimers() {
    DEBUG_PRINTLN("Initializing timer system...");
    
    try {
        // 타이머 인터럽트 초기화
        initializeTimerInterrupts();
        
        // 타이머 설정 검증
        debugTimerConfiguration();
        
        DEBUG_PRINTLN("Timer system initialized");
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Timer system initialization failed");
        return false;
    }
}

bool ESP32C3Mozzi::initializeBuffers() {
    DEBUG_PRINTLN("Initializing buffer system...");
    
    try {
        // 버퍼 관리자 초기화
        initializeBufferManager();
        
        // 버퍼 통계 출력
        printBufferStatistics();
        
        DEBUG_PRINTLN("Buffer system initialized");
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Buffer system initialization failed");
        return false;
    }
}

bool ESP32C3Mozzi::initializePerformanceMonitoring() {
    DEBUG_PRINTLN("Initializing performance monitoring...");
    
    try {
        // 성능 모니터링 초기화
        initializePerformanceMonitoring();
        
        // 성능 모니터링 시작
        startPerformanceMonitoring();
        
        performanceMonitoring = true;
        DEBUG_PRINTLN("Performance monitoring initialized");
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Performance monitoring initialization failed");
        performanceMonitoring = false;
        return false;
    }
}

bool ESP32C3Mozzi::startAudio() {
    if (!initialized) {
        DEBUG_PRINTLN("Cannot start audio: system not initialized");
        return false;
    }
    
    DEBUG_PRINTLN("Starting audio system...");
    
    try {
        // 타이머 시작
        startAudioTimer();
        startControlTimer();
        
        audioActive = true;
        DEBUG_PRINTLN("Audio system started");
        
        // 시작 상태 확인
        if (!isAudioTimerRunning()) {
            DEBUG_PRINTLN("WARNING: Audio timer not running");
            audioActive = false;
            return false;
        }
        
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Failed to start audio system");
        audioActive = false;
        return false;
    }
}

bool ESP32C3Mozzi::stopAudio() {
    DEBUG_PRINTLN("Stopping audio system...");
    
    try {
        // 타이머 정지
        stopAudioTimer();
        stopControlTimer();
        
        audioActive = false;
        DEBUG_PRINTLN("Audio system stopped");
        return true;
    }
    catch (...) {
        DEBUG_PRINTLN("Failed to stop audio system");
        return false;
    }
}

bool ESP32C3Mozzi::restartAudio() {
    DEBUG_PRINTLN("Restarting audio system...");
    
    if (stopAudio()) {
        delay(100); // 짧은 지연
        return startAudio();
    }
    
    return false;
}

void ESP32C3Mozzi::printSystemStatus() {
    DEBUG_PRINTLN("=== ESP32C3 Mozzi System Status ===");
    
    DEBUG_PRINT("System Initialized: ");
    DEBUG_PRINTLN(initialized ? "Yes" : "No");
    DEBUG_PRINT("Audio Active: ");
    DEBUG_PRINTLN(audioActive ? "Yes" : "No");
    DEBUG_PRINT("Performance Monitoring: ");
    DEBUG_PRINTLN(performanceMonitoring ? "Active" : "Inactive");
    
    // 오디오 상태
    DEBUG_PRINT("Audio Timer: ");
    DEBUG_PRINTLN(isAudioTimerRunning() ? "Running" : "Stopped");
    DEBUG_PRINT("Control Timer: ");
    DEBUG_PRINTLN(isControlTimerRunning() ? "Running" : "Stopped");
    
    // 버퍼 상태
    DEBUG_PRINT("Audio Buffer: ");
    DEBUG_PRINTLN(isBufferFull() ? "Full" : "Available");
    DEBUG_PRINT("Circular Buffer: ");
    DEBUG_PRINTLN(isCircularBufferEmpty() ? "Empty" : "Has Data");
    
    // 시스템 정보
    printSystemInfo();
}

void ESP32C3Mozzi::printPerformanceReport() {
    if (!performanceMonitoring) {
        DEBUG_PRINTLN("Performance monitoring not active");
        return;
    }
    
    printPerformanceReport();
    analyzeLatency();
}

void ESP32C3Mozzi::printBufferStatus() {
    DEBUG_PRINTLN("=== Buffer Status ===");
    
    printBufferStatistics();
    analyzeBufferUsage();
}

void ESP32C3Mozzi::printTimerStatus() {
    DEBUG_PRINTLN("=== Timer Status ===");
    
    printTimerPerformanceReport();
    validateAudioFrequency();
}

void ESP32C3Mozzi::printConfiguration() {
    DEBUG_PRINTLN("=== ESP32C3 Mozzi Configuration ===");
    
    DEBUG_PRINT("Audio Rate: ");
    DEBUG_PRINT(MOZZI_AUDIO_RATE);
    DEBUG_PRINTLN(" Hz");
    
    DEBUG_PRINT("Control Rate: ");
    DEBUG_PRINT(MOZZI_CONTROL_RATE);
    DEBUG_PRINTLN(" Hz");
    
    DEBUG_PRINT("Buffer Size: ");
    DEBUG_PRINT(MOZZI_OUTPUT_BUFFER_SIZE);
    DEBUG_PRINTLN(" samples");
    
    DEBUG_PRINT("Timer Interval: ");
    DEBUG_PRINT(TIMER_INTERVAL_US);
    DEBUG_PRINTLN(" μs");
    
    DEBUG_PRINT("Platform: ");
#ifdef PLATFORM_ESP32C3
    DEBUG_PRINTLN("ESP32C3");
#else
    DEBUG_PRINTLN("Unknown");
#endif
    
    DEBUG_PRINT("Timer Interrupt: ");
#ifdef ESP32C3_TIMER_INTERRUPT
    DEBUG_PRINTLN("Enabled");
#else
    DEBUG_PRINTLN("Disabled");
#endif
    
    DEBUG_PRINT("Performance Monitoring: ");
#ifdef ENABLE_PERFORMANCE_MONITORING
    DEBUG_PRINTLN("Enabled");
#else
    DEBUG_PRINTLN("Disabled");
#endif
}

void ESP32C3Mozzi::runSelfTest() {
    DEBUG_PRINTLN("=== Running ESP32C3 Mozzi Self Test ===");
    
    // 1. 시스템 상태 확인
    DEBUG_PRINTLN("1. System Status Check");
    printSystemStatus();
    
    // 2. 오디오 시스템 테스트
    DEBUG_PRINTLN("2. Audio System Test");
    runAudioTest();
    
    // 3. 타이머 시스템 테스트
    DEBUG_PRINTLN("3. Timer System Test");
    validateAudioFrequency();
    
    // 4. 버퍼 시스템 테스트
    DEBUG_PRINTLN("4. Buffer System Test");
    analyzeBufferUsage();
    
    // 5. 성능 벤치마크
    DEBUG_PRINTLN("5. Performance Benchmark");
    runPerformanceBenchmark();
    
    DEBUG_PRINTLN("=== Self Test Completed ===");
}

void ESP32C3Mozzi::runAudioTest() {
    DEBUG_PRINTLN("Running audio test...");
    
    // 440Hz 톤 생성 테스트
    DEBUG_PRINT("Generating 440Hz tone for 1 second...");
    testAudioOutput();
    DEBUG_PRINTLN("Done");
    
    // 오디오 출력 상태 확인
    printAudioOutputStatus();
    
    DEBUG_PRINTLN("Audio test completed");
}

void ESP32C3Mozzi::runPerformanceBenchmark() {
    DEBUG_PRINTLN("Running performance benchmark...");
    
    // 성능 벤치마크 실행
    ::runPerformanceBenchmark();
    
    // 추가 성능 정보
    printPerformanceReport();
}

void ESP32C3Mozzi::validateConfiguration() {
    DEBUG_PRINTLN("Validating configuration...");
    
    bool valid = true;
    
    // AudioRate 검증
    if (MOZZI_AUDIO_RATE > 32768) {
        DEBUG_PRINTLN("ERROR: Audio rate too high for ESP32C3");
        valid = false;
    }
    
    // 버퍼 크기 검증
    if (MOZZI_OUTPUT_BUFFER_SIZE < 64 || MOZZI_OUTPUT_BUFFER_SIZE > 1024) {
        DEBUG_PRINTLN("ERROR: Invalid buffer size");
        valid = false;
    }
    
    // 타이머 설정 검증
    if (TIMER_INTERVAL_US < 10) {
        DEBUG_PRINTLN("ERROR: Timer interval too small");
        valid = false;
    }
    
    if (valid) {
        DEBUG_PRINTLN("Configuration validation passed");
    } else {
        DEBUG_PRINTLN("Configuration validation failed");
    }
}

void ESP32C3Mozzi::resetAllCounters() {
    DEBUG_PRINTLN("Resetting all performance counters...");
    
    resetPerformanceCounters();
    resetAudioBuffer();
    clearCircularBuffer();
    
    DEBUG_PRINTLN("All counters reset");
}

void ESP32C3Mozzi::emergencyStop() {
    DEBUG_PRINTLN("EMERGENCY STOP ACTIVATED");
    
    // 모든 시스템 즉시 정지
    audioActive = false;
    performanceMonitoring = false;
    
    stopAudioTimer();
    stopControlTimer();
    
    DEBUG_PRINTLN("Emergency stop completed");
}

bool ESP32C3Mozzi::isSystemHealthy() {
    // 시스템 건강 상태 점검
    if (!initialized) return false;
    if (!isAudioTimerRunning()) return false;
    if (ESP.getFreeHeap() < 5000) return false; // 메모리 부족
    
    return true;
}

// =============================================================================
// 기본 콜백 함수 구현
// =============================================================================

void audioHook() {
    // 성능 모니터링 시작
    startAudioProcessingTimer();
    
    // Mozzi 오디오 처리 로직이 여기에 들어감
    // 예: Oscil, WaveShaper 등의 Mozzi 오브젝트들
    
    // 성능 모니터링 끝
    endAudioProcessingTimer();
    incrementAudioSampleCount();
}

void updateControl() {
    // Mozzi 제어 업데이트 로직
    // 예: ControlChange, Random, ControlDelay 등
}

// =============================================================================
// Arduino 호환성 함수
// =============================================================================

// Arduino IDE에서 Mozzi 사용 시 호출되는 함수들
void startMozzi() {
    mozziSystem.initialize();
    mozziSystem.startAudio();
}

// =============================================================================
// 통합 유틸리티 함수
// =============================================================================

// 빠른 시스템 상태 확인
bool isMozziSystemReady() {
    return mozziSystem.isInitialized() && mozziSystem.isAudioActive();
}

// 시스템 정보 간단 출력
void printQuickStatus() {
    DEBUG_PRINT("ESP32C3 Mozzi: ");
    DEBUG_PRINT(isMozziSystemReady() ? "Ready" : "Not Ready");
    DEBUG_PRINT(" | Heap: ");
    DEBUG_PRINT(ESP.getFreeHeap());
    DEBUG_PRINT(" | CPU: ");
    DEBUG_PRINT(ESP.getCpuFreqMHz());
    DEBUG_PRINTLN("MHz");
}

// =============================================================================
// 예외 처리 및 안전성 기능
// =============================================================================

// 시스템 오류 처리
void handleMozziError(const char* errorMessage) {
    DEBUG_PRINT("MOZZI ERROR: ");
    DEBUG_PRINTLN(errorMessage);
    
    // emergency stop
    mozziSystem.emergencyStop();
}

// Watchdog 기능 (필요시)
void mozziWatchdogKick() {
    //ESP32C3의 hardware watchdog 사용
    //테스트 시에는 비활성화하는 것이 좋음
}

// =============================================================================
// 종료 정리
// =============================================================================

void cleanupMozziSystem() {
    DEBUG_PRINTLN("Cleaning up Mozzi system...");
    
    mozziSystem.emergencyStop();
    
    // cleanup functions
    cleanupBufferManager();
    
    DEBUG_PRINTLN("Mozzi system cleanup completed");
}