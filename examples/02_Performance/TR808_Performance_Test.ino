/*
 * ESP32C3 TR-808 드럼 머신 성능 최적화 예제
 * 
 * 이 예제는 TR-808의 성능을 최적화하고 모니터링하는 방법을 보여줍니다.
 * 
 * 주요 기능:
 * - 실시간 성능 모니터링
 * - 메모리 사용량 최적화
 * - CPU 부하 테스트
 * - 버퍼 설정 최적화
 */

#include <I2S.h>
#include "arduino_tr808_config.h"
#include "tr808_drums.h"

// 성능 테스트 설정
#define PERFORMANCE_TEST_DURATION 10000  // 10초 테스트
#define CPU_STRESS_TEST_ENABLED  true
#define MEMORY_MONITORING_ENABLED true

// 전역 변수
TR808DrumMachine drumMachine;
int16_t i2sBuffer[256];

// 성능 메트릭
struct PerformanceMetrics {
    unsigned long totalSamples;
    unsigned long droppedSamples;
    unsigned long peakCpuUsage;
    unsigned long avgCpuUsage;
    unsigned long memoryUsage;
    unsigned long testStartTime;
    bool stressTestRunning;
} perfMetrics;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("===========================================");
    Serial.println("  ESP32C3 TR-808 성능 테스트");
    Serial.println("===========================================");
    
    // 성능 메트릭 초기화
    initializePerformanceMetrics();
    
    // I2S 초기화 (성능 최적화)
    initializeI2SForPerformance();
    
    // TR-808 초기화
    drumMachine.setMasterVolume(0.8f);
    
    Serial.println("✅ 시스템 초기화 완료");
    Serial.println("");
    
    // 성능 테스트 시작
    startPerformanceTests();
}

void loop() {
    // 성능 모니터링 루프
    updatePerformanceMetrics();
    
    // CPU 스트레스 테스트 (선택사항)
    if (CPU_STRESS_TEST_ENABLED && perfMetrics.stressTestRunning) {
        runCPUStressTest();
    }
    
    // 오디오 처리 (우선순위 낮음)
    processAudioOptimized();
    
    // 성능 보고서 출력 (5초마다)
    static unsigned long lastReport = 0;
    if (millis() - lastReport > 5000) {
        printPerformanceReport();
        lastReport = millis();
    }
    
    // 테스트 완료 여부 확인
    if (millis() - perfMetrics.testStartTime > PERFORMANCE_TEST_DURATION) {
        endPerformanceTests();
    }
    
    delayMicroseconds(10); // 안정성을 위한 짧은 지연
}

void initializePerformanceMetrics() {
    perfMetrics.totalSamples = 0;
    perfMetrics.droppedSamples = 0;
    perfMetrics.peakCpuUsage = 0;
    perfMetrics.avgCpuUsage = 0;
    perfMetrics.memoryUsage = ESP.getFreeHeap();
    perfMetrics.testStartTime = millis();
    perfMetrics.stressTestRunning = false;
    
    Serial.println("📊 성능 모니터링 초기화 완료");
}

void initializeI2SForPerformance() {
    Serial.println("🔧 성능 최적화된 I2S 설정...");
    
    // 고성능 I2S 설정
    i2s_mode_t mode = I2S_MODE_MASTER | I2S_MODE_TX;
    if (!MONO_OUTPUT) {
        mode |= I2S_MODE_DUAL;
    }
    
    if (!I2S.begin(mode, DEFAULT_SAMPLE_RATE, 16, 1)) {
        Serial.println("❌ I2S 초기화 실패");
        while(1) delay(1000);
    }
    
    Serial.println("  ✅ I2S 초기화 완료");
    Serial.println("     샘플레이트: " + String(DEFAULT_SAMPLE_RATE) + " Hz");
    Serial.println("     버퍼 크기: " + String(I2S_BUFFER_SIZE) + " 샘플");
    Serial.println("     모드: " + String(MONO_OUTPUT ? "모노" : "스테레오"));
}

void startPerformanceTests() {
    Serial.println("🚀 성능 테스트 시작...");
    Serial.println("테스트 시간: " + String(PERFORMANCE_TEST_DURATION/1000) + "초");
    Serial.println("");
    
    // 자동 드럼 테스트 시퀀스
    Serial.println("1단계: 기본 드럼 테스트");
    testBasicDrumSounds();
    
    delay(2000);
    
    Serial.println("2단계: 폴포니 테스트");
    testPolyphony();
    
    delay(2000);
    
    Serial.println("3단계: CPU 스트레스 테스트");
    perfMetrics.stressTestRunning = true;
}

