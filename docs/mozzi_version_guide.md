# TR-808 ESP32C3 Mozzi Library 버전 실행 가이드

## 개요

본 문서는 Mozzi Library를 기반으로 구현된 TR-808 드럼 머신의 설치, 설정, 실행 방법을 상세히 안내합니다. ESP32C3에서 최적화된 성능으로 진짜 TR-808 사운드를 재현하는 방법을 단계별로 설명합니다.

## 🚀 빠른 시작 (5분 설정)

### 1단계: 하드웨어 연결

```
ESP32C3 개발보드
├── GPIO 1  →  I2S_DATA (외부 DAC)
├── GPIO 2  →  I2S_BCLK  
├── GPIO 3  →  I2S_WSCLK
├── 3.3V    →  VCC (DAC)
└── GND     →  GND
```

**권장 오디오 구성:**
- **외부 DAC**: PCM5102, ES9023, WM8960
- **오디오 앰프**: PAM8403, LM386
- **스피커**: 4-8Ω, 3-5W

### 2단계: Arduino IDE 설정

1. **ESP32C3 브드 패키지 설치**
   ```
   File → Preferences → Additional Board Manager URLs
   추가: https://dl.espressif.com/dl/package_esp32_index.json
   
   Tools → Board → Boards Manager
   검색: "ESP32" → "esp32 by Espressif Systems" 설치
   ```

2. **브드 선택**
   ```
   Tools → Board → ESP32 Arduino → "ESP32C3 Dev Module"
   Tools → Flash Mode → QIO
   Tools → Flash Frequency → 80MHz
   ```

### 3단계: 필수 라이브러리 설치

Arduino Library Manager에서 다음 라이브러리들을 설치하세요:

1. **Mozzi Library**
   - Library Manager → "Mozzi" 검색
   - 최신 버전 설치 (v1.1.0 이상 권장)

2. **ESP32_C3_TimerInterrupt**
   ```
   Library Manager → "ESP32C3 TimerInterrupt" 검색
   설치: "ESP32_C3_TimerInterrupt" by Kevin Harrington
   ```

3. **TR-808 라이브러리**
   ```
   Sketch → Include Library → Add .ZIP Library
   또는 직접 설치: ~/Arduino/libraries/TR808_ESP32C3/
   ```

### 4단계: 첫 번째 실행

1. **예제 열기**
   ```
   File → Examples → TR808_ESP32C3 → esp32c3_mozzi_example
   ```

2. **기본 설정 확인**
   ```cpp
   // mozzi_config.h에서 확인
   #define MOZZI_AUDIO_RATE 32768
   #define MOZZI_CONTROL_RATE 256
   ```

3. **업로드 및 테스트**
   - Upload 버튼 클릭
   - 시리얼 모니터 (115200 baud) 열기
   - "Mozzi + ESP32C3 initialized" 메시지 확인

## 📋 상세 설치 가이드

### 라이브러리 종속성 관리

#### 필수 라이브러리 목록

| 라이브러리 | 버전 | 크기 | 기능 |
|------------|------|------|------|
| Mozzi | ≥1.1.0 | ~200KB | 오디오 엔진 |
| ESP32_C3_TimerInterrupt | ≥2.0.0 | ~50KB | ESP32C3 타이머 |
| Arduino Core ESP32 | ≥2.0.0 | ~2MB | ESP32 지원 |

#### 라이브러리 설치 방법

**방법 1: Library Manager (권장)**
```bash
# Arduino IDE → Tools → Manage Libraries
# 다음 라이브러리들을 순서대로 설치:
1. "Mozzi" - Biblioteca di sintesi sonora per Arduino
2. "ESP32C3_TimerInterrupt" - Hardware Timer Interrupt for ESP32C3
```

**방법 2: 수동 설치**
```bash
# Arduino 라이브러리 폴더로 이동
cd ~/Arduino/libraries/

# Mozzi 설치
git clone https://github.com/sensorium/Mozzi.git Mozzi

# ESP32C3 TimerInterrupt 설치
git clone https://github.com/khoih-prog/ESP32_C3_TimerInterrupt.git ESP32_C3_TimerInterrupt

# TR-808 라이브러리 설치
# (이 프로젝트의 ZIP 파일을 다운로드 후 설치)
```

### 컴파일러 설정

