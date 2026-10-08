#pragma once

#include <Geode/Geode.hpp>
#include <Geode/fmod/fmod.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>

class AudioManager {
private:
    static AudioManager* s_instance;

    FMOD::DSP* m_fftDSP = nullptr;
    float m_pulse = 0.0f;
    float m_bass = 0.0f;
    float m_mids = 0.0f;
    float m_time = 0.0f;
    float m_bpmClock = 0.0f;
    float m_energyAverage = 0.05f;
    float m_lastPeakTime = 0.0f;
    bool m_attached = false;

    AudioManager() = default;

public:
    static AudioManager* get();

    void attach();
    void detach();
    void update(float dt);

    float getPulse() const { return m_pulse; }
    float getBass() const { return m_bass; }
    float getMids() const { return m_mids; }
    float getTime() const { return m_time; }
    bool isAttached() const { return m_attached; }
};
