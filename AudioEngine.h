#pragma once
#include "Types.h"
#include <string>
#include <functional>
#include <memory>

namespace Periculium {

// ---- Audio Engine ----
// Handles audio file reading and microphone capture

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    // ---- File I/O ----
    
    // Load audio from file (WAV, FLAC, MP3 via FFmpeg, etc.)
    AudioBuffer loadFile(const std::string& path);
    
    // Load audio from video file (extracts audio via FFmpeg)
    AudioBuffer loadFromVideo(const std::string& path);

    // Resample audio to target sample rate
    static AudioBuffer resample(const AudioBuffer& input, double targetSr);

    // ---- Microphone Capture ----

    // List available input devices
    struct DeviceInfo {
        int id;
        std::string name;
        int maxChannels;
        double defaultSampleRate;
    };
    std::vector<DeviceInfo> getInputDevices();

    // Start/stop microphone capture
    using AudioCallback = std::function<void(const float* data, size_t frames, double timestamp)>;
    bool startCapture(int deviceId, double sampleRate, size_t bufferSize, AudioCallback callback);
    void stopCapture();
    bool isCapturing() const;

    // ---- Utility ----

    // Get file info without loading
    struct FileInfo {
        double duration;
        double sampleRate;
        int channels;
        int bitDepth;
        std::string format;
    };
    FileInfo getFileInfo(const std::string& path);

    // Check if FFmpeg is available
    static bool isFFmpegAvailable();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Periculium
