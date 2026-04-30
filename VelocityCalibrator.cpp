#include "VelocityCalibrator.h"
#include <numeric>
#include <algorithm>

namespace Periculium {

VelocityCalibrator::VelocityCalibrator() 
    : m_minVelocity(1), m_maxVelocity(127), m_calibrated(false) {}

void VelocityCalibrator::calibrate(std::vector<Note>& notes, const AudioBuffer& audio) {
    if (notes.empty() || audio.isEmpty()) return;
    
    // Get mono audio
    std::vector<float> mono = audio.getMono();
    
    // Calculate RMS energy for each note's onset window
    std::vector<float> noteEnergies;
    std::vector<int> noteVelocities;
    
    for (const auto& note : notes) {
        if (!note.isPitch()) continue;
        
        // Get onset frame
        size_t onsetFrame = static_cast<size_t>(note.start * audio.sampleRate);
        size_t windowSize = static_cast<size_t>(0.05 * audio.sampleRate); // 50ms window
        
        if (onsetFrame + windowSize >= mono.size()) continue;
        
        // Calculate RMS energy in onset window
        float sum = 0.0f;
        for (size_t i = onsetFrame; i < onsetFrame + windowSize; i++) {
            sum += mono[i] * mono[i];
        }
        float rms = std::sqrt(sum / windowSize);
        
        noteEnergies.push_back(rms);
        noteVelocities.push_back(note.velocity);
    }
    
    if (noteEnergies.empty()) return;
    
    // Find energy percentiles (ignore outliers)
    std::vector<float> energiesCopy = noteEnergies;
    std::sort(energiesCopy.begin(), energiesCopy.end());
    
    size_t p5Idx = energiesCopy.size() * 5 / 100;
    size_t p95Idx = energiesCopy.size() * 95 / 100;
    
    float energyP5 = energiesCopy[p5Idx];
    float energyP95 = energiesCopy[p95Idx];
    
    if (energyP95 <= energyP5) return; // No dynamic range
    
    // Map velocities to full [1, 127] range based on energy
    for (size_t i = 0; i < notes.size(); i++) {
        if (!notes[i].isPitch()) continue;
        
        float energy = noteEnergies[i];
        
        // Linear mapping based on energy percentile
        if (energy <= energyP5) {
            notes[i].velocity = 1;
        } else if (energy >= energyP95) {
            notes[i].velocity = 127;
        } else {
            float ratio = (energy - energyP5) / (energyP95 - energyP5);
            notes[i].velocity = static_cast<int>(1 + ratio * 126);
        }
        
        // Clamp
        notes[i].velocity = std::max(1, std::min(127, notes[i].velocity));
    }
    
    // Per-pitch velocity smoothing for rapid repeats
    // Real pianists don't wildly vary volume on fast repeated notes
    std::vector<std::vector<Note*>> byPitch(109); // 0-108
    
    for (auto& note : notes) {
        if (note.isPitch() && note.pitch >= 21 && note.pitch <= 108) {
            byPitch[note.pitch].push_back(&note);
        }
    }
    
    for (auto& pitchNotes : byPitch) {
        if (pitchNotes.size() < 2) continue;
        
        // Sort by start time
        std::sort(pitchNotes.begin(), pitchNotes.end(),
            [](const Note* a, const Note* b) { return a->start < b->start; });
        
        for (size_t i = 1; i < pitchNotes.size(); i++) {
            Note* prev = pitchNotes[i-1];
            Note* curr = pitchNotes[i];
            
            // Only smooth if notes are close in time (< 0.5s)
            if (curr->start - prev->end < 0.5) {
                curr->velocity = static_cast<int>(curr->velocity * 0.7 + prev->velocity * 0.3);
                curr->velocity = std::max(1, std::min(127, curr->velocity));
            }
        }
    }
    
    m_calibrated = true;
    
    // Calculate actual velocity range after calibration
    std::vector<int> finalVels;
    for (const auto& note : notes) {
        if (note.isPitch()) finalVels.push_back(note.velocity);
    }
    
    if (!finalVels.empty()) {
        m_minVelocity = *std::min_element(finalVels.begin(), finalVels.end());
        m_maxVelocity = *std::max_element(finalVels.begin(), finalVels.end());
    }
}

void VelocityCalibrator::getVelocityRange(int& minVel, int& maxVel) const {
    minVel = m_minVelocity;
    maxVel = m_maxVelocity;
}

} // namespace Periculium
