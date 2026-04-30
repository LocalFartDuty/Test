#pragma once
#include "Types.h"
#include <vector>
#include <memory>

namespace Periculium {

// ---- Adaptive Model ----
// Online learning system that trains the live detector from the main model

class AdaptiveModel {
public:
    AdaptiveModel();
    
    // Initialize with model parameters
    bool initialize();
    
    // Train with ground truth from main model
    void train(const std::vector<Note>& groundTruth, const AudioBuffer& audio);
    
    // Get current accuracy estimate
    double getAccuracy() const;
    
    // Enable/disable adaptive learning
    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Periculium
