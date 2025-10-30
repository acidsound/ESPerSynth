#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

#include <Arduino.h>
#include <MIDI.h>

// TR-808 드럼 머신 설정 상수
#define NUM_DRUMS 16          // 총 드럼 소스 수
#define NUM_PATTERNS 16       // 저장 가능한 패턴 수
#define MAX_STEP 16           // 16분음표 기준 패턴 길이
#define MAX_ACCENT_LEVEL 127  // 최대 악센트 레벨

// 드럼 소스 타입 정의
enum DrumSource {
    KICK = 0,
    SNARE,
    CYMBAL,
    TILT,
    OPEN_KICK,
    OPEN_SNARE,
    REAR,
    // 리어스 드럼들
    RIMSHOT,
    MARACAS,
    CLAP,
    COWBELL,
    CLAVE,
    HIGH_TOM,
    MID_TOM,
    LOW_TOM,
    HIGH_CONGA,
    MID_CONGA,
    LOW_CONGA
};

// 패턴 구조체
struct Pattern {
    uint16_t step[NUM_DRUMS][MAX_STEP]; // 각 드럼의 16단계 패턴 (0=빈 단계, 1-127=강도)
    uint8_t accent_step[MAX_STEP];       // 악센트 단계
    uint8_t name[16];                    // 패턴 이름
    uint8_t length;                      // 패턴 길이 (1-16)
    uint8_t swing;                       // 스윙 (0-127)
    uint8_t tempo;                       // 템포 (60-200 BPM)
    bool active;
};

// 믹서 채널 구조체
struct MixerChannel {
    uint8_t volume;        // 메인 볼륨 (0-127)
    uint8_t individual_vol[NUM_DRUMS]; // 드럼별 개별 볼륨
    bool mute[NUM_DRUMS];  // 음소거 상태
    bool solo[NUM_DRUMS];  // 솔로 상태
    uint8_t pan;           // 패닝 (-64~+63)
    uint8_t reverb;        // 리버브 레벨 (0-127)
};

// 미러 설정 구조체
struct MirrorSettings {
    bool enabled;              // 미러 기능 활성화
    uint8_t mirror_start;      // 미러 시작 단계 (0-15)
    uint8_t mirror_length;     // 미러 길이 (1-16)
    bool mirror_reverse;       // 역방향 미러
    uint8_t mirror_volume;     // 미러 볼륨 스케일 (0-127)
    uint8_t mirror_offset;     // 드럼별 미러 오프셋
};

// 시리얼 통신 명령어 타입
enum SerialCommand {
    CMD_NONE = 0,
    CMD_PATTERN_LOAD,
    CMD_PATTERN_SAVE,
    CMD_PATTERN_PLAY,
    CMD_PATTERN_STOP,
    CMD_PATTERN_CLEAR,
    CMD_TEMPO_SET,
    CMD_SWING_SET,
    CMD_LENGTH_SET,
    CMD_DRUM_TRIGGER,
    CMD_DRUM_SET,
    CMD_MIXER_SET,
    CMD_MIRROR_SET,
    CMD_STATUS_REQUEST,
    CMD_HELP_REQUEST
};

// MIDI 이벤트 구조체
struct MidiEvent {
    uint8_t type;      // MIDI 메시지 타입
    uint8_t note;      // 노트 번호
    uint8_t velocity;  // 강도 (0-127)
    uint8_t channel;   // MIDI 채널 (0-15)
};

// 시리얼 패킷 구조체
struct SerialPacket {
    uint8_t command;           // 명령어
    uint8_t drum_id;           // 드럼 ID
    uint8_t step;              // 단계 (0-15)
    uint8_t value;             // 값 (0-127)
    uint8_t data[16];          // 추가 데이터
    uint8_t checksum;          // 체크섬
};

// 사용자 인터페이스 클래스
class TR808UserInterface {
public:
    // 생성자 및 초기화
    TR808UserInterface();
    bool begin(uint32_t baud_rate = 115200);
    
    // 메인 업데이트 함수
    void update();
    
