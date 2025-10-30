#include "user_interface.h"

// 전역 MIDI 객체 생성
MIDIClass MIDI;

// TR-808 사용자 인터페이스 클래스 구현

TR808UserInterface::TR808UserInterface() {
    // 현재 패턴 인덱스 초기화
    current_pattern = 0;
    current_step = 0;
    play_step = 0;
    is_playing = false;
    is_paused = false;
    last_step_time = 0;
    step_interval = 0;
    
    // 시리얼 통신 버퍼 초기화
    serial_buffer_pos = 0;
    last_serial_time = 0;
    
    // MIDI 버퍼 초기화
    midi_buffer_pos = 0;
    last_midi_time = 0;
}

bool TR808UserInterface::begin(uint32_t baud_rate) {
    // 시리얼 통신 초기화
    Serial.begin(baud_rate);
    Serial.println("TR-808 사용자 인터페이스 시작됨");
    
    // MIDI 초기화
    MIDI.begin(1); // MIDI 채널 1 사용
    
    // 패턴, 믹서, 미러 초기화
    initializePatterns();
    initializeMixer();
    initializeMirror();
    
    // 초기 상태 출력
    printDrumMap();
    sendHelp();
    sendStatus();
    
    return true;
}

void TR808UserInterface::initializePatterns() {
    // 모든 패턴 초기화
    for (int p = 0; p < NUM_PATTERNS; p++) {
        // 패턴 이름 초기화
        sprintf((char*)patterns[p].name, "PATTERN_%d", p + 1);
        patterns[p].length = 16;
        patterns[p].swing = 0;
        patterns[p].tempo = 120;
        patterns[p].active = false;
        
        // 모든 단계 초기화
        for (int d = 0; d < NUM_DRUMS; d++) {
            for (int s = 0; s < MAX_STEP; s++) {
                patterns[p].step[d][s] = 0;
            }
        }
        
        // 악센트 초기화
        for (int s = 0; s < MAX_STEP; s++) {
            patterns[p].accent_step[s] = 127;
        }
    }
    
    // 기본 킥 패턴 생성
    patterns[0].active = true;
    patterns[0].tempo = 120;
    patterns[0].step[KICK][0] = 127;    // 1박
    patterns[0].step[KICK][4] = 80;     // 2박
    patterns[0].step[KICK][8] = 100;    // 3박
    patterns[0].step[KICK][12] = 85;    // 4박
    
    // 기본 스네어 패턴 생성
    patterns[0].step[SNARE][4] = 127;   // 2박
    patterns[0].step[SNARE][12] = 127;  // 4박
    
    // 기본 하이햇 패턴 생성
    for (int s = 0; s < 16; s += 2) {
        patterns[0].step[TILT][s] = 60;
    }
}

void TR808UserInterface::initializeMixer() {
    // 마스터 볼륨 초기화
    mixer.volume = 100;
    
    // 각 드럼별 개별 볼륨 초기화
    mixer.individual_vol[KICK] = 120;
    mixer.individual_vol[SNARE] = 110;
    mixer.individual_vol[CYMBAL] = 80;
    mixer.individual_vol[TILT] = 70;
    mixer.individual_vol[OPEN_KICK] = 100;
    mixer.individual_vol[OPEN_SNARE] = 90;
    mixer.individual_vol[REAR] = 85;
    
    // 리어스 드럼들 초기화
    mixer.individual_vol[RIMSHOT] = 95;
    mixer.individual_vol[MARACAS] = 60;
    mixer.individual_vol[CLAP] = 85;
    mixer.individual_vol[COWBELL] = 75;
    mixer.individual_vol[CLAVE] = 65;
    mixer.individual_vol[HIGH_TOM] = 90;
    mixer.individual_vol[MID_TOM] = 85;
    mixer.individual_vol[LOW_TOM] = 80;
    mixer.individual_vol[HIGH_CONGA] = 88;
    mixer.individual_vol[MID_CONGA] = 83;
    mixer.individual_vol[LOW_CONGA] = 78;
    
    // 음소거 및 솔로 상태 초기화
    for (int i = 0; i < NUM_DRUMS; i++) {
        mixer.mute[i] = false;
        mixer.solo[i] = false;
        mixer.pan = 0;
        mixer.reverb = 30;
    }
}

