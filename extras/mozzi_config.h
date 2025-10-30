/*
 * Mozzi Library ESP32C3 최적화 설정 파일
 * 
 * ESP32C3 (RISC-V)와 Mozzi Library의 호환성을 위한 최적화된 설정
 * 32.768kHz AudioRate, 외부 출력 모드, ESP32_C3_TimerInterrupt 활용
 * 
 * 작성일: 2025-10-30
 * 호환성: ESP32C3 + Mozzi Library
 */

#ifndef MOZZI_CONFIG_H
#define MOZZI_CONFIG_H

// =============================================================================
// ESP32C3와 Mozzi Library 호환성을 위한 기본 설정
// =============================================================================

// AudioRate 설정 (64kHz - 고성능 TR-808)
#define MOZZI_AUDIO_RATE 64000

// Control Rate 설정 (64kHz에 최적화)
#define MOZZI_CONTROL_RATE 512

// 출력 모드: 외부 타이밍 제어 모드
#define MOZZI_OUTPUT_EXTERNAL_TIMED

// =============================================================================
// ESP32C3 전용 타이머 인터럽트 설정
// =============================================================================

// ESP32C3 TimerInterrupt 라이브러리 사용
#define ESP32C3_TIMER_INTERRUPT

// 타이머 기본 설정
#define TIMER_BASE_CLK 80000000UL      // 80MHz 기준 클럭
#define TIMER_DIVIDER 80               // 1MHz 타이머 클럭 (분주 계수)
#define TIMER_CLOCK_FREQ 1000000UL     // 최종 타이머 클럭

// 타이머 인터럽트 우선순위 (높을수록 중요)
#define AUDIO_TIMER_PRIORITY 5
#define AUDIO_TIMER_SUB_PRIORITY 0

// 인터럽트 간격 계산 (마이크로초 단위)
#define TIMER_INTERVAL_US (1000000UL / MOZZI_AUDIO_RATE)  // 15.6μs @ 64kHz

// =============================================================================
// 버퍼 관리 최적화 설정
// =============================================================================

// 출력 버퍼 크기 (128 샘플 = 2.0ms @ 64kHz)
#define MOZZI_OUTPUT_BUFFER_SIZE 128

// 원형 버퍼 크기 (RAM 절약)
#define MOZZI_CIRCULAR_BUFFER_SIZE 64

// 더블 버퍼링 사용 (성능 최적화)
#define MOZZI_DOUBLE_BUFFERING

// =============================================================================
// ESP32C3 최적화 플래그
// =============================================================================

// RISC-V 아키텍처 최적화
#define ESP32C3_RISCV_OPTIMIZATION

// ISR 최적화 (인터럽트 서비스 루틴 최적화)
#define ESP32C3_OPTIMIZED_ISR

// 정적 메모리 할당 우선 (RAM 절약)
#define USE_STATIC_ALLOCATION

// 동적 메모리 사용 억제
#define DISABLE_DYNAMIC_MEMORY

// =============================================================================
// 성능 최적화 설정
// =============================================================================

// 메모리 풀 관리 사용
#define USE_MEMORY_POOL_MANAGEMENT

// 오디오 샘플 프리프로세싱 활성화
#define ENABLE_AUDIO_PREPROCESSING

// CPU 클럭 모니터링
#define MONITOR_CPU_CLOCK

// =============================================================================
// 디버그 및 개발 설정
// =============================================================================

// 디버그 모드 (개발 시 활성화)
// #define DEBUG_MOZZI_ESP32C3

#ifdef DEBUG_MOZZI_ESP32C3
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(fmt, ...)
#endif

// =============================================================================
// 오디오 품질 설정
// =============================================================================

// 오디오 비트 심도
#define MOZZI_AUDIO_BITS 16

// 오디오 채널 수 (모노)
#define MOZZI_MONO 1

// 오디오 출력 채널 매핑
#define AUDIO_OUTPUT_CHANNEL GPIO_NUM_18  // GPIO 18 (I2S 또는 PWM)

// PWM 설정 (필요 시 사용)
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8                    // 8-bit PWM
#define PWM_FREQUENCY 8000                  // 8kHz PWM 주파수

// =============================================================================
// 메모리 최적화 설정
// =============================================================================

// IRAM 사용 설정 (인터럽트 성능 향상)
#define USE_IRAM_FOR_ISR

// DTCM 사용 (데이터 메모리 최적화)
#define USE_DTCM_MEMORY

// 메모리 사용량 모니터링
#define MONITOR_MEMORY_USAGE

// =============================================================================
// 실시간 성능 모니터링
// =============================================================================

// 성능 측정 활성화
#define ENABLE_PERFORMANCE_MONITORING

// 지연 시간 측정
#define MEASURE_LATENCY

// CPU 사용률 모니터링
#define MONITOR_CPU_USAGE

// 인터럽트 서비스 시간 측정
#define MEASURE_ISR_TIMING

