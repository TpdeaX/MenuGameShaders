#include "ShaderManager.hpp"
#include "AudioManager.hpp"
#include <random>

using namespace geode::prelude;

ShaderManager* ShaderManager::s_instance = nullptr;

ShaderManager* ShaderManager::get() {
    if (!s_instance) {
        s_instance = new ShaderManager();
    }
    return s_instance;
}

bool ShaderManager::compile() {
    if (m_compiled && m_program) {
        return true;
    }

    m_program = new cocos2d::CCGLProgram();
    if (!m_program->initWithVertexShaderByteArray(Shaders::VERTEX_SHADER, Shaders::FRAGMENT_SHADER)) {
        log::error("MenuGameShaders: Failed to initialize vertex / fragment shader");
        delete m_program;
        m_program = nullptr;
        return false;
    }

    m_program->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
    m_program->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
    m_program->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);

    m_program->link();
    m_program->updateUniforms();

    m_uResolution = m_program->getUniformLocationForName("u_resolution");
    m_uTime = m_program->getUniformLocationForName("u_time");
    m_uPulse = m_program->getUniformLocationForName("u_pulse");
    m_uBass = m_program->getUniformLocationForName("u_bass");
    m_uMids = m_program->getUniformLocationForName("u_mids");
    m_uTreble = m_program->getUniformLocationForName("u_treble");
    m_uIntensity = m_program->getUniformLocationForName("u_intensity");
    m_uDistortion = m_program->getUniformLocationForName("u_distortion");
    m_uStyle = m_program->getUniformLocationForName("u_style");
    m_uColorMode = m_program->getUniformLocationForName("u_colorMode");
    m_uCustomColor = m_program->getUniformLocationForName("u_customColor");
    m_uChromatic = m_program->getUniformLocationForName("u_chromatic");
    m_uBeatFlash = m_program->getUniformLocationForName("u_beatFlash");
    m_uMouse = m_program->getUniformLocationForName("u_mouse");
    m_uShockwaveAmbient = m_program->getUniformLocationForName("u_shockwaveAmbient");
    m_uCustomShape = m_program->getUniformLocationForName("u_customShape");
    m_uCustomColor1 = m_program->getUniformLocationForName("u_customColor1");
    m_uCustomColor2 = m_program->getUniformLocationForName("u_customColor2");
    m_uCustomSpeed = m_program->getUniformLocationForName("u_customSpeed");
    m_uCustomScale = m_program->getUniformLocationForName("u_customScale");

    m_compiled = true;
    log::info("MenuGameShaders: Advanced shaders compiled and linked successfully");
    return true;
}

