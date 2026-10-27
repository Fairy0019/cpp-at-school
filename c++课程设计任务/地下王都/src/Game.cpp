#include "Game.h"

#include "Battle.h"
#include "Disaster.h"
#include "DisasterVisual.h"
#include "Frame.h"
#include "Item.h"
#include "Mimic.h"
#include "Monster.h"
#include "MonsterVisual.h"
#include "PlayerSprite.h"
#include "Random.h"
#include "RuinsRat.h"
#include "Shop.h"
#include "Slime.h"
#include "Sprites.h"
#include "Goblin.h"
#include "Guardian.h"
#include "UI.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kStoryProgressFile = "story_progress.sav";
constexpr int kStoryClueCount = 8;

struct StoryProgress {
    int stage = 0;
    int ending = 0;
};

StoryProgress loadStoryProgress(const std::string& playerName) {
    std::ifstream input(kStoryProgressFile);
    std::string savedName;
    int stage = 0;
    if (!std::getline(input, savedName) || savedName != "name=" + playerName ||
        !(input >> stage) || stage < 0 || stage > kStoryClueCount + 1) {
        return StoryProgress{};
    }

    StoryProgress progress;
    progress.stage = stage;

    std::string line;
    while (std::getline(input, line)) {
        if (line.rfind("ending=", 0) == 0) {
            try {
                const int ending = std::stoi(line.substr(7));
                if (ending >= 1 && ending <= 4) {
                    progress.ending = ending;
                }
            } catch (const std::exception&) {
                // 旧存档或损坏字段：保留默认结局标记。
            }
        }
    }
    return progress;
}

void saveStoryProgress(const std::string& playerName, int stage, int ending) {
    std::ofstream output(kStoryProgressFile, std::ios::out | std::ios::trunc);
    if (output) {
        output << "name=" << playerName << "\n" << stage << "\n";
        if (ending >= 1 && ending <= 4) {
            output << "ending=" << ending << "\n";
        }
    }
}

// 每一幕对应一个探索区域。线索在区域尽头发现，而不是一进城就自动弹出。
struct AreaInfo {
    std::string act;
    std::string name;
    std::vector<std::string> intro;
    std::vector<std::string> travel;
};

const std::vector<AreaInfo>& areas() {
    static const std::vector<AreaInfo> data = {
        {
            "第一幕", "裂缝前厅",
            {
                "石门在你身后合拢。前厅的墙面被一道新裂缝劈开。",
                "裂缝深处亮着微弱的红光，像有什么东西在下面呼吸。",
                "你听见回声。它不再只是方向，而像在等一个回答。"
            },
            {
                "你举着火把沿石阶向下。\n胸口的刺痛和地底的脉动错开半拍。",
                "旧王都的纹章被灰烬盖住一半。\n纹章上的炉形图案缺了一角。",
                "前厅尽头没有门，只有一道向下的裂缝。\n红光从裂缝深处透上来。"
            }
        },
        {
            "第二幕", "冻结粮仓",
            {
                "西区的空气突然变冷。盛夏的霜从门缝里爬出来。",
                "粮袋冻成石块，墙上的值班记录停在同一天。",
                "这里的人曾试过预测雪灾。他们失败了。"
            },
            {
                "霜层下埋着来不及运走的粮袋。\n每一袋都冻得像石头。",
                "墙上的日期停在盛夏。\n记录者反复写：又下雪了，没有预兆。",
                "粮仓后门被冰封死。\n门缝里吹出的风却带着地下深处的热气。"
            }
        },
        {
            "第三幕", "观测站",
            {
                "中区的观测站半塌进地里。铜盘断裂，指针却仍指着地下。",
                "旧地图上，所有震中都连向同一个没有名字的深处。",
                "地面每隔一会儿就轻轻跳一下。"
            },
            {
                "观测站的铜盘歪向一侧，指针却死死指着地下。\n它已经断了，仍在指。",
                "地面出现细长的裂缝。\n裂缝每次张开，灰尘就向上跳一下。",
                "一张旧地图铺在桌上。\n所有震中都连向同一个没有名字的深处。"
            }
        },
        {
            "第四幕", "熔毁工坊",
            {
                "东区的工坊被高温烧穿。管道垂在半空，空气里有铁和硫的味道。",
                "阀门卡在开启位置，墙上的警告只剩两个字：停炉。",
                "你第一次听见“地心炉”这个名字。"
            },
            {
                "管道表面还留着高温烧过的颜色。\n靴底踩上去，发出脆响。",
                "半熔的阀门卡在开启位置。\n墙上的警告只剩两个字：停炉。",
                "工坊尽头是一道被铁水封死的门。\n门后传来热浪，像有什么仍在燃烧。"
            }
        },
        {
            "第五幕", "王室档案馆",
            {
                "内城的档案馆被水泡过。书架倒成一片，封蜡还认得旧王朝的印。",
                "你从水里捞出一卷还没烂透的王室日志。",
                "那个在梦里背对你的老人，也许在这里留下过名字。"
            },
            {
                "档案馆的书架倒成一片。\n纸页被水泡过，字迹却比石头更固执。",
                "一卷王室日志摊在桌上。\n最后几页被人匆忙撕走。",
                "封蜡上压着旧王朝的炉形印。\n你指尖碰到它时，火光在记忆里闪了一下。"
            }
        },
        {
            "第六幕", "沉没街区",
            {
                "下层没有路，只有半座陷进地底的街区。",
                "你踩着屋顶往下走，像在走过一座倒置的城市。",
                "新王国的公文只记录救灾和失踪，没有一份提到炉心。"
            },
            {
                "你踩着倾斜的屋顶往下走。\n脚下的街道原本在几十尺之上。",
                "一扇窗里还亮着灯。\n靠近才发现，那是地底裂缝透出的火光。",
                "半座钟楼埋在瓦砾里。\n钟面停在灾害发生的那一刻。"
            }
        },
        {
            "第七幕", "骑士回廊",
            {
                "回廊两侧立着四尊骑士形态的守护机械。前三尊已经停摆。",
                "最后一尊还在巡行。甲胄摩擦的声音从黑暗里传来。",
                "门上刻着四个词：控制、防卫、配给、封存。"
            },
            {
                "回廊两侧各立着三尊空基座。\n第四尊基座上还连着断裂的锁链。",
                "甲胄摩擦的声音从黑暗里传来。\n它走得很慢，却没有停。",
                "一道石门挡在回廊尽头。\n门上刻着四个词：控制、防卫、配给、封存。"
            }
        },
        {
            "终幕", "核心门厅",
            {
                "门厅里没有风。火把的火焰却一直朝门内倾斜。",
                "门上的识别装置亮起微光，又熄灭。它在等一个不属于这个时代的签名。",
                "脉动越来越近。你听见一个苍老的声音，像从几百年前传来。"
            },
            {
                "门厅里没有风。\n火把的火焰却一直朝门内倾斜。",
                "门上的识别装置亮起微光，又熄灭。\n它在等一个不属于这个时代的签名。",
                "脉动越来越近。\n你听见一个苍老的声音，像从几百年前传来。"
            }
        }
    };
    return data;
}

