# Audio.h 라이브러리 사용 TR-808 예제

## 🎯 이 예제의 목적
Audio.h 라이브러리를 사용한 안정적인 I2S 오디오 출력을 통해 ESP32C3에서 TR-808 드럼 소리를 구현합니다.

## 📋 요구사항
- **Arduino IDE 2.x**
- **ESP32C3 Dev Module**
- **ESP32 Arduino Core 2.0.18**
- **ESP8266Audio 라이브러리 1.9.5**

## 🔌 하드웨어 연결

### I2S DAC (PCM5102) 연결
```
ESP32C3              PCM5102 DAC
GPIO 2          →      LCK (WS)
GPIO 3          →      BCK (BCK)
GPIO 4          →      DIN (DATA)
3.3V            →      VCC
GND             →      GND
```

### 오디오 앰프 연결
```
PCM5102 DAC           PAM8403 앰프
LOut            →      IN1 (좌측)
ROut            →      IN2 (우측)
3.3V            →      VCC
GND             →      GND

PAM8403 앰프        스피커
OUT+            →      스피커 +
OUT-            →      스피커 -
5V              →      VCC
GND             →      GND
```

## ⚙️ Arduino IDE 설정

### 1. ESP32 코어 설치
```
Tools → Board → Boards Manager
검색: "esp32"
설치: "ESP32 by Espressif Systems 2.0.18"
```

### 2. ESP8266Audio 라이브러리 설치
```
Tools → Manage Libraries
검색: "ESP8266Audio"
설치: "ESP8266Audio by earloffilhower 1.9.5"
```

### 3. 보드 설정
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
2. 업로드 클릭
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
pattern_stop    # 패턴 정지

# 볼륨
volume 0.5      # 50% 볼륨 설정

# 시스템
status          # 시스템 상태
help            # 도움말
test            # 오디오 테스트
reset           # 시스템 리셋
clear           # 통계 초기화
```

## 🔧 문제 해결

### I2S.h 오류가 발생하는 경우
```
✓ ESP32 코어 2.0.18 사용 확인
✓ ESP8266Audio 라이브러리 설치 확인
✓ Arduino IDE 재시작
✓ 하드웨어 연결 재검토
```

### 오디오가 들리지 않는 경우
```
✓ 오디오 앰프 전원 확인 (5V)
✓ I2S DAC 연결 확인 (GPIO 2,3,4)
✓ 스피커 연결 확인
✓ 볼륨 설정 확인 (volume 0.8)
```

### 컴파일 오류가 발생하는 경우
```
✓ Arduino IDE 2.x 사용
✓ ESP32C3 Dev Module 선택
✓ ESP32 코어 2.0.18 설치
✓ ESP8266Audio 1.9.5 설치
```

## 📊 성능 특징

| 항목 | 값 |
|------|-----|
| 샘플레이트 | 44.1kHz |
| 품질 | 최고 |
| CPU 사용률 | ~25% |
| RAM 사용량 | ~80KB |
| 오디오 지연 | ~2ms |

## 🎯 이 버전이 적합한 경우
- 안정적인 오디오 출력이 필요한 경우
- 고품질 사운드가 필요한 경우
- I2S DAC를 사용할 수 있는 경우
- 메모리가 충분한 경우

## 📞 추가 지원
```bash
# 시스템 정보 확인
status

# 간단한 오디오 테스트  
test

# 도움말 확인
help
```

**최종 목표**: ESP32C3에서 안정적인 I2S 오디오를 통한 TR-808 드럼 소리!