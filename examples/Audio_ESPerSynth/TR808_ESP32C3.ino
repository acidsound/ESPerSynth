/*
 * ESP32C3 TR-808 드럼 머신 - Audio.h 라이브러리 버전
 * 
 * Roland TR-808의 전설적인 드럼 사운드를 ESP32C3에서 구현
 * Audio.h 라이브러리를 사용한 안정적인 I2S 오디오 출력
 * 
 * 하드웨어 요구사항:
 * - ESP32C3 개발보드
 * - 외부 I2S DAC (PCM5102, ES9023 등)  
 * - 오디오 앰프 및 스피커
 * 
 * 핀 연결:
 * - GPIO 2: I2S Word Select (LRCLK/WS)
 * - GPIO 3: I2S Bit Clock (BCLK)
 * - GPIO 4: I2S Data Out (DOUT)
 * - GND: 공통 그라운드
 * 
 * 라이브러리:
 * - ESP8266Audio by earloffilhower (Audio.h)
 * - ESP32 Arduino Core 2.0.18
 * 
 * 제작: 2025-10-30
 * 버전: 2.0.0 (Audio.h 버전)
 */

#include "Arduino.h"
#include "Audio.h"
#include "tr808_drums.h"

// ============================================
// I2S 설정 (ESP32C3 최적화)
// ============================================

// Audio.h를 사용한 I2S 핀 설정
#define I2S_DOUT 4   // ESP32C3 GPIO 4 (Data Out)
#define I2S_BCLK 3   // ESP32C3 GPIO 3 (Bit Clock)  
#define I2S_LRC 2    // ESP32C3 GPIO 2 (Word Select)

// ============================================
// 전역 변수 및 인스턴스
// ============================================

// Audio.h 인스턴스 (I2S 오디오 출력)
Audio audio;

// 메인 TR808 드럼 머신
TR808DrumMachine drumMachine;

// 성능 모니터링 변수
unsigned long lastPerfUpdate = 0;
const unsigned long PERF_UPDATE_INTERVAL = 2000; // 2초마다
float peakAudioLevel = 0.0f;
int commandCount = 0;

// ============================================
// Audio.h 오디오 콜백 함수
// ============================================

// 오디오 샘플 생성 콜백 (Audio.h에서 호출)
void audioGenerateSample(int16_t sample) {
    // 드럼 머신에서 현재 샘플 생성
    float audioSample = drumMachine.process();
    
    // 파워 레벨 모니터링
    float absLevel = abs(audioSample);
    if (absLevel > peakAudioLevel) {
        peakAudioLevel = absLevel;
    }
    
    // 16-bit 샘플로 변환
    int16_t outputSample = (int16_t)(audioSample * 32767.0f);
    
    // Audio.h 콜백에 전달
    return outputSample;
}

// ============================================
// 초기화 함수들
// ============================================

void initializeAudio() {
    Serial.println("=== I2S 오디오 시스템 초기화 ===");
    
    // Audio.h I2S 핀 설정
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    Serial.println("✓ I2S 핀 설정 완료 (GPIO 2,3,4)");
    
    // 오디오 볼륨 설정 (0-21 범위)
    audio.setVolume(18); // 중등 볼륨
    Serial.println("✓ 오디오 볼륨 설정 완료 (18/21)");
    
    // 샘플레이트 확인
    Serial.printf("✓ 샘플레이트: %d Hz\n", audio.getSampleRate());
    Serial.println("=== I2S 시스템 준비 완료 ===\n");
}

void initializeDrumMachine() {
    Serial.println("=== TR-808 드럼 머신 초기화 ===");
    
    // 마스터 볼륨 설정
    drumMachine.setMasterVolume(0.8f);
    Serial.println("✓ 마스터 볼륨: 80%");
    
    // 드럼 엔진 준비 완료
    Serial.println("✓ 10개 드럼 소스 준비 완료");
    Serial.println("  - Kick, Snare, Cymbal, Hi-hat");
    Serial.println("  - Tom, Conga, Rimshot, Maracas");  
    Serial.println("  - Clap, Cowbell");
    Serial.println("=== 드럼 머신 준비 완료 ===\n");
}

// ============================================
// Serial 명령 처리
// ============================================

void processSerialCommand(String command) {
    command.trim();
    command.toLowerCase();
    
    Serial.printf(">> '%s'\n", command.c_str());
    commandCount++;
    
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
    
    // 볼륨 제어
    else if (command.startsWith("volume ")) {
        float volume = command.substring(7).toFloat();
        if (volume >= 0.0f && volume <= 1.0f) {
            drumMachine.setMasterVolume(volume);
            Serial.printf("✓ 볼륨 설정: %.1f%%\n", volume * 100);
        } else {
            Serial.println("✗ 볼륨 범위: 0.0 - 1.0");
        }
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
        Serial.println("✓ 시스템 리셋");
    }
    else if (command == "clear") {
        peakAudioLevel = 0.0f;
        commandCount = 0;
        Serial.println("✓ 통계 초기화");
    }
    
    else if (command.length() > 0) {
        Serial.println("✗ 알 수 없는 명령어. 'help' 입력");
    }
}

// ============================================
// 패턴 및 테스트 함수들
// ============================================

