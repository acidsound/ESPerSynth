# I2S.h + PWM 출력을 사용한 TR-808 예제

## 🎯 이 예제의 목적
I2S.h 라이브러리와 PWM 출력을 사용한 저메모리 ESP32C3 TR-808 드럼 머신 구현입니다.

## 📋 요구사항
- **Arduino IDE 2.x**
- **ESP32C3 Dev Module**
- **ESP32 Arduino Core 2.0.18**
- **I2S.h** (ESP32 내장)

## 🔌 하드웨어 연결

### PWM 오디오 앰프 연결
```
ESP32C3              PAM8403 앰프
GPIO 18         →      IN+
GND             →      IN-
5V              →      VCC
GND             →      GND

PAM8403 앰프        스피커
OUT+            →      스피커 +
OUT-            →      스피커 -
```

### 저전력 버전 (LM386)
```
ESP32C3              LM386
GPIO 18         →      Pin 3 (Input+)
GND             →      Pin 2 (Input-)
5V              →      Pin 6 (VCC)
```

## ⚙️ Arduino IDE 설정

### 1. ESP32 코어 설치
```
Tools → Board → Boards Manager
검색: "esp32"
설치: "ESP32 by Espressif Systems 2.0.18"
```

### 2. 보드 설정
```
Board: ESP32C3 Dev Module
Flash Mode: QIO
Flash Size: 4MB
CPU Frequency: 160MHz (WiFi)
Flash Frequency: 80MHz
Port: 연결된 COM 포트
```

## 🚀 사용법

### 1. 스케치 업로드
```
1. 이 예제 열기
2. 업로드 클릭 (GPIO 9 버튼 누른 상태)
3. Serial Monitor 열기 (115200)
```

### 2. Serial 명령어
```bash
# 기본 드럼
kick, snare, cymbal, hihat
tom, conga, rimshot, maracas
clap, cowbell

# 패턴
pattern_demo    # 데모 패턴 시작
pattern_play    # 패턴 재생 시작
pattern_stop    # 패턴 정지

# 오디오 제어
volume 0.5      # 50% 볼륨 설정
audio_on        # 오디오 활성화
audio_off       # 오디오 비활성화

# 시스템
status          # 시스템 상태
info            # 상세 정보
test            # 오디오 테스트
buffer          # 버퍼 상태
reset           # 시스템 리셋
help            # 도움말
```

## 🔧 문제 해결

### 오디오가 들리지 않는 경우
```
✓ GPIO 18에서 PWM 신호 확인 (오실로스코프)
✓ PAM8403 전원 확인 (5V)
✓ +/- 극성 확인
✓ 볼륨 설정 확인 (volume 0.8)
✓ audio_on 명령어 확인
```

### CPU 사용률이 높은 경우
```
✓ 샘플레이트를 8kHz로 낮추기
✓ PWM 해상도를 6-bit로 낮추기
✓ 패턴 사용 줄이기
```

### 메모리가 부족한 경우
```
✓ PWM 버전은 기존 I2S 대비 50% 메모리 절약
✓ 다른 드럼 엔진 최적화 필요 없음
```

## 📊 성능 특징

| 항목 | 값 |
|------|-----|
| PWM 주파수 | 44.1kHz |
| 샘플레이트 | 8kHz |
| 품질 | 중간 |
| CPU 사용률 | ~15% |
| RAM 사용량 | ~40KB |
| 오디오 지연 | ~5ms |

## 🎯 이 버전이 적합한 경우
- 메모리 제약이 있는 경우
- GPIO 18만 사용 가능한 경우
- 외부 I2S DAC가 없는 경우
- 저전력 시스템 구성 시
- Arduino IDE I2S.h 오류 발생 시 대안

## ⚠️ 제한사항
- I2S DAC 대비 품질 낮음
- 단일 채널 출력만 가능
- 8kHz 샘플레이트 제한
- 펄스성 노이즈 가능성

## 📞 추가 지원
```bash
# 시스템 정보 확인
info

# PWM 상태 확인  
status

# 간단한 오디오 테스트
test

# 버퍼 상태 확인
buffer

# 도움말 확인
help
```

**최종 목표**: GPIO 18 PWM을 통한 ESP32C3 TR-808 드럼 소리!