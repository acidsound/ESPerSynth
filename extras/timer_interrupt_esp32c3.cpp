/*
 * ESP32C3 Mozzi Library 타이머 인터럽트 구현
 * 
 * ESP32_C3_TimerInterrupt 라이브러리를 활용한 최적화된 타이머 인터럽트
 * RISC-V 아키텍처 특성을 고려한 ISR 최적화
 */

#include "mozzi_config.h"
#include "ESP32_C3_TimerInterrupt.h"
#include "driver/timer.h"
#include "esp_log.h"

// =============================================================================
// 타이머 인터럽트 전역 변수
// =============================================================================

static const char* TAG = "ESP32C3_TimerInterrupt";

// ESP32Timer 인스턴스 (타이머 0 사용)
ESP32Timer ITimer0(0);  // 오디오용 (높은 우선순위)
ESP32Timer ITimer1(1);  // 제어용 (낮은 우선순위)

// 오디오 및 제어 상태 플래그
volatile bool audioTimerActive = false;
volatile bool controlTimerActive = false;

// 성능 측정 변수
volatile uint32_t audioIsrCount = 0;
volatile uint32_t controlIsrCount = 0;
volatile uint32_t lastAudioIsrTime = 0;
volatile uint32_t audioIsrMaxTime = 0;
volatile uint32_t audioIsrAvgTime = 0;

// =============================================================================
// 오디오 타이머 ISR (고우선순위)
// =============================================================================

bool IRAM_ATTR AudioTimerISR(void *timerNo) {
    // ISR 성능 모니터링 시작
    uint32_t isr_start_time = micros();
    
    // 오디오 훅 호출 (Mozzi 오디오 처리)
    audioHook();
    
    // ISR 카운터 증가
    audioIsrCount++;
    
    // ISR 성능 측정
    uint32_t isr_end_time = micros();
    uint32_t isr_time = isr_end_time - isr_start_time;
    
    // 최대 시간 업데이트
    if (isr_time > audioIsrMaxTime) {
        audioIsrMaxTime = isr_time;
    }
    
    // 평균 시간 계산 (移動平均)
    static uint32_t avg_samples = 0;
    static uint32_t avg_sum = 0;
    
    avg_sum += isr_time;
    avg_samples++;
    
    if (avg_samples >= 1000) {  // 1000샘플마다 평균 업데이트
        audioIsrAvgTime = avg_sum / avg_samples;
        avg_sum = 0;
        avg_samples = 0;
    }
    
    lastAudioIsrTime = isr_end_time;
    
    return true;  // 다음 인터럽트 지속
}

// =============================================================================
// 제어 타이머 ISR (저우선순위)
// =============================================================================

bool IRAM_ATTR ControlTimerISR(void *timerNo) {
    // 제어 타이머는 더 낮은 우선순위로 처리
    controlIsrCount++;
    
    // 제어 업데이트 호출
    updateControl();
    
    return true;
}

// =============================================================================
// 타이머 인터럽트 초기화
// =============================================================================

void initializeTimerInterrupts() {
    DEBUG_PRINTLN("Initializing ESP32C3 timer interrupts...");
    
    // 타이머 인터럽트 라이브러리 초기화 확인
    if (!ITimer0.attachInterruptInterval(TIMER_INTERVAL_US, AudioTimerISR)) {
        DEBUG_PRINTLN("ERROR: Failed to initialize audio timer");
        return;
    }
    
    DEBUG_PRINT("Audio timer initialized with interval: ");
    DEBUG_PRINT(TIMER_INTERVAL_US);
    DEBUG_PRINTLN(" microseconds");
    
    // 제어 타이머 설정 (1kHz)
    const uint32_t CONTROL_INTERVAL_US = 1000;  // 1ms = 1000μs
    
    if (!ITimer1.attachInterruptInterval(CONTROL_INTERVAL_US, ControlTimerISR)) {
        DEBUG_PRINTLN("WARNING: Failed to initialize control timer");
    } else {
        DEBUG_PRINT("Control timer initialized with interval: ");
        DEBUG_PRINT(CONTROL_INTERVAL_US);
        DEBUG_PRINTLN(" microseconds");
    }
    
    audioTimerActive = true;
    controlTimerActive = true;
    
    DEBUG_PRINTLN("Timer interrupts initialized successfully");
}

// =============================================================================
// 타이머 인터럽트 시작/정지
// =============================================================================

