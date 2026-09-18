#include "OboeAudioDriver.hpp"
#include <android/log.h>

#define TAG "OboeAudioDriver"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

OboeAudioDriver::OboeAudioDriver() {
    mEngineHandle = polyrhythm_create(mSampleRate);
}

OboeAudioDriver::~OboeAudioDriver() {
    stop();
    if (mEngineHandle != nullptr) {
        polyrhythm_destroy(mEngineHandle);
        mEngineHandle = nullptr;
    }
}

bool OboeAudioDriver::start(uint32_t sampleRate, int32_t /*framesPerBurst*/) {
    if (mIsRunning.load()) return true;

    mSampleRate = sampleRate;
    if (mEngineHandle != nullptr) {
        polyrhythm_destroy(mEngineHandle);
    }
    mEngineHandle = polyrhythm_create(mSampleRate);
    polyrhythm_start(mEngineHandle);
    mIsRunning.store(true);
    return true;
}

void OboeAudioDriver::stop() {
    if (!mIsRunning.load()) return;
    if (mEngineHandle != nullptr) {
        polyrhythm_stop(mEngineHandle);
    }
    mIsRunning.store(false);
}

void OboeAudioDriver::renderAudio(float* audioData, int32_t numFrames) {
    if (!mIsRunning.load() || mEngineHandle == nullptr || audioData == nullptr) {
        return;
    }
    // C++20 Core render çağrısı (Real-time safe, non-blocking)
    polyrhythm_render(mEngineHandle, audioData, static_cast<size_t>(numFrames));
}
