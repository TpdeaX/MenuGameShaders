#include "AudioManager.hpp"
#include <algorithm>

using namespace geode::prelude;

AudioManager* AudioManager::s_instance = nullptr;

AudioManager* AudioManager::get() {
    if (!s_instance) {
        s_instance = new AudioManager();
    }
    return s_instance;
}

void AudioManager::attach() {
    if (m_attached) {
        return;
    }

    auto engine = FMODAudioEngine::sharedEngine();
    if (!engine || !engine->m_system || !engine->m_backgroundMusicChannel) {
        return;
    }

    if (!m_fftDSP) {
        FMOD_RESULT res = engine->m_system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fftDSP);
        if (res == FMOD_OK && m_fftDSP) {
            engine->m_backgroundMusicChannel->addDSP(0, m_fftDSP);
            m_attached = true;
            log::info("MenuGameShaders: Attached FMOD FFT DSP to background music channel");
        } else {
            log::warn("MenuGameShaders: Could not create FMOD FFT DSP (error code: {})", static_cast<int>(res));
        }
    }
}

void AudioManager::detach() {
    if (!m_attached && !m_fftDSP) {
        return;
    }

    auto engine = FMODAudioEngine::sharedEngine();
    if (engine && engine->m_backgroundMusicChannel && m_fftDSP) {
        engine->m_backgroundMusicChannel->removeDSP(m_fftDSP);
    }

    if (m_fftDSP) {
        m_fftDSP->release();
        m_fftDSP = nullptr;
    }

    m_attached = false;
    log::info("MenuGameShaders: Detached and released FMOD FFT DSP");
}

void AudioManager::update(float dt) {
    float speed = static_cast<float>(Mod::get()->getSettingValue<double>("speed"));
    m_time += dt * speed;

    bool audioReactive = Mod::get()->getSettingValue<bool>("audio-reactive");

    // Try to attach if not yet attached
    if (audioReactive && !m_attached) {
        this->attach();
    }

    float currentBass = 0.0f;
    float currentMids = 0.0f;

    if (audioReactive && m_attached && m_fftDSP) {
        FMOD_DSP_PARAMETER_FFT* fft = nullptr;
        FMOD_RESULT res = m_fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, reinterpret_cast<void**>(&fft), nullptr, nullptr, 0);

        if (res == FMOD_OK && fft && fft->length > 0 && fft->numchannels > 0 && fft->spectrum[0]) {
            // Bass: low frequency bins (0 to 15, approx. 20Hz - 250Hz)
            int bassBins = std::min(16, fft->length);
            for (int i = 0; i < bassBins; ++i) {
                currentBass += fft->spectrum[0][i];
            }
            currentBass /= static_cast<float>(bassBins);

            // Mids: middle frequency bins (16 to 64, approx. 250Hz - 2000Hz)
            int midBins = std::min(64, fft->length);
            for (int i = bassBins; i < midBins; ++i) {
                currentMids += fft->spectrum[0][i];
            }
            currentMids /= static_cast<float>(midBins - bassBins);
        }
    }

    // Smooth live audio levels
    m_bass = m_bass * 0.7f + currentBass * 0.3f;
    m_mids = m_mids * 0.7f + currentMids * 0.3f;

    // Moving average of bass energy for dynamic beat threshold
    m_energyAverage = m_energyAverage * 0.95f + m_bass * 0.05f;

    // Detect sudden audio energy transient (kick / beat)
    float pulseMultiplier = static_cast<float>(Mod::get()->getSettingValue<double>("pulse-intensity"));
    bool detectedBeat = false;

    if (audioReactive && m_bass > (m_energyAverage * 1.35f + 0.02f) && (m_time - m_lastPeakTime) > 0.16f) {
        m_pulse = std::min(1.0f, m_pulse + 0.9f * pulseMultiplier);
        m_lastPeakTime = m_time;
        detectedBeat = true;
    }

    // BPM Fallback / Rhythm sync clock
    float bpm = static_cast<float>(Mod::get()->getSettingValue<double>("bpm"));
    float bps = std::clamp(bpm / 60.0f, 0.5f, 5.0f);
    m_bpmClock += dt * bps;

    if (m_bpmClock >= 1.0f) {
        m_bpmClock -= 1.0f;
        // If live beat wasn't just triggered, trigger rhythmic BPM pulse
        if (!detectedBeat && (m_time - m_lastPeakTime) > 0.25f) {
            m_pulse = std::max(m_pulse, 0.75f * pulseMultiplier);
            m_lastPeakTime = m_time;
        }
    }

    // Smooth exponential decay
    m_pulse = std::max(0.0f, m_pulse - dt * 4.2f);
}
