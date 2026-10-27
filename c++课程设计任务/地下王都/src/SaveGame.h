#pragma once

#include <memory>
#include <string>

class Player;

struct SaveSummary {
    bool valid = false;
    bool fileExists = false;
    std::string name;
    std::string gender;
    std::string species;
    int level = 0;
    int gold = 0;
};

// 身份牌（存档）读写。
// 文件为纯文本，保存姓名、等级、经验、HP/MP、金币、背包与装备。
class SaveGame {
public:
    static constexpr const char* kFileName = "identity.sav";

    static bool exists();
    static SaveSummary peek();

    static bool save(const Player& player, std::string& error);
    static std::unique_ptr<Player> load(std::string& error);
};
