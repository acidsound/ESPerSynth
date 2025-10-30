#ifndef TR808_DRUMS_H
#define TR808_DRUMS_H

#include <stdint.h>
#include <math.h>
#include <Arduino.h>

// ESP32C3 최적화를 위한 상수 정의
#define MAX_SAMPLE_RATE 32768  // ESP32C3 권장 오디오 레이트
#define PI 3.14159265358979323846f
#define TWO_PI 6.28318530717958647692f
#define SAMPLE_TIME_US (1000000 / MAX_SAMPLE_RATE)

/**
 * 기본 Oscillator 클래스 - 사인파, 사각파, 톱니파 생성
 */
class TR808Oscillator {
private:
    float frequency;
    float phase;
    float phaseIncrement;
    float amplitude;
    
public:
    TR808Oscillator();
    void setFrequency(float freq);
    void setAmplitude(float amp);
    void resetPhase();
    void updatePhase();
    
    // 다양한 파형 생성
    float generateSine();
    float generateSquare();
    float generateSaw();
    float generatePinkNoise(); // 필터링된 핑크 노이즈
    float generateWhiteNoise();
};

/**
 * ADSR 엔벨롭 생성기
 */
class TR808Envelope {
private:
    float attackTime;
    float decayTime;
    float releaseTime;
    float sustainLevel;
    float currentLevel;
    bool isActive;
    uint32_t startTime;
    uint32_t attackEndTime;
    uint32_t decayEndTime;
    
public:
    TR808Envelope();
    void setAttack(float timeMs);
    void setDecay(float timeMs);
    void setRelease(float timeMs);
    void setSustain(float level);
    void trigger();
    void release();
    float getValue();
    bool isNoteActive();
};

/**
 * 밴드패스/하이패스/로우패스 필터
 */
class TR808Filter {
private:
    float cutoffFreq;
    float resonance;
    float alpha;
    float beta;
    float gamma;
    float delta;
    
    // 필터 상태
    float x1, x2; // 입력 지연
    float y1, y2; // 출력 지연
    
public:
    TR808Filter();
    void setCutoff(float freq);
    void setResonance(float q);
    float process(float input);
    void reset();
    
    // 다양한 필터 타입
    float processLowPass(float input);
    float processHighPass(float input);
    float processBandPass(float input);
};

/**
 * 사운드 프로세서 - VCA, 포화도 등
 */
class TR808Processor {
private:
    float masterGain;
    float saturatorAmount;
    
public:
    TR808Processor();
    void setGain(float gain);
    void setSaturation(float amount);
    float process(float input);
    float saturate(float input);
};

/**
 * 브리지드 T 발진기 - TR-808의 핵심 기술
 */
class TR808BridgedTOscillator {
private:
    float resonantFreq;
    float damping;
    float phase;
    float amplitude;
    float decayRate;
    
    // 브리지드 T 파라미터
    float r1, r2, c1, c2; // 저항/커패시터 값 (임시 계산)
    
public:
    TR808BridgedTOscillator();
    void setFrequency(float freq);
    void setDecay(float decayMs);
    void trigger();
    float generate();
    void reset();
};

/**
 * TR-808 림샷 전용 비조화 발진기
 */
class TR808InharmonicOscillator {
private:
    float freq1;  // ~1667 Hz
    float freq2;  // ~455 Hz
    float phase1, phase2;
    float mixRatio;
    
public:
    TR808InharmonicOscillator();
    void setFrequencies(float f1, float f2);
    void setMixRatio(float ratio);
    float generate();
    void reset();
};

/**
 * 베이스 드럼 (킥 드럼)
 */
class TR808Kick {
private:
    TR808BridgedTOscillator oscillator;
    TR808Envelope amplitudeEnvelope;
    TR808Envelope pitchEnvelope;
    TR808Filter toneFilter;
    TR808Processor processor;
    float subFrequency;
    bool isPlaying;
    
public:
    TR808Kick();
    void trigger(float velocity = 1.0f);
    float process();
    void setDecay(float decayMs);
    void setTone(float tone); // 0-1
    void setLevel(float level);
    bool isActive();
};

/**
 * 스네어 드럼
 */
class TR808Snare {
private:
    TR808BridgedTOscillator osc1, osc2;
    TR808Oscillator noiseOsc;
    TR808Envelope tonalEnvelope;
    TR808Envelope noiseEnvelope;
    TR808Filter noiseHPF;
    TR808Processor processor;
    bool isPlaying;
    
public:
    TR808Snare();
    void trigger(float velocity = 1.0f);
    float process();
    void setTone(float tone);
    void setSnappy(float snappy);
    void setLevel(float level);
    bool isActive();
};

/**
 * 심벌 (찰국)
 */
class TR808Cymbal {
private:
    TR808Oscillator oscillators[6]; // 6개 오실레이터 뱅크
    TR808Filter bpf1, bpf2; // 듀얼 밴드패스
    TR808Envelope envelope;
    TR808Filter hpf;
    TR808Processor processor;
    
    // 오실레이터 주파수 (TR-808 원본 설정)
    const float oscFreqs[6] = {800.0f, 540.0f, 522.7f, 369.6f, 304.4f, 205.3f};
    
public:
    TR808Cymbal();
    void trigger(float velocity = 1.0f);
    float process();
    void setDecay(float decayMs);
    void setTone(float tone);
    void setLevel(float level);
    bool isActive();
};

