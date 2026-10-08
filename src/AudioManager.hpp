#pragma once

#include <Geode/Geode.hpp>
#include <Geode/fmod/fmod.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>

class AudioManager {
private:
    static AudioManager* s_instance;

    FMOD::DSP* m_fftDSP = nullptr;
    float m_pulse = 0.0f;
    float m_kickPulse = 0.0f;
    float m_subBass = 0.0f;
    float m_bass = 0.0f;
    float m_mids = 0.0f;
    float m_treble = 0.0f;
    float m_prevSubBass = 0.0f;
    float m_time = 0.0f;
    float m_bpmClock = 0.0f;
    float m_energyAverage = 0.01f;
    float m_energyVariance = 0.005f;
    float m_maxObservedEnergy = 0.05f;
    float m_lastPeakTime = 0.0f;
    float m_shakeOffset = 0.0f;
    bool m_attached = false;

    AudioManager() = default;

public:
    static AudioManager* get();

    void attach();
    void detach();
    void update(float dt);

    float getPulse() const { return m_pulse; }
    float getKickPulse() const { return m_kickPulse; }
    float getSubBass() const { return m_subBass; }
    float getBass() const { return m_bass; }
    float getMids() const { return m_mids; }
    float getTreble() const { return m_treble; }
    float getTime() const { return m_time; }
    float getShakeOffset() const { return m_shakeOffset; }
    bool isAttached() const { return m_attached; }
};
