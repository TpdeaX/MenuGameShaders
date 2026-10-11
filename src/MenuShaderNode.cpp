#include "MenuShaderNode.hpp"
#include "ShaderManager.hpp"
#include "AudioManager.hpp"
#include "SystemAudioCapture.hpp"
#include <cmath>

using namespace geode::prelude;

MenuShaderNode* MenuShaderNode::create(MenuGameLayer* gameLayer) {
    auto ret = new MenuShaderNode();
    if (ret && ret->init(gameLayer)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool MenuShaderNode::init(MenuGameLayer* gameLayer) {
    if (!CCNode::init()) {
        return false;
    }

    m_gameLayer = gameLayer;

    // Compile shader and attach audio
    ShaderManager::get()->compile();
    AudioManager::get()->attach();

    return true;
}

void MenuShaderNode::cleanupResources() {
    if (m_gameLayer) {
        m_gameLayer->setVisible(true);
    }
    m_renderTexture = nullptr;
    m_renderSize = cocos2d::CCSizeZero;
    m_frameSize = cocos2d::CCSizeZero;
    m_viewPortRect = cocos2d::CCRectZero;
}

void MenuShaderNode::onExit() {
    CCNode::onExit();
    this->cleanupResources();
    m_gameLayer = nullptr;
}

MenuShaderNode::~MenuShaderNode() {
    this->cleanupResources();
    m_gameLayer = nullptr;
}

void MenuShaderNode::visit() {
    bool enabled = Mod::get()->getSettingValue<bool>("enabled");

    if (!enabled || !m_gameLayer) {
        SystemAudioCapture::get()->stop();
        if (m_gameLayer) {
            m_gameLayer->setVisible(true);
        }
        CCNode::visit();
        return;
    }

    auto director = CCDirector::sharedDirector();
    if (!director) {
        if (m_gameLayer) m_gameLayer->setVisible(true);
        CCNode::visit();
        return;
    }

    auto winSize = director->getWinSize();
    if (winSize.width <= 0.0f || winSize.height <= 0.0f) {
        if (m_gameLayer) m_gameLayer->setVisible(true);
        CCNode::visit();
        return;
    }

    // Try to reacquire MenuGameLayer if it ever became detached
    if (!m_gameLayer->getParent()) {
        auto parent = this->getParent();
        if (parent) {
            auto gl = typeinfo_cast<MenuGameLayer*>(parent->getChildByID("main-menu-bg"));
            if (!gl && parent->getChildrenCount() > 0) {
                gl = typeinfo_cast<MenuGameLayer*>(parent->getChildren()->objectAtIndex(0));
            }
            if (gl) {
                m_gameLayer = gl;
            }
        }
    }

    auto eglView = CCEGLView::sharedOpenGLView();
    auto frameSize = eglView ? eglView->getFrameSize() : CCSizeZero;
    auto viewPortRect = eglView ? eglView->getViewPortRect() : CCRectZero;

    // Detect if window mode (fullscreen / windowed / borderless) or resolution changed
    bool geometryChanged = (m_renderSize.width != winSize.width) ||
                           (m_renderSize.height != winSize.height) ||
                           (m_frameSize.width != frameSize.width) ||
                           (m_frameSize.height != frameSize.height) ||
                           (m_viewPortRect.size.width != viewPortRect.size.width) ||
                           (m_viewPortRect.size.height != viewPortRect.size.height) ||
                           (m_viewPortRect.origin.x != viewPortRect.origin.x) ||
                           (m_viewPortRect.origin.y != viewPortRect.origin.y);

    // Verify OpenGL handles remain valid across possible context resets
    bool rtValid = false;
    if (m_renderTexture && !geometryChanged) {
        GLuint fbo = m_renderTexture->m_uFBO;
        auto sprite = m_renderTexture->getSprite();
        auto tex = sprite ? sprite->getTexture() : nullptr;
        GLuint texId = tex ? tex->getName() : 0;

        if (fbo != 0 && texId != 0 && glIsFramebuffer(fbo) && glIsTexture(texId)) {
            rtValid = true;
        }
    }

    // Recreate CCRenderTexture if size changed or OpenGL context reset
    if (!rtValid) {
        m_renderTexture = nullptr;
        m_renderSize = winSize;
        m_frameSize = frameSize;
        m_viewPortRect = viewPortRect;

        int rtWidth = static_cast<int>(std::ceil(winSize.width));
        int rtHeight = static_cast<int>(std::ceil(winSize.height));

        if (rtWidth > 0 && rtHeight > 0) {
            m_renderTexture = CCRenderTexture::create(rtWidth, rtHeight);
            if (m_renderTexture) {
                auto sprite = m_renderTexture->getSprite();
                if (sprite) {
                    sprite->setAnchorPoint({ 0.0f, 0.0f });
                }
            }
        }
    }

    if (!m_renderTexture) {
        m_gameLayer->setVisible(true);
        CCNode::visit();
        return;
    }

    auto program = ShaderManager::get()->getProgram();
    if (!program) {
        m_gameLayer->setVisible(true);
        CCNode::visit();
        return;
    }

    // Update live audio FFT analysis and rhythmic beat clock
    float dt = director->getDeltaTime();
    AudioManager::get()->update(dt);

    // 1. Temporarily show m_gameLayer to capture it into the offscreen FBO
    m_gameLayer->setVisible(true);

    m_renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 1.0f);

    // Verify FBO is complete in current OpenGL state
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        log::warn("MenuGameShaders: FBO incomplete (0x{:x}), recreating next frame", fboStatus);
        m_renderTexture->end();
        director->setViewport();
        m_renderTexture = nullptr;
        m_gameLayer->setVisible(true);
        CCNode::visit();
        return;
    }

    m_gameLayer->visit();
    m_renderTexture->end();

    // Ensure Cocos2d-x viewport is cleanly restored after unbinding FBO
    director->setViewport();

    // 2. Hide m_gameLayer so MenuLayer does not draw it unshaded on top
    m_gameLayer->setVisible(false);

    // 3. Draw the captured texture through the custom GLSL post-processing shader
    this->drawShader(winSize);

    CCNode::visit();
}

void MenuShaderNode::drawShader(cocos2d::CCSize const& winSize) {
    auto program = ShaderManager::get()->getProgram();
    if (!program || !m_renderTexture) {
        return;
    }

    auto sprite = m_renderTexture->getSprite();
    if (!sprite) {
        return;
    }

    sprite->setShaderProgram(program);

    float shake = AudioManager::get()->getShakeOffset();
    auto spriteSize = sprite->getContentSize();
    float scaleX = (spriteSize.width > 0.0f) ? (winSize.width / spriteSize.width) : 1.0f;
    float scaleY = (spriteSize.height > 0.0f) ? (winSize.height / spriteSize.height) : 1.0f;

    kmGLPushMatrix();
    kmGLTranslatef(shake, shake * 0.6f, 0.0f);
    kmGLScalef(scaleX, scaleY, 1.0f);

    program->use();
    ShaderManager::get()->setUniforms(program, winSize);

    sprite->draw();

    kmGLPopMatrix();

    // Reset shader state to default in Cocos2d-x
    auto* defaultShader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    if (defaultShader) {
        defaultShader->use();
        defaultShader->setUniformsForBuiltins();
    }
}
