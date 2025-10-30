/*
 * Mozzi Library 통합 계획 헤더 파일
 * 
 * ESP32C3 TR-808 프로젝트와 Mozzi Library의 통합을 위한
 * 단계별 마이그레이션 전략과 인터페이스를 정의합니다.
 * 
 * 작성일: 2025-10-30
 * 버전: 1.0.0
 * 호환성: ESP32C3 + Mozzi Library
 */

#ifndef MOZZI_INTEGRATION_PLAN_H
#define MOZZI_INTEGRATION_PLAN_H

#include <Arduino.h>
#include "../../extras/mozzi_config.h"

// =============================================================================
// 통합 모드 설정
// =============================================================================

/**
 * Mozzi 통합 모드 선택
 * MOZZI_DISABLED: 기존 ESP32 I2S 방식 유지 (기본값)
 * MOZZI_HYBRID: Mozzi + ESP32 I2S 하이브리드 모드
 * MOZZI_FULL: 순수 Mozzi 아키텍처
 */
#ifndef MOZZI_INTEGRATION_MODE
    #define MOZZI_INTEGRATION_MODE MOZZI_DISABLED
#endif

// 통합 모드 상수
#define MOZZI_DISABLED    0
#define MOZZI_HYBRID      1
#define MOZZI_FULL        2

// =============================================================================
// ESP32C3 특화 설정
// =============================================================================

#ifdef ARDUINO_ARCH_ESP32C3
    #define IS_ESP32C3_PLATFORM true
    #define ESP32C3_TIMER_INTERRUPT_ENABLED
    
    // ESP32C3 Quirks 처리
    #ifdef ESP32C3_QUIRKS_HANDLING
        #define ESP32C3_IRAM_ATTR __attribute__((section(".iram1")))
        #define ESP32C3_INLINE inline __attribute__((always_inline))
    #else
        #define ESP32C3_IRAM_ATTR
        #define ESP32C3_INLINE inline
    #endif
#else
    #define IS_ESP32C3_PLATFORM false
    #define ESP32C3_IRAM_ATTR
    #define ESP32C3_INLINE inline
#endif

// =============================================================================
// 성능 최적화 설정
// =============================================================================

// 샘플레이트 설정 (Mozzi 권장값)
#ifndef MOZZI_AUDIO_RATE
    #define MOZZI_AUDIO_RATE 32768
#endif

// 컨트롤 레이트 설정
#ifndef MOZZI_CONTROL_RATE
    #define MOZZI_CONTROL_RATE 256
#endif

// 오디오 버퍼 설정
#ifndef MOZZI_OUTPUT_BUFFER_SIZE
    #define MOZZI_OUTPUT_BUFFER_SIZE 256
#endif

// 더블 버퍼링 사용 여부
#ifndef MOZZI_DOUBLE_BUFFERING
    #define MOZZI_DOUBLE_BUFFERING true
#endif

// 메모리 풀 관리
#ifndef USE_MEMORY_POOL_MANAGEMENT
    #define USE_MEMORY_POOL_MANAGEMENT true
#endif

// =============================================================================
// Mozzi TR-808 통합 인터페이스
// =============================================================================

#if MOZZI_INTEGRATION_MODE != MOZZI_DISABLED
    // Mozzi가 활성화된 경우에만 포함
    #include <MozziGuts.h>
    #include <Oscil.h>
    #include <ADSR.h>
    #include <tables/sine256.h>
    #include <tables/triangle256.h>
    #include <tables/saw256.h>
    #include <tables/square_no_alias_256.h>
    
    // Fast math 활용 (선택사항)
    #ifdef USE_FASTMATH
        #include <fixMath.h>
    #endif
#endif

// =============================================================================
// 브리지드 T 발진기 - Mozzi 최적화 버전
// =============================================================================

