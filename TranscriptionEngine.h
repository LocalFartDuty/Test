#pragma once
#include "Types.h"
#include <string>
#include <memory>

namespace Periculium {

// ---- Transcription Engine (Python Subprocess) ----
// Calls the Python Transkun model via subprocess
// This is the fastest path to a working C++ application since ONNX export is not possible

class TranscriptionEngine {
public:
    TranscriptionEngine();
    ~TranscriptionEngine();

    // ---- Initialization ----
    
    // Set Python executable path
    void setPythonPath(const std::string& pythonPath);
    
    // Set Transkun package path
    void setTranskunPath(const std::string& transkunPath);

    // ---- Transcription ----
    
    // Transcribe audio file (calls transcribe_stable.py)
    TranscriptionResult transcribeFile(const std::string& audioPath, 
                                       const std::string& outputPath,
                                       bool useGPU = true,
                                       bool useFP16 = true,
                                       bool verbose = false);
    
    // Transcribe with BETA mode (calls transcribe_beta.py)
    TranscriptionResult transcribeFileBeta(const std::string& audioPath,
                                          const std::string& outputPath,
                                          bool useGPU = true,
                                          bool useFP16 = true,
                                          bool verbose = false);

    // ---- Device Info ----
    
    // Set device (cuda/cpu)
    void setDevice(const std::string& device);
    std::string getDevice() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Periculium
