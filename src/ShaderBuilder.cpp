#include "ShaderBuilder.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/SliderThumb.hpp>

#include "ShaderManager.hpp"

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {
    std::string formatValue(std::string const& name, float value, bool isInt) {
        char buf[64];
        if (isInt) {
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
    menu->setID("inputs-menu");

    auto viewSpr = ButtonSprite::create("Configure");
    viewSpr->setScale(0.72f);
    auto viewBtn = CCMenuItemSpriteExtra::create(
        viewSpr, this, menu_selector(ShaderBuilderSettingNode::onOpen)
    );
    viewBtn->setPosition(width - 40.f, height - 20.f);
    menu->addChild(viewBtn);

    this->addChild(menu);
    handleTouchPriority(this);

    return true;
}

void ShaderBuilderSettingNode::onOpen(CCObject*) {
    ShaderBuilderPopup::create()->show();
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
    this->setAnchorPoint({0.5f, 0.5f});

    // Make sure the post-processing shader is compiled and linked.
    ShaderManager::get()->compile();

    // Render a static, high-detail test pattern once. The shader distorts and
    // tints this texture every frame, so it stays cheap while clearly showing
    // waves, bulges, vortexes, chromatic split and color tinting.
    m_pattern = CCRenderTexture::create(size.width, size.height);
    if (!m_pattern) {
        return false;
    }

    auto content = CCNode::create();
    content->setContentSize(size);
    content->setAnchorPoint({0.0f, 0.0f});
    content->setPosition({0.0f, 0.0f});

    auto bg = CCLayerColor::create(ccc4(28, 24, 54, 255), size.width, size.height);
    bg->setAnchorPoint({0.0f, 0.0f});
    bg->setPosition({0.0f, 0.0f});
    content->addChild(bg);

    auto draw = CCDrawNode::create();
    float w = size.width;
    float h = size.height;

    // Grid lines.
    for (int i = 1; i <= 7; i++) {
        float x = w * i / 8.0f;
        draw->drawSegment(ccp(x, 0.0f), ccp(x, h), 1.0f, ccc4f(0.55f, 0.55f, 0.9f, 1.0f));
    }
    for (int i = 1; i <= 3; i++) {
        float y = h * i / 4.0f;
        draw->drawSegment(ccp(0.0f, y), ccp(w, y), 1.0f, ccc4f(0.55f, 0.55f, 0.9f, 1.0f));
    }

    // Diagonals.
    draw->drawSegment(ccp(0.0f, 0.0f), ccp(w, h), 2.0f, ccc4f(1.0f, 0.4f, 0.9f, 1.0f));
    draw->drawSegment(ccp(0.0f, h), ccp(w, 0.0f), 2.0f, ccc4f(0.3f, 0.8f, 1.0f, 1.0f));

    // Filled dots.
    draw->drawDot(ccp(w * 0.5f, h * 0.5f), 28.0f, ccc4f(1.0f, 0.3f, 0.9f, 1.0f));
    draw->drawDot(ccp(w * 0.25f, h * 0.30f), 16.0f, ccc4f(0.2f, 1.0f, 0.9f, 1.0f));
    draw->drawDot(ccp(w * 0.75f, h * 0.70f), 20.0f, ccc4f(1.0f, 0.9f, 0.2f, 1.0f));
    draw->drawDot(ccp(w * 0.70f, h * 0.25f), 12.0f, ccc4f(1.0f, 0.5f, 0.3f, 1.0f));
    content->addChild(draw);

    m_pattern->beginWithClear(0.1f, 0.1f, 0.15f, 1.0f);
    content->visit();
    m_pattern->end();

    m_sprite = m_pattern->getSprite();
    if (m_sprite) {
        m_sprite->setAnchorPoint({0.0f, 0.0f});
        m_sprite->setPosition({0.0f, 0.0f});
    }

    auto program = ShaderManager::get()->getProgram();
    if (program) {
        m_uTime = program->getUniformLocationForName("u_time");
    }

    // Self-advancing clock so the preview animates even without music.
    this->scheduleUpdate();

    return true;
}

void ShaderPreviewNode::update(float dt) {
    m_time += dt;
}

void ShaderPreviewNode::visit() {
    auto program = ShaderManager::get()->getProgram();
    if (program && m_sprite) {
        m_sprite->setShaderProgram(program);
        m_sprite->setAnchorPoint({0.0f, 0.0f});
        m_sprite->setPosition({0.0f, 0.0f});
        m_sprite->setScale(1.0f);

        program->use();
        ShaderManager::get()->setUniforms(program, this->getContentSize());

        // Override u_time with our own clock so the preview is always moving.
        if (m_uTime != -1) {
            program->setUniformLocationWith1f(m_uTime, m_time);
        }

        m_sprite->draw();

        auto* def = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
        if (def) {
            def->use();
            def->setUniformsForBuiltins();
        }
    }

    CCNode::visit();
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
    if (!Popup::init(400.f, 300.f)) {
        return false;
    }

    this->setTitle("Live Shader Preview");

    m_preview = ShaderPreviewNode::create({360.f, 120.f});
    m_mainLayer->addChildAtPosition(m_preview, Anchor::Top, {0.f, -46.f});

    m_list = ScrollLayer::create({360.f, 138.f});
    m_mainLayer->addChildAtPosition(m_list, Anchor::Bottom, {0.f, 12.f});

    this->buildControls();
    handleTouchPriority(this);

    return true;
}

void ShaderBuilderPopup::buildControls() {
    static constexpr float ROW_HEIGHT = 32.f;
    static constexpr float ROW_GAP = 4.f;
    static constexpr float CONTENT_PAD = 10.f;
    static constexpr float CONTENT_W = 344.f;
    static constexpr float LEFT_X = 8.f;

    m_list->m_contentLayer->removeAllChildrenWithCleanup(true);
    m_sliderRows.clear();
    m_toggleRows.clear();

    auto mod = Mod::get();

    // Two-pass: build rows first, then place them. We keep it simple by
    // tracking the top Y coordinate and stacking downward.
    auto place = [&](CCNode* row) {
        row->setPosition({LEFT_X, m_rowY - ROW_HEIGHT});
        m_list->m_contentLayer->addChild(row);
        m_rowY -= ROW_HEIGHT + ROW_GAP;
    };

    int totalRows = 8 + 2;  // 8 sliders + 2 toggles
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
        label->setPosition({0.0f, ROW_HEIGHT / 2});
        label->setScale(0.45f);
        label->setString(formatValue(spec.name, spec.value, spec.isInt).c_str());
        row->addChild(label);

        auto slider = Slider::create(this, menu_selector(ShaderBuilderPopup::onSlider), 0.6f);
        slider->setValue(t);
        slider->setAnchorPoint({0.5f, 0.5f});
        slider->setPosition({CONTENT_W - 128.f, ROW_HEIGHT / 2});
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
    };

    for (auto const& spec : toggles) {
        auto row = CCNode::create();
        row->setContentSize({CONTENT_W, ROW_HEIGHT});
        row->setAnchorPoint({0.0f, 0.0f});

        auto label = CCLabelBMFont::create(spec.name.c_str(), "bigFont.fnt");
        label->setAnchorPoint({0.0f, 0.5f});
        label->setPosition({0.0f, ROW_HEIGHT / 2});
        label->setScale(0.45f);
        row->addChild(label);

        auto menu = CCMenu::create();
        menu->setPosition({0.0f, 0.0f});

        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(ShaderBuilderPopup::onToggle), 0.7f
        );
        toggler->toggle(spec.value);
        toggler->setPosition({CONTENT_W - 20.f, ROW_HEIGHT / 2});
        menu->addChild(toggler);
        row->addChild(menu);

        place(row);

        ToggleRow trow;
        trow.toggler = toggler;
        trow.key = spec.key;
        m_toggleRows.push_back(trow);
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
        break;
    }
}
