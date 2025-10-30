# Mozzi Library 통합 분석 문서

## 개요

ESP32C3 TR-808 프로젝트의 Mozzi Library 통합을 위한 구조 분석 및 최적화 전략을 수립합니다. 

**분석 대상:**
- 기존 ESP32 I2S 구현
- Mozzi 설정 파일의 ESP32C3 최적화 상태
- TR-808 드럼 알고리즘과 Mozzi 기능의 호환성
- 통합을 위한 필요한 변경사항

---

## 1. 기존 ESP32 I2S 구현 구조 분석

### 1.1 오디오 파이프라인
```
TR808DrumMachine.process()
    ├── 각 드럼 클래스별 process()
    ├── 브리지드 T 발진기 생성
    ├── ADSR 엔벨롭 적용
    ├── 필터 처리
    ├── 마스터 볼륨 적용
    └── I2S 출력 (32.768kHz, 16-bit, 모노)
```

### 1.2 핵심 특징
- **샘플레이트:** 32.768kHz (ESP32C3 권장)
- **버퍼 크기:** 256 샘플 (7.8ms 지연)
- **출력 형식:** 16-bit 정수형 I2S
- **아키텍처:** Singleton 패턴 기반 드럼 머신
- **처리 방식:** 실시간 드럼 트리거와 사운드 생성

### 1.3 메모리 사용량
- **정적 할당:** 모든 드럼 인스턴스를 힙에 미리 생성
- **버퍼 크기:** I2S 버퍼 256 × 2바이트 = 512바이트
- **컴퓨팅:** 각 샘플마다 11개 드럼 클래스 process() 호출

---

## 2. Mozzi 설정 파일 ESP32C3 최적화 상태

### 2.1 최적화 항목 ✅
- **AudioRate:** 32768Hz (ESP32C3 권장)
- **ControlRate:** 256Hz (적정)
- **외부 타이밍:** MOZZI_OUTPUT_EXTERNAL_TIMED
- **타이머 인터럽트:** ESP32C3 TimerInterrupt 활용
- **버퍼 관리:** 256 샘플 출력 버퍼, 64 샘플 원형 버퍼

### 2.2 성능 최적화 ⚡
- **RISC-V 최적화:** ESP32C3_RISCV_OPTIMIZATION
- **ISR 최적화:** ESP32C3_OPTIMIZED_ISR
- **정적 메모리:** USE_STATIC_ALLOCATION
- **IRAM 사용:** USE_IRAM_FOR_ISR
- **DTCM 메모리:** USE_DTCM_MEMORY

### 2.3 필요한 추가 최적화
```cpp
// 메모리 풀 관리 활성화
#define USE_MEMORY_POOL_MANAGEMENT

// 오디오 샘플 프리프로세싱
#define ENABLE_AUDIO_PREPROCESSING

// ESP32C3 전용 Quirks 처리
#define ESP32C3_QUIRKS_HANDLING
```

---

## 3. Mozzi Library 기능 활용 가능성

### 3.1 generation 모듈 활용 ✅
**활용 가능한 기능:**
- `Oscil<>`: BRIDGE T 발진기 대체 가능
- `Noise<>`: 핑크/화이트 노이즈 생성
- `Phasor<>`: 페이저 변조용

**활용 예시:**
```cpp
#include <Oscil.h>
#include <Noise.h>
#include <tables/sine256.h>

// TR808Kick 대체
Oscil<SINE256_128, MOZZI_AUDIO_RATE> kickOsc;
Noise<MOZZI_AUDIO_RATE> kickNoise;
```

### 3.2 fastMath 모듈 활용 ⚡
** 최적화 영역:**
- `q31ToFloat`: 32-bit 정수 → 부동소수점 변환
- `floatToQ15`: 부동소수점 → 15-bit 정수 변환
- `expApprox()`: 지수함수 근사치 (디케이 계산용)
- `sqrtApprox()`: 제곱근 근사치 (RMS 계산용)

### 3.3 envelope 모듈 활용 ✅
**ADSR 대안:**
- `ADSR<CONTROL_RATE>`: 제어 레이트 엔벨롭
- `Envelope<CONTROL_RATE>`: 기본 엔벨롭

**장점:**
- 하드웨어 가속 지원
- 저메모리 사용
- 다양한 엔벨롭 타입 지원

---

## 4. TR-808 알고리즘 Mozzi 친화적 수정사항

