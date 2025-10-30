/*
 * Mozzi 기반 TR-808 드럼 클래스 구현
 * 
 * 고성능 Mozzi Library를 완전히 활용한 TR-808 드럼 알고리즘
 * fastMath, generation, envelope 최적화, 64kHz 샘플링 레이트 지원
 * 
 * 작성일: 2025-10-30
 * 호환성: ESP32C3 + Mozzi Library
 */

#include "mozzi_tr808_drums.h"

// =============================================================================
// TR808KickMozzi 구현
// =============================================================================

TR808KickMozzi::TR808KickMozzi() 
    : _frequency(TR808_FREQ_C2), _decay_time(TR808_DECAY_TIME)
    , _pitch_decay(0), _current_pitch(TR808_FREQ_C2)
    , _envelope(), _is_playing(false), _start_time(0)
    , _note_duration(1000), _lookup_index(0) {
    
    // Kick envelope 설정
    _envelope.setADLevels(32768, 16384);
    _envelope.setTimes(TR808_ATTACK_TIME, 500, 200, TR808_RELEASE_TIME); // A, D, S, R
}

TR808_FASTMATH_INLINE void TR808KickMozzi::setFrequency(float freq_hz) {
    _frequency = Q16n16::toQ16n16(freq_hz);
    _current_pitch = _frequency;
}

TR808_FASTMATH_INLINE void TR808KickMozzi::setDecayTime(float decay_ms) {
    _decay_time = Q16n16::toQ16n16(decay_ms);
    _envelope.setDecayTime((int)decay_ms);
}

TR808_AUDIO_INLINE void TR808KickMozzi::start() {
    _is_playing = true;
    _start_time = millis();
    _current_pitch = _frequency;
    _pitch_decay = 0;
    _envelope.start();
}

TR808_AUDIO_INLINE void TR808KickMozzi::stop() {
    _is_playing = false;
    _envelope.stop();
}

TR808_ISR_OPTIMIZED void TR808KickMozzi::updatePitchDecay() {
    // Pitch envelope: exponential decay
    Q16n16 decay_rate = Q16n16(1) - Q16n16::div(Q16n16(1), _decay_time);
    _pitch_decay = _pitch_decay * decay_rate + Q16n16(1) * Q16n16::div(1, _decay_time);
    _current_pitch = _frequency - (_frequency * _pitch_decay);
}

TR808_AUDIO_INLINE Q15n16 TR808KickMozzi::generateKickWave(Q16n16 phase) {
    // TR-808 kick wave: sine wave with exponential decay
    // Fast lookup table approach
    int table_index = (int)(phase >> 8) & 0xFF;
    Q15n16 sine_value = sin2048_int8[table_index];
    
    // Apply pitch modulation
    Q15n16 pitch_modulated = (Q15n16)((int32_t)sine_value * (int32_t)_current_pitch >> 16);
    
    return pitch_modulated;
}

TR808_AUDIO_INLINE Q15n16 TR808KickMozzi::next() {
    if (!_is_playing) return 0;
    
    // Generate phase increment
    Q16n16 phase_increment = _current_pitch >> 8;
    _phase += phase_increment;
    
    // Wrap phase
    if (_phase >= Q16n16(1)) {
        _phase -= Q16n16(1);
    }
    
    // Generate wave
    Q15n16 wave = generateKickWave(_phase);
    
    // Apply envelope
    Q15n16 envelope_value = _envelope.next();
    Q15n16 output = (Q15n16)((int32_t)wave * (int32_t)envelope_value >> 15);
    
    // Check if finished
    if (envelope_value == 0) {
        _is_playing = false;
    }
    
    return output;
}

TR808_ISR_OPTIMIZED void TR808KickMozzi::update() {
    if (_is_playing) {
        updatePitchDecay();
        _envelope.update();
    }
}

TR808_FASTMATH_INLINE bool TR808KickMozzi::isFinished() const {
    return !_is_playing && _envelope.isFinished();
}

// =============================================================================
// TR808SnareMozzi 구현
// =============================================================================

TR808SnareMozzi::TR808SnareMozzi()
    : _noise_osc(BROWNNOISE8192_DATA)
    , _tone_osc(SQUARE2048_DATA)
    , _noise_env(), _tone_env()
    , _highpass(), _lowpass()
    , _is_playing(false), _start_time(0)
    , _decay_time(TR808_DECAY_TIME), _tone_decay(1000) {
    
    // Envelope 설정
    _noise_env.setADLevels(32768, 0);
    _noise_env.setTimes(10, 500, 0, 200);
    
    _tone_env.setADLevels(32768, 0);
    _tone_env.setTimes(5, _tone_decay, 0, 100);
    
    // Filter 설정
    _highpass.setCutoff(2000);
    _lowpass.setCutoff(10000);
}