// =============================================================================
// ESP32C3 하드웨어 특성 설정
// =============================================================================

// ESP32C3 하드웨어 설정
#define ESP32C3_HW_TIMER_COUNT 2           // 사용 가능한 하드웨어 타이머 수
#define ESP32C3_CORE_COUNT 1               // 코어 수 (싱글 코어)

// 전원 관리 설정
#define ENABLE_POWER_MANAGEMENT
#define WIFI_PS_MIN_MODEM WIFI_PS_MIN_MODEM  // WiFi 절전 모드

// =============================================================================
// Mozzi Library 기본 설정 재정의
// =============================================================================

// Mozzi의 기본 AUDIO_RATE 재정의
#ifdef MOZZI_AUDIO_RATE
#undef MOZZI_AUDIO_RATE
#endif
#define MOZZI_AUDIO_RATE 64000

// Mozzi의 기본 CONTROL_RATE 재정의
#ifdef MOZZI_CONTROL_RATE
#undef MOZZI_CONTROL_RATE
#endif
#define MOZZI_CONTROL_RATE 512

// =============================================================================
// 컴파일러 최적화 설정
// =============================================================================

// 최적화 플래그 (플랫폼에서 지원 시)
// -O2 -flto -fdata-sections -ffunction-sections -fno-builtin-printf

// =============================================================================
// 상수 정의 (계산값)
// =============================================================================

// 샘플 간 시간 간격 (마이크로초)
#define SAMPLE_INTERVAL_US TIMER_INTERVAL_US

// 버퍼 지연 시간 (밀리초)
#define BUFFER_LATENCY_MS ((MOZZI_OUTPUT_BUFFER_SIZE * 1000) / MOZZI_AUDIO_RATE)

// 타이머 카운터 값 (32.768kHz를 위한 카운터)
#define TIMER_COUNTER_VALUE (TIMER_CLOCK_FREQ / MOZZI_AUDIO_RATE)

// =============================================================================
// 함수 원형 선언
// =============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// 오디오 출력 함수 (외부 구현 필요)
void audioOutput(int16_t output);

// Mozzi 오디오 훅 (외부 구현 필요)
void audioHook();

// ISR 핸들러 (외부 구현 필요)
bool IRAM_ATTR TimerHandler(void *timerNo);

// 성능 모니터링 함수들
void initializePerformanceMonitoring(void);
void updatePerformanceMetrics(void);
void printPerformanceReport(void);

// 오디오 버퍼 관리
void initializeAudioBuffers(void);
bool bufferAudioSample(int16_t sample);
bool getAudioSample(int16_t *sample);
bool isBufferFull(void);
bool isBufferEmpty(void);

#ifdef __cplusplus
}
#endif

// =============================================================================
// 매크로 함수들
// =============================================================================

// 오디오 샘플 변환 매크로
#define CONVERT_TO_PWM_VALUE(output) \
    ((uint8_t)(((output) + 32768) >> 8))

// 오디오 샘플 검증 매크로
#define VALIDATE_AUDIO_SAMPLE(sample) \
    ((sample) > 32767 ? 32767 : ((sample) < -32768 ? -32768 : (sample)))

// ISR 안전 시간 측정 매크로
#define ISR_START_TIME() ({ static uint32_t _start_time = 0; _start_time = micros(); })
#define ISR_END_TIME() ({ static uint32_t _end_time = 0; _end_time = micros(); _end_time - _start_time; })

// =============================================================================
// 설정 검증 및 자동 설정
// =============================================================================

// 설정값 검증
#if MOZZI_AUDIO_RATE > 65536
#error "Audio rate too high for ESP32C3. Maximum recommended is 32768Hz"
#endif

#if MOZZI_OUTPUT_BUFFER_SIZE < 64
#error "Output buffer size too small. Minimum recommended is 64 samples"
#endif

#if MOZZI_OUTPUT_BUFFER_SIZE > 1024
#error "Output buffer size too large. Maximum recommended is 1024 samples"
#endif

// 자동 설정 (플랫폼별)
#if defined(ARDUINO_ARCH_ESP32C3)
    #define PLATFORM_ESP32C3
    #define SUPPORT_TIMER_INTERRUPT
    #define SUPPORT_PWM_OUTPUT
    // #define SUPPORT_I2S_OUTPUT  // I2S 사용 시 활성화
#endif

// =============================================================================
// 호환성 매크로
// =============================================================================

// Arduino 호환성
#ifdef ARDUINO
    #include <Arduino.h>
#endif

// ESP-IDF 호환성
#ifdef ESP_IDF_VERSION_VAL
    #include "esp_idf_version.h"
#endif

// =============================================================================
// 버전 정보
// =============================================================================

#define MOZZI_CONFIG_VERSION "1.0.0"
#define MOZZI_CONFIG_DATE "2025-10-30"
#define MOZZI_CONFIG_COMPATIBLE_ESP32C3

#endif /* MOZZI_CONFIG_H */