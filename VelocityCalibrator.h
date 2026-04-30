#pragma once
#include "Types.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace Periculium {

// ---- Velocity Calibrator ----
// Maps model velocities to full [1,127] range using actual audio energy

class VelocityCalibrator {
public:
    VelocityCalibrator();
    
    // Calibrate velocities using detected notes and audio RMS energy
    void calibrate(std::vector<Note>& notes, const AudioBuffer& audio);
    
    // Get the velocity range used for calibration
    void getVelocityRange(int& minVel, int& maxVel) const;

private:
    int m_minVelocity;
    int m_maxVelocity;
    bool m_calibrated;
};

} // namespace Periculium
