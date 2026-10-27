#include "SaveGame.h"

#include "Item.h"
#include "Player.h"

#include <cctype>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr const char* kFileHeader = "SHENMO_WANGDU 1";

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

bool parseInt(const std::string& text, int& value) {
    try {
        std::size_t used = 0;
        const int parsed = std::stoi(text, &used);
        while (used < text.size() &&
               std::isspace(static_cast<unsigned char>(text[used]))) {
            ++used;
        }
        if (used != text.size()) {
            return false;
        }
        value = parsed;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char character : text) {
        if (character == delimiter) {
            parts.push_back(current);
            current.clear();
        } else {
            current += character;
        }
    }
    parts.push_back(current);
    return parts;
}

std::string serializeItem(const Item& item) {
    if (const Potion* potion = dynamic_cast<const Potion*>(&item)) {
        const std::string effect =
            (potion->getEffect() == Potion::Effect::RestoreHp) ? "HP" : "MP";
        return "POTION|" + potion->getName() + "|" +
               std::to_string(potion->getPrice()) + "|" + effect + "|" +
               std::to_string(potion->getAmount());
    }

    if (const Weapon* weapon = dynamic_cast<const Weapon*>(&item)) {
        return "WEAPON|" + weapon->getName() + "|" +
               std::to_string(weapon->getPrice()) + "|" +
               std::to_string(weapon->getAttackBonus());
    }

    if (const Armor* armor = dynamic_cast<const Armor*>(&item)) {
        return "ARMOR|" + armor->getName() + "|" +
               std::to_string(armor->getPrice()) + "|" +
               std::to_string(armor->getDefenseBonus());
    }

    if (const Accessory* accessory = dynamic_cast<const Accessory*>(&item)) {
        return "ACCESSORY|" + accessory->getName() + "|" +
               std::to_string(accessory->getPrice()) + "|" +
               std::to_string(accessory->getAttackBonus()) + "|" +
               std::to_string(accessory->getDefenseBonus());
    }

    if (const Material* material = dynamic_cast<const Material*>(&item)) {
        return "MATERIAL|" + material->getName() + "|" +
               std::to_string(material->getPrice());
    }

    return std::string();
}

std::unique_ptr<Item> parseItem(const std::string& data, std::string& error) {
    const std::vector<std::string> parts = split(data, '|');
    if (parts.empty()) {
        error = "物品记录为空";
        return nullptr;
    }

    const std::string& type = parts[0];
    if (type == "POTION") {
        if (parts.size() != 5) {
            error = "药水记录字段数量不正确";
            return nullptr;
        }
        int price = 0;
        int amount = 0;
        if (!parseInt(parts[2], price) || !parseInt(parts[4], amount) ||
            price < 0 || amount <= 0) {
            error = "药水记录数值不正确";
            return nullptr;
        }
        if (parts[1].empty()) {
            error = "药水名称为空";
            return nullptr;
        }
        Potion::Effect effect;
        if (parts[3] == "HP") {
            effect = Potion::Effect::RestoreHp;
        } else if (parts[3] == "MP") {
            effect = Potion::Effect::RestoreMp;
        } else {
            error = "药水效果无法识别";
            return nullptr;
        }
        return std::make_unique<Potion>(parts[1], price, effect, amount);
    }

    if (type == "WEAPON") {
        if (parts.size() != 4) {
            error = "武器记录字段数量不正确";
            return nullptr;
        }
        int price = 0;
        int attack = 0;
        if (!parseInt(parts[2], price) || !parseInt(parts[3], attack) ||
            price < 0 || attack < 0 || parts[1].empty()) {
            error = "武器记录数值不正确";
            return nullptr;
        }
        return std::make_unique<Weapon>(parts[1], price, attack);
    }

    if (type == "ARMOR") {
        if (parts.size() != 4) {
            error = "防具记录字段数量不正确";
            return nullptr;
        }
        int price = 0;
        int defense = 0;
        if (!parseInt(parts[2], price) || !parseInt(parts[3], defense) ||
            price < 0 || defense < 0 || parts[1].empty()) {
            error = "防具记录数值不正确";
            return nullptr;
        }
        return std::make_unique<Armor>(parts[1], price, defense);
    }

    if (type == "ACCESSORY") {
        if (parts.size() != 5) {
            error = "饰品记录字段数量不正确";
            return nullptr;
        }
        int price = 0;
        int attack = 0;
        int defense = 0;
        if (!parseInt(parts[2], price) || !parseInt(parts[3], attack) ||
            !parseInt(parts[4], defense) || price < 0 || attack < 0 ||
            defense < 0 || parts[1].empty()) {
            error = "饰品记录数值不正确";
            return nullptr;
        }
        return std::make_unique<Accessory>(parts[1], price, attack, defense);
    }

    if (type == "MATERIAL") {
        if (parts.size() != 3) {
            error = "材料记录字段数量不正确";
            return nullptr;
        }
        int price = 0;
        if (!parseInt(parts[2], price) || price < 0 || parts[1].empty()) {
            error = "材料记录数值不正确";
            return nullptr;
        }
        return std::make_unique<Material>(parts[1], price);
    }

    error = "未知物品类型：" + type;
    return nullptr;
}

