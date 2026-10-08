#include <Geode/Geode.hpp>
#include "MenuShaderNode.hpp"
#include "ShaderManager.hpp"
#include "AudioManager.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("MenuGameShaders loaded successfully!");
}
