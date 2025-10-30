/*
 * Mozzi TR-808 드럼 머신 메인 예제
 * 
 * ESP32C3 + Mozzi Library + TR-808 드럼 머신 완전 통합 예제
 * 18개 드럼 소스, 실시간 패턴 재생, 성능 모니터링 지원
 * 
 * 작성일: 2025-10-30
 * 버전: 1.0.0
 * 
 * 연결 방법:
 * - GPIO 18: PWM 오디오 출력 (외부 DAC/오디오 앰프 연결)
 * - Serial: 115200 baud (명령어 입력/상태 모니터링)
 * 
 * 명령어:
 * - kick, snare, cymbal, hihat, tom, conga, rimshot, maracas, clap, cowbell
 * - volume <0.0-1.0> - 마스터 볼륨 설정
 * - pattern_demo - 데모 패턴 재생
 * - pattern_stop - 패턴 중지
 * - status - 시스템 상태 및 성능 통계
 * - list - 지원되는 드럼 목록
 * - help - 도움말 표시
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <mozzi_midi.h>
#include <EventDelay.h>
#include <Metronome.h>
#include <Line.h>

// TR-808 드럼 머신 헤더
#include "../src/tr808_drums.h"
#include "../src/mozzi_tr808_config.h"
#include "../src/esp32c3_mozzi_integration.h"

// =============================================================================
// 전역 인스턴스 생성
// =============================================================================

// TR-808 드럼 머신 메인 인스턴스
TR808DrumMachineMozzi tr808Mozzi;

// ESP32C3 Mozzi 시스템 인스턴스
ESP32C3Mozzi mozziSystem;

// 메트로놈 인스턴스 (패턴 재생용)
Metronome beat(120);  // 120 BPM 기본값

// 상태 표시용 EventDelay
EventDelay statusDelay(1000);  // 1초마다 상태 업데이트

// =============================================================================
// 설정 상수
// =============================================================================

// 시스템 설정
const uint32_t SERIAL_BAUD = 115200;
const char* VERSION = "Mozzi TR-808 ESP32C3 v1.0.0";

// 도움말 텍스트
const char* HELP_TEXT = R"(
🎵 Mozzi TR-808 드럼 머신 ESP32C3
=================================

📢 기본 드럼 명령어:
- kick          : 베이스 드럼 재생
- snare         : 스네어 드럼 재생  
- cymbal        : 심벌 재생
- hihat         : 하이햇 재생
- tom           : 톰 드럼 재생
- conga         : 콩가 재생
- rimshot       : 림샷 재생
- maracas       : 마라카스 재생
- clap          : 클랩 재생
- cowbell       : 카우벨 재생

🎚️ 볼륨 제어:
- volume <0.0-1.0>  : 마스터 볼륨 설정 (예: volume 0.7)

🎼 패턴 제어:
- pattern_demo   : 데모 패턴 재생
- pattern_stop   : 패턴 중지

📊 상태 정보:
- status         : 시스템 상태 및 성능 통계
- list           : 지원되는 드럼 목록
- help           : 도움말 표시

🔧 시스템:
- test           : 오디오 출력 테스트
- reset          : 시스템 리셋

)" ;

// =============================================================================
// setup() 함수 - 시스템 초기화
// =============================================================================

void setup() {
    // Serial 통신 초기화
    Serial.begin(SERIAL_BAUD);
    while (!Serial && millis() < 3000) {
        ; // Serial 연결 대기 (최대 3초)
    }
    
    Serial.println(F("================================================"));
    Serial.println(F("🎵 Mozzi TR-808 드럼 머신 ESP32C3 시작 중..."));
    Serial.println(F("================================================"));
    
    // ESP32C3 Mozzi 시스템 초기화
    Serial.println(F("📡 ESP32C3 Mozzi 시스템 초기화..."));
    if (!mozziSystem.initialize()) {
        Serial.println(F("❌ ESP32C3 Mozzi 시스템 초기화 실패!"));
        while (true) {
            delay(1000); // 시스템 정지
        }
    }
    Serial.println(F("✅ ESP32C3 Mozzi 시스템 초기화 완료"));
    
    // TR-808 드럼 머신 초기화
    Serial.println(F("🥁 TR-808 드럼 머신 초기화..."));
    if (!tr808Mozzi.initialize()) {
        Serial.println(F("❌ TR-808 드럼 머신 초기화 실패!"));
        while (true) {
            delay(1000); // 시스템 정지
        }
    }
    Serial.println(F("✅ TR-808 드럼 머신 초기화 완료"));
    
    // 성능 모니터링 초기화
    Serial.println(F("📊 성능 모니터링 시작..."));
    if (!tr808Mozzi.initializePerformanceMonitoring()) {
        Serial.println(F("⚠️ 성능 모니터링 초기화 경고"));
    } else {
        Serial.println(F("✅ 성능 모니터링 시작"));
    }
    
    // Mozzi 오디오 시스템 시작
    Serial.println(F("🔊 Mozzi 오디오 시스템 시작..."));
    startMozzi(); // startMozgi() 함수 호출 (esp32c3_mozzi_integration.h에서 정의)
    
    Serial.println(F("✅ Mozzi 오디오 시스템 시작"));
    
    // 오디오 출력 테스트
    Serial.println(F("🔊 오디오 출력 테스트..."));
    runAudioTest();
    
    // 시스템 상태 출력
    Serial.println(F("\n📋 시스템 초기화 완료!"));
    printSystemInfo();
    
    Serial.println(F("\n🚀 Mozzi TR-808 드럼 머신이 준비되었습니다!"));
    Serial.println(F("💡 'help' 명령어로 사용법 확인"));
    Serial.println(F("================================================\n"));
    
    // 상태 딜레이 초기화
    statusDelay.start();
}

// =============================================================================
// loop() 함수 - 메인 루프
// =============================================================================

void loop() {
    // Mozzi 오디오 처리 (필수!)
    audioHook();
    
    // TR-808 드럼 머신 제어 업데이트
    tr808Mozzi.updateControl();
    
    // Serial 명령어 처리
    if (Serial.available()) {
        processSerialInput();
    }
    
    // 주기적 상태 출력 (10초마다)
    if (statusDelay.ready()) {
        static uint32_t statusCounter = 0;
        statusCounter++;
        
        if (statusCounter % 10 == 0) { // 10초마다
            tr808Mozzi.printSystemStatus();
        }
        
        statusDelay.start();
    }
    
    // 성능 최적화를 위한 짧은 지연
    yield(); // ESP32C3 के लिए 중요한 함수
}

// =============================================================================
// Mozzi 필수 함수들 (extern "C"로 선언하여 Mozzi 호환성 보장)
// =============================================================================

extern "C" {
    /**
     * Mozzi 오디오 훅 - 오디오 샘플 처리
     * 이 함수는 Mozzi에서 자동으로 호출됩니다
     */
    void audioHook() {
        // TR-808 드럼 머신에서 오디오 샘플 생성
        int16_t sample = tr808Mozzi.updateAudio();
        
        // 오디오 출력
        audioWrite(sample);
    }
    
    /**
     * Mozzi 제어 업데이트 - 드럼 트리거 및 제어 처리
     * 이 함수는 Mozzi에서 자동으로 호출됩니다
     */
    void updateControl() {
        // TR-808 드럼 머신 제어 업데이트
        tr808Mozzi.updateControl();
    }
}

