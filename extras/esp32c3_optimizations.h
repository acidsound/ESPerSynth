#ifndef ESP32C3_OPTIMIZATIONS_H
#define ESP32C3_OPTIMizations_H

#include <Arduino.h>
#include <driver/gpio.h>
#include <driver/dma_types.h>
#include <driver/gptimer.h>
#include <driver/i2s.h>
#include <esp_private/esp_clk.h>
#include <esp_private/cache_ops.h>
#include <soc/periph_defs.h>
#include <math.h>

//==============================================================================
// 성능 최적화 상수 정의
//==============================================================================
#define ESP32C3_CPU_FREQ_MAX      160000000
#define ESP32C3_CPU_FREQ_80MHZ    80000000
#define ESP32C3_CPU_FREQ_160MHZ   160000000
#define GDMA_MAX_BLOCK_SIZE       4096
#define DMA_ALIGNMENT             4
#define AUDIO_BUFFER_SIZE         1024
#define AUDIO_SAMPLE_RATE         44100
#define AUDIO_BIT_DEPTH           16
#define AUDIO_CHANNELS            2
#define AUDIO_BLOCK_SIZE          (AUDIO_BUFFER_SIZE * AUDIO_CHANNELS * (AUDIO_BIT_DEPTH / 8))

//==============================================================================
// GDMA 채널 설정
//==============================================================================
typedef struct {
    gdma_channel_handle_t rx_channel;
    gdma_channel_handle_t tx_channel;
    gdma_strategy_config_t strategy_config;
    bool is_active;
} gdma_audio_config_t;

//==============================================================================
// 타이머 인터럽트 설정
//==============================================================================
typedef struct {
    TIMER_SRC timer_id;
    uint64_t timer_period_us;
    bool enabled;
    void (*callback)(void);
} timer_interrupt_config_t;

//==============================================================================
// 오디오 버퍼 관리
//==============================================================================
typedef struct {
    void* audio_buffer;
    size_t buffer_size;
    volatile uint32_t write_pos;
    volatile uint32_t read_pos;
    SemaphoreHandle_t mutex;
    bool is_filled;
} audio_buffer_t;

//==============================================================================
// 고정 소수점 연산 (16.16 형식)
//==============================================================================
#define FP16_16_SHIFT 16
#define FP16_16_SCALE 65536.0f

typedef int32_t fp16_16_t;

static inline fp16_16_t fp16_16_from_float(float x) {
    return (fp16_16_t)(x * FP16_16_SCALE);
}

static inline float fp16_16_to_float(fp16_16_t x) {
    return (float)x / FP16_16_SCALE;
}

static inline fp16_16_t fp16_16_add(fp16_16_t a, fp16_16_t b) {
    return a + b;
}

static inline fp16_16_t fp16_16_subtract(fp16_16_t a, fp16_16_t b) {
    return a - b;
}

static inline fp16_16_t fp16_16_multiply(fp16_16_t a, fp16_16_t b) {
    return (fp16_16_t)((int64_t)a * b >> FP16_16_SHIFT);
}

static inline fp16_16_t fp16_16_divide(fp16_16_t a, fp16_16_t b) {
    return (fp16_16_t)((int64_t)a << FP16_16_SHIFT / b);
}

//==============================================================================
// 성능 최적화 함수들
//==============================================================================

/**
 * @brief CPU 클록 주파수 설정
 * @param cpu_freqdesired_frequency 설정할 주파수 (80MHz 또는 160MHz)
 * @return bool 설정 성공 여부
 */
static inline bool set_cpu_frequency(uint32_t desired_frequency) {
    if (desired_frequency == ESP32C3_CPU_FREQ_80MHZ || desired_frequency == ESP32C3_CPU_FREQ_160MHZ) {
        if (desired_frequency == ESP32C3_CPU_FREQ_80MHZ) {
            ets_update_cpu_frequency(80);
            return true;
        } else {
            ets_update_cpu_frequency(160);
            return true;
        }
    }
    return false;
}

