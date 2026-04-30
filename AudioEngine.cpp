#include "AudioEngine.h"
#include <chrono>
#include <iostream>
#include <sstream>

#ifdef USE_SNDFILE
#include <sndfile.h>
#endif

#ifdef USE_PORTAUDIO
#include <portaudio.h>
#endif

namespace Periculium {

struct AudioEngine::Impl {
#ifdef USE_PORTAUDIO
    PaStream* stream = nullptr;
    AudioCallback callback;
    bool capturing = false;
#endif
};

AudioEngine::AudioEngine() : m_impl(std::make_unique<Impl>()) {
#ifdef USE_PORTAUDIO
    Pa_Initialize();
#endif
}

AudioEngine::~AudioEngine() {
    stopCapture();
#ifdef USE_PORTAUDIO
    Pa_Terminate();
#endif
}

AudioBuffer AudioEngine::loadFile(const std::string& path) {
    AudioBuffer result;
    
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &sfInfo);
    if (!file) {
        return result; // Return empty buffer on error
    }
    
    result.sampleRate = sfInfo.samplerate;
    result.channels = sfInfo.channels;
    result.samples.resize(sfInfo.frames * sfInfo.channels);
    
    sf_readf_float(file, result.samples.data(), sfInfo.frames);
    sf_close(file);
    
    return result;
}

AudioBuffer AudioEngine::loadFromVideo(const std::string& path) {
    // For now, return empty - FFmpeg integration to be added
    // This will require FFmpeg libraries and subprocess execution
    return AudioBuffer();
}

AudioBuffer AudioEngine::resample(const AudioBuffer& input, double targetSr) {
    if (input.sampleRate == targetSr || input.isEmpty()) {
        return input;
    }
    
    // Simple linear resampling (to be replaced with high-quality resampler)
    double ratio = targetSr / input.sampleRate;
    size_t outputFrames = static_cast<size_t>(input.frameCount() * ratio);
    
    AudioBuffer output(outputFrames * input.channels, targetSr, input.channels);
    
    for (size_t i = 0; i < outputFrames; i++) {
        double srcPos = i / ratio;
        size_t srcIdx = static_cast<size_t>(srcPos);
        double frac = srcPos - srcIdx;
        
        for (int c = 0; c < input.channels; c++) {
            size_t idx = srcIdx * input.channels + c;
            size_t nextIdx = (srcIdx + 1) * input.channels + c;
            
            float a = (idx < input.samples.size()) ? input.samples[idx] : 0.0f;
            float b = (nextIdx < input.samples.size()) ? input.samples[nextIdx] : 0.0f;
            
            output.samples[i * input.channels + c] = a * (1.0f - static_cast<float>(frac)) + b * static_cast<float>(frac);
        }
    }
    
    return output;
}

std::vector<AudioEngine::DeviceInfo> AudioEngine::getInputDevices() {
    std::vector<DeviceInfo> devices;
    
#ifdef USE_PORTAUDIO
    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) return devices;
    
    for (int i = 0; i < numDevices; i++) {
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
        if (info && info->maxInputChannels > 0) {
            DeviceInfo dev;
            dev.id = i;
            dev.name = info->name ? info->name : "Unknown";
            dev.maxChannels = info->maxInputChannels;
            dev.defaultSampleRate = info->defaultSampleRate;
            devices.push_back(dev);
        }
    }
#endif
    
    return devices;
}

#ifdef USE_PORTAUDIO
static int audioCallback(const void* inputBuffer, void* outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void* userData) {
    auto* impl = static_cast<AudioEngine::Impl*>(userData);
    if (impl->callback && inputBuffer) {
        impl->callback(static_cast<const float*>(inputBuffer), framesPerBuffer, 
                      timeInfo->currentTime);
    }
    return paContinue;
}
#endif

bool AudioEngine::startCapture(int deviceId, double sampleRate, size_t bufferSize, AudioCallback callback) {
#ifdef USE_PORTAUDIO
    stopCapture();
    
    m_impl->callback = callback;
    
    PaStreamParameters params;
    params.device = deviceId;
    params.channelCount = 1; // Mono for now
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(deviceId)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;
    
    PaError err = Pa_OpenStream(&m_impl->stream, &params, nullptr,
                                 sampleRate, bufferSize, paClipOff,
                                 audioCallback, m_impl.get());
    
    if (err != paNoError) return false;
    
    err = Pa_StartStream(m_impl->stream);
    if (err != paNoError) {
        Pa_CloseStream(m_impl->stream);
        m_impl->stream = nullptr;
        return false;
    }
    
    m_impl->capturing = true;
    return true;
#else
    return false;
#endif
}

void AudioEngine::stopCapture() {
#ifdef USE_PORTAUDIO
    if (m_impl->stream) {
        Pa_StopStream(m_impl->stream);
        Pa_CloseStream(m_impl->stream);
        m_impl->stream = nullptr;
    }
    m_impl->capturing = false;
#endif
}

bool AudioEngine::isCapturing() const {
#ifdef USE_PORTAUDIO
    return m_impl->capturing;
#else
    return false;
#endif
}

AudioEngine::FileInfo AudioEngine::getFileInfo(const std::string& path) {
    FileInfo info;
    
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* file = sf_open(path.c_str(), SFM_READ, &sfInfo);
    if (file) {
        info.duration = sfInfo.frames / static_cast<double>(sfInfo.samplerate);
        info.sampleRate = sfInfo.samplerate;
        info.channels = sfInfo.channels;
        info.bitDepth = 16; // Simplified
        info.format = "WAV/FLAC";
        sf_close(file);
    }
    
    return info;
}

bool AudioEngine::isFFmpegAvailable() {
    // Check if FFmpeg is in PATH
    return false; // To be implemented
}

} // namespace Periculium