TR808_FASTMATH_INLINE void TR808SnareMozzi::setDecayTime(float decay_ms) {
    _decay_time = Q16n16::toQ16n16(decay_ms);
    _noise_env.setDecayTime((int)decay_ms);
}

TR808_FASTMATH_INLINE void TR808SnareMozzi::setTone(float tone_hz) {
    _tone_osc.setFreq((int)tone_hz);
}

TR808_AUDIO_INLINE void TR808SnareMozzi::start() {
    _is_playing = true;
    _start_time = millis();
    _noise_env.start();
    _tone_env.start();
}

TR808_AUDIO_INLINE void TR808SnareMozzi::stop() {
    _is_playing = false;
    _noise_env.stop();
    _tone_env.stop();
}

TR808_AUDIO_INLINE Q15n16 TR808SnareMozzi::next() {
    if (!_is_playing) return 0;
    
    // Generate noise component
    Q15n16 noise = _noise_osc.next();
    
    // Generate tone component
    Q15n16 tone = _tone_osc.next();
    
    // Mix noise and tone
    Q15n16 mixed = (noise + tone) >> 1;
    
    // Apply filters
    mixed = _highpass.next(mixed);
    mixed = _lowpass.next(mixed);
    
    // Apply envelopes
    Q15n16 noise_env = _noise_env.next();
    Q15n16 tone_env = _tone_env.next();
    
    // Combine with envelopes
    Q15n16 output = ((noise * noise_env) + (tone * tone_env)) >> 1;
    
    // Check if finished
    if (noise_env == 0 && tone_env == 0) {
        _is_playing = false;
    }
    
    return output;
}

TR808_ISR_OPTIMIZED void TR808SnareMozzi::update() {
    if (_is_playing) {
        _noise_env.update();
        _tone_env.update();
        _highpass.update();
        _lowpass.update();
    }
}

TR808_FASTMATH_INLINE bool TR808SnareMozzi::isFinished() const {
    return !_is_playing && _noise_env.isFinished() && _tone_env.isFinished();
}

// =============================================================================
// TR808CymbalMozzi 구현
// =============================================================================

TR808CymbalMozzi::TR808CymbalMozzi()
    : _osc1(SIN2048_DATA), _osc2(SIN2048_DATA), _osc3(SIN2048_DATA)
    , _noise(BROWNNOISE8192_DATA)
    , _bandpass1(), _bandpass2(), _bandpass3()
    , _envelope()
    , _is_playing(false), _decay_time(2000), _resonance(0.5f)
    , _fm_phase(0), _fm_frequency(100), _fm_depth(0.1f) {
    
    // Oscillator frequencies (harmonic series)
    _osc1.setFreq(800);    // Fundamental
    _osc2.setFreq(1600);   // 2nd harmonic
    _osc3.setFreq(2400);   // 3rd harmonic
    
    // Band-pass filters for metallic sound
    _bandpass1.setCutoffFreq(800);
    _bandpass1.setResonance(TR808_BRIDGED_T_RESONANCE);
    
    _bandpass2.setCutoffFreq(1600);
    _bandpass2.setResonance(TR808_BRIDGED_T_RESONANCE);
    
    _bandpass3.setCutoffFreq(2400);
    _bandpass3.setResonance(TR808_BRIDGED_T_RESONANCE);
    
    // Envelope
    _envelope.setADLevels(32768, 0);
    _envelope.setTimes(50, 1000, 0, 200);
    
    // FM settings
    _fm_frequency = Q16n16::toQ16n16(100);
    _fm_depth = Q16n16::toQ16n16(0.1f);
}

TR808_FASTMATH_INLINE void TR808CymbalMozzi::setDecayTime(float decay_ms) {
    _decay_time = Q16n16::toQ16n16(decay_ms);
    _envelope.setDecayTime((int)decay_ms);
}

TR808_FASTMATH_INLINE void TR808CymbalMozzi::setResonance(float resonance) {
    _resonance = resonance;
    _bandpass1.setResonance(resonance);
    _bandpass2.setResonance(resonance);
    _bandpass3.setResonance(resonance);
}

TR808_FASTMATH_INLINE void TR808CymbalMozzi::setFMDepth(float depth) {
    _fm_depth = Q16n16::toQ16n16(depth);
}

