#include "Game.h"

#include "Battle.h"
#include "Disaster.h"
#include "DisasterVisual.h"
#include "Frame.h"
#include "Item.h"
#include "Monster.h"
#include "MonsterVisual.h"
#include "PlayerSprite.h"
#include "Random.h"
#include "RuinsRat.h"
#include "Shop.h"
#include "Slime.h"
#include "Sprites.h"
#include "Goblin.h"
#include "UI.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kStoryProgressFile = "story_progress.sav";
constexpr int kStoryClueCount = 8;

int loadStoryStage(const std::string& playerName) {
    std::ifstream input(kStoryProgressFile);
    std::string savedName;
    int stage = 0;
    if (!std::getline(input, savedName) || savedName != "name=" + playerName ||
        !(input >> stage) || stage < 0 || stage > kStoryClueCount + 1) {
        return 0;
    }
    return stage;
}

void saveStoryStage(const std::string& playerName, int stage) {
    std::ofstream output(kStoryProgressFile, std::ios::out | std::ios::trunc);
    if (output) {
        output << "name=" << playerName << "\n" << stage << "\n";
    }
}

std::string atColumn(const std::string& text, int column) {
    if (column <= 0) {
        return text;
    }
    return std::string(static_cast<std::size_t>(column), ' ') + text;
}

// 在同一行放置左右两段文字，用于拼出赶路场景。
std::string sceneRow(const std::string& left,
                     int leftColumn,
                     const std::string& right,
                     int rightColumn) {
    std::string line = atColumn(left, leftColumn);
    const int usedWidth = UI::displayWidth(line);
    if (usedWidth < rightColumn) {
        line.append(static_cast<std::size_t>(rightColumn - usedWidth), ' ');
    }
    line += right;
    return line;
}

std::string floorLine(int width) {
    std::string result;
    result.reserve(static_cast<std::size_t>(width) * 3);
    for (int index = 0; index < width; ++index) {
        result += "─";
    }
    return result;
}

std::string repeatText(const std::string& unit, int count) {
    std::string result;
    for (int index = 0; index < count; ++index) {
        result += unit;
    }
    return result;
}

std::string travelFlavor(int progress) {
    switch (progress) {
        case 1:
            return "你举着火把，沿着湿滑的石阶向下走。\n胸口传来微弱的刺痛，像有什么东西在黑暗里回应。";
        case 2:
            return "墙上刻着模糊的旧王都纹章，边缘覆着一层薄霜。\n这里明明没有通向地面的风。";
        case 3:
            return "半塌的木门挡在走廊尽头。\n门后的石墙留着焦痕，地面却仍在轻轻震动。";
        default:
            return "你继续向遗迹深处走去。";
    }
}

std::string shopItemLine(int index, const Item& item) {
    return UI::paint("[" + std::to_string(index) + "]", UI::Color::BrightYellow) + " " +
           UI::padRight(item.getName(), 8) + "  " +
           UI::padRight(std::to_string(item.getPrice()) + " 金币", 8) + "  " +
           item.getDescription();
}

std::string inventoryItemLine(int index, const Item& item, bool selling) {
    std::string line = UI::paint("[" + std::to_string(index) + "]", UI::Color::BrightYellow) +
                       " " + UI::padRight(item.getName(), 10) + " " +
                       item.getDescription();
    if (item.getKind() == ItemKind::Material) {
        line += "  [材料]";
    } else if (item.getKind() != ItemKind::Consumable) {
        line += "  [装备]";
    }
    if (selling) {
        line += "    售价 " + std::to_string(item.getSellPrice()) + " 金币";
    }
    return line;
}

std::string equipmentLabel(const Equipment* equipment) {
    if (equipment == nullptr) {
        return "无";
    }
    return equipment->getName() + "（" + equipment->getDescription() + "）";
}

std::string trimText(const std::string& text) {
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

std::string cardBorder(bool top) {
    std::string line = top ? "┌" : "└";
    for (int index = 0; index < 20; ++index) {
        line += "─";
    }
    line += top ? "┐" : "┘";
    return line;
}

std::string cardLine(const std::string& text) {
    return "│ " + UI::padRight(text, 18) + " │";
}

}  // namespace

Game::Game()
    : player_("旅人"), storyStage_(0) {
    // 真正的初始物品在城门登记新身份时发放。
}

void Game::run() {
    showTitleScreen();
    showPrologueScreen();

    if (!startFromGate()) {
        return;
    }

    storyStage_ = loadStoryStage(player_.getName());

    showCityEntryNarration();

    std::string message = "你在城内一处半塌的屋子里支起火堆。";
    bool running = true;

    while (running) {
        renderCamp(message);
        message.clear();

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 5);
        switch (choice) {
            case 1:
                showStatusScreen();
                break;
            case 2:
                showInventoryScreen();
                break;
            case 3:
                showShop();
                break;
            case 4:
                message = explore();
                break;
            case 5:
                player_.restoreAll();
                message = "你在火堆旁休息，HP 与 MP 已完全恢复。";
                break;
            case 0:
            case -1:
                running = false;
                break;
            default:
                break;
        }

        if (running) {
            std::string saveError;
            if (!SaveGame::save(player_, saveError)) {
                message = "自动存档失败：" + saveError;
            }
        }
    }

    std::string saveError;
    std::string warning;
    if (!SaveGame::save(player_, saveError)) {
        warning = "自动存档失败：" + saveError;
    }
    renderFarewell(warning);
}

