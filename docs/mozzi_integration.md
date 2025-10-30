# Mozzi Library와 ESP32C3 통합 최적화 가이드

## 개요

본 문서는 ESP32C3에서 Mozzi Library를 활용하는 실시간 오디오 처리 시스템을 위한 최적 설정값들과 구현 방안을 제시합니다. ESP32C3의 RISC-V 아키텍처와 Mozzi Library의 특성을 고려하여 호환성, AudioRate 설정, 버퍼 관리, 인터럽트 핸들링 등의 통합 방안을 상세히 다룹니다.

## ESP32C3와 Mozzi Library 호환성 분석

### ESP32C3 아키텍처 특성

ESP32C3는 기존 ESP32와 다른 RISC-V 아키텍처를 채택하고 있으며, 다음과 같은 특성을 가집니다:

- **프로세서**: RISC-V 32-bit single-core (80/160 MHz)
- **하드웨어 타이머**: 2개 (ESP32의 4개보다 제한적)
- **오디오 출력**: I2S, PWM, ADC (내장 DAC 없음)
- **DMA**: ADC/DAC 채널, SPI, I2S 등 지원

### Mozzi Library ESP32C3 호환성

Mozzi Library는 기본적으로 ESP32를 지원하지만, ESP32C3의 RISC-V 아키텍처와 제한된 하드웨어 타이머로 인해 몇 가지 수정이 필요합니다:

#### 호환성 문제점
1. **타이머 인터페이스 차이**: 기존 ESP32용 TimerInterrupt 라이브러리가 ESP32C3에서 정상 작동하지 않음
2. **내장 DAC 부재**: ESP32C3에는 ESP32의 GPIO25/26 내장 DAC가 없음
3. **하드웨어 타이머 제한**: 2개의 타이머만 사용 가능하여 기존 ESP32(4개)보다 제약적

#### 해결 방안
1. **ESP32_C3_TimerInterrupt 라이브러리 사용**: ESP32C3 전용 타이머 인터럽트 라이브러리 활용
2. **외부 오디오 출력 모드 사용**: `MOZZI_OUTPUT_EXTERNAL_TIMED` 또는 `MOZZI_OUTPUT_EXTERNAL_CUSTOM` 모드 활용
3. **I2S 또는 PWM 오디오 출력**: ESP32C3에서 지원하는 오디오 출력 방식 사용

## 최적 설정값들

### AudioRate 설정

#### 기본 권장값
```cpp
// ESP32C3 최적 AudioRate 설정
#define MOZZI_AUDIO_RATE 32768  // 32.768kHz (16비트 모드)
```

#### 샘플레이트별 권장값
- **16.384kHz**: 안정성 우선, 복잡한 합성
- **32.768kHz**: 품질 우선, 간단한 합성
- **예외 사항**: 일부 플랫폼에서 65536Hz 모드가 호환되지 않으므로 피할 것

### 버퍼 크기 설정

#### 권장 버퍼 크기
```cpp
// 최적화된 버퍼 크기 설정
#define MOZZI_OUTPUT_BUFFER_SIZE 256  // 256 샘플 버퍼 (8ms @ 32kHz)
#define MOZZI_CIRCULAR_BUFFER_SIZE 64  // 원형 버퍼 크기 (RAM 절약)
```

#### 지연 시간 계산
- **버퍼 크기 × (1/샘플레이트) = 지연 시간**
- 예: 256 샘플 × (1/32768) = 7.8ms 지연

### 인터럽트 설정

#### 타이머 인터럽트 초기화
```cpp
#include "ESP32_C3_TimerInterrupt.h"
#include "driver/timer.h"

// 타이머 인터럽트 초기화
ESP32Timer ITimer0(0);  // 타이머 0 사용
const uint32_t TIMER_INTERVAL_MS = 1000 / MOZZI_AUDIO_RATE;

// ISR 함수
bool IRAM_ATTR TimerHandler0(void *timerNo) {
    // 오디오 업데이트 로직
    return true;  // 다음 인터럽트 지속
}

// 설정 코드
void setup() {
    // 오디오 시스템 초기화
    audioHook();  // Mozzi audio hook 호출
    
    // 타이머 인터럽트 설정 (Microseconds)
    ITimer0.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler0);
}
```

