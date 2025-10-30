#include "tr808_drums.h"

// ================ TR808Oscillator 구현 ================

TR808Oscillator::TR808Oscillator() {
    frequency = 440.0f;
    phase = 0.0f;
    phaseIncrement = 0.0f;
    amplitude = 1.0f;
}

void TR808Oscillator::setFrequency(float freq) {
    frequency = freq;
    phaseIncrement = TWO_PI * frequency / MAX_SAMPLE_RATE;
}

void TR808Oscillator::setAmplitude(float amp) {
    amplitude = amp;
}

void TR808Oscillator::resetPhase() {
    phase = 0.0f;
}

void TR808Oscillator::updatePhase() {
    phase += phaseIncrement;
    if (phase >= TWO_PI) {
        phase -= TWO_PI;
    }
}

float TR808Oscillator::generateSine() {
    updatePhase();
    return amplitude * sinf(phase);
}

float TR808Oscillator::generateSquare() {
    updatePhase();
    float value = (phase < PI) ? amplitude : -amplitude;
    return value;
}

float TR808Oscillator::generateSaw() {
    updatePhase();
    float normalizedPhase = phase / TWO_PI;
    return amplitude * (2.0f * normalizedPhase - 1.0f);
}

float TR808Oscillator::generateWhiteNoise() {
    // ESP32C3 최적화된 랜덤 노이즈 생성
    static uint32_t seed = 0x12345678;
    seed = (seed * 1664525 + 1013904223) & 0xFFFFFFFF;
    return amplitude * ((seed & 0xFFFF) / 32768.0f - 1.0f);
}

float TR808Oscillator::generatePinkNoise() {
    // 간단한 1차 필터를 통한 핑크 노이즈
    static float lastOutput = 0.0f;
    float white = generateWhiteNoise();
    lastOutput = 0.98f * lastOutput + 0.02f * white;
    return amplitude * lastOutput;
}

// ================ TR808Envelope 구현 ================

TR808Envelope::TR808Envelope() {
    attackTime = 1.0f;
    decayTime = 100.0f;
    releaseTime = 100.0f;
    sustainLevel = 0.7f;
    currentLevel = 0.0f;
    isActive = false;
    startTime = 0;
    attackEndTime = 0;
    decayEndTime = 0;
}

void TR808Envelope::setAttack(float timeMs) {
    attackTime = timeMs;
}

void TR808Envelope::setDecay(float timeMs) {
    decayTime = timeMs;
}

void TR808Envelope::setRelease(float timeMs) {
    releaseTime = timeMs;
}

void TR808Envelope::setSustain(float level) {
    sustainLevel = level;
}

void TR808Envelope::trigger() {
    isActive = true;
    startTime = micros();
    attackEndTime = startTime + (uint32_t)(attackTime * 1000);
    decayEndTime = attackEndTime + (uint32_t)(decayTime * 1000);
}

void TR808Envelope::release() {
    // 릴리즈는 현재 레벨에서 시작
}

float TR808Envelope::getValue() {
    if (!isActive) return 0.0f;
    
    uint32_t currentTime = micros();
    
    if (currentTime < attackEndTime) {
        // 어택 단계
        float progress = (float)(currentTime - startTime) / (attackTime * 1000.0f);
        if (progress > 1.0f) progress = 1.0f;
        currentLevel = progress;
    } else if (currentTime < decayEndTime) {
        // 디케이 단계
        float progress = (float)(currentTime - attackEndTime) / (decayTime * 1000.0f);
        if (progress > 1.0f) progress = 1.0f;
        currentLevel = 1.0f - (1.0f - sustainLevel) * progress;
    } else {
        // 서스테인 단계
        currentLevel = sustainLevel;
    }
    
    return currentLevel;
}

bool TR808Envelope::isNoteActive() {
    return isActive && currentLevel > 0.001f;
}

// ================ TR808Filter 구현 ================

TR808Filter::TR808Filter() {
    cutoffFreq = 1000.0f;
    resonance = 1.0f;
    alpha = 0.0f;
    beta = 0.0f;
    gamma = 0.0f;
    delta = 0.0f;
    x1 = x2 = y1 = y2 = 0.0f;
}

