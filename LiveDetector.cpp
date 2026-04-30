#include "LiveDetector.h"

namespace Periculium {

struct LiveDetector::Impl {
    double sampleRate = 44100.0;
    NoteCallback callback;
    std::vector<Note> detectedNotes;
    double latency = 0.008; // 8ms target
};

LiveDetector::LiveDetector() : m_impl(std::make_unique<Impl>()) {}

LiveDetector::~LiveDetector() = default;

bool LiveDetector::initialize(double sampleRate) {
    m_impl->sampleRate = sampleRate;
    // TODO: Initialize CQT kernel, onset detector, etc.
    return true;
}

void LiveDetector::processChunk(const float* audio, size_t frames, double timestamp) {
    // TODO: Implement CQT streaming, onset detection, pitch classification
    // This is a stub for now
}

std::vector<Note> LiveDetector::getDetectedNotes() {
    auto notes = std::move(m_impl->detectedNotes);
    m_impl->detectedNotes.clear();
    return notes;
}

void LiveDetector::setNoteCallback(NoteCallback callback) {
    m_impl->callback = callback;
}

double LiveDetector::getLatency() const {
    return m_impl->latency;
}

} // namespace Periculium