struct StoryClue {
    std::string location;
    std::vector<std::string> discovery;
    std::vector<std::string> record;
    std::vector<std::string> reaction;
};

const std::vector<StoryClue>& storyClues() {
    static const std::vector<StoryClue> data = {
        {
            "裂缝前厅",
            {
                "你停在裂缝前。红光随着地底脉动一明一暗。",
                "你伸手触碰石壁，指尖发麻，脑海里闪过一座倒悬的城市。"
            },
            {
                "石壁上没有文字，只有一道反复刻下的记号：一个缺角的炉形。",
                "刻痕很新，边缘却没有留下任何人的指纹。"
            },
            {
                "你不知道这记号代表什么。胸口的疼痛却在你离开石壁后慢慢平息。",
                "那阵回声第一次有了方向：向下。"
            }
        },
        {
            "冻结粮仓",
            {
                "粮仓的门被冰封住。你砸开冰层，里面扑面而来的却是盛夏的霉味。",
                "墙上的值班记录停在同一天，纸页边缘结着霜。"
            },
            {
                "记录写着：午后降雪，粮袋冻结，城门封闭。",
                "往后翻，雪灾的日期没有规律。有人试过按节气预警，失败了。",
                "末尾有一句被划掉的话：‘地下的热源沉寂了。’"
            },
            {
                "雪不是从天上来的。至少，不只是。",
                "你第一次觉得，三场灾难也许共用同一个源头。"
            }
        },
        {
            "观测站",
            {
                "观测站的铜盘已经断裂，指针却仍固执地指向城市正下方。",
                "桌上摊着一张旧地图，所有震中都连向同一个没有名字的深层区域。"
            },
            {
                "旁注写着：‘支撑结构正在失效，不是单一断层。’",
                "旧王朝曾在这里监测地脉。记录提到一个被封住的核心设施。"
            },
            {
                "如果震动来自同一个地方，那三灾也许不是三场灾难。",
                "你想起梦里那座倒悬的城市。它也是一层一层往下沉的。"
            }
        },
        {
            "熔毁工坊",
            {
                "工坊的管道烧得变形，阀门却卡在开启位置。",
                "墙上的警告牌只剩两个字：停炉。"
            },
            {
                "残存铭文第一次写出完整的名字：地心炉。",
                "它由旧王朝国王主持建造，引导地脉能源。",
                "记录警告：热量与压力无法稳定控制。",
                "火灾不是从外部烧进来的。是炉心自己失控了。"
            },
            {
                "雪、地震、火。三条线在这里合成一条。",
                "你胸口的刺痛突然加剧，像有什么东西在炉心深处回应。"
            }
        },
        {
            "王室档案馆",
            {
                "档案馆的书架倒成一片。你从水里捞出一卷还没烂透的王室日志。",
                "封蜡上压着旧王朝的炉形印，边缘已经发黑。"
            },
            {
                "日志记载：国王下令停止扩建，并亲自进入核心区域。",
                "最后一页只有一句话：‘炉火已熄。不要让后来者重新点燃它。’",
                "国王随后离开王都，带走了关键图纸。"
            },
            {
                "他造了它，也关了它。可他为什么把门留给后人？",
                "你摸到封蜡，记忆里闪过一个老人的背影。他一直没有回头。"
            }
        },
        {
            "沉没街区",
            {
                "半座街区陷进地底。你踩着屋顶往下走，像在走过一座倒置的城市。",
                "一扇窗里还亮着灯。靠近才发现，那是地底裂缝透出的火光。"
            },
            {
                "三处灾区的记录拼在一起：寒冷、震动、火焰，都从地下设施异常后出现。",
                "新王国的公文只有救灾、迁移和失踪人数。没有一份提到地心炉。",
                "他们不知道自己在什么上面建城。"
            },
            {
                "这不是阴谋，也不是天罚。只是一台没人记得的旧机器，在慢慢坏掉。",
                "而它还在坏。下一次灾难不会等你想明白。"
            }
        },
        {
            "骑士回廊",
            {
                "回廊两侧立着四尊骑士形态的守护机械。前三尊已经停摆。",
                "最后一尊还在巡行。它的甲胄上刻着：封存。"
            },
            {
                "旧刻文记录四种职责：控制、防卫、配给、封存。",
                "幸存者的壁画把它们画成征服、战争、饥荒、死亡。传说与记录互相矛盾，却指向同一批身影。",
                "它们不是灾难的制造者。它们只是还在执行几百年前的命令。"
            },
            {
                "最后一尊骑士挡在核心门前。它不追逐地表的人，只阻挡未经授权者。",
                "你听见甲胄摩擦的声音。它已经发现你了。"
            }
        },
        {
            "核心门厅",
            {
                "门上的识别装置亮起。它没有看你的脸，也没有看你自报的物种。",
                "它读出一道旧王朝留下的签名。",
                "签名沿着血脉传了几百年，已经非常微弱。"
            },
            {
                "记录显示：旧国王在熄灭炉心前，把紧急访问权限留给了后代。",
                "地心炉没有意志。它只是在封存失效时，一遍遍发出求援信号。",
                "所谓‘修复者’，只是后人对这种权限的称呼。"
            },
            {
                "身份牌上的物种是你自己报的；旧装置只认这道签名。",
                "你继承的是进入核心的资格，不是必须服从的命令。",
                "门开了。里面没有王座，只有一台仍在发热的旧机器。"
            }
        }
    };
    return data;
}