void TR808Filter::setCutoff(float freq) {
    cutoffFreq = freq;
    // 간단한 1차 필터 계산
    float omega = TWO_PI * cutoffFreq / MAX_SAMPLE_RATE;
    alpha = omega / (omega + 1.0f);
}

void TR808Filter::setResonance(float q) {
    resonance = q;
}

float TR808Filter::processLowPass(float input) {
    float output = alpha * input + (1.0f - alpha) * y1;
    y1 = output;
    return output;
}

float TR808Filter::processHighPass(float input) {
    float output = alpha * (input - x1 + y1);
    x1 = input;
    y1 = output;
    return output;
}

float TR808Filter::processBandPass(float input) {
    // 2차 밴드패스 구현
    float output = alpha * (input - gamma * y1 - delta * y2);
    y2 = y1;
    y1 = output;
    return output;
}

void TR808Filter::reset() {
    x1 = x2 = y1 = y2 = 0.0f;
}

// ================ TR808Processor 구현 ================

TR808Processor::TR808Processor() {
    masterGain = 1.0f;
    saturatorAmount = 0.0f;
}

void TR808Processor::setGain(float gain) {
    masterGain = gain;
}

void TR808Processor::setSaturation(float amount) {
    saturatorAmount = amount;
}

float TR808Processor::process(float input) {
    float processed = saturate(input);
    return processed * masterGain;
}

float TR808Processor::saturate(float input) {
    if (saturatorAmount <= 0.0f) return input;
    
    // 간단한 소프트 클리핑
    float x = input * saturatorAmount;
    float output = tanhf(x) / saturatorAmount;
    return output;
}

// ================ TR808BridgedTOscillator 구현 ================

TR808BridgedTOscillator::TR808BridgedTOscillator() {
    resonantFreq = 60.0f;
    damping = 0.1f;
    phase = 0.0f;
    amplitude = 1.0f;
    decayRate = 0.1f;
}

void TR808BridgedTOscillator::setFrequency(float freq) {
    resonantFreq = freq;
}

void TR808BridgedTOscillator::setDecay(float decayMs) {
    decayRate = 1000.0f / decayMs / MAX_SAMPLE_RATE;
}

void TR808BridgedTOscillator::trigger() {
    amplitude = 1.0f;
    phase = 0.0f;
}

float TR808BridgedTOscillator::generate() {
    // 브리지드 T 발진기 시뮬레이션
    // 실제 TR-808의 브리지드 T 회로는 매우 복잡하므로 근사치로 구현
    float frequency = resonantFreq * (1.0f - 0.1f * amplitude);
    float sample = amplitude * sinf(phase);
    
    phase += TWO_PI * frequency / MAX_SAMPLE_RATE;
    if (phase >= TWO_PI) {
        phase -= TWO_PI;
    }
    
    amplitude *= (1.0f - decayRate);
    if (amplitude < 0.001f) amplitude = 0.0f;
    
    return sample;
}

void TR808BridgedTOscillator::reset() {
    phase = 0.0f;
    amplitude = 0.0f;
}

// ================ TR808InharmonicOscillator 구현 ================

TR808InharmonicOscillator::TR808InharmonicOscillator() {
    freq1 = 1667.0f; // ~G#6+6¢
    freq2 = 455.0f;  // ~A#4-42¢
    phase1 = phase2 = 0.0f;
    mixRatio = 0.5f;
}

void TR808InharmonicOscillator::setFrequencies(float f1, float f2) {
    freq1 = f1;
    freq2 = f2;
}

void TR808InharmonicOscillator::setMixRatio(float ratio) {
    mixRatio = ratio;
}

float TR808InharmonicOscillator::generate() {
    float sample1 = sinf(phase1);
    float sample2 = sinf(phase2);
    
    phase1 += TWO_PI * freq1 / MAX_SAMPLE_RATE;
    phase2 += TWO_PI * freq2 / MAX_SAMPLE_RATE;
    
    if (phase1 >= TWO_PI) phase1 -= TWO_PI;
    if (phase2 >= TWO_PI) phase2 -= TWO_PI;
    
    return mixRatio * sample1 + (1.0f - mixRatio) * sample2;
}