std::unique_ptr<Equipment> parseEquipment(const std::string& data,
                                          EquipmentSlot expectedSlot,
                                          std::string& error) {
    std::unique_ptr<Item> item = parseItem(data, error);
    if (!item) {
        return nullptr;
    }

    Equipment* equipment = dynamic_cast<Equipment*>(item.get());
    if (equipment == nullptr) {
        error = "身份牌中的装备记录不正确";
        return nullptr;
    }
    if (equipment->getSlot() != expectedSlot) {
        error = "身份牌中的装备部位不匹配";
        return nullptr;
    }

    return std::unique_ptr<Equipment>(static_cast<Equipment*>(item.release()));
}

}  // namespace

bool SaveGame::exists() {
    std::ifstream input(kFileName);
    return input.good();
}

SaveSummary SaveGame::peek() {
    SaveSummary summary;

    std::ifstream input(kFileName);
    if (!input) {
        return summary;
    }
    summary.fileExists = true;

    std::string line;
    if (!std::getline(input, line) || trim(line) != kFileHeader) {
        return summary;
    }

    int experience = 0;
    int hp = 0;
    int mp = 0;
    bool hasExperience = false;
    bool hasHp = false;
    bool hasMp = false;

    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "name") {
            summary.name = value;
        } else if (key == "gender") {
            summary.gender = value;
        } else if (key == "species") {
            summary.species = value;
        } else if (key == "level") {
            parseInt(value, summary.level);
        } else if (key == "exp") {
            hasExperience = parseInt(value, experience);
        } else if (key == "hp") {
            hasHp = parseInt(value, hp);
        } else if (key == "mp") {
            hasMp = parseInt(value, mp);
        } else if (key == "gold") {
            parseInt(value, summary.gold);
        }
    }

    summary.valid = !summary.name.empty() && summary.level >= 1 && summary.gold >= 0 &&
                    hasExperience && hasHp && hasMp &&
                    experience >= 0 && hp >= 0 && mp >= 0;
    if (summary.gender.empty()) {
        summary.gender = "不明";
    }
    if (summary.species.empty()) {
        summary.species = "人类";
    }
    return summary;
}

bool SaveGame::save(const Player& player, std::string& error) {
    std::ofstream output(kFileName, std::ios::out | std::ios::trunc);
    if (!output) {
        error = "无法打开身份牌文件";
        return false;
    }

    output << kFileHeader << "\n";
    output << "name=" << player.getName() << "\n";
    output << "gender=" << player.getGender() << "\n";
    output << "species=" << player.getSpecies() << "\n";
    output << "level=" << player.getLevel() << "\n";
    output << "exp=" << player.getExperience() << "\n";
    output << "hp=" << player.getHp() << "\n";
    output << "mp=" << player.getMp() << "\n";
    output << "gold=" << player.getGold() << "\n";
    output << "charge=" << player.getUltimateCharge() << "\n";

    const Equipment* weapon = player.getWeapon();
    const Equipment* armor = player.getArmor();
    const Equipment* accessory = player.getAccessory();
    output << "weapon=" << (weapon ? serializeItem(*weapon) : "none") << "\n";
    output << "armor=" << (armor ? serializeItem(*armor) : "none") << "\n";
    output << "accessory=" << (accessory ? serializeItem(*accessory) : "none") << "\n";

    for (const std::unique_ptr<Item>& item : player.getInventory().getItems()) {
        output << "item=" << serializeItem(*item) << "\n";
    }

    if (!output) {
        error = "写入身份牌失败";
        return false;
    }
    return true;
}

