#pragma once

#include "c_bridge.h"
#include <memory>
#include <atomic>

// Oboe veya AAudio desteği için C++ driver sınıfı
class OboeAudioDriver {
public:
    OboeAudioDriver();
    ~OboeAudioDriver();

    bool start(uint32_t sampleRate = 48000, int32_t framesPerBurst = 192);
    void stop();
    bool isRunning() const { return mIsRunning.load(); }

    PolyrhythmEngineHandle getEngineHandle() const { return mEngineHandle; }

    // Oboe AudioStreamCallback onAudioReady eşdeğeri render fonksiyonu
    void renderAudio(float* audioData, int32_t numFrames);

private:
    PolyrhythmEngineHandle mEngineHandle{nullptr};
    std::atomic<bool> mIsRunning{false};
    uint32_t mSampleRate{48000};
};
