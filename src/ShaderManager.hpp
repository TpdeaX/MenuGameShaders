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
    GLint m_uIntensity = -1;
    GLint m_uDistortion = -1;
    GLint m_uStyle = -1;
    GLint m_uColorMode = -1;
    GLint m_uCustomColor = -1;
    GLint m_uChromatic = -1;

    ShaderManager() = default;

public:
    static ShaderManager* get();

    bool compile();
    void setUniforms(cocos2d::CCGLProgram* program, cocos2d::CCSize const& winSize);

    cocos2d::CCGLProgram* getProgram() {
        if (!m_compiled) {
            this->compile();
        }
        return m_program;
    }
};
