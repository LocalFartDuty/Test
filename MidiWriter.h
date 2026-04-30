#pragma once
#include "Types.h"
#include <string>
#include <vector>

namespace Periculium {

// ---- MIDI Writer ----
// Writes notes and pedals to MIDI file format

class MidiWriter {
public:
    MidiWriter();
    
    // Write transcription result to MIDI file
    bool write(const std::string& path, const TranscriptionResult& result);
    
    // Write notes and pedals directly
    bool write(const std::string& path, const std::vector<Note>& notes, 
               const std::vector<PedalEvent>& pedals);
    
    // Set tempo (default 120 BPM)
    void setTempo(double bpm);
    
    // Set time signature (default 4/4)
    void setTimeSignature(int numerator, int denominator);

private:
    double m_tempo;
    int m_timeSigNum;
    int m_timeSigDen;
};

} // namespace Periculium
