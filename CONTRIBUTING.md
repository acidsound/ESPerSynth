# Contributing to ESPerSynth - TR-808 Drum Machine Library

먼저 이 프로젝트에 기여해 주셔서 감사합니다! 以下는 개발 환경 설정 및 코드 기여 가이드라인입니다.

## 🛠️ 개발 환경 설정

### 필수 요구사항
- Arduino IDE 2.x 이상
- ESP32C3 브드 지원 패키지
- Git
- 기본 C/C++ 개발 지식

### 환경 설정 단계

1. **Arduino IDE 설정**
   ```
   Arduino IDE → File → Preferences
   Additional Board Manager URLs에 추가:
   https://dl.espressif.com/dl/package_esp32_index.json
   ```

2. **ESP32C3 브드 설치**
   ```
   Tools → Board → Boards Manager
   "ESP32" 검색 → ESP32 by Espressif Systems 설치
   ```

3. **라이브러리 개발 환경**
   ```bash
   cd ~/Documents/Arduino/libraries/
   git clone https://github.com/your-username/esp32c3-tr808-library.git ESPerSynth
   ```

## 📝 코드 스타일 가이드

### Arduino Coding Standard

#### 기본 원칙
- **가독성 우선**: 명확하고 읽기 쉬운 코드 작성
- **일관성 유지**: 전체 프로젝트에서 동일한 스타일 사용
- **주석 필수**: 복잡한 로직과 알고리즘 설명
- **유닛 테스트**: 가능한 한 모든 새 기능 테스트

#### 네이밍 규칙
```cpp
// 클래스명: PascalCase
class TR808DrumMachine {
    // 함수명: camelCase
    void triggerKick();
    void setMasterVolume(float volume);
    
    // 변수명: camelCase
    float masterVolume;
    bool isActive;
    
    // 상수: UPPER_SNAKE_CASE
    static const int MAX_SAMPLES = 256;
    const float DEFAULT_VOLUME = 0.8f;
};
```

#### 코드 포맷팅
```cpp
// 들여쓰기: 4 스페이스 (탭 사용 금지)
// 중괄호 스타일: K&R 스타일
if (condition) {
    // 코드
} else {
    // 코드
}

// 함수 선언 스타일
ReturnType FunctionName(
    ParameterType param1,
    ParameterType param2
) {
    // 구현
}

// 포인터 및 참조
void processSample(const int16_t* samples, int count);
void processSample(int16_t* samples, int count);
```

#### 주석 스타일
```cpp
/**
 * 함수 설명 (JNI 스타일)
 * 
 * @param param1 설명
 * @param param2 설명
 * @return 반환값 설명
 */
ReturnType functionName(Type param1, Type param2) {
    // 구현
}

// 한 줄 주석
// 이 함수는 ...을 수행합니다.

/* 
 * 블록 주석 (필요시 사용)
 */
```

## 🧪 테스트 가이드라인

### 하드웨어 테스트
모든 새로운 기능은 실제 ESP32C3 보드에서 테스트해야 합니다.

#### 기본 테스트 체크리스트
```cpp
// 1. 오디오 품질 테스트
void testAudioQuality() {
    // 진폭, 주파수 응답, 지연시간 테스트
}

// 2. 성능 테스트
void testPerformance() {
    // CPU 사용률, 메모리 사용량, 지연시간 측정
}

// 3. 안정성 테스트
void testStability() {
    // 장시간 동작, 동시 발화, 에러 복구 테스트
}
```

#### 성능 벤치마크
- **샘플레이트 정확도**: 99% 이상
- **CPU 사용률**: 40% 이하 (모든 드럼 동시 발화)
- **메모리 사용량**: 80% 이하
- **오디오 지연**: 10ms 이하

### 코드 테스트
```cpp
// 유닛 테스트 예제
void testDrumTrigger() {
    TR808DrumMachine drum;
    
    // 테스트 1: 정상 트리거
    drum.triggerKick(1.0f);
    float sample = drum.process();
    assert(sample > 0.0f);
    
    // 테스트 2: 벨로시티 범위 테스트
    drum.triggerKick(0.0f);  // 무음
    drum.triggerKick(1.0f);  // 최대 볼륨
    
    // 테스트 3: 범위 밖 값 테스트
    drum.triggerKick(-0.5f); // 0으로 클램프
    drum.triggerKick(1.5f);  // 1.0으로 클램프
}
```

## 📂 프로젝트 구조

```
ESPerSynth/
├── src/                    # 메인 소스 코드
│   ├── ESPerSynth.ino  # 메인 Arduino 스케치
│   ├── tr808_drums.h      # 헤더 파일
│   ├── tr808_drums.cpp    # 구현 파일
│   └── arduino_tr808_config.h # 설정 파일
├── examples/              # 예제 코드
│   ├── 01_Basic_Usage/
│   ├── 02_Performance/
│   └── 03_Advanced_Features/
├── docs/                  # 문서
├── extras/               # 추가 파일
├── library.properties    # 라이브러리 메타데이터
├── keywords.txt          # IDE 키워드 highlighting
└── README.md             # 프로젝트 문서
```

## 🚀 기여 프로세스

### 1. Fork 및 브랜치 생성
```bash
git clone https://github.com/your-username/esp32c3-tr808-library.git
cd esp32c3-tr808-library
git checkout -b feature/amazing-feature
```

### 2. 개발 및 커밋
```bash
# 개발 진행
git add .
git commit -m "feat: amazing new feature

- Add new drum synthesis algorithm
- Optimize memory usage by 20%
- Add comprehensive tests"
```

### 3. Pull Request
1. GitHub에서 Pull Request 생성
2. PR 템플릿 작성
3. 코드 리뷰 대기
4. CI/CD 확인
5. 머지 승인