void TR808UserInterface::initializeMirror() {
    mirror.enabled = false;
    mirror.mirror_start = 0;
    mirror.mirror_length = 8;
    mirror.mirror_reverse = false;
    mirror.mirror_volume = 100;
    mirror.mirror_offset = 0;
}

void TR808UserInterface::update() {
    // 시리얼 통신 처리
    parseSerialCommand();
    
    // MIDI 이벤트 처리
    if (MIDI.read()) {
        handleMidiMessage();
    }
    
    // 패턴 재생 처리
    if (is_playing && !is_paused) {
        uint32_t current_time = millis();
        if (current_time - last_step_time >= step_interval) {
            updateCurrentStep();
            last_step_time = current_time;
        }
    }
}

void TR808UserInterface::applyMirrorSettings() {
    // 미러 설정이 활성화된 경우 적용
    if (mirror.enabled) {
        // TODO: 미러 로직 구현
        // 패턴의 특정 부분을 복사하여 다른 위치에 배치
    }
}

void TR808UserInterface::parseSerialCommand() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();
        
        if (input.length() == 0) return;
        
        // 간단한 텍스트 프로토콜 처리
        char cmd = input.charAt(0);
        
        switch (cmd) {
            case 'L': { // Load pattern
                int pattern_num = input.substring(1).toInt();
                if (loadPattern(pattern_num)) {
                    Serial.println("OK:PatternLoaded");
                } else {
                    Serial.println("ERR:InvalidPattern");
                }
                break;
            }
            
            case 'S': { // Save pattern
                int pattern_num = input.substring(1).toInt();
                if (savePattern(pattern_num)) {
                    Serial.println("OK:PatternSaved");
                } else {
                    Serial.println("ERR:InvalidPattern");
                }
                break;
            }
            
            case 'P': { // Play pattern
                if (input.length() == 1) {
                    playPattern();
                } else {
                    int pattern_num = input.substring(1).toInt();
                    playPattern(pattern_num);
                }
                Serial.println("OK:Playing");
                break;
            }
            
            case 'K': // Stop
                stopPattern();
                Serial.println("OK:Stopped");
                break;
                
            case 'C': // Clear
                clearPattern();
                Serial.println("OK:Cleared");
                break;
                
            case 'T': { // Tempo or Trigger
                if (input.charAt(1) == 'R') { // TRIGGER
                    int comma = input.indexOf(',');
                    if (comma > 0) {
                        int drum = input.substring(2, comma).toInt();
                        int velocity = input.substring(comma + 1).toInt();
                        triggerDrum((DrumSource)drum, velocity);
                        Serial.println("OK:Triggered");
                    }
                } else { // TEMPO
                    int tempo = input.substring(1).toInt();
                    setTempo(tempo);
                    Serial.println("OK:TempoSet");
                }
                break;
            }
            
            case 'V': { // Volume
                int volume = input.substring(1).toInt();
                setMasterVolume(volume);
                Serial.println("OK:VolumeSet");
                break;
            }
            
            case 'M': { // Mixer
                int comma = input.indexOf(',');
                if (comma > 0) {
                    int drum = input.substring(1, comma).toInt();
                    int volume = input.substring(comma + 1).toInt();
                    setIndividualVolume((DrumSource)drum, volume);
                    Serial.println("OK:MixerSet");
                }
                break;
            }
            
            case '?': // Help
                sendHelp();
                break;
                
            case 'H': // Status
                if (input.equals("STATUS")) {
                    sendStatus();
                } else {
                    sendHelp();
                }
                break;
                
            default:
                Serial.println("ERR:UnknownCommand");
                Serial.println("Type ? for help");
                break;
        }
    }
}