#### Arduino IDE 설정
```
Tools → Board: "ESP32C3 Dev Module"
Tools → CPU Frequency: "160MHz (WiFi)"
Tools → Flash Mode: "QIO"
Tools → Flash Size: "4MB (32Mb)"
Tools → Partition Scheme: "Default 4MB with spiffs"
Tools → Core Debug Level: "None"
Tools → Arduino Runs On: "Core 1"
Tools → Events Run On: "Core 1"
```

#### 수동 컴파일 설정 (platform.txt)
```
# Arduino/hardware/esp32/platform.txt 수정
compiler.c.extra_flags=-DMOZZI_AUDIO_RATE=32768
compiler.cpp.extra_flags=-DMOZZI_AUDIO_RATE=32768
compiler.c.elf.flags=-O2 -flto
compiler.c.elf.flags节约=-s -DARDUINO=200
```

### 하드웨어 연결 상세

#### I2S DAC 연결 (PCM5102)

```
ESP32C3       PCM5102       전원
GPIO 1    →    BCK          -
GPIO 2    →    WS           -
GPIO 3    →    DATA         -
3.3V      →    VCC          3.3V
GND       →    GND          GND
           →    FLT          GND (low-pass filter)
           →    SCL          GND (system clock)
```

**오디오 앰프 연결 (PAM8403):**
```
PCM5102       PAM8403       스피커
OUT_L     →    IN1          -
OUT_R     →    IN2          -
GND       →    GND          -
3.3V      →    VCC          5V
           →    OUT+        →  스피커+
           →    OUT-        →  스피커-
```

#### PWM 오디오 출력 ( alternatif )

외부 DAC이 없는 경우 PWM 출력 사용:

```cpp
// GPIO 18에 PWM 오디오 출력
#define AUDIO_PIN 18
#define PWM_CHANNEL 0

void setupPWMOutput() {
    ledcSetup(PWM_CHANNEL, 8000, 8);  // 8kHz, 8-bit
    ledcAttachPin(AUDIO_PIN, PWM_CHANNEL);
}

void audioOutput(int16_t sample) {
    // 16-bit → 8-bit PWM 변환
    uint8_t pwm_value = (uint16_t)(sample + 32768) >> 8;
    ledcWrite(PWM_CHANNEL, pwm_value);
}
```

### 설정 파일 구성

#### mozzi_config.h (최적화 설정)

```cpp
#ifndef MOZZI_CONFIG_H
#define MOZZI_CONFIG_H

// =============================================================================
// ESP32C3 최적 설정
// =============================================================================

// 오디오 설정
#define MOZZI_AUDIO_RATE 32768          // 32.768kHz (ESP32C3 최적화)
#define MOZZI_CONTROL_RATE 256           // 제어율 (256 Hz)

// 출력 설정
#define MOZZI_OUTPUT_EXTERNAL_TIMED      // 외부 타임드 출력
#define MOZZI_OUTPUT_BUFFER_SIZE 256     // 256 샘플 버퍼 (8ms)

// ESP32C3 전용 설정
#define USE_ESP32_C3_TIMER_INTERRUPT     // ESP32C3 타이머 사용
#define USE_HARDWARE_TIMER_0             // 타이머 0 사용

// =============================================================================
// TR-808 드럼 설정
// =============================================================================

// 드럼 파라미터
#define MAX_DRUM_VOICES 8               // 최대 동시 음원
#define MASTER_VOLUME 0.8f              // 마스터 볼륨

// 샘플 품질
#define NOISE_QUALITY HIGH              // 노이즈 품질 (HIGH/MEDIUM/LOW)
#define FILTER_OVERSAMPLING 4           // 필터 오버샘플링

// 메모리 최적화
#define USE_STATIC_ALLOCATION           // 정적 메모리 할당
#define OPTIMIZE_FOR_ESP32C3            // ESP32C3 최적화

// =============================================================================
// 디버그 및 성능 모니터링
// =============================================================================

#define ENABLE_DEBUG_OUTPUT             // 디버그 출력 활성화
#define ENABLE_PERFORMANCE_MONITOR      // 성능 모니터링
#define ENABLE_AUDIO_TEST               // 오디오 테스트 기능

// Serial 디버그 설정
#define DEBUG_BAUDRATE 115200          // 시리얼 디버그 속도
#define SERIAL_TIMEOUT_MS 1000         // Serial 타임아웃

#endif // MOZZI_CONFIG_H
```

#### platform_specific_config.h