void TR808InharmonicOscillator::reset() {
    phase1 = phase2 = 0.0f;
}

// ================ TR808Kick 구현 ================

TR808Kick::TR808Kick() {
    subFrequency = 50.0f;
    isPlaying = false;
    
    oscillator.setFrequency(60.0f);
    amplitudeEnvelope.setAttack(1.0f);
    amplitudeEnvelope.setDecay(500.0f);
    amplitudeEnvelope.setSustain(0.0f);
    
    pitchEnvelope.setAttack(0.5f);
    pitchEnvelope.setDecay(30.0f);
    pitchEnvelope.setSustain(0.0f);
    
    toneFilter.setCutoff(200.0f);
    processor.setGain(0.8f);
}

void TR808Kick::trigger(float velocity) {
    oscillator.trigger();
    amplitudeEnvelope.trigger();
    pitchEnvelope.trigger();
    isPlaying = true;
    
    // 서브 프퀀시 보강
    oscillator.setFrequency(60.0f + 20.0f * velocity);
}

float TR808Kick::process() {
    if (!isPlaying) return 0.0f;
    
    float pitchMod = pitchEnvelope.getValue();
    float freq = 60.0f * (1.0f - 0.5f * pitchMod);
    oscillator.setFrequency(freq);
    
    float tonal = oscillator.generate();
    float envelope = amplitudeEnvelope.getValue();
    
    float output = tonal * envelope;
    output = toneFilter.processLowPass(output);
    output = processor.process(output);
    
    // 서브 바디 보강
    static TR808Oscillator subOsc;
    subOsc.setFrequency(subFrequency);
    float sub = subOsc.generateSine() * envelope * 0.3f;
    output += sub;
    
    if (envelope <= 0.001f) {
        isPlaying = false;
    }
    
    return output;
}

void TR808Kick::setDecay(float decayMs) {
    amplitudeEnvelope.setDecay(decayMs);
}

void TR808Kick::setTone(float tone) {
    toneFilter.setCutoff(100.0f + tone * 300.0f);
}

void TR808Kick::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Kick::isActive() {
    return isPlaying;
}

// ================ TR808Snare 구현 ================

TR808Snare::TR808Snare() {
    isPlaying = false;
    
    // 두 개의 브리지드 T 발진기 설정
    osc1.setFrequency(200.0f);
    osc2.setFrequency(180.0f);
    
    // 노이즈 설정
    noiseOsc.setAmplitude(0.7f);
    
    // 엔벨롭 설정
    tonalEnvelope.setAttack(0.1f);
    tonalEnvelope.setDecay(50.0f);
    tonalEnvelope.setSustain(0.0f);
    
    noiseEnvelope.setAttack(0.1f);
    noiseEnvelope.setDecay(25.0f); // "Snappy"
    noiseEnvelope.setSustain(0.0f);
    
    // 필터 설정
    noiseHPF.setCutoff(1000.0f);
    noiseHPF.setResonance(1.0f);
    
    processor.setGain(0.6f);
}

void TR808Snare::trigger(float velocity) {
    osc1.trigger();
    osc2.trigger();
    tonalEnvelope.trigger();
    noiseEnvelope.trigger();
    isPlaying = true;
}

float TR808Snare::process() {
    if (!isPlaying) return 0.0f;
    
    float tonal1 = osc1.generate();
    float tonal2 = osc2.generate();
    float tonal = (tonal1 + tonal2) * 0.5f * tonalEnvelope.getValue();
    
    float noise = noiseOsc.generateWhiteNoise();
    noise = noiseHPF.processHighPass(noise);
    noise *= noiseEnvelope.getValue();
    
    float output = tonal + noise;
    output = processor.process(output);
    
    if (tonalEnvelope.getValue() <= 0.001f && noiseEnvelope.getValue() <= 0.001f) {
        isPlaying = false;
    }
    
    return output;
}

