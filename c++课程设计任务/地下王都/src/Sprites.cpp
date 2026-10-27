#include "Sprites.h"

namespace {

// 遗迹鼠：经典字符画，配合前面的「鼠」字标签。
const std::vector<std::string> kRat = {
    "  /\\_/\\",
    " ( o.o )",
    "  > ^ <"
};

// 史莱姆：身体中间直接写元素。
const std::vector<std::string> kWaterSlime = {
    "   .-~~~-.",
    "  (o 水 o )",
    "   `-...-'"};

const std::vector<std::string> kFireSlime = {
    "   .-~~~-.",
    "  (o 火 o )",
    "   `-...-'"};

// 哥布林：尖耳朵、单眼、短腿。
const std::vector<std::string> kGoblin = {
    "    .-.",
    "   (o.o)",
    "   /|_|\\",
    "    / \\"
};

// 石门：中间用「门」字，边框保持 4 列宽。
const std::vector<std::string> kDoor = {
    "┌──┐",
    "│门│",
    "└──┘"
};

// 营地火堆：由「火」字、斜线火焰和木柴组成。
const std::vector<std::string> kCampfire = {
    "  火",
    " /|\\",
    "/ | \\",
    "====="
};

// 商人：头部是「人」，身体是「商」。
const std::vector<std::string> kMerchant = {
    "    人",
    "   /商\\",
    "    / \\"
};

// 残破的城门机械门卫：由「我 / 是 / 人 / 机」几个字拼成。
const std::vector<std::string> kGatekeeper = {
    "     ┌────┐",
    "     │ 我 │",
    "     │ 是 │",
    "     └─┬──┘",
    "   \\  ┌┴人─┐  /",
    "    \\ │ 机 │ /",
    "      └─┬─┘",
    "       / \\"
};

// 营地里的灰袍幸存者：帽檐下没有脸，只有一块磨白的身份牌。
const std::vector<std::string> kSurvivor = {
    "    ┌───┐",
    "    │   │",
    "    └─┬─┘",
    "   / 灰 \\",
    "     / \\"
};

// 封存骑士：四守护单元中最后仍在执行命令的一尊。
const std::vector<std::string> kGuardian = {
    "     ┌─────┐",
    "   ┌─┤ 封  ├─┐",
    "   │ └──┬──┘ │",
    "   │  骑士  │",
    "   └──┬───┬─┘",
    "     /     \\"
};

// 奖励房间里的宝箱：盖子、锁扣和旧木纹。
const std::vector<std::string> kChest = {
    "    ┌──────┐",
    "   ┌┴──────┴┐",
    "   │  宝箱  │",
    "   ├───┬────┤",
    "   │  锁    │",
    "   └────────┘"
};

// 宝箱怪：宝箱张开后露出的牙齿。
const std::vector<std::string> kMimic = {
    "    ┌──────┐",
    "   ┌┴┬─┬─┬─┴┐",
    "   │ 宝 箱  │",
    "   ├─┴─┴─┴──┤",
    "   │ 牙 牙  │",
    "   └────────┘"
};

}  // namespace

namespace Sprites {

const std::vector<std::string>& rat() {
    return kRat;
}

const std::vector<std::string>& waterSlime() {
    return kWaterSlime;
}

const std::vector<std::string>& fireSlime() {
    return kFireSlime;
}

const std::vector<std::string>& goblin() {
    return kGoblin;
}

const std::vector<std::string>& door() {
    return kDoor;
}

const std::vector<std::string>& campfire() {
    return kCampfire;
}

const std::vector<std::string>& merchant() {
    return kMerchant;
}

const std::vector<std::string>& gatekeeper() {
    return kGatekeeper;
}

const std::vector<std::string>& survivor() {
    return kSurvivor;
}

const std::vector<std::string>& guardian() {
    return kGuardian;
}

const std::vector<std::string>& chest() {
    return kChest;
}

const std::vector<std::string>& mimic() {
    return kMimic;
}

}  // namespace Sprites