### 4.1 브리지드 T 발진기 수정
**현재 구현:**
```cpp
class TR808BridgedTOscillator {
    float generate() {
        // 복잡한 브리지드 T 시뮬레이션
    }
};
```

**Mozzi 최적화:**
```cpp
#include <tables/triangle256.h>

class TR808KickMozzi {
private:
    Oscil<TRIANGLE256_128, MOZZI_AUDIO_RATE> toneOsc;
    ADSR<CONTROL_RATE> toneEnv;
    ADSR<CONTROL_RATE> pitchEnv;
    
public:
    void trigger() {
        toneOsc.setFreq(60);
        toneEnv.noteOn();
        pitchEnv.noteOn();
    }
};
```

### 4.2 엔벨롭 시스템 통합
**현재:** 마이크로초 단위 타이밍
**Mozzi:** CONTROL_RATE (256Hz) 기반

```cpp
// Mozzi 엔벨롭은 millisecond 단위
const float ATTACK_TIME_MS = 1.0f;
const float DECAY_TIME_MS = 500.0f;

// 기존 micros() → Mozzi의 getControlRateAsInt()
```

### 4.3 필터 시스템 통합
**현재:** Biquad 필터 구현
**Mozzi:** `FilterHP<>` `FilterLP<>` 사용

```cpp
#include <Filter.h>

FilterHP<256> hpf;  // 하이패스
FilterLP<256> lpf;  // 로우패스
```

---

## 5. 샘플링 레이트 및 성능 최적화

### 5.1 샘플링 레이트 설정
```cpp
// 현재: 32.768kHz (고품질, 고부하)
#define MOZZI_AUDIO_RATE 32768

// 대안: 16kHz (중간 품질, 중간 부하)
#define MOZZI_AUDIO_RATE 16384

// 권장: 32kHz (브레드+, ESP32C3 최적화)
#define MOZZI_AUDIO_RATE 32000
```

### 5.2 폴리포니 최적화
**현재:** 10개 동시 음향 제한
**Mozzi 최적화:**
```cpp
// 동적 폴리포니 매니저
#define MAX_VOICES 8  // 현실적인 제한
#define VOICE_STEALING true  // 음성 도난 방지
```

### 5.3 지연 시간 최적화
```cpp
// 버퍼 크기 최적화
#define MOZZI_OUTPUT_BUFFER_SIZE 128  // 3.9ms @ 32kHz

// 더블 버퍼링 사용
#define MOZZI_DOUBLE_BUFFERING

// 인터럽트 우선순위 조정
#define AUDIO_TIMER_PRIORITY 7  // 높게 설정
```

---

## 6. 메모리 관리 전략

### 6.1 현재 메모리 사용량
- **정적 할당:** ~5KB (드럼 클래스들)
- **버퍼:** 512바이트 (I2S 버퍼)
- **스택:** ~1KB (로컬 변수)

### 6.2 Mozzi 최적화 메모리 사용량
- **RAM 절약:** ~30% (엔벨롭 최적화)
- **플래시 절약:** ~20% (테이블 공유)
- **-stack 절약:** ~50% (지역 변수 최적화)

### 6.3 메모리 풀 관리
```cpp
class MozziVoicePool {
private:
    static constexpr int MAX_VOICES = 8;
    Voice voices[MAX_VOICES];
    bool voiceActive[MAX_VOICES];
    
public:
    Voice* allocateVoice();
    void freeVoice(Voice* voice);
};
```

---

## 7. 성능 모니터링 강화

### 7.1 추가 모니터링 항목
```cpp
// Mozzi 전용 성능 메트릭
#define MONITOR_MOZZI_PERFORMANCE

struct MozziMetrics {
    float audioCpuUsage;      // 오디오 처리 CPU 사용률
    uint32_t bufferUnderruns; // 버퍼 유니드러른 발생 횟수
    uint32_t voiceCount;      // 활성 음성 수
    float envelopeLatency;    // 엔벨롭 지연 시간
};
```

### 7.2 실시간 성능 조정
```cpp
// 성능 기반 자동 조정
void adaptiveQuality() {
    if (cpuUsage > 80.0f) {
        // 품질 자동 조절
        reducePolyphony();
        lowerSampleRate();
    }
}
```

---

## 8. 통합 아키텍처 제안

