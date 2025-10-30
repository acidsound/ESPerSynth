/*
 * Mozzi 기반 TR-808 드럼 클래스 구현
 * 
 * 고성능 Mozzi Library를 완전히 활용한 TR-808 드럼 알고리즘
 * fastMath, generation, envelope 최적화, 64kHz 샘플링 레이트 지원
 * 
 * 작성일: 2025-10-30
 * 호환성: ESP32C3 + Mozzi Library
 */

#ifndef MOZZI_TR808_DRUMS_H
#define MOZZI_TR808_DRUMS_H

#include <Arduino.h>
#include <MozziGuts.h>
#include <mozzi_fixmath.h>
#include <mozzi_midi.h>
#include <Oscil.h>
#include <tables/square2048_int8.h>
#include <tables/triangle2048_int8.h>
#include <tables/sin2048_int8.h>
#include <tables/brownnoise8192_int8.h>
#include <tables/cos2048_int8.h>
#include <tables/envelope___adsr.h>
#include <mozzi_utils.h>
#include <DCfilter.h>
#include <LowPassFilter.h>
#include <HighPassFilter.h>
#include <ResonantFilter.h>
#include <BitCrusher.h>
#include <Phasor.h>
#include <RMS.h>
#include <AutoMap.h>

// Mozzi 64kHz 설정
#define MOZZI_TR808_AUDIO_RATE 64000
#define MOZZI_TR808_CONTROL_RATE 512

// TR-808 음정 정의 (fastMath 사용)
#define TR808_FREQ_C1   65536  // 32.7Hz
#define TR808_FREQ_C2   131072 // 65.4Hz  
#define TR808_FREQ_D2   147456 // 73.4Hz
#define TR808_FREQ_F#2  185856 // 92.5Hz
#define TR808_FREQ_A2   207360 // 103.4Hz
#define TR808_FREQ_C3   262144 // 130.8Hz

// 폴리포니 설정
#define TR808_MAX_VOICES 8
#define TR808_KICK_VOICES 2
#define TR808_SNARE_VOICES 2  
#define TR808_CYMBAL_VOICES 2
#define TR808_HIHAT_VOICES 2

// Envelope 설정 (fastMath 사용)
#define TR808_DECAY_TIME 2000    // 2초 maximum
#define TR808_ATTACK_TIME 100    // 0.1초
#define TR808_RELEASE_TIME 500   // 0.5초
#define TR808_SUSTAIN_LEVEL 32768 // 0.5

// 브리지드-T 발진기 설정
#define TR808_BRIDGED_T_FREQ 100.0f
#define TR808_BRIDGED_T_Q 5.0f
#define TR808_BRIDGED_T_RESONANCE 0.7f

// 드럼 타입 열거
enum TR808DrumType {
    TR808_KICK = 0,
    TR808_SNARE = 1,
    TR808_CYMBAL = 2,
    TR808_HIHAT = 3
};

/**
 * TR-808 Kick 드럼 (fastMath 활용)
 * 고정 소수점 수학 연산으로 성능 향상
 */
class TR808KickMozzi {
private:
    // fastMath 기반 변수들
    Q16n16 _frequency;
    Q16n16 _decay_time;
    Q16n16 _pitch_decay;
    Q16n16 _current_pitch;
    
    //Envelope
    ADSR<CONTROL_RATE, AUDIO_RATE> _envelope;
    
    // 상태 변수
    bool _is_playing;
    uint32_t _start_time;
    uint32_t _note_duration;
    
    // 성능 최적화 변수
    static constexpr int _table_size = 256;
    int _lookup_index;
    
public:
    TR808KickMozzi();
    
    // 기본 operations
    void start();
    void stop();
    void setFrequency(float freq_hz);
    void setDecayTime(float decay_ms);
    
    // Audio rate update (optimized)
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // State queries
    bool isPlaying() const { return _is_playing; }
    bool isFinished() const;
    
private:
    // TR-808 고유 알고리즘 (fastMath)
    Q15n16 generateKickWave(Q16n16 phase) IRAM_ATTR;
    void updatePitchDecay() IRAM_ATTR;
};

/**
 * TR-808 Snare 드럼 (envelope 최적화)
 * 고성능 envelope 처리
 */
