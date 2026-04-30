#pragma once
#include "Types.h"
#include <vector>

namespace Periculium {

// ---- Post Processor ----
// Final polish: overlap resolution, ghost notes, quantization
// NOTE: Made less aggressive to preserve more notes

class PostProcessor {
public:
    PostProcessor();
    
    // Main processing pipeline
    void process(std::vector<Note>& notes);
    
    // Configure processing thresholds
    void setMinNoteDuration(double duration) { m_minNoteDuration = duration; }
    void setMergeAggressiveness(double factor) { m_mergeFactor = factor; } // 0.0 = no merge, 1.0 = aggressive
    
    // Individual processing steps
    void resolveOverlaps(std::vector<Note>& notes);
    void mergeGhostNotes(std::vector<Note>& notes);
    void temporalSmoothing(std::vector<Note>& notes);
    void strictResolveOverlaps(std::vector<Note>& notes);

private:
    double m_minNoteDuration = 0.020;  // 20ms minimum (was 1ms, too aggressive)
    double m_mergeFactor = 0.3;        // Less aggressive merging
};

} // namespace Periculium
