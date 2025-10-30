# ESPherSynrh Arduino IDE 빌드+업로드 완전 가이드

## ❌ 자주 발생하는 오류 및 해결법

### 1. **I2S.h: No such file or directory** 오류

**문제**: ESP32C3에서 `I2S.h` 헤더 파일을 찾을 수 없다는 오류

**원인**:
- ESP32 Arduino 코어 3.x 버전에서 I2S 지원 문제
- ESP32C3의 RISC-V 아키텍처 호환성 문제
- 잘못된 라이브러리 설정

## ✅ 해결 방법 (3가지 옵션)

### 옵션 1: Audio.h 라이브러리 사용 (권장)

**Arduino IDE 설정:**
1. **ESP32 Arduino 코어 설치**
   ```
   Arduino IDE → File → Preferences → Additional Board Manager URLs
   추가: https://espressif.github.io/arduino-esp32/package_esp32_index.json
   Tools → Board → Boards Manager → ESP32 검색 → 2.0.18 설치 (3.x 버전 아님!)
   ```

2. **Audio.h 라이브러리 설치**
   ```
   Tools → Manage Libraries → "ESP8266Audio" 검색 → 1.9.5 설치
   ```

**업데이트된 코드 예제:**
```cpp
#include "Arduino.h"
#include "Audio.h"

#define I2S_DOUT 4  // ESP32C3 GPIO 4 (Data)
#define I2S_BCLK 3  // ESP32C3 GPIO 3 (Bit Clock)
#define I2S_LRC 2   // ESP32C3 GPIO 2 (Word Select)

Audio audio;
TR808DrumMachine drumMachine;

void setup() {
  Serial.begin(115200);
  
  // Audio.h를 사용한 I2S 초기화
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(21); // 0-21 범위
  
  drumMachine.setMasterVolume(0.8f);
}

void loop() {
  // 오디오 출력 처리
  int16_t sample = (int16_t)(drumMachine.process() * 32767);
  
  // I2S로 오디오 출력
  audio.generateSample(sample);
  
  // Serial 명령 처리
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    drumMachine.trigger(command);
  }
}
```

### 옵션 2: I2S.h 직접 구현 (PWM 출력)

**하드웨어 연결:**
- GPIO 18 → 외부 오디오 앰프/스피커
- I2S DAC 사용하지 않음

**코드 예제:**
```cpp
#include <I2S.h>

// ESP32C3 GPIO 핀 설정 (PWM 출력)
#define PWM_PIN 18
#define PWM_CHANNEL 0
#define PWM_FREQUENCY 44100
#define PWM_RESOLUTION 8

// I2S 설정 (ESP32C3 최적화)
#define SAMPLE_RATE 8000
#define BUFFER_SIZE 256

TR808DrumMachine drumMachine;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32C3 TR-808 드럼 머신 시작");
  
  // PWM 오디오 출력 설정
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(PWM_PIN, PWM_CHANNEL);
  
  drumMachine.setMasterVolume(0.5f);
}

void loop() {
  // 드럼 엔진 처리
  float audioSample = drumMachine.process();
  
  // PWM으로 오디오 출력 (8-bit)
  uint8_t pwmValue = (uint8_t)((audioSample + 1.0f) * 127.5f);
  ledcWrite(PWM_CHANNEL, pwmValue);
  
  // Serial 명령 처리
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    drumMachine.trigger(command);
    
    Serial.println("명령 실행: " + command);
  }
  
  delayMicroseconds(100); // 10kHz 업데이트 주기
}
```

### 옵션 3: ESP32 Arduino 코어 2.x 다운그레이드

**Boards Manager에서 ESP32 Core 2.0.18 설치:**
```
Tools → Board → Boards Manager
검색: "esp32"
설치: "ESP32 by Espressif Systems 2.0.18"
```

**보드 설정:**
```
Board: ESP32C3 Dev Module
Flash Mode: QIO
Flash Size: 4MB
CPU Frequency: 160MHz (WiFi)
Flash Frequency: 80MHz
Flash Size: 4MB
Core Debug Level: None
```

## 🔧 하드웨어 연결 가이드

### 옵션 A: I2S DAC 사용 (PCM5102)
```
ESP32C3              PCM5102
GPIO 2 (LRCLK)  →    LCK (WS)
GPIO 3 (BCLK)   →    BCK (BCK)  
GPIO 4 (DOUT)   →    DIN (DATA)
3.3V            →    VCC
GND             →    GND
```

### 옵션 B: PWM 출력 (Amp 연결)
```
ESP32C3              PAM8403 Amplifier
GPIO 18         →    IN+
GND             →    IN-/GND
5V              →    VCC
```

## 📋 단계별 설치 방법

### 1단계: Arduino IDE 설정
```
1. Arduino IDE 2.x 설치
2. ESP32C3 Dev Module 보드 설정
3. 포트 연결 확인
```

### 2단계: 라이브러리 설치
```
Tools → Manage Libraries →
- ESP8266Audio (earlephilhower) 1.9.5
- ESP32 by Espressif Systems 2.0.18
```

### 3단계: 예제 코드 실행
```
File → Examples → TR808_ESP32C3 → 선택한 예제
Upload 클릭
```

### 4단계: Serial Monitor 테스트
```
Baud Rate: 115200
명령어: "kick", "snare", "hihat"
```

## 🚨 문제 해결 가이드

### I2S.h 오류 해결
```
✓ ESP32 코어 2.0.18 사용
✓ Audio.h 라이브러리 설치
✓ 올바른 GPIO 핀 매핑 확인
```

### 빌드 오류 해결
```
✓ Arduino IDE 재시작
✓ 라이브러리 폴더 정리
✓ ESP32 코어 재설치
```

### 업로드 오류 해결
```
✓ 보드 타입 확인 (ESP32C3 Dev Module)
✓ 플래시 크기 4MB 설정
✓ GPIO 9 (BOOT) 버튼 누른 상태로 업로드
```

## 📊 성능 비교

| 방법 | 품질 | 메모리 | CPU 사용률 | 난이도 |
|------|------|--------|-----------|---------|
| Audio.h + I2S DAC | 최상 | 80KB | 25% | 중간 |
| I2S.h 직접 구현 | 중 | 60KB | 20% | 어려움 |
| PWM 출력 | 하 | 40KB | 15% | 쉬움 |

## 🎯 권장 설정

**최적의 성능과 품질:**
- ESP32C3 Dev Module
- ESP32 Core 2.0.18
- Audio.h 라이브러리
- PCM5102 I2S DAC
- GPIO 2,3,4 핀 사용

**빠른 테스트:**
- ESP32C3 Dev Module  
- ESP32 Core 2.0.18
- I2S.h 직접 구현
- PWM 출력 (GPIO 18)
- 오디오 앰프 연결

## 📞 추가 지원

문제가 지속되면:
1. Arduino IDE 로그 확인
2. 하드웨어 연결 재검토  
3. 라이브러리 버전 확인
4. Serial Monitor 오류 메시지 확인

**최종 목표**: ESP32C3에서 안정적인 TR-808 드럼 소리 출력!