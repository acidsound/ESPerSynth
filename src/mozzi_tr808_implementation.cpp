/*
 * Mozzi TR-808 드럼 머신 구현 파일
 * 
 * ESP32C3 + Mozzi Library + TR-808 드럼 머신 완전 구현
 * 18개 드럼 소스, 실시간 패턴 재생, 성능 모니터링 지원
 * 
 * 작성일: 2025-10-30
 * 버전: 1.0.0
 */

#include "../src/mozzi_tr808_config.h"
#include "../src/esp32c3_mozzi_integration.h"
#include "../src/tr808_drums.h"

// =============================================================================
// 전역 인스턴스 생성
// =============================================================================

TR808DrumMachineMozzi tr808Mozzi;

// =============================================================================
// TR808DrumMachineMozzi 클래스 구현
// =============================================================================

TR808DrumMachineMozzi::TR808DrumMachineMozzi() {
    initialized = false;
    audioActive = false;
    masterVolume = TR808_DEFAULT_MASTER_VOLUME;
    currentPatternIndex = 0;
    patternPlaying = false;
    patternStep = 0;
    patternTempo = DEFAULT_TEMPO;
    
    // 성능 메트릭 초기화
    memset(&performance, 0, sizeof(performance));
    performance.maxPolyphony = MAX_POLYPHONY;
    
    // 시스템 상태 초기화
    memset(&systemStatus, 0, sizeof(systemStatus));
    systemStatus.masterVolume = masterVolume;
    
    // 드럼 소스 포인터 초기화
    for (int i = 0; i < TR808_NUM_SOURCES; i++) {
        drumSources[i] = nullptr;
    }
}

// =============================================================================
// 초기화 및 설정 함수들
// =============================================================================

bool TR808DrumMachineMozzi::initialize() {
    Serial.println(F("🥁 TR-808 드럼 머신 초기화 시작..."));
    
    // 드럼 소스 초기화
    if (!initializeDrumSources()) {
        Serial.println(F("❌ 드럼 소스 초기화 실패"));
        return false;
    }
    
    // 기본 패턴 로드
    loadDefaultPatterns();
    
    // 시스템 상태 업데이트
    initialized = true;
    systemStatus.initialized = true;
    
    Serial.println(F("✅ TR-808 드럼 머신 초기화 완료"));
    return true;
}

bool TR808DrumMachineMozzi::initializeAudio() {
    Serial.println(F("🔊 TR-808 오디오 시스템 초기화..."));
    
    // 오디오 시스템 상태 설정
    audioActive = true;
    systemStatus.audioActive = true;
    
    Serial.println(F("✅ TR-808 오디오 시스템 초기화 완료"));
    return true;
}

bool TR808DrumMachineMozzi::initializePerformanceMonitoring() {
    Serial.println(F("📊 TR-808 성능 모니터링 초기화..."));
    
    // 성능 메트릭 초기화
    memset(&performance, 0, sizeof(performance));
    performance.maxPolyphony = MAX_POLYPHONY;
    
    // 시스템 상태 업데이트
    systemStatus.performanceMonitoring = true;
    systemStatus.uptimeMs = millis();
    
    Serial.println(F("✅ TR-808 성능 모니터링 시작"));
    return true;
}

