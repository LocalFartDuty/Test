#include "SustainProcessor.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace Periculium {

SustainProcessor::SustainProcessor() : m_audioVerification(true) {}

void SustainProcessor::setAudioVerification(bool enabled) {
    m_audioVerification = enabled;
}

void SustainProcessor::process(std::vector<Note>& notes, std::vector<PedalEvent>& pedals,
                               const AudioBuffer& audio) {
    if (pedals.empty()) return;
    
    // Step 1: Remove very short pedal events (< 0.1s = noise)
    removeNoisePedals(pedals);
    
    // Step 2: Merge overlapping/close pedal events
    mergeFragmentedPedals(pedals);
    
    // Step 3: Verify against audio energy
    if (m_audioVerification && !audio.isEmpty()) {
        verifyPedalsWithAudio(pedals, audio);
    }
    
    // Step 4: Hand-aware sustain trimming
    applyHandAwareTrimming(notes, pedals);
}

void SustainProcessor::removeNoisePedals(std::vector<PedalEvent>& pedals) {
    auto it = std::remove_if(pedals.begin(), pedals.end(),
        [](const PedalEvent& p) { return p.duration() < 0.1; });
    pedals.erase(it, pedals.end());
}

void SustainProcessor::mergeFragmentedPedals(std::vector<PedalEvent>& pedals) {
    if (pedals.empty()) return;
    
    std::sort(pedals.begin(), pedals.end(),
        [](const PedalEvent& a, const PedalEvent& b) { return a.start < b.start; });
    
    std::vector<PedalEvent> merged;
    const double MERGE_GAP = 0.15; // 150ms
    
    for (const auto& pedal : pedals) {
        if (merged.empty()) {
            merged.push_back(pedal);
        } else {
            auto& last = merged.back();
            if (pedal.start - last.end < MERGE_GAP) {
                last.end = std::max(last.end, pedal.end);
            } else {
                merged.push_back(pedal);
            }
        }
    }
    
    pedals = std::move(merged);
}

void SustainProcessor::verifyPedalsWithAudio(std::vector<PedalEvent>& pedals, const AudioBuffer& audio) {
    if (audio.isEmpty()) return;
    
    auto mono = audio.getMono();
    const size_t hop = 1024;
    const size_t nFrames = mono.size() / hop;
    
    if (nFrames == 0) return;
    
    // Compute energy envelope
    std::vector<double> energy(nFrames);
    for (size_t i = 0; i < nFrames; i++) {
        double sum = 0;
        for (size_t j = 0; j < hop && i * hop + j < mono.size(); j++) {
            sum += mono[i * hop + j] * mono[i * hop + j];
        }
        energy[i] = std::sqrt(sum / hop);
    }
    
    // Smooth energy
    if (energy.size() > 10) {
        std::vector<double> smoothed(energy.size());
        const int kernel = 10;
        for (size_t i = 0; i < energy.size(); i++) {
            double sum = 0;
            int count = 0;
            for (int k = -kernel; k <= kernel; k++) {
                size_t idx = i + k;
                if (idx < energy.size()) {
                    sum += energy[idx];
                    count++;
                }
            }
            smoothed[i] = sum / count;
        }
        energy = smoothed;
    }
    
    // Verify each pedal
    std::vector<PedalEvent> verified;
    for (const auto& pedal : pedals) {
        size_t startFrame = static_cast<size_t>(pedal.start * audio.sampleRate / hop);
        size_t endFrame = static_cast<size_t>(pedal.end * audio.sampleRate / hop);
        
        startFrame = std::min(startFrame, energy.size() - 1);
        endFrame = std::min(endFrame, energy.size() - 1);
        
        if (startFrame >= endFrame) {
            verified.push_back(pedal);
            continue;
        }
        
        // Check if there's energy during the pedal
        double avgEnergy = 0;
        for (size_t i = startFrame; i < endFrame; i++) {
            avgEnergy += energy[i];
        }
        avgEnergy /= (endFrame - startFrame);
        
        if (avgEnergy > 0.001) {
            verified.push_back(pedal);
        }
    }
    
    pedals = std::move(verified);
}

