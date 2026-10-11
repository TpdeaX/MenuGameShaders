#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/MenuGameLayer.hpp>

class MenuShaderNode : public cocos2d::CCNode {
private:
    geode::Ref<cocos2d::CCRenderTexture> m_renderTexture = nullptr;
    MenuGameLayer* m_gameLayer = nullptr;
    cocos2d::CCSize m_renderSize = cocos2d::CCSizeZero;
    cocos2d::CCSize m_frameSize = cocos2d::CCSizeZero;
    cocos2d::CCRect m_viewPortRect = cocos2d::CCRectZero;

public:
    static MenuShaderNode* create(MenuGameLayer* gameLayer);

    bool init(MenuGameLayer* gameLayer);
    void onExit() override;
    ~MenuShaderNode() override;

    void cleanupResources();
    void visit() override;
    void drawShader(cocos2d::CCSize const& winSize);
};
