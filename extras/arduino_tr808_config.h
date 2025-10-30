/*
 * Arduino TR-808 드럼 머신 설정 파일
 * 
 * ESP32C3 Arduino 환경에서 TR-808 드럼 머신의
 * 모든 설정과 상수를 중앙 집중식으로 관리
 * 
 * 이 파일을 수정하여 TR-808의 사운드와 성능을 조정할 수 있습니다.
 * 
 * 작성일: 2025-10-30
 * 버전: 1.0.0
 */

#ifndef ARDUINO_TR808_CONFIG_H
#define ARDUINO_TR808_CONFIG_H

#include <Arduino.h>

// ============================================
// 버전 및 메타 정보
// ============================================

#define TR808_VERSION_MAJOR    1
#define TR808_VERSION_MINOR    0
#define TR808_VERSION_PATCH    0
#define TR808_BUILD_DATE      "2025-10-30"
#define TR808_PROJECT_NAME    "ESP32C3 TR-808 Drum Machine"

// ============================================
// 하드웨어 플랫폼 정보
// ============================================

// 보드 및 CPU 정보
#ifdef ARDUINO_ARCH_ESP32
    #define BOARD_NAME "ESP32C3"
    #define IS_ESP32_PLATFORM true
#else
    #define BOARD_NAME "Unknown"
    #define IS_ESP32_PLATFORM false
#endif

// ============================================
// I2S 오디오 설정 (ESP32C3 최적화)
// ============================================

// I2S 핀 매핑 (ESP32C3 기본 설정)
#define I2S_WS_PIN     3   // Word Select (LRCLK) - GPIO 3
#define I2S_BCK_PIN    2   // Bit Clock (BCLK) - GPIO 2  
#define I2S_DATA_PIN   1   // Data Out (DATA) - GPIO 1
#define I2S_MCK_PIN    0   // Master Clock (선택사항) - GPIO 0

// 오디오 품질 설정
#define DEFAULT_SAMPLE_RATE    32768   // 32.768kHz (ESP32C3 권장)
#define MAX_SAMPLE_RATE        32768   // 최대 샘플링 레이트
#define MIN_SAMPLE_RATE        16000   // 최소 샘플링 레이트
#define I2S_BITS_PER_SAMPLE    16      // 16-bit 오디오
#define I2S_CHANNELS          1       // 모노 출력 (메모리 절약)
#define STEREO_OUTPUT         false   // 스테레오 사용 여부

// I2S 버퍼 설정
#define I2S_BUFFER_SIZE       256     // 기본 I2S 버퍼 크기
#define MIN_BUFFER_SIZE       128     // 최소 버퍼 크기
#define MAX_BUFFER_SIZE       1024    // 최대 버퍼 크기
#define BUFFER_UNDERFLOW_THRESHOLD  200  // 버퍼 부족 경계값

// I2S 성능 설정
#define I2S_USE_EXTERNAL_CLOCK    false // 외부 클록 사용 (고급 기능)
#define I2S_MASTER_MODE          true   // 마스터 모드
#define I2S_HIGH_PERFORMANCE     true   //高性能 모드

// ============================================
// TR-808 드럼 설정
// ============================================

// 마스터 설정
#define DEFAULT_MASTER_VOLUME    0.8f  // 0.0 - 1.0 (80%)
#define MIN_VOLUME              0.0f
#define MAX_VOLUME              1.0f
#define VOLUME_STEP             0.1f

// 폴포니 (동시 음향) 제한
#define MAX_POLYPHONY           10     // 최대 동시 드럼 음향
#define MAX_CONCURRENT_DRUM_SOUNDS  MAX_POLYPHONY

// 드럼별 기본 설정
// Kick (베이스 드럼)
#define DEFAULT_KICK_DECAY      500.0f  // ms (300-1200)
#define DEFAULT_KICK_TONE       0.5f    // 0.0-1.0
#define KICK_DECAY_RANGE_MIN    300.0f
#define KICK_DECAY_RANGE_MAX    1200.0f
#define KICK_TONE_RANGE_MIN     0.0f
#define KICK_TONE_RANGE_MAX     1.0f

