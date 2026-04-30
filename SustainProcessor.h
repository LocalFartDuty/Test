#pragma once
#include "Types.h"
#include <vector>

namespace Periculium {

// ---- Sustain Processor ----
// Enhanced sustain pedal detection and hand-aware trimming

class SustainProcessor {
public:
    SustainProcessor();
    
    // Process sustain pedal events and trim notes accordingly
    void process(std::vector<Note>& notes, std::vector<PedalEvent>& pedals, 
                const AudioBuffer& audio = AudioBuffer());
    
    // Enable/disable audio verification
    void setAudioVerification(bool enabled);

private:
    void mergeFragmentedPedals(std::vector<PedalEvent>& pedals);
    void removeNoisePedals(std::vector<PedalEvent>& pedals);
    void verifyPedalsWithAudio(std::vector<PedalEvent>& pedals, const AudioBuffer& audio);
    void applyHandAwareTrimming(std::vector<Note>& notes, const std::vector<PedalEvent>& pedals);
    
    bool m_audioVerification;
};

} // namespace Periculium