// =============================================================================
// Serial 명령 처리 함수들
// =============================================================================

/**
 * Serial 입력 처리
 */
void processSerialInput() {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();
    
    if (input.length() == 0) return;
    
    Serial.print(F("🔹 수신된 명령어: "));
    Serial.println(input);
    
    // TR-808 드럼 머신에서 명령어 처리
    bool handled = tr808Mozzi.processSerialCommand(input);
    
    if (!handled) {
        Serial.println(F("❓ 알 수 없는 명령어입니다. 'help'를 입력하여 도움말을 확인하세요."));
    }
}

/**
 * 오디오 출력 테스트
 */
void runAudioTest() {
    Serial.println(F("🔊 오디오 출력 테스트 시작 (3초)"));
    
    // 각 드럼을 짧게 재생하여 오디오 시스템 확인
    const char* testDrums[] = {
        "kick", "snare", "hihat", "tom", "conga"
    };
    
    for (int i = 0; i < 5; i++) {
        Serial.print(F("  ▶️ "));
        Serial.print(testDrums[i]);
        Serial.println(F(" 드럼 테스트"));
        
        tr808Mozzi.triggerDrum(testDrums[i], 0.5f);
        delay(500);
    }
    
    Serial.println(F("✅ 오디오 출력 테스트 완료"));
}

/**
 * 시스템 정보 출력
 */
