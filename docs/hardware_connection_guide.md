# ESPherSynrh 하드웨어 연결 가이드

## 🛠️ 하드웨어 요구사항

### 기본 구성요소

| 구성요소 | 필수/선택 | 권장 모델 | 가격대 |
|----------|-----------|-----------|--------|
| ESP32C3 보드 | 필수 | XIAO ESP32C3, ESP32C3 DevKit | $3-8 |
| I2S DAC | Audio.h 버전 | PCM5102, ES9023 | $2-5 |
| PWM 앰프 | PWM 버전 | PAM8403, LM386 | $1-3 |
| 스피커 | 필수 | 3W 4Ω 스피커 | $2-5 |
| 건전지/전원 | 선택 | 3.7V Li-ion, USB | $2-8 |

## 🔌 옵션 A: I2S DAC 연결 (Audio.h 버전)

### PCM5102 I2S DAC 연결

```
ESP32C3                      PCM5102 I2S DAC
GPIO 2 (IO2)           →      LCK (Word Select)
GPIO 3 (IO3)           →      BCK (Bit Clock)
GPIO 4 (IO4)           →      DIN (Data In)
3.3V                   →      VCC
GND                    →      GND
```

### 외부 오디오 앰프 연결

```
PCM5102 DAC                PAM8403 앰프
LOut (VOUTL)        →      IN1 (좌측)
ROut (VOUTR)        →      IN2 (우측)
3.3V                →      VCC
GND                 →      GND

PAM8403 앰프              스피커
OUT+                 →      스피커 +
OUT-                 →      스피커 -
5V (또는 USB)        →      VCC
GND                 →      GND
```

### 최종 하드웨어 구성도

```
                    ESP32C3
                      │
           I2S DAC (PCM5102)
                      │
                  PAM8403
                    │
              스피커 × 2
```

## 🔌 옵션 B: PWM 앰프 연결 (I2S+PWM 버전)

### PAM8403 직접 연결

```
ESP32C3                      PAM8403
GPIO 18               →      IN+
GND                   →      IN-
5V (또는 USB)         →      VCC
GND                   →      GND

PAM8403                스피커
OUT+                   →      스피커 +
OUT-                   →      스피커 -
```

### LM386 저전력 앰프

```
ESP32C3                      LM386
GPIO 18               →      Pin 3 (Input+)
GND                   →      Pin 2 (Input-)
5V                    →      Pin 6 (VCC)
```

## 🎛️ GPIO 핀 매핑 확인

### XIAO ESP32C3 핀 다이어그램

```
          ┌─────────────┐
          │  ┌───┐  RST │ ← GPIO 18
    3V3   │  │USB│   3V3 │ ← GPIO 17  
    GND   │  └───┘   GND │ ← GPIO 16
    5V    │  GND   GND  │ ← GPIO 15
    GPIO 19│  IO19  IO2  │ ← GPIO 2 (I2S WS)
    GPIO 18│  IO18  IO3  │ ← GPIO 3 (I2S BCK)
    GPIO 17│  IO17  IO4  │ ← GPIO 4 (I2S DOUT)
    GPIO 16│  IO16  IO5  │ ← GPIO 5
    GPIO 15│  IO15  IO6  │ ← GPIO 6
    GPIO 14│  IO14  IO7  │ ← GPIO 7
    GPIO 13│  IO13  IO8  │ ← GPIO 8
    GPIO 12│  IO12  IO9  │ ← GPIO 9 (BOOT)
    GPIO 11│  IO11  IO10 │ ← GPIO 10
    GPIO 0 │  IO0   GND  │ ← GPIO 0
          └─────────────┘
```

## 🔧 단계별 조립 가이드

### 1단계: ESP32C3 설정
```
1. ESP32C3보드를 Arduino IDE에 연결
2. 보드타입: "ESP32C3 Dev Module" 설정
3. 포트: 연결된 COM 포트 선택
4. 기본 연결 테스트 완료 확인
```

### 2단계: I2S DAC 연결 (Audio.h 버전)
```
1. ESP32C3의 3.3V → PCM5102 VCC 연결
2. ESP32C3의 GND → PCM5102 GND 연결  
3. GPIO 2 → PCM5102 LCK (WS) 연결
4. GPIO 3 → PCM5102 BCK (BCLK) 연결
5. GPIO 4 → PCM5102 DIN (DATA) 연결
```

