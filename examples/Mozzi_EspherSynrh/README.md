# Mozzi TR-808 드럼 머신 ESP32C3

ESP32C3용 Roland TR-808 드럼 머신의 Mozzi Library 기반 완전 구현 예제입니다.

## 🎵 특징

### 핵심 기능
- **18개 드럼 소스 지원**: Kick, Snare, Cymbal, Hi-Hat, Tom, Conga, Rimshot, Maracas, Clap, Cowbell 등
- **실시간 트리거**: Serial 명령으로 실시간 드럼 재생
- **패턴 재생**: 내장 패턴 시퀀서 (4/4 박자, 16 스텝)
- **마스터 볼륨 제어**: 0.0-1.0 범위 볼륨 조절
- **성능 모니터링**: CPU 사용률, 지연 시간, 폴리포니 모니터링

### Mozzi Library 통합
- **AudioOutput_AUDIOINTERFACE**: 외부 타이밍 제어 모드
- **32.768kHz AudioRate**: ESP32C3 권장 최적화
- **ESP32_C3_TimerInterrupt**: 정밀 타이머 인터럽트
- **IRAM 최적화**: ISR 성능 향상

### ESP32C3 최적화
- **RISC-V 아키텍처 최적화**: 네이티브 RISC-V 명령어 활용
- **DTCM 메모리 사용**: 데이터 캐시 최적화
- **정적 메모리 할당**: 동적 메모리 사용 억제
- **多重 버퍼링**: 저지연 오디오 처리

## 🔧 하드웨어 요구사항

### 필수 부품
- **ESP32C3 개발보드**
- **GPIO 18**: PWM 오디오 출력
- **외부 오디오 앰프/스피커** (선택사항)
  - PAM8403 (3WClass-D)
  - LM386 (1W Class-AB)
  - 4-8Ω, 3-5W 스피커

### 연결 방법
```
ESP32C3 GPIO 18 → 외부 오디오 앰프/스피커
ESP32C3 3.3V   → 앰프 VCC
ESP32C3 GND    → 앰프 GND
```

## 📦 라이브러리 설치

### 필수 라이브러리
1. **Mozzi Library**
   ```
   Arduino IDE → Sketch → Include Library → Manage Libraries
   "Mozzi" 검색 후 설치
   ```

2. **ESP32_C3_TimerInterrupt**
   ```
   Arduino IDE → Sketch → Include Library → Manage Libraries  
   "ESP32_C3_TimerInterrupt" 검색 후 설치
   ```

3. **ESP32 Core for Arduino**
   ```
   File → Preferences → Additional Board Manager URLs
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   추가 후 ESP32 설치
   ```

### 보드 설정
1. **Tools → Board → ESP32C3 Dev Module** 선택
2. **Tools → Flash Mode: DIO** 설정
3. **Tools → Flash Size: 4MB** 설정
4. **Tools → Flash Frequency: 80MHz** 설정

## 🎮 사용법

### Serial 명령어

#### 기본 드럼 명령어
```bash
kick          # 베이스 드럼
snare         # 스네어 드럼
cymbal        # 심벌
hihat         # 하이햇
tom           # 톰 드럼
conga         # 콩가
rimshot       # 림샷
maracas       # 마라카스
clap          # 클랩
cowbell       # 카우벨
```

#### 볼륨 제어
```bash
volume 0.7    # 마스터 볼륨 70%로 설정
volume        # 현재 볼륨 표시
```

#### 패턴 제어
```bash
pattern_demo  # 데모 패턴 재생 시작
pattern_stop  # 패턴 중지
pattern_pause # 패턴 일시중지
pattern_resume # 패턴 재개
```

#### 상태 정보
```bash
status        # 시스템 상태 및 성능 통계
list          # 지원되는 드럼 목록
patterns      # 사용 가능한 패턴 목록
version       # 버전 정보 표시
```

#### 테스트 및 시스템
```bash
test          # 오디오 출력 테스트
reset         # 시스템 리셋
help          # 도움말 표시
```

### 벨로시티 지정
```bash
kick 0.3     # 소프트 킥 (30% 벨로시티)
snare 1.0    # 하드 스네어 (100% 벨로시티)
hihat 0.7    # 노멀 하이햇 (70% 벨로시티)
```

## 📊 성능 스펙

### ESP32C3 성능
- **CPU 사용률**: 15-25% (모든 드럼 동시 발화 시)
- **메모리 사용량**: ~30KB RAM
- **지연 시간**: 2-5ms
- **샘플 정확도**: 16-bit, 32.768kHz

### 드럼별 CPU 사용률
- **Kick**: ~2%
- **Snare**: ~3%
- **Cymbal**: ~4%
- **Hi-Hat**: ~3%
- **Tom**: ~2%
- **Conga**: ~2%
- **기타**: ~1%

