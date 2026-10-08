#pragma once

#include <Geode/Geode.hpp>
#include <Geode/binding/MenuGameLayer.hpp>

class MenuShaderNode : public cocos2d::CCNode {
private:
    cocos2d::CCRenderTexture* m_renderTexture = nullptr;
    MenuGameLayer* m_gameLayer = nullptr;
    cocos2d::CCSize m_renderSize = cocos2d::CCSizeZero;
    bool m_capturedFirstFrame = false;

public:
    static MenuShaderNode* create(MenuGameLayer* gameLayer);

    bool init(MenuGameLayer* gameLayer);
    ~MenuShaderNode() override;

    void visit() override;
    void drawShader(cocos2d::CCSize const& winSize);
};