void startDemoPattern() {
    // 간단한 4/4 박자 패턴
    drumMachine.startPattern();
    
    // kick, snare, hi-hat 패턴 생성
    drumMachine.addPatternStep(0, "kick");    // 1박
    drumMachine.addPatternStep(2, "snare");   // 3박  
    drumMachine.addPatternStep(4, "kick");    // 5박
    drumMachine.addPatternStep(6, "snare");   // 7박
    
    // 연속 hi-hat
    for (int i = 0; i < 16; i++) {
        drumMachine.addPatternStep(i, "hihat");
    }
}

void runAudioTest() {
    Serial.println("=== 오디오 테스트 시작 ===");
    
    Serial.println("테스트 1: Kick 드럼");
    drumMachine.triggerKick();
    delay(500);
    
    Serial.println("테스트 2: Snare 드럼");  
    drumMachine.triggerSnare();
    delay(500);
    
    Serial.println("테스트 3: Cymbal 드럼");
    drumMachine.triggerCymbal();
    delay(1000);
    
    Serial.println("=== 오디오 테스트 완료 ===");
}

// ============================================
// 상태 출력 및 도움말
// ============================================

void printSystemStatus() {
    Serial.println("\n=== ESP32C3 TR-808 상태 ===");
    Serial.printf("CPU 사용률: %d%% ( 추정 )\n", drumMachine.getCPUUsage());
    Serial.printf("RAM 사용량: %d/%d bytes\n", drumMachine.getRAMUsage(), getTotalRAM());
    Serial.printf("드럼 엔진: %s\n", drumMachine.isActive() ? "활성" : "비활성");
    Serial.printf("마스터 볼륨: %.1f%%\n", drumMachine.getMasterVolume() * 100);
    Serial.printf("파워 레벨: %.3f\n", peakAudioLevel);
    Serial.printf("총 명령어: %d개\n", commandCount);
    Serial.println("========================\n");
}

void printHelp() {
    Serial.println("\n=== ESP32C3 TR-808 명령어 ===");
    Serial.println("기본 드럼:");
    Serial.println("  kick, snare, cymbal, hihat");
    Serial.println("  tom, conga, rimshot, maracas");
    Serial.println("  clap, cowbell");
    Serial.println("");
    Serial.println("패턴:");
    Serial.println("  pattern_demo - 데모 패턴");
    Serial.println("  pattern_stop - 패턴 정지");
    Serial.println("");
    Serial.println("볼륨:");
    Serial.println("  volume 0.0~1.0 - 마스터 볼륨");
    Serial.println("");
    Serial.println("시스템:");
    Serial.println("  status - 시스템 상태");
    Serial.println("  test - 오디오 테스트");
    Serial.println("  reset - 시스템 리셋");
    Serial.println("  clear - 통계 초기화");
    Serial.println("  help - 도움말");
    Serial.println("============================\n");
}

// ============================================
// 메인 함수들
// ============================================

void setup() {
    Serial.begin(115200);
    delay(2000); // 시리얼 연결 안정화
    
    Serial.println("\n");
    Serial.println("██╗  ██╗ █████╗ ███████╗ █████╗     ██████╗");
    Serial.println("██║ ██╔╝██╔══██╗██╔════╝██╔══██╗    ██╔══██╗");
    Serial.println("█████╔╝ ╚█████╔╝███████╗███████║    ██║  ██║");
    Serial.println("██╔═██╗██╔══██╗╚════██║██╔══██║    ██║  ██║");
    Serial.println("██║  ██╗╚█████╔╝███████║██║  ██║    ██████╔╝");
    Serial.println("╚═╝  ╚═╝ ╚════╝ ╚══════╝╚═╝  ╚═╝    ╚═════╝");
    Serial.println("");
    Serial.println("ESP32C3 TR-808 드럼 머신 (Audio.h 버전)");
    Serial.println("================================================");
    
    // 초기화 단계별 실행
    initializeAudio();
    initializeDrumMachine();
    
    // 시작 메시지
    Serial.println("🎵 시스템 준비 완료! Serial 명령어를 입력하세요.");
    Serial.println("💡 도움말: 'help' 명령어 입력");
    printHelp();
    
    // 초기 오디오 테스트
    drumMachine.triggerKick();
    delay(100);
    drumMachine.triggerSnare();
    
    Serial.println("▶️  준비 완료 - 명령 대기 중...\n");
}

void loop() {
    // Audio.h 오디오 처리 (내부적으로 자동 처리됨)
    // audio.loop(); // Audio.h 라이브러리가 내부적으로 처리
    
    // Serial 명령 처리
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        processSerialCommand(command);
    }
    
    // 성능 모니터링 (2초마다)
    unsigned long now = millis();
    if (now - lastPerfUpdate >= PERF_UPDATE_INTERVAL) {
        // 파워 레벨 리셋 (자동)
        peakAudioLevel *= 0.9f;
        lastPerfUpdate = now;
        
        // 간단한 하트비트 출력 (선택사항)
        // Serial.print(".");
    }
    
    // 시스템 안정화 지연
    delay(1);
}

// ============================================
// 유틸리티 함수들
// ============================================

int getTotalRAM() {
    return 400000; // ESP32C3 пример값
}