/**
 * 하이햇
 */
class TR808HiHat {
private:
    TR808Oscillator oscillators[6];
    TR808Filter bpf;
    TR808Envelope envelope;
    TR808Filter hpf;
    TR808Processor processor;
    bool isOpen; // 클로즈드/오픈 모드
    
public:
    TR808HiHat(bool open = false);
    void trigger(float velocity = 1.0f);
    float process();
    void setOpen(bool open);
    void setDecay(float decayMs);
    void setLevel(float level);
    bool isActive();
};

/**
 * 톰/틸프
 */
class TR808Tom {
private:
    TR808BridgedTOscillator oscillator;
    TR808Oscillator pinkNoiseOsc;
    TR808Envelope tonalEnvelope;
    TR808Envelope noiseEnvelope;
    TR808Filter noiseLPF;
    TR808Processor processor;
    float pitchBendRate;
    bool isPlaying;
    
public:
    TR808Tom();
    void trigger(float velocity = 1.0f);
    float process();
    void setTuning(float freq);
    void setDecay(float decayMs);
    void setLevel(float level);
    bool isActive();
};

/**
 * 콩가
 */
class TR808Conga {
private:
    TR808BridgedTOscillator oscillator;
    TR808Oscillator pinkNoiseOsc;
    TR808Envelope tonalEnvelope;
    TR808Envelope noiseEnvelope;
    TR808Filter noiseLPF;
    TR808Processor processor;
    bool isPlaying;
    
public:
    TR808Conga();
    void trigger(float velocity = 1.0f);
    float process();
    void setTuning(float freq);
    void setDecay(float decayMs);
    void setLevel(float level);
    bool isActive();
};

/**
 * 림샷
 */
class TR808Rimshot {
private:
    TR808InharmonicOscillator oscillator;
    TR808Envelope envelope;
    TR808Filter hpf;
    TR808Processor processor;
    bool noiseGateActive;
    float lastInput;
    
public:
    TR808Rimshot();
    void trigger(float velocity = 1.0f);
    float process();
    void setLevel(float level);
    bool isActive();
};

/**
 * 마라카스
 */
class TR808Maracas {
private:
    TR808Oscillator noiseOsc;
    TR808Envelope envelope;
    TR808Filter hpf;
    TR808Processor processor;
    bool isPlaying;
    
public:
    TR808Maracas();
    void trigger(float velocity = 1.0f);
    float process();
    void setLevel(float level);
    bool isActive();
};

/**
 * 핸드클랩
 */
class TR808Clap {
private:
    TR808Oscillator noiseOsc;
    TR808Filter bpf;
    TR808Envelope sawEnvelope; // 톱니파 엔벨롭 (3개 타격)
    TR808Envelope reverbEnvelope; // 리버브 엔벨롭
    TR808Processor processor;
    uint8_t hitCount;
    uint32_t lastHitTime;
    
public:
    TR808Clap();
    void trigger(float velocity = 1.0f);
    float process();
    void setLevel(float level);
    bool isActive();
};

/**
 * 카우벨
 */
class TR808Cowbell {
private:
    TR808Oscillator osc1, osc2; // 심벌과 공유
    TR808Filter bpf;
    TR808Filter hpf;
    TR808Envelope envelope;
    TR808Processor processor;
    
    // 카우벨 전용 주파수
    const float cowbellFreqs[2] = {800.0f, 540.0f};
    
public:
    TR808Cowbell();
    void trigger(float velocity = 1.0f);
    float process();
    void setLevel(float level);
    bool isActive();
};

/**
 * 메인 TR-808 드럼 머신 클래스
 */
class TR808DrumMachine {
private:
    TR808Kick kick;
    TR808Snare snare;
    TR808Cymbal cymbal;
    TR808HiHat hiHat;
    TR808Tom tom;
    TR808Conga conga;
    TR808Rimshot rimshot;
    TR808Maracas maracas;
    TR808Clap clap;
    TR808Cowbell cowbell;
    
    float masterVolume;
    
public:
    TR808DrumMachine();
    
    // 트리거 함수들
    void triggerKick(float velocity = 1.0f);
    void triggerSnare(float velocity = 1.0f);
    void triggerCymbal(float velocity = 1.0f);
    void triggerHiHat(float velocity = 1.0f, bool open = false);
    void triggerTom(float velocity = 1.0f);
    void triggerConga(float velocity = 1.0f);
    void triggerRimshot(float velocity = 1.0f);
    void triggerMaracas(float velocity = 1.0f);
    void triggerClap(float velocity = 1.0f);
    void triggerCowbell(float velocity = 1.0f);
    
    // 메인 처리 함수
    float process();
    
    // 설정 함수들
    void setMasterVolume(float volume);
    
    // 드럼별 설정
    void setKickDecay(float decayMs);
    void setKickTone(float tone);
    void setSnareTone(float tone);
    void setSnareSnappy(float snappy);
    void setCymbalDecay(float decayMs);
    void setCymbalTone(float tone);
    void setHiHatDecay(float decayMs);
    void setHiHatOpen(bool open);
    void setTomTuning(float freq);
    void setTomDecay(float decayMs);
    void setCongaTuning(float freq);
    void setCongaDecay(float decayMs);
};

#endif // TR808_DRUMS_H