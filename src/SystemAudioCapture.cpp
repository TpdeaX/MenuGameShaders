#include "SystemAudioCapture.hpp"
#include <Geode/Geode.hpp>

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <cstdint>
#include <cmath>
#include <thread>
#include <vector>

// Local GUID definitions to avoid any extra link dependencies.
static const GUID kSubTypePCM = {
    0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x3B, 0x89, 0x71 }
};
static const GUID kSubTypeIEEEFloat = {
    0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x3B, 0x89, 0x71 }
};
static const GUID kClsidMMDeviceEnumerator = {
    0xBCDE0395, 0xE52F, 0x467E, { 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E }
};
static const GUID kIidIMMDeviceEnumerator = {
    0xA95664D2, 0x9614, 0x4F35, { 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 }
};
static const GUID kIidIAudioClient = {
    0x1CB9AD4C, 0xDBFA, 0x4C32, { 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 }
};
static const GUID kIidIAudioCaptureClient = {
    0xC8ADBD64, 0xE71E, 0x48A0, { 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17 }
};

constexpr float kPi = 3.14159265358979323846f;

struct SystemAudioCapture::Impl {
    std::atomic<bool> running{ false };
    std::atomic<bool> stopRequested{ false };

    std::atomic<float> subBass{ 0.0f };
    std::atomic<float> bass{ 0.0f };
    std::atomic<float> mids{ 0.0f };
    std::atomic<float> treble{ 0.0f };
    std::atomic<bool> hasSignal{ false };

    std::thread thread;
    long long lastAttemptMs = 0;

    int fftSize = 2048;
    int hopSize = 1024;
    int sampleRate = 0;
    int channels = 0;
    int bytesPerSample = 0;
    bool isFloat = false;

    std::vector<float> window;
    std::vector<float> buffer;
};

static void runFFT(std::vector<float>& re, std::vector<float>& im) {
    const int n = static_cast<int>(re.size());

    // Bit-reversal permutation.
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(re[i], re[j]);
            std::swap(im[i], im[j]);
        }
    }

    // Iterative radix-2 Cooley-Tukey.
    for (int len = 2; len <= n; len <<= 1) {
        const float ang = -2.0f * kPi / static_cast<float>(len);
        const float wr = std::cos(ang);
        const float wi = std::sin(ang);
        for (int i = 0; i < n; i += len) {
            float curRe = 1.0f;
            float curIm = 0.0f;
            for (int k = 0; k < len / 2; ++k) {
                const float uRe = re[i + k];
                const float uIm = im[i + k];
                const float vRe = re[i + k + len / 2] * curRe - im[i + k + len / 2] * curIm;
                const float vIm = re[i + k + len / 2] * curIm + im[i + k + len / 2] * curRe;
                re[i + k] = uRe + vRe;
                im[i + k] = uIm + vIm;
                re[i + k + len / 2] = uRe - vRe;
                im[i + k + len / 2] = uIm - vIm;

                const float nextRe = curRe * wr - curIm * wi;
                curIm = curRe * wi + curIm * wr;
                curRe = nextRe;
            }
        }
    }
}

static void analyzeSpectrum(SystemAudioCapture::Impl* impl) {
    const int n = impl->fftSize;
    const int n2 = n / 2;

    thread_local std::vector<float> re;
    thread_local std::vector<float> im;
    re.assign(n, 0.0f);
    im.assign(n, 0.0f);

    for (int i = 0; i < n; ++i) {
        re[i] = impl->buffer[i] * impl->window[i];
    }

    runFFT(re, im);

    // Normalize so a full-scale sine at a bin center maps to ~1.0.
    const float norm = 2.0f / static_cast<float>(n);

    float sub = 0.0f, bass = 0.0f, mids = 0.0f, treble = 0.0f;
    int subC = 0, bassC = 0, midsC = 0, trebC = 0;

    for (int i = 1; i < n2; ++i) {
        const float freq = static_cast<float>(i) * static_cast<float>(impl->sampleRate) / static_cast<float>(n);
        const float mag = std::sqrt(re[i] * re[i] + im[i] * im[i]) * norm;

        if (freq < 110.0f) {
            sub += mag;
            ++subC;
        } else if (freq < 400.0f) {
            bass += mag;
            ++bassC;
        } else if (freq < 1400.0f) {
            mids += mag;
            ++midsC;
        } else if (freq < 4000.0f) {
            treble += mag;
            ++trebC;
        }
    }

    if (subC > 0) impl->subBass.store(sub / static_cast<float>(subC));
    if (bassC > 0) impl->bass.store(bass / static_cast<float>(bassC));
    if (midsC > 0) impl->mids.store(mids / static_cast<float>(midsC));
    if (trebC > 0) impl->treble.store(treble / static_cast<float>(trebC));
    impl->hasSignal.store((sub + bass + mids) > 0.0001f);
}

