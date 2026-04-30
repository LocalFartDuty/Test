#include "TranscriptionEngine.h"
#include <chrono>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Periculium {

struct TranscriptionEngine::Impl {
    std::string pythonPath = "python";
    std::string transkunPath = "../Transkun_Package";
    std::string device = "cuda";
};

TranscriptionEngine::TranscriptionEngine() : m_impl(std::make_unique<Impl>()) {}

TranscriptionEngine::~TranscriptionEngine() = default;

void TranscriptionEngine::setPythonPath(const std::string& pythonPath) {
    m_impl->pythonPath = pythonPath;
}

void TranscriptionEngine::setTranskunPath(const std::string& transkunPath) {
    m_impl->transkunPath = transkunPath;
}

void TranscriptionEngine::setDevice(const std::string& device) {
    m_impl->device = device;
}

std::string TranscriptionEngine::getDevice() const {
    return m_impl->device;
}

TranscriptionResult TranscriptionEngine::transcribeFile(const std::string& audioPath,
                                                       const std::string& outputPath,
                                                       bool useGPU,
                                                       bool useFP16,
                                                       bool verbose) {
    TranscriptionResult result;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Build command to call transcribe_stable.py
    std::ostringstream cmd;
    cmd << m_impl->pythonPath << " -m transkun.transcribe_stable";
    cmd << " \"" << audioPath << "\"";
    cmd << " \"" << outputPath << "\"";
    cmd << " --device " << (useGPU ? "cuda" : "cpu");
    if (useFP16 && useGPU) {
        cmd << " --fp16";
    }
    if (verbose) {
        cmd << " -v";
    }
    
    // Set PYTHONPATH
    std::string pythonPathEnv = m_impl->transkunPath;
    
#ifdef _WIN32
    _putenv_s("PYTHONPATH", pythonPathEnv.c_str());
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    std::string cmdStr = cmd.str();
    
    if (!CreateProcessA(NULL, const_cast<char*>(cmdStr.c_str()), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start transcription process" << std::endl;
        return result;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    if (exitCode != 0) {
        std::cerr << "Transcription failed with exit code: " << exitCode << std::endl;
        return result;
    }
#else
    setenv("PYTHONPATH", pythonPathEnv.c_str(), 1);
    
    std::string cmdStr = cmd.str();
    int exitCode = system(cmdStr.c_str());
    
    if (exitCode != 0) {
        std::cerr << "Transcription failed with exit code: " << exitCode << std::endl;
        return result;
    }
#endif
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTime = std::chrono::duration<double>(endTime - startTime).count();
    
    result.audioDuration = result.processingTime * 20; // Placeholder
    
    if (result.audioDuration > 0) {
        result.realtimeFactor = result.audioDuration / result.processingTime;
    }
    
    if (verbose) {
        std::cout << "Transcription completed in " << result.processingTime << "s" << std::endl;
        std::cout << "Output saved to: " << outputPath << std::endl;
    }
    
    return result;
}

TranscriptionResult TranscriptionEngine::transcribeFileBeta(const std::string& audioPath,
                                                           const std::string& outputPath,
                                                           bool useGPU,
                                                           bool useFP16,
                                                           bool verbose) {
    TranscriptionResult result;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::ostringstream cmd;
    cmd << m_impl->pythonPath << " -m transkun.transcribe_beta";
    cmd << " \"" << audioPath << "\"";
    cmd << " \"" << outputPath << "\"";
    cmd << " --device " << (useGPU ? "cuda" : "cpu");
    if (useFP16 && useGPU) {
        cmd << " --fp16";
    }
    if (verbose) {
        cmd << " -v";
    }
    
    std::string pythonPathEnv = m_impl->transkunPath;
    
#ifdef _WIN32
    _putenv_s("PYTHONPATH", pythonPathEnv.c_str());
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    std::string cmdStr = cmd.str();
    
    if (!CreateProcessA(NULL, const_cast<char*>(cmdStr.c_str()), NULL, NULL, FALSE,
                        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start transcription process" << std::endl;
        return result;
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    if (exitCode != 0) {
        std::cerr << "BETA transcription failed with exit code: " << exitCode << std::endl;
        return result;
    }
#else
    setenv("PYTHONPATH", pythonPathEnv.c_str(), 1);
    
    std::string cmdStr = cmd.str();
    int exitCode = system(cmdStr.c_str());
    
    if (exitCode != 0) {
        std::cerr << "BETA transcription failed with exit code: " << exitCode << std::endl;
        return result;
    }
#endif
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.processingTime = std::chrono::duration<double>(endTime - startTime).count();
    
    result.audioDuration = result.processingTime * 20;
    
    if (result.audioDuration > 0) {
        result.realtimeFactor = result.audioDuration / result.processingTime;
    }
    
    if (verbose) {
        std::cout << "BETA transcription completed in " << result.processingTime << "s" << std::endl;
        std::cout << "Output saved to: " << outputPath << std::endl;
    }
    
    return result;
}

} // namespace Periculium