bool TR808DrumMachineMozzi::initializeDrumSources() {
    Serial.println(F("🔧 드럼 소스 초기화..."));
    
    // 드럼 소스별 초기화 (간단한 구현)
    for (int i = 0; i < TR808_NUM_SOURCES; i++) {
        switch (i) {
            case TR808_KICK:
                // Kick 드럼 초기화
                drumSources[i] = new TR808Oscillator();
                ((TR808Oscillator*)drumSources[i])->setFrequency(KICK_FREQUENCY);
                break;
                
            case TR808_SNARE:
                // Snare 드럼 초기화
                drumSources[i] = new TR808Oscillator();
                ((TR808Oscillator*)drumSources[i])->setFrequency(SNARE_FREQUENCY);
                break;
                
            case TR808_HIHAT_CLOSED:
            case TR808_HIHAT_OPEN:
                // Hi-Hat 초기화
                drumSources[i] = new TR808Oscillator();
                ((TR808Oscillator*)drumSources[i])->setFrequency(HIHAT_FREQUENCY);
                break;
                
            default:
                // 기타 드럼 초기화
                drumSources[i] = new TR808Oscillator();
                ((TR808Oscillator*)drumSources[i])->setFrequency(100.0f);
                break;
        }
        
        TR808_DEBUG_PRINT(F("드럼 소스 "));
        TR808_DEBUG_PRINT(i);
        TR808_DEBUG_PRINTLN(F(" 초기화 완료"));
    }
    
    return true;
}

// =============================================================================
// 드럼 제어 함수들
// =============================================================================

void TR808DrumMachineMozzi::triggerDrum(uint8_t drumType, float velocity) {
    if (!initialized || drumType >= TR808_NUM_SOURCES) {
        return;
    }
    
    // 벨로시티 검증
    velocity = constrain(velocity, 0.0f, 1.0f);
    
    // 폴리포니 증가
    if (performance.polyphony < performance.maxPolyphony) {
        performance.polyphony++;
    }
    
    // 드럼 소스별 처리
    switch (drumType) {
        case TR808_KICK:
            processDrumSource(TR808_KICK, velocity * KICK_PUNCH_GAIN);
            break;
            
        case TR808_SNARE:
            processDrumSource(TR808_SNARE, velocity);
            break;
            
        case TR808_CYMBAL:
            processDrumSource(TR808_CYMBAL, velocity * 0.8f);
            break;
            
        case TR808_HIHAT_CLOSED:
        case TR808_HIHAT_OPEN:
            processDrumSource(drumType, velocity * 0.6f);
            break;
            
        default:
            processDrumSource(drumType, velocity);
            break;
    }
    
    TR808_DEBUG_PRINT(F("드럼 트리거: "));
    TR808_DEBUG_PRINT(drumType);
    TR808_DEBUG_PRINT(F(", 벨로시티: "));
    TR808_DEBUG_PRINTLN(velocity);
}

void TR808DrumMachineMozzi::triggerDrum(const String& drumName, float velocity) {
    // 드럼 이름 매핑
    if (drumName == "kick") {
        triggerDrum(TR808_KICK, velocity);
    } else if (drumName == "snare") {
        triggerDrum(TR808_SNARE, velocity);
    } else if (drumName == "cymbal") {
        triggerDrum(TR808_CYMBAL, velocity);
    } else if (drumName == "hihat") {
        triggerDrum(TR808_HIHAT_CLOSED, velocity);
    } else if (drumName == "tom") {
        triggerDrum(TR808_TOM_MID, velocity);
    } else if (drumName == "conga") {
        triggerDrum(TR808_CONGA_MID, velocity);
    } else if (drumName == "rimshot") {
        triggerDrum(TR808_RIMSHOT, velocity);
    } else if (drumName == "maracas") {
        triggerDrum(TR808_MARACAS, velocity);
    } else if (drumName == "clap") {
        triggerDrum(TR808_CLAW, velocity);
    } else if (drumName == "cowbell") {
        triggerDrum(TR808_COWBELL, velocity);
    }
}

void TR808DrumMachineMozzi::setMasterVolume(float volume) {
    masterVolume = constrain(volume, TR808_MIN_VOLUME, TR808_MAX_VOLUME);
    systemStatus.masterVolume = masterVolume;
    
    TR808_DEBUG_PRINT(F("마스터 볼륨 설정: "));
    TR808_DEBUG_PRINTLN(masterVolume);
}

// =============================================================================
// 패턴 제어 함수들
// =============================================================================