### 메모리 최적화 설정

#### 권장 컴파일러 플래그
```
-O2 -flto -fdata-sections -ffunction-sections -fno-builtin-printf
```

#### RAM 사용량 최소화
```cpp
// 정적 할당 우선
static int16_t audioBuffer[MOZZI_OUTPUT_BUFFER_SIZE];

// 동적 할당 지양
// int16_t* audioBuffer = new int16_t[MOZZI_OUTPUT_BUFFER_SIZE];  // 피할 것
```

## 실시간 오디오 처리 최적 설정

### ESP32C3 전용 타이머 인터럽트 설정

#### 타이머 설정 파라미터
```cpp
// ESP32C3 타이머 최적 설정
#define TIMER_BASE_CLK 80000000    // 80MHz 기준 클럭
#define TIMER_DIVIDER 80          // 분주 계수 (1MHz 타이머 클럭)
#define TIMER_CLOCK_FREQ 1000000   // 최종 타이머 클럭 (1MHz)

// 최적 주파수 계산
// 32.768kHz için: 1000000 / 32768 ≈ 30.5 (카운터 값)
```

#### 다중 타이머 활용
```cpp
ESP32Timer ITimer0(0);  // 오디오용 (높은 우선순위)
ESP32Timer ITimer1(1);  // 제어용 (낮은 우선순위)

// 오디오 타이머 (고우선순위)
ITimer0.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, AudioTimerISR);

// 제어 타이머 (저우선순위) 
ITimer1.attachInterruptInterval(1000, ControlTimerISR);  // 1kHz
```

### CPU 클럭 및 전원 관리

#### 최적 클럭 설정
```cpp
// Arduino IDE에서는 boards.txt에서 수정
// esp32c3.board=esp32c3.build.f_cpu=160000000L
```

#### 전원 관리
```cpp
// WiFi/BLE 사용 시 전원 최적화
#include "esp_wifi.h"
esp_wifi_set_ps(WIFI_PS_MIN_MODEM);  // 전원 절약 모드
```

## AudioRate 설정 상세 가이드

### 오디오 품질별 권장 설정

#### 최고 품질 설정 (32.768kHz)
```cpp
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 256
// 버퍼: 256 샘플 (7.8ms 지연)
// 메모리: ~4KB 사용
// CPU 부하: 중간
```

#### 안정성 우선 설정 (16.384kHz)
```cpp
#define MOZZI_AUDIO_RATE 16384
#define MOZZI_CONTROL_RATE 128
// 버퍼: 128 샘플 (7.8ms 지연)
// 메모리: ~2KB 사용  
// CPU 부하: 낮음
```

#### 저지연 설정 (48kHz 근접)
```cpp
#define MOZZI_AUDIO_RATE 49152  // 2의 제곱에 가깝게
#define MOZZI_CONTROL_RATE 384
// 버퍼: 192 샘플 (3.9ms 지연)
// 메모리: ~3KB 사용
// CPU 부하: 높음
```

### AudioRate 검증 방법

#### 타이머 검증 코드
```cpp
void validateAudioRate() {
    static uint32_t lastTime = micros();
    static uint32_t sampleCount = 0;
    
    // 오디오 ISR에서 호출
    sampleCount++;
    
    uint32_t currentTime = micros();
    uint32_t deltaTime = currentTime - lastTime;
    
    if (deltaTime >= 1000000) {  // 1초마다
        Serial.print("Actual Rate: ");
        Serial.print(sampleCount);
        Serial.println(" Hz");
        
        sampleCount = 0;
        lastTime = currentTime;
    }
}
```

