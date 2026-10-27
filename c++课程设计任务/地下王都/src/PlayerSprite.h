#pragma once

#include <string>
#include <vector>

class Player;
class Equipment;

// 玩家文字形象：头「人」、身体「旅」、右手武器「剑」。
// 护甲决定头部和身体颜色，武器决定剑的颜色，四肢颜色固定。
namespace PlayerSprite {

std::vector<std::string> lines(const Player& player);
std::vector<std::string> lines(const Equipment* weapon, const Equipment* armor);
std::vector<std::string> defaultLines();

}  // namespace PlayerSprite