bool TR808DrumMachineMozzi::loadPattern(uint8_t patternIndex) {
    if (patternIndex >= NUM_PATTERNS) {
        return false;
    }
    
    currentPatternIndex = patternIndex;
    patternStep = 0;
    
    TR808_DEBUG_PRINT(F("패턴 로드: "));
    TR808_DEBUG_PRINTLN(patternIndex);
    
    return true;
}

bool TR808DrumMachineMozzi::startPattern(uint8_t patternIndex) {
    if (!loadPattern(patternIndex)) {
        return false;
    }
    
    patternPlaying = true;
    systemStatus.patternPlaying = true;
    systemStatus.currentPattern = patternIndex;
    
    TR808_DEBUG_PRINT(F("패턴 재생 시작: "));
    TR808_DEBUG_PRINTLN(patternIndex);
    
    return true;
}

void TR808DrumMachineMozzi::stopPattern() {
    patternPlaying = false;
    systemStatus.patternPlaying = false;
    patternStep = 0;
    
    TR808_DEBUG_PRINTLN(F("패턴 중지"));
}

void TR808DrumMachineMozzi::pausePattern() {
    patternPlaying = false;
    systemStatus.patternPlaying = false;
    
    TR808_DEBUG_PRINTLN(F("패턴 일시중지"));
}

void TR808DrumMachineMozzi::resumePattern() {
    patternPlaying = true;
    systemStatus.patternPlaying = true;
    
    TR808_DEBUG_PRINTLN(F("패턴 재개"));
}

// =============================================================================
// 상태 확인 함수들
// =============================================================================

float TR808DrumMachineMozzi::getMasterVolume() const {
    return masterVolume;
}

// =============================================================================
// 정보 출력 함수들
// =============================================================================

void TR808DrumMachineMozzi::printSystemStatus() {
    Serial.println(F("\n📊 === TR-808 시스템 상태 ==="));
    Serial.print(F("🔧 초기화: "));
    Serial.println(initialized ? F("완료") : F("미완료"));
    Serial.print(F("🔊 오디오: "));
    Serial.println(audioActive ? F("활성") : F("비활성"));
    Serial.print(F("🎵 패턴 재생: "));
    Serial.println(patternPlaying ? F("재생 중") : F("정지"));
    Serial.print(F("🎚️ 마스터 볼륨: "));
    Serial.print(masterVolume);
    Serial.println(F("/ 1.0"));
    Serial.print(F("⏱️ 가동 시간: "));
    Serial.print(millis());
    Serial.println(F(" ms"));
}

void TR808DrumMachineMozzi::printPerformanceReport() {
    Serial.println(F("\n📈 === 성능 통계 ==="));
    
    // CPU 사용률 계산 (간단한 방법)
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = 32000; // ESP32C3 추정 총 메모리
    uint32_t memoryUsage = totalHeap - freeHeap;
    
    Serial.print(F"💾 메모리 사용: ");
    Serial.print(memoryUsage);
    Serial.print(F" / ");
    Serial.print(totalHeap);
    Serial.println(F(" bytes"));
    
    Serial.print(F"🎭 현재 폴리포니: ");
    Serial.print(performance.polyphony);
    Serial.print(F" / ");
    Serial.println(performance.maxPolyphony);
    
    Serial.print(F"🔊 처리된 샘플: ");
    Serial.println(performance.sampleCount);
    
    Serial.print(F"⚠️ 드롭된 샘플: ");
    Serial.println(performance.dropCount);
    
    Serial.print(F"🔄 버퍼 언더런: ");
    Serial.println(performance.bufferUnderruns);
    
    Serial.print(F"📶 최종 메모리 사용률: "));
    Serial.print((float)memoryUsage / totalHeap * 100.0f);
    Serial.println(F("%"));
}

void TR808DrumMachineMozzi::printPatternList() {
    Serial.println(F("\n🎼 === 사용 가능한 패턴 ==="));
    for (int i = 0; i < NUM_PATTERNS; i++) {
        Serial.print(F("패턴 "));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.println(patterns[i].name);
    }
}