TR808_AUDIO_INLINE void TR808CymbalMozzi::start() {
    _is_playing = true;
    _envelope.start();
    _fm_phase = 0;
}

TR808_AUDIO_INLINE void TR808CymbalMozzi::stop() {
    _is_playing = false;
    _envelope.stop();
}

TR808_AUDIO_INLINE Q15n16 TR808CymbalMozzi::next() {
    if (!_is_playing) return 0;
    
    // Generate FM modulation
    _fm_phase += _fm_frequency >> 8;
    if (_fm_phase >= Q16n16(1)) {
        _fm_phase -= Q16n16(1);
    }
    
    Q15n16 fm_value = sin2048_int8[(int)(_fm_phase >> 8) & 0xFF];
    Q16n16 fm_modulation = _fm_depth * fm_value;
    
    // Generate oscillator components with FM
    Q15n16 osc1_out = _osc1.next();
    Q15n16 osc2_out = _osc2.next();  
    Q15n16 osc3_out = _osc3.next();
    Q15n16 noise_out = _noise.next();
    
    // Apply FM to oscillators
    osc1_out += (osc1_out * fm_modulation) >> 16;
    osc2_out += (osc2_out * fm_modulation) >> 16;
    osc3_out += (osc3_out * fm_modulation) >> 16;
    
    // Mix all components
    Q15n16 mixed = (osc1_out + osc2_out + osc3_out + noise_out) >> 2;
    
    // Apply band-pass filters
    mixed = _bandpass1.next(mixed);
    mixed = _bandpass2.next(mixed);
    mixed = _bandpass3.next(mixed);
    
    // Apply envelope
    Q15n16 envelope_value = _envelope.next();
    Q15n16 output = (mixed * envelope_value) >> 15;
    
    // Check if finished
    if (envelope_value == 0) {
        _is_playing = false;
    }
    
    return output;
}

TR808_ISR_OPTIMIZED void TR808CymbalMozzi::update() {
    if (_is_playing) {
        _envelope.update();
        _bandpass1.update();
        _bandpass2.update();
        _bandpass3.update();
    }
}

TR808_FASTMATH_INLINE bool TR808CymbalMozzi::isFinished() const {
    return !_is_playing && _envelope.isFinished();
}

// =============================================================================
// TR808HihatMozzi 구현
// =============================================================================

TR808HihatMozzi::TR808HihatMozzi()
    : _noise(BROWNNOISE8192_DATA)
    , _hp1(), _hp2(), _lp()
    , _envelope()
    , _is_playing(false), _decay_time(200)
    , _cutoff_freq(8000), _attack_coeff(0), _decay_coeff(0) {
    
    // Filter setup for bright hi-hat sound
    _hp1.setCutoff(6000);
    _hp2.setCutoff(10000);
    _lp.setCutoff(12000);
    
    // Envelope (fast attack, short decay)
    _envelope.setADLevels(32768, 0);
    _envelope.setTimes(10, 200, 0, 50);
    
    // Calculate envelope coefficients
    _attack_coeff = Q16n16::toQ16n16(0.1f);
    _decay_coeff = Q16n16::toQ16n16(0.95f);
}

TR808_FASTMATH_INLINE void TR808HihatMozzi::setDecayTime(float decay_ms) {
    _decay_time = Q16n16::toQ16n16(decay_ms);
    _envelope.setDecayTime((int)decay_ms);
}

TR808_FASTMATH_INLINE void TR808HihatMozzi::setCutoff(float cutoff_hz) {
    _cutoff_freq = Q16n16::toQ16n16(cutoff_hz);
    _hp1.setCutoff(cutoff_hz);
    _hp2.setCutoff(cutoff_hz * 1.5f);
}

TR808_FASTMATH_INLINE void TR808HihatMozzi::setOpen(bool open) {
    if (open) {
        _envelope.setDecayTime(800);
    } else {
        _envelope.setDecayTime(200);
    }
}

TR808_AUDIO_INLINE void TR808HihatMozzi::start() {
    _is_playing = true;
    _envelope.start();
}

TR808_AUDIO_INLINE void TR808HihatMozzi::stop() {
    _is_playing = false;
    _envelope.stop();
}