### 3단계: 오디오 앰프 연결
```
1. PCM5102 LOut → PAM8403 IN1 연결
2. PCM5102 ROut → PAM8403 IN2 연결
3. 5V → PAM8403 VCC 연결
4. PAM8403 OUT+ → 좌측 스피커 +
5. PAM8403 OUT- → 좌측 스피커 -
6. 위와 동일하게 우측 스피커 연결
```

### 4단계: PWM 앰프 연결 (I2S+PWM 버전)
```
1. ESP32C3 GPIO 18 → PAM8403 IN+ 연결
2. ESP32C3 GND → PAM8403 IN- 연결
3. 5V → PAM8403 VCC 연결
4. PAM8403 OUT+ → 스피커 + 연결
5. PAM8403 OUT- → 스피커 - 연결
```

## ⚡ 전원 관리

### USB 전원 (개발용)
```
USB-C 케이블 → ESP32C3 → I2S DAC/PAM8403 → 스피커
장점: 간단한 구성, USB 디버깅 가능
단점: USB 전원만 사용 가능
```

### 외부 전원 (실사용)
```
3.7V Li-ion 배터리 → ESP32C3 + PAM8403
                 ↓
               스피커

또는

9V 건전지 → 5V 레귤레이터 → ESP32C3 + PAM8403
```

## 🛡️ 주의사항 및 안전수칙

###电气안전
```
✓ 모든 연결 전에 전원 차단
✓ +/- 극성 확인 후 연결
✓ 단락 방지 고려
✓ 권장 전압 범위 준수
```

### 전압 제한사항

| 구성요소 | 권장 전압 | 최대 전압 | 주의사항 |
|----------|-----------|-----------|----------|
| ESP32C3 | 3.3V | 3.6V | 5V 입력 불가 |
| PCM5102 | 3.3V | 3.6V | 3.3V فقط |
| PAM8403 | 3-5V | 5.5V | 5V 안전 |
| LM386 | 5-12V | 18V | 저전력 |

## 🔍 연결 확인 방법

### 1단계: 기본 연결 확인
```
1. ESP32C3를 USB로 PC에 연결
2. Arduino IDE에서 Serial Monitor 열기
3. 'test' 명령어로 간단한 오디오 테스트
4. 스피커에서 소리 확인
```

### 2단계: I2S 연결 확인 (Audio.h 버전)
```
1. Audio.h 버전 스케치 업로드
2. Serial Monitor에서 I2S 초기화 메시지 확인
3. 'kick' 명령어로 Kick 드럼 테스트
4. I2S DAC LED 확인 (PCM5102 DAC LED)
```

### 3단계: PWM 연결 확인 (PWM 버전)
```
1. PWM 버전 스케치 업로드
2. GPIO 18에서 PWM 신호 측정 (오실로스코프)
3. 'volume 1.0' 명령어로 최대 볼륨 테스트
4. 스피커에서 음성 확인
```

## 🏠 고성능 구성 (프로젝트용)

### 멀티채널 시스템
```
ESP32C3 × 2      I2S DAC × 2      앰프 × 4
     ↓                ↓                ↓
  패턴성능            품질향상         스테레오
```

### 효율적 시스템
```
ESP32C3      I2S DAC          다채널 앰프
  ↓            ↓                  ↓
최적화      고품질           다중출력
```

## 📊 성능 비교 및 권장사항

| 구성 | 품질 | 비용 | 난이도 | 사용처 |
|------|------|------|--------|---------|
| I2S DAC + PAM8403 | 최상 | $7-16 | 중간 | 음악 제작 |
| PWM + PAM8403 | 중상 | $6-12 | 쉬움 | 실험/프로토타입 |
| I2S DAC + LM386 | 중 | $5-10 | 어려움 | 저전력 |
| PWM + LM386 | 중하 | $4-8 | 쉬움 | 교육용 |

## 🎯 최종 권장사항

**완전主义者:**
- I2S DAC + PAM8403 + 고품질 스피커
- Audio.h 버전 사용
- ESP32C3 DevKit + 케이스

**실험가/프로토타입:**
- PWM + PAM8403 + 기본 스피커  
- I2S.h PWM 버전 사용
- XIAO ESP32C3 + 브레드보드

**교육용:**
- PWM + LM386 + 소형 스피커
- 저비용 구성
- 자가 조립 kits