void TR808UserInterface::processSerialPacket() {
    // 체크섬 검증
    if (!verifyChecksum()) {
        Serial.println("ERR:Checksum");
        return;
    }
    
    switch (serial_buffer.command) {
        case CMD_PATTERN_LOAD:
            if (loadPattern(serial_buffer.value)) {
                Serial.println("OK:PatternLoaded");
            } else {
                Serial.println("ERR:InvalidPattern");
            }
            break;
            
        case CMD_PATTERN_SAVE:
            if (savePattern(serial_buffer.value)) {
                Serial.println("OK:PatternSaved");
            } else {
                Serial.println("ERR:InvalidPattern");
            }
            break;
            
        case CMD_PATTERN_PLAY:
            playPattern(serial_buffer.value);
            Serial.println("OK:Playing");
            break;
            
        case CMD_PATTERN_STOP:
            stopPattern();
            Serial.println("OK:Stopped");
            break;
            
        case CMD_DRUM_TRIGGER:
            triggerDrum((DrumSource)serial_buffer.drum_id, serial_buffer.value);
            Serial.println("OK:Triggered");
            break;
            
        case CMD_DRUM_SET:
            setDrumStep((DrumSource)serial_buffer.drum_id, serial_buffer.step, serial_buffer.value);
            Serial.println("OK:DrumSet");
            break;
            
        case CMD_MIXER_SET:
            setIndividualVolume((DrumSource)serial_buffer.drum_id, serial_buffer.value);
            Serial.println("OK:MixerSet");
            break;
            
        case CMD_MIRROR_SET:
            setMirrorSettingsFromSerial();
            Serial.println("OK:MirrorSet");
            break;
            
        case CMD_STATUS_REQUEST:
            sendStatus();
            break;
            
        case CMD_HELP_REQUEST:
            sendHelp();
            break;
            
        default:
            Serial.println("ERR:UnknownCommand");
            break;
    }
}

void TR808UserInterface::handleMidiMessage() {
    MidiType midi_type = MIDI.getType();
    uint8_t note = MIDI.getData1();
    uint8_t velocity = MIDI.getData2();
    uint8_t channel = MIDI.getChannel();
    
    switch (midi_type) {
        case midi::NoteOn:
            handleMidiNoteOn(channel, note, velocity);
            break;
            
        case midi::NoteOff:
            handleMidiNoteOff(channel, note, velocity);
            break;
            
        case midi::ControlChange:
            handleMidiControlChange(channel, note, velocity);
            break;
            
        default:
            break;
    }
}

void TR808UserInterface::setMirrorSettingsFromSerial() {
    // 시리얼 데이터에서 미러 설정 적용
    mirror.enabled = (serial_buffer.step & 0x01) != 0;
    mirror.mirror_start = serial_buffer.step >> 4;
    mirror.mirror_length = serial_buffer.data[0];
    mirror.mirror_reverse = (serial_buffer.data[1] & 0x01) != 0;
    mirror.mirror_volume = serial_buffer.data[2];
    mirror.mirror_offset = serial_buffer.data[3];
}

void TR808UserInterface::handleMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    DrumSource drum = (DrumSource)mapNoteToDrum(note);
    if (drum < NUM_DRUMS) {
        uint8_t scaled_velocity = map(velocity, 0, 127, 0, 127);
        triggerDrum(drum, scaled_velocity);
    }
}

void TR808UserInterface::handleMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    // NoteOff는 TR-808에서 직접적인 처리가 필요 없음
}

void TR808UserInterface::handleMidiControlChange(uint8_t channel, uint8_t controller, uint8_t value) {
    switch (controller) {
        case 7: // Volume
            setMasterVolume(value);
            break;
        case 10: // Pan
            mixer.pan = value;
            break;
        case 64: // Sustain pedal (play/stop)
            if (value > 63) {
                playPattern();
            } else {
                stopPattern();
            }
            break;
        default:
            break;
    }
}

bool TR808UserInterface::loadPattern(uint8_t pattern_num) {
    if (pattern_num >= NUM_PATTERNS) return false;
    
    current_pattern = pattern_num;
    printPattern(pattern_num);
    return true;
}

bool TR808UserInterface::savePattern(uint8_t pattern_num) {
    if (pattern_num >= NUM_PATTERNS) return false;
    
    patterns[pattern_num] = patterns[current_pattern];
    patterns[pattern_num].active = true;
    return true;
}

