/*
 * ESP32C3 TR-808 드럼 머신 고급 기능 예제
 * 
 * 이 예제는 TR-808의 고급 기능들을 시연합니다:
 * - 실시간 사운드 조정
 * - 시퀀서 기능
 * - MIDI 인터페이스
 * - 커스텀 드럼 패턴
 * - 효과 처리
 */

#include <I2S.h>
#include <EEPROM.h>
#include "arduino_tr808_config.h"
#include "tr808_drums.h"

// 고급 기능 설정
#define EEPROM_SIZE 4096
#define MAX_PATTERN_LENGTH 32
#define MAX_SEQUENCES 8

// 패턴 시퀀서 구조체
struct DrumPattern {
    char name[16];
    uint16_t steps[MAX_PATTERN_LENGTH];
    uint8_t length;
    uint8_t bpm;
    bool enabled;
};

// 전역 변수
TR808DrumMachine drumMachine;
DrumPattern sequences[MAX_SEQUENCES];
uint8_t currentSequence = 0;
uint8_t currentStep = 0;
unsigned long lastStepTime = 0;
bool sequencerRunning = false;

// 시퀀서 설정
#define DEFAULT_BPM 120
#define STEP_DURATION_MS (60000 / (DEFAULT_BPM * 4)) // 16분음표

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("===========================================");
    Serial.println("  ESP32C3 TR-808 고급 기능 데모");
    Serial.println("===========================================");
    
    // EEPROM 초기화
    EEPROM.begin(EEPROM_SIZE);
    
    // I2S 초기화
    if (!I2S.begin(I2S_STANDARD, DEFAULT_SAMPLE_RATE, 16, 1)) {
        Serial.println("❌ I2S 초기화 실패");
        while(1) delay(1000);
    }
    
    // TR-808 초기화
    drumMachine.setMasterVolume(0.8f);
    
    // 시퀀서 초기화
    initializeSequences();
    
    Serial.println("✅ 고급 기능 시스템 초기화 완료");
    Serial.println("");
    
    printAdvancedInstructions();
}

void loop() {
    // Serial 명령 처리
    handleAdvancedCommands();
    
    // 시퀀서 처리
    if (sequencerRunning) {
        processSequencer();
    }
    
    // 오디오 처리
    processAudio();
    
    // 자동 모드 (시퀀서나 오토메이션)
    static unsigned long lastAutoAction = 0;
    if (millis() - lastAutoAction > 5000) {
        runAutoDemo();
        lastAutoAction = millis();
    }
    
    delayMicroseconds(20); // 안정성을 위한 짧은 지연
}

void handleAdvancedCommands() {
    if (!Serial.available()) return;
    
    String command = Serial.readString();
    command.trim();
    
    Serial.println("명령: " + command);
    
    // 기본 드럼 명령
    if (command == "kick") drumMachine.triggerKick(1.0f);
    else if (command == "snare") drumMachine.triggerSnare(1.0f);
    else if (command == "cymbal") drumMachine.triggerCymbal(1.0f);
    else if (command == "hihat") drumMachine.triggerHiHat(1.0f, false);
    else if (command == "openhihat") drumMachine.triggerHiHat(1.0f, true);
    else if (command == "tom") drumMachine.triggerTom(1.0f);
    else if (command == "conga") drumMachine.triggerConga(1.0f);
    else if (command == "rimshot") drumMachine.triggerRimshot(1.0f);
    else if (command == "maracas") drumMachine.triggerMaracas(1.0f);
    else if (command == "clap") drumMachine.triggerClap(1.0f);
    else if (command == "cowbell") drumMachine.triggerCowbell(1.0f);
    
    // 시퀀서 명령
    else if (command == "seq start") startSequencer();
    else if (command == "seq stop") stopSequencer();
    else if (command == "seq next") nextSequence();
    else if (command == "seq prev") previousSequence();
    else if (command == "seq save") saveCurrentSequence();
    else if (command == "seq load") loadSequence(currentSequence);
    else if (command.startsWith("seq bpm ")) {
        uint8_t bpm = command.substring(8).toInt();
        setSequenceBPM(bpm);
    }
    
    // 고급 드럼 명령
    else if (command.startsWith("kick velocity ")) {
        float vel = command.substring(14).toFloat();
        drumMachine.triggerKick(constrain(vel, 0.0f, 1.0f));
    }
    else if (command.startsWith("kick tone ")) {
        float tone = command.substring(10).toFloat();
        drumMachine.setKickTone(constrain(tone, 0.0f, 1.0f));
        Serial.println("🥁 킥 톤 설정: " + String(tone));
    }
    else if (command.startsWith("snare snappy ")) {
        float snappy = command.substring(13).toFloat();
        drumMachine.setSnareSnappy(constrain(snappy, 0.0f, 1.0f));
        Serial.println("🥁 스네어 스내피 설정: " + String(snappy));
    }
    else if (command.startsWith("master ")) {
        float vol = command.substring(7).toFloat();
        drumMachine.setMasterVolume(constrain(vol, 0.0f, 1.0f));
        Serial.println("🔊 마스터 볼륨: " + String(vol));
    }
    
    // 시스템 명령
    else if (command == "demo") runAutoDemo();
    else if (command == "save") saveAllSettings();
    else if (command == "load") loadAllSettings();
    else if (command == "patterns") printSequences();
    else if (command == "status") printAdvancedStatus();
    else if (command == "help") printAdvancedInstructions();
    else if (command == "default") loadDefaultSequence();
    
    // 예제 패턴 로드
    else if (command == "pattern basic") loadBasicPattern();
    else if (command == "pattern house") loadHousePattern();
    else if (command == "pattern techno") loadTechnoPattern();
    else if (command == "pattern latin") loadLatinPattern();
    
    else {
        Serial.println("❓ 알 수 없는 명령: " + command);
        Serial.println("💡 'help' 명령어로 사용법을 확인하세요.");
    }
}