// Snare (스네어 드럼)
#define DEFAULT_SNARE_TONE      0.7f    // 0.0-1.0
#define DEFAULT_SNARE_SNAPPY    0.8f    // 0.0-1.0
#define DEFAULT_SNARE_DECAY     150.0f  // ms
#define SNARE_TONE_RANGE_MIN    0.0f
#define SNARE_TONE_RANGE_MAX    1.0f
#define SNARE_SNAPPY_RANGE_MIN  0.0f
#define SNARE_SNAPPY_RANGE_MAX  1.0f

// Cymbal (심벌)
#define DEFAULT_CYMBAL_DECAY    800.0f  // ms (500-2000)
#define DEFAULT_CYMBAL_TONE     0.6f    // 0.0-1.0
#define CYMBAL_DECAY_RANGE_MIN  500.0f
#define CYMBAL_DECAY_RANGE_MAX  2000.0f
#define CYMBAL_TONE_RANGE_MIN   0.0f
#define CYMBAL_TONE_RANGE_MAX   1.0f

// Hi-Hat (하이햇)
#define DEFAULT_HIHAT_DECAY_CLOSED   50.0f   // ms
#define DEFAULT_HIHAT_DECAY_OPEN     200.0f  // ms
#define DEFAULT_HIHAT_TONE           0.8f    // 0.0-1.0
#define HIHAT_DECAY_RANGE_MIN    20.0f
#define HIHAT_DECAY_RANGE_MAX    400.0f

// Tom (톰)
#define DEFAULT_TOM_TUNING      165.0f  // Hz (120-200)
#define DEFAULT_TOM_DECAY       400.0f  // ms (200-800)
#define TOM_TUNING_RANGE_MIN    120.0f
#define TOM_TUNING_RANGE_MAX    200.0f
#define TOM_DECAY_RANGE_MIN     200.0f
#define TOM_DECAY_RANGE_MAX     800.0f

// Conga (콩가)
#define DEFAULT_CONGA_TUNING    370.0f  // Hz (250-400)
#define DEFAULT_CONGA_DECAY     300.0f  // ms (150-600)
#define CONGA_TUNING_RANGE_MIN  250.0f
#define CONGA_TUNING_RANGE_MAX  400.0f
#define CONGA_DECAY_RANGE_MIN   150.0f
#define CONGA_DECAY_RANGE_MAX   600.0f

// 기타 드럼들
#define DEFAULT_RIMSHOT_LEVEL   0.8f
#define DEFAULT_MARACAS_LEVEL   0.7f
#define DEFAULT_CLAP_LEVEL      0.9f
#define DEFAULT_COWBELL_LEVEL   0.6f

// ============================================
// 시퀀서 설정 (선택사항)
// ============================================

// 시퀀서 활성화
#define ENABLE_SEQUENCER        false   // 기본적으로 비활성화
#define DEFAULT_BPM             120     // 기본 BPM
#define MIN_BPM                 60      // 최소 BPM
#define MAX_BPM                 200     // 최대 BPM
#define DEFAULT_PATTERN_LENGTH  16      // 16비트 패턴 (기본)

// 패턴 저장
#define MAX_PATTERNS            16      // 최대 저장 패턴 수
#define PATTERN_EEPROM_ADDRESS  100     // EEPROM 저장 시작 주소

// ============================================
// 메모리 관리 설정
// ============================================

// 메모리 최적화
#define USE_STATIC_ALLOCATION   true    // 정적 메모리 할당 사용
#define OPTIMIZE_FOR_SMALL_RAM  true    // 소형 RAM 최적화
#define DISABLE_FLOAT_TRIG      false   // 부동소수점 트리거 사용 여부

// 버퍼 크기 최적화
#define AUDIO_BUFFER_SIZE       256     // 오디오 처리 버퍼
#define CONTROL_BUFFER_SIZE     64      // 제어 버퍼
#define SERIAL_BUFFER_SIZE      128     // Serial 통신 버퍼

// ============================================
// 성능 모니터링 설정
// ============================================

// 성능 모니터링 활성화
#define PERFORMANCE_MONITORING  true    // 성능 모니터링
#define CPU_USAGE_MONITORING    true    // CPU 사용률 모니터링
#define MEMORY_MONITORING       true    // 메모리 사용량 모니터링
#define AUDIO_LATENCY_MONITORING false  // 오디오 지연 모니터링