void TR808Snare::setTone(float tone) {
    osc1.setFrequency(180.0f + tone * 40.0f);
    osc2.setFrequency(160.0f + tone * 40.0f);
}

void TR808Snare::setSnappy(float snappy) {
    noiseEnvelope.setDecay(10.0f + snappy * 50.0f);
}

void TR808Snare::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Snare::isActive() {
    return isPlaying;
}

// ================ TR808Cymbal 구현 ================

TR808Cymbal::TR808Cymbal() {
    // 6개 오실레이터 초기화
    for (int i = 0; i < 6; i++) {
        oscillators[i].setFrequency(oscFreqs[i]);
        oscillators[i].setAmplitude(0.3f);
    }
    
    // 듀얼 밴드패스 필터
    bpf1.setCutoff(7100.0f); // ~7.1 kHz
    bpf2.setCutoff(3440.0f); // ~3.44 kHz
    
    // 엔벨롭
    envelope.setAttack(1.0f);
    envelope.setDecay(800.0f);
    envelope.setSustain(0.0f);
    
    // 하이패스
    hpf.setCutoff(2000.0f);
    
    processor.setGain(0.5f);
}

void TR808Cymbal::trigger(float velocity) {
    envelope.trigger();
}

float TR808Cymbal::process() {
    float envelope = this->envelope.getValue();
    if (envelope <= 0.001f) return 0.0f;
    
    // 6개 오실레이터 믹싱
    float mixed = 0.0f;
    for (int i = 0; i < 6; i++) {
        mixed += oscillators[i].generateSquare();
    }
    mixed /= 6.0f;
    
    // 듀얼 밴드패스 처리
    float bpf1_out = bpf1.processBandPass(mixed);
    float bpf2_out = bpf2.processBandPass(mixed);
    float filtered = 0.7f * bpf1_out + 0.3f * bpf2_out;
    
    filtered = hpf.processHighPass(filtered);
    filtered *= envelope;
    filtered = processor.process(filtered);
    
    return filtered;
}

void TR808Cymbal::setDecay(float decayMs) {
    envelope.setDecay(decayMs);
}

void TR808Cymbal::setTone(float tone) {
    float cutoff1 = 5000.0f + tone * 4000.0f;
    float cutoff2 = 2500.0f + tone * 2000.0f;
    bpf1.setCutoff(cutoff1);
    bpf2.setCutoff(cutoff2);
}

void TR808Cymbal::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Cymbal::isActive() {
    return envelope.getValue() > 0.001f;
}

// ================ TR808HiHat 구현 ================

TR808HiHat::TR808HiHat(bool open) {
    isOpen = open;
    
    // 6개 오실레이터 초기화 (심벌과 동일)
    for (int i = 0; i < 6; i++) {
        oscillators[i].setFrequency(oscFreqs[i]);
        oscillators[i].setAmplitude(0.2f);
    }
    
    // 밴드패스 필터
    bpf.setCutoff(8000.0f);
    
    // 엔벨롭
    if (open) {
        envelope.setDecay(200.0f);
    } else {
        envelope.setDecay(50.0f); // 클로즈드
    }
    envelope.setAttack(0.5f);
    envelope.setSustain(0.0f);
    
    // 하이패스
    hpf.setCutoff(3000.0f);
    
    processor.setGain(0.4f);
}

void TR808HiHat::trigger(float velocity) {
    envelope.trigger();
}

float TR808HiHat::process() {
    float envelope = this->envelope.getValue();
    if (envelope <= 0.001f) return 0.0f;
    
    // 6개 오실레이터 믹싱
    float mixed = 0.0f;
    for (int i = 0; i < 6; i++) {
        mixed += oscillators[i].generateSquare();
    }
    mixed /= 6.0f;
    
    float filtered = bpf.processBandPass(mixed);
    filtered = hpf.processHighPass(filtered);
    filtered *= envelope;
    filtered = processor.process(filtered);
    
    return filtered;
}

void TR808HiHat::setOpen(bool open) {
    isOpen = open;
    if (open) {
        envelope.setDecay(200.0f);
    } else {
        envelope.setDecay(50.0f);
    }
}