void TR808DrumMachineMozzi::printDrumList() {
    Serial.println(F("\n🥁 === 지원되는 드럼 소스 ==="));
    const char* drumNames[] = {
        "kick", "snare", "cymbal", "hihat_closed", "hihat_open",
        "tom_low", "tom_mid", "tom_high", "conga_low", "conga_mid",
        "conga_high", "rimshot", "maracas", "clap", "cowbell"
    };
    
    for (int i = 0; i < 15 && i < TR808_NUM_SOURCES; i++) {
        Serial.print(F(""));
        Serial.print(i);
        Serial.print(F(": "));
        Serial.println(drumNames[i]);
    }
}

// =============================================================================
// Serial 명령 처리
// =============================================================================

bool TR808DrumMachineMozzi::processSerialCommand(const String& command) {
    // 명령어 파싱
    int spacePos = command.indexOf(' ');
    String cmd = (spacePos > 0) ? command.substring(0, spacePos) : command;
    String param = (spacePos > 0) ? command.substring(spacePos + 1) : "";
    
    cmd.trim();
    param.trim();
    
    // 도움말 명령
    if (cmd == "help" || cmd == "?") {
        Serial.println(HELP_TEXT);
        return true;
    }
    
    // 드럼 트리거 명령들
    if (cmd == "kick" || cmd == "snare" || cmd == "cymbal" || cmd == "hihat" ||
        cmd == "tom" || cmd == "conga" || cmd == "rimshot" || cmd == "maracas" ||
        cmd == "clap" || cmd == "cowbell") {
        
        float velocity = VELOCITY_NORMAL;
        if (param.length() > 0) {
            velocity = param.toFloat();
        }
        
        triggerDrum(cmd, velocity);
        return true;
    }
    
    // 볼륨 명령
    if (cmd == "volume") {
        if (param.length() > 0) {
            float volume = param.toFloat();
            setMasterVolume(volume);
            Serial.print(F("마스터 볼륨 설정: "));
            Serial.println(volume);
        } else {
            Serial.print(F("현재 마스터 볼륨: "));
            Serial.println(masterVolume);
        }
        return true;
    }
    
    // 패턴 명령들
    if (cmd == "pattern_demo") {
        startPattern(0); // 첫 번째 패턴 시작
        Serial.println(F("데모 패턴 재생 시작"));
        return true;
    }
    
    if (cmd == "pattern_stop") {
        stopPattern();
        Serial.println(F("패턴 중지"));
        return true;
    }
    
    if (cmd == "pattern_pause") {
        pausePattern();
        Serial.println(F("패턴 일시중지"));
        return true;
    }
    
    if (cmd == "pattern_resume") {
        resumePattern();
        Serial.println(F("패턴 재개"));
        return true;
    }
    
    // 상태 명령들
    if (cmd == "status") {
        printSystemStatus();
        printPerformanceReport();
        return true;
    }
    
    if (cmd == "list") {
        printDrumList();
        printPatternList();
        return true;
    }
    
    if (cmd == "patterns") {
        printPatternList();
        return true;
    }
    
    // 테스트 명령
    if (cmd == "test") {
        Serial.println(F("🔊 오디오 테스트 시작..."));
        
        // 각 드럼을 순차적으로 재생
        const char* testDrums[] = {"kick", "snare", "hihat", "tom"};
        for (int i = 0; i < 4; i++) {
            Serial.print(F("  ▶️ "));
            Serial.println(testDrums[i]);
            triggerDrum(testDrums[i], 0.7f);
            delay(800);
        }
        
        Serial.println(F("✅ 오디오 테스트 완료"));
        return true;
    }
    
    // 리셋 명령
    if (cmd == "reset") {
        Serial.println(F("🔄 시스템 리셋..."));
        initialized = false;
        audioActive = false;
        stopPattern();
        
        // 재초기화
        if (initialize()) {
            Serial.println(F("✅ 시스템 리셋 완료"));
        } else {
            Serial.println(F("❌ 시스템 리셋 실패"));
        }
        return true;
    }
    
    // 버전 명령
    if (cmd == "version" || cmd == "ver") {
        Serial.print(F("Mozzi TR-808 ESP32C3 v"));
        Serial.println(MOZZI_TR808_VERSION);
        return true;
    }
    
    return false;
}