void TR808UserInterface::playPattern(uint8_t pattern_num) {
    if (pattern_num < NUM_PATTERNS) {
        current_pattern = pattern_num;
    }
    
    is_playing = true;
    is_paused = false;
    play_step = 0;
    step_interval = calculateStepInterval(patterns[current_pattern].tempo);
    last_step_time = millis();
    
    Serial.printf("Playing Pattern %d\n", current_pattern);
}

void TR808UserInterface::stopPattern() {
    is_playing = false;
    is_paused = false;
    play_step = 0;
}

void TR808UserInterface::clearPattern(uint8_t pattern_num) {
    if (pattern_num == 255) pattern_num = current_pattern;
    if (pattern_num >= NUM_PATTERNS) return;
    
    // 패턴 초기화
    for (int d = 0; d < NUM_DRUMS; d++) {
        for (int s = 0; s < MAX_STEP; s++) {
            patterns[pattern_num].step[d][s] = 0;
        }
    }
    
    // 악센트 초기화
    for (int s = 0; s < MAX_STEP; s++) {
        patterns[pattern_num].accent_step[s] = 127;
    }
    
    patterns[pattern_num].active = false;
}

void TR808UserInterface::triggerDrum(DrumSource drum, uint8_t velocity) {
    if (drum >= NUM_DRUMS) return;
    
    velocity = clampValue(velocity);
    
    // 솔로/뮤트 상태 체크
    bool solo_active = false;
    for (int i = 0; i < NUM_DRUMS; i++) {
        if (mixer.solo[i]) {
            solo_active = true;
            break;
        }
    }
    
    // 현재 드럼이 음소거되었는지 확인
    if ((solo_active && !mixer.solo[drum]) || (!solo_active && mixer.mute[drum])) {
        return;
    }
    
    // 실제 드럼 트리거 (하드웨어 제어 함수 호출 필요)
    // TODO: 실제 하드웨어 인터페이스와 연동
    
    Serial.printf("TRIGGER:%d:%d\n", drum, velocity);
}

void TR808UserInterface::setDrumStep(DrumSource drum, uint8_t step, uint16_t velocity) {
    if (drum >= NUM_DRUMS || step >= MAX_STEP) return;
    
    velocity = clampValue((uint8_t)velocity);
    patterns[current_pattern].step[drum][step] = velocity;
}

uint16_t TR808UserInterface::getDrumStep(DrumSource drum, uint8_t step) {
    if (drum >= NUM_DRUMS || step >= MAX_STEP) return 0;
    return patterns[current_pattern].step[drum][step];
}

void TR808UserInterface::setMasterVolume(uint8_t volume) {
    mixer.volume = clampValue(volume);
}

uint8_t TR808UserInterface::getMasterVolume() {
    return mixer.volume;
}

void TR808UserInterface::setIndividualVolume(DrumSource drum, uint8_t volume) {
    if (drum >= NUM_DRUMS) return;
    mixer.individual_vol[drum] = clampValue(volume);
}

uint8_t TR808UserInterface::getIndividualVolume(DrumSource drum) {
    if (drum >= NUM_DRUMS) return 0;
    return mixer.individual_vol[drum];
}

void TR808UserInterface::setMute(DrumSource drum, bool mute) {
    if (drum >= NUM_DRUMS) return;
    mixer.mute[drum] = mute;
}

bool TR808UserInterface::getMute(DrumSource drum) {
    if (drum >= NUM_DRUMS) return false;
    return mixer.mute[drum];
}

void TR808UserInterface::setSolo(DrumSource drum, bool solo) {
    if (drum >= NUM_DRUMS) return;
    mixer.solo[drum] = solo;
}

bool TR808UserInterface::getSolo(DrumSource drum) {
    if (drum >= NUM_DRUMS) return false;
    return mixer.solo[drum];
}

void TR808UserInterface::setMirrorEnabled(bool enabled) {
    mirror.enabled = enabled;
}

void TR808UserInterface::setMirrorStart(uint8_t start_step) {
    mirror.mirror_start = clampValue(start_step, 0, 15);
}

