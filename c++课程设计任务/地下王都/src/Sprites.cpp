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
    "  (  水  )",
    "   `-...-'"
};

const std::vector<std::string> kFireSlime = {
    "   .-~~~-.",
    "  (  火  )",
    "   `-...-'"
};

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

}  // namespace Sprites