void testBasicDrumSounds() {
    Serial.println("  - 각 드럼을 순차적으로 테스트...");
    
    drumMachine.triggerKick(1.0f);
    delay(200);
    drumMachine.triggerSnare(1.0f);
    delay(200);
    drumMachine.triggerCymbal(1.0f);
    delay(200);
    drumMachine.triggerHiHat(1.0f, false);
    delay(200);
    drumMachine.triggerTom(1.0f);
    delay(200);
    drumMachine.triggerConga(1.0f);
    
    Serial.println("  ✅ 기본 드럼 테스트 완료");
}

void testPolyphony() {
    Serial.println("  - 다중 드럼 동시 발화 테스트...");
    
    // 최대 폴포니까지 동시 발화
    for (int i = 0; i < MAX_POLYPHONY; i++) {
        if (i % 3 == 0) drumMachine.triggerKick(0.5f);
        if (i % 3 == 1) drumMachine.triggerSnare(0.5f);
        if (i % 3 == 2) drumMachine.triggerCymbal(0.3f);
        
        delay(50); // 50ms 간격
    }
    
    Serial.println("  ✅ 폴포니 테스트 완료 (" + String(MAX_POLYPHONY) + "개 동시)");
}

void runCPUStressTest() {
    // CPU 부하를 높이는 테스트 패턴
    static unsigned long lastStressTime = 0;
    
    if (millis() - lastStressTime > 100) { // 10Hz 스트레스
        // 복잡한 드럼 패턴
        drumMachine.triggerKick(1.0f);
        drumMachine.triggerSnare(0.8f);
        drumMachine.triggerHiHat(0.6f, false);
        
        lastStressTime = millis();
    }
}

void processAudioOptimized() {
    // 최적화된 오디오 처리 루프
    static unsigned long audioStartTime = 0;
    
    audioStartTime = micros();
    
    // TR-808 오디오 샘플 생성
    float audioSample = drumMachine.process();
    int16_t intSample = (int16_t)(audioSample * 32767);
    
    // I2S 버퍼 채우기
    for (int i = 0; i < I2S_BUFFER_SIZE; i++) {
        i2sBuffer[i] = intSample;
    }
    
    // I2S 출력
    size_t bytesWritten = 0;
    I2S.write(i2sBuffer, I2S_BUFFER_SIZE, &bytesWritten);
    
    if (bytesWritten != I2S_BUFFER_SIZE) {
        perfMetrics.droppedSamples++;
    }
    
    // 샘플 카운터 업데이트
    perfMetrics.totalSamples++;
    
    // CPU 사용률 계산
    unsigned long audioEndTime = micros();
    unsigned long audioProcessingTime = audioEndTime - audioStartTime;
    unsigned long totalCycleTime = 1000000 / DEFAULT_SAMPLE_RATE; // 30.5μs
    
    if (audioProcessingTime > 0 && totalCycleTime > 0) {
        float cpuUsage = (float)audioProcessingTime / totalCycleTime * 100.0f;
        if (cpuUsage > perfMetrics.peakCpuUsage) {
            perfMetrics.peakCpuUsage = cpuUsage;
        }
        perfMetrics.avgCpuUsage = (perfMetrics.avgCpuUsage * 0.9f) + (cpuUsage * 0.1f);
    }
}

void updatePerformanceMetrics() {
    if (MEMORY_MONITORING_ENABLED) {
        // 메모리 사용량 업데이트
        perfMetrics.memoryUsage = ESP.getFreeHeap();
    }
}

void printPerformanceReport() {
    unsigned long elapsedTime = millis() - perfMetrics.testStartTime;
    float actualSampleRate = (float)perfMetrics.totalSamples / (elapsedTime / 1000.0f);
    
    Serial.println("=== 성능 보고서 ===");
    Serial.println("시간: " + String(elapsedTime / 1000.0f, 1) + "초");
    Serial.println("샘플: " + String(perfMetrics.totalSamples));
    Serial.println("드롭드 샘플: " + String(perfMetrics.droppedSamples));
    Serial.println("실제 샘플레이트: " + String(actualSampleRate, 0) + " Hz");
    Serial.println("CPU 사용률: " + String(perfMetrics.avgCpuUsage, 1) + "% (피크: " + String(perfMetrics.peakCpuUsage, 1) + "%)");
    Serial.println("메모리: " + String(perfMetrics.memoryUsage) + " bytes");
    
    if (perfMetrics.totalSamples > 0) {
        float dropoutRate = (float)perfMetrics.droppedSamples / perfMetrics.totalSamples * 100.0f;
        Serial.println("드롭률: " + String(dropoutRate, 2) + "%");
    }
    
    Serial.println("===================");
}

