#include "MenuShaderNode.hpp"
#include "ShaderManager.hpp"
#include "AudioManager.hpp"

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

MenuShaderNode::~MenuShaderNode() {
    if (m_renderTexture) {
        m_renderTexture->release();
        m_renderTexture = nullptr;
    }
    if (m_gameLayer) {
        m_gameLayer->setVisible(true);
    }
}

void MenuShaderNode::visit() {
    bool enabled = Mod::get()->getSettingValue<bool>("enabled");

    if (!enabled || !m_gameLayer) {
        if (m_gameLayer) {
            m_gameLayer->setVisible(true);
        }
        CCNode::visit();
        return;
    }

    auto winSize = CCDirector::sharedDirector()->getWinSize();
    if (winSize.width <= 0 || winSize.height <= 0) {
        CCNode::visit();
        return;
    }

    // Recreate or allocate CCRenderTexture if size changed
    if (!m_renderTexture || m_renderSize != winSize) {
        if (m_renderTexture) {
            m_renderTexture->release();
        }
        m_renderSize = winSize;
        m_renderTexture = CCRenderTexture::create(winSize.width, winSize.height);
        if (m_renderTexture) {
            m_renderTexture->retain();
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
    float dt = CCDirector::sharedDirector()->getDeltaTime();
    AudioManager::get()->update(dt);

    // 1. Temporarily show m_gameLayer to capture it into the offscreen FBO
    m_gameLayer->setVisible(true);

    m_renderTexture->beginWithClear(0.0f, 0.0f, 0.0f, 1.0f);
    m_gameLayer->visit();
    m_renderTexture->end();

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
    sprite->setAnchorPoint({ 0.0f, 0.0f });
    sprite->setPosition({ 0.0f, 0.0f });
    sprite->setFlipY(false);

    program->use();
    ShaderManager::get()->setUniforms(program, winSize);

    sprite->draw();

    // Reset shader state to default in Cocos2d-x
    auto* defaultShader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    if (defaultShader) {
        defaultShader->use();
        defaultShader->setUniformsForBuiltins();
    }
}