#if MOZZI_INTEGRATION_MODE != MOZZI_DISABLED
/**
 * Mozzi 기반 브리지드 T 발진기
 * 기존 TR808BridgedTOscillator의 Mozzi 최적화 버전
 */
class MozziBridgedTOscillator {
private:
    float resonantFreq;
    float damping;
    float phase;
    float amplitude;
    float decayRate;
    
public:
    MozziBridgedTOscillator();
    void setFrequency(float freq);
    void setDecay(float decayMs);
    void trigger(float velocity = 1.0f);
    float generate();
    void reset();
};

/**
 * 하이브리드 브리지드 T 발진기
 * 기존 방식과 Mozzi 방식을 비교 테스트용
 */
class HybridBridgedTOscillator {
private:
    bool useMozziMode;
    MozziBridgedTOscillator mozziOsc;
    
    // 기존 방식 데이터
    float legacyFrequency;
    float legacyPhase;
    float legacyAmplitude;
    
public:
    HybridBridgedTOscillator();
    void setMozziMode(bool enable);
    void setFrequency(float freq);
    void setDecay(float decayMs);
    void trigger(float velocity = 1.0f);
    float generateMozzi();
    float generateLegacy();
    float generate(); // 현재 모드에 따른 자동 선택
};
#endif

// =============================================================================
// 엔벨롭 시스템 - Mozzi ADSR 활용
// =============================================================================

#if MOZZI_INTEGRATION_MODE != MOZZI_DISABLED
/**
 * Mozzi ADSR 기반 엔벨롭
 * 성능 최적화와 일관된 타임베이스 제공
 */
class MozziEnvelope {
private:
    ADSR<CONTROL_RATE> envelope;
    bool isActive;
    
public:
    MozziEnvelope();
    void setAttack(float timeMs);
    void setDecay(float timeMs);
    void setRelease(float timeMs);
    void setSustain(float level);
    void trigger();
    void release();
    float getValue();
    bool isNoteActive();
    void updateControl(); // Mozzi updateControl()에서 호출
};

/**
 * 하이브리드 엔벨롭
 * 기존 micros() 기반과 Mozzi ADSR 기반 비교
 */
class HybridEnvelope {
private:
    bool useMozziMode;
    MozziEnvelope mozziEnv;
    
    // 기존 방식 데이터
    float attackTime;
    float decayTime;
    float releaseTime;
    float sustainLevel;
    float currentLevel;
    uint32_t startTime;
    
public:
    HybridEnvelope();
    void setMozziMode(bool enable);
    void setAttack(float timeMs);
    void setDecay(float timeMs);
    void setRelease(float timeMs);
    void setSustain(float level);
    void trigger();
    float getValueMozzi();
    float getValueLegacy();
    float getValue(); // 현재 모드에 따른 자동 선택
    void updateControl(); // Mozzi 업데이트용
};
#endif

// =============================================================================
// 필터 시스템 - Mozzi Filter 클래스 활용
// =============================================================================

#if MOZZI_INTEGRATION_MODE != MOZZI_DISABLED
/**
 * Mozzi 기반 필터 래퍼
 * FilterHP, FilterLP, FilterBP 래핑
 */
class MozziFilterWrapper {
private:
    float cutoffFreq;
    float resonance;
    bool isHighPass;
    bool isBandPass;
    
    // 실제 필터 인스턴스 (템플릿이므로 포인터로 처리)
    void* filterPtr;
    
public:
    MozziFilterWrapper(bool hp = false, bool bp = false);
    void setCutoff(float freq);
    void setResonance(float q);
    float process(float input);
    void reset();
    ~MozziFilterWrapper();
};
#endif

// =============================================================================
// 메모리 풀 관리자
// =============================================================================

/**
 * 드럼 음성 풀 매니저
 * 제한된 자원으로 최대 성능 제공
 */
