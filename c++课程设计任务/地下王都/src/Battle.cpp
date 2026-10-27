#include "Battle.h"

#include "DisasterVisual.h"
#include "Frame.h"
#include "MonsterVisual.h"
#include "PlayerSprite.h"
#include "Random.h"
#include "Sprites.h"
#include "UI.h"

#include <algorithm>
#include <string>

namespace {

constexpr int kLeftCardWidth = 31;
constexpr int kRightCardWidth = 32;

}  // namespace

Battle::Battle(Player& player, int storyStage)
    : player_(player),
      turn_(1),
      storyStage_(storyStage),
      disasterCountdown_(Random::integer(2, 4)),
      hasDisaster_(false),
      disasterType_(DisasterType::Snow) {
}

void Battle::addLog(const std::string& message) {
    log_.push_back(message);
    if (log_.size() > 2) {
        log_.erase(log_.begin());
    }
}

void Battle::render(const Monster& monster, bool battleEnded) const {
    UI::clearScreen();

    Frame frame;

    frame.addColumns(
        UI::paint("地下遗迹 | 遭遇战", UI::Color::BrightCyan),
        UI::paint("回合 " + std::to_string(turn_) +
                  (hasDisaster_ ? " · " + disasterName_ : ""),
                  hasDisaster_ ? DisasterVisual::tint(disasterType_)
                               : UI::Color::Gray),
        40);
    frame.addDivider();
    frame.addBlank();

    const UI::Color enemyColor = monster.isAlive()
                                     ? MonsterVisual::tint(monster.getColor())
                                     : UI::Color::Gray;
    const std::string enemyName = monster.getName() + (monster.isAlive() ? "" : "（倒下）");

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& enemyArt = monster.getArt();

    frame.addSplit(
        UI::center(UI::paint(player_.getName() + "  Lv." +
                             std::to_string(player_.getLevel()),
                             UI::Color::BrightCyan), kLeftCardWidth),
        UI::center(UI::paint(enemyName, enemyColor), kRightCardWidth),
        kLeftCardWidth);

    const int artRows = std::max(4, static_cast<int>(enemyArt.size()));
    for (int index = 0; index < artRows; ++index) {
        const std::string playerLine =
            index < static_cast<int>(playerArt.size())
                ? playerArt[static_cast<std::size_t>(index)]
                : std::string();
        const std::string enemyLine =
            index < static_cast<int>(enemyArt.size())
                ? UI::paint(enemyArt[static_cast<std::size_t>(index)], enemyColor)
                : std::string();
        frame.addSplit(
            UI::center(playerLine, kLeftCardWidth),
            UI::center(enemyLine, kRightCardWidth),
            kLeftCardWidth);
    }

    frame.addBlank();

    frame.addSplit(
        "  HP " + UI::bar(player_.getHp(), player_.getMaxHp(), 12,
                          UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()),
        "  HP " + UI::bar(monster.getHp(), monster.getMaxHp(), 12, UI::Color::BrightRed) +
        "  " + std::to_string(monster.getHp()) + "/" + std::to_string(monster.getMaxHp()),
        kLeftCardWidth);

    frame.addSplit(
        "  MP " + UI::bar(player_.getMp(), player_.getMaxMp(), 12, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()),
        std::string(),
        kLeftCardWidth);

    const int charge = player_.getUltimateCharge();
    const bool ultimateReady = charge >= Player::kUltimateMax;
    std::string chargeLine =
        UI::paint("  充能 ", ultimateReady ? UI::Color::BrightYellow : UI::Color::Gray) +
        UI::bar(charge, Player::kUltimateMax, 10,
                ultimateReady ? UI::Color::BrightMagenta : UI::Color::Gray) +
        "  " + std::to_string(charge) + "/" + std::to_string(Player::kUltimateMax);
    if (ultimateReady) {
        chargeLine += UI::paint(" 就绪", UI::Color::BrightYellow);
    }
    frame.addSplit(chargeLine, std::string(), kLeftCardWidth);

    frame.addDivider();
    if (log_.empty()) {
        frame.addLine("· ...");
    } else {
        for (const std::string& message : log_) {
            frame.addWrapped(message, "· ");
        }
    }

    frame.addDivider();
    if (battleEnded) {
        frame.addLine(UI::paint("战斗结束", UI::Color::Gray));
    } else {
        const UI::Color magicColor = player_.getMp() >= 4
                                         ? UI::Color::BrightCyan
                                         : UI::Color::Gray;
        const UI::Color ultimateColor = ultimateReady
                                            ? UI::Color::BrightMagenta
                                            : UI::Color::Gray;
        frame.addLine(
            UI::paint("[1]", UI::Color::BrightYellow) + "攻击  " +
            UI::paint("[2]", UI::Color::BrightYellow) + "防御  " +
            UI::paint("[3]", UI::Color::BrightYellow) + "物品  " +
            UI::paint("[4]", magicColor) + UI::paint("魔法", magicColor) + "  " +
            UI::paint("[5]", ultimateColor) + UI::paint("大招", ultimateColor) + "  " +
            UI::paint("[0]", UI::Color::BrightYellow) + "逃跑");
    }

    frame.render();
}