/**
 * @brief GDMA 채널 초기화
 * @param config GDMA 설정 구조체 포인터
 * @param tx_channel_id 전송 채널 ID
 * @param rx_channel_id 수신 채널 ID
 * @return bool 초기화 성공 여부
 */
static inline bool initialize_gdma(gdma_audio_config_t* config, int tx_channel_id, int rx_channel_id) {
    gdma_general_config_t general_config = {
        .access_commit_mode = GDMA_ACCESS_MODE_AUTO
    };
    
    gdma_new_algorithm_group(&general_config, &config->strategy_config);
    
    // 수신 채널 설정
    gdma_rx_config_t rx_config = {
        .flags = {
            .out_ext_sel = false,
            .out_empty_thrsh_en = false,
            .out_full_thrsh_en = false,
        }
    };
    
    gdma_new_channel(rx_channel_id, &rx_config, &config->rx_channel);
    
    // 전송 채널 설정
    gdma_tx_config_t tx_config = {
        .flags = {
            .in_ext_sel = false,
            .in_ext_bits = 0,
        }
    };
    
    gdma_new_channel(tx_channel_id, &tx_config, &config->tx_channel);
    config->is_active = true;
    
    return true;
}

/**
 * @brief GDMA 채널 활성화/비활성화
 * @param config GDMA 설정 구조체
 * @param enable 활성화 여부
 */
static inline void gdma_enable_channel(gdma_audio_config_t* config, bool enable) {
    if (config->is_active) {
        if (enable) {
            gdma_start(config->rx_channel, (intptr_t)NULL);
            gdma_start(config->tx_channel, (intptr_t)NULL);
        } else {
            gdma_stop(config->rx_channel);
            gdma_stop(config->tx_channel);
        }
    }
}

/**
 * @brief 타이머 인터럽트 초기화
 * @param timer_id 타이머 ID
 * @param period_us 인터럽트 주기 (마이크로초)
 * @param callback 인터럽트 콜백 함수
 * @return bool 초기화 성공 여부
 */
static inline bool initialize_timer_interrupt(timer_interrupt_config_t* config, 
                                             TIMER_SRC timer_id, 
                                             uint64_t period_us, 
                                             void (*callback)(void)) {
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz
        .flags.intr_shared = false
    };
    
    gptimer_handle_t timer_handle;
    if (gptimer_new_timer(&timer_config, &timer_handle) != ESP_OK) {
        return false;
    }
    
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = callback,
    };
    
    gptimer_register_event_callbacks(timer_handle, &callbacks, NULL);
    
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = period_us,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    
    gptimer_set_alarm_action(timer_handle, &alarm_config);
    gptimer_enable(timer_handle);
    gptimer_start(timer_handle);
    
    config->timer_id = timer_id;
    config->timer_period_us = period_us;
    config->callback = callback;
    config->enabled = true;
    
    return true;
}

/**
 * @brief 오디오 버퍼 초기화
 * @param buffer 오디오 버퍼 구조체 포인터
 * @param size 버퍼 크기
 * @return bool 초기화 성공 여부
 */