void printSystemInfo() {
    Serial.println(F("\n📋 === 시스템 정보 ==="));
    Serial.print(F("📱 버전: "));
    Serial.println(VERSION);
    Serial.print(F("🖥️  플랫폼: ESP32C3 (RISC-V)"));
    Serial.print(F(" 🎵 오디오 레이트: "));
    Serial.print(MOZZI_AUDIO_RATE);
    Serial.println(F(" Hz"));
    Serial.print(F("🎛️  마스터 볼륨: "));
    Serial.print(tr808Mozzi.getMasterVolume());
    Serial.println(F("/ 1.0"));
    Serial.print(F("🥁 지원 드럼 소스: "));
    Serial.print(TR808_NUM_SOURCES);
    Serial.println(F("개"));
    Serial.print(F("📊 Serial 레이트: "));
    Serial.print(SERIAL_BAUD);
    Serial.println(F(" bps"));
}

// =============================================================================
// 추가 유틸리티 함수들
// =============================================================================

/**
 * 현재 시간 표시 (디버그용)
 */
uint32_t getCurrentTimeMs() {
    return millis();
}

/**
 * 메모리 사용량 확인
 */
uint32_t getFreeHeapSize() {
    return ESP.getFreeHeap();
}

/**
 * CPU 사용률 계산 (간단한 방법)
 */
float calculateCPUUsage() {
    static uint32_t lastIdleTime = 0;
    static uint32_t lastTime = 0;
    
    uint32_t currentTime = millis();
    uint32_t idleTime = ESP.getFreeHeap(); // 간단한 지표
    
    if (lastTime > 0) {
        uint32_t timeDiff = currentTime - lastTime;
        uint32_t idleDiff = idleTime - lastIdleTime;
        
        // 간단한 CPU 사용률 계산 (역수)
        float cpuUsage = 100.0f - ((float)idleDiff / timeDiff * 100.0f);
        cpuUsage = constrain(cpuUsage, 0.0f, 100.0f);
        
        lastTime = currentTime;
        lastIdleTime = idleTime;
        
        return cpuUsage;
    }
    
    lastTime = currentTime;
    lastIdleTime = idleTime;
    
    return 0.0f;
}

/**
 * 시스템 헬스 체크
 */
bool checkSystemHealth() {
    bool healthy = true;
    
    // 메모리 확인
    uint32_t freeHeap = getFreeHeapSize();
    if (freeHeap < 10000) { // 10KB 미만
        Serial.println(F("⚠️ 메모리 부족 경고"));
        healthy = false;
    }
    
    // Mozzi 시스템 상태 확인
    if (!mozziSystem.isAudioActive()) {
        Serial.println(F("⚠️ 오디오 시스템 비활성"));
        healthy = false;
    }
    
    // TR-808 시스템 상태 확인
    if (!tr808Mozzi.isInitialized()) {
        Serial.println(F("⚠️ TR-808 시스템 초기화되지 않음"));
        healthy = false;
    }
    
    return healthy;
}

// =============================================================================
// Arduino Mozzi 호환성 함수들
// =============================================================================

// Mozzi의 startMozgi() 함수를 위한 간단한 래퍼
// 이 함수는 esp32c3_mozzi_integration.h에 정의되어 있지만,
// 여기서 명확히 호출하기 위한 래퍼 함수

void startMozgi() {
    Serial.println(F("🔄 Mozzi 오디오 시스템 시작 중..."));
    
    // ESP32C3 Mozzi 시스템 시작
    if (!mozziSystem.startAudio()) {
        Serial.println(F("❌ Mozzi 오디오 시스템 시작 실패!"));
        return;
    }
    
    Serial.println(F("✅ Mozzi 오디오 시스템 시작 완료"));
}

// =============================================================================
// 주의사항 및 메모
// =============================================================================

/*
 * 이 예제는 다음과 같은 라이브러리들을 사용합니다:
 * - Mozzi Library (오디오 처리)
 * - ESP32_C3_TimerInterrupt (타이머 인터럽트)
 * - ESP32 Core (ESP32C3 하드웨어 지원)
 * 
 * 설치 필요:
 * 1. Arduino IDE에서 ESP32C3 보드 패키지 설치
 * 2. Mozzi Library 설치
 * 3. ESP32_C3_TimerInterrupt 라이브러리 설치
 * 
 * 하드웨어 연결:
 * - GPIO 18: PWM 오디오 출력 (외부 DAC/오디오 앰프 연결)
 * - Serial: 115200 baud (명령어 입력/상태 모니터링)
 * 
 * 성능 최적화:
 * - 32.768kHz AudioRate (최고 품질)
 * - IRAM 사용 (ISR 성능 향상)
 * - RISC-V 아키텍처 최적화
 * - 메모리 풀 관리 사용
 * 
 * 문제 해결:
 * - Serial 통신 문제: Boud rate 확인
 * - 오디오 출력 문제: GPIO 연결 확인
 * - 성능 문제: 볼륨 감소 또는 드럼 소스 제한
 * - 메모리 부족: Serial 출력 비활성화
 */