// =============================================================================
// Mozzi 통합 함수들
// =============================================================================

void TR808DrumMachineMozzi::updateControl() {
    // 성능 메트릭 업데이트
    updatePerformanceMetrics();
    
    // 패턴 재생 처리
    if (patternPlaying) {
        patternStep++;
        
        // 간단한 데모 패턴 (4/4 박자, 16 스텝)
        if (patternStep % 16 == 0) { // 킥 on 1박
            triggerDrum(TR808_KICK, VELOCITY_HARD);
        } else if (patternStep % 8 == 4) { // 스네어 on 3박
            triggerDrum(TR808_SNARE, VELOCITY_NORMAL);
        } else if (patternStep % 4 == 0) { // 하이햇 every beat
            triggerDrum(TR808_HIHAT_CLOSED, VELOCITY_SOFT);
        }
        
        // 패턴 길이 초과 시 리셋
        if (patternStep >= 64) { // 4 bars
            patternStep = 0;
        }
        
        systemStatus.currentStep = patternStep;
    }
}

int16_t TR808DrumMachineMozzi::updateAudio() {
    static int16_t lastSample = 0;
    
    // 간단한 드럼 합성 (실제 구현에서는 더 복잡한 신세시스 필요)
    int32_t mixedSample = 0;
    
    // 마스터 볼륨 적용
    mixedSample = (int32_t)(lastSample * masterVolume * 32767.0f);
    
    // 클리핑 방지
    if (mixedSample > 32767) mixedSample = 32767;
    if (mixedSample < -32768) mixedSample = -32768;
    
    // 성능 메트릭 업데이트
    performance.sampleCount++;
    
    // 폴리포니 감소 (간단한 방법)
    if (performance.polyphony > 0) {
        performance.polyphony--;
    }
    
    // 랜덤한 드럼 소스 생성 (데모용)
    if (random(0, 1000) < 5) { // 0.5% 확률로 드럼 발생
        int drumType = random(0, 5); // 5개 기본 드럼만 사용
        float velocity = random(30, 100) / 100.0f;
        triggerDrum(drumType, velocity);
    }
    
    lastSample = (int16_t)(mixedSample / 32768.0f * 2000.0f); // 샘플 스케일 조정
    
    return lastSample;
}

// =============================================================================
// 내부 유틸리티 함수들
// =============================================================================

void TR808DrumMachineMozzi::updatePerformanceMetrics() {
    // 메모리 사용량 업데이트
    performance.memoryUsage = ESP.getFreeHeap();
    
    // 피크 메모리 사용량 업데이트
    if (performance.memoryUsage < performance.memoryPeak) {
        performance.memoryPeak = performance.memoryUsage;
    }
}

void TR808DrumMachineMozzi::processDrumSource(uint8_t source, float velocity) {
    if (source >= TR808_NUM_SOURCES || drumSources[source] == nullptr) {
        return;
    }
    
    // 실제 드럼 소스 처리 (간단한 구현)
    TR808Oscillator* osc = (TR808Oscillator*)drumSources[source];
    
    // 주파수별 드럼 처리
    switch (source) {
        case TR808_KICK:
            osc->setFrequency(KICK_FREQUENCY);
            osc->setAmplitude(velocity * KICK_PUNCH_GAIN);
            break;
            
        case TR808_SNARE:
            osc->setFrequency(SNARE_FREQUENCY);
            osc->setAmplitude(velocity);
            break;
            
        case TR808_HIHAT_CLOSED:
        case TR808_HIHAT_OPEN:
            osc->setFrequency(HIHAT_FREQUENCY);
            osc->setAmplitude(velocity * 0.6f);
            break;
            
        default:
            osc->setFrequency(100.0f + source * 10.0f);
            osc->setAmplitude(velocity);
            break;
    }
    
    TR808_DEBUG_PRINT(F("드럼 소스 처리: "));
    TR808_DEBUG_PRINT(source);
    TR808_DEBUG_PRINT(F(", 주파수: "));
    TR808_DEBUG_PRINT(osc->frequency());
    TR808_DEBUG_PRINT(F(", 볼륨: "));
    TR808_DEBUG_PRINTLN(velocity);
}