## 버퍼 관리 전략

### 원형 버퍼 관리

#### 최적화된 원형 버퍼 구현
```cpp
template<typename T, size_t Size>
class OptimizedCircularBuffer {
private:
    T buffer[Size];
    volatile size_t writeIndex = 0;
    volatile size_t readIndex = 0;
    volatile size_t count = 0;
    
public:
    bool push(const T& item) {
        if (count >= Size) return false;  // 버퍼_FULL
        
        buffer[writeIndex] = item;
        writeIndex = (writeIndex + 1) % Size;
        count++;
        return true;
    }
    
    bool pop(T& item) {
        if (count == 0) return false;  // 버퍼_EMPTY
        
        item = buffer[readIndex];
        readIndex = (readIndex + 1) % Size;
        count--;
        return true;
    }
    
    size_t available() const { return count; }
    bool full() const { return count >= Size; }
    bool empty() const { return count == 0; }
};

// 사용 예시
OptimizedCircularBuffer<int16_t, 256> audioBuffer;
```

### 더블 버퍼링 (Double Buffering)

#### DMA 기반 더블 버퍼
```cpp
#define BUFFER_SIZE 256
static int16_t audioBuffer0[BUFFER_SIZE];
static int16_t audioBuffer1[BUFFER_SIZE];
static volatile uint8_t currentBuffer = 0;
static volatile uint8_t writeBuffer = 0;

// 오디오 콜백에서 사용
int16_t* getCurrentBuffer() {
    return (currentBuffer == 0) ? audioBuffer0 : audioBuffer1;
}

void switchBuffer() {
    currentBuffer = writeBuffer;
    writeBuffer = 1 - writeBuffer;
}
```

### 메모리 풀 관리

#### 고정 크기 메모리 할당자
```cpp
class AudioMemoryPool {
private:
    static const size_t POOL_SIZE = 4;  // 4개의 버퍼 풀
    static int16_t* pool[POOL_SIZE];
    static bool used[POOL_SIZE];
    
public:
    static void* allocate(size_t size) {
        for (int i = 0; i < POOL_SIZE; i++) {
            if (!used[i]) {
                used[i] = true;
                return pool[i];
            }
        }
        return nullptr;  // 풀_FULL
    }
    
    static void deallocate(void* ptr) {
        for (int i = 0; i < POOL_SIZE; i++) {
            if (pool[i] == ptr) {
                used[i] = false;
                break;
            }
        }
    }
};

// 메모리 풀 초기화
int16_t* AudioMemoryPool::pool[POOL_SIZE] = {
    new int16_t[BUFFER_SIZE],
    new int16_t[BUFFER_SIZE],
    new int16_t[BUFFER_SIZE],
    new int16_t[BUFFER_SIZE]
};
bool AudioMemoryPool::used[POOL_SIZE] = {false};
```

## 인터럽트 핸들링 최적화

### ISR 함수 최적화

#### ESP32C3 최적 ISR 코드
```cpp
// 포인터 매개변수 전달
bool IRAM_ATTR AudioTimerISR(void *timerNo) {
    // 지역 변수를 volatile로 선언하지 않고 static으로 사용
    static int16_t sample = 0;
    static uint32_t counter = 0;
    
    // 간단한 사인파 생성 (예시)
    sample = (int16_t)(32767 * sin(counter * 2 * PI / 256));
    counter++;
    
    // 오디오 출력
    audioOutput(sample);
    
    return true;  // 다음 인터럽트 지속
}

// 인터럽트 안전한 공유 변수 접근
volatile bool audioDataReady = false;
volatile int16_t sharedAudioData = 0;

// ISR에서만 값 설정
void IRAM_ATTR setAudioData(int16_t data) {
    sharedAudioData = data;
    audioDataReady = true;
}

// 메인 루프에서만 값 읽기
bool getAudioData(int16_t* data) {
    if (audioDataReady) {
        *data = sharedAudioData;
        audioDataReady = false;
        return true;
    }
    return false;
}
```