void TR808UserInterface::setMirrorLength(uint8_t length) {
    mirror.mirror_length = clampValue(length, 1, 16);
}

void TR808UserInterface::setMirrorReverse(bool reverse) {
    mirror.mirror_reverse = reverse;
}

void TR808UserInterface::setMirrorVolume(uint8_t volume) {
    mirror.mirror_volume = clampValue(volume);
}

void TR808UserInterface::setMirrorOffset(uint8_t offset) {
    mirror.mirror_offset = clampValue(offset);
}

bool TR808UserInterface::getMirrorEnabled() {
    return mirror.enabled;
}

uint8_t TR808UserInterface::getMirrorStart() {
    return mirror.mirror_start;
}

uint8_t TR808UserInterface::getMirrorLength() {
    return mirror.mirror_length;
}

bool TR808UserInterface::getMirrorReverse() {
    return mirror.mirror_reverse;
}

uint8_t TR808UserInterface::getMirrorVolume() {
    return mirror.mirror_volume;
}

uint8_t TR808UserInterface::getMirrorOffset() {
    return mirror.mirror_offset;
}

void TR808UserInterface::setTempo(uint8_t bpm) {
    bpm = clampValue(bpm, 60, 200);
    patterns[current_pattern].tempo = bpm;
    step_interval = calculateStepInterval(bpm);
}

uint8_t TR808UserInterface::getTempo() {
    return patterns[current_pattern].tempo;
}

void TR808UserInterface::setSwing(uint8_t swing) {
    patterns[current_pattern].swing = clampValue(swing);
}

uint8_t TR808UserInterface::getSwing() {
    return patterns[current_pattern].swing;
}

void TR808UserInterface::setPatternLength(uint8_t length) {
    patterns[current_pattern].length = clampValue(length, 1, 16);
}

uint8_t TR808UserInterface::getPatternLength() {
    return patterns[current_pattern].length;
}

void TR808UserInterface::updateCurrentStep() {
    uint8_t pattern_length = patterns[current_pattern].length;
    
    // 현재 단계의 모든 드럼 트리거
    for (int drum = 0; drum < NUM_DRUMS; drum++) {
        uint16_t velocity = patterns[current_pattern].step[drum][play_step];
        if (velocity > 0) {
            uint8_t accent = patterns[current_pattern].accent_step[play_step];
            uint8_t final_velocity = (velocity * accent) / 127;
            triggerDrum((DrumSource)drum, final_velocity);
        }
    }
    
    // 다음 단계로 이동
    play_step = (play_step + 1) % pattern_length;
}

uint32_t TR808UserInterface::calculateStepInterval(uint8_t bpm) {
    // 16분음표 기준 계산 (4/4박 기준)
    uint32_t beat_duration = 60000 / bpm; // 1박당 밀리초
    uint32_t step_duration = beat_duration / 4; // 16분음표
    return step_duration;
}

void TR808UserInterface::sendStatus() {
    Serial.println("=== TR-808 STATUS ===");
    Serial.printf("Current Pattern: %d\n", current_pattern);
    Serial.printf("Playing: %s\n", is_playing ? "Yes" : "No");
    Serial.printf("Tempo: %d BPM\n", getTempo());
    Serial.printf("Pattern Length: %d steps\n", getPatternLength());
    Serial.printf("Master Volume: %d\n", getMasterVolume());
    Serial.printf("Mirror Enabled: %s\n", getMirrorEnabled() ? "Yes" : "No");
    
    // 믹서 상태 출력
    Serial.println("\n=== MIXER STATUS ===");
    for (int drum = 0; drum < NUM_DRUMS; drum++) {
        Serial.printf("%s: vol=%d, mute=%s, solo=%s\n",
                     drum_names[drum],
                     getIndividualVolume((DrumSource)drum),
                     getMute((DrumSource)drum) ? "Yes" : "No",
                     getSolo((DrumSource)drum) ? "Yes" : "No");
    }
    
    // 미러 상태 출력
    Serial.println("\n=== MIRROR STATUS ===");
    Serial.printf("Enabled: %s\n", getMirrorEnabled() ? "Yes" : "No");
    Serial.printf("Start: %d\n", getMirrorStart());
    Serial.printf("Length: %d\n", getMirrorLength());
    Serial.printf("Reverse: %s\n", getMirrorReverse() ? "Yes" : "No");
    Serial.printf("Volume: %d\n", getMirrorVolume());
    
    Serial.println("===================");
}

