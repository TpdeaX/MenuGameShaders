#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Geode/Geode.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/binding/Slider.hpp>
#include <Geode/binding/CCMenuItemToggler.hpp>
#include <Geode/binding/FLAlertLayer.hpp>

// ---------------------------------------------------------------------------
// Live Shader Builder
//
// Mirrors the "custom setting + button + popup" pattern used by Jukebox's
// "indexes" setting (see its IndexSetting / IndexSettingNode / IndexesPopup):
//
//   * mod.json declares  `"type": "custom:shader-builder"`.
//   * main.cpp registers the type with `Mod::registerCustomSettingType`.
//   * ShaderBuilderSettingNode draws a "Configure" button row in the mod
//     settings.
//   * Clicking that button opens ShaderBuilderPopup, an in-game popup with a
//     live shader preview and real-time sliders / toggles that write straight
//     to the mod's regular settings, so the menu background updates instantly.
// ---------------------------------------------------------------------------

// Custom setting type. Its stored value is an unused bool placeholder; the
// setting only exists to render a launcher button in the settings list.
class ShaderBuilderSetting final : public geode::SettingBaseValueV3<bool> {
public:
    static geode::Result<std::shared_ptr<geode::SettingV3>> parse(
        std::string const& key, std::string const& modID, matjson::Value const& json);

    geode::SettingNodeV3* createNode(float width) override;
};

class ShaderBuilderSettingNode final : public geode::SettingValueNodeV3<ShaderBuilderSetting> {
protected:
    bool init(std::shared_ptr<ShaderBuilderSetting> const& setting, float width);
    void onOpen(cocos2d::CCObject* sender);

public:
    static ShaderBuilderSettingNode* create(std::shared_ptr<ShaderBuilderSetting> const& setting, float width);
};

// Renders a static, high-detail test pattern through the menu shader every
// frame, so the effect can be previewed inside the popup without depending on
// the current menu background.
class ShaderPreviewNode final : public cocos2d::CCNode {
protected:
    geode::Ref<cocos2d::CCRenderTexture> m_pattern;
    cocos2d::CCSprite* m_sprite = nullptr;

    bool init(cocos2d::CCSize const& size);
    void update(float dt) override;
    void draw() override;

public:
    static ShaderPreviewNode* create(cocos2d::CCSize const& size);
};

class ShaderBuilderPopup final : public geode::Popup {
protected:
    struct SliderRow {
        Slider* slider;
        cocos2d::CCLabelBMFont* label;
        std::string key;
        std::string name;
        float min;
        float max;
        bool isInt;
    };
    struct ToggleRow {
        CCMenuItemToggler* toggler;
        std::string key;
    };

    geode::Ref<ShaderPreviewNode> m_preview;
    geode::Ref<geode::ScrollLayer> m_list;
    std::vector<SliderRow> m_sliderRows;
    std::vector<ToggleRow> m_toggleRows;
    float m_rowY = 0.0f;
    FLAlertLayer* m_parentSettingsPopup = nullptr;

    bool init();
    void buildControls();
    void onSlider(cocos2d::CCObject* sender);
    void onToggle(cocos2d::CCObject* sender);
    void syncParentSettings();
    void onClose(cocos2d::CCObject* sender) override;

public:
    static ShaderBuilderPopup* create();
    void setParentSettingsPopup(FLAlertLayer* popup) { m_parentSettingsPopup = popup; }
};