void TR808HiHat::setDecay(float decayMs) {
    envelope.setDecay(decayMs);
}

void TR808HiHat::setLevel(float level) {
    processor.setGain(level);
}

bool TR808HiHat::isActive() {
    return envelope.getValue() > 0.001f;
}

// ================ TR808Tom 구현 ================

TR808Tom::TR808Tom() {
    isPlaying = false;
    pitchBendRate = 0.95f;
    
    oscillator.setFrequency(165.0f); // High Tom
    
    // 톤 엔벨롭
    tonalEnvelope.setAttack(0.5f);
    tonalEnvelope.setDecay(100.0f);
    tonalEnvelope.setSustain(0.0f);
    
    // 핑크 노이즈 엔벨롭 (가짜 잔향)
    noiseEnvelope.setAttack(1.0f);
    noiseEnvelope.setDecay(200.0f);
    noiseEnvelope.setSustain(0.0f);
    
    // 핑크 노이즈 설정
    pinkNoiseOsc.setAmplitude(0.1f);
    noiseLPF.setCutoff(500.0f);
    
    processor.setGain(0.7f);
}

void TR808Tom::trigger(float velocity) {
    oscillator.trigger();
    tonalEnvelope.trigger();
    noiseEnvelope.trigger();
    isPlaying = true;
}

float TR808Tom::process() {
    if (!isPlaying) return 0.0f;
    
    // 피치 벤드 효과 (하향)
    static float currentFreq = 165.0f;
    currentFreq *= pitchBendRate;
    oscillator.setFrequency(currentFreq);
    
    float tonal = oscillator.generate() * tonalEnvelope.getValue();
    float noise = pinkNoiseOsc.generatePinkNoise();
    noise = noiseLPF.processLowPass(noise);
    noise *= noiseEnvelope.getValue() * 0.3f;
    
    float output = tonal + noise;
    output = processor.process(output);
    
    // 빈도 복구
    currentFreq = 165.0f;
    
    if (tonalEnvelope.getValue() <= 0.001f) {
        isPlaying = false;
    }
    
    return output;
}

void TR808Tom::setTuning(float freq) {
    oscillator.setFrequency(freq);
}

void TR808Tom::setDecay(float decayMs) {
    tonalEnvelope.setDecay(decayMs);
}

void TR808Tom::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Tom::isActive() {
    return isPlaying;
}

// ================ TR808Conga 구현 ================

TR808Conga::TR808Conga() {
    isPlaying = false;
    
    oscillator.setFrequency(370.0f); // High Conga
    
    tonalEnvelope.setAttack(0.5f);
    tonalEnvelope.setDecay(80.0f);
    tonalEnvelope.setSustain(0.0f);
    
    noiseEnvelope.setAttack(1.0f);
    noiseEnvelope.setDecay(180.0f);
    noiseEnvelope.setSustain(0.0f);
    
    pinkNoiseOsc.setAmplitude(0.1f);
    noiseLPF.setCutoff(600.0f);
    
    processor.setGain(0.7f);
}

void TR808Conga::trigger(float velocity) {
    oscillator.trigger();
    tonalEnvelope.trigger();
    noiseEnvelope.trigger();
    isPlaying = true;
}

float TR808Conga::process() {
    if (!isPlaying) return 0.0f;
    
    float tonal = oscillator.generate() * tonalEnvelope.getValue();
    float noise = pinkNoiseOsc.generatePinkNoise();
    noise = noiseLPF.processLowPass(noise);
    noise *= noiseEnvelope.getValue() * 0.3f;
    
    float output = tonal + noise;
    output = processor.process(output);
    
    if (tonalEnvelope.getValue() <= 0.001f) {
        isPlaying = false;
    }
    
    return output;
}

void TR808Conga::setTuning(float freq) {
    oscillator.setFrequency(freq);
}

void TR808Conga::setDecay(float decayMs) {
    tonalEnvelope.setDecay(decayMs);
}

void TR808Conga::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Conga::isActive() {
    return isPlaying;
}

