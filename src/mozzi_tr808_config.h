/*
 * Mozzi TR-808 드럼 머신 전용 설정 파일
 * 
 * ESP32C3 + Mozzi Library + TR-808 드럼 머신 완전 통합 설정
 * AudioOutput_AUDIOINTERFACE와 ESP32_C3_TimerInterrupt 활용
 * 
 * 작성일: 2025-10-30
 * 버전: 1.0.0
 */

#ifndef MOZZI_TR808_CONFIG_H
#define MOZZI_TR808_CONFIG_H

#include "mozzi_config.h"
#include "arduino_tr808_config.h"
#include <Arduino.h>

// =============================================================================
// Mozzi Library 기본 설정 (AudioOutput_AUDIOINTERFACE)
// =============================================================================

// AudioRate: 32.768kHz (ESP32C3 권장, 최고 품질)
#define MOZZI_AUDIO_RATE 32768

// Control Rate: 128 (실시간 드럼 트리거에 최적화)
#define MOZZI_CONTROL_RATE 128

// 출력 모드: AudioOutput_AUDIOINTERFACE 사용
#define USE_AUDIO_OUTPUT_AUDIOINTERFACE

// 외부 타이밍 제어 모드 활성화
#define MOZZI_OUTPUT_EXTERNAL_TIMED

// =============================================================================
// ESP32_C3_TimerInterrupt 라이브러리 통합 설정
// =============================================================================

// TimerInterrupt 라이브러리 설정
#define TIMER_NUMBER 0               // Timer 0 사용
#define TIMER_INTERRUPT_CH 0         // 채널 0
#define TIMER_FREQUENCY 32768        // 32.768kHz
#define TIMER_PERIOD 30              // 30.5μs 주기
#define TIMER_RESOLUTION 1000000     // 마이크로초 단위

// 인터럽트 우선순위 (최고 우선순위)
#define MOZZI_TIMER_PRIORITY 0
#define MOZZI_TIMER_SUB_PRIORITY 0

// ISR 최적화 설정
#define TIMER_ISR_IRAM_ATTR IRAM_ATTR
#define TIMER_GROUP 0

// =============================================================================
// TR-808 드럼 머신 설정
// =============================================================================

// 지원되는 드럼 소스 수 (18개)
#define TR808_NUM_SOURCES 18

// 드럼 소스 정의
enum TR808DrumSource {
    TR808_KICK = 0,           // 베이스 드럼
    TR808_SNARE = 1,          // 스네어 드럼
    TR808_CYMBAL = 2,         // 심벌
    TR808_HIHAT_CLOSED = 3,   // 클로즈드 하이햇
    TR808_HIHAT_OPEN = 4,     // 오픈 하이햇
    TR808_TOM_LOW = 5,        // 로우 톰
    TR808_TOM_MID = 6,        // 미들 톰
    TR808_TOM_HIGH = 7,       // 하이 톰
    TR808_CONGA_LOW = 8,      // 로우 콩가
    TR808_CONGA_MID = 9,      // 미들 콩가
    TR808_CONGA_HIGH = 10,    // 하이 콩가
    TR808_RIMSHOT = 11,       // 림샷
    TR808_MARACAS = 12,       // 마라카스
    TR808_CLAW = 13,          // 클랩
    TR808_COWBELL = 14,       // 카우벨
    TR808_ClAP = 15,          // 추가 클랩
    TR808_SHAKER = 16,        // 셰이커
    TR808_CRASH = 17          // 크래시 심벌
};

// 마스터 볼륨 설정
#define TR808_DEFAULT_MASTER_VOLUME 0.7f
#define TR808_MIN_VOLUME 0.0f
#define TR808_MAX_VOLUME 1.0f

// =============================================================================
// 성능 모니터링 설정
// =============================================================================

// CPU 사용률 모니터링
#define ENABLE_CPU_MONITORING
#define CPU_MONITOR_INTERVAL_MS 1000

// 지연 시간 모니터링
#define ENABLE_LATENCY_MONITORING
#define LATENCY_SAMPLES 256

// 폴리포니 모니터링
#define ENABLE_POLYPHONY_MONITORING
#define MAX_POLYPHONY 18

// 메모리 사용량 모니터링
#define ENABLE_MEMORY_MONITORING

// =============================================================================
// 오디오 버퍼 설정
// =============================================================================

// 출력 버퍼 크기 (256 샘플 = 7.8ms @ 32.768kHz)
#define MOZZI_OUTPUT_BUFFER_SIZE 256

// 더블 버퍼링 사용
#define MOZZI_DOUBLE_BUFFERING

// 원형 버퍼 크기
#define MOZZI_CIRCULAR_BUFFER_SIZE 128

// =============================================================================
// 실시간 패턴 설정
// =============================================================================

// 패턴 버퍼 크기
#define PATTERN_BUFFER_SIZE 64

// 패턴 수
#define NUM_PATTERNS 8

// 템포 설정 (BPM)
#define DEFAULT_TEMPO 120
#define MIN_TEMPO 60
#define MAX_TEMPO 180

