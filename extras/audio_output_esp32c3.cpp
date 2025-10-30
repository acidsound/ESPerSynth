/*
 * ESP32C3 Mozzi Library 오디오 출력 구현
 * 
 * ESP32C3의 GPIO 18 (I2S 또는 PWM)을 통한 오디오 출력
 * PWM 기반의 외부 오디오 출력 모드 구현
 */

#include "mozzi_config.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

// =============================================================================
// 전역 변수 및 상수 정의
// =============================================================================

static const char* TAG = "ESP32C3_AudioOutput";

// PWM 채널 설정
#define PWM_TIMER_SPEED_HZ 8000      // 8kHz PWM 주파수
#define PWM_TIMER_RESOLUTION 8       // 8-bit 분해능 (0-255)

// GPIO 설정
#define AUDIO_OUTPUT_PIN GPIO_NUM_18
#define AUDIO_PWM_CHANNEL 0

// 오디오 버퍼 관리 (더블 버퍼링)
static int16_t audioBuffer0[MOZZI_OUTPUT_BUFFER_SIZE];
static int16_t audioBuffer1[MOZZI_OUTPUT_BUFFER_SIZE];
static volatile uint8_t currentBuffer = 0;
static volatile uint8_t writeBuffer = 0;
static volatile size_t bufferIndex = 0;

// =============================================================================
// 오디오 출력 초기화
// =============================================================================

void initializeAudioOutput() {
    DEBUG_PRINTLN("Initializing ESP32C3 audio output...");
    
    // GPIO 설정
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << AUDIO_OUTPUT_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    
    gpio_config(&io_conf);
    
    // PWM 채널 설정 (I2S 대신 PWM 출력 사용)
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = PWM_TIMER_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_TIMER_SPEED_HZ,
        .clk_cfg = LEDC_USE_APB_CLK,
    };
    
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = AUDIO_OUTPUT_PIN,
        .duty = 0,
        .hpoint = 0,
    };
    
    ledc_channel_config(&ledc_channel);
    
    // PWM 채널 시작
    ledc_fade_func_install(0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    DEBUG_PRINTLN("Audio output initialized successfully");
}

// =============================================================================
// 오디오 출력 함수 (Mozzi에서 호출)
// =============================================================================

void audioOutput(int16_t output) {
    // 오디오 샘플 검증 및 변환
    output = VALIDATE_AUDIO_SAMPLE(output);
    
    // signed 16-bit를 unsigned 8-bit PWM 값으로 변환
    uint8_t pwm_value = CONVERT_TO_PWM_VALUE(output);
    
    // PWM 출력
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm_value);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
#ifdef ENABLE_PERFORMANCE_MONITORING
    static uint32_t sample_count = 0;
    sample_count++;
#endif
}

// =============================================================================
// 오디오 버퍼 관리 (더블 버퍼링)
// =============================================================================

void initializeAudioBuffers() {
    DEBUG_PRINTLN("Initializing audio buffers...");
    
    // 버퍼 초기화
    memset(audioBuffer0, 0, sizeof(audioBuffer0));
    memset(audioBuffer1, 0, sizeof(audioBuffer1));
    
    currentBuffer = 0;
    writeBuffer = 1;
    bufferIndex = 0;
    
    DEBUG_PRINTLN("Audio buffers initialized");
}

bool bufferAudioSample(int16_t sample) {
    int16_t* current_write_buffer = (writeBuffer == 0) ? audioBuffer0 : audioBuffer1;
    
    if (bufferIndex >= MOZZI_OUTPUT_BUFFER_SIZE) {
        return false; // 버퍼 가득참
    }
    
    current_write_buffer[bufferIndex++] = sample;
    
    // 버퍼가 가득 차면 스위치
    if (bufferIndex >= MOZZI_OUTPUT_BUFFER_SIZE) {
        bufferIndex = 0;
        writeBuffer = 1 - writeBuffer;
        return true; // 버퍼 스위치 완료
    }
    
    return false;
}

bool getAudioSample(int16_t* sample) {
    int16_t* current_read_buffer = (currentBuffer == 0) ? audioBuffer0 : audioBuffer1;
    static size_t readIndex = 0;
    
    if (readIndex >= MOZZI_OUTPUT_BUFFER_SIZE) {
        readIndex = 0;
        currentBuffer = 1 - currentBuffer;
        return false;
    }
    
    *sample = current_read_buffer[readIndex++];
    return true;
}

