#include "AudioManager.hpp"
#include <algorithm>
#include <cmath>

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

    if (audioReactive && !m_attached) {
        this->attach();
    }

    float rawSubBass = 0.0f;
    float rawBass = 0.0f;
    float rawMids = 0.0f;
    float rawTreble = 0.0f;
    bool hasAudioSignal = false;

    if (audioReactive && m_attached && m_fftDSP) {
        FMOD_DSP_PARAMETER_FFT* fft = nullptr;
        FMOD_RESULT res = m_fftDSP->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, reinterpret_cast<void**>(&fft), nullptr, nullptr, 0);

        if (res == FMOD_OK && fft && fft->length > 0 && fft->numchannels > 0 && fft->spectrum[0]) {
            int len = fft->length;

            // 1. Sub-Bass & Kick Punch (Bins 1 to 5: ~20Hz to 110Hz)
            int subEnd = std::min(6, len);
            for (int i = 1; i < subEnd; ++i) {
                rawSubBass += fft->spectrum[0][i];
            }
            rawSubBass /= static_cast<float>(std::max(1, subEnd - 1));

            // 2. Bassline & Low Mids (Bins 6 to 18: ~110Hz to 400Hz)
            int bassEnd = std::min(19, len);
            for (int i = subEnd; i < bassEnd; ++i) {
                rawBass += fft->spectrum[0][i];
            }
            rawBass /= static_cast<float>(std::max(1, bassEnd - subEnd));

            // 3. Mids / Snare / Vocals (Bins 19 to 65: ~400Hz to 1400Hz)
            int midEnd = std::min(66, len);
            for (int i = bassEnd; i < midEnd; ++i) {
                rawMids += fft->spectrum[0][i];
            }
            rawMids /= static_cast<float>(std::max(1, midEnd - bassEnd));

            // 4. Treble / Hi-Hats / Cymbals (Bins 66 to 180: ~1400Hz to 4000Hz)
            int trebEnd = std::min(180, len);
            for (int i = midEnd; i < trebEnd; ++i) {
                rawTreble += fft->spectrum[0][i];
            }
            rawTreble /= static_cast<float>(std::max(1, trebEnd - midEnd));

            hasAudioSignal = (rawSubBass + rawBass + rawMids) > 0.0001f;
        }
    }

    // Adaptive Peak Normalizer / Auto-Gain
    m_maxObservedEnergy = std::max(0.01f, std::max(m_maxObservedEnergy * 0.995f, rawSubBass * 1.2f));

    // Normalize bands using dynamic scaling
    float normFactor = 1.0f / m_maxObservedEnergy;
    float normSubBass = std::clamp(rawSubBass * normFactor, 0.0f, 1.5f);
    float normBass = std::clamp(rawBass * normFactor * 1.2f, 0.0f, 1.5f);
    float normMids = std::clamp(rawMids * normFactor * 1.5f, 0.0f, 1.5f);
    float normTreble = std::clamp(rawTreble * normFactor * 2.0f, 0.0f, 1.5f);

    // Fast-attack, smooth-decay filter for continuous fluid vibration
    m_subBass = m_subBass * 0.5f + normSubBass * 0.5f;
    m_bass = m_bass * 0.6f + normBass * 0.4f;
    m_mids = m_mids * 0.6f + normMids * 0.4f;
    m_treble = m_treble * 0.7f + normTreble * 0.3f;

    // --- High-Fidelity Beat Transient Detection (Spectral Flux) ---
    float pulseMult = static_cast<float>(Mod::get()->getSettingValue<double>("pulse-intensity"));
    float sensitivity = static_cast<float>(Mod::get()->getSettingValue<double>("beat-sensitivity"));

    // Positive flux (sudden energy increase on kick)
    float flux = std::max(0.0f, normSubBass - m_prevSubBass);
    m_prevSubBass = normSubBass;

    // Rolling average & variance for dynamic kick threshold
    m_energyAverage = m_energyAverage * 0.92f + normSubBass * 0.08f;
    float diff = std::abs(normSubBass - m_energyAverage);
    m_energyVariance = m_energyVariance * 0.92f + diff * 0.08f;

    float threshold = m_energyAverage + m_energyVariance * (1.6f / std::max(0.2f, sensitivity));
    bool detectedKick = false;

    // Kick trigger: significant energy increase + refractory window (min 130ms between kicks)
    if (hasAudioSignal && flux > (0.12f / sensitivity) && normSubBass > threshold && (m_time - m_lastPeakTime) > 0.13f) {
        m_kickPulse = 1.0f * pulseMult;
        m_lastPeakTime = m_time;
        detectedKick = true;
    }

    // --- BPM Fallback / Clock Sync (keeps rhythm if music is quiet) ---
    float bpm = static_cast<float>(Mod::get()->getSettingValue<double>("bpm"));
    float bps = std::clamp(bpm / 60.0f, 0.5f, 5.0f);
    m_bpmClock += dt * bps;

    if (m_bpmClock >= 1.0f) {
        m_bpmClock -= 1.0f;
        if (!detectedKick && !hasAudioSignal && (m_time - m_lastPeakTime) > 0.22f) {
            m_kickPulse = 0.85f * pulseMult;
            m_lastPeakTime = m_time;
        }
    }

    // Exponential snappy decay for kick punch
    m_kickPulse = std::max(0.0f, m_kickPulse - dt * 6.2f);

    // Combined pulse: Snappy punch + Continuous audio groove
    m_pulse = std::clamp(m_kickPulse * 0.75f + m_subBass * 0.35f, 0.0f, 2.0f);

    // Bass screen shake computation
    bool shakeEnabled = Mod::get()->getSettingValue<bool>("bass-shake");
    if (shakeEnabled && m_kickPulse > 0.3f) {
        float shakeMag = (m_kickPulse - 0.3f) * 4.5f * pulseMult;
        m_shakeOffset = (static_cast<float>(rand() % 100) / 50.0f - 1.0f) * shakeMag;
    } else {
        m_shakeOffset = 0.0f;
    }
}
