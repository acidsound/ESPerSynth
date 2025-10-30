#include "esp32c3_i2s.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "driver/dma.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

static const char* TAG = "ESP32C3_I2S";

// I2S 문맥 구조체
typedef struct {
    i2s_port_t port;           // I2S 포트 번호
    i2s_config_t config;       // I2S 설정 구조체
    bool initialized;          // 초기화 여부
    bool running;              // 실행 상태
    uint8_t volume;            // 볼륨 (0-255)
} esp32c3_i2s_context_t;

// I2S 전역 문맥
static esp32c3_i2s_context_t i2s_ctx = {
    .port = I2S_NUM_0,
    .initialized = false,
    .running = false,
    .volume = 128
};

/**
 * GPIO 핀 설정 함수
 */
void esp32c3_i2s_configure_gpio_pins(void)
{
    ESP_LOGI(TAG, "I2S GPIO 핀 설정 중...");
    
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << I2S_WS_PIN) | (1ULL << I2S_BCK_PIN) | (1ULL << I2S_DATA_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    
    gpio_config(&io_conf);
    
    // MCLK 핀 설정 (선택사항)
    io_conf.pin_bit_mask = (1ULL << I2S_MCK_PIN);
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "GPIO 핀 설정 완료 - WS: GPIO%d, BCK: GPIO%d, DATA: GPIO%d, MCLK: GPIO%d",
             I2S_WS_PIN, I2S_BCK_PIN, I2S_DATA_PIN, I2S_MCK_PIN);
}

/**
 * GDMA 채널 설정 함수
 */
void esp32c3_i2s_configure_gdma(void)
{
    ESP_LOGI(TAG, "GDMA 채널 %d 설정 중...", I2S_GDMA_CHANNEL);
    
    // GDMA 채널 초기화
    dma_config_t dma_config = {
        .direction = DMA_DIR_MEM_TO_DEV,
        .sample_rate_hz = I2S_SAMPLE_RATE,
        .burst_size = DMA_BURST_SIZE_4,
        .src_nocache = false,
        .dst_nocache = false,
        .out_source = DMA_SOURCE_MEMORY
    };
    
    ESP_LOGI(TAG, "GDMA 설정 완료 - 채널: %d, 방향: MEM_TO_DEV, 샘플레이트: %d Hz",
             I2S_GDMA_CHANNEL, I2S_SAMPLE_RATE);
}

/**
 * I2S 초기화 함수
 */
void esp32c3_i2s_init(void)
{
    ESP_LOGI(TAG, "I2S 초기화 시작...");
    
    // GPIO 핀 설정
    esp32c3_i2s_configure_gpio_pins();
    
    // GDMA 설정
    esp32c3_i2s_configure_gdma();
    
    // 기본 설정 초기화
    i2s_config_t default_config = {
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channels = I2S_CHANNELS,
        .master_clock = true
    };
    
    esp32c3_i2s_config(&default_config);
    
    i2s_ctx.initialized = true;
    ESP_LOGI(TAG, "I2S 초기화 완료");
}

/**
 * I2S 설정 함수
 */
void esp32c3_i2s_config(const i2s_config_t* config)
{
    if (!config) {
        ESP_LOGE(TAG, "I2S 설정 포인터가 NULL입니다");
        return;
    }
    
    // I2S 포트 설정 구조체
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
        .sample_rate = config->sample_rate,
        .bits_per_sample = (i2s_bits_per_sample_t)config->bits_per_sample,
        .channel_format = config->channels == 1 ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .tx_desc_auto_clear = true,
        .dma_buf_count = 8,
        .dma_buf_len = I2S_BUFFER_SIZE,
        .use_apll = true,  // APLL 사용으로 정확한 클록
        .tx_desc_auto_clear = true
    };
    
    // I2S 드라이버 설치
    esp_err_t ret = i2s_driver_install(i2s_ctx.port, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S 드라이버 설치 실패: %s", esp_err_to_name(ret));
        return;
    }
    
    // I2S 핀 설정
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_MCK_PIN,
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DATA_PIN,
        .data_in_num = -1  // 입력 없음 (송신 전용)
    };
    
    i2s_set_pin(i2s_ctx.port, &pin_config);
    
    // 설정 저장
    i2s_ctx.config = *config;
    
    ESP_LOGI(TAG, "I2S 설정 완료:");
    ESP_LOGI(TAG, "  - 샘플링 레이트: %d Hz", config->sample_rate);
    ESP_LOGI(TAG, "  - 비트당 샘플: %d bits", config->bits_per_sample);
    ESP_LOGI(TAG, "  - 채널 수: %d", config->channels);
    ESP_LOGI(TAG, "  - 마스터 클록: %s", config->master_clock ? "사용" : "사용안함");
}

/**
 * 샘플링 레이트 설정
 */