class TR808SnareMozzi {
private:
    // Noise 소스 (brown noise table 사용)
    Oscil<BROWNNOISE8192_ISTEP, MOZZI_TR808_AUDIO_RATE> _noise_osc;
    
    // Tone component
    Oscil<SQUARE2048_ISTEP, MOZZI_TR808_AUDIO_RATE> _tone_osc;
    
    // Envelope (optimized)
    ADSR<CONTROL_RATE, AUDIO_RATE> _noise_env;
    ADSR<CONTROL_RATE, AUDIO_RATE> _tone_env;
    
    // Filters
    HighPassFilter _highpass;
    LowPassFilter _lowpass;
    
    // 상태 변수
    bool _is_playing;
    uint32_t _start_time;
    
    // Envelope 최적화
    Q16n16 _decay_time;
    Q16n16 _tone_decay;
    
public:
    TR808SnareMozzi();
    
    // 기본 operations
    void start();
    void stop();
    void setDecayTime(float decay_ms);
    void setTone(float tone_hz);
    
    // Audio rate update
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // State queries
    bool isPlaying() const { return _is_playing; }
    bool isFinished() const;
};

/**
 * TR-808 Cymbal 드럼 (generation 활용)
 * Mozzi generation 함수들 사용
 */
class TR808CymbalMozzi {
private:
    // Noise 기반 소스 (여러 주파수 성분)
    Oscil<SIN2048_ISTEP, MOZZI_TR808_AUDIO_RATE> _osc1;
    Oscil<SIN2048_ISTEP, MOZZI_TR808_AUDIO_RATE> _osc2;
    Oscil<SIN2048_ISTEP, MOZZI_TR808_AUDIO_RATE> _osc3;
    Oscil<BROWNNOISE8192_ISTEP, MOZZI_TR808_AUDIO_RATE> _noise;
    
    // Band-pass filters (metallic sound)
    ResonantFilter _bandpass1;
    ResonantFilter _bandpass2;
    ResonantFilter _bandpass3;
    
    // Envelope
    ADSR<CONTROL_RATE, AUDIO_RATE> _envelope;
    
    // 상태 변수
    bool _is_playing;
    Q16n16 _decay_time;
    Q16n16 _resonance;
    
    // Frequency modulation
    Phasor _fm_phase;
    Q16n16 _fm_frequency;
    Q16n16 _fm_depth;
    
public:
    TR808CymbalMozzi();
    
    // 기본 operations
    void start();
    void stop();
    void setDecayTime(float decay_ms);
    void setResonance(float resonance);
    void setFMDepth(float depth);
    
    // Audio rate update
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // State queries
    bool isPlaying() const { return _is_playing; }
    bool isFinished() const;
};

/**
 * TR-808 Hi-hat 드럼 (고성능 envelope)
 * 최적화된 envelope 처리
 */
class TR808HihatMozzi {
private:
    // High-frequency noise
    Oscil<BROWNNOISE8192_ISTEP, MOZZI_TR808_AUDIO_RATE> _noise;
    
    // Multiple band-pass filters
    HighPassFilter _hp1;
    HighPassFilter _hp2;
    LowPassFilter _lp;
    
    // Envelope (fast decay)
    ADSR<CONTROL_RATE, AUDIO_RATE> _envelope;
    
    // 상태 변수
    bool _is_playing;
    Q16n16 _decay_time;
    Q16n16 _cutoff_freq;
    
    // Envelope 최적화 파라미터
    Q16n16 _attack_coeff;
    Q16n16 _decay_coeff;
    
public:
    TR808HihatMozzi();
    
    // 기본 operations
    void start();
    void stop();
    void setDecayTime(float decay_ms);
    void setCutoff(float cutoff_hz);
    void setOpen(bool open);
    
    // Audio rate update
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // State queries
    bool isPlaying() const { return _is_playing; }
    bool isFinished() const;
};

/**
 * TR-808 브리지드-T 발진기 (fastMath)
 * 고정 소수점 연산을 위한 특수 발진기
 */
class TR808BridgedTOscillatorMozzi {
private:
    //-fastMath 기반 주파수 제어
    Q16n16 _frequency;
    Q16n16 _phase;
    Q16n16 _phase_increment;
    Q16n16 _resonance;
    Q16n16 _capacitance;
    
    // 상태 변수
    bool _is_active;
    Q15n16 _output;
    