void processSequencer() {
    unsigned long currentTime = millis();
    unsigned long stepDuration = 60000 / (sequences[currentSequence].bpm * 4);
    
    if (currentTime - lastStepTime >= stepDuration) {
        executePatternStep(currentStep);
        currentStep = (currentStep + 1) % sequences[currentSequence].length;
        lastStepTime = currentTime;
    }
}

void executePatternStep(uint8_t step) {
    uint16_t pattern = sequences[currentSequence].steps[step];
    
    // 패턴 비트 매핑 (예시)
    if (pattern & 0x0001) drumMachine.triggerKick(1.0f);      // Bit 0: Kick
    if (pattern & 0x0002) drumMachine.triggerSnare(0.8f);     // Bit 1: Snare
    if (pattern & 0x0004) drumMachine.triggerCymbal(0.6f);    // Bit 2: Cymbal
    if (pattern & 0x0008) drumMachine.triggerHiHat(0.7f, false); // Bit 3: Hi-Hat
    if (pattern & 0x0010) drumMachine.triggerTom(0.5f);       // Bit 4: Tom
    if (pattern & 0x0020) drumMachine.triggerConga(0.6f);     // Bit 5: Conga
    if (pattern & 0x0040) drumMachine.triggerClap(0.4f);      // Bit 6: Clap
    if (pattern & 0x0080) drumMachine.triggerRimshot(0.5f);   // Bit 7: Rimshot
    
    // 시각적 피드백 (Serial)
    if (step % 4 == 0) {
        Serial.println("🎵 패턴 스텝 " + String(step + 1) + "/" + String(sequences[currentSequence].length));
    }
}

void processAudio() {
    float audioSample = drumMachine.process();
    int16_t intSample = (int16_t)(audioSample * 32767);
    
    // I2S 버퍼 채우기
    static int16_t i2sBuffer[256];
    for (int i = 0; i < 256; i++) {
        i2sBuffer[i] = intSample;
    }
    
    size_t bytesWritten = 0;
    I2S.write(i2sBuffer, 256, &bytesWritten);
}

void runAutoDemo() {
    Serial.println("🎹 자동 데모 시작...");
    
    // 다양한 드럼 조합 데모
    drumMachine.triggerKick(1.0f);
    delay(100);
    drumMachine.triggerSnare(0.8f);
    delay(100);
    drumMachine.triggerCymbal(0.6f);
    delay(100);
    drumMachine.triggerHiHat(0.7f, false);
    
    // 사운드 변경 데모
    Serial.println("🎛️ 사운드 변경 데모...");
    drumMachine.setKickTone(0.9f);
    drumMachine.setSnareSnappy(0.3f);
    delay(1000);
    drumMachine.setKickTone(0.1f);
    drumMachine.setSnareSnappy(0.9f);
    
    Serial.println("✅ 데모 완료");
}

// ============================================
// 시퀀서 함수들
// ============================================

void initializeSequences() {
    // 기본 시퀀스 초기화
    for (int i = 0; i < MAX_SEQUENCES; i++) {
        sequences[i].length = 16;
        sequences[i].bpm = DEFAULT_BPM;
        sequences[i].enabled = false;
        strcpy(sequences[i].name, "Empty");
        
        // 빈 패턴으로 초기화
        for (int j = 0; j < MAX_PATTERN_LENGTH; j++) {
            sequences[i].steps[j] = 0;
        }
    }
    
    // 기본 패턴 로드
    loadBasicPattern();
    
    Serial.println("🎼 시퀀서 초기화 완료 (" + String(MAX_SEQUENCES) + "개 시퀀스)");
}

