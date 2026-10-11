#pragma once

#include <atomic>

// Captures the OS "system audio" (whatever the default playback device is
// currently outputting) via WASAPI loopback on Windows, runs an FFT over the
// mono mix, and exposes band energies for the beat engine.
//
// On non-Windows platforms every method is a safe no-op so the rest of the mod
// keeps compiling; FMOD remains the audio source there.
class SystemAudioCapture {
public:
    struct Impl;

    static SystemAudioCapture* get();

    // Returns true if a capture thread was (re)started.
    bool start();
    void stop();
    bool isRunning() const;

    float getSubBass() const;
    float getBass() const;
    float getMids() const;
    float getTreble() const;
    bool hasSignal() const;

private:
    static SystemAudioCapture* s_instance;
    Impl* m_impl = nullptr;
};