const AreaInfo& areaForStage(int stage) {
    const std::vector<AreaInfo>& data = areas();
    if (stage < 0) {
        stage = 0;
    }
    if (stage >= static_cast<int>(data.size())) {
        stage = static_cast<int>(data.size()) - 1;
    }
    return data[static_cast<std::size_t>(stage)];
}

std::string endingObjective(int ending) {
    switch (ending) {
        case 1:
            return "炉心已熄灭";
        case 2:
            return "炉心仍待维护";
        case 3:
            return "核心已摧毁";
        case 4:
            return "你拒绝了召唤";
        default:
            return "终幕已完成";
    }
}

std::string endingCampLine(int ending) {
    switch (ending) {
        case 1:
            return "炉心不再回应。营地的人开始收拾行装。";
        case 2:
            return "炉心仍在深处缓慢跳动。你知道那意味着什么。";
        case 3:
            return "王都深处已经坍塌。风从裂缝里吹上来。";
        case 4:
            return "核心的门关着。回声还在，只是你不再听。";
        default:
            return "核心的门已经关闭。";
    }
}

std::string endingCoreLine(int ending) {
    switch (ending) {
        case 1:
            return "炉心已经熄灭。门后没有脉动。";
        case 2:
            return "炉心被压低了声音。门后仍有缓慢的震动。";
        case 3:
            return "核心已经坍塌。门后只有碎石和风。";
        case 4:
            return "门关着。门后仍有脉动，只是你不再回应。";
        default:
            return "核心的门不再打开。";
    }
}

// 统一的故事页：标题居中，短句居中，长句自动换行。
void showStoryPage(const std::string& title,
                   const std::vector<std::string>& lines,
                   UI::Color color,
                   const std::string& prompt) {
    UI::clearScreen();

    Frame frame(105);
    frame.addBlank();
    frame.addBlank();
    frame.addCentered(UI::paint(title, color, true));
    frame.addBlank();

    for (const std::string& line : lines) {
        if (line.empty()) {
            frame.addBlank();
        } else if (UI::displayWidth(line) <= 88) {
            frame.addCentered(line);
        } else {
            frame.addWrapped(line, "  ");
        }
    }

    frame.addBlank();
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint(prompt, UI::Color::BrightGreen));
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