### 인터럽트 우선순위 설정

#### 타이머 우선순위 설정
```cpp
// 타이머 그룹 설정
timer_config_t timer_config = {
    .alarm_en = TIMER_ALARM_EN,
    .counter_en = TIMER_PAUSE,
    .intr_type = TIMER_INTR_LEVEL,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_EN,
    .divider = TIMER_DIVIDER  // 80 (1MHz 클럭)
};

// 타이머 초기화
timer_init(TIMER_GROUP_0, TIMER_0, &timer_config);
timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
timer_enable_intr(TIMER_GROUP_0, TIMER_0);
```

### IRQ 처리 최적화

#### ISR 루프 최소화와 타스크 위임
```cpp
// 메인 루프에서 처리할 오디오 이벤트 큐
QueueHandle_t audioEventQueue;

void IRAM_ATTR AudioISR() {
    // ISR에서는 최소한의 처리만
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // 이벤트 발생 신호를 메인 태스크에 전달
    xQueueSendFromISR(audioEventQueue, &audioData, &xHigherPriorityTaskWoken);
    
    // 필요시 컨텍스트 스위치 강제
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

// 메인 태스크에서 오디오 처리
void audioProcessingTask(void *params) {
    int16_t audioData;
    
    while (1) {
        if (xQueueReceive(audioEventQueue, &audioData, portMAX_DELAY)) {
            // 실제 오디오 처리 로직 (ISR 외에서 실행)
            processAudioData(audioData);
        }
    }
}
```

## ESP32C3 전용 설정 파일

### mozzi_config_esp32c3.h
```cpp
#ifndef MOZZI_CONFIG_ESP32C3_H
#define MOZZI_CONFIG_ESP32C3_H

// ESP32C3 최적 설정
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 256
#define MOZZI_OUTPUT_EXTERNAL_TIMED

// ESP32C3 전용 설정
#define ESP32C3_TIMER_INTERRUPT
#define ESP32C3_OPTIMIZED_ISR
#define ESP32C3_DOUBLE_BUFFERING

// 메모리 최적화
#define MOZZI_OUTPUT_BUFFER_SIZE 256
#define MOZZI_CIRCULAR_BUFFER_SIZE 64

// 인터럽트 설정
#define AUDIO_TIMER_PRIORITY 5
#define AUDIO_TIMER_SUB_PRIORITY 0

#endif
```

### audio_output_esp32c3.cpp
```cpp
#include "MozziGuts.h"
#include "AudioOutput.h"

// ESP32C3 오디오 출력 구현
void audioOutput(int16_t output) {
    // I2S 또는 PWM 출력을 위한 코드
    // 예시: GPIO 18 (I2S DATA) 출력
    
    static const int16_t offset = 32768;  // Unsigned 변환용
    
    // signed 16-bit를 unsigned로 변환
    uint16_t dac_value = (uint16_t)(output + offset);
    
    // PWM 채널에 출력 (예시)
    ledcWrite(PWM_CHANNEL, dac_value >> 8);  // 8-bit PWM
    
    // 또는 I2S 출력 (別 라이브러리 필요)
    // i2s_write(I2S_NUM, &dac_value, sizeof(dac_value), &bytes_written, 0);
}

bool canBufferAudioOutput() {
    // 새로운 샘플을 출력할 수 있는지 확인
    // 예시: 더블 버퍼링을 위한 확인
    return !bufferFull();
}

bool bufferFull() {
    // 더블 버퍼 상태 확인 로직
    return (writeIndex == readIndex && bufferCount > 0);
}

#endif
```

## 성능 모니터링 및 디버깅

### 성능 측정 도구

