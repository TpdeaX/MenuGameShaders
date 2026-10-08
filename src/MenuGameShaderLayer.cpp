#include "MenuGameShaderLayer.hpp"

using namespace geode::prelude;

bool MenuGameShaderLayer::init() {
    if (!MenuGameLayer::init()) {
        return false;
    }

    AudioManager::get()->attach();
    ShaderManager::get()->compile();

    return true;
}

void MenuGameShaderLayer::onExit() {
    MenuGameLayer::onExit();
    AudioManager::get()->detach();
}

void MenuGameShaderLayer::visit() {
    bool enabled = Mod::get()->getSettingValue<bool>("enabled");

    if (!enabled || m_fields->m_isRendering) {
        MenuGameLayer::visit();
        return;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    if (winSize.width <= 0 || winSize.height <= 0) {
        MenuGameLayer::visit();
        return;
    }

    // Recreate render target texture on screen resize or first frame
    if (!m_fields->m_renderTexture || m_fields->m_renderSize != winSize) {
        m_fields->m_renderSize = winSize;
        m_fields->m_renderTexture = CCRenderTexture::create(winSize.width, winSize.height);
    }

    if (!m_fields->m_renderTexture) {
        MenuGameLayer::visit();
        return;
    }

    auto program = ShaderManager::get()->getProgram();
    if (!program) {
        MenuGameLayer::visit();
        return;
    }

    // Advance live audio analysis and beat rhythm
    float dt = CCDirector::sharedDirector()->getDeltaTime();
    AudioManager::get()->update(dt);

    // 1. Render all MenuGameLayer children into the offscreen texture
    m_fields->m_isRendering = true;
    m_fields->m_renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 0.0f);
    MenuGameLayer::visit();
    m_fields->m_renderTexture->end();
    m_fields->m_isRendering = false;

    // 2. Draw the rendered texture using the custom post-processing shader
    this->drawShadedQuad(winSize);
}

void MenuGameShaderLayer::drawShadedQuad(cocos2d::CCSize const& winSize) {
    auto program = ShaderManager::get()->getProgram();
    if (!program || !m_fields->m_renderTexture) {
        return;
    }

    program->use();
    program->setUniformsForBuiltins();

    auto sprite = m_fields->m_renderTexture->getSprite();
    if (!sprite || !sprite->getTexture()) {
        return;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sprite->getTexture()->getName());

    ShaderManager::get()->setUniforms(program, winSize);

    // Quad covering the full viewport
    GLfloat vertices[] = {
        0.0f,          0.0f,           0.0f,
        winSize.width, 0.0f,           0.0f,
        0.0f,          winSize.height, 0.0f,
        winSize.width, winSize.height, 0.0f
    };

    // Texture UV mapping matching CCRenderTexture's OpenGL orientation
    GLfloat texCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords);
    glVertexAttribPointer(kCCVertexAttrib_Position, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
}
