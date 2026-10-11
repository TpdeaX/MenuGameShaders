#include <Geode/Geode.hpp>
#include "MenuShaderNode.hpp"
#include "ShaderManager.hpp"
#include "AudioManager.hpp"
#include "ShaderBuilder.hpp"

using namespace geode::prelude;

// Register the "shader-builder" custom setting type declared in mod.json.
// Must run before the mod's settings are parsed.
$execute {
    (void)Mod::get()->registerCustomSettingType("shader-builder", &ShaderBuilderSetting::parse);
}

$on_mod(Loaded) {
    log::info("MenuGameShaders loaded successfully!");
}