#### CPU 사용률 모니터링
```cpp
void monitorPerformance() {
    static uint32_t lastTime = millis();
    static uint32_t audioSamples = 0;
    static uint32_t controlUpdates = 0;
    
    uint32_t currentTime = millis();
    
    if (currentTime - lastTime >= 1000) {  // 1초마다
        Serial.println("=== Performance Monitor ===");
        Serial.print("Audio Samples/s: ");
        Serial.println(audioSamples);
        Serial.print("Control Updates/s: ");
        Serial.println(controlUpdates);
        
        // ESP32C3 전용 성능 측정
        Serial.print("Free Heap: ");
        Serial.println(ESP.getFreeHeap());
        Serial.print("CPU Frequency: ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");
        
        audioSamples = 0;
        controlUpdates = 0;
        lastTime = currentTime;
    }
}
```

#### 지연 시간 측정
```cpp
class LatencyMonitor {
private:
    static const size_t LATENCY_BUFFER_SIZE = 100;
    uint32_t latencyBuffer[LATENCY_BUFFER_SIZE];
    size_t latencyIndex = 0;
    uint32_t lastInterruptTime = 0;
    
public:
    void startTimer() {
        lastInterruptTime = micros();
    }
    
    void endTimer() {
        uint32_t latency = micros() - lastInterruptTime;
        latencyBuffer[latencyIndex] = latency;
        latencyIndex = (latencyIndex + 1) % LATENCY_BUFFER_SIZE;
    }
    
    void printLatencyStats() {
        uint32_t min = UINT32_MAX, max = 0, sum = 0;
        
        for (size_t i = 0; i < LATENCY_BUFFER_SIZE; i++) {
            uint32_t val = latencyBuffer[i];
            min = min(min, val);
            max = max(max, val);
            sum += val;
        }
        
        Serial.println("=== Latency Statistics (μs) ===");
        Serial.print("Min: "); Serial.println(min);
        Serial.print("Max: "); Serial.println(max);
        Serial.print("Avg: "); Serial.println(sum / LATENCY_BUFFER_SIZE);
    }
};

LatencyMonitor latencyMonitor;
```

### 디버깅 지원

#### 디버그 모드 설정
```cpp
#ifdef DEBUG_MOZZI_ESP32C3
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

// 디버그 정보 출력 함수
void printMozziStatus() {
    DEBUG_PRINTLN("=== Mozzi Status ===");
    DEBUG_PRINT("Audio Rate: "); DEBUG_PRINT(MOZZI_AUDIO_RATE); DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Control Rate: "); DEBUG_PRINT(MOZZI_CONTROL_RATE); DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Buffer Size: "); DEBUG_PRINT(MOZZI_OUTPUT_BUFFER_SIZE); DEBUG_PRINTLN(" samples");
    DEBUG_PRINT("Timer Frequency: "); 
    DEBUG_PRINT(1000000 / (1000000 / (TIMER_BASE_CLK / TIMER_DIVIDER)));
    DEBUG_PRINTLN(" Hz");
}
```

## 문제 해결 가이드

### 일반적인 문제점 및 해결책

#### 1. 오디오 일시중지 또는 지연
**증상**: 오디오가 간헐적으로 멈추거나 지연됨
**원인 및 해결**:
- **CPU 과부하**: 샘플레이트를 낮추거나 버퍼 크기를 늘리기
- **ISR 차단**: ISR 내 지연 함수 사용 회피
- **메모리 부족**: 동적 할당 대신 정적 할당 사용

#### 2. 인터럽트 충돌
**증상**: 타이머가 작동하지 않음
**원인 및 해결**:
- **타이머 경쟁**: Mozzi의 기본 타이머 사용 중단, ESP32_C3_TimerInterrupt 사용
- **우선순위 충돌**: IRQ 우선순위 재설정
- **ISR 함수 길이**: ISR 내 처리 시간 단축