static void pushSample(SystemAudioCapture::Impl* impl, float sample) {
    impl->buffer.push_back(sample);
    if (static_cast<int>(impl->buffer.size()) >= impl->fftSize) {
        analyzeSpectrum(impl);
        impl->buffer.erase(impl->buffer.begin(), impl->buffer.begin() + impl->hopSize);
    }
}

static void processFrames(SystemAudioCapture::Impl* impl, const unsigned char* data, unsigned int frames) {
    const int ch = impl->channels;

    if (impl->isFloat) {
        const float* p = reinterpret_cast<const float*>(data);
        for (unsigned int f = 0; f < frames; ++f) {
            float acc = 0.0f;
            for (int c = 0; c < ch; ++c) {
                acc += p[f * ch + c];
            }
            pushSample(impl, acc / static_cast<float>(ch));
        }
    } else {
        const std::int16_t* p = reinterpret_cast<const std::int16_t*>(data);
        for (unsigned int f = 0; f < frames; ++f) {
            float acc = 0.0f;
            for (int c = 0; c < ch; ++c) {
                acc += static_cast<float>(p[f * ch + c]) / 32768.0f;
            }
            pushSample(impl, acc / static_cast<float>(ch));
        }
    }
}

static void captureThread(SystemAudioCapture::Impl* impl) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        geode::log::error("MenuGameShaders: system audio CoInitializeEx failed (hr=0x{:X})", static_cast<unsigned long>(hr));
        impl->running.store(false);
        return;
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    WAVEFORMATEX* mixFormat = nullptr;
    bool started = false;

    hr = CoCreateInstance(
        kClsidMMDeviceEnumerator, nullptr, CLSCTX_ALL,
        kIidIMMDeviceEnumerator, reinterpret_cast<void**>(&enumerator));
    if (SUCCEEDED(hr) && !enumerator) {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    }
    if (SUCCEEDED(hr) && !device) {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        hr = device->Activate(kIidIAudioClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient));
    }
    if (SUCCEEDED(hr) && !audioClient) {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        hr = audioClient->GetMixFormat(&mixFormat);
    }

    if (SUCCEEDED(hr) && mixFormat) {
        impl->channels = mixFormat->nChannels;
        impl->sampleRate = mixFormat->nSamplesPerSec;
        impl->isFloat = false;
        impl->bytesPerSample = mixFormat->wBitsPerSample / 8;

        if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            impl->isFloat = true;
            impl->bytesPerSample = 4;
        } else if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            auto* ext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(mixFormat);
            if (IsEqualGUID(ext->SubFormat, kSubTypeIEEEFloat)) {
                impl->isFloat = true;
                impl->bytesPerSample = 4;
            } else if (IsEqualGUID(ext->SubFormat, kSubTypePCM)) {
                impl->isFloat = false;
            }
        }

        const bool supported = (impl->isFloat && impl->bytesPerSample == 4) ||
                               (!impl->isFloat && impl->bytesPerSample == 2);
        if (impl->sampleRate <= 0 || impl->channels <= 0 || !supported) {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr)) {
        hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, mixFormat, nullptr);
    }
    if (SUCCEEDED(hr)) {
        hr = audioClient->GetService(kIidIAudioCaptureClient, reinterpret_cast<void**>(&captureClient));
    }
    if (SUCCEEDED(hr) && !captureClient) {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr)) {
        hr = audioClient->Start();
        started = SUCCEEDED(hr);
    }

    if (!started) {
        geode::log::error("MenuGameShaders: system audio setup failed (hr=0x{:X})", static_cast<unsigned long>(hr));
        if (mixFormat) CoTaskMemFree(mixFormat);
        if (captureClient) captureClient->Release();
        if (audioClient) audioClient->Release();
        if (device) device->Release();
        if (enumerator) enumerator->Release();
        CoUninitialize();
        impl->running.store(false);
        return;
    }

    // Hann window.
    impl->window.resize(impl->fftSize);
    for (int i = 0; i < impl->fftSize; ++i) {
        impl->window[i] = 0.5f * (1.0f - std::cos(2.0f * kPi * static_cast<float>(i) / static_cast<float>(impl->fftSize - 1)));
    }
    impl->buffer.clear();
    impl->buffer.reserve(impl->fftSize);

    while (!impl->stopRequested.load()) {
        Sleep(10);

        UINT32 nextPacket = 0;
        if (FAILED(captureClient->GetNextPacketSize(&nextPacket))) {
            break;
        }

        while (nextPacket > 0 && !impl->stopRequested.load()) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;

            hr = captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (SUCCEEDED(hr)) {
                if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                    for (UINT32 f = 0; f < frames; ++f) {
                        pushSample(impl, 0.0f);
                    }
                } else {
                    processFrames(impl, data, frames);
                }
                captureClient->ReleaseBuffer(frames);
            }

            if (FAILED(captureClient->GetNextPacketSize(&nextPacket))) {
                break;
            }
        }
    }

    audioClient->Stop();

    if (mixFormat) CoTaskMemFree(mixFormat);
    captureClient->Release();
    audioClient->Release();
    device->Release();
    enumerator->Release();
    CoUninitialize();

    impl->running.store(false);
}

