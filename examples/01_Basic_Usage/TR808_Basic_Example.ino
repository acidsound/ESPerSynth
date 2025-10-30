/*
 * ESP32C3 TR-808 드럼 머신 I2S 출력 예제
 * 
 * 하드웨어 연결:
 * - I2S WS (LRCLK): GPIO 3
 * - I2S SCK (BCLK): GPIO 2  
 * - I2S SD (Data): GPIO 1
 * - GND: Common ground
 * 
 * 필요한 라이브러리:
 * - I2S library (ESP32 내장)
 * 
 * 오디오 품질 설정:
 * - 샘플 레이트: 32.768kHz
 * - 비트 심도: 16-bit
 * - 채널: 모노
 */

#include <I2S.h>
#include "tr808_drums.h"

// I2S 설정 상수
#define SAMPLE_RATE 32768
#define BUFFER_SIZE 256
#define I2S_PORT I2S_NUM_0

// 전역 드럼 머신 인스턴스
TR808DrumMachine drumMachine;

// I2S 버퍼
int16_t i2s_buffer[BUFFER_SIZE];

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("ESP32C3 TR-808 드럼 머신 시작...");
    
    // I2S 초기화
    if (!I2S.begin(I2S_STANDARD, SAMPLE_RATE, 16, 1)) {
        Serial.println("I2S 초기화 실패!");
        while(1) delay(100);
    }
    
    Serial.println("I2S 초기화 성공!");
    
    // 드럼 머신 기본 설정
    drumMachine.setMasterVolume(0.8f);
    
    // 드럼별 세부 설정
    drumMachine.setKickDecay(500.0f);    // 킥 디케이 500ms
    drumMachine.setKickTone(0.5f);       // 킥 톤 중간
    drumMachine.setSnareTone(0.7f);      // 스네어 톤 밝게
    drumMachine.setSnareSnappy(0.8f);    // 스네어 스내피 강화
    drumMachine.setCymbalDecay(800.0f);  // 심벌 디케이 800ms
    drumMachine.setCymbalTone(0.6f);     // 심벌 톤 중간
    drumMachine.setHiHatDecay(50.0f);    // 클로즈드 햇
    drumMachine.setTomTuning(165.0f);    // 톰 튜닝
    drumMachine.setCongaTuning(370.0f);  // 콩가 튜닝
    
    Serial.println("TR-808 드럼 머신 준비 완료!");
    printInstructions();
}

void loop() {
    //Serial 명령 처리
    if (Serial.available()) {
        String command = Serial.readString();
        command.trim();
        
        if (command == "kick" || command == "1") {
            drumMachine.triggerKick(1.0f);
            Serial.println("Kick triggered!");
        }
        else if (command == "snare" || command == "2") {
            drumMachine.triggerSnare(1.0f);
            Serial.println("Snare triggered!");
        }
        else if (command == "cymbal" || command == "3") {
            drumMachine.triggerCymbal(1.0f);
            Serial.println("Cymbal triggered!");
        }
        else if (command == "hihat" || command == "4") {
            drumMachine.triggerHiHat(1.0f, false);
            Serial.println("Closed Hi-Hat triggered!");
        }
        else if (command == "openhihat" || command == "5") {
            drumMachine.triggerHiHat(1.0f, true);
            Serial.println("Open Hi-Hat triggered!");
        }
        else if (command == "tom" || command == "6") {
            drumMachine.triggerTom(1.0f);
            Serial.println("Tom triggered!");
        }
        else if (command == "conga" || command == "7") {
            drumMachine.triggerConga(1.0f);
            Serial.println("Conga triggered!");
        }
        else if (command == "rimshot" || command == "8") {
            drumMachine.triggerRimshot(1.0f);
            Serial.println("Rimshot triggered!");
        }
        else if (command == "maracas" || command == "9") {
            drumMachine.triggerMaracas(1.0f);
            Serial.println("Maracas triggered!");
        }
        else if (command == "clap" || command == "0") {
            drumMachine.triggerClap(1.0f);
            Serial.println("Clap triggered!");
        }
        else if (command == "cowbell" || command == "c") {
            drumMachine.triggerCowbell(1.0f);
            Serial.println("Cowbell triggered!");
        }
        else if (command.startsWith("kick ")) {
            float level = command.substring(5).toFloat();
            drumMachine.triggerKick(level);
            Serial.println("Kick triggered with velocity: " + String(level));
        }
        else if (command.startsWith("master ")) {
            float level = command.substring(7).toFloat();
            drumMachine.setMasterVolume(level);
            Serial.println("Master volume set to: " + String(level));
        }
        else if (command == "help" || command == "h") {
            printInstructions();
        }
        else if (command == "status" || command == "s") {
            printStatus();
        }
    }
    
    // 오디오 처리 및 I2S 출력
    processAudio();
    
    delayMicroseconds(30); // 안정화를 위한 짧은 지연
}

