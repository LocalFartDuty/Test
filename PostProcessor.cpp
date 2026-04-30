#include "PostProcessor.h"
#include <map>
#include <algorithm>
#include <cmath>

namespace Periculium {

PostProcessor::PostProcessor() 
    : m_minNoteDuration(0.020)  // 20ms minimum - preserves more notes
    , m_mergeFactor(0.3) {}     // Less aggressive merging

void PostProcessor::process(std::vector<Note>& notes) {
    // Process in order of least to most destructive
    temporalSmoothing(notes);      // Just timing adjustments
    resolveOverlaps(notes);        // Fix overlaps
    mergeGhostNotes(notes);        // Merge only obvious ghosts
    strictResolveOverlaps(notes);  // Final cleanup
}

void PostProcessor::resolveOverlaps(std::vector<Note>& notes) {
    // Basic overlap resolution - same as strictResolveOverlaps
    strictResolveOverlaps(notes);
}

void PostProcessor::mergeGhostNotes(std::vector<Note>& notes) {
    // Less aggressive ghost note merging
    // Only merge if notes are VERY close (5ms) AND very short (<30ms)
    const double minGap = 0.005 * (1.0 - m_mergeFactor);      // 3.5ms with default 0.3 factor
    const double minDuration = 0.030;  // Only merge short staccato notes
    
    // Separate pitch notes and control changes
    std::vector<Note> pitchNotes;
    std::vector<Note> ccNotes;
    
    for (const auto& note : notes) {
        if (note.isPitch()) {
            pitchNotes.push_back(note);
        } else {
            ccNotes.push_back(note);
        }
    }
    
    // Group by pitch
    std::map<int, std::vector<Note>> byPitch;
    for (const auto& note : pitchNotes) {
        byPitch[note.pitch].push_back(note);
    }
    
    // Merge only obvious ghost notes (very close, very short)
    std::vector<Note> merged;
    for (auto& pair : byPitch) {
        auto& pnotes = pair.second;
        std::sort(pnotes.begin(), pnotes.end(),
            [](const Note& a, const Note& b) { return a.start < b.start; });
        
        size_t i = 0;
        while (i < pnotes.size()) {
            Note current = pnotes[i];
            size_t j = i + 1;
            
            // Merge only if VERY close AND short duration
            while (j < pnotes.size()) {
                Note next = pnotes[j];
                double gap = next.start - current.end;
                double currDur = current.duration();
                
                // Strict criteria: tiny gap AND short note
                if (gap < minGap && currDur < minDuration && gap < 0.010) {
                    current.end = std::max(current.end, next.end);
                    current.velocity = std::max(current.velocity, next.velocity);
                    j++;
                } else {
                    break;
                }
            }
            
            merged.push_back(current);
            i = j;
        }
    }
    
    // Reassemble
    notes.clear();
    notes.insert(notes.end(), merged.begin(), merged.end());
    notes.insert(notes.end(), ccNotes.begin(), ccNotes.end());
    
    std::sort(notes.begin(), notes.end(),
        [](const Note& a, const Note& b) { return a.start < b.start; });
}

void PostProcessor::temporalSmoothing(std::vector<Note>& notes) {
    std::vector<Note> pitchNotes;
    std::vector<Note> ccNotes;
    
    for (const auto& note : notes) {
        if (note.isPitch()) {
            pitchNotes.push_back(note);
        } else {
            ccNotes.push_back(note);
        }
    }
    
    if (pitchNotes.empty()) return;
    
    // Chord alignment - notes within 15ms should be simultaneous
    const double CHORD_TOLERANCE = 0.015;
    std::sort(pitchNotes.begin(), pitchNotes.end(),
        [](const Note& a, const Note& b) { return a.start < b.start; });
    
    std::vector<std::vector<Note>> chordGroups;
    std::vector<Note> currentGroup = {pitchNotes[0]};
    
    for (size_t i = 1; i < pitchNotes.size(); i++) {
        if (pitchNotes[i].start - currentGroup[0].start < CHORD_TOLERANCE) {
            currentGroup.push_back(pitchNotes[i]);
        } else {
            chordGroups.push_back(currentGroup);
            currentGroup = {pitchNotes[i]};
        }
    }
    chordGroups.push_back(currentGroup);
    
    for (auto& group : chordGroups) {
        if (group.size() > 1) {
            double chordTime = group[0].start;
            for (auto& note : group) {
                note.start = chordTime;
            }
        }
    }
    
    // Fine quantization to 5ms grid
    const double GRID_SIZE = 0.005;
    for (auto& note : pitchNotes) {
        note.start = std::round(note.start / GRID_SIZE) * GRID_SIZE;
        note.end = std::round(note.end / GRID_SIZE) * GRID_SIZE;
        if (note.end <= note.start) {
            note.end = note.start + GRID_SIZE;
        }
    }
    
    // Reassemble
    notes.clear();
    notes.insert(notes.end(), pitchNotes.begin(), pitchNotes.end());
    notes.insert(notes.end(), ccNotes.begin(), ccNotes.end());
    
    std::sort(notes.begin(), notes.end(),
        [](const Note& a, const Note& b) { return a.start < b.start; });
}

void PostProcessor::strictResolveOverlaps(std::vector<Note>& notes) {
    // Multiple passes to handle cascading fixes
    // NOTE: Do NOT drop short notes - they might be valid staccato notes
    for (int pass = 0; pass < 2; pass++) {  // Reduced from 3 to 2 passes for speed
        std::map<int, std::vector<Note>> byPitch;
        std::vector<Note> ccNotes;
        
        for (auto& note : notes) {
            if (note.isPitch()) {
                byPitch[note.pitch].push_back(note);
            } else {
                ccNotes.push_back(note);
            }
        }
        
        std::vector<Note> clean;
        for (auto& pair : byPitch) {
            auto& pnotes = pair.second;
            std::sort(pnotes.begin(), pnotes.end(),
                [](const Note& a, const Note& b) { return a.start < b.start; });
            
            for (size_t i = 0; i < pnotes.size() - 1; i++) {
                if (pnotes[i].end > pnotes[i + 1].start) {
                    pnotes[i].end = pnotes[i + 1].start - 0.003;  // 3ms gap
                    // Ensure minimum duration but don't drop the note
                    if (pnotes[i].end <= pnotes[i].start) {
                        pnotes[i].end = pnotes[i].start + m_minNoteDuration;
                    }
                }
            }
            
            // Keep ALL notes - only drop if they're truly invalid (zero or negative duration)
            // This was dropping valid short notes before!
            for (const auto& note : pnotes) {
                if (note.end > note.start) {  // Was: note.end > note.start + 0.001
                    clean.push_back(note);
                }
            }
        }
        
        notes = clean;
        notes.insert(notes.end(), ccNotes.begin(), ccNotes.end());
    }
}

} // namespace Periculium