// 성능 보고서 간격
#define PERFORMANCE_REPORT_INTERVAL   5000   // 5초마다
#define PERFORMANCE_SAMPLE_COUNT      100    // 성능 샘플 수

// ============================================
// Serial 통신 설정
// ============================================

// Serial 설정
#define SERIAL_BAUDRATE         115200 // Serial 통신 속도
#define SERIAL_TIMEOUT          1000   // Serial 타임아웃 (ms)
#define COMMAND_BUFFER_SIZE     64     // 명령어 버퍼 크기

// 자동 저장 설정
#define ENABLE_AUTO_SAVE        false  // 자동 저장 기능
#define AUTO_SAVE_INTERVAL      30000  // 자동 저장 간격 (30초)
#define EEPROM_SIZE             4096   // EEPROM 크기 (ESP32C3)

// ============================================
// MIDI 지원 설정 (선택사항)
// ============================================

// MIDI 인터페이스
#define ENABLE_MIDI             false  // MIDI 지원 (기본 비활성화)
#define MIDI_BAUDRATE           31250  // MIDI 통신 속도
#define MIDI_CHANNEL            10     // GM 드럼 채널 (10)

// MIDI 노트 매핑
#define MIDI_KICK_NOTE          36     // Kick Drum
#define MIDI_SNARE_NOTE         38     // Acoustic Snare
#define MIDI_CYMBAL_NOTE        49     // Crash Cymbal
#define MIDI_HIHAT_CLOSED_NOTE  42     // Closed Hi-Hat
#define MIDI_HIHAT_OPEN_NOTE    46     // Open Hi-Hat
#define MIDI_TOM_LOW_NOTE       45     // Low Tom
#define MIDI_TOM_MID_NOTE       47     // Mid Tom
#define MIDI_TOM_HIGH_NOTE      43     // High Tom
#define MIDI_CONGA_LOW_NOTE     62     // Low Conga
#define MIDI_CONGA_HIGH_NOTE    63     // High Conga
#define MIDI_RIMSHOT_NOTE       37     // Rim Shot
#define MIDI_MARACAS_NOTE       82     // Maracas
#define MIDI_CLAP_NOTE          39     // Hand Clap
#define MIDI_COWBELL_NOTE       56     // Cowbell

// ============================================
// 디버그 및 개발 설정
// ============================================

// 디버그 출력
#define DEBUG_MODE              false  // 디버그 모드
#define VERBOSE_DEBUG           false  // 상세 디버그 정보
#define DEBUG_SERIAL_OUTPUT     true   // Serial 디버그 출력

// 오디오 테스트
#define AUTO_TEST_ON_STARTUP    true   // 시작시 자동 오디오 테스트
#define AUDIO_TEST_DURATION     1000   // 오디오 테스트 지속시간 (ms)

// ============================================
// 고급 설정
// ============================================

// 고급 오디오 처리
#define USE_ANTI_ALIASING       true   // 앤티앨리어싱
#define USE_SOFT_CLIPPING       true   // 소프트 클리핑
#define USE_COMPANDING          false  // 컴패딩 (음량 압축)
#define USE_REVERB              false  // 리버브 효과

// 필터 설정
#define DEFAULT_FILTER_CUTOFF   8000.0f  // 기본 필터 컷오프 주파수
#define DEFAULT_FILTER_RESONANCE 0.7f   // 기본 필터 공진
#define MAX_FILTER_CUTOFF       20000.0f // 최대 컷오프 주파수
#define MIN_FILTER_CUTOFF       100.0f   // 최소 컷오프 주파수

// ============================================
// 유틸리티 매크로
// ============================================

// 유틸리티 함수들
#define constrain(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define mapRange(value, inMin, inMax, outMin, outMax) ((value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin)

// 포팅 매크로
#if defined(ARDUINO_ARCH_ESP32C3)
    #define IS_ESP32C3 true
    #define IS_ESP32 true
#else
    #define IS_ESP32C3 false
    #define IS_ESP32 false
#endif

// ============================================
// 설정 검증 (컴파일 타임)
// ============================================

// 컴파일 타임 설정 검증
static_assert(DEFAULT_SAMPLE_RATE >= MIN_SAMPLE_RATE && DEFAULT_SAMPLE_RATE <= MAX_SAMPLE_RATE, 
               "샘플레이트 설정 오류");