void processAudio() {
    // 드럼 머신에서 오디오 샘플 생성
    float audioSample = drumMachine.process();
    
    // 32-bit float를 16-bit int로 변환
    int16_t intSample = (int16_t)(audioSample * 32767);
    
    // I2S 버퍼에 채우기
    for (int i = 0; i < BUFFER_SIZE; i++) {
        i2s_buffer[i] = intSample;
    }
    
    // I2S로 출력
    size_t bytes_written = 0;
    I2S.write(i2s_buffer, BUFFER_SIZE, &bytes_written);
    
    if (bytes_written != BUFFER_SIZE) {
        Serial.println("I2S write warning: " + String(bytes_written) + " / " + String(BUFFER_SIZE));
    }
}

void printInstructions() {
    Serial.println("\n=== TR-808 드럼 머신 제어 명령 ===");
    Serial.println("kick, snare, cymbal, hihat,openhihat, tom, conga");
    Serial.println("rimshot, maracas, clap, cowbell");
    Serial.println("1,2,3,4,5,6,7,8,9,0,c (숫자키도 가능)");
    Serial.println("kick <velocity> - 킥을 특정 벨로시티로 트리거");
    Serial.println("master <volume> - 마스터 볼륨 설정 (0.0-1.0)");
    Serial.println("status - 현재 상태 표시");
    Serial.println("help - 이 도움말 표시");
    Serial.println("==================================\n");
}

void printStatus() {
    Serial.println("\n=== TR-808 상태 ===");
    Serial.println("마스터 볼륨: 0.8 (기본값)");
    Serial.println("샘플 레이트: " + String(SAMPLE_RATE) + " Hz");
    Serial.println("버퍼 크기: " + String(BUFFER_SIZE) + " 샘플");
    Serial.println("I2S 포트: " + String(I2S_PORT));
    Serial.println("==================\n");
}

/*
 * MIDI 입력 인터페이스 (선택적)
 * MIDI 장치를 연결하여 실시간 드럼 연주 가능
 */
#ifdef USE_MIDI_INPUT
#include <MIDI.h>
MIDI_CREATE_DEFAULT_INSTANCE();

void handleMIDI(byte channel, byte pitch, byte velocity) {
    // GM 드럼 매핑 (MIDI 채널 10)
    switch(pitch) {
        case 36: // Kick Drum
            drumMachine.triggerKick(velocity / 127.0f);
            break;
        case 38: // Acoustic Snare  
            drumMachine.triggerSnare(velocity / 127.0f);
            break;
        case 42: // Closed Hi-Hat
            drumMachine.triggerHiHat(velocity / 127.0f, false);
            break;
        case 46: // Open Hi-Hat
            drumMachine.triggerHiHat(velocity / 127.0f, true);
            break;
        case 45: // Low Tom
            drumMachine.setTomTuning(120.0f);
            drumMachine.triggerTom(velocity / 127.0f);
            break;
        case 47: // Low-Mid Tom
            drumMachine.setTomTuning(140.0f);
            drumMachine.triggerTom(velocity / 127.0f);
            break;
        case 43: // High Tom
            drumMachine.setTomTuning(165.0f);
            drumMachine.triggerTom(velocity / 127.0f);
            break;
        case 49: // Crash Cymbal
            drumMachine.triggerCymbal(velocity / 127.0f);
            break;
        case 63: // Conga High
            drumMachine.setCongaTuning(370.0f);
            drumMachine.triggerConga(velocity / 127.0f);
            break;
        case 62: // Conga Low
            drumMachine.setCongaTuning(250.0f);
            drumMachine.triggerConga(velocity / 127.0f);
            break;
        case 37: // Acoustic Snare Rim Shot
            drumMachine.triggerRimshot(velocity / 127.0f);
            break;
        case 82: // Maracas
            drumMachine.triggerMaracas(velocity / 127.0f);
            break;
        case 39: // Hand Clap
            drumMachine.triggerClap(velocity / 127.0f);
            break;
    }
}
#endif