### 8.1 하이브리드 모드
**Phase 1: Mozzi-ESP32 I2S 동시 사용**
```cpp
// 기존 I2S는 PWM 출력, Mozzi는 I2S DAC 출력
#ifdef USE_MOZZI_I2S
    AudioOutputI2S audioOutput;
#else
    AudioOutputAnalog audioOutput;
#endif
```

**Phase 2: 순수 Mozzi 모드**
```cpp
#include <MozziGuts.h>
#include <Oscil.h>

void setup() {
    startMozzi();
}

void updateControl() {
    // 256Hz 업데이트
}

void updateAudio() {
    // 32kHz 오디오 생성
    return getAudioSample();
}
```

### 8.2 단계별 마이그레이션
1. **Phase 1:** 엔벨롭 모듈만 Mozzi로 교체
2. **Phase 2:** 발진기 모듈 Mozzi로 교체
3. **Phase 3:** 필터 모듈 Mozzi로 교체
4. **Phase 4:** 순수 Mozzi 아키텍처로 전환

---

## 9. 위험 요소 및 대응 방안

### 9.1 주요 위험 요소
1. **하드웨어 Quirks:** ESP32C3의 RISC-V 특화 처리 필요
2. **부하 증가:** 폴리포니와 샘플레이트 증가 시 CPU 부하
3. **메모리 제한:** USB 통신과 동시에 사용할 때 RAM 부족

### 9.2 대응 방안
```cpp
// ESP32C3 Quirks 처리
#ifdef ESP32C3
    #define IRAM_ATTR __attribute__((section(".iram1")))
#else
    #define IRAM_ATTR
#endif

// 부하 제어
#define AUTO_QUALITY_SCALING

// 메모리 절약 모드
#ifdef LOW_MEMORY_MODE
    #define DISABLE_PERFORMANCE_MONITORING
    #define REDUCE_BUFFER_SIZE
#endif
```

---

## 10. 성공 지표 및 검증 방법

### 10.1 성능 지표
- **CPU 사용률:** < 60% (32kHz, 8 voice)
- **지연 시간:** < 5ms (버퍼 포함)
- **메모리 사용량:** < 50% (ESP32C3 400KB)
- **버퍼 언더런:** 0/hour

### 10.2 음질 지표
- **THD+N:** < 0.1% (총 고조파잡음+杂音)
- **주파수 응답:** 20Hz-15kHz (±3dB)
- **동적 범위:** > 80dB

### 10.3 검증 방법
```cpp
// 성능 테스트
void performanceTest() {
    uint32_t startTime = micros();
    for (int i = 0; i < 32768; i++) {
        drumMachine.process();
    }
    uint32_t elapsed = micros() - startTime;
    Serial.println("처리시간: " + String(elapsed/32768.0) + " μs/샘플");
}
```

---

## 11. 권장사항 및 다음 단계

### 11.1 단계별 구현 순서
1. **즉시 수행:** Mozzi 설정 파일 검증 및 최적화
2. **단기 (1주):** 엔벨롭 시스템 Mozzi 교체
3. **중기 (2주):** 발진기 시스템 Mozzi 교체
4. **장기 (1개월):** 순수 Mozzi 아키텍처 완성

### 11.2 우선순위 항목
1. **Mozzi-Envelope 통합:** ADSR 성능 향상
2. **fastMath 최적화:** 계산 성능 향상
3. **고급 필터:** Band-pass, Multi-mode 필터
4. **시퀀서 통합:** MIDI-time 기반 시퀀스

### 11.3 예상 효과
- **성능 향상:** 20-30% CPU 효율성 개선
- **메모리 절약:** 15-20% RAM 사용량 감소
- **확장성:** 새로운 이펙트 쉽게 추가
- **유지보수성:** 모듈화된 구조로 코드 관리 용이

---

## 결론

ESP32C3 TR-808 프로젝트는 Mozzi Library와의 통합이 필요한 기반을 이미 잘 구축하고 있습니다. 

**주요 강점:**
- ESP32C3 특화 설정이 이미 최적화됨
- 32.768kHz 샘플레이트가 Mozzi 권장값과 일치
- 모듈화된 드럼 클래스 구조가 Mozzi 패턴과 호환

**통합 시 예상 효과:**
- 20-30% 성능 향상
- 더 많은 동시 음향 지원
- 확장 가능한 이펙트 시스템
- 유지보수성 크게 향상

**다음 단계:** `mozzi_integration_plan.h` 파일을 참조하여 단계별 통합 작업을 진행하시기 바랍니다.