void Game::showTitleScreen() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();

    constexpr int kInnerWidth = 58;
    const std::string innerTop =
        "╔" + repeatText("═", kInnerWidth - 2) + "╗";
    const std::string innerBlank =
        "║" + std::string(kInnerWidth - 2, ' ') + "║";
    const std::string innerBottom =
        "╚" + repeatText("═", 15) + "╤" + repeatText("═", 24) +
        "╤" + repeatText("═", 15) + "╝";

    const auto centerInner = [](const std::string& line) {
        return "   " + line + "   ";
    };

    frame.addLine(UI::fit(UI::paint(centerInner(innerTop), UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(centerInner(innerBlank), UI::Color::Gray), 64));

    std::string titleLine =
        "   " + UI::paint("║", UI::Color::Gray) +
        UI::padRight(UI::center(UI::paint("沉   没   王   都",
                                          UI::Color::BrightYellow, true), 56), 56) +
        UI::paint("║", UI::Color::Gray) + "   ";
    frame.addLine(UI::fit(titleLine, 64));

    frame.addLine(UI::fit(UI::paint(centerInner(innerBlank), UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(centerInner(innerBottom), UI::Color::Gray), 64));

    frame.addLine(UI::fit(UI::paint(
        "     s    ╲                    ╱          ╲              a     ╱",
        UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(
        "   s ` s   ╲      wera        ╱      x     ╲           aa     ╱",
        UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(
        "     s      ╲        sdf     ╱       x      ╲            ac  ╱",
        UI::Color::Gray), 64));

    frame.addLine(UI::fit(UI::paint(
        "   ══════════    ═══════════════════════════    ══════════",
        UI::Color::Gray), 64));

    frame.addLine(UI::fit(UI::paint(
        "        as  ╱           asd  ╲        sss   ╱                ╲",
        UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(
        "    hf     ╱      adsa        ╲   mki      ╱           o      ╲",
        UI::Color::Gray), 64));
    frame.addLine(UI::fit(UI::paint(
        "          ╱  k          gsw    ╲          ╱             o      ╲",
        UI::Color::Gray), 64));

    frame.addCentered(UI::paint("·   古   ·   城   ·   咒   ·   罪   ·   墟   ·",
                                UI::Color::Gray));
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车 > ", UI::Color::BrightGreen));
}

void Game::showPrologueScreen() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("那场灾难之后，你开始听见地底的回声。", UI::Color::BrightCyan));
    frame.addBlank();
    frame.addCentered("不合时令的雪落下，地面随后震动，远处又映出火光。");
    frame.addCentered("你在混乱中受了伤。昏迷时，梦见一座沉入地下的王都。");
    frame.addCentered("醒来后，旧身份证明已经在灾害中遗失。");
    frame.addCentered("你只记得一个方向，和一段不属于自己的回声。");
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

bool Game::startFromGate() {
    while (true) {
        renderRoad();

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
        if (choice == 0) {
            showRoadLeaveScreen();
            return false;
        }
        if (choice == -1) {
            return false;
        }

        if (showGatekeeper()) {
            return true;
        }
    }
}

void Game::showRoadLeaveScreen() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("不过，你离开了。", UI::Color::Gray));
    frame.addBlank();
    frame.addCentered("你与一些神奇的故事也分开了。");
    frame.addCentered("这似乎是命中注定的。");
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车离开 > ", UI::Color::Green));
}

void Game::renderRoad() const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("城门外 · 荒路", UI::Color::BrightCyan),
        UI::paint("身份牌：待检验", UI::Color::Gray),
        40);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::defaultLines();
    const std::vector<std::string>& gateArt = Sprites::door();
    for (int index = 0; index < 4; ++index) {
        const std::string gateLine =
            index < 3
                ? UI::paint(gateArt[static_cast<std::size_t>(index)], UI::Color::Yellow)
                : UI::paint("（我是城门）", UI::Color::Gray);
        frame.addLine(sceneRow(playerArt[static_cast<std::size_t>(index)], 8, gateLine, 50));
    }
    frame.addLine(UI::paint(floorLine(60), UI::Color::Gray));
    frame.addLine(sceneRow(
        UI::paint("旅人", UI::Color::BrightCyan), 8,
        UI::paint("城门", UI::Color::Yellow), 50));

    frame.addBlank();
    frame.addWrapped("风卷着灰烬吹过荒路。这里刚经历过一场无法解释的灾变。", "  ");
    frame.addWrapped("城门半塌，门后没有灯。一个残破的机械仍守着入口。", "  ");
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 进城   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开游戏");
    frame.render();
}

bool Game::showGatekeeper() {
    std::string message;

    const SaveSummary introSummary = SaveGame::peek();
    const std::vector<std::string> dialogue = gateDialogue(introSummary);
    for (const std::string& line : dialogue) {
        renderGate(introSummary, "", line, false);
        UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
    }

    while (true) {
        const SaveSummary summary = SaveGame::peek();
        renderGate(summary, message, "", true);

        if (summary.valid) {
            const int choice = UI::readChoice(
                UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 2);
            if (choice == 0 || choice == -1) {
                return false;
            }

            if (choice == 1) {
                std::string error;
                std::unique_ptr<Player> loaded = SaveGame::load(error);
                if (loaded) {
                    player_ = std::move(*loaded);
                    return true;
                }
                message = "身份牌读取失败：" + error;
                continue;
            }

            std::string registerMessage;
            if (registerNewIdentity(registerMessage)) {
                return true;
            }
            message = registerMessage;
            continue;
        }

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
        if (choice == 0 || choice == -1) {
            return false;
        }

        std::string registerMessage;
        if (registerNewIdentity(registerMessage)) {
            return true;
        }
        message = registerMessage;
    }
}

