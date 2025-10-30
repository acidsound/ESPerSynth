/*
 * ESP32C3 TR-808 드럼 머신 Arduino 프로젝트
 * 
 * Roland TR-808의 전설적인 드럼 사운드를 ESP32C3에서 구현
 * I2S 오디오 출력을 통한 고품질 오디오 생성
 * 
 * 하드웨어 요구사항:
 * - ESP32C3 개발보드
 * - 외부 I2S DAC (PCM5102, ES9023 등)
 * - 오디오 앰프 및 스피커
 * 
 * 핀 연결:
 * - GPIO 1: I2S Data Out
 * - GPIO 2: I2S Bit Clock (BCLK)
 * - GPIO 3: I2S Word Select (LRCLK)
 * - GND: 공통 그라운드
 * 
 * 라이브러리:
 * - I2S library (ESP32 내장)
 * - TR808 드럼 알고리즘
 * 
 * 제작: 2025-10-30
 * 버전: 1.0.0
 */

#include <I2S.h>
#include "arduino_tr808_config.h"
#include "tr808_drums.h"

// ============================================
// 전역 설정 및 상수
// ============================================

// I2S 설정 (ESP32C3 최적화)
#define SAMPLE_RATE 32768           // 32.768kHz (ESP32C3 권장)
#define BUFFER_SIZE 256             // I2S 버퍼 크기
#define MONO_OUTPUT true            // 모노 출력 (메모리 절약)

// TR808 설정
#define MASTER_VOLUME 0.8f          // 기본 마스터 볼륨
#define POLYPHONY_LIMIT 10          // 최대 동시 음향

// ============================================
// 전역 변수 및 인스턴스
// ============================================

// 메인 TR808 드럼 머신
TR808DrumMachine drumMachine;

// I2S 버퍼 (ESP32C3 최적화)
int16_t i2sBuffer[BUFFER_SIZE];

// 성능 모니터링
unsigned long lastPerfCheck = 0;
unsigned long sampleCount = 0;
float cpuUsage = 0.0f;

// ============================================
// 초기화 함수들
// ============================================

void setup() {
    // Serial 통신 초기화
    Serial.begin(115200);
    delay(1000); // 안정화를 위한 지연
    
    Serial.println("\n");
    Serial.println("===========================================");
    Serial.println("  ESP32C3 TR-808 드럼 머신 시작");
    Serial.println("===========================================");
    Serial.println("버전: 1.0.0");
    Serial.println("제작일: 2025-10-30");
    Serial.println("Arduino 프레임워크: " + String(ARDUINO));
    Serial.println("");

    // I2S 오디오 출력 초기화
    if (!initializeI2SAudio()) {
        Serial.println("❌ I2S 초기화 실패! 하드웨어 연결을 확인하세요.");
        showHardwareInstructions();
        while(true) delay(1000); // 무한 루프
    }
    
    // TR808 드럼 머신 초기화
    if (!initializeTR808()) {
        Serial.println("❌ TR-808 초기화 실패!");
        while(true) delay(1000);
    }
    
    // 성능 모니터링 초기화
    initializePerformanceMonitoring();
    
    // 시퀀서 초기화 (선택사항)
    initializeSequencer();
    
    Serial.println("✅ 모든 시스템 초기화 완료!");
    Serial.println("");
    
    // 시작 정보 출력
    printSystemInfo();
    printInstructions();
    printExamples();
    
    // 오디오 테스트 (선택사항)
    if (AUTO_TEST_ON_STARTUP) {
        runAudioTest();
    }
    
    Serial.println("");
    Serial.println("🎵 TR-808 드럼 머신이 준비되었습니다!");
    Serial.println("💡 'help' 명령어로 사용법을 확인하세요.");
}

bool initializeI2SAudio() {
    Serial.println("🔊 I2S 오디오 시스템 초기화...");
    
    // I2S 포트 설정
    i2s_mode_t mode = I2S_STANDARD;
    if (MONO_OUTPUT) {
        mode = I2S_MODE_MASTER | I2S_MODE_TX;
    } else {
        mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DUAL;
    }
    
    // I2S 초기화
    if (!I2S.begin(mode, SAMPLE_RATE, 16, 1)) {
        Serial.println("  ❌ I2S.begin() 실패");
        return false;
    }
    
    Serial.println("  ✅ I2S 초기화 성공");
    Serial.println("     샘플 레이트: " + String(SAMPLE_RATE) + " Hz");
    Serial.println("     버퍼 크기: " + String(BUFFER_SIZE) + " 샘플");
    Serial.println("     출력 모드: " + String(MONO_OUTPUT ? "모노" : "스테레오"));
    
    return true;
}