static_assert(I2S_BUFFER_SIZE >= MIN_BUFFER_SIZE && I2S_BUFFER_SIZE <= MAX_BUFFER_SIZE, 
               "버퍼 크기 설정 오류");
static_assert(DEFAULT_MASTER_VOLUME >= MIN_VOLUME && DEFAULT_MASTER_VOLUME <= MAX_VOLUME, 
               "마스터 볼륨 설정 오류");

// 설정 유효성 검증 함수
inline void validateTR808Config() {
    // 샘플레이트 검증
    if (DEFAULT_SAMPLE_RATE < MIN_SAMPLE_RATE || DEFAULT_SAMPLE_RATE > MAX_SAMPLE_RATE) {
        Serial.println("경고: 샘플레이트가 권장 범위를 벗어났습니다.");
    }
    
    // 버퍼 크기 검증
    if (I2S_BUFFER_SIZE < MIN_BUFFER_SIZE || I2S_BUFFER_SIZE > MAX_BUFFER_SIZE) {
        Serial.println("경고: 버퍼 크기가 권장 범위를 벗어났습니다.");
    }
    
    // 볼륨 검증
    if (DEFAULT_MASTER_VOLUME < MIN_VOLUME || DEFAULT_MASTER_VOLUME > MAX_VOLUME) {
        Serial.println("경고: 마스터 볼륨이 유효 범위를 벗어났습니다.");
    }
    
    Serial.println("설정 검증 완료");
}

// ============================================
// 호환성 매크로
// ============================================

// Arduino IDE와의 호환성
#if !defined(ARDUINO_ARCH_ESP32)
    #warning "이 라이브러리는 ESP32C3에 최적화되어 있습니다."
#endif

// 라이브러리 버전 매크로
#define TR808_LIBRARY_VERSION   TR808_VERSION_MAJOR.TR808_VERSION_MINOR.TR808_VERSION_PATCH
#define TR808_LIBRARY_NAME      "ESP32C3 TR-808 Drum Machine"

// 시작/종료 매크로
#define TR808_BEGIN()           /* 시작 콜백 */
#define TR808_END()             /* 종료 콜백 */

// ============================================
// 설정 표시 함수
// ============================================

// 설정 정보를 Serial로 출력하는 함수
inline void printTR808Config() {
    Serial.println("=== TR-808 설정 정보 ===");
    Serial.println("프로젝트: " + String(TR808_PROJECT_NAME));
    Serial.println("버전: " + String(TR808_VERSION_MAJOR) + "." + 
                   String(TR808_VERSION_MINOR) + "." + 
                   String(TR808_VERSION_PATCH));
    Serial.println("빌드일: " + String(TR808_BUILD_DATE));
    Serial.println("");
    
    Serial.println("하드웨어:");
    Serial.println("  보드: " + String(BOARD_NAME));
    Serial.println("  I2S 핀: WS=" + String(I2S_WS_PIN) + 
                   ", BCK=" + String(I2S_BCK_PIN) + 
                   ", DATA=" + String(I2S_DATA_PIN));
    Serial.println("");
    
    Serial.println("오디오 설정:");
    Serial.println("  샘플레이트: " + String(DEFAULT_SAMPLE_RATE) + " Hz");
    Serial.println("  버퍼 크기: " + String(I2S_BUFFER_SIZE) + " 샘플");
    Serial.println("  마스터 볼륨: " + String(DEFAULT_MASTER_VOLUME));
    Serial.println("  폴포니: " + String(MAX_POLYPHONY));
    Serial.println("");
    
    Serial.println("기능:");
    Serial.println("  시퀀서: " + String(ENABLE_SEQUENCER ? "활성화" : "비활성화"));
    Serial.println("  MIDI: " + String(ENABLE_MIDI ? "활성화" : "비활성화"));
    Serial.println("  성능 모니터링: " + String(PERFORMANCE_MONITORING ? "활성화" : "비활성화"));
    Serial.println("  자동 저장: " + String(ENABLE_AUTO_SAVE ? "활성화" : "비활성화"));
    Serial.println("  디버그 모드: " + String(DEBUG_MODE ? "활성화" : "비활성화"));
    Serial.println("========================");
    Serial.println("");
}

#endif // ARDUINO_TR808_CONFIG_H

// ============================================
// End of Config File
// ============================================