class DrumVoicePool {
private:
    static constexpr int MAX_VOICES = 8;
    void* voices[MAX_VOICES];
    bool voiceActive[MAX_VOICES];
    int voiceType[MAX_VOICES]; // 0=none, 1=kick, 2=snare, etc.
    uint32_t voiceStartTime[MAX_VOICES];
    
public:
    DrumVoicePool();
    void* allocateVoice(int drumType);
    void freeVoice(void* voice);
    int getActiveVoiceCount();
    void updatePool();
    void reset();
};

/**
 * 버퍼 풀 매니저
 * 오디오 버퍼 재사용으로 메모리 절약
 */
class AudioBufferPool {
private:
    static constexpr int MAX_BUFFERS = 4;
    int16_t* buffers[MAX_BUFFERS];
    bool bufferUsed[MAX_BUFFERS];
    
public:
    AudioBufferPool();
    int16_t* allocateBuffer(int size);
    void freeBuffer(int16_t* buffer);
    void reset();
    ~AudioBufferPool();
};

// =============================================================================
// 성능 모니터링 클래스
// =============================================================================

/**
 * Mozzi 전용 성능 메트릭
 */
struct MozziPerformanceMetrics {
    float audioCpuUsage;      // 오디오 처리 CPU 사용률
    float controlCpuUsage;    // 컨트롤 처리 CPU 사용률
    uint32_t bufferUnderruns; // 버퍼 언더런 발생 횟수
    uint32_t activeVoices;    // 활성 음성 수
    uint32_t maxVoices;       // 최대 동시 음성 수
    float envelopeLatency;    // 엔벨롭 지연 시간 (μs)
    float filterLatency;      // 필터 지연 시간 (μs)
    uint32_t memoryUsage;     // 메모리 사용량 (바이트)
};

/**
 * 성능 모니터링 관리자
 */
class PerformanceMonitor {
private:
    MozziPerformanceMetrics metrics;
    uint32_t lastUpdateTime;
    uint32_t sampleCount;
    uint32_t controlCount;
    
public:
    PerformanceMonitor();
    void startAudioSample();
    void endAudioSample();
    void startControlUpdate();
    void endControlUpdate();
    void updateBufferUnderrun();
    void updateVoiceCount(uint32_t count);
    void calculateMetrics();
    MozziPerformanceMetrics getMetrics();
    void printReport();
    void reset();
};

// =============================================================================
// 통합 관리자 클래스
// =============================================================================

/**
 * Mozzi-TR808 통합 관리자
 * 전체 시스템의 통합을 책임지는 메인 클래스
 */
class MozziTR808Manager {
private:
    int integrationMode;
    bool isInitialized;
    PerformanceMonitor perfMonitor;
    DrumVoicePool voicePool;
    AudioBufferPool bufferPool;
    
    // 모드별 처리 함수
    float processLegacyMode();
    float processHybridMode();
    float processMozziMode();
    
public:
    MozziTR808Manager();
    void setIntegrationMode(int mode);
    int getIntegrationMode();
    bool initialize();
    void updateControl();  // Mozzi에서 호출
    int16_t updateAudio(); // Mozzi에서 호출
    void shutdown();
    PerformanceMonitor* getPerformanceMonitor();
    void runDiagnostics();
    void reset();
};

// =============================================================================
// 유틸리티 함수들
// =============================================================================

/**
 * ESP32C3 특화 초기화
 */