void Game::renderGate(const SaveSummary& summary, const std::string& message,
                      const std::string& dialogueLine, bool showMenu) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("城门 · 登记处", UI::Color::BrightCyan),
        UI::paint("身份牌检测", UI::Color::Gray),
        40);
    frame.addDivider();
    frame.addBlank();

    std::string cardStatus;
    std::string cardName;
    std::string cardGender;
    std::string cardSpecies;
    std::string cardLevel;
    std::string cardGold;

    if (summary.valid) {
        cardStatus = "临时";
        cardName = summary.name;
        cardGender = summary.gender;
        cardSpecies = summary.species;
        cardLevel = "Lv." + std::to_string(summary.level);
        cardGold = std::to_string(summary.gold);
    } else if (summary.fileExists) {
        cardStatus = "损坏";
        cardName = "无法识别";
        cardGender = "??";
        cardSpecies = "??";
        cardLevel = "--";
        cardGold = "--";
    } else {
        cardStatus = "无";
        cardName = "未登记";
        cardGender = "未登记";
        cardSpecies = "未登记";
        cardLevel = "--";
        cardGold = "--";
    }

    std::vector<std::string> rightRows;
    rightRows.push_back(std::string());
    rightRows.push_back(UI::center(cardBorder(true), 34));
    rightRows.push_back(UI::center(cardLine("身份牌  " + cardStatus), 34));
    rightRows.push_back(UI::center(cardLine("姓名  " + cardName), 34));
    rightRows.push_back(UI::center(cardLine("性别  " + cardGender), 34));
    rightRows.push_back(UI::center(cardLine("物种  " + cardSpecies), 34));
    rightRows.push_back(UI::center(cardLine("等级  " + cardLevel), 34));
    rightRows.push_back(UI::center(cardLine("金币  " + cardGold), 34));
    rightRows.push_back(UI::center(cardBorder(false), 34));

    const std::vector<std::string>& robot = Sprites::gatekeeper();
    const int robotRows = static_cast<int>(robot.size()) + 1;
    const int rowCount = std::max(robotRows, static_cast<int>(rightRows.size()));

    for (int row = 0; row < rowCount; ++row) {
        std::string left;
        if (row < static_cast<int>(robot.size())) {
            left = UI::center(UI::paint(robot[static_cast<std::size_t>(row)],
                                        UI::Color::Gray), 30);
        } else if (row == static_cast<int>(robot.size())) {
            left = UI::center(UI::paint("机械门卫", UI::Color::Yellow), 30);
        }

        const std::string right =
            row < static_cast<int>(rightRows.size())
                ? rightRows[static_cast<std::size_t>(row)]
                : std::string();
        frame.addSplit(left, right, 30);
    }

    frame.addBlank();
    if (!dialogueLine.empty()) {
        frame.addWrapped(dialogueLine, "  ");
    }
    if (!message.empty()) {
        frame.addWrapped(message, "  ");
    }
    frame.addBlank();
    frame.addDivider();

    if (showMenu) {
        if (summary.valid) {
            frame.addLine(
                UI::paint("[1]", UI::Color::BrightYellow) + " 这是我的身份牌   " +
                UI::paint("[2]", UI::Color::BrightYellow) + " 不，我忘记了自己是谁   " +
                UI::paint("[0]", UI::Color::BrightYellow) + " 返回荒路");
        } else if (summary.fileExists) {
            frame.addLine(
                UI::paint("[1]", UI::Color::BrightYellow) + " 重新登记   " +
                UI::paint("[0]", UI::Color::BrightYellow) + " 返回荒路");
        } else {
            frame.addLine(
                UI::paint("[1]", UI::Color::BrightYellow) + " 登记身份   " +
                UI::paint("[0]", UI::Color::BrightYellow) + " 返回荒路");
        }
    }
    frame.render();
}

std::vector<std::string> Game::gateDialogue(const SaveSummary& summary) const {
    if (summary.valid) {
        return {
            "机械门卫：>「出示临时身份牌」<",
            "「你的旧身份证明缺失，这张牌子只能保证你进城。」",
            "「出示它，我就放你进去。」"
        };
    }
    if (summary.fileExists) {
        return {
            "机械门卫：>「临时身份牌损坏」<",
            "「你只能重新登记一个临时身份。」",
            "「这次别再弄丢了。」"
        };
    }
    return {
        "机械门卫：>「没有身份牌」<",
        "「旧身份证明缺失。登记一个临时身份。」",
        "「名字、性别、物种由你自报。」"
    };
}

bool Game::registerNewIdentity(std::string& message) {
    const std::string name = readIdentityText("  登记名字（直接回车取消） > ", 10);
    if (name.empty()) {
        message = "登记取消。";
        return false;
    }

    std::string gender;
    if (!chooseIdentityGender(gender)) {
        message = "登记取消。";
        return false;
    }

    std::string species;
    if (!chooseIdentitySpecies(species)) {
        message = "登记取消。";
        return false;
    }

    Player newPlayer(name);
    newPlayer.setIdentity(gender, species);
    newPlayer.getInventory().add(
        std::make_unique<Potion>("红药水", 8, Potion::Effect::RestoreHp, 15));
    newPlayer.getInventory().add(
        std::make_unique<Potion>("红药水", 8, Potion::Effect::RestoreHp, 15));

    std::string error;
    if (!SaveGame::save(newPlayer, error)) {
        message = "登记失败：" + error;
        return false;
    }

    saveStoryStage(newPlayer.getName(), 0);

    player_ = std::move(newPlayer);
    return true;
}

std::string Game::readIdentityText(const std::string& prompt, int maxWidth) const {
    while (true) {
        std::cout << UI::paint(prompt, UI::Color::BrightGreen);

        std::string line;
        if (!std::getline(std::cin, line)) {
            return std::string();
        }

        const std::string text = trimText(line);
        if (text.empty()) {
            return std::string();
        }
        if (UI::displayWidth(text) > maxWidth) {
            std::cout << "  内容太长，最多 " << maxWidth << " 个显示列。\n";
            continue;
        }
        return text;
    }
}