    // 브리지드-T 네트워크 계수 (고정 소수점)
    Q16n16 _rc_coeff;
    Q16n16 _feedback_coeff;
    
public:
    TR808BridgedTOscillatorMozzi();
    
    // 기본 operations
    void setFrequency(float freq_hz);
    void setResonance(float resonance);
    void setCapacitance(float capacitance);
    void start();
    void stop();
    
    // Audio rate update (fastMath 최적화)
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // State queries
    bool isActive() const { return _is_active; }
    
private:
    // 브리지드-T 알고리즘 (fixed-point)
    Q15n16 calculateBridgedTOutput(Q16n16 input, Q16n16 feedback) IRAM_ATTR;
    void updateCoefficients() IRAM_ATTR;
};

/**
 * TR-808 드럼 머신 (폴리포니 지원)
 * 여러 드럼 소스를 동시에 재생
 */
class TR808DrumMachineMozzi {
private:
    // 드럼 voices (폴리포니)
    TR808KickMozzi _kicks[TR808_KICK_VOICES];
    TR808SnareMozzi _snares[TR808_SNARE_VOICES];
    TR808CymbalMozzi _cymbals[TR808_CYMBAL_VOICES];
    TR808HihatMozzi _hihats[TR808_HIHAT_VOICES];
    
    // Voice management
    uint8_t _kick_voice_index;
    uint8_t _snare_voice_index;
    uint8_t _cymbal_voice_index;
    uint8_t _hihat_voice_index;
    
    // Global envelope/compression
    RMS _rms;
    BitCrusher _bitcrusher;
    LowPassFilter _master_lpf;
    
    // Performance monitoring
    bool _performance_mode;
    uint32_t _processing_time_us;
    uint32_t _max_processing_time_us;
    
    // Audio mixing
    Q15n16 _mix_levels[4]; // kick, snare, cymbal, hihat
    
public:
    TR808DrumMachineMozzi();
    
    // 초기화
    void begin();
    void setSampleRate(uint32_t rate);
    
    // Drum triggers
    void triggerKick();
    void triggerSnare();
    void triggerCymbal();
    void triggerHihat();
    
    // Parameter control
    void setKickDecay(float decay_ms);
    void setSnareDecay(float decay_ms);
    void setCymbalDecay(float decay_ms);
    void setHihatDecay(float decay_ms);
    void setMixLevel(uint8_t drum_type, float level);
    
    // Audio rate update (main processing)
    Q15n16 next() IRAM_ATTR;
    
    // Control rate update
    void update();
    
    // Performance monitoring
    void enablePerformanceMode(bool enable);
    uint32_t getProcessingTime() const { return _processing_time_us; }
    uint32_t getMaxProcessingTime() const { return _max_processing_time_us; }
    float getCPUUsage() const;
    
    // State management
    void stopAll();
    bool isAnyVoicePlaying() const;
    
    // Advanced features
    void setMasterVolume(float volume);
    void setBitCrushDepth(uint8_t depth);
    void setMasterFilterCutoff(float cutoff_hz);
    
private:
    // Voice allocation
    uint8_t allocateKickVoice() IRAM_ATTR;
    uint8_t allocateSnareVoice() IRAM_ATTR;
    uint8_t allocateCymbalVoice() IRAM_ATTR;
    uint8_t allocateHihatVoice() IRAM_ATTR;
    
    // Audio mixing (optimized)
    Q15n16 mixVoices() IRAM_ATTR;
    void applyMasterProcessing(Q15n16 &audio) IRAM_ATTR;
    
    // Performance optimization
    void optimizeForPerformance() IRAM_ATTR;
    void updateProcessingTime() IRAM_ATTR;
};

// =============================================================================
// 성능 최적화 매크로들
// =============================================================================

// ISR 최적화 매크로
#define TR808_ISR_OPTIMIZED __attribute__((always_inline)) IRAM_ATTR

// 메모리 최적화 매크로  
#define TR808_USE_DTCM __attribute__((section(".dtcm")))

// fastMath 최적화 매크로
#define TR808_FASTMATH_INLINE static inline __attribute__((always_inline)) 

// Audio rate 최적화
#define TR808_AUDIO_INLINE static inline Q15n16 __attribute__((always_inline)) IRAM_ATTR

#endif /* MOZZI_TR808_DRUMS_H */