TR808_AUDIO_INLINE Q15n16 TR808HihatMozzi::next() {
    if (!_is_playing) return 0;
    
    // Generate high-frequency noise
    Q15n16 noise = _noise.next();
    
    // Apply multiple filters for brightness
    noise = _hp1.next(noise);
    noise = _hp2.next(noise);
    noise = _lp.next(noise);
    
    // Apply fast envelope
    Q15n16 envelope_value = _envelope.next();
    Q15n16 output = (noise * envelope_value) >> 15;
    
    // Check if finished
    if (envelope_value == 0) {
        _is_playing = false;
    }
    
    return output;
}

TR808_ISR_OPTIMIZED void TR808HihatMozzi::update() {
    if (_is_playing) {
        _envelope.update();
        _hp1.update();
        _hp2.update();
        _lp.update();
    }
}

TR808_FASTMATH_INLINE bool TR808HihatMozzi::isFinished() const {
    return !_is_playing && _envelope.isFinished();
}

// =============================================================================
// TR808BridgedTOscillatorMozzi 구현
// =============================================================================

TR808BridgedTOscillatorMozzi::TR808BridgedTOscillatorMozzi()
    : _frequency(TR808_FREQ_C1), _phase(0), _phase_increment(0)
    , _resonance(TR808_BRIDGED_T_RESONANCE), _capacitance(0.01f)
    , _is_active(false), _output(0)
    , _rc_coeff(0), _feedback_coeff(0) {
    
    updateCoefficients();
    setFrequency(TR808_BRIDGED_T_FREQ);
}

TR808_FASTMATH_INLINE void TR808BridgedTOscillatorMozzi::setFrequency(float freq_hz) {
    _frequency = Q16n16::toQ16n16(freq_hz);
    _phase_increment = _frequency >> 8; // Convert to phase increment
}

TR808_FASTMATH_INLINE void TR808BridgedTOscillatorMozzi::setResonance(float resonance) {
    _resonance = Q16n16::toQ16n16(resonance);
    updateCoefficients();
}

TR808_FASTMATH_INLINE void TR808BridgedTOscillatorMozzi::setCapacitance(float capacitance) {
    _capacitance = Q16n16::toQ16n16(capacitance);
    updateCoefficients();
}

TR808_ISR_OPTIMIZED void TR808BridgedTOscillatorMozzi::updateCoefficients() {
    // Calculate RC network coefficients (fixed-point)
    _rc_coeff = Q16n16::toQ16n16(1.0f / (2.0f * PI * TR808_BRIDGED_T_FREQ * _capacitance));
    _feedback_coeff = _resonance * _rc_coeff;
}

TR808_AUDIO_INLINE void TR808BridgedTOscillatorMozzi::start() {
    _is_active = true;
    _phase = 0;
}

TR808_AUDIO_INLINE void TR808BridgedTOscillatorMozzi::stop() {
    _is_active = false;
}

TR808_AUDIO_INLINE Q15n16 TR808BridgedTOscillatorMozzi::calculateBridgedTOutput(Q16n16 input, Q16n16 feedback) {
    // Bridged-T network algorithm (fixed-point)
    // Output = Input - Feedback * RC_coefficient
    Q16n16 filtered = input - (feedback * _rc_coeff);
    
    // Apply resonance
    Q16n16 resonant = filtered + (filtered * _feedback_coeff >> 16);
    
    return (Q15n16)(resonant >> 1);
}

TR808_AUDIO_INLINE Q15n16 TR808BridgedTOscillatorMozzi::next() {
    if (!_is_active) return 0;
    
    // Update phase
    _phase += _phase_increment;
    if (_phase >= Q16n16(1)) {
        _phase -= Q16n16(1);
    }
    
    // Generate sine wave
    int table_index = (int)(_phase >> 8) & 0xFF;
    Q15n16 sine_wave = sin2048_int8[table_index];
    
    // Apply bridged-T filter
    Q16n16 input = Q16n16(sine_wave) << 8; // Convert to Q16n16
    Q16n16 feedback = Q16n16(_output) << 8;
    
    _output = calculateBridgedTOutput(input, feedback);
    
    return _output;
}

TR808_ISR_OPTIMIZED void TR808BridgedTOscillatorMozzi::update() {
    if (_is_active) {
        // Update filter coefficients if needed
        updateCoefficients();
    }
}

// =============================================================================
// TR808DrumMachineMozzi 구현
// =============================================================================

