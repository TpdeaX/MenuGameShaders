#include "ShaderManager.hpp"
#include "AudioManager.hpp"

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

    m_compiled = true;
    log::info("MenuGameShaders: Advanced shaders compiled and linked successfully");
    return true;
}

void ShaderManager::setUniforms(cocos2d::CCGLProgram* program, cocos2d::CCSize const& winSize) {
    if (!program) return;

    auto audio = AudioManager::get();

    int style = static_cast<int>(Mod::get()->getSettingValue<int64_t>("shader-style"));
    int colorMode = static_cast<int>(Mod::get()->getSettingValue<int64_t>("color-tint-mode"));
    float intensity = static_cast<float>(Mod::get()->getSettingValue<double>("pulse-intensity"));
    float distortion = static_cast<float>(Mod::get()->getSettingValue<double>("distortion-strength"));
    bool chromatic = Mod::get()->getSettingValue<bool>("chromatic-aberration");
    bool flashEnabled = Mod::get()->getSettingValue<bool>("beat-flash");
    auto customCol = Mod::get()->getSettingValue<cocos2d::ccColor3B>("custom-color");

    if (m_uResolution != -1) {
        program->setUniformLocationWith2f(m_uResolution, winSize.width, winSize.height);
    }
    if (m_uTime != -1) {
        program->setUniformLocationWith1f(m_uTime, audio->getTime());
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
        auto mousePos = cocos::getMousePos();
        float normX = std::clamp(mousePos.x / std::max(winSize.width, 1.0f), 0.0f, 1.0f);
        float normY = std::clamp(mousePos.y / std::max(winSize.height, 1.0f), 0.0f, 1.0f);
        program->setUniformLocationWith2f(m_uMouse, normX, normY);
    }
}