// ================ TR808Rimshot 구현 ================

TR808Rimshot::TR808Rimshot() {
    noiseGateActive = false;
    lastInput = 0.0f;
    
    // 림샷 전용 주파수 설정
    oscillator.setFrequencies(1667.0f, 455.0f);
    
    envelope.setAttack(1.0f);
    envelope.setDecay(10.0f); // 극단적 스냅
    envelope.setSustain(0.0f);
    
    hpf.setCutoff(800.0f);
    processor.setGain(0.8f);
}

void TR808Rimshot::trigger(float velocity) {
    envelope.trigger();
    oscillator.reset();
}

float TR808Rimshot::process() {
    float envelope = this->envelope.getValue();
    if (envelope <= 0.001f) return 0.0f;
    
    float tonal = oscillator.generate();
    float filtered = hpf.processHighPass(tonal);
    
    // 노이즈 게이트 시뮬레이션
    if (abs(filtered) < 0.01f) {
        noiseGateActive = true;
    } else {
        noiseGateActive = false;
    }
    
    float output = filtered * envelope;
    output = processor.process(output);
    
    return output;
}

void TR808Rimshot::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Rimshot::isActive() {
    return envelope.getValue() > 0.001f;
}

// ================ TR808Maracas 구현 ================

TR808Maracas::TR808Maracas() {
    isPlaying = false;
    
    noiseOsc.setAmplitude(0.5f);
    
    envelope.setAttack(0.5f);
    envelope.setDecay(30.0f); // AR 엔벨롭
    envelope.setSustain(0.0f);
    
    hpf.setCutoff(1500.0f);
    processor.setGain(0.3f);
}

void TR808Maracas::trigger(float velocity) {
    envelope.trigger();
    isPlaying = true;
}

float TR808Maracas::process() {
    if (!isPlaying) return 0.0f;
    
    float noise = noiseOsc.generateWhiteNoise();
    noise = hpf.processHighPass(noise);
    noise *= envelope.getValue();
    
    float output = processor.process(noise);
    
    if (envelope.getValue() <= 0.001f) {
        isPlaying = false;
    }
    
    return output;
}

void TR808Maracas::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Maracas::isActive() {
    return isPlaying;
}

// ================ TR808Clap 구현 ================

TR808Clap::TR808Clap() {
    hitCount = 0;
    lastHitTime = 0;
    
    noiseOsc.setAmplitude(0.8f);
    
    bpf.setCutoff(1000.0f); // ~1 kHz
    
    // 톱니파 엔벨롭 (3개 타격)
    sawEnvelope.setAttack(1.0f);
    sawEnvelope.setDecay(10.0f);
    sawEnvelope.setSustain(0.0f);
    
    // 리버브 엔벨롭
    reverbEnvelope.setAttack(5.0f);
    reverbEnvelope.setDecay(100.0f);
    reverbEnvelope.setSustain(0.0f);
    
    processor.setGain(0.6f);
}

void TR808Clap::trigger(float velocity) {
    // 3개 타격 생성
    for (int i = 0; i < 3; i++) {
        sawEnvelope.trigger();
        delayMicroseconds(15000); // 15ms 간격
    }
    reverbEnvelope.trigger();
}

float TR808Clap::process() {
    float noise = noiseOsc.generateWhiteNoise();
    noise = bpf.processBandPass(noise);
    
    float saw = sawEnvelope.getValue();
    float reverb = reverbEnvelope.getValue();
    
    float output = noise * (saw + reverb * 0.5f);
    output = processor.process(output);
    
    return output;
}

void TR808Clap::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Clap::isActive() {
    return (sawEnvelope.getValue() > 0.001f || reverbEnvelope.getValue() > 0.001f);
}

// ================ TR808Cowbell 구현 ================

TR808Cowbell::TR808Cowbell() {
    osc1.setFrequency(cowbellFreqs[0]); // 800 Hz
    osc2.setFrequency(cowbellFreqs[1]); // 540 Hz
    
    bpf.setCutoff(2000.0f);
    hpf.setCutoff(500.0f);
    
    envelope.setAttack(0.5f);
    envelope.setDecay(80.0f);
    envelope.setSustain(0.0f);
    
    processor.setGain(0.5f);
}