bool Game::chooseIdentityGender(std::string& gender) const {
    while (true) {
        std::cout << "\n  性别登记：\n";
        std::cout << "  1 男    2 女    3 不明    4 自定义    0 取消\n";

        const int choice = UI::readChoice(
            UI::paint("  选择性别 > ", UI::Color::BrightGreen), 0, 4);
        if (choice == 0 || choice == -1) {
            return false;
        }

        switch (choice) {
            case 1:
                gender = "男";
                return true;
            case 2:
                gender = "女";
                return true;
            case 3:
                gender = "不明";
                return true;
            case 4: {
                const std::string custom =
                    readIdentityText("  自定义性别（留空返回菜单） > ", 6);
                if (custom.empty()) {
                    std::cout << "  自定义性别不能为空。\n";
                    continue;
                }
                gender = custom;
                return true;
            }
            default:
                break;
        }
    }
}

bool Game::chooseIdentitySpecies(std::string& species) const {
    while (true) {
        std::cout << "\n  物种登记：\n";
        std::cout << "  1 人类   2 精灵   3 兽人   4 机械   5 亡灵   6 自定义   0 取消\n";

        const int choice = UI::readChoice(
            UI::paint("  选择物种 > ", UI::Color::BrightGreen), 0, 6);
        if (choice == 0 || choice == -1) {
            return false;
        }

        switch (choice) {
            case 1:
                species = "人类";
                return true;
            case 2:
                species = "精灵";
                return true;
            case 3:
                species = "兽人";
                return true;
            case 4:
                species = "机械";
                return true;
            case 5:
                species = "亡灵";
                return true;
            case 6: {
                const std::string custom =
                    readIdentityText("  自定义物种（留空返回菜单） > ", 10);
                if (custom.empty()) {
                    std::cout << "  自定义物种不能为空。\n";
                    continue;
                }
                species = custom;
                return true;
            }
            default:
                break;
        }
    }
}

void Game::showCityEntryNarration() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("城门在你身后合拢。", UI::Color::BrightCyan));
    frame.addBlank();
    frame.addCentered("街上留着低头行走的人影，看不清脸，像凝固的最后一幕。");
    frame.addCentered("这里的人用身份牌代替脸。假身份、抢劫和失踪，都没人过问。");
    frame.addCentered("门卫的识别声已停，胸前的临时身份牌却仍透着凉意。");
    frame.addBlank();
    frame.addCentered(UI::paint(player_.getName() + "，欢迎来到沉没的王都。",
                                UI::Color::BrightYellow));
    frame.addCentered(UI::paint("身份牌：" + player_.getName() + " · " +
                                player_.getGender() + " · " + player_.getSpecies(),
                                UI::Color::Gray));
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showEnterRuinsCutscene() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("你推开沉重的石门。", UI::Color::BrightCyan));
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& doorArt = Sprites::door();
    for (int index = 0; index < 4; ++index) {
        const std::string doorLine =
            index < 3 ? UI::paint(doorArt[static_cast<std::size_t>(index)], UI::Color::Yellow)
                      : std::string();
        frame.addColumns(
            UI::center(playerArt[static_cast<std::size_t>(index)], 32),
            UI::center(doorLine, 32),
            32);
    }

    frame.addBlank();
    frame.addCentered("潮湿的冷风从地底吹来，火把猛地朝深处倾斜。");
    frame.addCentered("那阵熟悉的回声再次响起。你沿石阶走进沉没的王都。");
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