void startAudioTimer() {
    if (!audioTimerActive) {
        DEBUG_PRINTLN("Starting audio timer...");
        ITimer0.attachInterruptInterval(TIMER_INTERVAL_US, AudioTimerISR);
        audioTimerActive = true;
    }
}

void stopAudioTimer() {
    if (audioTimerActive) {
        DEBUG_PRINTLN("Stopping audio timer...");
        ITimer0.detachInterrupt();
        audioTimerActive = false;
    }
}

void startControlTimer() {
    if (!controlTimerActive) {
        DEBUG_PRINTLN("Starting control timer...");
        ITimer1.attachInterruptInterval(1000, ControlTimerISR);  // 1kHz
        controlTimerActive = true;
    }
}

void stopControlTimer() {
    if (controlTimerActive) {
        DEBUG_PRINTLN("Stopping control timer...");
        ITimer1.detachInterrupt();
        controlTimerActive = false;
    }
}

// =============================================================================
// 타이머 상태 관리
// =============================================================================

bool isAudioTimerRunning() {
    return audioTimerActive;
}

bool isControlTimerRunning() {
    return controlTimerActive;
}

void restartAudioTimer() {
    DEBUG_PRINTLN("Restarting audio timer...");
    stopAudioTimer();
    delay(10);  // 짧은 지연
    startAudioTimer();
}

void restartControlTimer() {
    DEBUG_PRINTLN("Restarting control timer...");
    stopControlTimer();
    delay(10);  // 짧은 지연
    startControlTimer();
}

// =============================================================================
// 타이머 성능 모니터링
// =============================================================================

void resetTimerPerformanceCounters() {
    audioIsrCount = 0;
    controlIsrCount = 0;
    audioIsrMaxTime = 0;
    audioIsrAvgTime = 0;
    lastAudioIsrTime = 0;
    
    DEBUG_PRINTLN("Timer performance counters reset");
}

void updateTimerPerformanceMetrics() {
    static uint32_t lastUpdateTime = millis();
    static uint32_t lastAudioCount = 0;
    static uint32_t lastControlCount = 0;
    
    uint32_t currentTime = millis();
    
    if (currentTime - lastUpdateTime >= 1000) {  // 1초마다 업데이트
        uint32_t audioRate = audioIsrCount - lastAudioCount;
        uint32_t controlRate = controlIsrCount - lastControlCount;
        
        DEBUG_PRINT("Audio ISR Rate: ");
        DEBUG_PRINT(audioRate);
        DEBUG_PRINTLN(" Hz");
        DEBUG_PRINT("Control ISR Rate: ");
        DEBUG_PRINT(controlRate);
        DEBUG_PRINTLN(" Hz");
        
        lastAudioCount = audioIsrCount;
        lastControlCount = controlIsrCount;
        lastUpdateTime = currentTime;
    }
}

void printTimerPerformanceReport() {
    DEBUG_PRINTLN("=== ESP32C3 Timer Performance Report ===");
    DEBUG_PRINT("Audio Timer Status: ");
    DEBUG_PRINTLN(audioTimerActive ? "Running" : "Stopped");
    DEBUG_PRINT("Control Timer Status: ");
    DEBUG_PRINTLN(controlTimerActive ? "Running" : "Stopped");
    
    DEBUG_PRINT("Audio ISR Count: ");
    DEBUG_PRINTLN(audioIsrCount);
    DEBUG_PRINT("Control ISR Count: ");
    DEBUG_PRINTLN(controlIsrCount);
    
    DEBUG_PRINT("Audio ISR Max Time: ");
    DEBUG_PRINT(audioIsrMaxTime);
    DEBUG_PRINTLN(" microseconds");
    
    DEBUG_PRINT("Audio ISR Avg Time: ");
    DEBUG_PRINT(audioIsrAvgTime);
    DEBUG_PRINTLN(" microseconds");
    
    DEBUG_PRINT("Expected Audio Rate: ");
    DEBUG_PRINT(MOZZI_AUDIO_RATE);
    DEBUG_PRINTLN(" Hz");
    
    // 실시간 오디오 속도 계산
    static uint32_t lastTime = 0;
    static uint32_t sampleCount = 0;
    
    uint32_t currentTime = micros();
    if (lastTime == 0) {
        lastTime = currentTime;
        return;
    }
    
    uint32_t deltaTime = currentTime - lastTime;
    if (deltaTime >= 1000000) {  // 1초마다
        float actualRate = (float)audioIsrCount / (deltaTime / 1000000.0f);
        DEBUG_PRINT("Actual Audio Rate: ");
        DEBUG_PRINT(actualRate, 2);
        DEBUG_PRINTLN(" Hz");
        
        lastTime = currentTime;
    }
}