void Battle::renderInventory(const std::string& message) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("战斗背包", UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "   背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        30);
    frame.addDivider();
    frame.addBlank();

    frame.addLine(
        "HP " + UI::bar(player_.getHp(), player_.getMaxHp(), 16,
                        UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()) +
        "    MP " + UI::bar(player_.getMp(), player_.getMaxMp(), 12, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()));

    frame.addBlank();

    const std::vector<std::unique_ptr<Item>>& items = player_.getInventory().getItems();
    if (items.empty()) {
        frame.addLine("背包里没有物品。");
    } else {
        for (int index = 0; index < static_cast<int>(items.size()); ++index) {
            const Item& item = *items[static_cast<std::size_t>(index)];
            std::string line = UI::paint("[" + std::to_string(index + 1) + "]",
                                         UI::Color::BrightYellow) +
                               " " + UI::padRight(item.getName(), 10) + " " +
                               item.getDescription();
            if (item.getKind() == ItemKind::Material) {
                line += "  （材料）";
            } else if (item.getKind() != ItemKind::Consumable) {
                line += "  （装备）";
            }
            frame.addLine(line);
        }
    }

    frame.addBlank();
    if (!message.empty()) {
        frame.addWrapped(message, "  ");
    } else {
        frame.addWrapped("战斗中使用消耗品会消耗一个回合。", "  ");
    }
    frame.addDivider();
    frame.addLine("输入物品编号使用；装备不能在这里更换。");
    frame.addLine(UI::paint("[0]", UI::Color::BrightYellow) + " 返回战斗");
    frame.render();
}

bool Battle::useItemFromInventory(std::string& message) {
    std::string hint = "选择一件消耗品使用。";

    while (true) {
        renderInventory(hint);

        const int choice = UI::readChoice(
            UI::paint("  物品编号（0 返回） > ", UI::Color::BrightGreen),
            0,
            player_.getInventory().size());
        if (choice == 0 || choice == -1) {
            message.clear();
            return false;
        }

        Item* item = player_.getInventory().getItem(choice - 1);
        if (item == nullptr) {
            hint = "物品不存在。";
            continue;
        }
        if (item->getKind() == ItemKind::Material) {
            hint = "材料不能在战斗中使用。";
            continue;
        }
        if (item->getKind() != ItemKind::Consumable) {
            hint = "战斗中不能更换装备。";
            continue;
        }

        std::string useMessage;
        if (player_.getInventory().useItem(choice - 1, player_, useMessage)) {
            message = useMessage;
            return true;
        }
        hint = useMessage;
    }
}

BattleResult Battle::fight(Monster& monster) {
    log_.clear();
    turn_ = 1;
    addLog(monster.getName() + " 挡住了去路。");

    while (player_.isAlive() && monster.isAlive()) {
        // 防御姿态只持续到玩家下一次行动之前。
        player_.setDefending(false);

        if (--disasterCountdown_ <= 0) {
            disasterCountdown_ = Random::integer(2, 4);
            const DisasterEvent event = Disaster::generate(storyStage_);
            hasDisaster_ = true;
            disasterType_ = event.type;
            disasterName_ = event.name;

            const int playerDamage = player_.takeDamage(event.hpDamage);
            const int monsterDamage = monster.takeDamage(event.hpDamage);
            int mpLost = 0;
            if (event.mpDamage > 0) {
                mpLost = std::min(event.mpDamage, player_.getMp());
                if (mpLost > 0) {
                    player_.spendMp(mpLost);
                }
            }

            std::string disasterLog = "天灾·" + event.name + "：" + event.description +
                                      "（你 -" + std::to_string(playerDamage) + " HP";
            if (monsterDamage > 0) {
                disasterLog += "，" + monster.getName() + " -" +
                               std::to_string(monsterDamage) + " HP";
            }
            if (mpLost > 0) {
                disasterLog += "，MP -" + std::to_string(mpLost);
            }
            disasterLog += "）";
            addLog(disasterLog);

            if (!player_.isAlive()) {
                addLog("你在天灾中倒下了。");
                render(monster, true);
                return BattleResult::Defeat;
            }
            if (!monster.isAlive()) {
                addLog(monster.getName() + " 在天灾中倒下了。");
                render(monster, true);
                return BattleResult::Victory;
            }
        }

        render(monster);

        int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 5);
        if (choice == -1) {
            return BattleResult::Escaped;
        }

        bool turnConsumed = true;

        switch (choice) {
            case 0:
                if (Random::chance(0.7)) {
                    addLog("你趁乱退回了营地。");
                    render(monster, true);
                    return BattleResult::Escaped;
                }
                addLog("逃跑失败，" + monster.getName() + "挡住了退路！");
                break;

            case 1:
                addLog(player_.takeTurn(monster));
                break;

            case 2:
                player_.setDefending(true);
                addLog("你举起武器，摆出防御姿态。");
                break;

            case 3: {
                std::string message;
                if (useItemFromInventory(message)) {
                    addLog(message);
                } else {
                    turnConsumed = false;
                }
                break;
            }

            case 4: {
                std::string message;
                if (player_.castMagic(monster, message)) {
                    addLog(message);
                } else {
                    addLog(message);
                    turnConsumed = false;
                }
                break;
            }

            case 5: {
                std::string message;
                if (player_.ultimateAttack(monster, message)) {
                    addLog(message);
                } else {
                    addLog(message);
                    turnConsumed = false;
                }
                break;
            }

            default:
                break;
        }

        // 取消行动、MP / 充能不足、药水无效时不进入怪物回合。
        if (!turnConsumed) {
            continue;
        }

        if (!monster.isAlive()) {
            addLog(monster.getName() + " 倒下了。");
            render(monster, true);
            return BattleResult::Victory;
        }

        addLog(monster.takeTurn(player_));
        if (!player_.isAlive()) {
            addLog("你倒下了。");
            render(monster, true);
            return BattleResult::Defeat;
        }

        ++turn_;
    }

    if (!monster.isAlive()) {
        return BattleResult::Victory;
    }
    if (!player_.isAlive()) {
        return BattleResult::Defeat;
    }
    return BattleResult::Escaped;
}