## 📋 PR 템플릿

```markdown
## 기능 설명
새로운 기능에 대한 명확하고 간결한 설명

## 변경사항
- 추가된 기능 1
- 추가된 기능 2
- 수정된 버그

## 테스트
- [ ] 하드웨어 테스트 완료
- [ ] 성능 테스트 완료  
- [ ] 기존 기능 영향 없음
- [ ] 새 기능 테스트 추가

## 스크린샷 (해당시)
기능 데모 이미지나 오실로스코프 화면

## 체크리스트
- [ ] 코드 스타일 가이드 준수
- [ ] 주석 추가 완료
- [ ] 성능 영향 없음
- [ ] 문서 업데이트
```

## 🔧 디버깅 가이드

### Serial 디버깅
```cpp
#ifdef DEBUG_MODE
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif
```

### 성능 분석
```cpp
void profileFunction() {
    unsigned long start = micros();
    
    // 프로파일링할 함수
    drumMachine.process();
    
    unsigned long end = micros();
    unsigned long duration = end - start;
    
    Serial.println("Function took: " + String(duration) + " μs");
}
```

## 📚 문서 가이드라인

### API 문서
```cpp
/**
 * TR-808 베이스 드럼 (킥) 트리거
 * 
 * 이 함수는 Roland TR-808의 전설적인 베이스 드럼 사운드를 생성합니다.
 * 브리지드 T 발진기와 서브 오실레이터를 조합하여 실제 TR-808의
 * 음색 특성을 재현합니다.
 * 
 * @param velocity 벨로시티 (0.0-1.0). 0.0은 무음, 1.0은 최대 볼륨
 * @return void
 * 
 * @note 벨로시티가 0인 경우 소리 없이 트리거만 됩니다
 * @warning 1.0을 초과하는 벨로시티는 1.0으로 클램프됩니다
 * 
 * @see setKickDecay, setKickTone, triggerSnare
 * 
 * @example
 * @code
 * // 기본 사용법
 * drumMachine.triggerKick(1.0f);
 * 
 * // 다양한 벨로시티
 * drumMachine.triggerKick(0.3f);  // 소프트
 * drumMachine.triggerKick(0.8f);  // 하드
 * drumMachine.triggerKick(0.0f);  // 무음
 * @endcode
 */
void TR808DrumMachine::triggerKick(float velocity) {
    // 구현
}
```

### README 업데이트
새로운 기능을 추가할 때는 README.md도 함께 업데이트해주세요.

## 🐛 버그 리포트

### 버그 리포트 템플릿
```markdown
## 버그 설명
버그에 대한 명확하고 간결한 설명

## 재현 단계
1. '...'로 이동
2. '...' 클릭
3. '...'까지 스크롤
4. 오류 확인

## 예상 동작
기대하는 정상 동작에 대한 설명

## 실제 동작
실제로 발생한 문제에 대한 설명

## 스크린샷
문제를 보여주는 스크린샷 (해당시)

## 환경 정보
- 보드: ESP32C3
- Arduino IDE 버전: x.x.x
- 라이브러리 버전: x.x.x
- OS: [Windows/Mac/Linux]

## 추가 정보
문제를 이해하는 데 도움이 되는 추가 정보
```

## 📋 Issue 가이드라인

### Feature Request
```markdown
## 기능 제안
원하는 기능에 대한 간결한 설명

## 기존과의 차이점
기존 라이브러리에 없는 새로운 가치를 제시

## 구현 아이디어
기능을 구현하는 방법에 대한 아이디어 (선택사항)

## 대안
동일한 목적을 달성하는 다른 방법들 (선택사항)
```

## 🎯 개발 우선순위

### 높은 우선순위
1. 성능 최적화
2. 안정성 개선
3. 버그 수정
4. 새로운 드럼 소스 추가

### 중간 우선순위
1. 사용자 인터페이스 개선
2. 시퀀서 기능 확장
3. MIDI 지원 강화
4. 문서 및 예제 추가

### 낮은 우선순위
1. 추가 효과 (리버브, 딜레이 등)
2. 다른 보드 지원
3. 모바일 앱 연동
4. 웹 인터페이스

## 💡 코드 리뷰 가이드라인

### 리뷰어 체크리스트
- [ ] 코드 스타일 가이드 준수
- [ ] 성능 영향 없음
- [ ] 메모리 사용량 적절
- [ ] 오류 처리 적절
- [ ] 테스트 커버리지 적절
- [ ] 문서 업데이트 완료
- [ ] 하드웨어 호환성 확인

### 리뷰어 팁
- 긍정적이고 건설적인 피드백
- 구체적이고 실행 가능한 제안
- 교육적인 설명 제공
- 구현 방법보다 결과에 집중

## 🏆 기여자 가이드

### 새로운 기여자
1. `good first issue` 라벨이 있는 이슈부터 시작
2. 문서나 번역 기여로 시작
3. 작은 버그 수정부터 시도

### 숙련된 기여자
1. 큰 기능 구현 주도
2. 코드 리뷰 진행
3. 다른 기여자 멘토링
4. 프로젝트 방향성 논의

## 📞 지원

### 질문 및 논의
- GitHub Discussions: 일반적인 질문
- GitHub Issues: 버그 리포트 및 기능 요청
- Email: maintainers@tr808-esp32c3.com

### 개발팀 연락처
- Project Lead: lead@tr808-esp32c3.com
- Audio Engineer: audio@tr808-esp32c3.com
- Hardware Expert: hardware@tr808-esp32c3.com

---

**개발에 참여해 주셔서 다시 한번 감사합니다! 🎵**

이 프로젝트는 전 세계 개발자들의 기여로 만들어집니다. 
함께素晴らしい TR-808 드럼 머신 라이브러리를 만들어봅시다!