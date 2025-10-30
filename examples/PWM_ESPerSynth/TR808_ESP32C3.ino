/*
 * ESP32C3 TR-808 드럼 머신 - I2S.h 직접 구현 버전 (PWM 출력)
 * 
 * Roland TR-808의 전설적인 드럼 사운드를 ESP32C3에서 구현
 * I2S.h 라이브러리와 PWM 출력을 사용한 저메모리 버전
 * 
 * 하드웨어 요구사항:
 * - ESP32C3 개발보드
 * - PWM 오디오 앰프 (PAM8403, LM386 등)
 * - 스피커
 * 
 * 핀 연결:
 * - GPIO 18: PWM 오디오 출력 (8-bit)
 * - GPIO 9: 부트 버튼 (업로드 시 필요)
 * - 5V: 앰프 전원
 * - GND: 공통 그라운드
 * 
 * 라이브러리:
 * - ESP32 Arduino Core 2.0.18 (3.x 버전 아님)
 * - I2S.h (ESP32 내장)
 * 
 * 제작: 2025-10-30
 * 버전: 2.1.0 (I2S.h + PWM 버전)
 */

#include <I2S.h>
#include "tr808_drums.h"

// ============================================
// PWM 오디오 출력 설정 (ESP32C3 최적화)
// ============================================

// PWM 설정
#define PWM_PIN 18              // GPIO 18 (PWM 출력)
#define PWM_CHANNEL 0           // PWM 채널 0
#define PWM_FREQUENCY 44100     // 44.1kHz PWM 주파수
#define PWM_RESOLUTION 8        // 8-bit 해상도
#define PWM_RANGE 255           // PWM 값 범위

// I2S 설정 (타이밍 생성용)
#define SAMPLE_RATE 8000        // 8kHz 오디오 샘플레이트
#define BUFFER_SIZE 128         // I2S 버퍼 크기
#define BITS_PER_SAMPLE 16      // 16-bit 샘플

// ============================================
// 전역 변수 및 인스턴스
// ============================================

// I2S 버퍼 (오디오 샘플링용)
short i2sBuffer[BUFFER_SIZE];
int bufferIndex = 0;

// 메인 TR808 드럼 머신
TR808DrumMachine drumMachine;

// 성능 모니터링
unsigned long lastPerfUpdate = 0;
const unsigned long PERF_UPDATE_INTERVAL = 2000;
float currentAudioLevel = 0.0f;
int bufferUnderrun = 0;
unsigned long totalSamples = 0;

// PWM 기반 오디오 제어를 위한 변수
bool audioEnabled = true;
uint8_t lastPWMValue = 0;
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 100; // 10kHz 업데이트 주기

// ============================================
// PWM 오디오 출력 함수
// ============================================