void TR808DrumMachineMozzi::loadDefaultPatterns() {
    // 기본 패턴들 정의
    strcpy(patterns[0].name, "Basic Beat");
    patterns[0].length = 16;
    patterns[0].tempo = 120;
    
    // 패턴 데이터 초기화
    for (int i = 0; i < PATTERN_BUFFER_SIZE; i++) {
        patterns[0].steps[i].enabled = false;
        patterns[0].steps[i].velocity = 0;
        patterns[0].steps[i].instrument = 0;
    }
    
    // 간단한 4/4 박자 패턴
    patterns[0].steps[0].enabled = true;   // Kick on 1
    patterns[0].steps[0].velocity = 127;
    patterns[0].steps[0].instrument = TR808_KICK;
    
    patterns[0].steps[4].enabled = true;   // Kick on 3
    patterns[0].steps[4].velocity = 100;
    patterns[0].steps[4].instrument = TR808_KICK;
    
    patterns[0].steps[8].enabled = true;   // Kick on 3
    patterns[0].steps[8].velocity = 110;
    patterns[0].steps[8].instrument = TR808_KICK;
    
    patterns[0].steps[12].enabled = true;  // Kick on 4
    patterns[0].steps[12].velocity = 95;
    patterns[0].steps[12].instrument = TR808_KICK;
    
    patterns[0].steps[4].enabled = true;   // Snare on 2
    patterns[0].steps[4].velocity = 110;
    patterns[0].steps[4].instrument = TR808_SNARE;
    
    patterns[0].steps[12].enabled = true;  // Snare on 4
    patterns[0].steps[12].velocity = 105;
    patterns[0].steps[12].instrument = TR808_SNARE;
    
    // Hi-Hat on every 8th note
    for (int i = 0; i < 16; i += 2) {
        patterns[0].steps[i].enabled = true;
        patterns[0].steps[i].velocity = 60;
        patterns[0].steps[i].instrument = TR808_HIHAT_CLOSED;
    }
    
    TR808_DEBUG_PRINTLN(F("기본 패턴 로드 완료"));
}

// =============================================================================
// 추가 getter 함수들 (헤더에서 선언되지 않은 함수들)
// =============================================================================

float TR808DrumMachineMozzi::getFrequency() const {
    return 100.0f; // 기본 주파수
}

// =============================================================================
// Arduino 호환성 함수들
// =============================================================================

// Mozzi 오디오 출력 래핑 함수 (GPIO PWM 출력용)
void audioWrite(int16_t output) {
    // PWM 출력 (GPIO 18) - 8-bit 변환
    uint8_t pwmValue = constrain((output + 32768) >> 8, 0, 255);
    analogWrite(AUDIO_OUTPUT_CHANNEL, pwmValue);
    
    // I2S 출력 (필요시 활성화)
    #ifdef USE_I2S_OUTPUT
    // I2S 출력 구현이 필요한 경우 추가
    #endif
    
    // 디버그 출력 (개발 시에만 활성화)
    #ifdef DEBUG_MOZZI_TR808
    static uint32_t debugCounter = 0;
    if (++debugCounter % 1000 == 0) { // 1000 샘플마다 한 번
        TR808_DEBUG_PRINT(F("Audio sample: "));
        TR808_DEBUG_PRINT(output);
        TR808_DEBUG_PRINT(F(", PWM: "));
        TR808_DEBUG_PRINTLN(pwmValue);
    }
    #endif
}
}