std::string travelFlavor(int stage, int progress) {
    if (progress < 1 || progress > 3) {
        return "你继续向遗迹深处走去。";
    }
    return areaForStage(stage).travel[static_cast<std::size_t>(progress - 1)];
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
    : player_("旅人"), storyStage_(0), storyEnding_(0),
      shopAvailable_(Random::chance(0.5)) {
    // 真正的初始物品在城门登记新身份时发放。
}

void Game::run() {
    showTitleScreen();
    showPrologueScreen();

    if (!startFromGate()) {
        return;
    }

    const StoryProgress progress = loadStoryProgress(player_.getName());
    storyStage_ = progress.stage;
    storyEnding_ = progress.ending;

    showCityEntryNarration();

    std::string message;
    if (storyStage_ >= kStoryClueCount + 1) {
        message = endingCampLine(storyEnding_);
    } else if (storyStage_ == kStoryClueCount) {
        message = "核心的门已经打开。最深处传来缓慢的脉动。";
    } else {
        message = "你在城内一处半塌的屋子里支起火堆。";
    }
    bool running = true;

    while (running) {
        renderCamp(message);
        message.clear();

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 6);
        switch (choice) {
            case 1:
                showStatusScreen();
                break;
            case 2:
                showInventoryScreen();
                break;
            case 3:
                if (shopAvailable_) {
                    showShop();
                } else {
                    showTrader();
                }
                break;
            case 4:
                showCampTalk();
                break;
            case 5:
                message = explore();
                refreshCampShop();
                break;
            case 6:
                player_.restoreAll();
                message = "你在火堆旁休息，HP 与 MP 已完全恢复。";
                refreshCampShop();
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
    showStoryPage(
        "序章 · 不合时令的灾难",
        {
            "先是雪。盛夏的雪压在屋脊上，像一层没有温度的灰。",
            "然后是震动。墙上的灰簌簌落下，远处的天边亮起火光。",
            "人们说这是三场灾难。可它们偏偏在同一天到来。",
            "你被埋在塌下的屋檐下，听见地底传来一声很慢的脉动。"
        },
        UI::Color::BrightCyan,
        "  按回车继续 > ");

    showStoryPage(
        "昏迷 · 倒悬的王都",
        {
            "梦里，一座城市倒悬在地下。",
            "街道在上，屋顶在下。所有人都低着头，脸藏在灰布里。",
            "城市中央有一台熄灭的炉子。一个老人背对着你，站在炉前。",
            "他说：‘如果你听得见……不要让它再烧起来。’",
            "你伸出手，炉心却先一步暗了下去。"
        },
        UI::Color::BrightMagenta,
        "  按回车继续 > ");

    showStoryPage(
        "醒来 · 遗失的证明",
        {
            "你在废墟里醒来。雪停了，地面还在轻轻震动。",
            "旧身份证明在灾害中遗失。你只记得自己的名字，和一个方向。",
            "那阵回声从地底传来，像有人在很远的地方敲一面铁鼓。",
            "你沿着回声，走向一座不该还有人居住的城。"
        },
        UI::Color::BrightYellow,
        "  按回车继续 > ");
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
            "「地下最近又震了。进去的人，十个回来三个。」",
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

    saveStoryProgress(newPlayer.getName(), 0, 0);

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
    frame.addCentered("一个披灰布的人从巷口看了你一眼，又低下头。");
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

    Frame frame(105);
    frame.addBlank();
    frame.addBlank();
    frame.addCentered(UI::paint(storyStage_ == 0 ? "你推开沉重的石门。" : "你再次推开石门。",
                                UI::Color::BrightCyan));
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& doorArt = Sprites::door();
    for (int index = 0; index < 4; ++index) {
        const std::string doorLine =
            index < 3 ? UI::paint(doorArt[static_cast<std::size_t>(index)], UI::Color::Yellow)
                      : std::string();
        frame.addColumns(
            UI::center(playerArt[static_cast<std::size_t>(index)], 49),
            UI::center(doorLine, 50),
            49);
    }

    frame.addBlank();
    frame.addCentered("潮湿的冷风从地底吹来，火把猛地朝深处倾斜。");
    frame.addCentered("那阵熟悉的回声再次响起。你沿石阶走进沉没的王都。");
    frame.addBlank();
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

bool Game::showStoryBeat() {
    if (storyStage_ >= kStoryClueCount) {
        return false;
    }

    const StoryClue& clue = storyClues()[static_cast<std::size_t>(storyStage_)];
    const std::string title = "线索 " + std::to_string(storyStage_ + 1) + "/" +
                              std::to_string(kStoryClueCount) + " · " + clue.location;

    showStoryPage(title, clue.discovery, UI::Color::BrightCyan,
                  "  按回车查看残存记录 > ");

    std::vector<std::string> record = clue.record;
    record.push_back("");
    record.insert(record.end(), clue.reaction.begin(), clue.reaction.end());
    showStoryPage("残存记录 · " + clue.location, record, UI::Color::BrightYellow,
                  "  按回车继续 > ");

    ++storyStage_;
    saveStoryProgress(player_.getName(), storyStage_, storyEnding_);
    return true;
}

void Game::showAreaIntro(int stage) const {
    const AreaInfo& area = areaForStage(stage);
    showStoryPage(area.act + " · " + area.name, area.intro, UI::Color::BrightCyan,
                  "  按回车深入 > ");
}

void Game::showCoreSealedScene() const {
    std::vector<std::string> lines;
    lines.push_back(endingCoreLine(storyEnding_));
    lines.push_back("你站在核心门前。门上的识别装置已经熄灭。");

    if (storyEnding_ == 4) {
        lines.push_back("王都的回声仍在地底徘徊。");
    } else if (storyEnding_ == 2) {
        lines.push_back("你知道有一天还得回来。");
    } else {
        lines.push_back("王都的回声渐渐远去。");
    }

    showStoryPage("核心 · 终幕之后", lines, UI::Color::Gray,
                  "  按回车返回营地 > ");
}

bool Game::enterCore() {
    showStoryPage(
        "终幕 · 炉心大厅",
        {
            "核心的门在身后合拢。热浪让空气变得扭曲。",
            "大厅中央悬着一台巨大的旧机器，像一颗被铁链锁住的心脏。",
            "每一次脉动，墙上的裂缝就亮一下。三灾不是过去，是仍在发生的事。"
        },
        UI::Color::BrightRed,
        "  按回车继续 > ");

    showStoryPage(
        "旧国王的留言",
        {
            "一个苍老的声音从炉心深处传来，像反复播放了几百年。",
            "‘如果你听见这段话，说明封存已经失效。’",
            "‘我造了它，也关过它。但我不能替后来的人决定它该不该存在。’",
            "‘你可以熄灭它，稳定它，毁掉它，或者转身离开。’",
            "‘无论选哪个，都不要说是血脉替你选的。’"
        },
        UI::Color::BrightYellow,
        "  按回车走到炉心前 > ");

    while (true) {
        UI::clearScreen();

        Frame frame;
        frame.addBlank();
        frame.addCentered(UI::paint("终幕 · 最后的选择", UI::Color::BrightYellow, true));
        frame.addBlank();
        frame.addWrapped("炉心仍在紊乱。先祖选择过熄灭，但那是他的决定。", "  ");
        frame.addWrapped("现在，选择权在你手中。", "  ");
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
            return false;
        }

        showEnding(choice);
        storyEnding_ = choice;
        storyStage_ = kStoryClueCount + 1;
        saveStoryProgress(player_.getName(), storyStage_, storyEnding_);
        return true;
    }
}

void Game::showEnding(int choice) const {
    std::string title;
    std::string epilogueTitle;
    UI::Color color = UI::Color::BrightYellow;
    std::vector<std::string> first;
    std::vector<std::string> second;

    switch (choice) {
        case 1:
            title = "结局 · 熄灭炉心";
            epilogueTitle = "尾声 · 熄灭炉心";
            color = UI::Color::BrightYellow;
            first = {
                "你按先祖留下的步骤切断热流。炉心的脉动越来越慢。",
                "最后一声响动很轻，像一块烧红的铁落进水里。",
                "四尊守护机械同时垂下头。封存骑士的甲胄裂开，里面空无一物。"
            };
            second = {
                "三灾停了。王都仍埋在地下，旧王朝的技术也随炉心一起封存。",
                "你从核心走出来。营地的人问你下面有什么。",
                "你想了很久，只说：‘一台已经熄了的炉子。’"
            };
            break;

        case 2:
            title = "结局 · 稳定炉心";
            epilogueTitle = "尾声 · 稳定炉心";
            color = UI::Color::BrightCyan;
            first = {
                "你降低炉心输出，重新接上残存的封存结构。",
                "震动一点点平息，热浪退回管道。",
                "封存骑士单膝跪下，像在确认一条迟到了几百年的命令。"
            };
            second = {
                "灾害风险没有归零。炉心仍需要有人看护。",
                "你回到营地，把旧王朝的记录摊在火堆旁。",
                "幸存者们第一次知道，自己脚下埋着什么。",
                "有人问：接下来怎么办？你说：先学会看着它。"
            };
            break;

        case 3:
            title = "结局 · 摧毁炉心";
            epilogueTitle = "尾声 · 摧毁炉心";
            color = UI::Color::BrightRed;
            first = {
                "你砸断核心的支撑。最后一次地震从脚下炸开。",
                "整座王都开始坍塌，碎石和火光从穹顶落下。",
                "封存骑士没有追你。它留在门后，直到穹顶把它一起埋住。"
            };
            second = {
                "炉心永远不会再启动。三灾的源头消失了。",
                "旧王朝的秘密也一起埋进黑暗。",
                "你活着回到地面。远处的天空很安静，安静得让人不习惯。"
            };
            break;

        default:
            title = "结局 · 拒绝介入";
            epilogueTitle = "尾声 · 拒绝介入";
            color = UI::Color::Gray;
            first = {
                "你后退一步，关上了核心的门。",
                "识别装置的光在你身后慢慢熄灭。",
                "你没有替先祖做决定，也没有替这座城市做决定。"
            };
            second = {
                "召唤声没有消失，只是变远了。",
                "你知道炉心还在下面，还在坏。",
                "你回到营地，收拾行囊。有人问你还会不会回来。",
                "你说：不知道。"
            };
            break;
    }

    showStoryPage(title, first, color, "  按回车继续 > ");
    showStoryPage(epilogueTitle, second, color, "  按回车返回营地 > ");
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

    std::string objective;
    if (storyStage_ >= kStoryClueCount + 1) {
        objective = endingObjective(storyEnding_);
    } else if (storyStage_ == kStoryClueCount) {
        objective = "核心已开启";
    } else {
        objective = "下一段：" + areaForStage(storyStage_).name;
    }
    frame.addLine("剧情线索 " + std::to_string(std::min(storyStage_, kStoryClueCount)) +
                  "/" + std::to_string(kStoryClueCount) + "    " + objective);
    frame.addLine(UI::paint(shopAvailable_ ? "今日：游商在营地，货物齐全。"
                                           : "今日：游商没来，只有拾荒者。",
                            UI::Color::Gray));

    frame.addDivider();
    frame.addLine(UI::paint("营地记事", UI::Color::Gray));
    if (message.empty()) {
        if (storyStage_ >= kStoryClueCount + 1) {
            frame.addLine(endingCampLine(storyEnding_));
        } else if (storyStage_ == kStoryClueCount) {
            frame.addLine("核心的门已经打开。你可以进去做出选择。");
        } else if (storyStage_ == kStoryClueCount - 1) {
            frame.addLine("封存骑士守在核心门前。它还在等。");
        } else {
            frame.addLine("火堆噼啪作响，遗迹入口就在不远处。");
        }
    } else {
        frame.addWrapped(message, "· ");
    }

    frame.addDivider();
    const std::string shopLabel = shopAvailable_ ? "商店" : "与人交易";
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 查看状态   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 背包   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " " + shopLabel + "   " +
        UI::paint("[4]", UI::Color::BrightYellow) + " 交谈");

    std::string exploreLabel = "进入遗迹";
    if (storyStage_ >= kStoryClueCount + 1) {
        exploreLabel = "回望核心";
    } else if (storyStage_ == kStoryClueCount) {
        exploreLabel = "进入核心";
    }
    frame.addLine(
        UI::paint("[5]", UI::Color::BrightYellow) + " " + exploreLabel + "   " +
        UI::paint("[6]", UI::Color::BrightYellow) + " 休息   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开（自动存档）");

    frame.render();
}

void Game::renderTravel(int progress, int playerColumn) const {
    UI::clearScreen();

    const AreaInfo& area = areaForStage(storyStage_);

    Frame frame;
    frame.addColumns(
        UI::paint(area.act + " · " + area.name, UI::Color::BrightCyan),
        UI::paint("探索进度 " + std::to_string(progress) + "/3", UI::Color::Gray),
        44);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& doorArt = Sprites::door();
    const std::string doorLabel = storyStage_ >= 7 ? "核心门"
                                 : (storyStage_ >= 6 ? "石门" : "木门");

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
        UI::paint(doorLabel, UI::Color::Yellow), 56));

    frame.addBlank();
    frame.addWrapped(travelFlavor(storyStage_, progress), "  ");
    frame.addBlank();
    frame.addDivider();

    const std::string forwardLabel =
        (progress >= 3) ? (storyStage_ >= 7 ? "靠近核心门" : "靠近木门") : "继续前进";
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
    frame.addCentered(UI::paint("身份牌上的名字，是这里唯一承认的你。", UI::Color::Gray));
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

    std::string storyLine = "剧情线索 " +
                            std::to_string(std::min(storyStage_, kStoryClueCount)) +
                            "/" + std::to_string(kStoryClueCount);
    if (storyStage_ >= kStoryClueCount + 1) {
        storyLine += "    已抵达终幕";
    } else if (storyStage_ == kStoryClueCount) {
        storyLine += "    核心已开启";
    } else {
        storyLine += "    下一段：" + areaForStage(storyStage_).name;
    }
    frame.addLine(storyLine);

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