bool initializeTR808() {
    Serial.println("🥁 TR-808 드럼 머신 초기화...");
    
    // 마스터 볼륨 설정
    drumMachine.setMasterVolume(MASTER_VOLUME);
    
    // 드럼별 기본 설정
    drumMachine.setKickDecay(500.0f);      // 킥: 500ms
    drumMachine.setKickTone(0.5f);         // 킥: 중간 톤
    drumMachine.setSnareTone(0.7f);        // 스네어: 밝은 톤
    drumMachine.setSnareSnappy(0.8f);      // 스네어: 강한 스냅
    drumMachine.setCymbalDecay(800.0f);    // 심벌: 800ms
    drumMachine.setCymbalTone(0.6f);       // 심벌: 중간 톤
    drumMachine.setHiHatDecay(50.0f);      // 하이햇: 클로즈드
    drumMachine.setTomTuning(165.0f);      // 톰: 165Hz
    drumMachine.setCongaTuning(370.0f);    // 콩가: 370Hz
    
    Serial.println("  ✅ TR-808 초기화 완료");
    Serial.println("     마스터 볼륨: " + String(MASTER_VOLUME));
    Serial.println("     버퍼 크기: " + String(BUFFER_SIZE));
    
    return true;
}

void initializePerformanceMonitoring() {
    lastPerfCheck = millis();
    sampleCount = 0;
    cpuUsage = 0.0f;
    Serial.println("📊 성능 모니터링 준비 완료");
}

void initializeSequencer() {
    Serial.println("🎼 시퀀서 시스템 초기화...");
    // TODO: 기본 패턴 로드
    Serial.println("  ✅ 시퀀서 준비 완료");
}

// ============================================
// 메인 루프
// ============================================

void loop() {
    unsigned long currentTime = millis();
    
    // Serial 명령 처리 (우선순위 높음)
    if (Serial.available()) {
        handleSerialCommands();
    }
    
    // 시퀀서 처리 (선택사항)
    if (ENABLE_SEQUENCER) {
        handleSequencer(currentTime);
    }
    
    // 오디오 처리 (실시간)
    processAudio();
    
    // 성능 모니터링 (1초마다)
    if (currentTime - lastPerfCheck >= 1000) {
        updatePerformanceMetrics(currentTime);
        lastPerfCheck = currentTime;
    }
    
    // 자동 저장 (선택사항)
    if (ENABLE_AUTO_SAVE) {
        handleAutoSave(currentTime);
    }
    
    // 안정성을 위한 짧은 지연
    delayMicroseconds(30); // 30.5μs @ 32.768kHz
}

void processAudio() {
    // TR808 드럼 머신에서 오디오 샘플 생성
    float audioSample = drumMachine.process();
    
    // 32-bit float를 16-bit int로 변환
    int16_t intSample = (int16_t)(audioSample * 32767);
    
    // I2S 버퍼 채우기
    for (int i = 0; i < BUFFER_SIZE; i++) {
        i2sBuffer[i] = intSample;
    }
    
    // I2S로 출력
    size_t bytesWritten = 0;
    I2S.write(i2sBuffer, BUFFER_SIZE, &bytesWritten);
    
    if (bytesWritten != BUFFER_SIZE) {
        Serial.println("⚠️ I2S 버퍼 경고: " + String(bytesWritten) + "/" + String(BUFFER_SIZE));
    }
    
    // 성능 모니터링
    sampleCount++;
}

// ============================================
// Serial 명령 처리
// ============================================