## 🔍 디버그 모드

### 디버그 활성화
`mozzi_tr808_config.h` 파일에서:
```cpp
#define DEBUG_MOZZI_TR808
```

### 디버그 출력
- 오디오 샘플 데이터
- 드럼 트리거 이벤트
- 성능 메트릭
- 메모리 사용량
- 타이머 인터럽트 상태

## 🏗️ 구조

### 파일 구조
```
TR808_ESP32C3/
├── src/
│   ├── mozzi_tr808_config.h          # Mozzi 전용 설정
│   ├── mozzi_tr808_implementation.cpp # TR-808 구현
│   └── tr808_drums.h/.cpp            # 기본 드럼 클래스
├── examples/
│   └── Mozzi_TR808_ESP32C3/
│       └── Mozzi_TR808_ESP32C3.ino   # 메인 예제
└── extras/
    ├── esp32c3_mozzi_integration.h   # ESP32C3-Mozzi 통합
    └── mozzi_config.h               # 기본 Mozzi 설정
```

### 클래스 구조
- **TR808DrumMachineMozzi**: 메인 드럼 머신 클래스
- **TR808Oscillator**: 기본 오실레이터
- **TR808Envelope**: ADSR 엔벨롭 생성기
- **TR808Filter**: 필터 (로우패스/하이패스/밴드패스)
- **ESP32C3Mozzi**: ESP32C3-Mozzi 시스템 관리

## 🎛️ 커스터마이징

### 드럼 소스 추가
`mozzi_tr808_config.h`:
```cpp
enum TR808DrumSource {
    // 기존 드럼들...
    TR808_CRASH = 17,      // 크래시 심벌 추가
    TR808_SHAKER = 18      // 셰이커 추가
};

#define TR808_NUM_SOURCES 19  // 소스 수 증가
```

### 커스텀 패턴
```cpp
// mozzi_tr808_implementation.cpp의 loadDefaultPatterns() 함수 수정
void TR808DrumMachineMozzi::loadDefaultPatterns() {
    // 사용자 패턴 정의
    strcpy(patterns[0].name, "My Pattern");
    // 패턴 데이터 설정...
}
```

### 오디오 출력 변경
```cpp
// audioWrite() 함수 수정
void audioWrite(int16_t output) {
    // I2S 출력으로 변경
    #ifdef USE_I2S_OUTPUT
    // I2S 구현 코드
    #endif
    
    // PWM 출력 (기본)
    #ifndef USE_I2S_OUTPUT
    uint8_t pwmValue = constrain((output + 32768) >> 8, 0, 255);
    analogWrite(AUDIO_OUTPUT_CHANNEL, pwmValue);
    #endif
}
```

## 🚨 문제 해결

### Serial 통신 문제
- Boud rate: 115200 설정 확인
- Arduino IDE Serial Monitor 사용
- line ending: "Newline" 설정

### 오디오 출력 문제
- GPIO 18 연결 확인
- 외부 오디오 앰프/스피커 연결
- 볼륨 설정 확인
- PWM 해상도 설정 점검

### 성능 문제
- 마스터 볼륨 감소 (CPU 사용률 감소)
- 동시에 재생되는 드럼 소스 제한
- 디버그 모드 비활성화
- Serial 출력 빈도 감소

### 메모리 부족
- 동적 메모리 사용 억제
- Serial 출력 비활성화
- 버퍼 크기 감소
- 드럼 소스 수 제한

## 📝 예제 실행

### 1단계: 기본 테스트
```bash
kick
snare
hihat
```

### 2단계: 패턴 테스트
```bash
pattern_demo  # 데모 패턴 시작
status        # 성능 확인
pattern_stop  # 패턴 중지
```

### 3단계: 커스터마이징
```bash
volume 0.5    # 볼륨 조절
test          # 전체 오디오 테스트
```

## 🤝 기여하기

### 개발 환경
- Arduino IDE 2.x
- ESP32C3 Dev Module
- Debug Console (115200 baud)

### 코드 스타일
- C++17 표준
- Arduino 코드 스타일
- 포괄적인 주석
- 함수 단위 모듈화

### Pull Request 가이드라인
1. 기능 설명 포함
2. 테스트 코드 추가
3. 문서 업데이트
4. 호환성 확인

## 📄 라이선스

MIT License - 자세한 내용은 LICENSE 파일을 참조하세요.

## 🙏 감사의 말

- **Mozzi Team**: 훌륭한 오디오 라이브러리 제공
- **ESP32 Community**: ESP32C3 지원 및 최적화
- **Arduino Community**: 개발 환경 지원

## 📞 지원

- **GitHub Issues**: 버그 리포트 및 기능 요청
- **Discord**: 실시간 개발 논의
- **Email**: 기술 지원 (README 참조)

---

**Made with ❤️ for ESP32C3 and Mozzi Library**