void endPerformanceTests() {
    Serial.println("");
    Serial.println("🎉 성능 테스트 완료!");
    perfMetrics.stressTestRunning = false;
    
    // 최종 성능 보고서
    printFinalPerformanceReport();
    
    // 시스템 안정성 평가
    evaluateSystemStability();
    
    // 권장 설정 제안
    suggestOptimalSettings();
    
    Serial.println("");
    Serial.println("💡 더 자세한 테스트를 위해 다른 예제를 시도해보세요.");
}

void printFinalPerformanceReport() {
    unsigned long elapsedTime = millis() - perfMetrics.testStartTime;
    float actualSampleRate = (float)perfMetrics.totalSamples / (elapsedTime / 1000.0f);
    float accuracy = actualSampleRate / DEFAULT_SAMPLE_RATE * 100.0f;
    
    Serial.println("📊 최종 성능 분석:");
    Serial.println("  샘플 정확도: " + String(accuracy, 1) + "%");
    Serial.println("  평균 CPU: " + String(perfMetrics.avgCpuUsage, 1) + "%");
    Serial.println("  피크 CPU: " + String(perfMetrics.peakCpuUsage, 1) + "%");
    Serial.println("  메모리 안정성: " + String(ESP.getFreeHeap()) + " bytes");
    
    if (perfMetrics.totalSamples > 0) {
        float dropoutRate = (float)perfMetrics.droppedSamples / perfMetrics.totalSamples * 100.0f;
        Serial.println("  오디오 품질: " + String(100.0f - dropoutRate, 1) + "%");
    }
}

void evaluateSystemStability() {
    Serial.println("");
    Serial.println("🔍 시스템 안정성 평가:");
    
    // 샘플 정확도 평가
    float sampleAccuracy = (float)perfMetrics.totalSamples / ((millis() - perfMetrics.testStartTime) / 1000.0f) / DEFAULT_SAMPLE_RATE * 100.0f;
    if (sampleAccuracy > 95.0f) {
        Serial.println("  ✅ 샘플 타이밍: 우수");
    } else if (sampleAccuracy > 80.0f) {
        Serial.println("  ⚠️ 샘플 타이밍: 보통");
    } else {
        Serial.println("  ❌ 샘플 타이밍: 불안정");
    }
    
    // CPU 사용률 평가
    if (perfMetrics.avgCpuUsage < 20.0f) {
        Serial.println("  ✅ CPU 사용률: 최적");
    } else if (perfMetrics.avgCpuUsage < 40.0f) {
        Serial.println("  ⚠️ CPU 사용률: 경고");
    } else {
        Serial.println("  ❌ CPU 사용률: 과부하");
    }
    
    // 메모리 평가
    if (ESP.getFreeHeap() > 200000) {
        Serial.println("  ✅ 메모리: 충분");
    } else if (ESP.getFreeHeap() > 100000) {
        Serial.println("  ⚠️ 메모리: 제한적");
    } else {
        Serial.println("  ❌ 메모리: 부족");
    }
}

void suggestOptimalSettings() {
    Serial.println("");
    Serial.println("💡 권장 설정:");
    
    // CPU 기반 권장사항
    if (perfMetrics.avgCpuUsage > 30.0f) {
        Serial.println("  - MAX_POLYPHONY 감소 (현재 " + String(MAX_POLYPHONY) + " → 5)");
        Serial.println("  - BUFFER_SIZE 증가 (성능 향상)");
    } else if (perfMetrics.avgCpuUsage < 15.0f) {
        Serial.println("  - 추가 효과 활성화 가능");
        Serial.println("  - 샘플레이트 증가 고려");
    }
    
    // 메모리 기반 권장사항
    if (ESP.getFreeHeap() < 150000) {
        Serial.println("  - 정적 할당 활성화");
        Serial.println("  - 로컬 변수 최소화");
    }
    
    // 드롭샘플 기반 권장사항
    if (perfMetrics.droppedSamples > 0) {
        Serial.println("  - I2S 버퍼 크기 증가");
        Serial.println("  - 최적화 레벨 조정");
    }
    
    Serial.println("  - 지속적 성능 모니터링 활성화");
    Serial.println("  - 정기적 리셋 고려");
}

void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        
        if (cmd == "stress") {
            Serial.println("🔥 CPU 스트레스 테스트 시작");
            perfMetrics.stressTestRunning = true;
        }
        else if (cmd == "stop") {
            Serial.println("⏹️ 테스트 중지");
            perfMetrics.stressTestRunning = false;
        }
        else if (cmd == "reset") {
            Serial.println("🔄 성능 메트릭 리셋");
            initializePerformanceMetrics();
        }
        else if (cmd == "report") {
            printPerformanceReport();
        }
        else if (cmd == "kick") {
            drumMachine.triggerKick(1.0f);
        }
        else if (cmd == "snare") {
            drumMachine.triggerSnare(1.0f);
        }
        // ... 다른 명령어
    }
}