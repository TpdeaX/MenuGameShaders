#include "ShaderBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/SliderThumb.hpp>
#include <cocos-ext.h>

#include <Geode/ui/GeodeUI.hpp>
#include "ShaderManager.hpp"
#include "AudioManager.hpp"

using namespace geode::prelude;
using namespace cocos2d::extension;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {
    std::string getStyleName(int style) {
        static const char* const STYLE_NAMES[] = {
            "Shockwave", "Audio Waves", "Beat Bulge", "Neon Beat",
            "Glitch", "Liquid", "Radial EQ", "Hyperspace",
            "Retro CRT", "Kaleidoscope", "Pixel Matrix", "Vortex",
            "Subwoofer", "Custom"
        };
        if (style >= 0 && style < 14) {
            return STYLE_NAMES[style];
        }
        return "Unknown";
    }

    std::string getShapeName(int shape) {
        static const char* const SHAPE_NAMES[] = {
            "Rings", "Waves", "Bulge", "Kaleido", "Vortex", "Tunnel"
        };
        if (shape >= 0 && shape < 6) {
            return SHAPE_NAMES[shape];
        }
        return "Unknown";
    }

    std::string getTintName(int tint) {
        static const char* const TINT_NAMES[] = {
            "Off", "Cycle", "Purple", "Magenta", "Green", "Custom"
        };
        if (tint >= 0 && tint < 6) {
            return TINT_NAMES[tint];
        }
        return "Unknown";
    }

    std::string formatValue(std::string const& name, float value, bool isInt) {
        char buf[64];
        if (name == "Style") {
            int idx = (int)std::llround(value);
            snprintf(buf, sizeof(buf), "Style: %d (%s)", idx, getStyleName(idx).c_str());
        }
        else if (name == "Shape") {
            int idx = (int)std::llround(value);
            snprintf(buf, sizeof(buf), "Shape: %d (%s)", idx, getShapeName(idx).c_str());
        }
        else if (name == "Tint Mode") {
            int idx = (int)std::llround(value);
            snprintf(buf, sizeof(buf), "Tint: %d (%s)", idx, getTintName(idx).c_str());
        }
        else if (isInt) {
            snprintf(buf, sizeof(buf), "%s: %lld", name.c_str(), (long long)std::llround(value));
        }
        else {
            snprintf(buf, sizeof(buf), "%s: %.2f", name.c_str(), (double)value);
        }
        return std::string(buf);
    }
}

// ---------------------------------------------------------------------------
// Custom setting type
// ---------------------------------------------------------------------------

geode::SettingNodeV3* ShaderBuilderSetting::createNode(float width) {
    return ShaderBuilderSettingNode::create(
        std::static_pointer_cast<ShaderBuilderSetting>(shared_from_this()), width
    );
}

geode::Result<std::shared_ptr<geode::SettingV3>> ShaderBuilderSetting::parse(
    std::string const& key, std::string const& modID, matjson::Value const& json
) {
    auto ret = std::make_shared<ShaderBuilderSetting>();
    auto root = checkJson(json, "ShaderBuilderSetting");
    ret->parseBaseProperties(key, modID, root);

    root.checkUnknownKeys();

    return root.ok(ret).map([](std::shared_ptr<ShaderBuilderSetting> value) {
        return std::static_pointer_cast<geode::SettingV3>(value);
    });
}

bool ShaderBuilderSettingNode::init(std::shared_ptr<ShaderBuilderSetting> const& setting, float width) {
    if (!SettingValueNodeV3::init(setting, width)) {
        return false;
    }

    float height = 40.0f;
    this->setContentSize({width, height});

    auto menu = CCMenu::create();
    menu->setPosition({0.0f, 0.0f});
    menu->setContentSize({width, height});
    menu->setID("inputs-menu");

    auto viewSpr = ButtonSprite::create("Configure");
    viewSpr->setScale(0.70f);
    auto viewBtn = CCMenuItemSpriteExtra::create(
        viewSpr, this, menu_selector(ShaderBuilderSettingNode::onOpen)
    );
    float btnWidth = viewSpr->getScaledContentWidth();
    viewBtn->setPosition(width - btnWidth * 0.5f - 16.f, height * 0.5f);
    menu->addChild(viewBtn);

    this->addChild(menu);
    handleTouchPriority(this);

    return true;
}

void ShaderBuilderSettingNode::onOpen(CCObject*) {
    auto popup = ShaderBuilderPopup::create();
    if (popup) {
        CCNode* p = this->getParent();
        while (p && !typeinfo_cast<FLAlertLayer*>(p)) {
            p = p->getParent();
        }
        popup->setParentSettingsPopup(p);
        popup->show();
    }
}