void TR808UserInterface::sendHelp() {
    Serial.println("=== TR-808 COMMAND HELP ===");
    Serial.println("PATTERN CONTROL:");
    Serial.println("  L[0-F] - Load pattern");
    Serial.println("  S[0-F] - Save pattern");  
    Serial.println("  P      - Play current pattern");
    Serial.println("  P[n]   - Play pattern n");
    Serial.println("  K      - Stop");
    Serial.println("  C      - Clear current pattern");
    
    Serial.println("\nTEMPO/PATTERN:");
    Serial.println("  T[60-200] - Set tempo BPM");
    Serial.println("  L[1-16]   - Set pattern length");
    Serial.println("  W[0-127]  - Set swing");
    
    Serial.println("\nDRUM CONTROL:");
    Serial.println("  D[d][s][0-127] - Set drum d step s to velocity");
    Serial.println("  T[d][0-127]    - Trigger drum d with velocity");
    
    Serial.println("\nMIXER CONTROL:");
    Serial.println("  V[0-127]       - Set master volume");
    Serial.println("  M[d][0-127]    - Set drum d volume");
    Serial.println("  MU[d]          - Mute drum d");
    Serial.println("  MU[d][0]       - Unmute drum d");
    Serial.println("  SO[d]          - Solo drum d");
    Serial.println("  SO[d][0]       - Unsolo drum d");
    
    Serial.println("\nMIRROR CONTROL:");
    Serial.println("  E[0-1]         - Enable/disable mirror");
    Serial.println("  O[0-15]        - Set mirror offset");
    Serial.println("  LEN[1-16]      - Set mirror length");
    Serial.println("  REV[0-1]       - Reverse mirror");
    Serial.println("  MV[0-127]      - Set mirror volume");
    
    Serial.println("\nSTATUS:");
    Serial.println("  ?             - Show this help");
    Serial.println("  STATUS        - Show current status");
    Serial.println("  PATT[n]       - Print pattern n");
    
    Serial.println("\nMIDI MAPPING:");
    Serial.println("  Note 36-51    - Trigger drums");
    Serial.println("  CC7           - Master volume");
    Serial.println("  CC10          - Pan");
    Serial.println("  CC64          - Play/Stop");
    
    Serial.println("========================");
}

void TR808UserInterface::printPattern(uint8_t pattern_num) {
    Serial.printf("=== PATTERN %d ===\n", pattern_num);
    Serial.printf("Name: %s\n", patterns[pattern_num].name);
    Serial.printf("Tempo: %d BPM\n", patterns[pattern_num].tempo);
    Serial.printf("Length: %d steps\n", patterns[pattern_num].length);
    Serial.printf("Swing: %d\n", patterns[pattern_num].swing);
    
    // 패턴 매트릭스 출력
    Serial.println("\nDrum Pattern Matrix:");
    Serial.print("    ");
    for (int step = 0; step < patterns[pattern_num].length; step++) {
        Serial.printf("%2d ", step + 1);
    }
    Serial.println();
    
    for (int drum = 0; drum < NUM_DRUMS; drum++) {
        Serial.printf("%-10s ", drum_names[drum]);
        for (int step = 0; step < patterns[pattern_num].length; step++) {
            uint16_t velocity = patterns[pattern_num].step[drum][step];
            if (velocity > 0) {
                Serial.printf(" X ");
            } else {
                Serial.printf(" . ");
            }
        }
        Serial.println();
    }
    Serial.println("=================");
}

