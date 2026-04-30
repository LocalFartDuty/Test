#include "MidiWriter.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace Periculium {

// Simple MIDI file writer
// MIDI file format: https://www.midi.org/specifications

struct MidiEvent {
    double time;  // seconds
    int status;
    int data1;
    int data2;
};

MidiWriter::MidiWriter() 
    : m_tempo(120.0), m_timeSigNum(4), m_timeSigDen(4) {}

void MidiWriter::setTempo(double bpm) {
    m_tempo = bpm;
}

void MidiWriter::setTimeSignature(int numerator, int denominator) {
    m_timeSigNum = numerator;
    m_timeSigDen = denominator;
}

bool MidiWriter::write(const std::string& path, const TranscriptionResult& result) {
    return write(path, result.notes, result.pedals);
}

bool MidiWriter::write(const std::string& path, const std::vector<Note>& notes,
                       const std::vector<PedalEvent>& pedals) {
    // Convert notes and pedals to MIDI events
    std::vector<MidiEvent> events;
    
    // Note on/off events
    for (const auto& note : notes) {
        if (!note.isPitch()) continue;
        
        MidiEvent noteOn;
        noteOn.time = note.start;
        noteOn.status = 0x90; // Note on, channel 0
        noteOn.data1 = note.pitch;
        noteOn.data2 = note.velocity;
        events.push_back(noteOn);
        
        MidiEvent noteOff;
        noteOff.time = note.end;
        noteOff.status = 0x80; // Note off, channel 0
        noteOff.data1 = note.pitch;
        noteOff.data2 = 0;
        events.push_back(noteOff);
    }
    
    // Pedal events (CC64 = sustain)
    for (const auto& pedal : pedals) {
        if (pedal.ccNumber == 64) {
            MidiEvent pedalOn;
            pedalOn.time = pedal.start;
            pedalOn.status = 0xB0; // CC, channel 0
            pedalOn.data1 = 64;
            pedalOn.data2 = pedal.valueOn;
            events.push_back(pedalOn);
            
            MidiEvent pedalOff;
            pedalOff.time = pedal.end;
            pedalOff.status = 0xB0;
            pedalOff.data1 = 64;
            pedalOff.data2 = pedal.valueOff;
            events.push_back(pedalOff);
        }
    }
    
    // Sort events by time
    std::sort(events.begin(), events.end(),
        [](const MidiEvent& a, const MidiEvent& b) { return a.time < b.time; });
    
    // Write MIDI file
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    // MIDI header
    file.write("MThd", 4);
    uint32_t headerLength = 6;
    file.write(reinterpret_cast<const char*>(&headerLength), 4);
    uint16_t format = 0; // Single track
    file.write(reinterpret_cast<const char*>(&format), 2);
    uint16_t numTracks = 1;
    file.write(reinterpret_cast<const char*>(&numTracks), 2);
    uint16_t division = 480; // Ticks per quarter note
    file.write(reinterpret_cast<const char*>(&division), 2);
    
    // MIDI track
    file.write("MTrk", 4);
    
    // Calculate ticks per second: tempo / 60 * division
    double ticksPerSecond = (m_tempo / 60.0) * division;
    
    // Build track data
    std::vector<uint8_t> trackData;
    
    // Tempo meta event
    trackData.push_back(0x00); // Delta time 0
    trackData.push_back(0xFF); // Meta
    trackData.push_back(0x51); // Set tempo
    trackData.push_back(0x03); // Length 3
    uint32_t microsPerQuarter = static_cast<uint32_t>(60000000.0 / m_tempo);
    trackData.push_back((microsPerQuarter >> 16) & 0xFF);
    trackData.push_back((microsPerQuarter >> 8) & 0xFF);
    trackData.push_back(microsPerQuarter & 0xFF);
    
    // Write events
    double lastTime = 0;
    for (const auto& event : events) {
        // Calculate delta time in ticks
        double deltaTime = event.time - lastTime;
        uint32_t deltaTicks = static_cast<uint32_t>(deltaTime * ticksPerSecond);
        
        // Variable-length quantity encoding
        std::vector<uint8_t> vlq;
        uint32_t value = deltaTicks;
        while (value > 0x7F) {
            vlq.push_back((value & 0x7F) | 0x80);
            value >>= 7;
        }
        vlq.push_back(value & 0x7F);
        std::reverse(vlq.begin(), vlq.end());
        
        trackData.insert(trackData.end(), vlq.begin(), vlq.end());
        
        // Event status
        trackData.push_back(event.status);
        trackData.push_back(event.data1);
        if ((event.status & 0xF0) != 0xC0 && (event.status & 0xF0) != 0xD0) {
            trackData.push_back(event.data2);
        }
        
        lastTime = event.time;
    }
    
    // End of track
    trackData.push_back(0x00); // Delta time 0
    trackData.push_back(0xFF); // Meta
    trackData.push_back(0x2F); // End of track
    trackData.push_back(0x00); // Length 0
    
    // Write track length
    uint32_t trackLength = trackData.size();
    file.write(reinterpret_cast<const char*>(&trackLength), 4);
    
    // Write track data
    file.write(reinterpret_cast<const char*>(trackData.data()), trackData.size());
    
    file.close();
    return true;
}

} // namespace Periculium
