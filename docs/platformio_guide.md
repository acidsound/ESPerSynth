# ESPerSynth PlatformIO 개발 가이드

## 목차
1. [PlatformIO 소개](#platformio-소개)
2. [설치 방법](#설치-방법)
3. [프로젝트 설정](#프로젝트-설정)
4. [빌드 및 업로드](#빌드-및-업로드)
5. [예제별 설정](#예제별-설정)
6. [문제 해결](#문제-해결)

---

## PlatformIO 소개

PlatformIO는 Arduino IDE의 강력한 대안으로, 다음과 같은 장점을 제공합니다:

### Arduino IDE 대비 장점
- **더 빠른 빌드 속도**: 증분 컴파일과 캐싱으로 빌드 시간 단축
- **강력한 라이브러리 관리**: 자동 의존성 해결 및 버전 관리
- **멀티 보드 지원**: 하나의 프로젝트에서 여러 보드 타겟 설정 가능
- **고급 디버깅**: 브레이크포인트, 변수 검사 등 전문적인 디버깅 도구
- **통합 개발 환경**: VS Code와 완벽한 통합
- **빌드 플래그 관리**: 컴파일 옵션을 세밀하게 제어
- **유닛 테스트 지원**: 자동화된 테스트 환경

### ESPerSynth 프로젝트에 최적
- ESP32C3의 복잡한 빌드 옵션을 `platformio.ini`로 간편하게 관리
- 여러 오디오 라이브러리(Audio.h, Mozzi, PWM) 간 쉬운 전환
- 최적화 플래그 및 메모리 설정을 프로파일별로 관리

---

## 설치 방법

### 1. Visual Studio Code 설치
```bash
# Ubuntu/Debian
sudo snap install code --classic

# macOS
brew install --cask visual-studio-code

# Windows
# https://code.visualstudio.com/ 에서 다운로드
```

### 2. PlatformIO IDE 확장 설치
1. VS Code 실행
2. 왼쪽 사이드바에서 Extensions 아이콘 클릭 (또는 `Ctrl+Shift+X`)
3. "PlatformIO IDE" 검색
4. "Install" 클릭
5. VS Code 재시작

### 3. 명령줄 도구 설치 (선택사항)
```bash
# Python pip를 통한 설치
pip install platformio

# 설치 확인
pio --version
```

---

## 프로젝트 설정

### 방법 1: 기존 예제 사용 (권장)

ESPerSynth는 각 예제별로 `platformio.ini` 파일을 제공합니다.

```bash
# 1. 저장소 클론
git clone https://github.com/acidsound/ESPerSynth.git
cd ESPerSynth

# 2. 원하는 예제 폴더로 이동
cd examples/Audio_EspherSynrh

# 3. VS Code로 열기
code .
```

VS Code에서 PlatformIO가 자동으로 프로젝트를 인식하고 필요한 라이브러리를 다운로드합니다.

### 방법 2: 새 프로젝트 생성

1. **VS Code에서 PlatformIO 홈 열기**
   - 아래 상태바에서 "PlatformIO" 아이콘 클릭
   - 또는 `Ctrl+Shift+P` → "PlatformIO: Home" 선택

2. **새 프로젝트 생성**
   - "New Project" 클릭
   - **Name**: `ESPerSynth_Project`
   - **Board**: `ESP32-C3-DevKitM-1` (또는 사용하는 ESP32C3 보드)
   - **Framework**: `Arduino`
   - "Finish" 클릭

3. **라이브러리 의존성 추가**
   
   `platformio.ini` 파일을 열고 다음 내용을 추가:

   ```ini
   [env:esp32-c3-devkitm-1]
   platform = espressif32
   board = esp32-c3-devkitm-1
   framework = arduino
   
   ; 라이브러리 의존성 (예제에 따라 선택)
   lib_deps = 
       https://github.com/acidsound/ESPerSynth.git
       earlephilhower/ESP8266Audio@^1.9.7
       sensorium/Mozzi@^2.0.0
   
   ; 빌드 플래그
   build_flags = 
       -DCORE_DEBUG_LEVEL=3
       -DBOARD_HAS_PSRAM
   
   ; 업로드 속도
   upload_speed = 921600
   monitor_speed = 115200
   ```

---

## 빌드 및 업로드

### VS Code UI 사용

1. **프로젝트 열기**
   - `File` → `Open Folder` → 예제 폴더 선택

2. **보드 연결**
   - ESP32C3 보드를 USB로 연결

3. **빌드 (컴파일)**
   - 하단 상태바에서 ✓ (체크) 아이콘 클릭
   - 또는 `Ctrl+Alt+B`

4. **업로드**
   - 하단 상태바에서 → (화살표) 아이콘 클릭
   - 또는 `Ctrl+Alt+U`

5. **시리얼 모니터**
   - 하단 상태바에서 🔌 (플러그) 아이콘 클릭
   - 또는 `Ctrl+Alt+S`

6. **빌드 + 업로드 + 모니터 (한번에)**
   - 하단 상태바에서 ⚡ 아이콘 클릭

### 명령줄 사용

```bash
# 프로젝트 디렉토리로 이동
cd examples/Audio_EspherSynrh

# 빌드만
pio run

# 빌드 + 업로드
pio run --target upload

# 업로드 + 시리얼 모니터
pio run --target upload && pio device monitor

# 클린 빌드
pio run --target clean
pio run
```

---

## 예제별 설정

### 1. Audio_EspherSynrh (Audio.h 라이브러리)

**특징**: 가장 안정적인 I2S 출력, 권장 방식

```ini
[env:esp32-c3-audio]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

lib_deps = 
    https://github.com/acidsound/ESPerSynth.git
    earlephilhower/ESP8266Audio@^1.9.7

build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DUSE_AUDIO_LIB

upload_speed = 921600
monitor_speed = 115200
```

### 2. PWM_EspherSynrh (PWM 방식)

**특징**: 최소 메모리 사용, 외부 라이브러리 불필요

```ini
[env:esp32-c3-pwm]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

lib_deps = 
    https://github.com/acidsound/ESPerSynth.git

build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DUSE_PWM_OUTPUT
    -Os  ; 크기 최적화

upload_speed = 921600
monitor_speed = 115200
```

### 3. Mozzi_EspherSynrh (Mozzi 라이브러리)

**특징**: 고급 사운드 합성 기능

```ini
[env:esp32-c3-mozzi]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino

lib_deps = 
    https://github.com/acidsound/ESPerSynth.git
    sensorium/Mozzi@^2.0.0

build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DMOZZI_AUDIO_RATE=32768
    -DUSE_MOZZI

upload_speed = 921600
monitor_speed = 115200
```

### 4. 멀티 환경 설정 (고급)

하나의 `platformio.ini`에서 여러 빌드 타겟을 관리:

```ini
; 공통 설정
[env]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
upload_speed = 921600
monitor_speed = 115200

; Audio.h 버전
[env:audio]
lib_deps = 
    https://github.com/acidsound/ESPerSynth.git
    earlephilhower/ESP8266Audio@^1.9.7
build_flags = -DUSE_AUDIO_LIB

; PWM 버전
[env:pwm]
lib_deps = 
    https://github.com/acidsound/ESPerSynth.git
build_flags = -DUSE_PWM_OUTPUT -Os

; Mozzi 버전
[env:mozzi]
lib_deps = 
    https://github.com/acidsound/ESPerSynth.git
    sensorium/Mozzi@^2.0.0
build_flags = -DMOZZI_AUDIO_RATE=32768 -DUSE_MOZZI
```

**빌드 방법**:
```bash
# 특정 환경 빌드
pio run -e audio
pio run -e pwm
pio run -e mozzi

# 특정 환경 업로드
pio run -e audio --target upload
```

---

## 고급 설정

### 최적화 플래그

```ini
build_flags = 
    ; 성능 최적화
    -O3
    -ffast-math
    
    ; 크기 최적화
    -Os
    -ffunction-sections
    -fdata-sections
    
    ; ESP32C3 특화
    -DBOARD_HAS_PSRAM
    -DCONFIG_ARDUHAL_LOG_DEFAULT_LEVEL_INFO
    
    ; TR-808 특화
    -DSAMPLE_RATE=44100
    -DBUFFER_SIZE=512
```

### 메모리 설정

```ini
board_build.flash_mode = qio
board_build.f_flash = 80000000L
board_build.f_cpu = 160000000L
board_build.partitions = huge_app.csv
```

### 커스텀 파티션 (선택사항)

큰 샘플 데이터를 위해 파티션 테이블 수정:

`partitions_custom.csv`:
```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x1E0000,
spiffs,   data, spiffs,  0x1F0000,0x110000,
```

`platformio.ini`에 추가:
```ini
board_build.partitions = partitions_custom.csv
```

---

## 문제 해결

### 1. 빌드 오류: "Platform espressif32 is not installed"

```bash
# 플랫폼 수동 설치
pio platform install espressif32
```

### 2. 업로드 오류: "No serial ports found"

**원인**: ESP32C3가 인식되지 않음

**해결**:
```bash
# Linux: 사용자 권한 추가
sudo usermod -a -G dialout $USER
# 로그아웃 후 재로그인

# 포트 확인
pio device list

# platformio.ini에 포트 명시
upload_port = /dev/ttyUSB0  ; Linux
upload_port = COM3          ; Windows
```

### 3. 빌드 오류: "Library not found"

```bash
# 라이브러리 캐시 정리
pio lib --global uninstall ESPerSynth
pio lib --global install https://github.com/acidsound/ESPerSynth.git

# 또는 프로젝트별 설치
pio lib install
```

### 4. 메모리 부족 오류

**증상**: `region 'iram0_0_seg' overflowed`

**해결**:
```ini
build_flags = 
    -Os  ; O3 대신 크기 최적화
    -DCORE_DEBUG_LEVEL=0  ; 디버그 로그 비활성화

board_build.partitions = huge_app.csv
```

### 5. Audio.h 관련 오류

```ini
; platformio.ini에 명시적으로 추가
lib_deps = 
    earlephilhower/ESP8266Audio@^1.9.7
    
build_flags = 
    -DAUDIO_INCLUDE_AUDIO_H
```

### 6. Mozzi 컴파일 오류

```ini
build_flags = 
    -DMOZZI_AUDIO_RATE=32768
    -DMOZZI_CONTROL_RATE=256
    -DMOZZI_ESP32
```

---

## 추가 리소스

### PlatformIO 공식 문서
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/index.html)
- [ESP32 Platform](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [Library Management](https://docs.platformio.org/en/latest/librarymanager/index.html)

### ESPerSynth 프로젝트
- [GitHub Repository](https://github.com/acidsound/ESPerSynth)
- [Arduino IDE 빌드 가이드](./esp32c3_tr808_build_guide.md)
- [하드웨어 연결 가이드](./hardware_connection_guide.md)

---

## 성능 비교

| 환경 | Arduino IDE | PlatformIO |
|------|-------------|------------|
| 빌드 시간 (초) | 45-60 | 15-25 |
| 라이브러리 관리 | 수동 | 자동 |
| 멀티 보드 지원 | ❌ | ✅ |
| 고급 디버깅 | ❌ | ✅ |
| 빌드 캐싱 | ❌ | ✅ |

---

## 마무리

PlatformIO는 특히 복잡한 프로젝트나 여러 타겟을 관리해야 할 때 Arduino IDE보다 훨씬 효율적입니다. ESPerSynth 프로젝트의 경우 Audio.h, PWM, Mozzi 세 가지 버전을 쉽게 전환하며 개발할 수 있습니다.

추가 질문이나 문제가 있다면 [GitHub Issues](https://github.com/acidsound/ESPerSynth/issues)에 등록해주세요.