#### 3. 컴파일 오류
**증상**: 라이브러리 호환성 오류
**원인 및 해결**:
- **라이브러리 버전**: ESP32-C3 TimerInterrupt 최신 버전 사용
- **헤더 파일**: 올바른 헤더 포함 확인
- **플랫폼 설정**: ESP32C3 보드 설정 확인

### 단계별 디버깅 절차

#### 1단계: 기본 기능 확인
```cpp
void basicAudioTest() {
    Serial.println("Starting basic audio test...");
    
    // 1초간 440Hz 톤 생성
    for (int i = 0; i < MOZZI_AUDIO_RATE; i++) {
        int16_t sample = 32767 * sin(i * 2 * PI * 440 / MOZZI_AUDIO_RATE);
        audioOutput(sample);
    }
    
    Serial.println("Basic audio test completed");
}
```

#### 2단계: 타이머 인터럽트 확인
```cpp
volatile uint32_t interruptCount = 0;

void testTimerInterrupt() {
    Serial.println("Testing timer interrupt...");
    
    // 10초간 인터럽트 카운트
    uint32_t startCount = interruptCount;
    delay(10000);
    uint32_t endCount = interruptCount;
    
    Serial.print("Interrupts in 10s: ");
    Serial.println(endCount - startCount);
    Serial.print("Expected rate: ");
    Serial.print(MOZZI_AUDIO_RATE * 10);
    Serial.println(" interrupts");
}

// ISR에서 증가시키는 카운터
void IRAM_ATTR audioTimerISR() {
    interruptCount++;
    audioHook();
}
```

#### 3단계: 성능 최적화
```cpp
void performanceOptimizationTest() {
    Serial.println("Running performance optimization test...");
    
    uint32_t startTime = micros();
    
    // 1000 오디오 샘플 생성
    for (int i = 0; i < 1000; i++) {
        // 복잡한 오디오 처리 시뮬레이션
        int16_t sample = 0;
        for (int j = 0; j < 10; j++) {
            sample += random(-1000, 1000);
        }
        audioOutput(sample);
    }
    
    uint32_t endTime = micros();
    uint32_t processingTime = endTime - startTime;
    
    Serial.print("1000 samples processing time: ");
    Serial.print(processingTime);
    Serial.println(" μs");
    Serial.print("Per sample average: ");
    Serial.print(processingTime / 1000);
    Serial.println(" μs");
    
    if (processingTime / 1000 > 30) {  // 30μs 이상이면 성능 문제
        Serial.println("WARNING: Audio processing may be too slow");
    }
}
```

## 최종 권장 설정값 요약

### ESP32C3 + Mozzi Library 최적 설정

#### 기본 권장값
```cpp
// AudioRate 설정
#define MOZZI_AUDIO_RATE 32768      // 품질 우선
#define MOZZI_CONTROL_RATE 256      // 제어율

// 출력 모드
#define MOZZI_OUTPUT_EXTERNAL_TIMED

// 버퍼 크기
#define MOZZI_OUTPUT_BUFFER_SIZE 256
#define MOZZI_CIRCULAR_BUFFER_SIZE 64

// 타이머 설정
#define TIMER_BASE_CLK 80000000
#define TIMER_DIVIDER 80
#define AUDIO_TIMER_PRIORITY 5

// 메모리 최적화
#define USE_STATIC_ALLOCATION
#define DISABLE_DYNAMIC_MEMORY

// ESP32C3 전용 설정
#define ESP32C3_TIMER_INTERRUPT
#define ESP32C3_DOUBLE_BUFFERING
```

#### 안정성 우선 설정
```cpp
#define MOZZI_AUDIO_RATE 16384      // 안정성 우선
#define MOZZI_CONTROL_RATE 128
#define MOZZI_OUTPUT_BUFFER_SIZE 128
#define MOZZI_CIRCULAR_BUFFER_SIZE 32
```

#### 저지연 설정
```cpp
#define MOZZI_AUDIO_RATE 32768      // 32.768kHz
#define MOZZI_CONTROL_RATE 384
#define MOZZI_OUTPUT_BUFFER_SIZE 128  // 더 작은 버퍼
#define MOZZI_CIRCULAR_BUFFER_SIZE 128
```