static inline bool initialize_audio_buffer(audio_buffer_t* buffer, size_t size) {
    buffer->buffer_size = size;
    buffer->write_pos = 0;
    buffer->read_pos = 0;
    buffer->is_filled = false;
    
    // DMA 정렬된 버퍼 할당
    buffer->audio_buffer = heap_caps_malloc_aligned(DMA_ALIGNMENT, size, MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    
    if (buffer->audio_buffer == NULL) {
        return false;
    }
    
    buffer->mutex = xSemaphoreCreateMutex();
    
    if (buffer->mutex == NULL) {
        free(buffer->audio_buffer);
        return false;
    }
    
    return true;
}

/**
 * @brief 오디오 데이터 쓰기 (포인터 연산 최적화)
 * @param buffer 오디오 버퍼 구조체
 * @param data 데이터 포인터
 * @param length 데이터 길이
 * @return size_t 작성된 바이트 수
 */
static inline size_t write_audio_data(audio_buffer_t* buffer, const void* data, size_t length) {
    if (xSemaphoreTake(buffer->mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }
    
    uint8_t* buffer_ptr = (uint8_t*)buffer->audio_buffer;
    const uint8_t* data_ptr = (const uint8_t*)data;
    size_t bytes_written = 0;
    
    // 포인터 연산으로 순환 버퍼 처리
    while (bytes_written < length && !buffer->is_filled) {
        size_t available_space = buffer->buffer_size - buffer->write_pos;
        size_t space_to_write = min(available_space, length - bytes_written);
        
        memcpy(buffer_ptr + buffer->write_pos, data_ptr + bytes_written, space_to_write);
        
        buffer->write_pos += space_to_write;
        bytes_written += space_to_write;
        
        if (buffer->write_pos >= buffer->buffer_size) {
            buffer->write_pos = 0;
            buffer->is_filled = true;
        }
    }
    
    xSemaphoreGive(buffer->mutex);
    return bytes_written;
}

/**
 * @brief 오디오 데이터 읽기 (포인터 연산 최적화)
 * @param buffer 오디오 버퍼 구조체
 * @param data 데이터를 저장할 포인터
 * @param length 읽을 데이터 길이
 * @return size_t 읽은 바이트 수
 */
static inline size_t read_audio_data(audio_buffer_t* buffer, void* data, size_t length) {
    if (xSemaphoreTake(buffer->mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }
    
    uint8_t* buffer_ptr = (uint8_t*)buffer->audio_buffer;
    uint8_t* data_ptr = (uint8_t*)data;
    size_t bytes_read = 0;
    
    // 포인터 연산으로 순환 버퍼 처리
    while (bytes_read < length && (buffer->is_filled || buffer->write_pos > buffer->read_pos)) {
        size_t available_data = buffer->is_filled ? 
            buffer->buffer_size - buffer->read_pos : 
            buffer->write_pos - buffer->read_pos;
        
        size_t data_to_read = min(available_data, length - bytes_read);
        
        memcpy(data_ptr + bytes_read, buffer_ptr + buffer->read_pos, data_to_read);
        
        buffer->read_pos += data_to_read;
        bytes_read += data_to_read;
        
        if (buffer->read_pos >= buffer->buffer_size) {
            buffer->read_pos = 0;
            buffer->is_filled = false;
        }
    }
    
    xSemaphoreGive(buffer->mutex);
    return bytes_read;
}

/**
 * @brief I2S DMA 설정 최적화
 * @param sample_rate 샘플레이트
 * @param bits_per_sample 비트당 샘플 수
 * @param channel_fmt 채널 형식
 * @return bool 설정 성공 여부
 */
static inline bool configure_i2s_optimized(uint32_t sample_rate, uint8_t bits_per_sample, i2s_channel_fmt_t channel_fmt) {
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX,
        .sample_rate = sample_rate,
        .bits_per_sample = bits_per_sample,
        .channel_format = channel_fmt,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 8,
        .dma_buf_len = AUDIO_BUFFER_SIZE,
        .use_apll = true,
        .tx_desc_auto_clear = true,
        .rx_desc_auto_clear = true,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .bits_per_chan = bits_per_sample,
    };
    
    i2s_pin_config_t pin_config = {
        .mck_io_num = -1,
        .bck_io_num = 7,
        .ws_io_num = 8,
        .data_out_num = 9,
        .data_in_num = 10,
        .external_clock_flags = 0,
    };
    
    if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
        return false;
    }
    
    if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
        return false;
    }
    
    return true;
}