// =============================================================================
// Serial 명령 인터페이스 설정
// =============================================================================

// Serial baud rate
#define SERIAL_BAUD_RATE 115200

// 명령어 버퍼 크기
#define COMMAND_BUFFER_SIZE 64

// 최대 명령어 길이
#define MAX_COMMAND_LENGTH 32

// =============================================================================
// ESP32C3 하드웨어 최적화 설정
// =============================================================================

// ESP32C3 전용 최적화
#define USE_ESP32C3_OPTIMIZATIONS

// RISC-V 최적화
#define ESP32C3_RISCV_OPTIMIZATION

// IRAM 사용 (ISR 성능 향상)
#define USE_IRAM_FOR_ISR

// DTCM 메모리 사용 (데이터 캐시 최적화)
#define USE_DTCM_MEMORY

// =============================================================================
// 디버그 및 개발 설정
// =============================================================================

// 디버그 모드
// #define DEBUG_MOZZI_TR808

#ifdef DEBUG_MOZZI_TR808
    #define TR808_DEBUG_PRINT(x) Serial.print(x)
    #define TR808_DEBUG_PRINTLN(x) Serial.println(x)
    #define TR808_DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
    #define TR808_DEBUG_PRINTLN_HEX(x) Serial.println(x, HEX)
    #define TR808_DEBUG_PRINTLN_BIN(x) Serial.println(x, BIN)
#else
    #define TR808_DEBUG_PRINT(x)
    #define TR808_DEBUG_PRINTLN(x)
    #define TR808_DEBUG_PRINTF(fmt, ...)
    #define TR808_DEBUG_PRINTLN_HEX(x)
    #define TR808_DEBUG_PRINTLN_BIN(x)
#endif

// =============================================================================
// 오디오 출력 설정
// =============================================================================

// AudioOutput_AUDIOINTERFACE 설정
#define AUDIO_OUTPUT_PIN GPIO_NUM_18      // GPIO 18
#define AUDIO_OUTPUT_RESOLUTION 16        // 16-bit 오디오
#define AUDIO_OUTPUT_CHANNELS 1           // 모노 출력

// PWM 출력 설정 (대안)
#define USE_PWM_OUTPUT
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8                  // 8-bit PWM
#define PWM_FREQUENCY 8000                // 8kHz PWM 주파수

// I2S 출력 설정 (선택사항)
#define USE_I2S_OUTPUT
#define I2S_BCLK_PIN GPIO_NUM_2           // GPIO 2
#define I2S_WS_PIN GPIO_NUM_3             // GPIO 3
#define I2S_DATA_PIN GPIO_NUM_1           // GPIO 1

// =============================================================================
// 드럼별 기본 파라미터 설정
// =============================================================================

// Kick 드럼
#define KICK_FREQUENCY 60.0f
#define KICK_ATTACK_TIME 2.0f
#define KICK_DECAY_TIME 150.0f
#define KICK_PUNCH_GAIN 0.8f

// Snare 드럼
#define SNARE_FREQUENCY 200.0f
#define SNARE_NOISE_LEVEL 0.6f
#define SNARE_TONE_LEVEL 0.4f
#define SNARE_DECAY_TIME 100.0f

// Hi-Hat
#define HIHAT_FREQUENCY 8000.0f
#define HIHAT_ATTACK_TIME 0.1f
#define HIHAT_DECAY_TIME_CLOSED 50.0f
#define HIHAT_DECAY_TIME_OPEN 200.0f

// Tom
#define TOM_LOW_FREQUENCY 85.0f
#define TOM_MID_FREQUENCY 120.0f
#define TOM_HIGH_FREQUENCY 170.0f
#define TOM_DECAY_TIME 250.0f

// =============================================================================
// 패턴 데이터 구조체
// =============================================================================

// 패턴 스텝 구조
struct PatternStep {
    uint8_t velocity;      // 0-127 (벨로시티)
    uint8_t instrument;    // 드럼 소스 인덱스
    bool enabled;          // 스텝 활성화 여부
};

// 패턴 구조
struct TR808Pattern {
    char name[16];         // 패턴 이름
    uint8_t length;        // 패턴 길이 (스텝 수)
    uint8_t tempo;         // 템포 (BPM)
    PatternStep steps[PATTERN_BUFFER_SIZE]; // 패턴 스텝들
};

// =============================================================================
// 성능 메트릭 구조체
// =============================================================================

struct TR808PerformanceMetrics {
    uint32_t cpuUsage;         // CPU 사용률 (백분율)
    uint32_t latencyUs;        // 지연 시간 (마이크로초)
    uint32_t polyphony;        // 현재 폴리포니
    uint32_t maxPolyphony;     // 최대 폴리포니
    uint32_t sampleCount;      // 처리된 샘플 수
    uint32_t dropCount;        // 드롭된 샘플 수
    uint32_t memoryUsage;      // 메모리 사용량 (바이트)
    uint32_t memoryPeak;       // 최대 메모리 사용량
    uint32_t bufferUnderruns;  // 버퍼 언더런 발생 횟수
    uint32_t bufferOverruns;   // 버퍼 오버런 발생 횟수
};