    // 패턴 제어
    bool loadPattern(uint8_t pattern_num);
    bool savePattern(uint8_t pattern_num);
    void playPattern(uint8_t pattern_num = 255);
    void stopPattern();
    void clearPattern(uint8_t pattern_num = 255);
    void applyMirrorSettings();
    
    // 드럼 제어
    void triggerDrum(DrumSource drum, uint8_t velocity = 127);
    void setDrumStep(DrumSource drum, uint8_t step, uint16_t velocity);
    uint16_t getDrumStep(DrumSource drum, uint8_t step);
    
    // 믹서 제어
    void setMasterVolume(uint8_t volume);
    uint8_t getMasterVolume();
    void setIndividualVolume(DrumSource drum, uint8_t volume);
    uint8_t getIndividualVolume(DrumSource drum);
    void setMute(DrumSource drum, bool mute);
    bool getMute(DrumSource drum);
    void setSolo(DrumSource drum, bool solo);
    bool getSolo(DrumSource drum);
    
    // 미러 설정
    void setMirrorEnabled(bool enabled);
    void setMirrorStart(uint8_t start_step);
    void setMirrorLength(uint8_t length);
    void setMirrorReverse(bool reverse);
    void setMirrorVolume(uint8_t volume);
    void setMirrorOffset(uint8_t offset);
    bool getMirrorEnabled();
    uint8_t getMirrorStart();
    uint8_t getMirrorLength();
    bool getMirrorReverse();
    uint8_t getMirrorVolume();
    uint8_t getMirrorOffset();
    
    // 템포 및 패턴 설정
    void setTempo(uint8_t bpm);
    uint8_t getTempo();
    void setSwing(uint8_t swing);
    uint8_t getSwing();
    void setPatternLength(uint8_t length);
    uint8_t getPatternLength();
    
    // 시리얼 통신
    bool parseSerialCommand();
    void sendStatus();
    void sendHelp();
    
    // MIDI 처리
    void handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    void handleMidiControlChange(uint8_t channel, uint8_t controller, uint8_t value);
    
    // 현재 상태 조회
    uint8_t getCurrentPattern();
    bool isPlaying();
    bool isPaused();
    
private:
    // 상태 변수
    Pattern patterns[NUM_PATTERNS];
    MixerChannel mixer;
    MirrorSettings mirror;
    uint8_t current_pattern;
    uint8_t current_step;
    uint8_t play_step;
    bool is_playing;
    bool is_paused;
    uint32_t last_step_time;
    uint32_t step_interval;
    
    // 시리얼 통신 변수
    SerialPacket serial_buffer;
    uint8_t serial_buffer_pos;
    uint32_t last_serial_time;
    
    // MIDI 관련
    MidiEvent midi_buffer[16];
    uint8_t midi_buffer_pos;
    uint32_t last_midi_time;
    
    // 내부 함수들
    void initializePatterns();
    void initializeMixer();
    void initializeMirror();
    void processSerialPacket();
    void processMidiEvent();
    void updateCurrentStep();
    void applyMirrorSettings();
    uint8_t calculateChecksum();
    bool verifyChecksum();
    void sendSerialPacket(const SerialPacket& packet);
    void printPattern(uint8_t pattern_num);
    void printMixerStatus();
    void printMirrorStatus();
    void printDrumMap();
    
    // 패턴 복사 및 비교
    void copyPattern(const Pattern& src, Pattern& dst);
    bool comparePatterns(const Pattern& p1, const Pattern& p2);
    
    // 유틸리티 함수
    uint8_t clampValue(uint8_t value, uint8_t min = 0, uint8_t max = 127);
    uint8_t mapNoteToDrum(uint8_t note);
    uint8_t mapDrumToNote(DrumSource drum);
    uint32_t calculateStepInterval(uint8_t bpm);
};

// 전역 MIDI 객체
extern MIDIClass MIDI;

// 드럼 이름 배열 (전역)
extern const char* drum_names[];

// 유틸리티 함수
uint8_t clampValue(uint8_t value, uint8_t min = 0, uint8_t max = 127);
uint32_t calculateStepInterval(uint8_t bpm);

#endif // USER_INTERFACE_H