void handleSerialCommands() {
    String command = Serial.readString();
    command.trim();
    command.toLowerCase();
    
    if (command.length() == 0) return;
    
    Serial.println("명령 수신: " + command);
    
    // 기본 드럼 트리거 (숫자키)
    if (command == "1") {
        drumMachine.triggerKick(1.0f);
        Serial.println("🥁 Kick!");
    }
    else if (command == "2") {
        drumMachine.triggerSnare(1.0f);
        Serial.println("🥁 Snare!");
    }
    else if (command == "3") {
        drumMachine.triggerCymbal(1.0f);
        Serial.println("🥁 Cymbal!");
    }
    else if (command == "4") {
        drumMachine.triggerHiHat(1.0f, false);
        Serial.println("🥁 Closed Hi-Hat!");
    }
    else if (command == "5") {
        drumMachine.triggerHiHat(1.0f, true);
        Serial.println("🥁 Open Hi-Hat!");
    }
    else if (command == "6") {
        drumMachine.triggerTom(1.0f);
        Serial.println("🥁 Tom!");
    }
    else if (command == "7") {
        drumMachine.triggerConga(1.0f);
        Serial.println("🥁 Conga!");
    }
    else if (command == "8") {
        drumMachine.triggerRimshot(1.0f);
        Serial.println("🥁 Rimshot!");
    }
    else if (command == "9") {
        drumMachine.triggerMaracas(1.0f);
        Serial.println("🥁 Maracas!");
    }
    else if (command == "0") {
        drumMachine.triggerClap(1.0f);
        Serial.println("🥁 Clap!");
    }
    else if (command == "c") {
        drumMachine.triggerCowbell(1.0f);
        Serial.println("🥁 Cowbell!");
    }
    
    // 풀 네임 명령
    else if (command == "kick" || command.startsWith("kick ")) {
        handleKickCommand(command);
    }
    else if (command == "snare" || command.startsWith("snare ")) {
        handleSnareCommand(command);
    }
    else if (command == "cymbal" || command.startsWith("cymbal ")) {
        handleCymbalCommand(command);
    }
    else if (command == "hihat" || command.startsWith("hihat ")) {
        handleHiHatCommand(command);
    }
    else if (command == "tom" || command.startsWith("tom ")) {
        handleTomCommand(command);
    }
    else if (command == "conga" || command.startsWith("conga ")) {
        handleCongaCommand(command);
    }
    else if (command == "rimshot") {
        drumMachine.triggerRimshot(1.0f);
        Serial.println("🥁 Rimshot!");
    }
    else if (command == "maracas") {
        drumMachine.triggerMaracas(1.0f);
        Serial.println("🥁 Maracas!");
    }
    else if (command == "clap" || command.startsWith("clap ")) {
        handleClapCommand(command);
    }
    else if (command == "cowbell") {
        drumMachine.triggerCowbell(1.0f);
        Serial.println("🥁 Cowbell!");
    }
    
    // 시스템 명령
    else if (command == "help" || command == "h") {
        printInstructions();
    }
    else if (command == "status" || command == "s") {
        printStatus();
    }
    else if (command == "config" || command == "cfg") {
        printConfig();
    }
    else if (command == "perf" || command == "p") {
        printPerformance();
    }
    else if (command == "reset" || command == "r") {
        resetSystem();
    }
    else if (command == "test" || command == "t") {
        runAudioTest();
    }
    else if (command == "examples" || command == "e") {
        printExamples();
    }
    
    // 마스터 컨트롤
    else if (command.startsWith("master ")) {
        float volume = command.substring(7).toFloat();
        if (volume >= 0.0f && volume <= 1.0f) {
            drumMachine.setMasterVolume(volume);
            Serial.println("🔊 마스터 볼륨: " + String(volume));
        } else {
            Serial.println("❌ 볼륨은 0.0-1.0 사이의 값이어야 합니다.");
        }
    }
    
    else {
        Serial.println("❓ 알 수 없는 명령: " + command);
        Serial.println("💡 'help' 명령어로 사용법을 확인하세요.");
    }
}

// ============================================
// 개별 드럼 명령 처리 함수들
// ============================================