```cpp
#ifndef PLATFORM_SPECIFIC_CONFIG_H
#define PLATFORM_SPECIFIC_CONFIG_H

#ifdef ARDUINO_ARCH_ESP32C3

// ESP32C3 전용 설정
#define TIMER_BASE_CLK 80000000UL       // 80MHz 기본 클럭
#define TIMER_DIVIDER 80                // 1MHz 타이머 클럭
#define TIMER_PRESCALER 1000000         // 1MHz 프리스케일러

// I/O 설정
#define I2S_DATA_PIN 1                  // I2S 데이터 핀
#define I2S_BCLK_PIN 2                  // I2S 비트 클럭 핀
#define I2S_WSCLK_PIN 3                 // I2S 워드 셀렉트 핀

// PWM 설정 ( alternatif )
#define PWM_AUDIO_PIN 18                // PWM 오디오 핀
#define PWM_FREQUENCY 8000              // PWM 주파수 (8kHz)
#define PWM_RESOLUTION 8                // PWM 해상도 (8-bit)

// 전원 관리
#define USE_WIFI_PS_MIN_MODEM           // WiFi 전원 절약
#define DISABLE_BLE_IF_NOT_USED         // 미사용 시 BLE 비활성화

#else

// 기본 설정 (다른 플랫폼용)
#warning "ESP32C3이 아닌 플랫폼에서 컴파일 중"
#define TIMER_BASE_CLK 240000000UL      // ESP32 기본 클럭
#define TIMER_DIVIDER 240               // 1MHz 타이머 클럭

#endif // ARDUINO_ARCH_ESP32C3

#endif // PLATFORM_SPECIFIC_CONFIG_H
```

## 🎮 실행 및 사용법

### 첫 실행 시 확인사항

#### 1. 시스템 초기화 체크리스트

```
☐ ESP32C3 보드 연결 확인
☐ I2S DAC 연결 확인 (GPIO 1,2,3)
☐ 시리얼 모니터 115200 baud 설정
☐ Arduino IDE → ESP32C3 Dev Module 선택
☐ Mozzi Library v1.1.0+ 설치 확인
☐ ESP32_C3_TimerInterrupt 설치 확인
☐ esp32c3_mozzi_example 업로드
```

#### 2. 시리얼 모니터 메시지 확인

정상 동작 시 시리얼 모니터에 다음과 같은 메시지가 표시됩니다:

```
=== ESP32C3 + Mozzi Audio System ===
Initializing...
✓ System initialized successfully
✓ Oscillators configured
✓ Audio system started

=== System Status ===
Audio Rate: 32768 Hz
Control Rate: 256 Hz
Buffer Size: 256 samples
Timer Frequency: 32768 Hz
Free Heap: 324 KB
System Status: Healthy

=== System Ready ===
Commands:
  't' - Run audio test
  's' - Show system status
  'p' - Show performance report
  'b' - Run performance benchmark
  'm' - Toggle sequencer mode
  'r' - Restart system
  'h' - Show help
```

#### 3. 오디오 테스트 실행

```
'audio test' 또는 't' 입력:
Running audio test...
Generating 440Hz tone for 5 seconds...
Audio test completed
✓ All channels working correctly
```

### 기본 사용법

#### 1. 시리얼 명령어 참조

| 명령어 | 설명 | 예시 |
|--------|------|------|
| `t` | 오디오 테스트 실행 | `t` |
| `s` | 시스템 상태 표시 | `s` |
| `p` | 성능 보고서 표시 | `p` |
| `b` | 성능 벤치마크 실행 | `b` |
| `m` | 시퀀서 모드 토글 | `m` |
| `r` | 시스템 재시작 | `r` |
| `h` | 도움말 표시 | `h` |

#### 2. 드럼 연주 (확장 가능)

TR-808 드럼 소스 연주를 위한 확장 명령어:

```cpp
// 시퀀스 정의 (16스텝)
uint8_t kickPattern[16] = {
    1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0
};

uint8_t snarePattern[16] = {
    0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0
};

uint8_t hihatPattern[16] = {
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1
};
```

### 고급 설정

#### 1. 커스텀 드럼 사운드 생성