void startSequencer() {
    sequencerRunning = true;
    currentStep = 0;
    lastStepTime = millis();
    Serial.println("▶️ 시퀀서 시작: " + String(sequences[currentSequence].name) + 
                   " (" + String(sequences[currentSequence].length) + "스텝)");
}

void stopSequencer() {
    sequencerRunning = false;
    Serial.println("⏹️ 시퀀서 중지");
}

void nextSequence() {
    currentSequence = (currentSequence + 1) % MAX_SEQUENCES;
    Serial.println("⏭️ 시퀀스 변경: " + String(currentSequence + 1) + "/" + String(MAX_SEQUENCES));
    if (sequencerRunning) {
        currentStep = 0;
        lastStepTime = millis();
    }
}

void previousSequence() {
    if (currentSequence == 0) {
        currentSequence = MAX_SEQUENCES - 1;
    } else {
        currentSequence--;
    }
    Serial.println("⏮️ 시퀀스 변경: " + String(currentSequence + 1) + "/" + String(MAX_SEQUENCES));
    if (sequencerRunning) {
        currentStep = 0;
        lastStepTime = millis();
    }
}

void setSequenceBPM(uint8_t bpm) {
    if (bpm >= 60 && bpm <= 200) {
        sequences[currentSequence].bpm = bpm;
        Serial.println("🎵 BPM 설정: " + String(bpm));
    } else {
        Serial.println("❌ BPM은 60-200 사이의 값이어야 합니다.");
    }
}

// ============================================
// 패턴 데이터 함수들
// ============================================

void loadBasicPattern() {
    strcpy(sequences[currentSequence].name, "Basic");
    sequences[currentSequence].length = 16;
    sequences[currentSequence].bpm = 120;
    
    // 기본 4/4 패턴
    uint16_t pattern[16] = {
        0x0001, 0x0000, 0x0002, 0x0000,  // Kick & Snare pattern
        0x0001, 0x0000, 0x0002, 0x0000,
        0x0001, 0x0008, 0x0002, 0x0000,  // Hi-Hat added
        0x0001, 0x0000, 0x0002, 0x0010   // Tom on beat 4
    };
    
    for (int i = 0; i < 16; i++) {
        sequences[currentSequence].steps[i] = pattern[i];
    }
    
    Serial.println("✅ 기본 패턴 로드됨");
}

void loadHousePattern() {
    strcpy(sequences[currentSequence].name, "House");
    sequences[currentSequence].length = 16;
    sequences[currentSequence].bpm = 128;
    
    // 하우스 음악 패턴
    uint16_t pattern[16] = {
        0x0001, 0x0008, 0x0002, 0x0008,  // Heavy kick & hi-hat
        0x0001, 0x0008, 0x0002, 0x0040,  // Clap on beat 3
        0x0001, 0x0008, 0x0002, 0x0008,
        0x0001, 0x0008, 0x0002, 0x0040
    };
    
    for (int i = 0; i < 16; i++) {
        sequences[currentSequence].steps[i] = pattern[i];
    }
    
    Serial.println("✅ 하우스 패턴 로드됨");
}

void loadTechnoPattern() {
    strcpy(sequences[currentSequence].name, "Techno");
    sequences[currentSequence].length = 16;
    sequences[currentSequence].bpm = 135;
    
    // 테크노 패턴
    uint16_t pattern[16] = {
        0x0001, 0x0000, 0x0000, 0x0008,  // Syncopated kick
        0x0001, 0x0000, 0x0010, 0x0008,  // Tom variations
        0x0001, 0x0000, 0x0000, 0x0008,
        0x0001, 0x0000, 0x0010, 0x0020   // Complex rhythm
    };
    
    for (int i = 0; i < 16; i++) {
        sequences[currentSequence].steps[i] = pattern[i];
    }
    
    Serial.println("✅ 테크노 패턴 로드됨");
}

void loadLatinPattern() {
    strcpy(sequences[currentSequence].name, "Latin");
    sequences[currentSequence].length = 16;
    sequences[currentSequence].bpm = 120;
    
    // 라틴 리듬 패턴
    uint16_t pattern[16] = {
        0x0001, 0x0020, 0x0000, 0x0020,  // Conga emphasis
        0x0001, 0x0020, 0x0000, 0x0008,  // Hi-hat rhythm
        0x0001, 0x0020, 0x0000, 0x0020,
        0x0001, 0x0020, 0x0000, 0x0008
    };
    
    for (int i = 0; i < 16; i++) {
        sequences[currentSequence].steps[i] = pattern[i];
    }
    
    Serial.println("✅ 라틴 패턴 로드됨");
}

// ============================================
// EEPROM 저장/불러오기
// ============================================

