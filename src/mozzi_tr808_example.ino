/*
 * Mozzi 기반 TR-808 드럼 머신 예제
 * 
 * 64kHz 고성능 TR-808 드럼 구현
 * fastMath, generation, envelope 최적화, 폴리포니 지원
 * 
 * 작성일: 2025-10-30
 * 호환성: ESP32C3 + Mozzi Library
 */

#include <MozziGuts.h>
#include <mozzi_fixmath.h>
#include "mozzi_tr808_drums.h"
#include "mozzi_config.h"

// 전역 오브젝트
TR808DrumMachineMozzi drum_machine;

// 성능 모니터링
unsigned long last_performance_check = 0;
const unsigned long PERFORMANCE_CHECK_INTERVAL = 1000; // 1초

// 시퀀스 타이밍
unsigned long last_step_time = 0;
const unsigned long STEP_INTERVAL = 125; // 120 BPM = 500ms per beat = 125ms per 16th note

// 16步 시퀀스 패턴 (0 = rest, 1 = trigger)
const uint8_t kick_pattern[16] = {
    1,0,0,0, 0,0,0,0, 1,0,0,1, 0,0,0,0
};

const uint8_t snare_pattern[16] = {
    0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0
};

const uint8_t cymbal_pattern[16] = {
    0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0
};

const uint8_t hihat_pattern[16] = {
    1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1
};

uint8_t current_step = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== Mozzi TR-808 드럼 머신 시작 ===");
    Serial.println("64kHz 샘플링 레이트, 폴리포니 지원, fastMath 최적화");
    
    // Mozzi 초기화
    startMozzi(MOZZI_TR808_AUDIO_RATE);
    
    // 드럼 머신 초기화
    drum_machine.begin();
    
    // 성능 모니터링 활성화
    drum_machine.enablePerformanceMode(true);
    
    // 드럼 파라미터 설정
    drum_machine.setKickDecay(800.0f);      // Kick: 800ms decay
    drum_machine.setSnareDecay(400.0f);     // Snare: 400ms decay  
    drum_machine.setCymbalDecay(1200.0f);   // Cymbal: 1200ms decay
    drum_machine.setHihatDecay(150.0f);     // Hi-hat: 150ms decay
    
    // 믹스 레벨 설정
    drum_machine.setMixLevel(TR808_KICK, 0.9f);
    drum_machine.setMixLevel(TR808_SNARE, 0.8f);
    drum_machine.setMixLevel(TR808_CYMBAL, 0.7f);
    drum_machine.setMixLevel(TR808_HIHAT, 0.6f);
    
    // 마스터 설정
    drum_machine.setMasterVolume(0.8f);
    drum_machine.setMasterFilterCutoff(12000.0f);
    
    Serial.println("드럼 머신 초기화 완료");
    Serial.println("시퀀스 시작...");
    
    // 초기 시퀀스 출력
    printPatternInfo();
}

void loop() {
    audioHook(); // Mozzi 오디오 처리
    
    unsigned long current_time = millis();
    
    // 시퀀스 처리 (16步 패턴)
    if (current_time - last_step_time >= STEP_INTERVAL) {
        processSequenceStep(current_step);
        current_step = (current_step + 1) % 16;
        last_step_time = current_time;
    }
    
    // 성능 모니터링 (1초마다)
    if (current_time - last_performance_check >= PERFORMANCE_CHECK_INTERVAL) {
        printPerformanceReport();
        last_performance_check = current_time;
    }
    
    // 모든 드럼이 종료되었는지 확인
    if (!drum_machine.isAnyVoicePlaying() && current_step == 0) {
        Serial.println("시퀀스 완료, 재시작...");
        delay(1000);
    }
}

// =============================================================================
// 시퀀스 처리 함수들
// =============================================================================

void processSequenceStep(uint8_t step) {
    // Kick 트리거
    if (kick_pattern[step]) {
        drum_machine.triggerKick();
        Serial.print("K");
    }
    
    // Snare 트리거
    if (snare_pattern[step]) {
        drum_machine.triggerSnare();
        Serial.print("S");
    }
    
    // Cymbal 트리거
    if (cymbal_pattern[step]) {
        drum_machine.triggerCymbal();
        Serial.print("C");
    }
    
    // Hi-hat 트리거 (매 스텝)
    if (hihat_pattern[step]) {
        drum_machine.triggerHihat();
        Serial.print("H");
    }
    
    Serial.print(" ");
    
    // 4步마다 새로운 줄
    if (step % 4 == 3) {
        Serial.println();
    }
}