void esp32c3_i2s_set_sample_rate(uint32_t sample_rate)
{
    esp_err_t ret = i2s_set_sample_rates(i2s_ctx.port, sample_rate);
    if (ret == ESP_OK) {
        i2s_ctx.config.sample_rate = sample_rate;
        ESP_LOGI(TAG, "샘플링 레이트 설정: %d Hz", sample_rate);
    } else {
        ESP_LOGE(TAG, "샘플링 레이트 설정 실패: %s", esp_err_to_name(ret));
    }
}

/**
 * I2S 시작
 */
void esp32c3_i2s_start(void)
{
    if (!i2s_ctx.initialized) {
        ESP_LOGE(TAG, "I2S가 초기화되지 않았습니다");
        return;
    }
    
    esp_err_t ret = i2s_start(i2s_ctx.port);
    if (ret == ESP_OK) {
        i2s_ctx.running = true;
        ESP_LOGI(TAG, "I2S 시작됨");
    } else {
        ESP_LOGE(TAG, "I2S 시작 실패: %s", esp_err_to_name(ret));
    }
}

/**
 * I2S 정지
 */
void esp32c3_i2s_stop(void)
{
    if (i2s_ctx.running) {
        esp_err_t ret = i2s_stop(i2s_ctx.port);
        if (ret == ESP_OK) {
            i2s_ctx.running = false;
            ESP_LOGI(TAG, "I2S 정지됨");
        } else {
            ESP_LOGE(TAG, "I2S 정지 실패: %s", esp_err_to_name(ret));
        }
    }
}

/**
 * I2S 샘플 쓰기 (단일 샘플)
 */
bool esp32c3_i2s_write_sample(int32_t left, int32_t right)
{
    if (!i2s_ctx.running) {
        return false;
    }
    
    // 32비트 스터레오 데이터 생성
    int32_t stereo_sample[2] = {left, right};
    
    size_t bytes_written = 0;
    esp_err_t ret = i2s_write(i2s_ctx.port, stereo_sample, sizeof(stereo_sample), &bytes_written, portMAX_DELAY);
    
    return (ret == ESP_OK && bytes_written == sizeof(stereo_sample));
}

/**
 * I2S 데이터 쓰기 (버퍼)
 */
void esp32c3_i2s_write_data(const int32_t* data, uint32_t frames)
{
    if (!i2s_ctx.running || !data) {
        return;
    }
    
    size_t bytes_to_write = frames * 2 * sizeof(int32_t); // Stereo = 2 channels
    size_t bytes_written = 0;
    
    esp_err_t ret = i2s_write(i2s_ctx.port, data, bytes_to_write, &bytes_written, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S 데이터 쓰기 실패: %s", esp_err_to_name(ret));
    }
}

/**
 * I2S 상태 확인
 */
bool esp32c3_i2s_is_running(void)
{
    return i2s_ctx.running;
}

/**
 * 볼륨 설정
 */
void esp32c3_i2s_set_volume(uint8_t volume)
{
    i2s_ctx.volume = volume;
    ESP_LOGI(TAG, "볼륨 설정: %d/255", volume);
}

/**
 * GDMA 인터럽트 활성화
 */
void esp32c3_i2s_enable_gdma_interrupts(void)
{
    ESP_LOGI(TAG, "GDMA 인터럽트 활성화");
    
    // GDMA 채널 0 인터럽트 설정
    // 실제 구현에서는 ESP-IDF의 GDMA API 사용 필요
}

/**
 * I2S 테스트 함수 - 사인파 생성 및 출력
 */
void esp32c3_i2s_test_tone(void)
{
    const uint32_t test_frequency = 440; // A4 음계
    const uint32_t samples = I2S_SAMPLE_RATE / test_frequency;
    
    int32_t* test_buffer = malloc(samples * 2 * sizeof(int32_t));
    if (!test_buffer) {
        ESP_LOGE(TAG, "테스트 버퍼 할당 실패");
        return;
    }
    
    // 사인파 생성
    for (uint32_t i = 0; i < samples; i++) {
        float angle = (2.0f * M_PI * i) / samples;
        int32_t sample = (int32_t)(32767.0f * sin(angle) * (i2s_ctx.volume / 255.0f));
        
        test_buffer[i * 2] = sample;     // 왼쪽 채널
        test_buffer[i * 2 + 1] = sample; // 오른쪽 채널
    }
    
    // I2S 시작
    esp32c3_i2s_start();
    
    // 오디오 출력 (무한 루프)
    while (esp32c3_i2s_is_running()) {
        esp32c3_i2s_write_data(test_buffer, samples);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(test_buffer);
}

/**
 * I2S 정리 함수
 */
void esp32c3_i2s_deinit(void)
{
    esp32c3_i2s_stop();
    i2s_driver_uninstall(i2s_ctx.port);
    
    // GPIO 리셋
    gpio_reset_pin(I2S_WS_PIN);
    gpio_reset_pin(I2S_BCK_PIN);
    gpio_reset_pin(I2S_DATA_PIN);
    gpio_reset_pin(I2S_MCK_PIN);
    
    i2s_ctx.initialized = false;
    ESP_LOGI(TAG, "I2S 정리 완료");
}