```cpp
// TR808_DrumMachine.h에서 확장 가능
class CustomTR808DrumMachine : public TR808DrumMachine {
public:
    // 커스텀 킥 드럼
    void triggerCustomKick(float velocity) {
        // 브리지드 T 발진기 파라미터 조정
        setBridgedTOscillatorFreq(55.0f);     // 55Hz 기본 주파수
        setDecayTime(300.0f);                  // 300ms 디케이
        setTone(0.8f);                         // 밝은 톤
        
        // 발진기 시작
        startBridgedTOscillator(velocity);
    }
    
    // 커스텀 스네어
    void triggerCustomSnare(float velocity) {
        // 듀얼 브리지드 T + 노이즈
        setNoiseLevel(0.6f * velocity);
        setToneFilterFreq(1800.0f);            // 1.8kHz 필터
        setSnappiness(0.7f);
        
        // 동시 발화
        triggerTone(velocity * 0.8f);
        triggerNoise(velocity * 0.6f);
    }
};
```

#### 2. 성능 튜닝

```cpp
// 성능 최적화 설정
void optimizePerformance() {
    // 오디오 품질 낮춰서 성능 향상
    #ifdef PERFORMANCE_MODE
        #undef MOZZI_AUDIO_RATE
        #define MOZZI_AUDIO_RATE 16384    // 16.384kHz로 낮춤
        
        #undef MOZZI_OUTPUT_BUFFER_SIZE  
        #define MOZZI_OUTPUT_BUFFER_SIZE 128  // 버퍼 축소
        
        #undef MAX_DRUM_VOICES
        #define MAX_DRUM_VOICES 4        // 동시 음원 제한
    #endif
}
```

#### 3. 메모리 최적화

```cpp
// 메모리 사용량 최소화
void optimizeMemory() {
    // 정적 메모리 사용으로 힙 사용량 감소
    static Oscil<MOZZI_AUDIO_RATE> kickOsc;
    static Oscil<MOZZI_AUDIO_RATE> snareOsc;
    
    // 테이블 크기 최적화
    #ifdef REDUCED_MEMORY_MODE
        #undef USE_FULL_SINTABLE
        #define USE_HALF_SINTABLE true
        #undef USE_FULL_NOISE_TABLE
        #define USE_COMPRESSED_NOISE_TABLE true
    #endif
}
```

## 🔧 문제 해결 가이드

### 일반적인 문제 및 해결법

#### 1. 컴파일 오류

**증상**: 
```
fatal error: MozziGuts.h: No such file or directory
```

**해결법**:
```bash
# 1. Mozzi Library 설치 확인
cd ~/Arduino/libraries/
ls -la | grep Mozzi

# 2. 올바른 헤더 포함 확인
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
```

**증상**:
```
error: 'timer' was not declared in this scope
```

**해결법**:
```cpp
// ESP32C3 전용 헤더 추가
#include <ESP32_C3_TimerInterrupt.h>

// 올바른 타이머 인터페이스 사용
ESP32Timer ITimer0(0);  // 타이머 0 초기화
```

#### 2. I2S 오디오 문제

**증상**: 소리 없음 또는 잡음

**진단 단계**:
```cpp
void diagnoseAudioOutput() {
    Serial.println("=== Audio Diagnostics ===");
    
    // 1. I2S 핀 연결 확인
    Serial.print("I2S Data Pin (GPIO 1): ");
    Serial.println(digitalRead(1) == INPUT ? "Connected" : "Not connected");
    
    // 2. 외부 DAC 응답 테스트
    testDACResponse();
    
    // 3. 오디오 콜백 실행 확인
    testAudioCallback();
}

void testDACResponse() {
    // 테스트 톤 생성
    for (int i = 0; i < 100; i++) {
        int16_t sample = 32767 * sin(i * 2 * PI / 100);
        audioOutput(sample);
        delayMicroseconds(30);  // 32.768kHz 타이밍
    }
}
```

**해결법**:
```cpp
// I2S 초기화 코드 수정
void initI2SAudio() {
    // GPIO 설정
    pinMode(1, OUTPUT);   // I2S_DATA
    pinMode(2, OUTPUT);   // I2S_BCLK  
    pinMode(3, OUTPUT);   // I2S_WSCLK
    
    // 또는 PWM 오디오 출력으로 대체
    initPWMAudio();
}
```

#### 3. 타이머 인터럽트 문제

**증상**: 오디오가 멈추거나 불규칙함