uint8_t TR808UserInterface::mapNoteToDrum(uint8_t note) {
    // GM 드럼 맵 기반 TR-808 맵핑
    switch (note) {
        case 36: return KICK;        // Bass Drum 1
        case 38: return SNARE;       // Acoustic Snare
        case 42: return CLAP;        // Hand Clap
        case 44: return HIGH_CONGA;  // High Conga
        case 45: return MID_CONGA;   // Mid Conga
        case 46: return LOW_CONGA;   // Low Conga
        case 49: return CYMBAL;      // Crash Cymbal 1
        case 51: return TILT;        // Ride Cymbal 1
        case 37: return RIMSHOT;     // Side Stick
        case 50: return HIGH_TOM;    // High Tom 2
        case 52: return HIGH_TOM;    // Chinese Cymbal
        case 53: return MID_TOM;     // Ride Bell
        case 54: return MARACAS;     // Tambourine
        case 56: return COWBELL;     // Cowbell
        case 64: return LOW_TOM;     // High Bongo
        case 65: return LOW_TOM;     // Low Bongo
        default: return NUM_DRUMS;   // 지원되지 않는 노트
    }
}

uint8_t TR808UserInterface::mapDrumToNote(DrumSource drum) {
    switch (drum) {
        case KICK: return 36;
        case SNARE: return 38;
        case CLAP: return 42;
        case HIGH_CONGA: return 44;
        case MID_CONGA: return 45;
        case LOW_CONGA: return 46;
        case CYMBAL: return 49;
        case TILT: return 51;
        case RIMSHOT: return 37;
        case HIGH_TOM: return 50;
        case MID_TOM: return 53;
        case LOW_TOM: return 64;
        case MARACAS: return 54;
        case COWBELL: return 56;
        default: return 60; // Middle C (기본값)
    }
}

uint8_t TR808UserInterface::clampValue(uint8_t value, uint8_t min, uint8_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

uint8_t TR808UserInterface::calculateChecksum() {
    uint8_t checksum = 0;
    uint8_t* data = (uint8_t*)&serial_buffer;
    for (int i = 0; i < sizeof(SerialPacket) - 1; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool TR808UserInterface::verifyChecksum() {
    return calculateChecksum() == serial_buffer.checksum;
}

void TR808UserInterface::copyPattern(const Pattern& src, Pattern& dst) {
    memcpy(&dst, &src, sizeof(Pattern));
}

bool TR808UserInterface::comparePatterns(const Pattern& p1, const Pattern& p2) {
    return memcmp(&p1, &p2, sizeof(Pattern)) == 0;
}

uint8_t TR808UserInterface::getCurrentPattern() {
    return current_pattern;
}

bool TR808UserInterface::isPlaying() {
    return is_playing;
}

bool TR808UserInterface::isPaused() {
    return is_paused;
}

// 드럼 이름 배열 정의 (전역)
const char* drum_names[] = {
    "KICK",
    "SNARE", 
    "CYMBAL",
    "TILT",
    "OPEN_KICK",
    "OPEN_SNARE", 
    "REAR",
    "RIMSHOT",
    "MARACAS",
    "CLAP",
    "COWBELL",
    "CLAVE",
    "HIGH_TOM",
    "MID_TOM",
    "LOW_TOM",
    "HIGH_CONGA",
    "MID_CONGA", 
    "LOW_CONGA"
};

void TR808UserInterface::printDrumMap() {
    Serial.println("=== DRUM MAP ===");
    Serial.println("Note  Drum");
    Serial.println("36    KICK");
    Serial.println("38    SNARE");
    Serial.println("42    CLAP");
    Serial.println("44    HIGH_CONGA");
    Serial.println("45    MID_CONGA");
    Serial.println("46    LOW_CONGA");
    Serial.println("49    CYMBAL");
    Serial.println("51    TILT");
    Serial.println("37    RIMSHOT");
    Serial.println("50    HIGH_TOM");
    Serial.println("53    MID_TOM");
    Serial.println("64    LOW_TOM");
    Serial.println("54    MARACAS");
    Serial.println("56    COWBELL");
    Serial.println("===============");
}

// 전역 유틸리티 함수 구현
uint8_t clampValue(uint8_t value, uint8_t min, uint8_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

uint32_t calculateStepInterval(uint8_t bpm) {
    uint32_t beat_duration = 60000 / bpm; // 1박당 밀리초
    uint32_t step_duration = beat_duration / 4; // 16분음표
    return step_duration;
}