void ShaderManager::setUniforms(cocos2d::CCGLProgram* program, cocos2d::CCSize const& winSize) {
    if (!program) return;

    auto audio = AudioManager::get();

    int style = getEffectiveStyle();
    int colorMode = static_cast<int>(Mod::get()->getSettingValue<int64_t>("color-tint-mode"));
    float intensity = static_cast<float>(Mod::get()->getSettingValue<double>("pulse-intensity"));
    float distortion = static_cast<float>(Mod::get()->getSettingValue<double>("distortion-strength"));
    float speed = static_cast<float>(Mod::get()->getSettingValue<double>("speed"));
    bool chromatic = Mod::get()->getSettingValue<bool>("chromatic-aberration");
    bool flashEnabled = Mod::get()->getSettingValue<bool>("beat-flash");
    auto customCol = Mod::get()->getSettingValue<cocos2d::ccColor3B>("custom-color");

    if (m_uResolution != -1) {
        program->setUniformLocationWith2f(m_uResolution, winSize.width, winSize.height);
    }
    if (m_uTime != -1) {
        program->setUniformLocationWith1f(m_uTime, audio->getTime() * speed);
    }
    if (m_uPulse != -1) {
        program->setUniformLocationWith1f(m_uPulse, audio->getPulse());
    }
    if (m_uBass != -1) {
        program->setUniformLocationWith1f(m_uBass, audio->getBass());
    }
    if (m_uMids != -1) {
        program->setUniformLocationWith1f(m_uMids, audio->getMids());
    }
    if (m_uTreble != -1) {
        program->setUniformLocationWith1f(m_uTreble, audio->getTreble());
    }
    if (m_uIntensity != -1) {
        program->setUniformLocationWith1f(m_uIntensity, intensity);
    }
    if (m_uDistortion != -1) {
        program->setUniformLocationWith1f(m_uDistortion, distortion);
    }
    if (m_uStyle != -1) {
        program->setUniformLocationWith1i(m_uStyle, style);
    }
    if (m_uColorMode != -1) {
        program->setUniformLocationWith1i(m_uColorMode, colorMode);
    }
    if (m_uCustomColor != -1) {
        program->setUniformLocationWith3f(
            m_uCustomColor,
            static_cast<float>(customCol.r) / 255.0f,
            static_cast<float>(customCol.g) / 255.0f,
            static_cast<float>(customCol.b) / 255.0f
        );
    }
    if (m_uChromatic != -1) {
        program->setUniformLocationWith1f(m_uChromatic, chromatic ? 1.0f : 0.0f);
    }
    if (m_uBeatFlash != -1) {
        float flashVal = flashEnabled ? audio->getKickPulse() * 0.35f : 0.0f;
        program->setUniformLocationWith1f(m_uBeatFlash, flashVal);
    }
    if (m_uMouse != -1) {
        bool cursorTracking = Mod::get()->getSettingValue<bool>("cursor-tracking");
        if (cursorTracking) {
            auto mousePos = cocos::getMousePos();
            auto director = cocos2d::CCDirector::sharedDirector();
            auto refSize = director ? director->getWinSize() : winSize;
            float normX = std::clamp(mousePos.x / std::max(refSize.width, 1.0f), 0.0f, 1.0f);
            float normY = std::clamp(mousePos.y / std::max(refSize.height, 1.0f), 0.0f, 1.0f);
            program->setUniformLocationWith2f(m_uMouse, normX, normY);
        } else {
            program->setUniformLocationWith2f(m_uMouse, 0.5f, 0.5f);
        }
    }
    if (m_uShockwaveAmbient != -1) {
        bool ambient = Mod::get()->getSettingValue<bool>("shockwave-ambient");
        program->setUniformLocationWith1f(m_uShockwaveAmbient, ambient ? 1.0f : 0.0f);
    }

    // Custom style (13) uniforms
    if (m_uCustomShape != -1) {
        program->setUniformLocationWith1i(m_uCustomShape, static_cast<int>(Mod::get()->getSettingValue<int64_t>("custom-shape")));
    }
    if (m_uCustomColor1 != -1) {
        auto c1 = Mod::get()->getSettingValue<cocos2d::ccColor3B>("custom-color1");
        program->setUniformLocationWith3f(m_uCustomColor1, static_cast<float>(c1.r) / 255.0f, static_cast<float>(c1.g) / 255.0f, static_cast<float>(c1.b) / 255.0f);
    }
    if (m_uCustomColor2 != -1) {
        auto c2 = Mod::get()->getSettingValue<cocos2d::ccColor3B>("custom-color2");
        program->setUniformLocationWith3f(m_uCustomColor2, static_cast<float>(c2.r) / 255.0f, static_cast<float>(c2.g) / 255.0f, static_cast<float>(c2.b) / 255.0f);
    }
    if (m_uCustomSpeed != -1) {
        program->setUniformLocationWith1f(m_uCustomSpeed, static_cast<float>(Mod::get()->getSettingValue<double>("custom-speed")));
    }
    if (m_uCustomScale != -1) {
        program->setUniformLocationWith1f(m_uCustomScale, static_cast<float>(Mod::get()->getSettingValue<double>("custom-scale")));
    }
}

void ShaderManager::rerollRandomStyle() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 13);
    m_randomStyle = dist(gen);
}

int ShaderManager::getEffectiveStyle() const {
    if (Mod::get()->getSettingValue<bool>("randomize-shaders")) {
        return m_randomStyle;
    }
    return static_cast<int>(Mod::get()->getSettingValue<int64_t>("shader-style"));
}