void initializePWMAudio() {
    Serial.println("=== PWM 오디오 시스템 초기화 ===");
    
    // PWM 채널 설정
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(PWM_PIN, PWM_CHANNEL);
    Serial.printf("✓ PWM 채널 %d 설정: GPIO %d, %dHz, %d-bit\n", 
                  PWM_CHANNEL, PWM_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
    
    // PWM duty cycle 초기화
    ledcWrite(PWM_CHANNEL, 0);
    Serial.println("✓ PWM 출력 초기화 완료");
    
    audioEnabled = true;
    Serial.println("=== PWM 시스템 준비 완료 ===\n");
}

void updatePWMAudio() {
    if (!audioEnabled) return;
    
    unsigned long currentTime = micros();
    if (currentTime - lastUpdateTime < UPDATE_INTERVAL) return;
    lastUpdateTime = currentTime;
    
    // TR-808 드럼 엔진에서 현재 샘플 생성
    float audioSample = drumMachine.process();
    
    // 8-bit PWM 값으로 변환 (0-255)
    //中心값을 128로 설정하여 양과 음의 음성 모두 처리
    int pwmValue = (int)((audioSample + 1.0f) * 127.5f);
    
    // 0-255 범위로 제한
    pwmValue = constrain(pwmValue, 0, 255);
    
    // PWM 출력 업데이트
    ledcWrite(PWM_CHANNEL, pwmValue);
    lastPWMValue = pwmValue;
    
    // 오디오 레벨 모니터링
    float absLevel = abs(audioSample);
    if (absLevel > currentAudioLevel) {
        currentAudioLevel = absLevel;
    }
    
    totalSamples++;
}

// ============================================
// I2S 초기화 및 처리 (내부 타이밍용)
// ============================================

void initializeI2S() {
    Serial.println("=== I2S 타이밍 시스템 초기화 ===");
    
    // I2S 핀 설정 (사용하지 않지만 타이밍 생성을 위해 필요)
    I2S.setAllPins(-1, -1, -1, -1, -1); // 빈 핀 설정
    
    // I2S 버퍼 초기화
    for (int i = 0; i < BUFFER_SIZE; i++) {
        i2sBuffer[i] = 0;
    }
    bufferIndex = 0;
    
    Serial.println("✓ I2S 버퍼 초기화 완료");
    Serial.printf("✓ 샘플레이트: %d Hz, 버퍼 크기: %d\n", 
                  SAMPLE_RATE, BUFFER_SIZE);
    Serial.println("=== I2S 타이밍 시스템 준비 완료 ===\n");
}

void processI2SBuffer() {
    // I2S 버퍼에 PWM 기반 오디오 샘플 추가
    if (bufferIndex < BUFFER_SIZE) {
        // PWM 출력에서 오디오 샘플 읽기
        short sample = (short)((lastPWMValue - 128) * 256); // 16-bit로 변환
        i2sBuffer[bufferIndex] = sample;
        bufferIndex++;
        
        // 버퍼가 가득 차면 전송 (개념적, 실제로는 사용하지 않음)
        if (bufferIndex >= BUFFER_SIZE) {
            // I2S.write(i2sBuffer, BUFFER_SIZE * sizeof(short));
            bufferIndex = 0; // 버퍼 재사용
        }
    }
}

// ============================================
// Serial 명령 처리
// ============================================

void processSerialCommand(String command) {
    command.trim();
    command.toLowerCase();
    
    Serial.printf(">> '%s'\n", command.c_str());
    
    // 기본 드럼 명령어들
    if (command == "kick") {
        drumMachine.triggerKick();
        Serial.println("✓ Kick 트리거");
    }
    else if (command == "snare") {
        drumMachine.triggerSnare();
        Serial.println("✓ Snare 트리거");
    }
    else if (command == "cymbal") {
        drumMachine.triggerCymbal();
        Serial.println("✓ Cymbal 트리거");
    }
    else if (command == "hihat") {
        drumMachine.triggerHihat();
        Serial.println("✓ Hi-hat 트리거");
    }
    else if (command == "tom") {
        drumMachine.triggerTom();
        Serial.println("✓ Tom 트리거");
    }
    else if (command == "conga") {
        drumMachine.triggerConga();
        Serial.println("✓ Conga 트리거");
    }
    else if (command == "rimshot") {
        drumMachine.triggerRimshot();
        Serial.println("✓ Rimshot 트리거");
    }
    else if (command == "maracas") {
        drumMachine.triggerMaracas();
        Serial.println("✓ Maracas 트리거");
    }
    else if (command == "clap") {
        drumMachine.triggerClap();
        Serial.println("✓ Clap 트리거");
    }
    else if (command == "cowbell") {
        drumMachine.triggerCowbell();
        Serial.println("✓ Cowbell 트리거");
    }
    
    // 패턴 명령어들
    else if (command == "pattern_demo") {
        startDemoPattern();
        Serial.println("✓ 데모 패턴 시작");
    }
    else if (command == "pattern_stop") {
        drumMachine.stopPattern();
        Serial.println("✓ 패턴 정지");
    }
    else if (command == "pattern_play") {
        drumMachine.startPattern();
        Serial.println("✓ 패턴 재생 시작");
    }
    
    // 오디오 제어
    else if (command.startsWith("volume ")) {
        float volume = command.substring(7).toFloat();
        if (volume >= 0.0f && volume <= 1.0f) {
            drumMachine.setMasterVolume(volume);
            Serial.printf("✓ 볼륨 설정: %.1f%%\n", volume * 100);
        } else {
            Serial.println("✗ 볼륨 범위: 0.0 - 1.0");
        }
    }
    else if (command == "audio_on") {
        audioEnabled = true;
        ledcWrite(PWM_CHANNEL, 0); // 소거
        Serial.println("✓ 오디오 활성화");
    }
    else if (command == "audio_off") {
        audioEnabled = false;
        ledcWrite(PWM_CHANNEL, 128); // 중심값 (무음)
        Serial.println("✓ 오디오 비활성화");
    }
    
    // 시스템 명령어들
    else if (command == "status") {
        printSystemStatus();
    }
    else if (command == "help") {
        printHelp();
    }
    else if (command == "test") {
        runAudioTest();
    }
    else if (command == "reset") {
        drumMachine.reset();
        audioEnabled = true;
        ledcWrite(PWM_CHANNEL, 128);
        Serial.println("✓ 시스템 리셋");
    }
    else if (command == "info") {
        printSystemInfo();
    }
    else if (command == "buffer") {
        Serial.printf("✓ 버퍼 상태: %d/%d (언더런: %d)\n", 
                      bufferIndex, BUFFER_SIZE, bufferUnderrun);
    }
    
    else if (command.length() > 0) {
        Serial.println("✗ 알 수 없는 명령어. 'help' 입력");
    }
}

// ============================================
// 패턴 및 테스트 함수들
// ============================================

void startDemoPattern() {
    // 4/4박자 데모 패턴
    drumMachine.startPattern();
    
    // 기본 패턴 생성
    drumMachine.addPatternStep(0, "kick");    // 1박
    drumMachine.addPatternStep(4, "kick");    // 3박
    drumMachine.addPatternStep(8, "kick");    // 5박
    drumMachine.addPatternStep(12, "kick");   // 7박
    
    drumMachine.addPatternStep(2, "snare");   // 2박
    drumMachine.addPatternStep(6, "snare");   // 4박
    drumMachine.addPatternStep(10, "snare");  // 6박
    drumMachine.addPatternStep(14, "snare");  // 8박
    
    // 연속 hi-hat
    for (int i = 0; i < 16; i++) {
        if (i % 2 == 0) { // 8분음표
            drumMachine.addPatternStep(i, "hihat");
        }
    }
}

void runAudioTest() {
    Serial.println("=== PWM 오디오 테스트 시작 ===");
    
    Serial.println("테스트 1: Kick 드럼");
    drumMachine.triggerKick();
    delay(500);
    
    Serial.println("테스트 2: Snare 드럼");
    drumMachine.triggerSnare();
    delay(500);
    
    Serial.println("테스트 3: Hi-hat 스트링");
    for (int i = 0; i < 8; i++) {
        drumMachine.triggerHihat();
        delay(200);
    }
    
    Serial.println("=== PWM 오디오 테스트 완료 ===");
}

// ============================================
// 상태 출력 및 도움말
// ============================================

void printSystemStatus() {
    Serial.println("\n=== ESP32C3 TR-808 (I2S+PWM) 상태 ===");
    Serial.printf("PWM 출력: GPIO %d, 채널 %d\n", PWM_PIN, PWM_CHANNEL);
    Serial.printf("PWM 주파수: %d Hz, 해상도: %d-bit\n", PWM_FREQUENCY, PWM_RESOLUTION);
    Serial.printf("오디오 샘플레이트: %d Hz\n", SAMPLE_RATE);
    Serial.printf("I2S 버퍼: %d/%d (언더런: %d)\n", bufferIndex, BUFFER_SIZE, bufferUnderrun);
    Serial.printf("오디오 상태: %s\n", audioEnabled ? "활성" : "비활성");
    Serial.printf("PWM 값: %d (0-255)\n", lastPWMValue);
    Serial.printf("오디오 레벨: %.3f\n", currentAudioLevel);
    Serial.printf("총 샘플: %lu\n", totalSamples);
    Serial.printf("마스터 볼륨: %.1f%%\n", drumMachine.getMasterVolume() * 100);
    Serial.printf("드럼 엔진: %s\n", drumMachine.isActive() ? "활성" : "비활성");
    Serial.println("===============================\n");
}

void printSystemInfo() {
    Serial.println("\n=== 시스템 정보 ===");
    Serial.printf("ESP32C3 TR-808 드럼 머신 v2.1.0\n");
    Serial.printf("I2S.h + PWM 오디오 출력\n");
    Serial.printf("Arduino Core: %s\n", ARDUINO);
    Serial.printf("Chip: ESP32-C3 (RISC-V)\n");
    Serial.printf("Flash: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("RAM: %d bytes 사용가능\n", ESP.getFreeHeap());
    Serial.printf("CPU: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.println("===================\n");
}

void printHelp() {
    Serial.println("\n=== ESP32C3 TR-808 (I2S+PWM) 명령어 ===");
    Serial.println("기본 드럼:");
    Serial.println("  kick, snare, cymbal, hihat");
    Serial.println("  tom, conga, rimshot, maracas");
    Serial.println("  clap, cowbell");
    Serial.println("");
    Serial.println("패턴:");
    Serial.println("  pattern_demo - 데모 패턴");
    Serial.println("  pattern_play - 패턴 재생");
    Serial.println("  pattern_stop - 패턴 정지");
    Serial.println("");
    Serial.println("오디오 제어:");
    Serial.println("  volume 0.0~1.0 - 마스터 볼륨");
    Serial.println("  audio_on/off - 오디오 활성화/비활성화");
    Serial.println("");
    Serial.println("시스템:");
    Serial.println("  status - 시스템 상태");
    Serial.println("  info - 상세 정보");
    Serial.println("  test - 오디오 테스트");
    Serial.println("  buffer - 버퍼 상태");
    Serial.println("  reset - 시스템 리셋");
    Serial.println("  help - 도움말");
    Serial.println("=====================================\n");
}

// ============================================
// 메인 함수들
// ============================================

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n");
    Serial.println("██╗  ██╗ █████╗ ███████╗ █████╗     ██████╗");
    Serial.println("██║ ██╔╝██╔══██╗██╔════╝██╔══██╗    ██╔══██╗");
    Serial.println("█████╔╝ ╚█████╔╝███████╗███████║    ██║  ██║");
    Serial.println("██╔═██╗██╔══██╗╚════██║██╔══██║    ██║  ██║");
    Serial.println("██║  ██╗╚█████╔╝███████║██║  ██║    ██████╔╝");
    Serial.println("╚═╝  ╚═╝ ╚════╝ ╚══════╝╚═╝  ╚═╝    ╚═════╝");
    Serial.println("");
    Serial.println("ESP32C3 TR-808 드럼 머신 (I2S+PWM)");
    Serial.println("================================================");
    
    // 시스템 정보 출력
    printSystemInfo();
    
    // 초기화 단계별 실행
    initializePWMAudio();
    initializeI2S();
    
    // 드럼 머신 초기화
    drumMachine.setMasterVolume(0.6f); // PWM 출력용 중간 볼륨
    Serial.println("✓ 마스터 볼륨: 60% (PWM 최적화)");
    Serial.println("✓ 10개 드럼 소스 준비 완료");
    
    // 시작 메시지
    Serial.println("🎵 PWM 오디오 시스템 준비 완료!");
    Serial.println("💡 GPIO 18에서 PWM 오디오 출력");
    Serial.println("💡 'help' 명령어로 사용법 확인");
    printHelp();
    
    // 초기 테스트
    drumMachine.triggerKick();
    delay(200);
    drumMachine.triggerSnare();
    
    Serial.println("▶️  준비 완료 - 명령 대기 중...\n");
}

void loop() {
    // PWM 오디오 업데이트 (10kHz)
    updatePWMAudio();
    
    // I2S 버퍼 처리 (개념적)
    processI2SBuffer();
    
    // Serial 명령 처리
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        processSerialCommand(command);
    }
    
    // 성능 모니터링 (2초마다)
    unsigned long now = millis();
    if (now - lastPerfUpdate >= PERF_UPDATE_INTERVAL) {
        // 오디오 레벨 자동 감쇠
        currentAudioLevel *= 0.95f;
        
        // 간단한 하트비트
        Serial.print(".");
        
        lastPerfUpdate = now;
    }
    
    // 짧은 지연으로 안정화
    delay(1);
}