**진단**:
```cpp
// 타이머 인터럽트 상태 모니터링
volatile uint32_t interruptCount = 0;
volatile uint32_t lastInterruptTime = 0;

bool IRAM_ATTR audioTimerISR(void *timerNo) {
    interruptCount++;
    lastInterruptTime = micros();
    audioHook();  // Mozzi 오디오 훅 호출
    return true;
}

void checkTimerHealth() {
    static uint32_t lastCheckTime = 0;
    static uint32_t lastInterruptCount = 0;
    
    uint32_t now = millis();
    uint32_t deltaTime = now - lastCheckTime;
    uint32_t deltaInterrupts = interruptCount - lastInterruptCount;
    
    if (deltaTime > 1000) {  // 1초마다 체크
        Serial.print("Interrupts/sec: ");
        Serial.println(deltaInterrupts);
        Serial.print("Expected: 32768");
        Serial.print(" - Error: ");
        Serial.println(abs(deltaInterrupts - 32768));
        
        if (abs(deltaInterrupts - 32768) > 100) {
            Serial.println("WARNING: Timer interrupt irregular!");
        }
        
        lastCheckTime = now;
        lastInterruptCount = interruptCount;
    }
}
```

**해결법**:
```cpp
// ESP32_C3_TimerInterrupt 라이브러리 사용
#include <ESP32_C3_TimerInterrupt.h>

// 타이머 재설정
ESP32Timer ITimer0(0);  // 타이머 0 사용
const uint32_t TIMER_INTERVAL_US = 1000000 / MOZZI_AUDIO_RATE;  // 30.5μs

// 인터럽트 서비스 루틴
bool IRAM_ATTR audioTimerISR(void *timerNo) {
    audioHook();
    return true;
}

// 타이머 시작
void startAudioTimer() {
    if (!ITimer0.attachInterruptInterval(TIMER_INTERVAL_US, audioTimerISR)) {
        Serial.println("FATAL: Cannot start audio timer!");
        return;
    }
    
    Serial.println("✓ Audio timer started successfully");
}
```

#### 4. 메모리 부족 문제

**증상**: 시스템 크래시 또는 불안정

**진단**:
```cpp
void checkMemoryHealth() {
    Serial.println("=== Memory Health Check ===");
    
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    
    Serial.print("Min Free Heap: ");
    Serial.print(ESP.getMinFreeHeap());
    Serial.println(" bytes");
    
    Serial.print("Heap Size: ");
    Serial.print(ESP.getHeapSize());
    Serial.println(" bytes");
    
    // 메모리 부족 경고
    if (ESP.getFreeHeap() < 50000) {
        Serial.println("WARNING: Low memory detected!");
        Serial.println("Consider reducing buffer sizes");
    }
}
```

**해결법**:
```cpp
// 메모리 최적화 설정
#define MOZZI_OUTPUT_BUFFER_SIZE 128    // 256 → 128로 축소
#define MOZZI_CIRCULAR_BUFFER_SIZE 32   // 축소
#define MAX_DRUM_VOICES 4               // 동시 음원 제한

// 정적 메모리 사용
static int16_t audioBuffer[MOZZI_OUTPUT_BUFFER_SIZE];
static float filterState[MAX_DRUM_VOICES][2];

// 불필요한 오브젝트 제거
#ifndef DEBUG_MODE
    #undef ENABLE_PERFORMANCE_MONITOR
    #undef ENABLE_DEBUG_OUTPUT
#endif
```

### 성능 최적화 문제

#### 1. CPU 사용률 최적화

**증상**: 오디오 드롭아웃, 지연 증가

**최적화 코드**:
```cpp
// audioHook 최적화
void audioHook() {
    // ISR 최적화: 최소한의 작업만
    static int16_t sample = 0;
    
    // 빠른 오디오 생성 (ISR에서만 실행)
    sample = generateFastAudioSample();
    
    // I2S 출력 (가능하면 타이머에서 분리)
    queueAudioSample(sample);
}

// 제어 로직은 메인 루프에서 처리
void updateControl() {
    // 복잡한 처리 - ISR에서는 실행하지 않음
    if (controlNeedsUpdate) {
        updateDrumParameters();
        controlNeedsUpdate = false;
    }
}
```

#### 2. 지연시간 최적화