TR808DrumMachineMozzi::TR808DrumMachineMozzi()
    : _kick_voice_index(0), _snare_voice_index(0)
    , _cymbal_voice_index(0), _hihat_voice_index(0)
    , _rms(), _bitcrusher(8), _master_lpf()
    , _performance_mode(false), _processing_time_us(0)
    , _max_processing_time_us(0) {
    
    // Set default mix levels
    _mix_levels[0] = Q15n16(0.8f); // Kick
    _mix_levels[1] = Q15n16(0.7f); // Snare
    _mix_levels[2] = Q15n16(0.6f); // Cymbal
    _mix_levels[3] = Q15n16(0.5f); // Hi-hat
    
    // Master filter setup
    _master_lpf.setCutoff(15000);
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::begin() {
    // Initialize all voices
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        _kicks[i] = TR808KickMozzi();
    }
    
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        _snares[i] = TR808SnareMozzi();
    }
    
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        _cymbals[i] = TR808CymbalMozzi();
    }
    
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        _hihats[i] = TR808HihatMozzi();
    }
    
    // Start performance monitoring if enabled
    if (_performance_mode) {
        optimizeForPerformance();
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setSampleRate(uint32_t rate) {
    // Adjust all components for new sample rate
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        // Update kick sample rate dependent parameters
    }
    
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        // Update snare sample rate dependent parameters
    }
    
    // Similar for other drum types...
}

TR808_ISR_OPTIMIZED uint8_t TR808DrumMachineMozzi::allocateKickVoice() {
    // Allocate first available kick voice
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        if (!_kicks[i].isPlaying()) {
            _kick_voice_index = i;
            return i;
        }
    }
    // If all busy, use round-robin
    _kick_voice_index = (_kick_voice_index + 1) % TR808_KICK_VOICES;
    return _kick_voice_index;
}

TR808_ISR_OPTIMIZED uint8_t TR808DrumMachineMozzi::allocateSnareVoice() {
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        if (!_snares[i].isPlaying()) {
            _snare_voice_index = i;
            return i;
        }
    }
    _snare_voice_index = (_snare_voice_index + 1) % TR808_SNARE_VOICES;
    return _snare_voice_index;
}

TR808_ISR_OPTIMIZED uint8_t TR808DrumMachineMozzi::allocateCymbalVoice() {
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        if (!_cymbals[i].isPlaying()) {
            _cymbal_voice_index = i;
            return i;
        }
    }
    _cymbal_voice_index = (_cymbal_voice_index + 1) % TR808_CYMBAL_VOICES;
    return _cymbal_voice_index;
}

TR808_ISR_OPTIMIZED uint8_t TR808DrumMachineMozzi::allocateHihatVoice() {
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        if (!_hihats[i].isPlaying()) {
            _hihat_voice_index = i;
            return i;
        }
    }
    _hihat_voice_index = (_hihat_voice_index + 1) % TR808_HIHAT_VOICES;
    return _hihat_voice_index;
}

TR808_AUDIO_INLINE void TR808DrumMachineMozzi::triggerKick() {
    uint8_t voice = allocateKickVoice();
    _kicks[voice].start();
}

TR808_AUDIO_INLINE void TR808DrumMachineMozzi::triggerSnare() {
    uint8_t voice = allocateSnareVoice();
    _snares[voice].start();
}

TR808_AUDIO_INLINE void TR808DrumMachineMozzi::triggerCymbal() {
    uint8_t voice = allocateCymbalVoice();
    _cymbals[voice].start();
}

