#pragma once
#include "Types.h"
#include <vector>

namespace Periculium {

// ---- Hand Physics Model ----
// Assigns fingers to notes and adjusts release times based on physical constraints

class HandPhysicsModel {
public:
    HandPhysicsModel();
    
    // Assign fingers and hands to notes, adjust release times
    void process(std::vector<Note>& notes);
    
    // Get statistics about finger assignments
    struct Stats {
        int leftHandNotes;
        int rightHandNotes;
        int unassignedNotes;
        int impossibleStretches;
    };
    Stats getStats() const;

private:
    void assignHands(std::vector<Note>& notes);
    void assignFingers(std::vector<Note>& notes);
    void adjustReleaseTimes(std::vector<Note>& notes);
    bool isPhysicallyPossible(const Note& a, const Note& b, int hand) const;
    
    Stats m_stats;
};

} // namespace Periculium