bool Game::showStoryBeat() {
    static const std::vector<std::vector<std::string>> clues = {
        {"你停在一面裂开的石墙前。指尖触到纹路时，胸口的疼痛忽然与地下的震动重合。",
         "这不是一句话，更像一段断续的求援信号。你不知道它来自哪里。"},
        {"旧粮仓的墙上刻着灾后记录：盛夏降雪，粮食冻结，城门封闭。",
         "几份记录的日期彼此不相邻，也找不到固定间隔。雪灾从未按季节或周期到来。"},
        {"观测站的地图把裂缝标向城市地下深处。震动先从那里出现，再向街区扩散。",
         "旧记录称地脉压力正在升高，却没有人知道是什么推动着它。"},
        {"焦黑的工坊里，烧熔的管道通向一座被封住的核心设施。残存铭文写着：地心炉。",
         "旧王朝曾用它引导地脉能源。记录同时警告，炉心的热量与压力无法稳定控制。"},
        {"一页王室日志写道：发明者本人下令停止扩建，并亲自进入核心区域。",
         "最后一条完整记录只有一句：‘炉火已熄。不要让后来者重新点燃它。’"},
        {"你把三处灾区的记录按时间拼在一起：寒冷、震动、火焰都从地下设施异常后出现。",
         "灾难的间隔并不固定。新王国的文书只记载救灾和撤离，没有提到地心炉——他们并不知道它存在。"},
        {"一道石门后，四尊骑士形态的守护机械仍在巡行。它们并不追逐地表的人，只阻挡通向核心的道路。",
         "旧刻文记录了控制、防卫、配给与封存四种职责。后世把这些身影称为天启四骑士。"},
        {"核心门前的识别装置终于读取出完整档案：你是那位国王的后裔。",
         "地心炉没有意志。封存结构受损后，它只是沿着国王留下的血脉权限发出求援信号。你继承的是进入核心的资格，不是必须服从的命令。"}
    };

    if (storyStage_ < kStoryClueCount) {
        UI::clearScreen();
        Frame frame;
        frame.addBlank();
        frame.addCentered(UI::paint("遗迹记录 · " + std::to_string(storyStage_ + 1) +
                                    "/" + std::to_string(kStoryClueCount),
                                    UI::Color::BrightCyan));
        frame.addBlank();
        for (const std::string& line : clues[static_cast<std::size_t>(storyStage_)]) {
            frame.addWrapped(line, "  ");
            frame.addBlank();
        }
        frame.render();
        UI::waitForEnter(UI::paint("  按回车继续探索 > ", UI::Color::BrightGreen));
        ++storyStage_;
        saveStoryStage(player_.getName(), storyStage_);
        return false;
    }

    if (storyStage_ == kStoryClueCount) {
        UI::clearScreen();
        Frame frame;
        frame.addBlank();
        frame.addCentered(UI::paint("地心炉 · 最后的选择", UI::Color::BrightYellow, true));
        frame.addBlank();
        frame.addWrapped("炉心仍在紊乱。先祖曾选择熄灭它，但那是他的决定。现在，选择权在你手中。", "  ");
        frame.addBlank();
        frame.addLine("[1] 再次熄灭炉心：止住灾害，封存王都");
        frame.addLine("[2] 尝试稳定炉心：保留遗迹，但需要持续维护");
        frame.addLine("[3] 摧毁炉心：彻底断绝风险，承受最后一次地震");
        frame.addLine("[4] 拒绝介入：离开核心，未来仍可能再次失控");
        frame.addLine("[0] 暂时离开");
        frame.addBlank();
        frame.render();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 4);
        if (choice == 0 || choice == -1) {
            return true;
        }

        std::vector<std::string> ending;
        switch (choice) {
            case 1:
                ending = {"你执行了熄灭程序。炉心的脉动渐渐停下，四尊守护机械也垂下了头。",
                          "沉没王都仍留在地下，但三灾不再从这里醒来。"};
                break;
            case 2:
                ending = {"你降低炉心输出，重新接上部分封存结构。震动暂时平息，核心仍需要有人维护。",
                          "你决定带着这段历史回到营地，与其他人商量接下来的守护方式。"};
                break;
            case 3:
                ending = {"你破坏了核心。最后一次震动撕开地层，旧王都的深处彻底坍塌。",
                          "炉心不再运行，代价是这座城市与它的秘密一同埋入黑暗。"};
                break;
            default:
                ending = {"你转身离开，没有替血脉或先祖作出决定。",
                          "召唤声渐远，但炉心的故障尚未解决。未来仍留着不确定的回响。"};
                break;
        }
        storyStage_ = kStoryClueCount + 1;
        saveStoryStage(player_.getName(), storyStage_);
        showResult("旅  程  终  章", ending, UI::Color::BrightYellow);
        return true;
    }

    return false;
}

void Game::renderCamp(const std::string& message) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("城内 · 营地", UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "    背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        44);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& fireArt = Sprites::campfire();
    for (int index = 0; index < 4; ++index) {
        frame.addColumns(
            UI::center(playerArt[static_cast<std::size_t>(index)], 32),
            UI::center(UI::paint(fireArt[static_cast<std::size_t>(index)],
                                 UI::Color::BrightYellow), 32),
            32);
    }
    frame.addColumns(
        UI::center(UI::paint(player_.getName(), UI::Color::BrightCyan), 32),
        UI::center(UI::paint("火堆", UI::Color::Yellow), 32),
        32);

    frame.addBlank();

    frame.addLine(UI::paint(player_.getName(), UI::Color::BrightCyan) +
                  "  Lv." + std::to_string(player_.getLevel()));
    frame.addLine(
        "HP " + UI::bar(player_.getHp(), player_.getMaxHp(), 20,
                        UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()));
    frame.addLine(
        "MP " + UI::bar(player_.getMp(), player_.getMaxMp(), 20, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()));
    frame.addLine(
        "EXP " + UI::bar(player_.getExperience(), player_.getExperienceToNextLevel(), 20,
                         UI::Color::BrightMagenta) +
        "  " + std::to_string(player_.getExperience()) + "/" +
        std::to_string(player_.getExperienceToNextLevel()));

    frame.addDivider();
    frame.addLine(UI::paint("营地记事", UI::Color::Gray));
    if (message.empty()) {
        frame.addLine("火堆噼啪作响，遗迹入口就在不远处。");
    } else {
        frame.addWrapped(message, "· ");
    }

    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 查看状态   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 背包   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 商店");
    frame.addLine(
        UI::paint("[4]", UI::Color::BrightYellow) + " 进入遗迹   " +
        UI::paint("[5]", UI::Color::BrightYellow) + " 休息   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开（自动存档）");

    frame.render();
}

void Game::renderTravel(int progress, int playerColumn) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("地下遗迹 · 第一层", UI::Color::BrightCyan),
        UI::paint("探索进度 " + std::to_string(progress) + "/3", UI::Color::Gray),
        44);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& doorArt = Sprites::door();

    frame.addLine(sceneRow(
        UI::paint("火", UI::Color::Yellow), 12,
        UI::paint("火", UI::Color::Yellow), 44));

    for (int index = 0; index < 4; ++index) {
        const std::string doorLine =
            index < 3 ? UI::paint(doorArt[static_cast<std::size_t>(index)], UI::Color::Yellow)
                      : std::string();
        frame.addLine(sceneRow(playerArt[static_cast<std::size_t>(index)], playerColumn,
                               doorLine, 56));
    }

    frame.addLine(UI::paint(floorLine(60), UI::Color::Gray));
    frame.addLine(sceneRow(
        UI::paint(player_.getName(), UI::Color::BrightCyan), playerColumn,
        UI::paint("木门", UI::Color::Yellow), 56));

    frame.addBlank();
    frame.addWrapped(travelFlavor(progress), "  ");
    frame.addBlank();
    frame.addDivider();

    const std::string forwardLabel = (progress >= 3) ? "靠近木门" : "继续前进";
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " " + forwardLabel + "   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 查看状态   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 返回营地");

    frame.render();
}