SystemAudioCapture* SystemAudioCapture::s_instance = nullptr;

SystemAudioCapture* SystemAudioCapture::get() {
    if (!s_instance) {
        s_instance = new SystemAudioCapture();
    }
    return s_instance;
}

bool SystemAudioCapture::start() {
    if (!m_impl) {
        m_impl = new Impl();
    }
    if (m_impl->running.load()) {
        return true;
    }

    // Cooldown so a device that keeps failing setup doesn't spawn a thread
    // every frame.
    const long long now = static_cast<long long>(GetTickCount64());
    if (now - m_impl->lastAttemptMs < 2000) {
        return false;
    }
    m_impl->lastAttemptMs = now;

    // Reap any previously-finished thread before reassigning. Assigning a new
    // thread to a still-joinable std::thread calls std::terminate() and would
    // crash Geometry Dash.
    if (m_impl->thread.joinable()) {
        m_impl->thread.join();
    }

    m_impl->stopRequested.store(false);
    m_impl->running.store(true);
    m_impl->thread = std::thread(&captureThread, m_impl);
    return true;
}

void SystemAudioCapture::stop() {
    if (!m_impl) {
        return;
    }
    m_impl->stopRequested.store(true);
    if (m_impl->thread.joinable()) {
        m_impl->thread.join();
    }
    m_impl->running.store(false);
}

bool SystemAudioCapture::isRunning() const {
    return m_impl && m_impl->running.load();
}

float SystemAudioCapture::getSubBass() const {
    return m_impl ? m_impl->subBass.load() : 0.0f;
}

float SystemAudioCapture::getBass() const {
    return m_impl ? m_impl->bass.load() : 0.0f;
}

float SystemAudioCapture::getMids() const {
    return m_impl ? m_impl->mids.load() : 0.0f;
}

float SystemAudioCapture::getTreble() const {
    return m_impl ? m_impl->treble.load() : 0.0f;
}

bool SystemAudioCapture::hasSignal() const {
    return m_impl && m_impl->hasSignal.load();
}

#else // _WIN32

SystemAudioCapture* SystemAudioCapture::s_instance = nullptr;

SystemAudioCapture* SystemAudioCapture::get() {
    if (!s_instance) {
        s_instance = new SystemAudioCapture();
    }
    return s_instance;
}

bool SystemAudioCapture::start() { return false; }
void SystemAudioCapture::stop() {}
bool SystemAudioCapture::isRunning() const { return false; }
float SystemAudioCapture::getSubBass() const { return 0.0f; }
float SystemAudioCapture::getBass() const { return 0.0f; }
float SystemAudioCapture::getMids() const { return 0.0f; }
float SystemAudioCapture::getTreble() const { return 0.0f; }
bool SystemAudioCapture::hasSignal() const { return false; }

#endif // _WIN32
