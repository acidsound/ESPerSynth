#include "esp32c3_optimizations.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp32/rom/ets_sys.h>

//==============================================================================
// 전역 변수
//==============================================================================
static gdma_audio_config_t g_gdma_config = {0};
static audio_buffer_t g_audio_input_buffer = {0};
static audio_buffer_t g_audio_output_buffer = {0};
static volatile bool g_audio_processing_enabled = false;
static TaskHandle_t g_audio_processing_task_handle = NULL;

//==============================================================================
// 최적화된 오디오 처리 루프
//==============================================================================
void optimize_audio_processing_loop() {
    const uint32_t cpu_freq = ets_get_cpu_frequency();
    Serial.printf("CPU 주파수: %lu MHz\n", cpu_freq);
    
    // GDMA 성능 측정
    uint32_t start_cycles = esp_cycle_counter();
    
    gdma_channel_handle_t tx_channel = g_gdma_config.tx_channel;
    gdma_channel_handle_t rx_channel = g_gdma_config.rx_channel;
    
    // GDMA 성능 최적화를 위한 설정
    if (tx_channel && rx_channel) {
        // 우선순위 설정
        gdma_channel_set_priority(tx_channel, 5);
        gdma_channel_set_priority(rx_channel, 5);
        
        // 트리거 설정
        gdma_trigger_config_t trigger_config = {
            .trigger_type = GDMA_TRIG_PERIPH_GPTIMER,
            .trigger_group = 0,
            .trigger_src = 0,
            .trigger_mode = GDMA_TRIG_MODE_CONTINUOUS,
        };
        
        gdma_channel_set_trigger(tx_channel, &trigger_config);
        gdma_channel_set_trigger(rx_channel, &trigger_config);
    }
    
    uint32_t end_cycles = esp_cycle_counter();
    Serial.printf("GDMA 설정 완료 (사이클: %lu)\n", end_cycles - start_cycles);
}

//==============================================================================
// 비동기 오디오 처리기 초기화
//==============================================================================
bool initialize_audio_processor() {
    Serial.println("오디오 처리기 초기화 시작...");
    
    // CPU 주파수 160MHz로 설정
    if (!set_cpu_frequency(ESP32C3_CPU_FREQ_160MHZ)) {
        Serial.println("CPU 주파수 설정 실패");
        return false;
    }
    
    Serial.printf("CPU 주파수 설정: %lu MHz\n", ets_get_cpu_frequency());
    
    // I2S 최적화 설정
    if (!configure_i2s_optimized(AUDIO_SAMPLE_RATE, AUDIO_BIT_DEPTH, I2S_CHANNEL_FMT_RIGHT_LEFT)) {
        Serial.println("I2S 설정 실패");
        return false;
    }
    
    // GDMA 채널 초기화
    if (!initialize_gdma(&g_gdma_config, 1, 0)) {
        Serial.println("GDMA 초기화 실패");
        return false;
    }
    
    // 오디오 버퍼 초기화
    if (!initialize_audio_buffer(&g_audio_input_buffer, AUDIO_BLOCK_SIZE)) {
        Serial.println("입력 버퍼 초기화 실패");
        return false;
    }
    
    if (!initialize_audio_buffer(&g_audio_output_buffer, AUDIO_BLOCK_SIZE)) {
        Serial.println("출력 버퍼 초기화 실패");
        return false;
    }
    
    // 인터럽트 우선순위 최적화
    optimize_interrupt_priority(5, I2S_INTERRUPT);
    optimize_interrupt_priority(3, GDMA_INTERRUPT);
    
    // GDMA 채널 활성화
    gdma_enable_channel(&g_gdma_config, true);
    
    g_audio_processing_enabled = true;
    
    // 오디오 처리 태스크 생성
    if (xTaskCreatePinnedToCore(
        audio_processing_task,
        "AudioProcessing",
        8192,  // 스택 크기
        NULL,  // 파라미터
        12,    // 우선순위 (높음)
        &g_audio_processing_task_handle,
        0      // CPU 0에서 실행
    ) != pdPASS) {
        Serial.println("오디오 처리 태스크 생성 실패");
        return false;
    }
    
    Serial.println("오디오 처리기 초기화 완료");
    return true;
}