void SustainProcessor::applyHandAwareTrimming(std::vector<Note>& notes, const std::vector<PedalEvent>& pedals) {
    if (pedals.empty()) return;
    
    // Build pedal intervals
    std::vector<std::pair<double, double>> pedalIntervals;
    for (const auto& p : pedals) {
        pedalIntervals.emplace_back(p.start, p.end);
    }
    std::sort(pedalIntervals.begin(), pedalIntervals.end());
    
    // Sort notes by start time
    std::sort(notes.begin(), notes.end(),
        [](const Note& a, const Note& b) { return a.start < b.start; });
    
    for (auto& note : notes) {
        if (!note.isPitch()) continue;
        
        // Check if note is under sustain
        bool underSustain = false;
        std::pair<double, double> pedalInterval;
        
        for (const auto& interval : pedalIntervals) {
            if (note.start >= interval.first && note.start < interval.second) {
                underSustain = true;
                pedalInterval = interval;
                break;
            }
        }
        
        if (!underSustain) continue;
        
        double pedalDuration = pedalInterval.second - pedalInterval.first;
        double noteDuration = note.duration();
        double spanRatio = pedalDuration > 0 ? noteDuration / pedalDuration : 0;
        
        // Determine hand
        bool isLeftHand = note.pitch <= Hand::LEFT_HAND_MAX_PITCH;
        
        // Count hand activity during note
        int handRangeMin = isLeftHand ? Hand::MIN_PITCH : Hand::RIGHT_HAND_MIN_PITCH;
        int handRangeMax = isLeftHand ? Hand::LEFT_HAND_MAX_PITCH : Hand::MAX_PITCH;
        
        int newNotesDuring = 0;
        for (const auto& other : notes) {
            if (!other.isPitch()) continue;
            if (other.pitch < handRangeMin || other.pitch > handRangeMax) continue;
            if (other.start > note.start + 0.05 && other.start < note.end - 0.05) {
                newNotesDuring++;
            }
        }
        
        // Decide whether to trim
        bool shouldTrim = false;
        double trimTarget = note.end;
        
        if (isLeftHand) {
            // Left hand: more lenient
            if (spanRatio > 0.8 && newNotesDuring >= 3) {
                shouldTrim = true;
                trimTarget = note.start + std::min(2.5, noteDuration * 0.5);
            } else if (spanRatio > 0.9 && newNotesDuring >= 1) {
                shouldTrim = true;
                trimTarget = note.start + std::min(2.0, noteDuration * 0.4);
            }
        } else {
            // Right hand: more aggressive
            if (spanRatio > 0.6 && newNotesDuring >= 2) {
                shouldTrim = true;
                trimTarget = note.start + std::min(1.5, noteDuration * 0.4);
            } else if (spanRatio > 0.8 && newNotesDuring >= 1) {
                shouldTrim = true;
                trimTarget = note.start + std::min(1.2, noteDuration * 0.35);
            } else if (noteDuration > 3.0 && newNotesDuring >= 1) {
                shouldTrim = true;
                trimTarget = note.start + std::min(1.5, noteDuration * 0.3);
            }
        }
        
        if (shouldTrim && trimTarget < note.end) {
            note.end = trimTarget;
        }
    }
    
    // Ensure no note extends past next same-pitch onset
    std::map<int, std::vector<Note*>> byPitch;
    for (auto& note : notes) {
        if (note.isPitch()) {
            byPitch[note.pitch].push_back(&note);
        }
    }
    
    for (auto& pair : byPitch) {
        auto& pitchNotes = pair.second;
        std::sort(pitchNotes.begin(), pitchNotes.end(),
            [](const Note* a, const Note* b) { return a->start < b->start; });
        
        for (size_t i = 0; i < pitchNotes.size() - 1; i++) {
            if (pitchNotes[i]->end > pitchNotes[i + 1]->start - 0.01) {
                pitchNotes[i]->end = pitchNotes[i + 1]->start - 0.01;
                if (pitchNotes[i]->end <= pitchNotes[i]->start) {
                    pitchNotes[i]->end = pitchNotes[i]->start + 0.01;
                }
            }
        }
    }
}

} // namespace Periculium
