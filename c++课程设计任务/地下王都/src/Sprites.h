#pragma once

#include <string>
#include <vector>

// 用文字组成的角色与场景素材。
// 所有线条都保持块状宽度，方便按显示宽度居中。
namespace Sprites {

const std::vector<std::string>& rat();
const std::vector<std::string>& waterSlime();
const std::vector<std::string>& fireSlime();
const std::vector<std::string>& goblin();
const std::vector<std::string>& door();
const std::vector<std::string>& campfire();
const std::vector<std::string>& merchant();
const std::vector<std::string>& gatekeeper();

}  // namespace Sprites