//==============================================================================
// 고성능 오디오 처리 루프
//==============================================================================
void audio_processing_task(void* parameters) {
    Serial.println("오디오 처리 태스크 시작");
    
    // 성능 모니터링 변수들
    uint32_t max_processing_cycles = 0;
    uint32_t total_processing_time = 0;
    uint32_t processing_count = 0;
    
    while (g_audio_processing_enabled) {
        uint32_t cycle_start = esp_cycle_counter();
        
        // 오디오 버퍼 확인
        size_t available_input = 0;
        
        // 입력 버퍼에서 읽을 수 있는 데이터 확인
        xSemaphoreTake(g_audio_input_buffer.mutex, portMAX_DELAY);
        if (g_audio_input_buffer.is_filled || g_audio_input_buffer.write_pos > g_audio_input_buffer.read_pos) {
            available_input = g_audio_input_buffer.is_filled ? 
                g_audio_input_buffer.buffer_size - g_audio_input_buffer.read_pos : 
                g_audio_input_buffer.write_pos - g_audio_input_buffer.read_pos;
        }
        xSemaphoreGive(g_audio_input_buffer.mutex);
        
        // 충분한 데이터가 있으면 처리
        if (available_input >= AUDIO_BUFFER_SIZE * 2) {
            // 임시 버퍼 할당 (최적화된 스택 메모리 사용)
            static int16_t temp_buffer[512];
            static int16_t output_buffer[512];
            
            // 오디오 데이터 읽기
            size_t bytes_read = read_audio_data(&g_audio_input_buffer, temp_buffer, sizeof(temp_buffer));
            
            if (bytes_read > 0) {
                // 고정 소수점 연산으로 빠른 처리
                fp16_16_t volume_scale = fp16_16_from_float(0.8f);  // 80% 볼륨
                
                // 빠른 오디오 스케일링 (고정 소수점 사용)
                fast_audio_scale(temp_buffer, output_buffer, bytes_read / sizeof(int16_t), volume_scale);
                
                // 출력 버퍼에 쓰기
                write_audio_data(&g_audio_output_buffer, output_buffer, bytes_read);
            }
        }
        
        // 성능 측정
        uint32_t cycle_end = esp_cycle_counter();
        uint32_t processing_cycles = cycle_end - cycle_start;
        
        if (processing_cycles > max_processing_cycles) {
            max_processing_cycles = processing_cycles;
        }
        
        total_processing_time += processing_cycles;
        processing_count++;
        
        // 1초마다 성능 통계 출력
        if (processing_count % 1000 == 0) {
            float avg_processing_time_us = ((float)total_processing_time / processing_count) / 160.0f;
            float max_processing_time_us = (float)max_processing_cycles / 160.0f;
            
            Serial.printf("오디오 처리 성능:\n");
            Serial.printf("  평균 처리 시간: %.3f μs\n", avg_processing_time_us);
            Serial.printf("  최대 처리 시간: %.3f μs\n", max_processing_time_us);
            Serial.printf("  처리 횟수: %lu\n", processing_count);
            Serial.printf("  사용 가능한 입력 데이터: %zu bytes\n", available_input);
            
            // 성능 메트릭 초기화
            max_processing_cycles = 0;
            total_processing_time = 0;
            processing_count = 0;
        }
        
        // 작은 지연으로 다른 태스크에게 CPU 시간 제공
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    Serial.println("오디오 처리 태스크 종료");
    vTaskDelete(NULL);
}

//==============================================================================
// 유틸리티 함수들
//==============================================================================

/**
 * @brief 메모리 사용량 정보 출력
 */
void print_memory_info() {
    multi_heap_info_t heap_info;
    heap_caps_get_info(&heap_info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    Serial.printf("=== 메모리 정보 ===\n");
    Serial.printf("총 힙 크기: %u bytes\n", heap_info.total_free_bytes + heap_info.total_allocated_bytes);
    Serial.printf("사용 가능한 메모리: %u bytes\n", heap_info.total_free_bytes);
    Serial.printf("할당된 메모리: %u bytes\n", heap_info.total_allocated_bytes);
    Serial.printf("최대 할당 가능: %u bytes\n", heap_info.largest_free_block);
}

/**
 * @brief GDMA 성능 테스트
 */
void test_gdma_performance() {
    Serial.println("=== GDMA 성능 테스트 시작 ===");
    
    // 큰 버퍼로 성능 테스트
    const size_t test_size = 8192;
    uint8_t* test_buffer = (uint8_t*)heap_caps_malloc(test_size, MALLOC_CAP_INTERNAL);
    
    if (test_buffer == NULL) {
        Serial.println("테스트 버퍼 할당 실패");
        return;
    }
    
    // 테스트 데이터 초기화
    for (size_t i = 0; i < test_size; i++) {
        test_buffer[i] = i & 0xFF;
    }
    
    // GDMA 전송 성능 측정
    uint32_t start_cycles = esp_cycle_counter();
    
    // GDMA 채널을 사용한 대용량 데이터 전송
    if (g_gdma_config.tx_channel) {
        gdma_start(g_gdma_config.tx_channel, (intptr_t)test_buffer);
        gdma_wait(g_gdma_config.tx_channel, portMAX_DELAY);
    }
    
    uint32_t end_cycles = esp_cycle_counter();
    uint32_t transfer_cycles = end_cycles - start_cycles;
    
    float transfer_time_ms = (float)transfer_cycles / 160000.0f;
    float throughput_mbps = (test_size * 8.0f) / (transfer_cycles / 160.0f);
    
    Serial.printf("GDMA 전송 성능:\n");
    Serial.printf("  전송 크기: %u bytes\n", test_size);
    Serial.printf("  전송 시간: %.3f ms\n", transfer_time_ms);
    Serial.printf("  처리량: %.2f Mbps\n", throughput_mbps);
    Serial.printf("  전송 사이클: %lu\n", transfer_cycles);
    
    free(test_buffer);
    Serial.println("=== GDMA 성능 테스트 완료 ===");
}

/**
 * @brief CPU 캐시 성능 테스트
 */
void test_cache_performance() {
    Serial.println("=== CPU 캐시 성능 테스트 시작 ===");
    
    const size_t array_size = 16384;
    uint32_t* test_array = (uint32_t*)heap_caps_malloc(array_size * sizeof(uint32_t), MALLOC_CAP_INTERNAL);
    
    if (test_array == NULL) {
        Serial.println("캐시 테스트 배열 할당 실패");
        return;
    }
    
    // 배열 초기화
    for (size_t i = 0; i < array_size; i++) {
        test_array[i] = i * 2;
    }
    
    // 순차 접근 성능 측정
    uint32_t start_cycles_seq = esp_cycle_counter();
    uint64_t sum_seq = 0;
    
    for (size_t i = 0; i < array_size; i++) {
        sum_seq += test_array[i];
    }
    
    uint32_t end_cycles_seq = esp_cycle_counter();
    uint32_t seq_cycles = end_cycles_seq - start_cycles_seq;
    
    // 임의 접근 성능 측정 (캐시 미스 유도)
    uint32_t start_cycles_rand = esp_cycle_counter();
    uint64_t sum_rand = 0;
    
    for (size_t i = 0; i < array_size; i++) {
        size_t random_index = (i * 12345 + 67890) % array_size;
        sum_rand += test_array[random_index];
    }
    
    uint32_t end_cycles_rand = esp_cycle_counter();
    uint32_t rand_cycles = end_cycles_rand - start_cycles_rand;
    
    Serial.printf("캐시 성능 테스트:\n");
    Serial.printf("  배열 크기: %u integers\n", array_size);
    Serial.printf("  순차 접근 사이클: %lu\n", seq_cycles);
    Serial.printf("  임의 접근 사이클: %lu\n", rand_cycles);
    Serial.printf("  캐시 미스 비율: %.1f%%\n", 
                  ((float)rand_cycles / seq_cycles - 1.0f) * 100.0f);
    Serial.printf("  계산 결과 합계: %llu\n", sum_seq + sum_rand);
    
    free(test_array);
    Serial.println("=== CPU 캐시 성능 테스트 완료 ===");
}

/**
 * @brief 전체 시스템 성능 테스트 실행
 */
void run_full_performance_test() {
    Serial.println("===============================================");
    Serial.println("           ESP32C3 성능 최적화 테스트 시작");
    Serial.println("===============================================");
    
    // 메모리 정보 출력
    print_memory_info();
    Serial.println();
    
    // GDMA 성능 테스트
    test_gdma_performance();
    Serial.println();
    
    // CPU 캐시 성능 테스트
    test_cache_performance();
    Serial.println();
    
    // 오디오 처리기 성능 테스트
    if (g_audio_processing_enabled) {
        Serial.println("오디오 처리기 성능 테스트:");
        
        // 테스트 데이터 생성
        static int16_t test_audio[512];
        for (int i = 0; i < 512; i++) {
            test_audio[i] = sin(i * 0.1) * 3000;
        }
        
        // 오디오 처리 성능 측정
        uint32_t start_cycles = esp_cycle_counter();
        
        fp16_16_t test_scale = fp16_16_from_float(1.5f);
        int16_t test_output[512];
        fast_audio_scale(test_audio, test_output, 512, test_scale);
        
        uint32_t end_cycles = esp_cycle_counter();
        uint32_t audio_cycles = end_cycles - start_cycles;
        
        float audio_processing_time_us = (float)audio_cycles / 160.0f;
        float max_latency_us = (float)AUDIO_BLOCK_SIZE * 8.0f / AUDIO_SAMPLE_RATE;
        
        Serial.printf("  오디오 처리 시간: %.3f μs\n", audio_processing_time_us);
        Serial.printf("  최대 지연 시간: %.3f μs\n", max_latency_us);
        Serial.printf("  지연 시간 여유분: %.1f%%\n", 
                      (max_latency_us / audio_processing_time_us) * 100.0f);
    }
    
    Serial.println("===============================================");
    Serial.println("           ESP32C3 성능 최적화 테스트 완료");
    Serial.println("===============================================");
}