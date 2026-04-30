#pragma once
#include "Types.h"
#include <vector>
#include <memory>
#include <functional>

namespace Periculium {

// ---- Live Detector ----
// Real-time pitch/onset detector using CQT streaming

class LiveDetector {
public:
    LiveDetector();
    ~LiveDetector();
    
    // Initialize with sample rate
    bool initialize(double sampleRate);
    
    // Process audio chunk (real-time)
    void processChunk(const float* audio, size_t frames, double timestamp);
    
    // Get detected notes since last poll
    std::vector<Note> getDetectedNotes();
    
    // Set callback for new note detection
    using NoteCallback = std::function<void(const Note&)>;
    void setNoteCallback(NoteCallback callback);
    
    // Get current latency in seconds
    double getLatency() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Periculium