ShaderBuilderSettingNode* ShaderBuilderSettingNode::create(
    std::shared_ptr<ShaderBuilderSetting> const& setting, float width
) {
    auto ret = new ShaderBuilderSettingNode();
    if (ret->init(setting, width)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Live preview node
// ---------------------------------------------------------------------------

ShaderPreviewNode* ShaderPreviewNode::create(cocos2d::CCSize const& size) {
    auto ret = new ShaderPreviewNode();
    if (ret->init(size)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ShaderPreviewNode::init(cocos2d::CCSize const& size) {
    if (!CCNode::init()) {
        return false;
    }

    this->setContentSize(size);
    this->setAnchorPoint({0.0f, 0.0f});
    this->ignoreAnchorPointForPosition(true);

    // Make sure the post-processing shader is compiled and linked.
    ShaderManager::get()->compile();

    // Render a static, high-detail test pattern once. The shader distorts and
    // tints this texture every frame with live beat & audio reaction.
    m_pattern = CCRenderTexture::create(size.width, size.height);
    if (!m_pattern) {
        return false;
    }

    auto content = CCNode::create();
    content->setContentSize(size);
    content->setAnchorPoint({0.0f, 0.0f});
    content->setPosition({0.0f, 0.0f});

    auto bg = CCLayerColor::create(ccc4(24, 20, 48, 255), size.width, size.height);
    bg->setAnchorPoint({0.0f, 0.0f});
    bg->setPosition({0.0f, 0.0f});
    content->addChild(bg);

    auto draw = CCDrawNode::create();
    float w = size.width;
    float h = size.height;

    // Grid lines.
    for (int i = 1; i <= 7; i++) {
        float x = w * i / 8.0f;
        draw->drawSegment(ccp(x, 0.0f), ccp(x, h), 1.0f, ccc4f(0.50f, 0.50f, 0.85f, 0.8f));
    }
    for (int i = 1; i <= 3; i++) {
        float y = h * i / 4.0f;
        draw->drawSegment(ccp(0.0f, y), ccp(w, y), 1.0f, ccc4f(0.50f, 0.50f, 0.85f, 0.8f));
    }

    // Diagonals.
    draw->drawSegment(ccp(0.0f, 0.0f), ccp(w, h), 1.5f, ccc4f(1.0f, 0.35f, 0.85f, 0.9f));
    draw->drawSegment(ccp(0.0f, h), ccp(w, 0.0f), 1.5f, ccc4f(0.25f, 0.85f, 1.0f, 0.9f));

    // Filled colorful test shapes to clearly preview chromatic split, bulges & ripples.
    draw->drawDot(ccp(w * 0.5f, h * 0.5f), 26.0f, ccc4f(1.0f, 0.25f, 0.85f, 1.0f));
    draw->drawDot(ccp(w * 0.25f, h * 0.35f), 16.0f, ccc4f(0.2f, 1.0f, 0.85f, 1.0f));
    draw->drawDot(ccp(w * 0.75f, h * 0.65f), 20.0f, ccc4f(1.0f, 0.9f, 0.2f, 1.0f));
    draw->drawDot(ccp(w * 0.70f, h * 0.25f), 13.0f, ccc4f(1.0f, 0.45f, 0.25f, 1.0f));
    draw->drawDot(ccp(w * 0.28f, h * 0.75f), 13.0f, ccc4f(0.35f, 0.9f, 0.35f, 1.0f));
    content->addChild(draw);

    m_pattern->beginWithClear(0.09f, 0.08f, 0.16f, 1.0f);
    content->visit();
    m_pattern->end();

    m_sprite = m_pattern->getSprite();
    if (m_sprite) {
        m_sprite->setAnchorPoint({0.0f, 0.0f});
        m_sprite->setPosition({0.0f, 0.0f});
    }

    // Update audio and beat clock so preview reacts in real time.
    this->scheduleUpdate();

    return true;
}

void ShaderPreviewNode::update(float dt) {
    AudioManager::get()->update(dt);
}

void ShaderPreviewNode::draw() {
    CCNode::draw();

    auto program = ShaderManager::get()->getProgram();
    if (!program || !m_sprite) {
        return;
    }

    m_sprite->setShaderProgram(program);
    m_sprite->setAnchorPoint({0.0f, 0.0f});
    m_sprite->setPosition({0.0f, 0.0f});
    m_sprite->setScale(1.0f);

    program->use();
    program->setUniformsForBuiltins();
    ShaderManager::get()->setUniforms(program, this->getContentSize());

    m_sprite->draw();

    auto* def = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    if (def) {
        def->use();
        def->setUniformsForBuiltins();
    }
}

// ---------------------------------------------------------------------------
// Popup
// ---------------------------------------------------------------------------

ShaderBuilderPopup* ShaderBuilderPopup::create() {
    auto ret = new ShaderBuilderPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ShaderBuilderPopup::init() {
    if (!Popup::init(400.f, 295.f)) {
        return false;
    }

    this->setTitle("Live Shader Preview");

    // Make sure audio is attached so the preview reacts to the beat immediately.
    AudioManager::get()->attach();

    // Dark frame behind the preview screen
    auto previewBg = CCScale9Sprite::create("square02_small.png");
    previewBg->setContentSize({354.f, 114.f});
    previewBg->setPosition({23.f, 150.f});
    previewBg->setAnchorPoint({0.0f, 0.0f});
    previewBg->setOpacity(180);
    previewBg->setColor({0, 0, 0});
    m_mainLayer->addChild(previewBg);

    m_preview = ShaderPreviewNode::create({350.f, 110.f});
    m_preview->setPosition({25.f, 152.f});
    m_mainLayer->addChild(m_preview);

    // Dark frame behind the scrollable controls
    auto listBg = CCScale9Sprite::create("square02_small.png");
    listBg->setContentSize({354.f, 134.f});
    listBg->setPosition({23.f, 11.f});
    listBg->setAnchorPoint({0.0f, 0.0f});
    listBg->setOpacity(100);
    listBg->setColor({0, 0, 0});
    m_mainLayer->addChild(listBg);

    m_list = ScrollLayer::create({350.f, 130.f});
    m_list->setPosition({25.f, 13.f});
    m_mainLayer->addChild(m_list);

    this->buildControls();
    handleTouchPriority(this);

    return true;
}

void ShaderBuilderPopup::buildControls() {
    static constexpr float ROW_HEIGHT = 28.f;
    static constexpr float ROW_GAP = 3.f;
    static constexpr float CONTENT_PAD = 6.f;
    static constexpr float CONTENT_W = 340.f;

    m_list->m_contentLayer->removeAllChildrenWithCleanup(true);
    m_sliderRows.clear();
    m_toggleRows.clear();

    auto mod = Mod::get();

    auto place = [&](CCNode* row) {
        row->setPosition({5.f, m_rowY - ROW_HEIGHT});
        m_list->m_contentLayer->addChild(row);
        m_rowY -= ROW_HEIGHT + ROW_GAP;
    };

    int totalRows = 8 + 5;  // 8 sliders + 5 toggles
    float contentH = CONTENT_PAD * 2 + totalRows * ROW_HEIGHT + (totalRows - 1) * ROW_GAP;
    m_list->m_contentLayer->setContentSize({m_list->getContentWidth(), contentH});
    m_rowY = contentH - CONTENT_PAD;

    // --- Sliders ----------------------------------------------------------

    struct SliderSpec {
        std::string name;
        std::string key;
        float min;
        float max;
        bool isInt;
        float value;
    };

    std::vector<SliderSpec> sliders = {
        {"Style", "shader-style", 0.f, 13.f, true, (float)ShaderManager::get()->getEffectiveStyle()},
        {"Intensity", "pulse-intensity", 0.f, 3.f, false, (float)mod->getSettingValue<double>("pulse-intensity")},
        {"Distortion", "distortion-strength", 0.f, 3.f, false, (float)mod->getSettingValue<double>("distortion-strength")},
        {"Speed", "speed", 0.1f, 3.f, false, (float)mod->getSettingValue<double>("speed")},
        {"Tint Mode", "color-tint-mode", 0.f, 5.f, true, (float)mod->getSettingValue<int64_t>("color-tint-mode")},
        {"Shape", "custom-shape", 0.f, 5.f, true, (float)mod->getSettingValue<int64_t>("custom-shape")},
        {"Custom Speed", "custom-speed", 0.1f, 3.f, false, (float)mod->getSettingValue<double>("custom-speed")},
        {"Custom Scale", "custom-scale", 0.f, 3.f, false, (float)mod->getSettingValue<double>("custom-scale")},
    };

    for (auto const& spec : sliders) {
        float t = std::clamp((spec.value - spec.min) / (spec.max - spec.min), 0.f, 1.f);

        auto row = CCNode::create();
        row->setContentSize({CONTENT_W, ROW_HEIGHT});
        row->setAnchorPoint({0.0f, 0.0f});

        auto label = CCLabelBMFont::create("", "bigFont.fnt");
        label->setAnchorPoint({0.0f, 0.5f});
        label->setPosition({4.f, ROW_HEIGHT / 2.0f});
        label->setScale(0.36f);
        label->setString(formatValue(spec.name, spec.value, spec.isInt).c_str());
        row->addChild(label);

        auto slider = Slider::create(this, menu_selector(ShaderBuilderPopup::onSlider), 0.55f);
        slider->setValue(t);
        slider->setPosition({CONTENT_W - 75.f, ROW_HEIGHT / 2.0f});
        row->addChild(slider);

        place(row);

        SliderRow srow;
        srow.slider = slider;
        srow.label = label;
        srow.key = spec.key;
        srow.name = spec.name;
        srow.min = spec.min;
        srow.max = spec.max;
        srow.isInt = spec.isInt;
        m_sliderRows.push_back(srow);
    }

    // --- Toggles ----------------------------------------------------------

    struct ToggleSpec {
        std::string name;
        std::string key;
        bool value;
    };

    std::vector<ToggleSpec> toggles = {
        {"Chromatic Aberration", "chromatic-aberration", mod->getSettingValue<bool>("chromatic-aberration")},
        {"Beat Flash", "beat-flash", mod->getSettingValue<bool>("beat-flash")},
        {"Ambient Shockwave", "shockwave-ambient", mod->getSettingValue<bool>("shockwave-ambient")},
        {"Bulge Outward", "bulge-outward", mod->getSettingValue<bool>("bulge-outward")},
        {"Cursor Tracking", "cursor-tracking", mod->getSettingValue<bool>("cursor-tracking")},
    };

    for (auto const& spec : toggles) {
        auto row = CCNode::create();
        row->setContentSize({CONTENT_W, ROW_HEIGHT});
        row->setAnchorPoint({0.0f, 0.0f});

        auto label = CCLabelBMFont::create(spec.name.c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.0f, 0.5f});
        label->setPosition({4.f, ROW_HEIGHT / 2.0f});
        label->setScale(0.36f);
        row->addChild(label);

        auto menu = CCMenu::create();
        menu->setPosition({0.0f, 0.0f});
        menu->setContentSize({CONTENT_W, ROW_HEIGHT});

        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(ShaderBuilderPopup::onToggle), 0.65f
        );
        toggler->toggle(spec.value);
        toggler->setPosition({CONTENT_W - 25.f, ROW_HEIGHT / 2.0f});
        menu->addChild(toggler);
        row->addChild(menu);

        place(row);

        ToggleRow trow;
        trow.toggler = toggler;
        trow.key = spec.key;
        m_toggleRows.push_back(trow);
    }

    // Scroll to the very beginning so the list opens at the top
    m_list->scrollToTop();
    if (contentH > m_list->getContentHeight()) {
        m_list->m_contentLayer->setPositionY(m_list->getContentHeight() - contentH);
    }
}

void ShaderBuilderPopup::onSlider(CCObject* sender) {
    auto* thumb = static_cast<SliderThumb*>(sender);

    for (auto& row : m_sliderRows) {
        if (row.slider->getThumb() != thumb) {
            continue;
        }

        float t = std::clamp(thumb->getValue(), 0.f, 1.f);
        float value = row.min + t * (row.max - row.min);

        if (row.isInt) {
            int64_t iv = (int64_t)std::llround(value);
            Mod::get()->setSettingValue<int64_t>(row.key, iv);

            // Picking a style explicitly turns the randomizer off so the
            // choice actually takes effect.
            if (row.key == "shader-style") {
                Mod::get()->setSettingValue<bool>("randomize-shaders", false);
            }

            row.label->setString(formatValue(row.name, (float)iv, true).c_str());
        }
        else {
            Mod::get()->setSettingValue<double>(row.key, (double)value);
            row.label->setString(formatValue(row.name, value, false).c_str());
        }

        // Immediately persist settings to disk
        (void)Mod::get()->saveData();
        break;
    }
}

void ShaderBuilderPopup::onToggle(CCObject* sender) {
    auto* toggler = static_cast<CCMenuItemToggler*>(sender);

    for (auto& row : m_toggleRows) {
        if (row.toggler != toggler) {
            continue;
        }

        // Use the setting itself as the source of truth and flip it. This is
        // independent of whether CCMenuItemToggler flips its visual state
        // before or after the callback fires.
        bool current = Mod::get()->getSettingValue<bool>(row.key);
        Mod::get()->setSettingValue<bool>(row.key, !current);

        // Immediately persist settings to disk
        (void)Mod::get()->saveData();
        break;
    }
}

void ShaderBuilderPopup::onClose(CCObject* sender) {
    // 1. Immediately persist all mod settings to disk
    (void)Mod::get()->saveData();

    auto parent = m_parentSettingsPopup;
    Popup::onClose(sender);

    // 2. If opened from Geode's ModSettingsPopup, refresh it so its UI displays the new values
    if (parent && parent->getParent()) {
        parent->keyBackClicked();
        geode::openSettingsPopup(Mod::get());
    }
}
