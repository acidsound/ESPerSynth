# ESP32C3 + Mozzi Library 통합 시스템

ESP32C3 (RISC-V 아키텍처)와 Mozzi Library의 완전한 통합을 위한 최적화된 설정 및 구현입니다.

## 📋 목차

- [개요](#개요)
- [주요 특징](#주요-특징)
- [하드웨어 요구사항](#하드웨어-요구사항)
- [소프트웨어 의존성](#소프트웨어-의존성)
- [설치 가이드](#설치-가이드)
- [빠른 시작](#빠른-시작)
- [API 참조](#api-참조)
- [설정 옵션](#설정-옵션)
- [성능 최적화](#성능-최적화)
- [문제 해결](#문제-해결)
- [예제 및 튜토리얼](#예제-및-튜토리얼)
- [기여](#기여)

## 🎵 개요

이 프로젝트는 ESP32C3의 RISC-V 아키텍처 특성을 고려하여 Mozzi Library를 최적화한 완전한 통합 시스템입니다. 32.768kHz 오디오 처리, 실시간 성능 모니터링, 버퍼 관리 등 모던 오디오 애플리케이션 개발에 필요한 모든 기능을 제공합니다.

### 지원 기능

- ✅ 32.768kHz 고품질 오디오 처리
- ✅ ESP32_C3_TimerInterrupt 기반 타이밍 제어
- ✅ 최적화된 버퍼 관리 (더블 버퍼링 + 원형 버퍼)
- ✅ 실시간 성능 모니터링
- ✅ PWM 및 I2S 오디오 출력 지원
- ✅ 자동 오류 감지 및 복구
- ✅ 포괄적인 디버깅 지원

## ✨ 주요 특징

### 🎯 성능 최적화
- **32.768kHz AudioRate**: 품질과 성능의 최적 균형점
- **최적화된 ISR**: RISC-V 특화 인터럽트 처리
- **메모리 풀 관리**: 제한된 ESP32C3 RAM 효율적 활용
- **CPU 사용률 모니터링**: 실시간 시스템 부하 추적

### 🛡️ 안정성
- **자동 오류 복구**: 오버플로우 및 인터럽트 누락 감지
- **버퍼 오버플로우 보호**: 데이터 손실 방지
- **메모리 누수 방지**: 정적 할당 우선 정책
- **응급 정지 시스템**: Critical 상황에서의 안전Shutdown

### 🔧 개발 편의성
- **단일 클래스 인터페이스**: 복잡한 초기화 과정 단순화
- **자동 시스템 관리**: 수동 설정 최소화
- **포괄적 로깅**: 상세한 디버그 정보 제공
- **테스트 모드**: self-test 및 벤치마크 기능

## 🔧 하드웨어 요구사항

### 최소 요구사항
- **보드**: ESP32C3 (RISC-V single-core)
- **RAM**: 4MB Flash, 400KB+ SRAM
- **GPIO**: GPIO 18 (오디오 출력)
- **클럭**: 80MHz 이상

### 권장 설정
- **개발 환경**: Arduino IDE 2.x 또는 PlatformIO
- **시리얼 연결**: 115200 baud
- **외부 오디오 장치**:amplifier, speaker, headphones

### 연결도
```
ESP32C3          오디오 출력
GPIO 18 ---------> PWM/O (8kHz)
GND    -----------> GND (audio reference)
```

## 📦 소프트웨어 의존성

### 필수 라이브러리
```bash
Arduino Core for ESP32C3 (최신 버전)
Mozzi Library (v1.1.0+)
ESP32_C3_TimerInterrupt (v3.0.0+)
```

### 설치 방법

#### Arduino IDE
1. Arduino IDE 2.x 설치
2. ESP32C3 보드 패키지 설치
3. 라이브러리 매니저에서 다음 설치:
   - "Mozzi"
   - "ESP32C3 Timer Interrupt"

#### PlatformIO
```ini
[env:esp32c3]
platform = espressif32
board = esp32c3
framework = arduino
lib_deps = 
    mozzi/Mozzi@^1.1.0
    khoih-prog/ESP32_C3_TimerInterrupt@^3.0.0
```

## 🚀 빠른 시작

### 1단계: 기본 예제 실행
```cpp
#include "esp32c3_mozzi_integration.h"

void setup() {
    Serial.begin(115200);
    
    // 시스템 초기화 및 시작
    if (mozziSystem.initialize()) {
        mozziSystem.startAudio();
        Serial.println("Sistema listo!");
    }
}

void loop() {
    // 오디오 처리는 자동 처리됨
    // mozziSystem.printStatus(); // 주기적 상태 확인
}
```

### 2단계: 오디오 합성 추가
```cpp
#include <Oscil.h>
#include <tables/sin2048_int8.h>

template <uint32_t FREQUENCY>
Oscil<MOZZI_AUDIO_RATE, MOZZI_CONTROL_RATE> osc;

void updateControl() {
    osc.setFreq(440); // 440Hz
}

void customAudioProcessing() {
    return osc.next(); // 사인파 출력
}
```

### 3단계: 성능 모니터링
```cpp
void loop() {
    static uint32_t lastCheck = 0;
    
    if (millis() - lastCheck > 5000) {
        mozziSystem.printPerformanceReport();
        lastCheck = millis();
    }
}
```

## 📚 API 참조

### ESP32C3Mozzi 클래스

#### 초기화 함수들
```cpp
bool initialize();                    // 전체 시스템 초기화
bool initializeAudio();               // 오디오 시스템 초기화
bool initializeTimers();              // 타이머 시스템 초기화
bool initializeBuffers();             // 버퍼 시스템 초기화
bool initializePerformanceMonitoring(); // 성능 모니터링 초기화
```

#### 실행 제어 함수들
```cpp
bool startAudio();                    // 오디오 시스템 시작
bool stopAudio();                     // 오디오 시스템 정지
bool restartAudio();                  // 오디오 시스템 재시작
```

#### 상태 확인 함수들
```cpp
bool isInitialized() const;           // 시스템 초기화 상태
bool isAudioActive() const;           // 오디오 실행 상태
bool isSystemHealthy();               // 시스템 건강 상태
```

#### 정보 출력 함수들
```cpp
void printSystemStatus();             // 전체 시스템 상태
void printPerformanceReport();        // 성능 보고서
void printBufferStatus();             // 버퍼 상태
void printTimerStatus();              // 타이머 상태
void printConfiguration();            // 설정 정보
```

#### 테스트 함수들
```cpp
void runSelfTest();                   // 전체 시스템 자가 테스트
void runAudioTest();                  // 오디오 시스템 테스트
void runPerformanceBenchmark();       // 성능 벤치마크
void validateConfiguration();         // 설정 검증
```

### 전역 함수들

#### 초기화 및 시작
```cpp
void startMozzi();                    // Arduino 호환 시작 함수
bool isMozziSystemReady();            // 시스템 준비 상태 확인
void printQuickStatus();              // 빠른 상태 확인
```

#### 오류 처리
```cpp
void handleMozziError(const char* error); // 오류 처리
void cleanupMozziSystem();            // 시스템 정리
```

### 매크로 함수들

#### 편의 매크로
```cpp
MOZZI_SYSTEM_INIT()                   // 시스템 초기화
MOZZI_AUDIO_INIT()                    // 오디오 초기화
MOZZI_TIMER_INIT()                    // 타이머 초기화
MOZZI_START()                         // 시스템 시작
MOZZI_STOP()                          // 시스템 정지
MOZZI_IS_READY()                      // 준비 상태 확인
MOZZI_SELF_TEST()                     // 자가 테스트 실행
```

## ⚙️ 설정 옵션

### mozzi_config.h 주요 설정

#### AudioRate 설정
```cpp
#define MOZZI_AUDIO_RATE 32768        // 32.768kHz (권장)
#define MOZZI_CONTROL_RATE 256        // 256Hz 제어율
```

#### 버퍼 설정
```cpp
#define MOZZI_OUTPUT_BUFFER_SIZE 256  // 출력 버퍼 크기
#define MOZZI_CIRCULAR_BUFFER_SIZE 64 // 원형 버퍼 크기
#define MOZZI_DOUBLE_BUFFERING        // 더블 버퍼링 사용
```

#### 타이머 설정
```cpp
#define TIMER_BASE_CLK 80000000UL     // 80MHz 기준 클럭
#define TIMER_DIVIDER 80              // 1MHz 타이머 클럭
#define AUDIO_TIMER_PRIORITY 5        // 타이머 우선순위
```

#### 성능 모니터링
```cpp
#define ENABLE_PERFORMANCE_MONITORING // 성능 모니터링 활성화
#define MEASURE_LATENCY               // 지연 시간 측정
#define MONITOR_CPU_USAGE             // CPU 사용률 모니터링
```

#### 디버그 설정
```cpp
#define DEBUG_MOZZI_ESP32C3           // 디버그 모드 활성화
```

### 플랫폼별 최적화

#### ESP32C3 전용 설정
```cpp
#define ESP32C3_TIMER_INTERRUPT       // ESP32C3 타이머 인터럽트
#define ESP32C3_OPTIMIZED_ISR         // 최적화된 ISR
#define ESP32C3_RISCV_OPTIMIZATION    // RISC-V 최적화
```

#### 메모리 최적화
```cpp
#define USE_STATIC_ALLOCATION         // 정적 할당 우선
#define DISABLE_DYNAMIC_MEMORY        // 동적 메모리 비활성화
#define USE_MEMORY_POOL_MANAGEMENT    // 메모리 풀 관리
```

## 🚀 성능 최적화

### AudioRate 최적화

#### 품질 우선 (기본값)
```cpp
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 256
// 버퍼: 256 샘플 (7.8ms 지연)
// 메모리: ~4KB 사용
// CPU 부하: 중간
```

#### 안정성 우선
```cpp
#define MOZZI_AUDIO_RATE 16384
#define MOZZI_CONTROL_RATE 128
// 버퍼: 128 샘플 (7.8ms 지연)
// 메모리: ~2KB 사용
// CPU 부하: 낮음
```

#### 저지연 우선
```cpp
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 384
// 버퍼: 128 샘플 (3.9ms 지연)
// 메모리: ~3KB 사용
// CPU 부하: 높음
```

### 메모리 최적화

#### 정적 할당 전략
```cpp
// 정적 버퍼 사용 (RAM 절약)
static int16_t audioBuffer[MOZZI_OUTPUT_BUFFER_SIZE];

// 메모리 풀 활용
void* ptr = allocateAudioMemory();
// 사용 후
deallocateAudioMemory(ptr);
```

#### 컴파일러 최적화 플래그
```ini
# platformio.ini
build_flags = 
    -O2 -flto -fdata-sections -ffunction-sections
    -fno-builtin-printf
```

### CPU 최적화

#### ISR 최적화
```cpp
// IRAM 속성으로 최적화
bool IRAM_ATTR TimerHandler(void *timerNo) {
    // 간단하고 빠른 처리만
    audioHook();
    return true;
}
```

#### 우선순위 조정
```cpp
// 타이머 우선순위 설정
#define AUDIO_TIMER_PRIORITY 5        // 높은 우선순위
#define CONTROL_TIMER_PRIORITY 3      // 더 높은 우선순위
```

## 🔧 문제 해결

### 일반적인 문제점

#### 1. 컴파일 오류

**문제**: `ESP32_C3_TimerInterrupt` 라이브러리 오류
```cpp
// 해결: 최신 버전 설치
// Arduino IDE > Tools > Manage Libraries > ESP32C3 Timer Interrupt
```

**문제**: Mozzi 헤더 파일 오류
```cpp
// 해결: Arduino/Core for ESP32C3 업데이트
// ESP32C3 보드 패키지 최신 버전 설치
```

#### 2. 오디오 출력 문제

**문제**: 오디오가 들리지 않음
```cpp
// 해결 방법:
1. GPIO 18 연결 확인
2. 오디오 앰프/스피커 연결 확인
3. PWM 채널 설정 확인
4. `testAudioOutput()` 실행
```

**문제**: 오디오가 깨져 들림
```cpp
// 해결:
1. AudioRate 낮추기 (16384로 변경)
2. 버퍼 크기 늘리기 (512로 변경)
3. CPU 클럭 160MHz로 설정
```

#### 3. 성능 문제

**문제**: 오디오 일시중지
```cpp
// 해결:
1. CPU 사용률 확인: mozziSystem.printPerformanceReport()
2. AudioRate 낮추기
3. 버퍼 크기 늘리기
4. 불필요한 오실레이터 제거
```

**문제**: 높은 지연 시간
```cpp
// 해결:
1. Interrupt 우선순위 조정
2. ISR 내 처리 로직 최적화
3. 메모리 풀 사용
4. DMA 버퍼 활용 (I2S 모드)
```

### 디버깅 도구

#### 성능 모니터링 활성화
```cpp
#define DEBUG_MOZZI_ESP32C3
#define ENABLE_PERFORMANCE_MONITORING

void loop() {
    static uint32_t lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        mozziSystem.printPerformanceReport();
        lastCheck = millis();
    }
}
```

#### 시스템 상태 점검
```cpp
void checkSystemHealth() {
    Serial.println("=== System Health Check ===");
    
    // 메모리 확인
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    
    // 버퍼 상태 확인
    Serial.print("Buffer Full: ");
    Serial.println(isBufferFull() ? "Yes" : "No");
    
    // 타이머 상태 확인
    Serial.print("Audio Timer: ");
    Serial.println(isAudioTimerRunning() ? "Running" : "Stopped");
    
    // 시스템 건강 상태
    Serial.print("System Healthy: ");
    Serial.println(mozziSystem.isSystemHealthy() ? "Yes" : "No");
}
```

#### 오류 로깅
```cpp
void logError(const char* message) {
    Serial.print("[ERROR] ");
    Serial.println(message);
    
    // 상세 정보 로깅
    Serial.print("Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" | Uptime: ");
    Serial.print(millis());
    Serial.println("ms");
}
```

### 단계별 문제 해결

#### 1단계: 기본 연결 확인
```cpp
void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Testing basic connections...");
    
    // GPIO 18 출력 테스트
    pinMode(18, OUTPUT);
    for (int i = 0; i < 5; i++) {
        digitalWrite(18, HIGH);
        delay(500);
        digitalWrite(18, LOW);
        delay(500);
    }
    
    Serial.println("GPIO test completed");
}
```

#### 2단계: 라이브러리 호환성 확인
```cpp
#include "ESP32_C3_TimerInterrupt.h"
#include "MozziGuts.h"

void testLibraries() {
    Serial.println("Testing library compatibility...");
    
    // TimerInterrupt 테스트
    ESP32Timer testTimer(0);
    Serial.println("✓ ESP32_C3_TimerInterrupt OK");
    
    // Mozzi 기본 설정 테스트
    Serial.print("Moazi Audio Rate: ");
    Serial.println(MOZZI_AUDIO_RATE);
    Serial.println("✓ Mozzi Configuration OK");
}
```

#### 3단계: 단계별 시스템 구축
```cpp
void setup() {
    Serial.begin(115200);
    
    // 1. 버퍼만 초기화
    initializeBufferManager();
    Serial.println("✓ Buffer manager OK");
    
    // 2. 오디오만 초기화
    initializeAudioOutput();
    Serial.println("✓ Audio output OK");
    
    // 3. 타이머만 초기화
    initializeTimerInterrupts();
    Serial.println("✓ Timer interrupts OK");
    
    // 4. 전체 시스템 통합
    mozziSystem.initialize();
    Serial.println("✓ Full system OK");
}
```

## 📖 예제 및 튜토리얼

### 예제 1: 기본 오디오 합성
```cpp
#include "esp32c3_mozzi_integration.h"
#include <Oscil.h>
#include <tables/sin2048_int8.h>

template <uint32_t FREQUENCY>
Oscil<MOZZI_AUDIO_RATE, MOZZI_CONTROL_RATE> osc;

void setup() {
    Serial.begin(115200);
    
    // 오실레이터 설정
    osc.setFreq(440); // 440Hz
    
    // 시스템 시작
    mozziSystem.initialize();
    mozziSystem.startAudio();
    
    Serial.println("Basic synth ready!");
}

void updateControl() {
    // 1초마다 주파수 변경
    static uint32_t lastChange = 0;
    if (millis() - lastChange > 1000) {
        int newFreq = 220 + random(0, 880);
        osc.setFreq(newFreq);
        Serial.print("Frequency: ");
        Serial.println(newFreq);
        lastChange = millis();
    }
}

void customAudioProcessing() {
    return osc.next(); // 사인파 출력
}
```

### 예제 2: 다중 오실레이터 합성
```cpp
#include <tables/saw2048_int8.h>
#include <tables/squarewave_noalias_2048_int8.h>

Oscil<MOZZI_AUDIO_RATE, MOZZI_CONTROL_RATE> sinOsc;
Oscil<MOZZI_AUDIO_RATE, MOZZI_CONTROL_RATE> sawOsc;
Oscil<MOZZI_AUDIO_RATE, MOZZI_CONTROL_RATE> sqrOsc;

void setup() {
    sinOsc.setFreq(220);
    sawOsc.setFreq(220);
    sawOsc.setTable(SAW2048_DATA);
    sqrOsc.setFreq(220);
    sqrOsc.setTable(SQUAREWAVE_NOALIAS_2048_DATA);
}

int16_t customAudioProcessing() {
    int16_t sample = 0;
    
    // 3개 오실레이터 혼합
    sample += sinOsc.next() * 0.4f;
    sample += sawOsc.next() * 0.3f;
    sample += sqrOsc.next() * 0.3f;
    
    // 클리핑 방지
    return constrain(sample, -32768, 32767);
}
```

### 예제 3: 시퀀서 및 패턴 생성
```cpp
const int NUM_STEPS = 16;
const int bassPattern[NUM_STEPS] = {
    110, 0, 146, 0, 130, 0, 164, 0,
    110, 0, 146, 0, 130, 0, 164, 0
};

int currentStep = 0;
uint32_t stepTimer = 0;
const uint32_t STEP_DURATION = 200; // 200ms per step

void updateControl() {
    if (millis() - stepTimer > STEP_DURATION) {
        currentStep = (currentStep + 1) % NUM_STEPS;
        int freq = bassPattern[currentStep];
        if (freq > 0) {
            sinOsc.setFreq(freq);
        }
        stepTimer = millis();
    }
}
```

### 예제 4: 성능 모니터링 대시보드
```cpp
void printPerformanceDashboard() {
    static uint32_t lastUpdate = 0;
    
    if (millis() - lastUpdate > 2000) { // 2초마다 업데이트
        Serial.println("\n=== Performance Dashboard ===");
        
        // CPU 및 메모리
        Serial.print("CPU Usage: ");
        Serial.print(cpuUsagePercent);
        Serial.print("% | Heap: ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes");
        
        // 오디오 통계
        Serial.print("Audio Samples: ");
        Serial.print(audioSamplesProcessed);
        Serial.print(" | Dropped: ");
        Serial.print(audioDroppedSamples);
        Serial.print(" | Overflows: ");
        Serial.println(audioBufferOverflows);
        
        // 버퍼 상태
        Serial.print("Buffer Usage: ");
        Serial.print(writeIndex);
        Serial.print("/");
        Serial.print(MOZZI_OUTPUT_BUFFER_SIZE);
        Serial.print(" | ");
        Serial.print(isBufferFull() ? "FULL" : "OK");
        
        // 타이머 성능
        Serial.print(" | ISR Max: ");
        Serial.print(maxTimerLatency);
        Serial.print("μs");
        
        Serial.println();
        
        lastUpdate = millis();
    }
}
```

## 🤝 기여

### 개발 환경 설정
1. 이 저장소 Fork
2. feature 브랜치 생성: `git checkout -b feature/amazing-feature`
3. 변경사항 Commit: `git commit -m 'Add amazing feature'`
4. 브랜치 Push: `git push origin feature/amazing-feature`
5. Pull Request 생성

### 코드 스타일 가이드
- 4 spaces 인디entation
- 함수명: camelCase
- 변수명: snake_case
- 상수: UPPER_SNAKE_CASE
- 클래스명: PascalCase

### 버그 리포트
버그 발견 시 다음 정보와 함께 이슈 생성:
- ESP32C3 모델 및 revision
- Arduino Core 버전
- Mozzi Library 버전
- 재현 단계
- 예상 결과 vs 실제 결과
- 시리얼 출력 (디버그 모드 활성화 시)

### 기능 요청
새로운 기능 제안 시:
- 사용 시나리오 설명
- 예상되는 성능 영향
- 구현 복잡도 평가
- 대안 고려사항

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 📞 지원 및 커뮤니티

- **문서**: [Wiki](https://github.com/your-repo/esp32c3-mozzi/wiki)
- **이슈**: [GitHub Issues](https://github.com/your-repo/esp32c3-mozzi/issues)
- **토론**: [GitHub Discussions](https://github.com/your-repo/esp32c3-mozzi/discussions)

## 🙏 acknowledgments

- **Mozzi Team**: 훌xn한 오디오 라이브러리 제공
- **Espressif**: ESP32C3 플랫폼 지원
- **Arduino Community**: 디버깅 및 최적화 팁
- **RISC-V Community**: RISC-V 아키텍처 최적화 가이드

## 📈 로드맵

### v1.1.0 (계획중)
- [ ] I2S 오디오 출력 지원
- [ ] DMA 기반 버퍼 관리
- [ ] FFT 분석 기능 추가
- [ ] 모바일 앱 연동 지원

### v1.2.0 (계획중)
- [ ] WebUI 대시보드
- [ ] preset management 시스템
- [ ] MIDI 입력 지원
- [ ] 음성 합성 (wavetable) 확장

### v2.0.0 (장기 계획)
- [ ] 멀티 코어 지원 (future ESP32 variants)
- [ ] 실시간 오디오 효과
- [ ] 클라우드 동기화
- [ ] AI 기반 오디오 합성

---

**ESP32C3 + Mozzi Library Integration**  
버전 1.0.0 | 빌드 2025-10-30  
라이선스: MIT | 기여자: ESP32C3 Audio Community