// =============================================================================
// 시스템 상태 구조체
// =============================================================================

struct TR808SystemStatus {
    bool initialized;          // 초기화 완료 여부
    bool audioActive;          // 오디오 활성 상태
    bool patternPlaying;       // 패턴 재생 중
    bool performanceMonitoring; // 성능 모니터링 활성
    uint32_t uptimeMs;         // 시스템 작동 시간 (ms)
    float masterVolume;        // 마스터 볼륨
    uint8_t currentPattern;    // 현재 재생 중인 패턴
    uint32_t currentStep;      // 현재 스텝
};

// =============================================================================
// 함수 원형 선언
// =============================================================================

// TR-808 드럼 머신 메인 클래스
class TR808DrumMachineMozzi {
private:
    bool initialized;
    bool audioActive;
    float masterVolume;
    
    // 성능 모니터링
    TR808PerformanceMetrics performance;
    TR808SystemStatus systemStatus;
    
    // 패턴 데이터
    TR808Pattern patterns[NUM_PATTERNS];
    uint8_t currentPatternIndex;
    bool patternPlaying;
    uint32_t patternStep;
    uint32_t patternTempo;
    
    // 드럼 소스
    void* drumSources[TR808_NUM_SOURCES];
    
    // 내부 함수들
    bool initializeDrumSources();
    void updatePerformanceMetrics();
    void processDrumSource(uint8_t source, float velocity);
    void loadDefaultPatterns();
    
public:
    TR808DrumMachineMozzi();
    
    // 초기화 및 설정
    bool initialize();
    bool initializeAudio();
    bool initializePerformanceMonitoring();
    
    // 드럼 제어
    void triggerDrum(uint8_t drumType, float velocity = 1.0f);
    void triggerDrum(const String& drumName, float velocity = 1.0f);
    void setMasterVolume(float volume);
    
    // 패턴 제어
    bool loadPattern(uint8_t patternIndex);
    bool startPattern(uint8_t patternIndex);
    void stopPattern();
    void pausePattern();
    void resumePattern();
    
    // 상태 확인
    bool isInitialized() const { return initialized; }
    bool isAudioActive() const { return audioActive; }
    bool isPatternPlaying() const { return patternPlaying; }
    float getMasterVolume() const { return masterVolume; }
    
    // 정보 출력
    void printSystemStatus();
    void printPerformanceReport();
    void printPatternList();
    void printDrumList();
    
    // Serial 명령 처리
    bool processSerialCommand(const String& command);
    
    // Mozzi 통합 함수
    void updateControl();
    void updateAudio();
};

// =============================================================================
// 전역 인스턴스
// =============================================================================

extern TR808DrumMachineMozzi tr808Mozzi;

// =============================================================================
// 상수 정의
// =============================================================================

// 오디오 샘플 크기
#define AUDIO_SAMPLE_SIZE sizeof(int16_t)

// 더블 버퍼 크기
#define DOUBLE_BUFFER_SIZE (MOZZI_OUTPUT_BUFFER_SIZE / 2)

// 타이머 간격 (마이크로초)
#define AUDIO_TIMER_INTERVAL (1000000UL / MOZZI_AUDIO_RATE)

// =============================================================================
// 매크로 함수들
// =============================================================================

// 드럼 이름 매핑
#define DRUM_NAME_KICK "kick"
#define DRUM_NAME_SNARE "snare"
#define DRUM_NAME_CYMBAL "cymbal"
#define DRUM_NAME_HIHAT "hihat"
#define DRUM_NAME_TOM "tom"
#define DRUM_NAME_CONGA "conga"
#define DRUM_NAME_RIMSHOT "rimshot"
#define DRUM_NAME_MARACAS "maracas"
#define DRUM_NAME_CLAW "clap"
#define DRUM_NAME_COWBELL "cowbell"

// 벨로시티 매크로
#define VELOCITY_SOFT 0.3f
#define VELOCITY_NORMAL 0.7f
#define VELOCITY_HARD 1.0f

// =============================================================================
// 설정 검증
// =============================================================================

// 설정값 검증
#if MOZZI_AUDIO_RATE > 32768
#error "Audio rate too high for ESP32C3. Maximum recommended is 32768Hz"
#endif

#if MOZZI_CONTROL_RATE > 256
#error "Control rate too high. Maximum recommended is 256Hz"
#endif

#if MOZZI_OUTPUT_BUFFER_SIZE < 64
#error "Output buffer size too small. Minimum recommended is 64 samples"
#endif

#if TR808_NUM_SOURCES > 32
#error "Too many drum sources. Maximum recommended is 32"
#endif

// =============================================================================
// 버전 정보
// =============================================================================

#define MOZZI_TR808_VERSION "1.0.0"
#define MOZZI_TR808_DATE "2025-10-30"
#define MOZZI_TR808_COMPATIBLE_ESP32C3

#endif /* MOZZI_TR808_CONFIG_H */