std::unique_ptr<Player> SaveGame::load(std::string& error) {
    std::ifstream input(kFileName);
    if (!input) {
        error = "找不到身份牌";
        return nullptr;
    }

    std::string line;
    if (!std::getline(input, line) || trim(line) != kFileHeader) {
        error = "身份牌格式不正确";
        return nullptr;
    }

    std::string name;
    std::string gender = "不明";
    std::string species = "人类";
    int level = 0;
    int experience = 0;
    int hp = 0;
    int mp = 0;
    int gold = 0;
    int ultimateCharge = 0;
    bool hasName = false;
    bool hasLevel = false;
    bool hasExperience = false;
    bool hasHp = false;
    bool hasMp = false;
    bool hasGold = false;
    std::string weaponData = "none";
    std::string armorData = "none";
    std::string accessoryData = "none";
    std::vector<std::string> inventoryData;

    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "name") {
            name = value;
            hasName = !name.empty();
        } else if (key == "gender") {
            if (!value.empty()) {
                gender = value;
            }
        } else if (key == "species") {
            if (!value.empty()) {
                species = value;
            }
        } else if (key == "level") {
            hasLevel = parseInt(value, level);
        } else if (key == "exp") {
            hasExperience = parseInt(value, experience);
        } else if (key == "hp") {
            hasHp = parseInt(value, hp);
        } else if (key == "mp") {
            hasMp = parseInt(value, mp);
        } else if (key == "gold") {
            hasGold = parseInt(value, gold);
        } else if (key == "charge") {
            parseInt(value, ultimateCharge);
        } else if (key == "weapon") {
            weaponData = value;
        } else if (key == "armor") {
            armorData = value;
        } else if (key == "accessory") {
            accessoryData = value;
        } else if (key == "item") {
            inventoryData.push_back(value);
        }
    }

    if (!hasName || !hasLevel || !hasExperience || !hasHp || !hasMp || !hasGold) {
        error = "身份牌缺少必要字段";
        return nullptr;
    }
    if (level < 1 || level > 99 || experience < 0 || hp < 0 || mp < 0 || gold < 0 ||
        ultimateCharge < 0 || ultimateCharge > Player::kUltimateMax ||
        experience > 1000000 || hp > 100000 || mp > 100000 || gold > 1000000) {
        error = "身份牌数值超出范围";
        return nullptr;
    }
    if (static_cast<int>(inventoryData.size()) > Inventory::kCapacity) {
        error = "身份牌中的背包物品超过容量";
        return nullptr;
    }

    std::unique_ptr<Player> player = std::make_unique<Player>(name);
    player->setIdentity(gender, species);
    player->restoreProgress(level, experience, hp, mp, gold, ultimateCharge);

    for (const std::string& data : inventoryData) {
        std::unique_ptr<Item> item = parseItem(data, error);
        if (!item) {
            return nullptr;
        }
        if (!player->getInventory().add(std::move(item))) {
            error = "身份牌中的背包物品无法放入背包";
            return nullptr;
        }
    }

    std::unique_ptr<Equipment> weapon;
    std::unique_ptr<Equipment> armor;
    std::unique_ptr<Equipment> accessory;
    if (weaponData != "none") {
        weapon = parseEquipment(weaponData, EquipmentSlot::Weapon, error);
        if (!weapon) {
            return nullptr;
        }
    }
    if (armorData != "none") {
        armor = parseEquipment(armorData, EquipmentSlot::Armor, error);
        if (!armor) {
            return nullptr;
        }
    }
    if (accessoryData != "none") {
        accessory = parseEquipment(accessoryData, EquipmentSlot::Accessory, error);
        if (!accessory) {
            return nullptr;
        }
    }
    player->setEquipment(std::move(weapon), std::move(armor), std::move(accessory));

    error.clear();
    return player;
}