void Game::showCampTalk() {
    std::string message = "灰袍人往火堆里添了一根柴。‘你想问什么？’";

    while (true) {
        UI::clearScreen();

        Frame frame(105);
        frame.addColumns(
            UI::paint("营地 · 篝火旁", UI::Color::BrightCyan),
            UI::paint("线索 " + std::to_string(std::min(storyStage_, kStoryClueCount)) +
                      "/" + std::to_string(kStoryClueCount),
                      UI::Color::Gray),
            85);
        frame.addDivider();
        frame.addBlank();

        for (const std::string& line : Sprites::survivor()) {
            frame.addCentered(UI::paint(line, UI::Color::Gray));
        }
        frame.addCentered(UI::paint("灰袍幸存者", UI::Color::Yellow));
        frame.addBlank();
        frame.addWrapped("他没有脸。胸前那块身份牌磨得发白，只剩一个看不清的姓。", "  ");
        frame.addBlank();
        frame.addWrapped(message, "  ");
        frame.addDivider();
        frame.addLine(
            UI::paint("[1]", UI::Color::BrightYellow) + " 问灾难   " +
            UI::paint("[2]", UI::Color::BrightYellow) + " 问遗迹   " +
            UI::paint("[3]", UI::Color::BrightYellow) + " 问四骑士   " +
            UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
        frame.render();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 3);
        if (choice == 0 || choice == -1) {
            return;
        }

        switch (choice) {
            case 1:
                if (storyStage_ >= kStoryClueCount + 1) {
                    if (storyEnding_ == 1) {
                        message = "炉子停了。雪还在下，但至少不像有人在下面喘气了。";
                    } else if (storyEnding_ == 2) {
                        message = "炉子还在烧，只是没以前那么凶。";
                    } else if (storyEnding_ == 3) {
                        message = "核心塌了。地底安静了，至少现在是。";
                    } else {
                        message = "炉子没停。只是没人再下去看了。";
                    }
                } else if (storyStage_ >= 6) {
                    message = "现在我知道那不是天罚。可知道又怎样？我们连它的名字都不敢说。";
                } else if (storyStage_ >= 3) {
                    message = "老辈人说，地下有台炉子。雪是它冷了，火是它烧过头。地震？那是它翻身。";
                } else {
                    message = "夏天落雪，地动，火光。官府说是三场天灾，可我见过它们同一天来。";
                }
                break;

            case 2:
                if (storyStage_ >= kStoryClueCount + 1) {
                    if (storyEnding_ == 1) {
                        message = "核心已经封了。你出来的时候，脸上像老了好几岁。";
                    } else if (storyEnding_ == 2) {
                        message = "核心还开着一条缝。总得有人下去看看。";
                    } else if (storyEnding_ == 3) {
                        message = "核心没了。下面只剩碎石。";
                    } else {
                        message = "核心门关着。你回来以后，地底的震动还没停过。";
                    }
                } else if (storyStage_ >= 6) {
                    message = "核心门开了。能进去的人，大概不是来发财的。";
                } else if (storyStage_ >= 3) {
                    message = "王室档案馆在最里面。那地方的门认旧王朝的印，不认身份牌。";
                } else {
                    message = "粮仓、观测站、工坊。越往里，记录越旧。别只看墙，也看看墙缝。";
                }
                break;

            case 3:
                if (storyStage_ >= kStoryClueCount + 1) {
                    if (storyEnding_ == 1) {
                        message = "骑士都停了。至少，没人再听见甲胄的声音。";
                    } else if (storyEnding_ == 2) {
                        message = "封存骑士还跪在门前。它好像认了新主人。";
                    } else if (storyEnding_ == 3) {
                        message = "骑士和核心一起埋了。";
                    } else {
                        message = "骑士还在下面。夜里还能听见甲胄的声音。";
                    }
                } else if (storyStage_ >= 6) {
                    message = "我见过最后一个。甲胄上写着‘封存’。它不杀地表的人，只守门。";
                } else {
                    message = "征服、战争、饥荒、死亡。幸存者都这么叫。可我觉得他们看错了。";
                }
                break;

            default:
                break;
        }
    }
}