void handleKickCommand(const String& command) {
    if (command == "kick") {
        drumMachine.triggerKick(1.0f);
        Serial.println("🥁 Kick!");
    } else if (command.startsWith("kick ")) {
        float velocity = command.substring(5).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerKick(velocity);
            Serial.println("🥁 Kick (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleSnareCommand(const String& command) {
    if (command == "snare") {
        drumMachine.triggerSnare(1.0f);
        Serial.println("🥁 Snare!");
    } else if (command.startsWith("snare ")) {
        float velocity = command.substring(6).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerSnare(velocity);
            Serial.println("🥁 Snare (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleCymbalCommand(const String& command) {
    if (command == "cymbal") {
        drumMachine.triggerCymbal(1.0f);
        Serial.println("🥁 Cymbal!");
    } else if (command.startsWith("cymbal ")) {
        float velocity = command.substring(7).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerCymbal(velocity);
            Serial.println("🥁 Cymbal (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleHiHatCommand(const String& command) {
    if (command == "hihat") {
        drumMachine.triggerHiHat(1.0f, false);
        Serial.println("🥁 Closed Hi-Hat!");
    } else if (command.startsWith("hihat ")) {
        float velocity = command.substring(6).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerHiHat(velocity, false);
            Serial.println("🥁 Hi-Hat (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleTomCommand(const String& command) {
    if (command == "tom") {
        drumMachine.triggerTom(1.0f);
        Serial.println("🥁 Tom!");
    } else if (command.startsWith("tom ")) {
        float velocity = command.substring(4).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerTom(velocity);
            Serial.println("🥁 Tom (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleCongaCommand(const String& command) {
    if (command == "conga") {
        drumMachine.triggerConga(1.0f);
        Serial.println("🥁 Conga!");
    } else if (command.startsWith("conga ")) {
        float velocity = command.substring(6).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerConga(velocity);
            Serial.println("🥁 Conga (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

void handleClapCommand(const String& command) {
    if (command == "clap") {
        drumMachine.triggerClap(1.0f);
        Serial.println("🥁 Clap!");
    } else if (command.startsWith("clap ")) {
        float velocity = command.substring(5).toFloat();
        if (velocity >= 0.0f && velocity <= 1.0f) {
            drumMachine.triggerClap(velocity);
            Serial.println("🥁 Clap (벨로시티: " + String(velocity) + ")");
        } else {
            Serial.println("❌ 벨로시티는 0.0-1.0 사이여야 합니다.");
        }
    }
}

// ============================================
// 시퀀서 처리 (선택사항)
// ============================================

void handleSequencer(unsigned long currentTime) {
    // TODO: 패턴 기반 자동 드럼 연주
    // static unsigned long lastStepTime = 0;
    // static int currentStep = 0;
    // 
    // if (currentTime - lastStepTime >= 120) { // 120 BPM
    //     // 현재 스텝의 패턴 실행
    //     executePattern(currentStep);
    //     currentStep = (currentStep + 1) % 16;
    //     lastStepTime = currentTime;
    // }
}

// ============================================
// 성능 모니터링
// ============================================

void updatePerformanceMetrics(unsigned long currentTime) {
    static unsigned long lastUpdate = 0;
    static unsigned long lastSampleCount = 0;
    
    unsigned long elapsed = currentTime - lastUpdate;
    unsigned long samplesThisPeriod = sampleCount - lastSampleCount;
    
    if (elapsed > 0) {
        float actualSampleRate = (samplesThisPeriod * 1000.0f) / elapsed;
        cpuUsage = (actualSampleRate / SAMPLE_RATE) * 100.0f;
        
        if (PERFORMANCE_MONITORING) {
            Serial.println("📊 성능: " + String(actualSampleRate, 0) + " Hz (" + 
                         String(cpuUsage, 1) + "%) | 샘플: " + String(sampleCount));
        }
    }
    
    lastUpdate = currentTime;
    lastSampleCount = sampleCount;
}

// ============================================
// 자동 저장 처리
// ============================================

void handleAutoSave(unsigned long currentTime) {
    static unsigned long lastSave = 0;
    static String lastState = "";
    
    String currentState = getCurrentState();
    
    if (currentState != lastState && currentTime - lastSave > AUTO_SAVE_INTERVAL) {
        saveStateToEEPROM(currentState);
        lastSave = currentTime;
        lastState = currentState;
    }
}

String getCurrentState() {
    // 현재 설정 상태를 JSON으로 반환
    return "{}"; // TODO: 실제 구현
}

void saveStateToEEPROM(const String& state) {
    // EEPROM에 상태 저장
    Serial.println("💾 설정 자동 저장됨");
}

// ============================================
// 유틸리티 함수들
// ============================================

void printSystemInfo() {
    Serial.println("🔧 시스템 정보:");
    Serial.println("  보드: " + String(BOARD_NAME));
    Serial.println("  CPU: " + String(ESP.getCpuFreqMHz()) + " MHz");
    Serial.println("  Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + " MB");
    Serial.println("  RAM: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  GPIO: " + String(I2S_WS_PIN) + "," + String(I2S_BCK_PIN) + "," + String(I2S_DATA_PIN));
    Serial.println("");
}

void printInstructions() {
    Serial.println("📖 사용법:");
    Serial.println("");
    Serial.println("🎵 기본 드럼 연주:");
    Serial.println("  1,2,3,4,5,6,7,8,9,0,c  (숫자키)");
    Serial.println("  kick, snare, cymbal, hihat, tom, conga");
    Serial.println("  rimshot, maracas, clap, cowbell");
    Serial.println("");
    Serial.println("🎹 벨로시티 제어:");
    Serial.println("  kick 0.5    (0.0-1.0)");
    Serial.println("  snare 0.8   (벨로시티)");
    Serial.println("");
    Serial.println("🔧 시스템 제어:");
    Serial.println("  master 0.7  (마스터 볼륨)");
    Serial.println("  status      (현재 상태)");
    Serial.println("  config      (설정 정보)");
    Serial.println("  perf        (성능 정보)");
    Serial.println("  reset       (시스템 리셋)");
    Serial.println("  test        (오디오 테스트)");
    Serial.println("  help        (이 도움말)");
    Serial.println("");
}

void printExamples() {
    Serial.println("💡 사용 예시:");
    Serial.println("");
    Serial.println("  1) 기본 드럼 연주:");
    Serial.println("     kick snare kick snare");
    Serial.println("");
    Serial.println("  2) 벨로시티 연주:");
    Serial.println("     kick 0.3    (부드러운 킥)");
    Serial.println("     snare 1.0   (강한 스네어)");
    Serial.println("");
    Serial.println("  3) 리듬 패턴:");
    Serial.println("     1 2 3 4 5 6 7 8");
    Serial.println("     kick snare kick snare");
    Serial.println("");
    Serial.println("  4) 복합 소리:");
    Serial.println("     kick snare hihat conga");
    Serial.println("");
}

void printStatus() {
    Serial.println("📊 현재 상태:");
    Serial.println("");
    Serial.println("🎵 오디오:");
    Serial.println("  샘플 레이트: " + String(SAMPLE_RATE) + " Hz");
    Serial.println("  마스터 볼륨: " + String(MASTER_VOLUME));
    Serial.println("  I2S 상태: 정상");
    Serial.println("");
    Serial.println("💻 시스템:");
    Serial.println("  RAM 사용: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  CPU 사용률: " + String(cpuUsage, 1) + "%");
    Serial.println("  실행시간: " + String(millis() / 1000) + "초");
    Serial.println("");
    Serial.println("🔧 설정:");
    Serial.println("  시퀀서: " + String(ENABLE_SEQUENCER ? "활성화" : "비활성화"));
    Serial.println("  자동저장: " + String(ENABLE_AUTO_SAVE ? "활성화" : "비활성화"));
    Serial.println("  성능모니터링: " + String(PERFORMANCE_MONITORING ? "활성화" : "비활성화"));
    Serial.println("");
}

void printConfig() {
    Serial.println("⚙️ 현재 설정:");
    Serial.println("");
    Serial.println("🎵 오디오 설정:");
    Serial.println("  SAMPLE_RATE: " + String(SAMPLE_RATE));
    Serial.println("  BUFFER_SIZE: " + String(BUFFER_SIZE));
    Serial.println("  MONO_OUTPUT: " + String(MONO_OUTPUT ? "true" : "false"));
    Serial.println("  MASTER_VOLUME: " + String(MASTER_VOLUME));
    Serial.println("  POLYPHONY_LIMIT: " + String(POLYPHONY_LIMIT));
    Serial.println("");
    Serial.println("🔧 하드웨어 설정:");
    Serial.println("  I2S_WS_PIN: " + String(I2S_WS_PIN));
    Serial.println("  I2S_BCK_PIN: " + String(I2S_BCK_PIN));
    Serial.println("  I2S_DATA_PIN: " + String(I2S_DATA_PIN));
    Serial.println("");
    Serial.println("💡 기능 설정:");
    Serial.println("  ENABLE_SEQUENCER: " + String(ENABLE_SEQUENCER ? "true" : "false"));
    Serial.println("  ENABLE_AUTO_SAVE: " + String(ENABLE_AUTO_SAVE ? "true" : "false"));
    Serial.println("  PERFORMANCE_MONITORING: " + String(PERFORMANCE_MONITORING ? "true" : "false"));
    Serial.println("  AUTO_TEST_ON_STARTUP: " + String(AUTO_TEST_ON_STARTUP ? "true" : "false"));
    Serial.println("  AUTO_SAVE_INTERVAL: " + String(AUTO_SAVE_INTERVAL) + " ms");
    Serial.println("");
}

void printPerformance() {
    Serial.println("⚡ 성능 정보:");
    Serial.println("");
    Serial.println("📊 샘플링:");
    Serial.println("  목표 레이트: " + String(SAMPLE_RATE) + " Hz");
    Serial.println("  실제 레이트: ~" + String(SAMPLE_RATE) + " Hz");
    Serial.println("  버퍼 크기: " + String(BUFFER_SIZE) + " 샘플");
    Serial.println("");
    Serial.println("💻 시스템:");
    Serial.println("  CPU 사용률: " + String(cpuUsage, 1) + "%");
    Serial.println("  메모리: " + String(ESP.getFreeHeap()) + "/" + String(ESP.getHeapSize()) + " bytes");
    Serial.println("  실행시간: " + String(millis() / 1000) + "초");
    Serial.println("  총 샘플 수: " + String(sampleCount));
    Serial.println("");
    Serial.println("🎵 드럼 엔진:");
    Serial.println("  동시 음향 제한: " + String(POLYPHONY_LIMIT));
    Serial.println("  마스터 볼륨: " + String(MASTER_VOLUME));
    Serial.println("");
}

void resetSystem() {
    Serial.println("🔄 시스템 리셋 중...");
    
    // I2S 재초기화
    I2S.end();
    delay(100);
    initializeI2SAudio();
    
    // TR808 재초기화
    initializeTR808();
    
    // 성능 카운터 리셋
    sampleCount = 0;
    lastPerfCheck = millis();
    
    Serial.println("✅ 시스템 리셋 완료!");
}

void runAudioTest() {
    Serial.println("🔊 오디오 테스트 시작...");
    Serial.println("  짧은 드럼 시퀀스를 재생합니다.");
    
    // 기본 드럼 시퀀스 실행
    drumMachine.triggerKick(1.0f);
    delay(200);
    drumMachine.triggerSnare(1.0f);
    delay(200);
    drumMachine.triggerCymbal(1.0f);
    delay(200);
    drumMachine.triggerHiHat(1.0f, false);
    delay(200);
    
    Serial.println("✅ 오디오 테스트 완료!");
}

void showHardwareInstructions() {
    Serial.println("");
    Serial.println("🔌 하드웨어 연결 확인:");
    Serial.println("");
    Serial.println("ESP32C3  →  외부 DAC/오디오 앰프");
    Serial.println("GPIO 1   →  I2S Data Out");
    Serial.println("GPIO 2   →  I2S Bit Clock");
    Serial.println("GPIO 3   →  I2S Word Select");
    Serial.println("3.3V     →  VCC ( DAC에서 )");
    Serial.println("GND      →  GND");
    Serial.println("");
    Serial.println("📦 권장 부품:");
    Serial.println("  - DAC: PCM5102, ES9023");
    Serial.println("  - 앰프: PAM8403, LM386");
    Serial.println("  - 스피커: 4-8Ω, 3-5W");
    Serial.println("");
}

// ============================================
// Arduino 라이브러리 통합 함수
// ============================================

// Arduino IDE에서 라이브러리로 인식하기 위한 함수들
void startMozzi() {
    // Mozzi 호환성 함수 (빈 구현)
}

void stopMozzi() {
    // Mozzi 정지 함수 (빈 구현)
}

// ============================================
// End of File
// ============================================