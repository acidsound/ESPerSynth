#ifndef ESP32C3_I2S_H
#define ESP32C3_I2S_H

#include <stdint.h>
#include <stdbool.h>

// I2S 핀 매핑 (ESP32C3)
#define I2S_WS_PIN     2   // Word Select (LRCLK) 핀
#define I2S_BCK_PIN    3   // Bit Clock (SCLK) 핀  
#define I2S_DATA_PIN   4   // Data Out 핀
#define I2S_MCK_PIN    1   // Master Clock 핀 (선택사항)

// I2S 기본 설정
#define I2S_SAMPLE_RATE    32768   // 32.768kHz 샘플링 레이트
#define I2S_BITS_PER_SAMPLE  32    // 32비트 샘플
#define I2S_CHANNELS        2     // Stereo (L/R)
#define I2S_BUFFER_SIZE     1024  // I2S 버퍼 크기

// GDMA 채널 설정
#define I2S_GDMA_CHANNEL    0     // GDMA 채널 0 사용

// I2S 클록 분주 설정
#define I2S_MCK_DIV     8         // MCLK 분주비
#define I2S_BCK_DIV     4         // BCLK 분주비  
#define I2S_WS_DIV      64        // WS(LRCLK) 분주비

// I2S 포맷 설정
typedef struct {
    uint32_t sample_rate;      // 샘플링 레이트
    uint32_t bits_per_sample;  // 샘플당 비트 수
    uint8_t channels;          // 채널 수 (1=모노, 2=스테레오)
    bool master_clock;         // MCLK 사용 여부
} i2s_config_t;

// I2S 전송 데이터 구조체
typedef struct {
    int32_t left_channel;      // 왼쪽 채널 데이터
    int32_t right_channel;     // 오른쪽 채널 데이터
} i2s_audio_frame_t;

// I2S 초기화 및 설정 함수
void esp32c3_i2s_init(void);
void esp32c3_i2s_config(const i2s_config_t* config);
void esp32c3_i2s_set_sample_rate(uint32_t sample_rate);
void esp32c3_i2s_write_data(const int32_t* data, uint32_t frames);
void esp32c3_i2s_start(void);
void esp32c3_i2s_stop(void);
bool esp32c3_i2s_write_sample(int32_t left, int32_t right);

// GPIO 설정 함수
void esp32c3_i2s_configure_gpio_pins(void);

// I2S 상태 확인 함수
bool esp32c3_i2s_is_running(void);
void esp32c3_i2s_set_volume(uint8_t volume); // 볼륨 설정 (0-255)

// GDMA 관련 함수
void esp32c3_i2s_configure_gdma(void);
void esp32c3_i2s_enable_gdma_interrupts(void);

#endif /* ESP32C3_I2S_H */