```cpp
// 더 작은 버퍼로 지연시간 감소
#define MOZZI_OUTPUT_BUFFER_SIZE 64     // 64 샘플 (2ms)
#define MOZZI_CIRCULAR_BUFFER_SIZE 32

// 더 높은 샘플레이트
#define MOZZI_AUDIO_RATE 49152          // 49.152kHz (2의 제곱에 근접)

// DMA 버퍼 사용
extern "C" void audioOutput(int16_t output) {
    // I2S DMA에 직접 출력
    i2s_write(I2S_NUM_0, &output, sizeof(int16_t), &bytes_written, 0);
}
```

### 디버깅 도구

#### 1. 실시간 성능 모니터

```cpp
class RealTimeMonitor {
private:
    uint32_t samples = 0;
    uint32_t droppedSamples = 0;
    uint32_t lastReportTime = 0;
    float cpuUsage = 0.0f;
    
public:
    void update() {
        samples++;
        
        // CPU 사용률 계산
        cpuUsage = calculateCPUUsage();
        
        // 5초마다 보고
        if (millis() - lastReportTime > 5000) {
            printReport();
            resetCounters();
            lastReportTime = millis();
        }
    }
    
    void printReport() {
        Serial.println("=== Performance Report ===");
        Serial.print("Samples/sec: "); Serial.println(samples / 5);
        Serial.print("Dropped samples: "); Serial.println(droppedSamples);
        Serial.print("CPU Usage: "); Serial.print(cpuUsage); Serial.println("%");
        Serial.print("Free Heap: "); Serial.println(ESP.getFreeHeap());
        Serial.println();
    }
    
private:
    float calculateCPUUsage() {
        // 간단한 CPU 사용률 계산
        uint32_t cycles = micros();
        // ... 계산 로직
        return calculatedUsage;
    }
};

RealTimeMonitor performanceMonitor;
```

#### 2. 오디오 품질 분석기

```cpp
class AudioQualityAnalyzer {
private:
    static const int HISTORY_SIZE = 256;
    int16_t sampleHistory[HISTORY_SIZE];
    int historyIndex = 0;
    
public:
    void analyzeSample(int16_t sample) {
        sampleHistory[historyIndex] = sample;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        
        // 주기적으로 품질 분석
        if (historyIndex == 0) {
            analyzeQuality();
        }
    }
    
private:
    void analyzeQuality() {
        // SNR (Signal-to-Noise Ratio) 계산
        float signalPower = calculateSignalPower();
        float noisePower = calculateNoisePower();
        float snr = 10 * log10(signalPower / noisePower);
        
        // 왜율 (THD) 계산  
        float thd = calculateTHD();
        
        Serial.print("Audio Quality Report:");
        Serial.print(" SNR: "); Serial.print(snr); Serial.print(" dB");
        Serial.print(" THD: "); Serial.print(thd); Serial.print("%");
        Serial.println();
    }
};
```

## 📊 성능 벤치마크

### 자동 성능 테스트

```cpp
void runPerformanceBenchmark() {
    Serial.println("=== TR-808 Performance Benchmark ===");
    
    // 1. CPU 성능 테스트
    testCPUPerformance();
    
    // 2. 메모리 사용량 테스트  
    testMemoryUsage();
    
    // 3. 오디오 품질 테스트
    testAudioQuality();
    
    // 4. 다중 음원 테스트
    testPolyphony();
    
    // 5. 안정성 테스트
    testStability();
    
    // 벤치마크 결과 종합
    printBenchmarkSummary();
}

void testCPUPerformance() {
    Serial.println("Testing CPU Performance...");
    
    uint32_t startTime = micros();
    uint32_t samplesProcessed = 0;
    
    // 10,000 샘플 처리 시간 측정
    for (int i = 0; i < 10000; i++) {
        generateAudioSample();
        samplesProcessed++;
    }
    
    uint32_t endTime = micros();
    uint32_t totalTime = endTime - startTime;
    float avgTimePerSample = (float)totalTime / samplesProcessed;
    
    Serial.print("Avg processing time per sample: ");
    Serial.print(avgTimePerSample); 
    Serial.println(" μs");
    
    // 목표 성능과 비교
    if (avgTimePerSample > 30.5) {  // 32.768kHz = 30.5μs
        Serial.println("WARNING: CPU may be overloaded");
    } else {
        Serial.println("✓ CPU performance OK");
    }
}
```

### 결과 해석 가이드

**성능 등급:**
- ⭐⭐⭐⭐⭐ **Excellent** (30μs 미만)
- ⭐⭐⭐⭐ **Good** (30-40μs)  
- ⭐⭐⭐ **Fair** (40-50μs)
- ⭐⭐ **Poor** (50-60μs)
- ⭐ **Critical** (60μs 초과)