void printPatternInfo() {
    Serial.println("\n=== TR-808 패턴 정보 ===");
    Serial.println("BPM: 120 (125ms per 16th note)");
    Serial.println("패턴:");
    
    Serial.print("Kick:  ");
    for (int i = 0; i < 16; i++) {
        Serial.print(kick_pattern[i] ? "● " : "○ ");
    }
    Serial.println();
    
    Serial.print("Snare: ");
    for (int i = 0; i < 16; i++) {
        Serial.print(snare_pattern[i] ? "● " : "○ ");
    }
    Serial.println();
    
    Serial.print("Cymbal: ");
    for (int i = 0; i < 16; i++) {
        Serial.print(cymbal_pattern[i] ? "● " : "○ ");
    }
    Serial.println();
    
    Serial.print("Hi-hat: ");
    for (int i = 0; i < 16; i++) {
        Serial.print(hihat_pattern[i] ? "● " : "○ ");
    }
    Serial.println();
    Serial.println();
}

void printPerformanceReport() {
    Serial.println("=== 성능 리포트 ===");
    
    uint32_t processing_time = drum_machine.getProcessingTime();
    uint32_t max_processing_time = drum_machine.getMaxProcessingTime();
    float cpu_usage = drum_machine.getCPUUsage();
    
    Serial.printf("현재 처리 시간: %lu μs\n", processing_time);
    Serial.printf("최대 처리 시간: %lu μs\n", max_processing_time);
    Serial.printf("CPU 사용률: %.2f%%\n", cpu_usage);
    
    // 가용 시간 계산 (64kHz 기준)
    uint32_t available_time_us = 1000000UL / MOZZI_TR808_AUDIO_RATE;
    Serial.printf("가용 처리 시간: %lu μs\n", available_time_us);
    
    if (processing_time > available_time_us) {
        Serial.println("⚠️  처리 시간 초과! 성능 최적화 필요");
    } else {
        Serial.println("✅ 성능 양호");
    }
    
    Serial.printf("버퍼 지연 시간: %.1f ms\n", BUFFER_LATENCY_MS);
    Serial.printf("샘플 간격: %.1f μs\n", SAMPLE_INTERVAL_US);
    Serial.println();
}

// =============================================================================
// Mozzi 콜백 함수들
// =============================================================================

/**
 * Mozzi 오디오 업데이트 (64kHz)
 * ISR에서 호출됨
 */
AudioOutput_t updateAudio() {
    // 드럼 머신에서 오디오 샘플 생성
    Q15n16 audio_sample = drum_machine.next();
    
    // Mozzi 포맷으로 변환
    return MonoOutput::from16Bit(audio_sample);
}

/**
 * Mozzi 컨트롤 업데이트 (512Hz)
 * 드럼 파라미터 업데이트, envelope 처리 등
 */
void updateControl() {
    drum_machine.update();
    
    // 추가적인 컨트롤 로직이 있다면 여기서 처리
    // 예: MIDI 입력, 시퀀스 제어, 파라미터 변화 등
}

// =============================================================================
// 추가 테스트 함수들
// =============================================================================

void testIndividualDrums() {
    Serial.println("=== 개별 드럼 테스트 ===");
    
    // Kick 테스트
    Serial.println("Kick 테스트...");
    drum_machine.triggerKick();
    delay(1000);
    
    // Snare 테스트
    Serial.println("Snare 테스트...");
    drum_machine.triggerSnare();
    delay(1000);
    
    // Cymbal 테스트
    Serial.println("Cymbal 테스트...");
    drum_machine.triggerCymbal();
    delay(1500);
    
    // Hi-hat 테스트
    Serial.println("Hi-hat 테스트...");
    drum_machine.triggerHihat();
    delay(500);
    
    Serial.println("개별 테스트 완료\n");
}

void testPolyphony() {
    Serial.println("=== 폴리포니 테스트 ===");
    
    // 여러 드럼 동시 재생
    Serial.println("다중 드럼 재생...");
    
    for (int i = 0; i < 4; i++) {
        drum_machine.triggerKick();
        delay(100);
        drum_machine.triggerSnare();
        delay(100);
        drum_machine.triggerCymbal();
        delay(100);
        drum_machine.triggerHihat();
        delay(100);
    }
    
    Serial.println("폴리포니 테스트 완료\n");
}

void testParameterChanges() {
    Serial.println("=== 파라미터 변화 테스트 ===");
    
    // Decay time 변화 테스트
    Serial.println("Kick decay 시간 변화...");
    
    float decay_times[] = {200.0f, 500.0f, 800.0f, 1200.0f};
    
    for (int i = 0; i < 4; i++) {
        drum_machine.setKickDecay(decay_times[i]);
        Serial.printf("Decay 시간: %.0f ms\n", decay_times[i]);
        
        drum_machine.triggerKick();
        delay(1000);
    }
    
    // Mix level 변화 테스트
    Serial.println("Mix level 변화 테스트...");
    
    float mix_levels[] = {0.3f, 0.6f, 0.9f, 1.0f};
    
    for (int i = 0; i < 4; i++) {
        drum_machine.setMixLevel(TR808_KICK, mix_levels[i]);
        Serial.printf("Mix level: %.1f\n", mix_levels[i]);
        
        drum_machine.triggerKick();
        delay(800);
    }
    
    Serial.println("파라미터 테스트 완료\n");
}