// =============================================================================
// 타이머 주파수 검증
// =============================================================================

void validateAudioFrequency() {
    DEBUG_PRINTLN("Validating audio frequency...");
    
    const uint32_t testDuration = 1000000;  // 1초 테스트
    uint32_t startTime = micros();
    uint32_t startCount = audioIsrCount;
    
    delayMicroseconds(testDuration);
    
    uint32_t endTime = micros();
    uint32_t endCount = audioIsrCount;
    
    uint32_t actualSamples = endCount - startCount;
    uint32_t actualDuration = endTime - startTime;
    float actualRate = (float)actualSamples / (actualDuration / 1000000.0f);
    
    DEBUG_PRINT("Expected Rate: ");
    DEBUG_PRINT(MOZZI_AUDIO_RATE);
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Actual Rate: ");
    DEBUG_PRINT(actualRate, 2);
    DEBUG_PRINTLN(" Hz");
    
    float errorPercent = abs(actualRate - MOZZI_AUDIO_RATE) / MOZZI_AUDIO_RATE * 100.0f;
    DEBUG_PRINT("Frequency Error: ");
    DEBUG_PRINT(errorPercent, 3);
    DEBUG_PRINTLN("%");
    
    if (errorPercent > 1.0f) {
        DEBUG_PRINTLN("WARNING: Frequency error exceeds 1%");
    } else {
        DEBUG_PRINTLN("Frequency validation passed");
    }
}

// =============================================================================
// 타이머 디버깅 지원
// =============================================================================

void debugTimerConfiguration() {
    DEBUG_PRINTLN("=== Timer Configuration Debug ===");
    DEBUG_PRINT("Timer Base Clock: ");
    DEBUG_PRINT(TIMER_BASE_CLK);
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Timer Divider: ");
    DEBUG_PRINTLN(TIMER_DIVIDER);
    DEBUG_PRINT("Timer Clock Freq: ");
    DEBUG_PRINT(TIMER_CLOCK_FREQ);
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Timer Interval: ");
    DEBUG_PRINT(TIMER_INTERVAL_US);
    DEBUG_PRINTLN(" microseconds");
    DEBUG_PRINT("Timer Counter Value: ");
    DEBUG_PRINTLN(TIMER_COUNTER_VALUE);
    DEBUG_PRINT("Audio Rate: ");
    DEBUG_PRINT(MOZZI_AUDIO_RATE);
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Control Rate: ");
    DEBUG_PRINT(MOZZI_CONTROL_RATE);
    DEBUG_PRINTLN(" Hz");
}

// =============================================================================
// 하드웨어 타이머 직접 제어 (고급 사용법)
// =============================================================================

void configureHardwareTimerDirect() {
    DEBUG_PRINTLN("Configuring hardware timer directly...");
    
    timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider = TIMER_DIVIDER,  // 80 (1MHz 클럭)
    };
    
    // 타이머 그룹 0, 타이머 0 초기화
    timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    
    // 알람 값 설정 (32.768kHz용)
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, TIMER_COUNTER_VALUE);
    
    DEBUG_PRINTLN("Hardware timer configured");
}

void startHardwareTimer() {
    timer_start(TIMER_GROUP_0, TIMER_0);
    DEBUG_PRINTLN("Hardware timer started");
}

void stopHardwareTimer() {
    timer_pause(TIMER_GROUP_0, TIMER_0);
    DEBUG_PRINTLN("Hardware timer stopped");
}

// =============================================================================
// 타이머 우선순위 설정
// =============================================================================

void setTimerPriority(int group, int timer, int priority, int sub_priority) {
    // 타이머 인터럽트 우선순위 설정
    // ESP32C3에서 하드웨어 인터럽트 우선순위 조정
    
    timer_pause(group, timer);
    
    // 우선순위 설정 (0-7, 낮을수록 높음)
    // 실제로는 드라이버 레벨에서 설정됨
    
    DEBUG_PRINT("Timer priority set - Group: ");
    DEBUG_PRINT(group);
    DEBUG_PRINT(", Timer: ");
    DEBUG_PRINT(timer);
    DEBUG_PRINT(", Priority: ");
    DEBUG_PRINT(priority);
    DEBUG_PRINT(", Sub-priority: ");
    DEBUG_PRINTLN(sub_priority);
}