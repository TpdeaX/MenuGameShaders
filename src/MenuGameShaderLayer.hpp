#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/MenuGameLayer.hpp>
#include "ShaderManager.hpp"
#include "AudioManager.hpp"

class $modify(MenuGameShaderLayer, MenuGameLayer) {
    struct Fields {
        geode::Ref<cocos2d::CCRenderTexture> m_renderTexture = nullptr;
        cocos2d::CCSize m_renderSize = cocos2d::CCSizeZero;
        bool m_isRendering = false;
    };

    bool init();
    void onExit();
    void visit();
    void drawShadedQuad(cocos2d::CCSize const& winSize);
};