void TR808Cowbell::trigger(float velocity) {
    envelope.trigger();
}

float TR808Cowbell::process() {
    float envelope = this->envelope.getValue();
    if (envelope <= 0.001f) return 0.0f;
    
    float osc1_out = osc1.generateSquare();
    float osc2_out = osc2.generateSquare();
    float mixed = 0.6f * osc1_out + 0.4f * osc2_out;
    
    mixed = bpf.processBandPass(mixed);
    mixed = hpf.processHighPass(mixed);
    mixed *= envelope;
    mixed = processor.process(mixed);
    
    return mixed;
}

void TR808Cowbell::setLevel(float level) {
    processor.setGain(level);
}

bool TR808Cowbell::isActive() {
    return envelope.getValue() > 0.001f;
}

// ================ TR808DrumMachine 구현 ================

TR808DrumMachine::TR808DrumMachine() {
    masterVolume = 0.8f;
}

void TR808DrumMachine::triggerKick(float velocity) {
    kick.trigger(velocity);
}

void TR808DrumMachine::triggerSnare(float velocity) {
    snare.trigger(velocity);
}

void TR808DrumMachine::triggerCymbal(float velocity) {
    cymbal.trigger(velocity);
}

void TR808DrumMachine::triggerHiHat(float velocity, bool open) {
    hiHat.setOpen(open);
    hiHat.trigger(velocity);
}

void TR808DrumMachine::triggerTom(float velocity) {
    tom.trigger(velocity);
}

void TR808DrumMachine::triggerConga(float velocity) {
    conga.trigger(velocity);
}

void TR808DrumMachine::triggerRimshot(float velocity) {
    rimshot.trigger(velocity);
}

void TR808DrumMachine::triggerMaracas(float velocity) {
    maracas.trigger(velocity);
}

void TR808DrumMachine::triggerClap(float velocity) {
    clap.trigger(velocity);
}

void TR808DrumMachine::triggerCowbell(float velocity) {
    cowbell.trigger(velocity);
}

float TR808DrumMachine::process() {
    float output = 0.0f;
    
    output += kick.process();
    output += snare.process();
    output += cymbal.process();
    output += hiHat.process();
    output += tom.process();
    output += conga.process();
    output += rimshot.process();
    output += maracas.process();
    output += clap.process();
    output += cowbell.process();
    
    // 마스터 볼륨 적용 및 클리핑 방지
    output *= masterVolume;
    if (output > 1.0f) output = 1.0f;
    if (output < -1.0f) output = -1.0f;
    
    return output;
}

void TR808DrumMachine::setMasterVolume(float volume) {
    masterVolume = volume;
}

// 드럼별 설정 함수들
void TR808DrumMachine::setKickDecay(float decayMs) {
    kick.setDecay(decayMs);
}

void TR808DrumMachine::setKickTone(float tone) {
    kick.setTone(tone);
}

void TR808DrumMachine::setSnareTone(float tone) {
    snare.setTone(tone);
}

void TR808DrumMachine::setSnareSnappy(float snappy) {
    snare.setSnappy(snappy);
}

void TR808DrumMachine::setCymbalDecay(float decayMs) {
    cymbal.setDecay(decayMs);
}

void TR808DrumMachine::setCymbalTone(float tone) {
    cymbal.setTone(tone);
}

void TR808DrumMachine::setHiHatDecay(float decayMs) {
    hiHat.setDecay(decayMs);
}

void TR808DrumMachine::setHiHatOpen(bool open) {
    hiHat.setOpen(open);
}

void TR808DrumMachine::setTomTuning(float freq) {
    tom.setTuning(freq);
}

void TR808DrumMachine::setTomDecay(float decayMs) {
    tom.setDecay(decayMs);
}

void TR808DrumMachine::setCongaTuning(float freq) {
    conga.setTuning(freq);
}

void TR808DrumMachine::setCongaDecay(float decayMs) {
    conga.setDecay(decayMs);
}