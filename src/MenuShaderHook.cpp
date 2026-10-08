#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/binding/MenuGameLayer.hpp>
#include "MenuShaderNode.hpp"
#include "AudioManager.hpp"

using namespace geode::prelude;

class $modify(MenuShaderHook, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) {
            return false;
        }

        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            return true;
        }

        // Locate MenuGameLayer either by ID "main-menu-bg" or by first child
        MenuGameLayer* gameLayer = typeinfo_cast<MenuGameLayer*>(this->getChildByID("main-menu-bg"));
        if (!gameLayer && this->getChildrenCount() > 0) {
            gameLayer = typeinfo_cast<MenuGameLayer*>(this->getChildren()->objectAtIndex(0));
        }

        if (gameLayer) {
            auto shaderNode = MenuShaderNode::create(gameLayer);
            if (shaderNode) {
                shaderNode->setID("menu-shader-node"_spr);
                this->addChild(shaderNode, gameLayer->getZOrder());
                gameLayer->setVisible(false);
                log::info("MenuGameShaders: Attached MenuShaderNode to MenuLayer successfully");
            }
        } else {
            log::warn("MenuGameShaders: Could not find MenuGameLayer in MenuLayer");
        }

        return true;
    }

    void onExit() {
        MenuLayer::onExit();
        AudioManager::get()->detach();
    }
};
