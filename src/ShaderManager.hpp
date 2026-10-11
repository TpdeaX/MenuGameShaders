#pragma once

#include <Geode/Geode.hpp>
#include "Shaders.hpp"

class ShaderManager {
private:
    static ShaderManager* s_instance;

    cocos2d::CCGLProgram* m_program = nullptr;
    bool m_compiled = false;

    GLint m_uResolution = -1;
    GLint m_uTime = -1;
    GLint m_uPulse = -1;
    GLint m_uBass = -1;
    GLint m_uMids = -1;
    GLint m_uTreble = -1;
    GLint m_uIntensity = -1;
    GLint m_uDistortion = -1;
    GLint m_uStyle = -1;
    GLint m_uColorMode = -1;
    GLint m_uCustomColor = -1;
    GLint m_uChromatic = -1;
    GLint m_uBeatFlash = -1;
    GLint m_uMouse = -1;
    GLint m_uShockwaveAmbient = -1;
    GLint m_uCustomShape = -1;
    GLint m_uCustomColor1 = -1;
    GLint m_uCustomColor2 = -1;
    GLint m_uCustomSpeed = -1;
    GLint m_uCustomScale = -1;

    int m_randomStyle = 0;

    ShaderManager() = default;

public:
    static ShaderManager* get();

    bool compile();
    void setUniforms(cocos2d::CCGLProgram* program, cocos2d::CCSize const& winSize);

    // Picks a new random shader style (used by the shader randomizer).
    void rerollRandomStyle();

    // Returns the shader-style setting, or a random one when randomization is on.
    int getEffectiveStyle() const;

    cocos2d::CCGLProgram* getProgram() {
        if (!m_compiled) {
            this->compile();
        }
        return m_program;
    }
};