TR808_AUDIO_INLINE void TR808DrumMachineMozzi::triggerHihat() {
    uint8_t voice = allocateHihatVoice();
    _hihats[voice].start();
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setKickDecay(float decay_ms) {
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        _kicks[i].setDecayTime(decay_ms);
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setSnareDecay(float decay_ms) {
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        _snares[i].setDecayTime(decay_ms);
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setCymbalDecay(float decay_ms) {
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        _cymbals[i].setDecayTime(decay_ms);
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setHihatDecay(float decay_ms) {
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        _hihats[i].setDecayTime(decay_ms);
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setMixLevel(uint8_t drum_type, float level) {
    if (drum_type < 4) {
        _mix_levels[drum_type] = Q15n16(level);
    }
}

TR808_ISR_OPTIMIZED void TR808DrumMachineMozzi::updateProcessingTime() {
    if (_performance_mode) {
        uint32_t current_time = micros();
        // Processing time would be calculated in next() method
    }
}

TR808_AUDIO_INLINE Q15n16 TR808DrumMachineMozzi::mixVoices() IRAM_ATTR {
    Q15n16 mixed = 0;
    
    // Mix all active kick voices
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        if (_kicks[i].isPlaying()) {
            Q15n16 kick_out = _kicks[i].next();
            mixed += (kick_out * _mix_levels[0]) >> 15;
        }
    }
    
    // Mix all active snare voices
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        if (_snares[i].isPlaying()) {
            Q15n16 snare_out = _snares[i].next();
            mixed += (snare_out * _mix_levels[1]) >> 15;
        }
    }
    
    // Mix all active cymbal voices
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        if (_cymbals[i].isPlaying()) {
            Q15n16 cymbal_out = _cymbals[i].next();
            mixed += (cymbal_out * _mix_levels[2]) >> 15;
        }
    }
    
    // Mix all active hi-hat voices
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        if (_hihats[i].isPlaying()) {
            Q15n16 hihat_out = _hihats[i].next();
            mixed += (hihat_out * _mix_levels[3]) >> 15;
        }
    }
    
    return mixed;
}

TR808_ISR_OPTIMIZED void TR808DrumMachineMozzi::applyMasterProcessing(Q15n16 &audio) {
    // Apply RMS compression
    audio = _rms.next(audio);
    
    // Apply bit crushing
    audio = _bitcrusher.next(audio);
    
    // Apply master filter
    audio = _master_lpf.next(audio);
    
    // Normalize output
    if (audio > 32767) audio = 32767;
    else if (audio < -32768) audio = -32768;
}

TR808_AUDIO_INLINE Q15n16 TR808DrumMachineMozzi::next() {
    if (_performance_mode) {
        uint32_t start_time = micros();
    }
    
    // Mix all voices
    Q15n16 mixed_audio = mixVoices();
    
    // Apply master processing
    applyMasterProcessing(mixed_audio);
    
    if (_performance_mode) {
        uint32_t end_time = micros();
        _processing_time_us = end_time - start_time;
        if (_processing_time_us > _max_processing_time_us) {
            _max_processing_time_us = _processing_time_us;
        }
    }
    
    return mixed_audio;
}

TR808_ISR_OPTIMIZED void TR808DrumMachineMozzi::update() {
    // Update all drum voices
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        _kicks[i].update();
    }
    
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        _snares[i].update();
    }
    
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        _cymbals[i].update();
    }
    
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        _hihats[i].update();
    }
    
    // Update master processing components
    _rms.update();
    _master_lpf.update();
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::stopAll() {
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        _kicks[i].stop();
    }
    
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        _snares[i].stop();
    }
    
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        _cymbals[i].stop();
    }
    
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        _hihats[i].stop();
    }
}

TR808_FASTMATH_INLINE bool TR808DrumMachineMozzi::isAnyVoicePlaying() const {
    for (int i = 0; i < TR808_KICK_VOICES; i++) {
        if (_kicks[i].isPlaying()) return true;
    }
    
    for (int i = 0; i < TR808_SNARE_VOICES; i++) {
        if (_snares[i].isPlaying()) return true;
    }
    
    for (int i = 0; i < TR808_CYMBAL_VOICES; i++) {
        if (_cymbals[i].isPlaying()) return true;
    }
    
    for (int i = 0; i < TR808_HIHAT_VOICES; i++) {
        if (_hihats[i].isPlaying()) return true;
    }
    
    return false;
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::enablePerformanceMode(bool enable) {
    _performance_mode = enable;
    if (enable) {
        optimizeForPerformance();
    }
}

TR808_ISR_OPTIMIZED void TR808DrumMachineMozzi::optimizeForPerformance() {
    // Enable performance optimizations
    // This would include cache optimization, memory management, etc.
    #ifdef ESP32C3_RISCV_OPTIMIZATION
    // RISC-V specific optimizations
    #endif
    
    #ifdef USE_DTCM_MEMORY
    // Use DTCM for critical data
    #endif
}

TR808_FASTMATH_INLINE float TR808DrumMachineMozzi::getCPUUsage() const {
    if (_processing_time_us == 0) return 0.0f;
    
    // Calculate CPU usage based on processing time vs available time
    uint32_t available_time_us = 1000000UL / MOZZI_TR808_AUDIO_RATE; // 15.6μs @ 64kHz
    return (float)_processing_time_us / (float)available_time_us * 100.0f;
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setMasterVolume(float volume) {
    // Apply volume to all mix levels
    for (int i = 0; i < 4; i++) {
        _mix_levels[i] = _mix_levels[i] * Q15n16(volume);
    }
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setBitCrushDepth(uint8_t depth) {
    _bitcrusher.setBits(depth);
}

TR808_FASTMATH_INLINE void TR808DrumMachineMozzi::setMasterFilterCutoff(float cutoff_hz) {
    _master_lpf.setCutoff(cutoff_hz);
}