void Game::animateWalk(int fromColumn, int toColumn, int progress) const {
    if (!UI::isInteractive()) {
        renderTravel(progress, toColumn);
        return;
    }

    constexpr int kFrames = 5;
    for (int frame = 1; frame <= kFrames; ++frame) {
        const int column = fromColumn + (toColumn - fromColumn) * frame / kFrames;
        renderTravel(progress, column);
        UI::sleepFor(65);
    }
}

void Game::triggerTravelDisaster() {
    const DisasterEvent event = Disaster::generate(storyStage_);

    const int hpBefore = player_.getHp();
    const int damage = std::min(event.hpDamage, std::max(0, hpBefore - 1));
    const int hpLost = player_.takeDamage(damage);

    int mpLost = 0;
    if (event.mpDamage > 0) {
        mpLost = std::min(event.mpDamage, player_.getMp());
        if (mpLost > 0) {
            player_.spendMp(mpLost);
        }
    }

    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("天灾 · " + event.name,
                                DisasterVisual::tint(event.type), true));
    frame.addBlank();
    frame.addWrapped(event.description, "  ");
    frame.addBlank();

    std::string loss = "你损失了 " + std::to_string(hpLost) + " 点 HP";
    if (mpLost > 0) {
        loss += "，" + std::to_string(mpLost) + " 点 MP";
    }
    loss += "。";
    frame.addCentered(loss);
    if (hpLost == 0) {
        frame.addCentered("你勉强撑住了。");
    }

    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showStatusScreen() const {
    UI::clearScreen();

    Frame frame;
    frame.addLine(UI::paint("角色状态", UI::Color::BrightCyan));
    frame.addDivider();

    for (const std::string& line : PlayerSprite::lines(player_)) {
        frame.addCentered(line);
    }
    frame.addCentered(UI::paint(player_.getName(), UI::Color::BrightCyan));
    frame.addCentered(UI::paint("我帅的要命，真的 ㄟ", UI::Color::Gray));
    frame.addDivider();

    frame.addLine("姓名 " + player_.getName() +
                  "   性别 " + player_.getGender() +
                  "   物种 " + player_.getSpecies() +
                  "   等级 Lv." + std::to_string(player_.getLevel()));
    frame.addLine(
        "HP   " + UI::bar(player_.getHp(), player_.getMaxHp(), 20,
                          UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()));
    frame.addLine(
        "MP   " + UI::bar(player_.getMp(), player_.getMaxMp(), 20, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()));
    frame.addLine(
        "EXP  " + UI::bar(player_.getExperience(), player_.getExperienceToNextLevel(), 20,
                          UI::Color::BrightMagenta) +
        "  " + std::to_string(player_.getExperience()) + "/" +
        std::to_string(player_.getExperienceToNextLevel()));

    int attackBonus = 0;
    int defenseBonus = 0;
    if (player_.getWeapon() != nullptr) {
        attackBonus += player_.getWeapon()->getAttackBonus();
    }
    if (player_.getArmor() != nullptr) {
        defenseBonus += player_.getArmor()->getDefenseBonus();
    }
    if (player_.getAccessory() != nullptr) {
        attackBonus += player_.getAccessory()->getAttackBonus();
        defenseBonus += player_.getAccessory()->getDefenseBonus();
    }

    frame.addLine("攻击   " + std::to_string(player_.getAttack()) +
                  "  ( + " + std::to_string(attackBonus) + " )" +
                  "    防御   " + std::to_string(player_.getDefense()) +
                  "  ( + " + std::to_string(defenseBonus) + " )");

    const int criticalRate = static_cast<int>(Character::kCriticalChance * 100.0 + 0.5);
    const int criticalDamage =
        static_cast<int>((Character::kCriticalMultiplier - 1.0) * 100.0 + 0.5);
    frame.addLine("基础暴击率   " + std::to_string(criticalRate) +
                  "%    暴击伤害   " + std::to_string(criticalDamage) +
                  "%（伤害 ×1.5）");

    frame.addLine("金币   " + std::to_string(player_.getGold()) +
                  "    药水   " + std::to_string(player_.getInventory().countPotions()) + " 瓶");
    frame.addLine("武器   " + equipmentLabel(player_.getWeapon()));
    frame.addLine("防具   " + equipmentLabel(player_.getArmor()));
    frame.addLine("饰品   " + equipmentLabel(player_.getAccessory()));

    const int remaining = std::max(0, player_.getExperienceToNextLevel() - player_.getExperience());
    frame.addWrapped("距离下一次升级还需要 " + std::to_string(remaining) + " 点经验。");
    frame.render();

    UI::waitForEnter(UI::paint("  按回车返回 > ", UI::Color::BrightGreen));
}

void Game::showShop() {
    Shop shop;
    std::string message = "看看货吧，都是遗迹里少见的好东西。";

    while (true) {
        renderShop(shop, message);

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 3);
        switch (choice) {
            case 1:
                message = buyFromShop(shop);
                break;
            case 2:
                message = sellToShop();
                break;
            case 3:
                showInventoryScreen();
                message = "还要买点什么吗？";
                break;
            case 0:
            case -1:
                return;
            default:
                break;
        }
    }
}