**CPU 사용률 기준:**
- 15% 미만: ⭐⭐⭐⭐⭐ Excellent
- 15-25%: ⭐⭐⭐⭐ Good
- 25-35%: ⭐⭐⭐ Fair  
- 35-45%: ⭐⭐ Poor
- 45% 초과: ⭐ Critical

## 🎯 고급 사용법

### 1. MIDI 컨트롤러 연동

```cpp
// MIDI 입력을 통한 드럼 제어
#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

void setup() {
    // MIDI 초기화
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(handleMidiNoteOn);
    MIDI.setHandleNoteOff(handleMidiNoteOff);
    
    Serial.println("MIDI controller ready");
}

void handleMidiNoteOn(byte channel, byte pitch, byte velocity) {
    float vel = velocity / 127.0f;
    
    // GM 드럼 매핑
    switch(pitch) {
        case 36: drumMachine.triggerKick(vel); break;      // Bass Drum 1
        case 38: drumMachine.triggerSnare(vel); break;     // Acoustic Snare
        case 42: drumMachine.triggerHiHat(vel, false); break; // Closed Hi-Hat
        case 46: drumMachine.triggerHiHat(vel, true); break;  // Open Hi-Hat
        case 49: drumMachine.triggerCymbal(vel); break;    // Crash Cymbal 1
        // ... 더 많은 매핑
    }
}
```

### 2. 웹 인터페이스 연동 (WiFi)

```cpp
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// 간단한 웹 인터페이스
void setupWebInterface() {
    WiFi.begin("your-ssid", "your-password");
    
    server.on("/", handleRoot);
    server.on("/kick", handleKick);
    server.on("/snare", handleSnare);
    server.on("/cymbal", handleCymbal);
    
    server.begin();
    Serial.print("Web interface: http://");
    Serial.println(WiFi.localIP());
}

void handleRoot() {
    server.send(200, "text/html", 
        "<html><body>"
        "<h1>TR-808 Control Panel</h1>"
        "<button onclick='kick()'>Kick</button>"
        "<button onclick='snare()'>Snare</button>"
        "<button onclick='cymbal()'>Cymbal</button>"
        "<script>"
        "function kick(){fetch('/kick');}"
        "function snare(){fetch('/snare');}"
        "function cymbal(){fetch('/cymbal');}"
        "</script>"
        "</body></html>"
    );
}
```

### 3. Android 앱 연동

```cpp
// Bluetooth Classic을 통한 안드로이드 앱 연동
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

void setupBluetooth() {
    SerialBT.begin("TR808_DrumMachine");
    Serial.println("Bluetooth device started, you can pair it with Bluetooth!");
}

void loop() {
    if (SerialBT.available()) {
        String command = SerialBT.readString();
        command.trim();
        
        // 앱 명령어 해석
        if (command.startsWith("kick:")) {
            float velocity = command.substring(5).toFloat();
            drumMachine.triggerKick(velocity);
        }
        else if (command.startsWith("snare:")) {
            float velocity = command.substring(6).toFloat();
            drumMachine.triggerSnare(velocity);
        }
        // ... 더 많은 명령어
    }
}
```

## 📚 참고 자료 및 학습 리소스

### 공식 문서
- [Mozzi Library 공식 문서](https://sensorium.github.io/Mozzi/)
- [ESP32C3 데이터시트](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [ESP32_C3_TimerInterrupt 라이브러리](https://github.com/khoih-prog/ESP32_C3_TimerInterrupt)

### 커뮤니티
- [Arduino 포럼 - 오디오 섹션](https://forum.arduino.cc/c/hardware/esp32/37)
- [Mozzi GitHub Discussions](https://github.com/sensorium/Mozzi/discussions)
- [ESP32 커뮤니티 포럼](https://esp32.com/)

### 튜토리얼
- [TR-808 사운드 합성 원리](../tr808_algorithms.md)
- [I2S 오디오 프로그래밍](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
- [실시간 오디오 처리 기법](https://www.earlevel.com/main/2003/02/28/biquad-calculations/)

---

**이 가이드를 통해 ESP32C3에서 Mozzi Library 기반 TR-808 드럼 머신의 성공적인 구현을 달성할 수 있습니다.**