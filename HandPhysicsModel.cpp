#include "HandPhysicsModel.h"
#include <algorithm>
#include <map>

namespace Periculium {

HandPhysicsModel::HandPhysicsModel() {
    m_stats = {0, 0, 0, 0};
}

void HandPhysicsModel::process(std::vector<Note>& notes) {
    // Reset stats
    m_stats = {0, 0, 0, 0};
    
    // Sort notes by start time
    std::sort(notes.begin(), notes.end(), 
        [](const Note& a, const Note& b) { return a.start < b.start; });
    
    assignHands(notes);
    assignFingers(notes);
    adjustReleaseTimes(notes);
}

void HandPhysicsModel::assignHands(std::vector<Note>& notes) {
    for (auto& note : notes) {
        if (!note.isPitch()) continue;
        
        if (note.pitch <= Hand::LEFT_HAND_MAX_PITCH) {
            note.hand = 1; // Left hand
            m_stats.leftHandNotes++;
        } else if (note.pitch >= Hand::RIGHT_HAND_MIN_PITCH) {
            note.hand = 2; // Right hand
            m_stats.rightHandNotes++;
        } else {
            // In the middle range - assign based on context
            note.hand = 0; // Unassigned for now
            m_stats.unassignedNotes++;
        }
    }
}

void HandPhysicsModel::assignFingers(std::vector<Note>& notes) {
    // Group notes by hand
    std::vector<std::vector<Note*>> byHand(3); // 0=unassigned, 1=left, 2=right
    
    for (auto& note : notes) {
        if (note.isPitch() && note.hand >= 1 && note.hand <= 2) {
            byHand[note.hand].push_back(&note);
        }
    }
    
    // Assign fingers for each hand
    for (int hand = 1; hand <= 2; hand++) {
        auto& handNotes = byHand[hand];
        if (handNotes.empty()) continue;
        
        // Sort by start time
        std::sort(handNotes.begin(), handNotes.end(),
            [](const Note* a, const Note* b) { return a->start < b->start; });
        
        // Greedy finger assignment
        // For each time slice, assign up to 5 notes to fingers 1-5
        std::map<int, double> fingerReleaseTime; // finger -> when it's free
        
        for (auto* note : handNotes) {
            // Find available fingers
            std::vector<int> availableFingers;
            for (int f = 1; f <= Hand::FINGERS_PER_HAND; f++) {
                if (fingerReleaseTime[f] <= note->start) {
                    availableFingers.push_back(f);
                }
            }
            
            if (availableFingers.empty()) {
                // No fingers available - must release a note
                // Release the note that ends soonest
                int releaseFinger = 1;
                double soonestRelease = fingerReleaseTime[1];
                for (int f = 2; f <= Hand::FINGERS_PER_HAND; f++) {
                    if (fingerReleaseTime[f] < soonestRelease) {
                        soonestRelease = fingerReleaseTime[f];
                        releaseFinger = f;
                    }
                }
                
                // Adjust the note being released to end at current note's start
                // This is handled in adjustReleaseTimes
                note->finger = releaseFinger;
            } else {
                // Assign to closest finger (by pitch proximity)
                // This is a simplified heuristic
                note->finger = availableFingers[0];
            }
            
            // Update finger release time
            fingerReleaseTime[note->finger] = note->end;
        }
    }
}

void HandPhysicsModel::adjustReleaseTimes(std::vector<Note>& notes) {
    // Group notes by hand and finger
    std::map<std::pair<int, int>, std::vector<Note*>> byHandFinger;
    
    for (auto& note : notes) {
        if (note.isPitch() && note.hand >= 1 && note.hand <= 2 && note.finger >= 1) {
            byHandFinger[{note.hand, note.finger}].push_back(&note);
        }
    }
    
    // For each finger, ensure notes don't overlap impossibly
    for (auto& [key, fingerNotes] : byHandFinger) {
        if (fingerNotes.size() < 2) continue;
        
        // Sort by start time
        std::sort(fingerNotes.begin(), fingerNotes.end(),
            [](const Note* a, const Note* b) { return a->start < b->start; });
        
        for (size_t i = 0; i < fingerNotes.size() - 1; i++) {
            Note* current = fingerNotes[i];
            Note* next = fingerNotes[i + 1];
            
            // If current note overlaps with next note
            if (current->end > next->start) {
                // Calculate realistic release time
                // Finger needs to move: transition time + deceleration
                double transitionTime = Hand::FINGER_TRANSITION_NORMAL_MS / 1000.0;
                
                // Release before next note starts
                double releaseTime = next->start - transitionTime;
                
                // Ensure minimum duration
                if (releaseTime > current->start + 0.05) {
                    current->end = releaseTime;
                } else {
                    // Too short - just end before next note
                    current->end = next->start - Hand::MIN_RELEASE_GAP_S;
                }
                
                // Ensure positive duration
                if (current->end <= current->start) {
                    current->end = current->start + 0.05;
                }
            }
        }
    }
    
    // Check for impossible stretches
    // Notes in the same hand that are too far apart to play simultaneously
    for (int hand = 1; hand <= 2; hand++) {
        std::vector<Note*> handNotes;
        for (auto& note : notes) {
            if (note.isPitch() && note.hand == hand) {
                handNotes.push_back(&note);
            }
        }
        
        for (size_t i = 0; i < handNotes.size(); i++) {
            for (size_t j = i + 1; j < handNotes.size(); j++) {
                if (!isPhysicallyPossible(*handNotes[i], *handNotes[j], hand)) {
                    m_stats.impossibleStretches++;
                    // Release the earlier note
                    if (handNotes[i]->start < handNotes[j]->start) {
                        handNotes[i]->end = handNotes[j]->start - Hand::MIN_RELEASE_GAP_S;
                    } else {
                        handNotes[j]->end = handNotes[i]->start - Hand::MIN_RELEASE_GAP_S;
                    }
                }
            }
        }
    }
}

bool HandPhysicsModel::isPhysicallyPossible(const Note& a, const Note& b, int hand) const {
    // Check if two notes overlap and are too far apart for one hand
    if (a.end <= b.start || b.end <= a.start) return true; // Don't overlap
    
    double overlapStart = std::max(a.start, b.start);
    double overlapEnd = std::min(a.end, b.end);
    
    if (overlapStart >= overlapEnd) return true; // No actual overlap
    
    // Check pitch distance
    int pitchDiff = std::abs(a.pitch - b.pitch);
    
    // Maximum stretch is about 10 semitones (octave + 2)
    if (pitchDiff > Hand::MAX_HAND_SPAN_SEMITONES) {
        return false;
    }
    
    return true;
}

HandPhysicsModel::Stats HandPhysicsModel::getStats() const {
    return m_stats;
}

} // namespace Periculium