ESP32C3_INLINE void initializeESP32C3Optimizations() {
    #ifdef IS_ESP32C3_PLATFORM
        // CPU 클럭 확인
        Serial.println("CPU 클럭: " + String(ESP.getCpuFreqMHz()) + " MHz");
        
        // 메모리 정보 출력
        Serial.println("플래시 크기: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
        Serial.println("RAM 크기: " + String(ESP.getHeapSize() / 1024) + " KB");
        
        // 성능 모드 설정
        #ifdef CONFIG_ESP32C3_DEFAULT_CPU_FREQ_160
            setCpuFrequencyMhz(160);
            Serial.println("CPU 주파수 160MHz로 설정됨");
        #endif
    #endif
}

/**
 * 모드 비교 테스트
 */
#if MOZZI_INTEGRATION_MODE == MOZZI_HYBRID
    void runModeComparisonTest();
#endif

/**
 * 성능 벤치마크
 */
void runPerformanceBenchmark();

/**
 * 메모리 사용량 분석
 */
void analyzeMemoryUsage();

/**
 * 디버그 정보 출력
 */
void printDebugInfo();

// =============================================================================
// 매크로 함수들
// =============================================================================

// 오디오 샘플 변환
#define CONVERT_TO_MOZZI_SAMPLE(float_sample) \
    ((int16_t)(constrain(float_sample, -1.0f, 1.0f) * 32767))

#define CONVERT_FROM_MOZZI_SAMPLE(int_sample) \
    (((float)(int_sample)) / 32767.0f)

// ISR 안전 시간 측정
#define ISR_BEGIN_PROFILE() ({ static uint32_t _start = 0; _start = micros(); })
#define ISR_END_PROFILE() ({ static uint32_t _end = 0; _end = micros(); _end - _start; })

// 성능 측정 데코레이터
#ifdef MOZZI_PERFORMANCE_MONITORING
    #define PROFILE_FUNCTION(name) static uint32_t _prof_##name = 0; \
        uint32_t _prof_start_##name = micros();
    #define PROFILE_END(name) _prof_##name += micros() - _prof_start_##name;
#else
    #define PROFILE_FUNCTION(name)
    #define PROFILE_END(name)
#endif

// =============================================================================
// 컴파일 타임 검증
// =============================================================================

// 설정값 검증
static_assert(MOZZI_AUDIO_RATE >= 8000 && MOZZI_AUDIO_RATE <= 65536, 
               "Audio rate must be between 8kHz and 65.536kHz");

static_assert(MOZZI_CONTROL_RATE >= 32 && MOZZI_CONTROL_RATE <= 1024,
               "Control rate must be between 32Hz and 1024Hz");

static_assert(MOZZI_OUTPUT_BUFFER_SIZE >= 64 && MOZZI_OUTPUT_BUFFER_SIZE <= 1024,
               "Buffer size must be between 64 and 1024 samples");

// 모드 검증
static_assert(MOZZI_INTEGRATION_MODE >= 0 && MOZZI_INTEGRATION_MODE <= 2,
               "Invalid integration mode");

// =============================================================================
// 통합 단계별 기능 플래그
// =============================================================================

// 단계별 활성화 플래그
#define INTEGRATION_PHASE_1_ENVELOPE    1  // 엔벨롭만 Mozzi
#define INTEGRATION_PHASE_2_OSCILLATOR  2  // 발진기도 추가
#define INTEGRATION_PHASE_3_FILTER      3  // 필터도 추가
#define INTEGRATION_PHASE_FULL_MOZZI    4  // 전부 Mozzi

#ifndef INTEGRATION_PHASE
    #define INTEGRATION_PHASE INTEGRATION_PHASE_1_ENVELOPE
#endif

// =============================================================================
// 호환성 매크로
// =============================================================================

// 기존 TR808 함수들과의 호환성
#define TR808_TRIGGER_KICK(velocity) drumMachine.triggerKick(velocity)
#define TR808_TRIGGER_SNARE(velocity) drumMachine.triggerSnare(velocity)
#define TR808_TRIGGER_CYMBAL(velocity) drumMachine.triggerCymbal(velocity)

// =============================================================================
// 헤더 파일 의존성
// =============================================================================

// 조건부 포함
#if MOZZI_INTEGRATION_MODE != MOZZI_DISABLED
    #include <MozziGuts.h>
    #include <Oscil.h>
    #include <ADSR.h>
#endif

#endif /* MOZZI_INTEGRATION_PLAN_H */

// =============================================================================
// End of Integration Plan Header
// =============================================================================