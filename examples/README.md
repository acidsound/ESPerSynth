# ESPerSynth examples 폴더 사용 가이드

## 📁 examples 폴더 구조

```
examples/
├── 01_Basic_Usage/           # 기본 사용법
├── 02_Performance/           # 성능 최적화 예제  
├── 03_Advanced_Features/     # 고급 기능
├── Audio_ESPerSynth/      # Audio.h 라이브러리 버전 ⭐
├── PWM_ESPerSynth/        # I2S.h PWM 버전
└── Mozzi_ESPerSynth/      # Mozzi Library 버전
```

## 🎯 권장 순서

### 1단계: 하드웨어 연결 확인
```
1. Arduino IDE에서 Audio_ESPerSynth 예제 사용
2. I2S DAC 연결 후 Audio.h 버전 테스트
3. Serial Monitor에서 'help' 명령어 확인
```

### 2단계: 다양한 구현 방식 테스트
```
1. Audio_ESPerSynth - 안정적인 I2S 출력
2. PWM_ESPerSynth - 저메모리 PWM 출력  
3. Mozzi_ESPerSynth - Mozzi 고급 기능
```

### 3단계: 필요에 따라 최적화
```
1. 성능이 중요하면 I2S DAC + Audio.h 사용
2. 메모리가 부족하면 PWM 버전 사용
3. 개발이 중요하면 Mozzi Library 사용
```

## 📋 각 예제별 특징

### Audio_ESPerSynth (권장)
- **라이브러리**: Audio.h (ESP8266Audio)
- **품질**: 최고
- **메모리**: 80KB
- **추천**: 안정적인 프로젝트용

### PWM_ESPerSynth (실험용)
- **라이브러리**: I2S.h + PWM
- **품질**: 중간
- **메모리**: 40KB  
- **추천**: 메모리 제약이 있는 경우

### Mozzi_ESPerSynth (개발용)
- **라이브러리**: Mozzi Library
- **품질**: 高
- **메모리**: 60KB
- **추천**: 고급 기능 개발

## ⚠️ Arduino IDE 빌드 오류 해결

### I2S.h: No such file or directory 오류 발생 시:

1. **Audio.h 버전 사용** (권장)
   ```
   examples/Audio_ESPerSynth/ESPerSynth.ino
   ```

2. **ESP32 Arduino 코어 2.0.18 설치**
   ```
   Tools → Board → Boards Manager
   ESP32 by Espressif Systems → 2.0.18 설치
   ```

3. **필수 라이브러리 설치**
   ```
   Tools → Manage Libraries
   ESP8266Audio 1.9.5 설치
   Mozzi (최신)
   ESP32_C3_TimerInterrupt (최신)
   ```

## 🎵 실행 순서

### 1단계: 하드웨어 연결
```
Audio.h 버전:
- ESP32C3 → I2S DAC (GPIO 2,3,4) → 앰프 → 스피커

PWM 버전:
- ESP32C3 → PWM 앰프 (GPIO 18) → 스피커
```

### 2단계: Arduino IDE 설정
```
Board: ESP32C3 Dev Module
Port: 연결된 COM 포트
Flash Size: 4MB
```

### 3단계: 업로드 및 테스트
```
1. 예제 업로드
2. Serial Monitor 열기 (115200)
3. 'help' 명령어 입력
4. 'kick', 'snare' 테스트
```

## 📞 추가 지원

문제가 지속되면:
1. 하드웨어 연결 재검토
2. Arduino IDE 로그 확인
3. 라이브러리 버전 확인
4. ESP32 코어 재설치