## 결론

ESP32C3와 Mozzi Library의 성공적인 통합을 위해서는 다음 핵심 요소들을 고려해야 합니다:

1. **ESP32C3 전용 타이머 인터럽트 라이브러리 사용**으로 RISC-V 아키텍처 호환성 확보
2. **외부 오디오 출력 모드 활용**으로 내장 DAC 부재 해결
3. **32.768kHz AudioRate**와 적절한 버퍼 크기로 품질과 성능의 균형
4. **ISR 최적화**와 **정적 메모리 할당**으로 실시간 성능 확보
5. **포괄적인 성능 모니터링**으로 지속적 최적화

이 가이드를 바탕으로 ESP32C3에서도 안정적이고 고품질의 Mozzi 기반 오디오 애플리케이션을 구현할 수 있습니다.

## 참고 자료

1. [Mozzi Library 공식 문서](https://sensorium.github.io/Mozzi/)
2. [ESP32C3 I2S 문서](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/i2s.html)
3. [ESP32-C3 TimerInterrupt 라이브러리](https://github.com/khoih-prog/ESP32_C3_TimerInterrupt)
4. [ESP32C3 성능 최적화 가이드](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/performance/speed.html)
5. [ESP32와 Mozzi 통합 사례](https://diyelectromusic.com/2024/03/19/esp32-and-mozzi/)

## 부록: 전체 예제 코드

### 완전한 ESP32C3 + Mozzi 예제
```cpp
#include <MozziGuts.h>
#include <Oscil.h>
#include <ESP32_C3_TimerInterrupt.h>

// 설정값
#define MOZZI_AUDIO_RATE 32768
#define MOZZI_CONTROL_RATE 256
#define MOZZI_OUTPUT_EXTERNAL_TIMED

// 오실레이터 설정 (사용할 주파수)
template <uint32_t FREQUENCY>
Oscil<MOZZI_AUDIO_RATE> kOsc;

// 타이머 인터럽트 설정
ESP32Timer ITimer0(0);
const uint32_t TIMER_INTERVAL_US = 1000000 / MOZZI_AUDIO_RATE;

// 오디오 출력 함수
void audioOutput(int16_t output) {
    // I2S 또는 PWM 출력 코드
    // 예시: GPIO 18에 PWM 출력
    static const int PWM_CHANNEL = 0;
    static const int PWM_PIN = 18;
    
    // signed 16-bit를 unsigned 8-bit로 변환
    uint8_t pwm_value = (uint16_t)(output + 32768) >> 8;
    ledcWrite(PWM_CHANNEL, pwm_value);
}

// ISR 함수
bool IRAM_ATTR AudioTimerISR(void *timerNo) {
    audioHook();  // Mozzi 오디오 훅 호출
    return true;
}

// 제어 업데이트
void updateControl() {
    // 사용자 제어 로직
}

// 설정 함수
void setup() {
    Serial.begin(115200);
    
    // PWM 채널 초기화 (필요시)
    ledcSetup(PWM_CHANNEL, 8000, 8);  // 8kHz, 8-bit
    
    // Mozzi 시작
    startMozzi();
    
    // 타이머 인터럽트 시작
    ITimer0.attachInterruptInterval(TIMER_INTERVAL_US, AudioTimerISR);
    
    Serial.println("Mozzi + ESP32C3 initialized");
}

// 메인 루프
void loop() {
    // Mozzi 업데이트 (논블로킹)
    audioHook();
}
```

이 예제는 ESP32C3에서 Mozzi Library를 사용하는 기본 템플릿으로 제공됩니다. 실제 프로젝트에서는 오실레이터 설정, PWM 채널 구성, 오디오 출력 방식을 프로젝트 요구사항에 맞게 수정해야 합니다.