bool isBufferFull() {
    return (bufferIndex >= MOZZI_OUTPUT_BUFFER_SIZE);
}

bool isBufferEmpty() {
    return (bufferIndex == 0);
}

void switchBuffer() {
    currentBuffer = writeBuffer;
    writeBuffer = 1 - writeBuffer;
    bufferIndex = 0;
}

// =============================================================================
// 오디오 핫플러그 감지 (외장 오디오 장치 연결 감지)
// =============================================================================

void initializeAudioDetect() {
    // 오디오 출력 핀을 입력으로 설정하여 연결 상태 감지
    gpio_config_t detect_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << AUDIO_OUTPUT_PIN),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    
    gpio_config(&detect_conf);
    
    DEBUG_PRINTLN("Audio detect initialized");
}

bool isAudioDeviceConnected() {
    // GPIO 입력값으로 오디오 장치 연결 확인
    int level = gpio_get_level(AUDIO_OUTPUT_PIN);
    return (level == 1); // 풀업저항이 있는 경우 연결됨
}

// =============================================================================
// I2S 출력 모드 (선택사항)
// =============================================================================

#ifdef SUPPORT_I2S_OUTPUT
#include "driver/i2s.h"

void initializeI2SOutput() {
    DEBUG_PRINTLN("Initializing I2S audio output...");
    
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = MOZZI_AUDIO_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = AUDIO_OUTPUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    
    DEBUG_PRINTLN("I2S audio output initialized");
}

void audioOutputI2S(int16_t* samples, size_t sample_count) {
    size_t bytes_written = 0;
    i2s_write(I2S_NUM_0, samples, sample_count * sizeof(int16_t), &bytes_written, 0);
}
#endif

// =============================================================================
// 오디오 품질 테스트
// =============================================================================

void testAudioOutput() {
    DEBUG_PRINTLN("Running audio output test...");
    
    // 440Hz 사인파 테스트
    for (int i = 0; i < MOZZI_AUDIO_RATE; i++) {
        int16_t sample = (int16_t)(32767 * sin(i * 2.0 * PI * 440.0 / MOZZI_AUDIO_RATE));
        audioOutput(sample);
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }
    
    // 880Hz 사인파 테스트
    for (int i = 0; i < MOZZI_AUDIO_RATE; i++) {
        int16_t sample = (int16_t)(32767 * sin(i * 2.0 * PI * 880.0 / MOZZI_AUDIO_RATE));
        audioOutput(sample);
        delayMicroseconds(SAMPLE_INTERVAL_US);
    }
    
    DEBUG_PRINTLN("Audio output test completed");
}

// =============================================================================
// 전압 레퍼런스 설정
// =============================================================================

void setAudioReferenceVoltage(float voltage) {
    // 외부 DAC 또는 오디오 앰프를 위한 전압 레퍼런스 설정
    // 필요시 GPIO를 통해 전압 제어
    if (voltage > 2.5) {
        // 고전압 모드 - GPIO로 전압 레퍼런스 제어
        gpio_set_level(AUDIO_OUTPUT_PIN, 1);
    } else {
        // 저전압 모드
        gpio_set_level(AUDIO_OUTPUT_PIN, 0);
    }
}

// =============================================================================
// 오디오 출력 상태 정보
// =============================================================================

void printAudioOutputStatus() {
    DEBUG_PRINTLN("=== ESP32C3 Audio Output Status ===");
    DEBUG_PRINT("Output Pin: GPIO ");
    DEBUG_PRINTLN(AUDIO_OUTPUT_PIN);
    DEBUG_PRINT("Sample Rate: ");
    DEBUG_PRINT(MOZZI_AUDIO_RATE);
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Current Buffer: ");
    DEBUG_PRINTLN(currentBuffer);
    DEBUG_PRINT("Buffer Index: ");
    DEBUG_PRINTLN(bufferIndex);
    DEBUG_PRINT("Buffer Full: ");
    DEBUG_PRINTLN(isBufferFull() ? "Yes" : "No");
#ifdef SUPPORT_I2S_OUTPUT
    DEBUG_PRINTLN("I2S Mode: Enabled");
#else
    DEBUG_PRINTLN("PWM Mode: Enabled");
#endif
}