void Game::renderShop(const Shop& shop, const std::string& message) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("地下集市", UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "   背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        30);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string>& merchant = Sprites::merchant();
    constexpr int kMerchantWidth = 22;
    const int rowCount = std::max(5, shop.size());

    for (int row = 0; row < rowCount; ++row) {
        std::string left;
        if (row < 3) {
            left = UI::center(UI::paint(merchant[static_cast<std::size_t>(row)],
                                        UI::Color::Yellow), kMerchantWidth);
        } else if (row == 3) {
            left = UI::center(UI::paint("═══════", UI::Color::Gray), kMerchantWidth);
        } else if (row == 4) {
            left = UI::center(UI::paint("商人", UI::Color::Yellow), kMerchantWidth);
        }

        std::string right;
        if (row < shop.size()) {
            right = shopItemLine(row + 1, shop[row]);
        }
        frame.addSplit(left, right, kMerchantWidth);
    }

    frame.addBlank();
    frame.addWrapped("商人：" + message, "  ");
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 购买   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 出售   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 查看背包   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
    frame.render();
}

std::string Game::buyFromShop(Shop& shop) {
    const int index = UI::readChoice("  商品编号（0 取消） > ", 0, shop.size());
    if (index == 0 || index == -1) {
        return "慢慢挑。";
    }

    Item& item = shop[index - 1];
    if (player_.getInventory().full()) {
        return "你的背包满了，先整理一下吧。";
    }
    if (!player_.getWallet().canAfford(item.getPrice())) {
        return "金币不够，先去遗迹里转转吧。";
    }

    const std::string name = item.getName();
    const int price = item.getPrice();
    std::unique_ptr<Item> copy = item.clone();

    player_.getWallet() -= price;
    player_ + std::move(copy);

    return "成交，" + name + " 归你了。";
}

std::string Game::sellToShop() {
    Inventory& inventory = player_.getInventory();
    if (inventory.empty()) {
        return "背包是空的，没什么可卖的。";
    }

    renderInventory("出售物品", "选择要出售的物品。", true);
    const int index = UI::readChoice("  物品编号（0 取消） > ", 0, inventory.size());
    if (index == 0 || index == -1) {
        return "再看看货吧。";
    }

    Item* item = inventory.getItem(index - 1);
    if (item == nullptr) {
        return "这件东西不在了。";
    }

    const std::string name = item->getName();
    const int price = item->getSellPrice();

    inventory.takeItem(index - 1);
    player_.getWallet() += price;

    return "这件" + name + "，我出 " + std::to_string(price) + " 金币。";
}

void Game::showInventoryScreen() {
    std::string message = "选择物品进行使用或装备。";

    while (true) {
        renderInventory("背包", message, false);

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 4);
        if (choice == 0 || choice == -1) {
            return;
        }

        switch (choice) {
            case 1: {
                if (player_.getInventory().empty()) {
                    message = "背包是空的。";
                    break;
                }

                const int itemIndex = UI::readChoice(
                    "  物品编号（0 取消） > ", 0, player_.getInventory().size());
                if (itemIndex == 0 || itemIndex == -1) {
                    message = "选择物品进行使用或装备。";
                    break;
                }

                Item* item = player_.getInventory().getItem(itemIndex - 1);
                if (item == nullptr) {
                    message = "物品不存在。";
                    break;
                }

                if (item->getKind() == ItemKind::Consumable) {
                    std::string useMessage;
                    player_.getInventory().useItem(itemIndex - 1, player_, useMessage);
                    message = useMessage;
                } else if (item->getKind() == ItemKind::Material) {
                    message = "材料不能直接使用，可以卖给商人。";
                } else {
                    std::string equipMessage;
                    player_.equipFromInventory(itemIndex - 1, equipMessage);
                    message = equipMessage;
                }
                break;
            }

            case 2: {
                std::string unequipMessage;
                player_.unequip(EquipmentSlot::Weapon, unequipMessage);
                message = unequipMessage;
                break;
            }

            case 3: {
                std::string unequipMessage;
                player_.unequip(EquipmentSlot::Armor, unequipMessage);
                message = unequipMessage;
                break;
            }

            case 4: {
                std::string unequipMessage;
                player_.unequip(EquipmentSlot::Accessory, unequipMessage);
                message = unequipMessage;
                break;
            }

            default:
                break;
        }
    }
}

void Game::renderInventory(const std::string& title,
                           const std::string& message,
                           bool selling) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint(title, UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "   背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        30);
    frame.addDivider();
    frame.addBlank();

    std::string weaponText = "武器   " + equipmentLabel(player_.getWeapon());
    std::string armorText = "防具   " + equipmentLabel(player_.getArmor());
    std::string accessoryText = "饰品   " + equipmentLabel(player_.getAccessory());
    if (selling && player_.getWeapon() != nullptr) {
        weaponText += "（已装备）";
    }
    if (selling && player_.getArmor() != nullptr) {
        armorText += "（已装备）";
    }
    if (selling && player_.getAccessory() != nullptr) {
        accessoryText += "（已装备）";
    }
    frame.addLine(weaponText);
    frame.addLine(armorText);
    frame.addLine(accessoryText);
    frame.addDivider();

    const std::vector<std::unique_ptr<Item>>& items = player_.getInventory().getItems();
    if (items.empty()) {
        frame.addLine("背包是空的。");
    } else {
        for (int index = 0; index < static_cast<int>(items.size()); ++index) {
            frame.addLine(inventoryItemLine(
                index + 1, *items[static_cast<std::size_t>(index)], selling));
        }
    }

    frame.addDivider();
    if (!message.empty()) {
        frame.addWrapped(message, "· ");
    } else {
        frame.addLine(" ");
    }
    frame.addDivider();

    if (selling) {
        frame.addLine("输入物品编号出售，0 返回商店");
    } else {
        frame.addLine(
            UI::paint("[1]", UI::Color::BrightYellow) + " 使用/装备   " +
            UI::paint("[2]", UI::Color::BrightYellow) + " 卸下武器   " +
            UI::paint("[3]", UI::Color::BrightYellow) + " 卸下防具");
        frame.addLine(
            UI::paint("[4]", UI::Color::BrightYellow) + " 卸下饰品   " +
            UI::paint("[0]", UI::Color::BrightYellow) + " 返回");
    }
    frame.render();
}

