#pragma once
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

namespace Periculium {

// ---- Core Data Types ----

struct Note {
    double start;       // seconds
    double end;         // seconds
    int pitch;          // MIDI pitch (21-108), negative for CC events (-64 = sustain)
    int velocity;       // 1-127
    int finger;         // 0=unassigned, 1-5=thumb..pinky
    int hand;           // 0=unassigned, 1=left, 2=right
    bool hasOnset;
    bool hasOffset;

    Note() : start(0), end(0), pitch(0), velocity(64), finger(0), hand(0),
             hasOnset(true), hasOffset(true) {}

    Note(double s, double e, int p, int v, bool on = true, bool off = true)
        : start(s), end(e), pitch(p), velocity(v), finger(0), hand(0),
          hasOnset(on), hasOffset(off) {}

    double duration() const { return end - start; }
    bool isPitch() const { return pitch > 0; }
    bool isSustain() const { return pitch == -64; }
    bool isValid() const { return end > start; }
};

struct PedalEvent {
    double start;
    double end;
    int ccNumber;       // 64 = sustain, 66 = sostenuto, 67 = soft
    int valueOn;        // 0-127
    int valueOff;

    PedalEvent() : start(0), end(0), ccNumber(64), valueOn(127), valueOff(0) {}
    PedalEvent(double s, double e, int cc = 64) 
        : start(s), end(e), ccNumber(cc), valueOn(127), valueOff(0) {}
    double duration() const { return end - start; }
};

struct TranscriptionResult {
    std::vector<Note> notes;
    std::vector<PedalEvent> pedals;
    double audioDuration;
    double processingTime;
    double realtimeFactor;
    int noteCount;

    TranscriptionResult() : audioDuration(0), processingTime(0), 
                            realtimeFactor(0), noteCount(0) {}
};

// ---- Audio Buffer ----

struct AudioBuffer {
    std::vector<float> samples;
    double sampleRate;
    int channels;

    AudioBuffer() : sampleRate(44100.0), channels(1) {}
    AudioBuffer(size_t size, double sr = 44100.0, int ch = 1) 
        : samples(size, 0.0f), sampleRate(sr), channels(ch) {}

    size_t frameCount() const { return channels > 0 ? samples.size() / channels : 0; }
    double duration() const { return sampleRate > 0 ? frameCount() / sampleRate : 0; }
    bool isEmpty() const { return samples.empty(); }
    
    // Get mono samples (average channels if stereo)
    std::vector<float> getMono() const {
        if (channels == 1) return samples;
        std::vector<float> mono(frameCount());
        for (size_t i = 0; i < mono.size(); i++) {
            float sum = 0;
            for (int c = 0; c < channels; c++) {
                sum += samples[i * channels + c];
            }
            mono[i] = sum / channels;
        }
        return mono;
    }
};

// ---- Hand Constants ----

namespace Hand {
    constexpr int LEFT_HAND_MAX_PITCH = 54;     // F#3
    constexpr int RIGHT_HAND_MIN_PITCH = 55;    // G3
    constexpr int MIN_PITCH = 21;               // A0
    constexpr int MAX_PITCH = 108;              // C8
    constexpr int FINGERS_PER_HAND = 5;
    constexpr double MAX_HAND_SPAN_SEMITONES = 10;  // ~octave + 2
    constexpr double FINGER_TRANSITION_FAST_MS = 80;
    constexpr double FINGER_TRANSITION_NORMAL_MS = 200;
    constexpr double MIN_RELEASE_GAP_S = 0.005;     // 5ms minimum gap
}

} // namespace Periculium