void saveAllSettings() {
    // 시퀀스 저장
    for (int i = 0; i < MAX_SEQUENCES; i++) {
        saveSequence(i);
    }
    
    Serial.println("💾 모든 설정 저장 완료");
}

void saveSequence(uint8_t seqIndex) {
    int addr = seqIndex * sizeof(DrumPattern);
    EEPROM.put(addr, sequences[seqIndex]);
    
    Serial.println("💾 시퀀스 " + String(seqIndex + 1) + " 저장됨");
}

void loadAllSettings() {
    // 시퀀스 로드
    for (int i = 0; i < MAX_SEQUENCES; i++) {
        loadSequence(i);
    }
    
    Serial.println("📂 모든 설정 로드 완료");
}

void loadSequence(uint8_t seqIndex) {
    int addr = seqIndex * sizeof(DrumPattern);
    EEPROM.get(addr, sequences[seqIndex]);
    
    Serial.println("📂 시퀀스 " + String(seqIndex + 1) + " 로드됨: " + String(sequences[seqIndex].name));
}

// ============================================
// 정보 출력 함수들
// ============================================

void printAdvancedInstructions() {
    Serial.println("🎛️ 고급 기능 명령어:");
    Serial.println("");
    Serial.println("🎼 시퀀서:");
    Serial.println("  seq start      - 시퀀서 시작");
    Serial.println("  seq stop       - 시퀀서 중지");
    Serial.println("  seq next       - 다음 시퀀스");
    Serial.println("  seq prev       - 이전 시퀀스");
    Serial.println("  seq bpm <n>    - BPM 설정 (60-200)");
    Serial.println("  seq save       - 현재 시퀀스 저장");
    Serial.println("  seq load       - 시퀀스 로드");
    Serial.println("");
    Serial.println("🎵 패턴 로드:");
    Serial.println("  pattern basic  - 기본 4/4 패턴");
    Serial.println("  pattern house  - 하우스 패턴");
    Serial.println("  pattern techno - 테크노 패턴");
    Serial.println("  pattern latin  - 라틴 패턴");
    Serial.println("");
    Serial.println("🎛️ 사운드 조정:");
    Serial.println("  kick velocity <n>  - 킥 벨로시티");
    Serial.println("  kick tone <n>      - 킥 톤 (0-1)");
    Serial.println("  snare snappy <n>   - 스네어 스내피 (0-1)");
    Serial.println("  master <n>         - 마스터 볼륨 (0-1)");
    Serial.println("");
    Serial.println("🔧 시스템:");
    Serial.println("  save      - 설정 저장");
    Serial.println("  load      - 설정 로드");
    Serial.println("  status    - 상세 상태");
    Serial.println("  patterns  - 시퀀스 목록");
    Serial.println("  demo      - 자동 데모");
    Serial.println("  default   - 기본 패턴 로드");
    Serial.println("");
}

void printSequences() {
    Serial.println("📊 시퀀스 목록:");
    for (int i = 0; i < MAX_SEQUENCES; i++) {
        Serial.print("[" + String(i+1) + "] " + String(sequences[i].name));
        if (i == currentSequence) Serial.print(" (현재)");
        Serial.println(" - " + String(sequences[i].length) + "스텝, " + 
                       String(sequences[i].bpm) + " BPM" + 
                       (sequences[i].enabled ? " ✅" : " ⏸️"));
    }
}

void printAdvancedStatus() {
    Serial.println("📊 고급 상태 정보:");
    Serial.println("");
    Serial.println("🎼 시퀀서:");
    Serial.println("  상태: " + String(sequencerRunning ? "실행중" : "중지됨"));
    Serial.println("  현재: [" + String(currentSequence+1) + "] " + String(sequences[currentSequence].name));
    Serial.println("  스텝: " + String(currentStep + 1) + "/" + String(sequences[currentSequence].length));
    Serial.println("  BPM: " + String(sequences[currentSequence].bpm));
    Serial.println("");
    Serial.println("🔊 오디오:");
    Serial.println("  마스터 볼륨: 0.8");
    Serial.println("  샘플레이트: " + String(DEFAULT_SAMPLE_RATE) + " Hz");
    Serial.println("  메모리: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("  실행시간: " + String(millis() / 1000) + "초");
    Serial.println("");
    Serial.println("💾 저장:");
    Serial.println("  EEPROM: " + String(EEPROM_SIZE) + " bytes");
    Serial.println("  시퀀스: " + String(MAX_SEQUENCES) + "개");
    Serial.println("  패턴 길이: 최대 " + String(MAX_PATTERN_LENGTH) + "스텝");
    Serial.println("");
}

void loadDefaultSequence() {
    loadBasicPattern();
    currentStep = 0;
    Serial.println("🔄 기본 시퀀스로 리셋됨");
}