void Game::showEncounterCutscene(const Monster& monster) const {
    UI::clearScreen();

    const UI::Color tint = MonsterVisual::tint(monster.getColor());

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("遭遇 · " + monster.getName(), tint));
    frame.addBlank();

    for (const std::string& line : monster.getArt()) {
        frame.addCentered(UI::paint(line, tint));
    }

    frame.addBlank();
    frame.addWrapped(monster.getEncounterText(), "  ");
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车迎战 > ", UI::Color::BrightRed));
}

std::unique_ptr<Monster> Game::createRandomMonster() const {
    const int roll = Random::integer(0, 99);
    if (roll < 40) {
        return std::make_unique<RuinsRat>();
    }
    if (roll < 75) {
        const SlimeElement element = Random::chance(0.5)
                                          ? SlimeElement::Water
                                          : SlimeElement::Fire;
        return std::make_unique<Slime>(element);
    }
    return std::make_unique<Goblin>();
}

void Game::showReturnCutscene() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("你沿着原路返回营地。", UI::Color::BrightCyan));
    frame.addBlank();

    for (const std::string& line : PlayerSprite::lines(player_)) {
        frame.addCentered(line);
    }
    frame.addCentered(UI::paint(player_.getName(), UI::Color::BrightCyan));

    frame.addBlank();
    frame.addCentered("火堆的光从营地那头照了过来。");
    frame.addBlank();
    frame.render();

    UI::sleepFor(900);
}

void Game::showResult(const std::string& title,
                      const std::vector<std::string>& lines,
                      UI::Color color) const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint(title, color, true));
    frame.addBlank();
    for (const std::string& line : lines) {
        frame.addCentered(line);
    }
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车返回营地 > ", UI::Color::BrightGreen));
}

std::string Game::explore() {
    showEnterRuinsCutscene();
    if (showStoryBeat()) {
        showReturnCutscene();
        return "你离开了核心区域，回到营地。地心炉的回声仍在身后。";
    }

    int progress = 1;
    int playerColumn = 8;
    std::unique_ptr<Monster> monster;
    bool disasterShown = false;

    while (true) {
        renderTravel(progress, playerColumn);

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 1, 3);
        if (choice == 2) {
            showStatusScreen();
            continue;
        }
        if (choice == 3 || choice == -1) {
            showReturnCutscene();
            return "你沿着来路退回营地。";
        }

        if (progress < 3) {
            const int targetColumn = playerColumn + 16;
            animateWalk(playerColumn, targetColumn, progress);
            playerColumn = targetColumn;
            ++progress;
            if (!disasterShown && Random::chance(0.45)) {
                triggerTravelDisaster();
                disasterShown = true;
            }
        } else {
            animateWalk(playerColumn, 46, progress);
            playerColumn = 46;
            UI::sleepFor(200);
            if (!disasterShown) {
                triggerTravelDisaster();
                disasterShown = true;
            }
            monster = createRandomMonster();
            showEncounterCutscene(*monster);
            break;
        }
    }

    Battle battle(player_, storyStage_);
    const BattleResult result = battle.fight(*monster);

    if (result == BattleResult::Victory) {
        const int levelsGained = player_.gainExperience(monster->getExpReward());
        player_.gainGold(monster->getGoldReward());

        std::vector<std::string> lines;
        lines.push_back("你击败了" + monster->getName() + "。");
        lines.push_back("获得 " + std::to_string(monster->getGoldReward()) + " 金币、" +
                        std::to_string(monster->getExpReward()) + " 经验。");

        std::unique_ptr<Item> drop = monster->rollDrop();
        if (drop) {
            const std::string dropName = drop->getName();
            if (player_.getInventory().canAdd()) {
                player_ + std::move(drop);
                lines.push_back("掉落：" + dropName + "。");
            } else {
                lines.push_back("掉落：" + dropName + "，但背包已满。");
            }
        }

        if (levelsGained > 0) {
            lines.push_back("升级！你现在是 Lv." + std::to_string(player_.getLevel()) + "。");
            lines.push_back("HP/MP 已恢复，属性提升。");
        }
        showResult("胜  利", lines, UI::Color::BrightYellow);
        showReturnCutscene();
        return "你沿着原路回到营地，火堆还在燃烧。";
    }

    if (result == BattleResult::Defeat) {
        const int lostGold = std::min(10, player_.getGold());
        if (lostGold > 0) {
            player_.spendGold(lostGold);
        }
        player_.restoreAll();

        std::vector<std::string> lines;
        lines.push_back("你被" + monster->getName() + "击倒。");
        lines.push_back("醒来时，你回到了营地。");
        if (lostGold > 0) {
            lines.push_back("丢失了 " + std::to_string(lostGold) + " 枚金币。");
        }
        showResult("战  败", lines, UI::Color::BrightRed);
        return "你在营地休整，准备再次出发。";
    }

    showReturnCutscene();
    return "你退回营地，拍了拍身上的灰。";
}

void Game::renderFarewell(const std::string& warning) const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();
    frame.addCentered(UI::paint("旅  程  暂  告  一  段  落", UI::Color::BrightYellow, true));
    frame.addBlank();
    frame.addCentered("你离开了沉没王都。");
    frame.addCentered("灰烬仍浮在风中，王都的回声渐渐远去。");
    frame.addBlank();
    frame.addCentered("最终等级 Lv." + std::to_string(player_.getLevel()) +
                      "    金币 " + std::to_string(player_.getGold()));
    if (!warning.empty()) {
        frame.addBlank();
        frame.addWrapped(warning, "  ");
    }
    frame.addBlank();
    frame.render();
}