void Game::showShop() {
    showShopScreen(true);
}

void Game::showTrader() {
    showShopScreen(false);
}

void Game::refreshCampShop() {
    shopAvailable_ = Random::chance(0.5);
}

void Game::showShopScreen(bool full) {
    std::unique_ptr<Shop> shop;
    if (full) {
        shop = std::make_unique<Shop>();
    } else {
        shop = std::make_unique<Shop>(Random::integer(3, 5),
                                      Random::real(0.75, 1.4));
    }

    std::string message = full ? "看看货吧，都是遗迹里少见的好东西。"
                               : "就这些了。价格嘛，看你顺不顺眼。";

    while (true) {
        renderShop(*shop, message, full);

        const int choice = UI::readChoice(UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 3);
        switch (choice) {
            case 1:
                message = buyFromShop(*shop);
                break;
            case 2:
                message = sellToShop();
                break;
            case 3:
                showInventoryScreen();
                message = full ? "还要买点什么吗？" : "还要换点什么吗？";
                break;
            case 0:
            case -1:
                return;
            default:
                break;
        }
    }
}

void Game::renderShop(const Shop& shop, const std::string& message, bool full) const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint(full ? "地下集市" : "营地 · 拾荒者", UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "   背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        30);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string>& art =
        full ? Sprites::merchant() : Sprites::survivor();
    const std::string traderLabel = full ? "商人" : "拾荒者";
    constexpr int kMerchantWidth = 22;
    const int artRows = static_cast<int>(art.size());
    const int rowCount = std::max(artRows + 2, shop.size());

    for (int row = 0; row < rowCount; ++row) {
        std::string left;
        if (row < artRows) {
            left = UI::center(UI::paint(art[static_cast<std::size_t>(row)],
                                        full ? UI::Color::Yellow : UI::Color::Gray),
                              kMerchantWidth);
        } else if (row == artRows) {
            left = UI::center(UI::paint("═══════", UI::Color::Gray), kMerchantWidth);
        } else if (row == artRows + 1) {
            left = UI::center(UI::paint(traderLabel, UI::Color::Yellow), kMerchantWidth);
        }

        std::string right;
        if (row < shop.size()) {
            right = shopItemLine(row + 1, shop[row]);
        }
        frame.addSplit(left, right, kMerchantWidth);
    }

    frame.addBlank();
    frame.addWrapped(traderLabel + "：" + message, "  ");
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

std::unique_ptr<Monster> Game::createGuardian() const {
    return std::make_unique<Guardian>(player_.getLevel());
}

std::unique_ptr<Monster> Game::createMimic() const {
    return std::make_unique<Mimic>(player_.getLevel());
}

void Game::grantChestLoot(std::vector<std::string>& lines) {
    const int roll = Random::integer(0, 99);

    if (roll < 45) {
        const int gold = Random::integer(15, 40);
        player_.gainGold(gold);
        lines.push_back("宝箱里有 " + std::to_string(gold) + " 枚金币。");
        return;
    }

    std::unique_ptr<Item> item;
    if (roll < 65) {
        item = std::make_unique<Potion>("红药水", 8, Potion::Effect::RestoreHp, 15);
    } else if (roll < 80) {
        item = std::make_unique<Potion>("蓝药水", 10, Potion::Effect::RestoreMp, 10);
    } else if (roll < 90) {
        item = std::make_unique<Weapon>("旧王都短剑", 30, 3);
    } else {
        item = std::make_unique<Armor>("旧王都护甲", 35, 3);
    }

    const std::string name = item->getName();
    if (player_.getInventory().canAdd()) {
        player_ + std::move(item);
        lines.push_back("宝箱里有：" + name + "。");
    } else {
        const int gold = item->getSellPrice();
        player_.gainGold(gold);
        lines.push_back("背包已满，你把" + name + "折成了 " +
                        std::to_string(gold) + " 枚金币。");
    }
}

std::string Game::showRewardRoom() {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("奖励房间", UI::Color::BrightCyan),
        UI::paint("宝箱", UI::Color::Yellow),
        44);
    frame.addDivider();
    frame.addBlank();

    for (const std::string& line : Sprites::chest()) {
        frame.addCentered(UI::paint(line, UI::Color::Yellow));
    }

    frame.addBlank();
    frame.addWrapped("房间中央放着一只旧木箱。锁扣上没有灰，像刚被人擦过。", "  ");
    frame.addWrapped("箱盖轻轻动了一下。也可能只是风。", "  ");
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 打开宝箱   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
    frame.render();

    const int choice = UI::readChoice(
        UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
    if (choice == 0 || choice == -1) {
        return "你没有打开宝箱，回到营地。";
    }

    if (!Random::chance(0.3)) {
        std::vector<std::string> lines;
        lines.push_back("锁扣弹开。箱子里没有机关。");
        grantChestLoot(lines);
        showResult("宝  箱", lines, UI::Color::BrightYellow);
        return "你从奖励房间带回了一些东西。";
    }

    std::unique_ptr<Monster> mimic = createMimic();
    showEncounterCutscene(*mimic);

    Battle battle(player_, storyStage_);
    const BattleResult result = battle.fight(*mimic);

    if (result == BattleResult::Victory) {
        const int levelsGained = player_.gainExperience(mimic->getExpReward());
        player_.gainGold(mimic->getGoldReward());

        std::vector<std::string> lines;
        lines.push_back("宝箱怪倒下了。箱盖里没有舌头，只有一排断牙。");
        lines.push_back("获得 " + std::to_string(mimic->getGoldReward()) + " 金币、" +
                        std::to_string(mimic->getExpReward()) + " 经验。");

        std::unique_ptr<Item> drop = mimic->rollDrop();
        if (drop) {
            const std::string dropName = drop->getName();
            if (player_.getInventory().canAdd()) {
                player_ + std::move(drop);
                lines.push_back("掉落：" + dropName + "。");
            } else {
                lines.push_back("掉落：" + dropName + "，但背包已满。");
            }
        }

        lines.push_back("它的肚子里还塞着上一批冒险者的东西。");
        grantChestLoot(lines);

        if (levelsGained > 0) {
            lines.push_back("升级！你现在是 Lv." + std::to_string(player_.getLevel()) + "。");
        }
        showResult("宝箱怪 · 胜利", lines, UI::Color::BrightYellow);
        return "你从宝箱怪的残骸里捡回了一些东西。";
    }

    if (result == BattleResult::Defeat) {
        const int lostGold = std::min(10, player_.getGold());
        if (lostGold > 0) {
            player_.spendGold(lostGold);
        }
        player_.restoreAll();

        std::vector<std::string> lines;
        lines.push_back("宝箱怪把你咬昏了。");
        lines.push_back("醒来时，你已经回到营地。");
        if (lostGold > 0) {
            lines.push_back("丢失了 " + std::to_string(lostGold) + " 枚金币。");
        }
        showResult("宝箱怪 · 战败", lines, UI::Color::BrightRed);
        return "宝箱怪把你打昏了。醒来时你回到了营地。";
    }

    showResult("逃  离", {"你退出了奖励房间。"}, UI::Color::Gray);
    return "你退出了奖励房间。";
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
    if (storyStage_ >= kStoryClueCount + 1) {
        showCoreSealedScene();
        return "你回到营地。核心的门没有再打开。";
    }

    if (storyStage_ == kStoryClueCount) {
        if (enterCore()) {
            return "核心的门在身后合拢。你回到营地。";
        }
        return "你暂时离开核心。炉心仍在下面跳动。";
    }

    showEnterRuinsCutscene();
    showAreaIntro(storyStage_);

    const bool bossBattle = (storyStage_ == kStoryClueCount - 1);

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
            monster = bossBattle ? createGuardian() : createRandomMonster();
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

        if (bossBattle) {
            showStoryPage(
                "守护者 · 封存解除",
                {
                    "封存骑士的甲胄从胸口裂开。里面没有身体，只有一枚停转的齿轮。",
                    "它最后重复了一次旧命令：‘封存……完成。’",
                    "挡在门前的锁链松开了一道。门上的识别装置随即亮起。"
                },
                UI::Color::BrightMagenta,
                "  按回车继续 > ");
        }

        showStoryBeat();
        const std::string rewardMessage = showRewardRoom();
        showReturnCutscene();
        const std::string outcome = bossBattle
                                        ? "封存骑士倒下了。核心的门已经打开。"
                                        : "你带着新发现的记录回到营地。";
        return outcome + " " + rewardMessage;
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
        if (bossBattle) {
            lines.push_back("封存骑士仍守在核心门前。");
        }
        showResult("战  败", lines, UI::Color::BrightRed);
        return bossBattle ? "你在营地休整。封存骑士仍在核心门前。"
                          : "你在营地休整，准备再次出发。";
    }

    showReturnCutscene();
    return bossBattle ? "你退回营地。封存骑士没有追来，仍守着核心的门。"
                      : "你退回营地，拍了拍身上的灰。";
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