/**
 * @brief 빠른 오디오 스케일링 (고정 소수점 사용)
 * @param input 입력 샘플 포인터
 * @param output 출력 샘플 포인터
 * @param length 샘플 길이
 * @param scale_factor 스케일 팩터 (fp16_16_t)
 */
static inline void fast_audio_scale(const int16_t* input, int16_t* output, 
                                  size_t length, fp16_16_t scale_factor) {
    for (size_t i = 0; i < length; i++) {
        // 고정 소수점 연산으로 빠른 스케일링
        int32_t scaled_value = ((int32_t)input[i] * scale_factor) >> FP16_16_SHIFT;
        
        // 오버플로우 방지
        if (scaled_value > INT16_MAX) {
            output[i] = INT16_MAX;
        } else if (scaled_value < INT16_MIN) {
            output[i] = INT16_MIN;
        } else {
            output[i] = (int16_t)scaled_value;
        }
    }
}

/**
 * @brief 빠른 오디오 믹싱 (포인터 연산 최적화)
 * @param output 출력 버퍼 포인터
 * @param inputs 입력 버퍼 배열
 * @param input_count 입력 개수
 * @param length 샘플 길이
 * @param weights 가중치 배열 (fp16_16_t)
 */
static inline void fast_audio_mix(int16_t* output, int16_t** inputs, 
                                uint8_t input_count, size_t length, fp16_16_t* weights) {
    memset(output, 0, length * sizeof(int16_t));
    
    for (uint8_t i = 0; i < input_count; i++) {
        int16_t* input = inputs[i];
        fp16_16_t weight = weights[i];
        
        // 포인터 연산으로 효율적 믹싱
        for (size_t j = 0; j < length; j++) {
            int32_t weighted_sample = ((int32_t)input[j] * weight) >> FP16_16_SHIFT;
            output[j] += (int16_t)weighted_sample;
        }
    }
    
    // 클리핑
    for (size_t i = 0; i < length; i++) {
        if (output[i] > INT16_MAX) {
            output[i] = INT16_MAX;
        } else if (output[i] < INT16_MIN) {
            output[i] = INT16_MIN;
        }
    }
}

/**
 * @brief 캐시 라인 정렬 매크로
 */
#define CACHE_LINE_SIZE 32
#define CACHE_LINE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))

/**
 * @brief 성능 모니터링 시작
 */
static inline void perf_monitor_start() {
    esp_cache_way_mask_set_all(0);
}

/**
 * @brief 성능 모니터링 끝
 * @return uint32_t CPU 사이클 수
 */
static inline uint32_t perf_monitor_end() {
    return esp_cycle_counter();
}

/**
 * @brief 인터럽트 우선순위 최적화
 * @param priority 우선순위 (1-7)
 * @param peripheral peripheral 기기 ID
 */
static inline void optimize_interrupt_priority(int priority, int peripheral) {
    if (priority >= 1 && priority <= 7) {
        // 설정 가능한 최대 우선순위
        intr_matrix_set(PRO_CPU_NUM, peripheral, priority);
    }
}

/**
 * @brief RTOS 태스크 최적화
 * @param taskPriority 태스크 우선순위
 * @param taskCore 태스크가 실행될 코어 (-1은 자동할당)
 */
static inline void optimize_task_priority(int taskPriority, int taskCore) {
    // 높은 우선순위를 가진 태스크는 코어 고정
    if (taskPriority > 10 && taskCore >= 0) {
        vTaskCoreAffinitySet(NULL, (1 << taskCore));
    }
}

//==============================================================================
// 고성능 오디오 처리 루프
//==============================================================================
void optimize_audio_processing_loop();

/**
 * @brief 비동기 오디오 처리기 초기화
 */
bool initialize_audio_processor();

/**
 * @brief 오디오 처리 루프 (고성능)
 */
void audio_processing_task(void* parameters);

#endif /* ESP32C3_OPTIMIZATIONS_H */