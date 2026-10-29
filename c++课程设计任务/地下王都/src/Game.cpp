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

const std::vector<std::string>& hubLocationNames() {
    static const std::vector<std::string> names = {"城门", "商店", "营地", "酒馆", "高塔"};
    return names;
}

const std::vector<int>& hubLocationPositions() {
    static const std::vector<int> positions = {14, 34, 54, 74, 90};
    return positions;
}

struct StoryProgress {
    int stage = 0;
    int ending = 0;
    std::set<std::string> flags;
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
        } else if (line.rfind("flag=", 0) == 0 && line.size() > 5) {
            progress.flags.insert(line.substr(5));
        }
    }
    return progress;
}

void saveStoryProgress(const std::string& playerName, int stage, int ending,
                       const std::set<std::string>& flags) {
    std::ofstream output(kStoryProgressFile, std::ios::out | std::ios::trunc);
    if (output) {
        output << "name=" << playerName << "\n" << stage << "\n";
        if (ending >= 1 && ending <= 4) {
            output << "ending=" << ending << "\n";
        }
        for (const std::string& flag : flags) {
            output << "flag=" << flag << "\n";
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

struct JournalEntry {
    std::string id;
    std::string category;
    std::string title;
    std::vector<std::string> lines;
};

const std::vector<JournalEntry>& journalEntries() {
    static const std::vector<JournalEntry> data = [] {
        std::vector<JournalEntry> entries;
        const std::vector<StoryClue>& clues = storyClues();
        for (int index = 0; index < static_cast<int>(clues.size()); ++index) {
            const StoryClue& clue = clues[static_cast<std::size_t>(index)];
            JournalEntry entry;
            entry.id = "clue." + std::to_string(index + 1);
            entry.category = "主线";
            entry.title = clue.location;
            entry.lines = clue.record;
            entry.lines.push_back("");
            entry.lines.insert(entry.lines.end(), clue.reaction.begin(), clue.reaction.end());
            entries.push_back(entry);
        }

        entries.push_back({
            "talk.disaster", "传闻", "灰袍人的灾难传闻",
            {
                "灰袍人说，夏天落雪、地动和火光经常在同一天出现。",
                "官府把它们当成三场互不相关的天灾，但他见过它们同时来。",
                "老辈人相信地下有一台炉子：雪是它冷了，火是它烧过头。"
            }
        });
        entries.push_back({
            "talk.ruins", "传闻", "遗迹与旧王朝",
            {
                "灰袍人说，粮仓、观测站和工坊越往里，记录越旧。",
                "王室档案馆的门认旧王朝的印，不认现在的身份牌。",
                "他提醒你：别只看墙，也看看墙缝里的东西。"
            }
        });
        entries.push_back({
            "talk.knights", "传闻", "四骑士的传说",
            {
                "幸存者把地下的守护身影称为征服、战争、饥荒、死亡。",
                "灰袍人只见过最后一个，甲胄上写着‘封存’。",
                "它不杀地表的人，只挡核心的门。"
            }
        });
        entries.push_back({
            "ancestor.room", "暗线", "地下五层的绞索",
            {
                "塌陷矿道尽头有一间被封死的休息室。",
                "梁上垂着半截绳索，遗骸下面的矿灯早已熄灭。",
                "墙上有一个缺角的炉形记号，和主角梦里的记号相似。"
            }
        });
        entries.push_back({
            "ancestor.truth", "暗线", "遗骸与家族记号",
            {
                "遗骸的衣料不属于这个时代，随身有矿灯、镐和半张身份牌。",
                "官方档案说他是叛徒；商人说他是替身；考古者说可能是更早的矿工。",
                "没有一种说法能完全对上，但这条路已经有人走过。"
            }
        });
        entries.push_back({
            "relic.slate", "古物", "刻纹石板",
            {
                "地面商店老板认出这是一块古国刻纹石板。",
                "上面记着地下五层的一条旧矿井路线，以及一间被钉死的休息室。",
                "老板年轻时挖过几年遗址，收下石板并给了你九折优惠。"
            }
        });
        entries.push_back({
            "tavern.ash", "传闻", "酒馆里的火山灰",
            {
                "酒馆老板说，地震撕开山体后，火山灰就再没真正散过。",
                "居民外出必须戴面具，身份牌只负责登记，不再代替脸。",
                "他还说，地下五层有一扇被钉死的门，只有少数人知道。"
            }
        });
        entries.push_back({
            "tower.gate", "地点", "高塔的锁链",
            {
                "高塔入口被灰烬和锁链封住。",
                "塔顶偶尔有金属反光，但暂时上不去。",
                "这里以后会通向征服骑士。"
            }
        });
        return entries;
    }();
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
    frame.renderTypewriter();

    UI::waitForEnter(UI::paint(prompt, UI::Color::BrightGreen));
}

std::string atColumn(const std::string& text, int column) {
    if (column <= 0) {
        return text;
    }
    return std::string(static_cast<std::size_t>(column), ' ') + text;
}

void appendAtColumn(std::string& line, const std::string& text, int column) {
    const int usedWidth = UI::displayWidth(line);
    if (usedWidth < column) {
        line.append(static_cast<std::size_t>(column - usedWidth), ' ');
    }
    line += text;
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

std::vector<std::string> stretchArtToCanvas(const std::vector<std::string>& art,
                                            int width, int height) {
    if (art.empty() || width <= 0 || height <= 0) {
        return {};
    }

    int sourceWidth = 0;
    for (const std::string& line : art) {
        sourceWidth = std::max(sourceWidth, static_cast<int>(line.size()));
    }
    if (sourceWidth <= 0) {
        return {};
    }

    std::vector<std::string> result(static_cast<std::size_t>(height));
    for (int row = 0; row < height; ++row) {
        const int sourceRow = std::min(
            static_cast<int>(art.size()) - 1,
            row * static_cast<int>(art.size()) / height);
        const std::string& source = art[static_cast<std::size_t>(sourceRow)];
        result[static_cast<std::size_t>(row)].reserve(
            static_cast<std::size_t>(width));
        for (int column = 0; column < width; ++column) {
            const int sourceColumn = std::min(
                sourceWidth - 1, column * sourceWidth / width);
            result[static_cast<std::size_t>(row)] +=
                sourceColumn < static_cast<int>(source.size())
                    ? source[static_cast<std::size_t>(sourceColumn)]
                    : ' ';
        }
    }
    return result;
}

std::string shopItemLine(int index, const Item& item, bool discounted) {
    int price = item.getPrice();
    if (discounted) {
        price = std::max(1, price * 9 / 10);
    }
    return UI::paint("[" + std::to_string(index) + "]", UI::Color::BrightYellow) + " " +
           UI::padRight(item.getName(), 8) + "  " +
           UI::padRight(std::to_string(price) + " 金币", 8) + "  " +
           item.getDescription();
}

std::string inventoryItemLine(int index, const Item& item, bool selling) {
    std::string line = UI::paint("[" + std::to_string(index) + "]", UI::Color::BrightYellow) +
                       " " + UI::padRight(item.getName(), 10) + " " +
                       item.getDescription();
    if (item.getKind() == ItemKind::Material) {
        line += "  [材料]";
    } else if (item.getKind() == ItemKind::Relic) {
        line += "  [遗物]";
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

std::string joinNames(const std::vector<std::string>& names) {
    std::string result;
    for (const std::string& name : names) {
        if (!result.empty()) {
            result += "、";
        }
        result += name;
    }
    return result;
}

std::string corpseHint(int stage) {
    switch (stage) {
        case 0:
            return "别在裂缝前停太久。地底会记住你的脚步声。";
        case 1:
            return "雪灾没有周期。看到日期不对，就往下走。";
        case 2:
            return "震中不在断层。所有线都指向城市正下方。";
        case 3:
            return "炉心不是神。它只是一台坏掉的机器。";
        case 4:
            return "国王关过它一次。门是留给后人的，不是给信徒的。";
        case 5:
            return "新王国不知道炉心。公文里没有的东西，不一定不存在。";
        case 6:
            return "四骑士里，只有封存还醒着。";
        case 7:
            return "门认签名，不认身份牌上的物种。";
        default:
            return "往下走，别回头。";
    }
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
      shopAvailable_(Random::chance(0.5)), resumedFromSave_(false),
      survivorTalked_(false), askedSurvivorDisaster_(false),
      askedSurvivorRuins_(false), askedSurvivorKnights_(false) {
    // 真正的初始物品在城门登记新身份时发放。
}

void Game::run() {
    showTitleScreen();
    showEngineLogo();
    showHealthReminder();
    showPrologueScreen();

    if (!startFromGate()) {
        return;
    }

    const StoryProgress progress = loadStoryProgress(player_.getName());
    storyStage_ = progress.stage;
    storyEnding_ = progress.ending;
    storyFlags_ = progress.flags;
    survivorTalked_ = hasStoryFlag("talk.survivor");
    askedSurvivorDisaster_ = hasStoryFlag("talk.disaster");
    askedSurvivorRuins_ = hasStoryFlag("talk.ruins");
    askedSurvivorKnights_ = hasStoryFlag("talk.knights");

    if (resumedFromSave_) {
        showReawakening();
    } else {
        showCityEntryNarration();
    }

    showGroundHub();
}

void Game::showTitleScreen() const {
    UI::clearScreen();

    Frame frame;
    frame.addBlank();

    constexpr int kInnerWidth = 88;
    const std::string innerTop =
        "╔" + repeatText("═", kInnerWidth - 2) + "╗";
    const std::string innerBlank =
        "║" + std::string(kInnerWidth - 2, ' ') + "║";
    const std::string innerBottom =
        "╚" + repeatText("═", 22) + "╤" + repeatText("═", 40) +
        "╤" + repeatText("═", 22) + "╝";

    const auto centerInner = [](const std::string& line) {
        return "   " + line + "   ";
    };

    frame.addCentered(UI::fit(UI::paint(centerInner(innerTop), UI::Color::Gray), 94));
    frame.addCentered(UI::fit(UI::paint(centerInner(innerBlank), UI::Color::Gray), 94));

    std::string titleLine =
        "   " + UI::paint("║", UI::Color::Gray) +
        UI::padRight(UI::center(UI::paint("沉   没   王   都",
                                          UI::Color::BrightYellow, true), 86), 86) +
        UI::paint("║", UI::Color::Gray) + "   ";
    frame.addCentered(UI::fit(titleLine, 94));

    frame.addCentered(UI::fit(UI::paint(centerInner(innerBlank), UI::Color::Gray), 94));
    frame.addCentered(UI::fit(UI::paint(centerInner(innerBottom), UI::Color::Gray), 94));

    frame.addCentered(UI::fit(UI::paint(
        "     s    ╲                    ╱          ╲              a     ╱",
        UI::Color::Gray), 64));
    frame.addCentered(UI::fit(UI::paint(
        "   s ` s   ╲      wera        ╱      x     ╲           aa     ╱",
        UI::Color::Gray), 64));
    frame.addCentered(UI::fit(UI::paint(
        "     s      ╲        sdf     ╱       x      ╲            ac  ╱",
        UI::Color::Gray), 64));

    frame.addCentered(UI::fit(UI::paint(
        "   ══════════    ═══════════════════════════    ══════════",
        UI::Color::Gray), 64));

    frame.addCentered(UI::fit(UI::paint(
        "        as  ╱           asd  ╲        sss   ╱                ╲",
        UI::Color::Gray), 64));
    frame.addCentered(UI::fit(UI::paint(
        "    hf     ╱      adsa        ╲   mki      ╱           o      ╲",
        UI::Color::Gray), 64));
    frame.addCentered(UI::fit(UI::paint(
        "          ╱  k          gsw    ╲          ╱             o      ╲",
        UI::Color::Gray), 64));

    frame.addCentered(UI::paint("·   古   ·   城   ·   咒   ·   罪   ·   墟   ·",
                                UI::Color::Gray));
    frame.addBlank();
    frame.render();

    UI::waitForEnter(UI::paint("  按回车 > ", UI::Color::BrightGreen));
}

void Game::showEngineLogo() const {
    UI::clearScreen();

    Frame frame;
    frame.addCentered(UI::paint("沉 没 王 都  ·  |引 擎|", UI::Color::Gray));
    frame.addBlank();

    // 按 README 里的不规则 0/9 外框还原：0 在左，9 在右。
    const std::vector<std::string> logo = {
        "                                0000000000000000000000000000000000000000",
        "                       0000000000000000000000000000000000000000000000000000000000",
        "                   00000000000000000000000000000000000000000000000000000000000000000000000000000",
        "                 000000 ██████ ╗ 00000000000000000000000000000000000000000000000000000000",
        "               00000 ██ ╔═════╝ 999999 █ ╗ 999999 █ ╗ 99999999999999999999999999999999999999999999999",
        "              000000 ██ ║ 999999999 ███████ ║9 ███████ ║ 99999999999999999999999999999999999999999",
        "              000000 ██ ║ 99 ██ 9999 ╚ █ ╔╝ 999 ╚ █ ╔╝ 9999999999999999999999999999999999999999999",
        "               00000 ╚██████ ╔═╝ 9999 ╚═╝ 999999 ╚═╝ 9999999999999999999999999999999999999999999999",
        "                000000 ╚═════╝ 000000000000000000000000000000000000000000000000000000000000",
        "                  000000000000000000000000000000000000000000000000000000000000000000000000",
        "                     0000000000000000000000000000000000000000000000000000000000",
        "                              0000000000000000000000000000000000000000"
    };

    const auto colorLogoLine = [](const std::string& raw) {
        // 去掉两个前导空格，最长的一行刚好收进 99 列内容区。
        std::string line = raw;
        for (int trim = 0; trim < 2 && !line.empty() && line.front() == ' '; ++trim) {
            line.erase(line.begin());
        }
        std::string result;
        int column = 0;
        for (std::size_t index = 0; index < line.size();) {
            const unsigned char first = static_cast<unsigned char>(line[index]);
            std::size_t bytes = 1;
            if ((first & 0x80) == 0) {
                bytes = 1;
            } else if ((first & 0xE0) == 0xC0) {
                bytes = 2;
            } else if ((first & 0xF0) == 0xE0) {
                bytes = 3;
            } else {
                bytes = 4;
            }
            const std::string glyph = line.substr(index, bytes);
            const double t = std::min(1.0, static_cast<double>(column) / 98.0);
            if (glyph == "0") {
                // 0 始终是紫色：左侧深紫，向右逐渐变亮。
                const int red = 88 + static_cast<int>((178 - 88) * t);
                const int green = 12 + static_cast<int>((96 - 12) * t);
                const int blue = 198 + static_cast<int>((255 - 198) * t);
                result += UI::paintRgb(glyph, red, green, blue);
            } else if (glyph == "9") {
                // 9 始终是白色：左侧带一点淡紫，向右到纯白。
                const int red = 218 + static_cast<int>((255 - 218) * t);
                const int green = 202 + static_cast<int>((255 - 202) * t);
                result += UI::paintRgb(glyph, red, green, 255);
            } else if (glyph == " ") {
                result += ' ';
            } else {
                result += UI::paint(glyph, UI::Color::BrightYellow);
            }
            ++column;
            index += bytes;
        }
        return result;
    };

    for (const std::string& line : logo) {
        frame.addLine(colorLogoLine(line));
    }

    frame.addBlank();
    frame.addCentered(UI::paint("帝王引擎：C++", UI::Color::BrightYellow));
    frame.addBlank();
    const std::string windowsTop =
        UI::paint("██", UI::Color::BrightRed) + " " +
        UI::paint("██", UI::Color::BrightGreen);
    const std::string windowsBottom =
        UI::paint("██", UI::Color::BrightBlue) + " " +
        UI::paint("██", UI::Color::BrightYellow) + "  " +
        UI::paint("Microsoft Windows", UI::Color::Gray);
    const int windowsWidth = std::max(UI::displayWidth(windowsTop),
                                      UI::displayWidth(windowsBottom));
    frame.addCentered(UI::padRight(windowsTop, windowsWidth));
    frame.addCentered(windowsBottom);

    const std::vector<std::string> decor = {
        "           ||    ||    ||   D     ===          ||                     ___",
        "            || ||  || ||    |   ||   ||    ====||  ||===||  || | ||  |___",
        "             ||     ||      |   ||   ||   ||___||  ||___||   || ||   ___|"
    };
    frame.addBlank();
    for (const std::string& line : decor) {
        frame.addCentered(UI::paint(line, UI::Color::Gray));
    }

    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showHealthReminder() const {
    UI::clearScreen();

    Frame frame;
    frame.addCentered(UI::paint("温 馨 提 醒", UI::Color::BrightYellow, true));
    frame.addBlank();

    const std::vector<std::string> heart = {
        "  ♥     ♥  ",
        " ♥       ♥ ",
        "♥         ♥",
        "♥         ♥",
        " ♥       ♥ ",
        "  ♥     ♥  ",
        "    ♥ ♥    ",
        "     ♥     "
    };
    for (const std::string& line : heart) {
        frame.addCentered(UI::paint(line, UI::Color::BrightRed));
    }

    frame.addBlank();
    frame.addCentered("长时间游玩请记得休息。");
    frame.addCentered("如果出现头晕、眼花、恶心、心悸或手部麻木，请立即停止游戏。");
    frame.addCentered("到通风处坐一会儿，喝点水；症状持续或加重，请及时就医。");
    frame.addBlank();
    frame.addCentered(UI::paint("游戏可以重来，身体只有一次。",
                                UI::Color::BrightYellow));
    frame.addCentered(UI::paint("愿你在王都里走得远，也记得回到地面。",
                                UI::Color::Gray));
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showPrologueScreen() const {
    showStoryPage(
        "序章 · 不合时令的灾难",
        {
            "先是雪。盛夏的雪压在屋脊上，像一层没有温度的灰。",
            "然后是震动。山体裂开，火山灰混着火星落下来。",
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
    frame.renderTypewriter();

    UI::waitForEnter(UI::paint("  按回车离开 > ", UI::Color::Green));
}

void Game::renderRoad() const {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("城门外 · 荒路", UI::Color::BrightCyan),
        UI::paint("身份牌：待检验", UI::Color::Gray),
        65);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::defaultLines();
    const std::vector<std::string>& gateArt = Sprites::door();
    for (int index = 0; index < 4; ++index) {
        const std::string gateLine =
            index < 3
                ? UI::paint(gateArt[static_cast<std::size_t>(index)], UI::Color::Yellow)
                : UI::paint("（我是城门）", UI::Color::Gray);
        frame.addLine(sceneRow(playerArt[static_cast<std::size_t>(index)], 10, gateLine, 84));
    }
    frame.addLine(UI::paint(floorLine(92), UI::Color::Gray));
    frame.addLine(sceneRow(
        UI::paint("旅人", UI::Color::BrightCyan), 10,
        UI::paint("城门", UI::Color::Yellow), 84));

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
                    resumedFromSave_ = true;
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
        65);
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
    rightRows.push_back(UI::center(cardBorder(true), 54));
    rightRows.push_back(UI::center(cardLine("身份牌  " + cardStatus), 54));
    rightRows.push_back(UI::center(cardLine("姓名  " + cardName), 54));
    rightRows.push_back(UI::center(cardLine("性别  " + cardGender), 54));
    rightRows.push_back(UI::center(cardLine("物种  " + cardSpecies), 54));
    rightRows.push_back(UI::center(cardLine("等级  " + cardLevel), 54));
    rightRows.push_back(UI::center(cardLine("金币  " + cardGold), 54));
    rightRows.push_back(UI::center(cardBorder(false), 54));

    const std::vector<std::string>& robot = Sprites::gatekeeper();
    const int robotRows = static_cast<int>(robot.size()) + 1;
    const int rowCount = std::max(robotRows, static_cast<int>(rightRows.size()));

    for (int row = 0; row < rowCount; ++row) {
        std::string left;
        if (row < static_cast<int>(robot.size())) {
            left = UI::center(UI::paint(robot[static_cast<std::size_t>(row)],
                                        UI::Color::Gray), 45);
        } else if (row == static_cast<int>(robot.size())) {
            left = UI::center(UI::paint("机械门卫", UI::Color::Yellow), 45);
        }

        const std::string right =
            row < static_cast<int>(rightRows.size())
                ? rightRows[static_cast<std::size_t>(row)]
                : std::string();
        frame.addSplit(left, right, 45);
    }

    frame.addBlank();
    std::vector<int> typedRows;
    if (!dialogueLine.empty()) {
        const int start = frame.rowCount();
        frame.addWrapped(dialogueLine, "  ");
        for (int index = start; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }
    }
    if (!message.empty()) {
        const int start = frame.rowCount();
        frame.addWrapped(message, "  ");
        for (int index = start; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }
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

    if (typedRows.empty()) {
        frame.render();
    } else {
        frame.renderTypewriter(typedRows);
    }
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

    saveStoryProgress(newPlayer.getName(), 0, 0, {});
    storyFlags_.clear();

    player_ = std::move(newPlayer);
    resumedFromSave_ = false;
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
    frame.addCentered("街上留着低头行走的人影。他们戴着面具，脸藏在灰烬和护目镜后面。");
    frame.addCentered("这里的人用身份牌登记身份。假身份、抢劫和失踪，都没人过问。");
    frame.addCentered("一个披灰布的人从巷口看了你一眼，又低下头。");
    frame.addCentered("门卫的识别声已停，胸前的临时身份牌却仍透着凉意。");
    frame.addBlank();
    frame.addCentered(UI::paint(player_.getName() + "，欢迎来到沉没的王都。",
                                UI::Color::BrightYellow));
    frame.addCentered(UI::paint("身份牌：" + player_.getName() + " · " +
                                player_.getGender() + " · " + player_.getSpecies(),
                                UI::Color::Gray));
    frame.addBlank();
    frame.renderTypewriter();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showReawakening() const {
    showStoryPage(
        "梦 · 沉没的王都",
        {
            "你又梦见那座城。街道倒悬在头顶，窗户里没有灯。",
            "无脸的人从你身边走过，胸前的身份牌轻轻相撞。",
            "城市中央，一台熄灭的炉子还在发热。一个老人背对着你。",
            "你想问他为什么。他先开口：‘你终于回来了。’",
            "你想说自己不是回来，只是被带到了这里。可梦里没有声音。"
        },
        UI::Color::BrightMagenta,
        "  按回车继续 > ");

    showStoryPage(
        "醒来 · 营地的火",
        {
            "你在一间半塌的屋子里醒来。火堆快灭了，身份牌压在胸口。",
            "梦里的话还留在耳边。你分不清那是记忆，还是别人塞给你的。",
            "你记得自己来过这里，却想不起来最初为什么出发。",
            "像是有人在你胸口系了一根很细的线，另一端在地底。"
        },
        UI::Color::BrightCyan,
        "  按回车继续 > ");

    std::vector<std::string> memory = {
        "你想起荒路、城门和那个机械门卫。",
        "你记得自己迈出每一步，也记得自己可以停下。",
        "可你偏偏没有停。仿佛来这里并不是你的意愿，",
        "而是某个更古老的东西，先替你做了决定。"
    };

    if (storyStage_ >= kStoryClueCount + 1) {
        memory.push_back("你记得自己已经站在炉心前，做过一个不能撤回的选择。");
    } else if (storyStage_ == kStoryClueCount) {
        memory.push_back("你记得核心的门已经打开，门后有什么在等你。");
    } else if (storyStage_ > 0) {
        memory.push_back("你记得自己已经往下走过 " + std::to_string(storyStage_) +
                         " 段路。");
    } else {
        memory.push_back("你记得自己还没有真正走进遗迹。");
    }
    memory.push_back("火堆噼啪响了一声。你站起来，决定先弄清楚那根线通向哪里。");

    showStoryPage("回忆 · 来时的路", memory, UI::Color::BrightYellow,
                  "  按回车继续 > ");
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
    saveStoryProgress(player_.getName(), storyStage_, storyEnding_, storyFlags_);
    unlockJournal("clue." + std::to_string(storyStage_));
    return true;
}

bool Game::hasStoryFlag(const std::string& flag) const {
    return storyFlags_.find(flag) != storyFlags_.end();
}

void Game::setStoryFlag(const std::string& flag) {
    if (storyFlags_.insert(flag).second) {
        saveStoryProgress(player_.getName(), storyStage_, storyEnding_, storyFlags_);
    }
}

bool Game::hasRelic() const {
    const auto& items = player_.getInventory().getItems();
    for (const std::unique_ptr<Item>& item : items) {
        if (item->getKind() == ItemKind::Relic) {
            return true;
        }
    }
    return false;
}

bool Game::takeRelic(std::string& relicName) {
    Inventory& inventory = player_.getInventory();
    for (int index = 0; index < inventory.size(); ++index) {
        const Item* item = inventory.getItem(index);
        if (item != nullptr && item->getKind() == ItemKind::Relic) {
            relicName = item->getName();
            inventory.takeItem(index);
            return true;
        }
    }
    return false;
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
        saveStoryProgress(player_.getName(), storyStage_, storyEnding_, storyFlags_);
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
        UI::paint("地面枢纽 · 废墟营地", UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "    背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        65);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const std::vector<std::string>& fireArt = Sprites::campfire();
    for (int index = 0; index < 4; ++index) {
        frame.addColumns(
            UI::center(playerArt[static_cast<std::size_t>(index)], 49),
            UI::center(UI::paint(fireArt[static_cast<std::size_t>(index)],
                                 UI::Color::BrightYellow), 49),
            49);
    }
    frame.addColumns(
        UI::center(UI::paint(player_.getName(), UI::Color::BrightCyan), 49),
        UI::center(UI::paint("火堆", UI::Color::Yellow), 49),
        49);

    frame.addBlank();

    frame.addLine(UI::paint(player_.getName(), UI::Color::BrightCyan) +
                  "  Lv." + std::to_string(player_.getLevel()));
    frame.addLine(
        "HP " + UI::bar(player_.getHp(), player_.getMaxHp(), 30,
                        UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()));
    frame.addLine(
        "MP " + UI::bar(player_.getMp(), player_.getMaxMp(), 30, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()));
    frame.addLine(
        "EXP " + UI::bar(player_.getExperience(), player_.getExperienceToNextLevel(), 30,
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
    std::vector<int> typedRows;
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
        const int start = frame.rowCount();
        frame.addWrapped(message, "· ");
        for (int index = start; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }
    }

    frame.addDivider();
    const std::string shopLabel = shopAvailable_ ? "商店" : "与人交易";
    const UI::Color talkColor =
        survivorTalked_ ? UI::Color::Gray : UI::Color::BrightYellow;
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 查看状态   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 背包   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " " + shopLabel + "   " +
        UI::paint("[4]", talkColor) + " " + UI::paint("交谈", talkColor));

    std::string exploreLabel = "进入遗迹";
    if (storyStage_ >= kStoryClueCount + 1) {
        exploreLabel = "回望核心";
    } else if (storyStage_ == kStoryClueCount) {
        exploreLabel = "进入核心";
    }
    frame.addLine(
        UI::paint("[5]", UI::Color::BrightYellow) + " 见闻   " +
        UI::paint("[6]", UI::Color::BrightYellow) + " " + exploreLabel + "   " +
        UI::paint("[7]", UI::Color::BrightYellow) + " 休息   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 返回枢纽（自动存档）");

    if (typedRows.empty()) {
        frame.render();
    } else {
        frame.renderTypewriter(typedRows);
    }
}

void Game::renderCorridor(int floor, int depth, int areaStage,
                          int playerScreen, int camera,
                          const std::vector<std::string>* previewArt,
                          UI::Color previewColor) const {
    UI::clearScreen();

    const AreaInfo& area = areaForStage(areaStage);

    Frame frame;
    frame.addColumns(
        UI::paint(area.act + " · " + area.name + " · 第 " +
                      std::to_string(floor) + " 层",
                  UI::Color::BrightCyan),
        UI::paint("深度 " + std::to_string(depth) + "    线索 " +
                      std::to_string(std::min(storyStage_, kStoryClueCount)) +
                      "/" + std::to_string(kStoryClueCount),
                  UI::Color::Gray),
        60);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);

    // 6 行通道画面：0—1 火把，2—5 角色 / 事件，随后是地面横线。
    constexpr int kSceneRows = 6;
    std::vector<std::string> scene(kSceneRows);

    const auto put = [&](int row, int column, const std::string& text) {
        if (row < 0 || row >= kSceneRows || column < 0 || column >= 99) {
            return;
        }
        appendAtColumn(scene[static_cast<std::size_t>(row)], text, column);
    };

    // 三支火把把画面分成四块：15 / 45 / 75 为世界坐标，镜头移动时整体左移。
    for (int worldX = 15; worldX < 300; worldX += 30) {
        const int screenX = worldX - camera;
        if (screenX < 0 || screenX >= 99) {
            continue;
        }
        put(0, screenX, UI::paint("火", UI::Color::Yellow));
        put(1, screenX, UI::paint("把", UI::Color::Yellow));
    }

    // 角色从第二行开始，避开头和火把“把”字共行造成的错位。
    for (int index = 0; index < 4; ++index) {
        put(2 + index, playerScreen, playerArt[static_cast<std::size_t>(index)]);
    }

    // 预显示事件画在右侧，底部贴着地面横线；陷阱和塌方不预显示。
    if (previewArt != nullptr && !previewArt->empty()) {
        int artWidth = 0;
        for (const std::string& line : *previewArt) {
            artWidth = std::max(artWidth, UI::displayWidth(line));
        }
        const int previewX = 90 - camera - artWidth / 2;
        const int artRows = static_cast<int>(previewArt->size());
        const int startRow = 6 - artRows;
        if (previewX >= 0 && previewX + artWidth < 99) {
            for (int index = 0; index < artRows; ++index) {
                const int row = startRow + index;
                if (row < 0 || row >= 6) {
                    continue;
                }
                put(row, previewX,
                    UI::paint(UI::center(
                                  (*previewArt)[static_cast<std::size_t>(index)],
                                  artWidth),
                              previewColor));
            }
        }
    }

    for (const std::string& row : scene) {
        frame.addLine(row);
    }

    frame.addLine(UI::paint(floorLine(92), UI::Color::Gray));
    frame.addLine(sceneRow(
        UI::paint(player_.getName(), UI::Color::BrightCyan), playerScreen,
        UI::paint("通道", UI::Color::Yellow), 84));

    frame.addBlank();
    frame.addWrapped("火把照亮一截湿冷的石廊。前面没有固定的门，只有更深处的黑暗。", "  ");
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 继续前进   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 查看状态   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 背包   " +
        UI::paint("[4]", UI::Color::BrightYellow) + " 返回营地");

    frame.render();
}

void Game::handleCorridorTrap(int floor) {
    const int rawDamage = 2 + std::max(0, floor) + Random::integer(0, 2);
    const int hpBefore = player_.getHp();
    const int damage = std::min(rawDamage, std::max(0, hpBefore - 1));
    const int lost = player_.takeDamage(damage);

    showStoryPage(
        "通道 · 陷阱",
        {
            "脚下的石板突然下陷，锈蚀的尖刺从缝里刺出来。",
            "你损失了 " + std::to_string(lost) + " 点 HP。",
            lost == 0 ? "你勉强避开了要害。" : "伤口不深，但血腥味会招来东西。"
        },
        UI::Color::BrightRed,
        "  按回车继续 > ");
}

void Game::handleCorridorCaveIn(int floor, int& nextFloor) {
    nextFloor = floor + 1;
    const int rawDamage = 3 + std::max(0, floor) + Random::integer(0, 2);
    const int hpBefore = player_.getHp();
    const int damage = std::min(rawDamage, std::max(0, hpBefore - 1));
    const int lost = player_.takeDamage(damage);

    showStoryPage(
        "通道 · 塌方",
        {
            "头顶的支撑结构发出断裂声。你来不及后退。",
            "碎石把你推向下一层。你损失了 " + std::to_string(lost) + " 点 HP。",
            "灰尘散去时，你落在第 " + std::to_string(nextFloor) + " 层。"
        },
        UI::Color::BrightYellow,
        "  按回车继续 > ");
}

void Game::showHangedCorpseCG() const {
    UI::clearScreen();

    Frame frame;
    const std::vector<std::string> art = {
        "hbddbbbbhhhabaahaaaaaaaaaaaaaaaaaaaaaaahaabhhhahhkkhhkkkkbbkkkkbkkhkkkhhkkahhahahhaahaaaaakkppddkbd",
        "ddhbdkbdkhkakaaaaaaaaaaahhkaaaaaaaaaaaahhabbkhhbdpdbbbpbbpbkbadpbdbdbbkkddaaahkaaahakahhhkkkkdqqhbp",
        "wdppdhbqkhdhbaakhaaaaaakhkhkhaaaaaaaaaahdkpdddbqmpqwqwqqqpdhkbdmZqpmqpdpbdhhaahkaahhhhkkhbkbpqwmkbb",
        "dbqpqwpqhkdbdaakdaahaakbkahkkkbhahaaaahbhbpqwpqqOJOmOwqZmkwkbmw0QXOmZqqhqbdbkbkbhdhhhkbbpbkpwbbqmqb",
        "pppqwpqmkkpbphkbkakhhakkbdbdbbbkhkkkkZphbdqOmqwpLQJZQQOQ0Zmp0qLCciJ0CCQqqppdbbbbhkkkhdbqddkkpmqpwwb",
        "hbmwqwqOmkwddakpbhbabhbdbdddbddkkkbqbZbkkdmQp0pZQCc0QL0QOXQZC0JvrU|YcZQkmwqqppppbkbdkpddbmdbbQwwOmd",
        "dq0ZmwwbqkwdqkbddahaaakbdbhkbppdkbwwbqhkdwbZqdZOYYCqLzJXC0Z0UvJYunhOv00LpqkkkddkhkkhbppqkmbbwLOpqZd",
        "ZwLZLqqwqbwbppmwbddaaakZdOdqbpdpdpmdwmOkdppkOqphUJOmJYQuOUCZmp0zx0hhLLOZwqphhqpddadkqdpZbOkbOOOwqdm",
        "QmULJCmwpbUbOqbbpbhbakhwpZpLbwqqmwZ0pwwmbQmCJZ0Lm0wZQJQJ0OwwOL0QOpdpqw0ZpkOphdbmwhdbqpwZpmbbqOOmCkm",
        "cczYCi-wwbZhOpZdwqkhhdbbwQZdwpOqLqLmdZcnmZm00ppqmw0CQ0U0ZQLUfwhkdZqbZmOhkabpahbpwbqwpqZZbZqqZ1ZQLdp",
        "XYvL~zmQZOmbOOQmdpdhhqqZJmkhwZZQXQ00pphqOLcjzqhkkkChj\\f/zJCkaaaahbdbbbpmpdhwpwOdmZwqkqz0pQwbZiuLjYJ",
        "00LLahO0Cd0tnjJw+dphqmmZwbqZZwOO+i{cQdhbbwpObpkkhkq\\fkwLmZkaahdhqpdqmbdZpbpkmpOwmZmQX0uYXzC00zzJxYX",
        "wpQOhq0LUbYUYzJ}YdbqOqwQZZpZqZOJbkuZOLbbhkkbdbqw0OvULqdbkhaqbkZdmdbbppdqhadwwbQC0qpm>0\\vnCC0wqawdqQ",
        "pdmabhkphppkpUdkpqZwqmqqZdmOdkOmpmwqqpqddmbbahaOQvcULahbbdwdmkOq0mddqdOqkpbm0O0pLq0OhZCOddLCQObhkap",
        "mkhhbbqdhdbhbp0kqwww00OmkwdbOpdbbbwdZbdqdmqpdhaL0YzcudpqOwqdpqkpdhbbhkw0bwqkmZZOwqbpbphZqmwmwwmadaw",
        "qqdwhdqbbmqkZpwqdqpdOqqwmwpqbbkhbbbkwqqmmbpkbhhXzCzfYObwwqhaQOdZphpbkkZmmhkmddqdwwkpbhkwmmqw0mmmmhp",
        "khahaaabdhbhpwqphmOqpwpmqmbdkppdqkkawdpwwqbbkhzuzfCUXCqmmpbdpmbwmhkhhddwm0wqpkmdbZbmbdhbqwkwdqmqaah",
        "bqkpaakhqahhwddkkhdkqdwwmbbddpddkddbwhpbZpphbkCuQzUuvmpO0QkapqkqpahpadZmqqpaaaaadwwbbdabQhaaahbZdbd",
        "pbdkaawhpapwhdbhkhhhdappdbdbbwdpphpqhaqbdbwqbhZvzaCuQpmbhkaaqpppbpkdaapbOaahhkdkkahdakk0ZaabppZdpdb",
        "hdkbaaadhdkpqpdkbkhakakdbdpphbpbkkkdaapbqddbahhmmaCYakpdhbaapwkdwkpqpqqwpahdZppbpbpddkdwkaawqqmaqkb",
        "hkhkhaqkbbhqabhhdhhahdqbhhqpddpbbpdqppkpbqhpkkahpahpabwhkpdqwpdwdhkdawmZhhdqpmqdpqhhahqqkaamOZdakkk",
        "dakdaaaahaaaaaaahpadpwbhkkbddpddkbkbqbddpqddhaabZabOZhqbhpppqdqkddbdbbbdbhqqwppwqpkbkkbpddbwqphapqd",
        "kabaaaaaaaaaqaaahahkadbkkdbbhdkppdbbbbbppdOkhaahmadmhadhhppqbpbdkakbbdbhkapmppqZddhhhkkbdkhpqQbadkb",
        "habaaaaaaaaakaakaakhadkabhbkhqhhbbbkbhhpwkhdhhaaqhwqaahhhwbhppbkbhbbdbkkdaZhqkkkaaakhkhkbqbhppwdbbb",
        "hhhaaaaaaaaaaaahahahabkaaaahaaaaakkbkhhkahhhhaaabLZbabahkhbbdbdkkkpaqqhbbaaahhkdqkkhhahbhaadppbakkb",
        "khhaaaaaaaaaaaaahpdhahahahaaahahkhahhahhhhkaaaaaqk0haadhhhakhbkhkkkhhhhkkaabkhhbhkhhaakahaaahdbhhka",
        "hhaaaaaaaaaaaaaaakhhabkbbkkbbkbbkdbdkmmmqqqbhhaaakhahhhbhkdbkhkdqwpbkpqqdpddkdhhhkhhhhaahaaakbkakhh",
        "hhaaaaaaaaaaaaakaahbdddbwwpmqmppmZZZqOpZ0Zmb0bpppqqO0ZZmmpZqpmZZOZmwppmqmqqpmpqqwwpkdbkkkpbbahhahkh"
    };
    struct CorpseTone {
        int red;
        int green;
        int blue;
    };
    const auto corpseTone = [](char character) -> CorpseTone {
        // 火把、绳索和少量高光保留一点暖色。
        if (character == '\\' || character == '/' || character == '|' ||
            character == '{' || character == '}' || character == '>' ||
            character == '~' || character == '+' || character == '-' ||
            character == '1') {
            return {188, 118, 54};
        }

        double density = 0.55;
        switch (character) {
            case 'a': density = 0.08; break;
            case 'h': density = 0.16; break;
            case 'b': density = 0.26; break;
            case 'd': density = 0.32; break;
            case 'k': density = 0.38; break;
            case 'p': density = 0.46; break;
            case 'q': density = 0.54; break;
            case 'w': density = 0.62; break;
            case 'm': density = 0.68; break;
            case 'Z': density = 0.78; break;
            case 'O': density = 0.82; break;
            case 'Q': density = 0.90; break;
            case '0': density = 0.95; break;
            case 'L': density = 0.86; break;
            case 'C': density = 0.80; break;
            case 'J': density = 0.78; break;
            case 'Y': density = 0.74; break;
            case 'U': density = 0.72; break;
            default: break;
        }

        struct Stop {
            double at;
            int red;
            int green;
            int blue;
        };
        static const Stop stops[] = {
            {0.00, 26, 33, 29},
            {0.22, 42, 54, 44},
            {0.42, 64, 60, 46},
            {0.58, 88, 78, 60},
            {0.74, 120, 122, 110},
            {0.88, 158, 160, 148},
            {1.00, 190, 184, 160}
        };
        constexpr int kStopCount = static_cast<int>(sizeof(stops) / sizeof(stops[0]));
        for (int index = 0; index < kStopCount - 1; ++index) {
            if (density <= stops[index + 1].at) {
                const double span = stops[index + 1].at - stops[index].at;
                const double ratio = span > 0.0
                                         ? (density - stops[index].at) / span
                                         : 0.0;
                return {
                    stops[index].red +
                        static_cast<int>((stops[index + 1].red - stops[index].red) * ratio),
                    stops[index].green +
                        static_cast<int>((stops[index + 1].green - stops[index].green) * ratio),
                    stops[index].blue +
                        static_cast<int>((stops[index + 1].blue - stops[index].blue) * ratio)
                };
            }
        }
        return {stops[kStopCount - 1].red, stops[kStopCount - 1].green,
                stops[kStopCount - 1].blue};
    };

    for (const std::string& line : art) {
        std::string painted;
        painted.reserve(line.size() * 24);
        for (char character : line) {
            if (character == ' ') {
                painted += ' ';
                continue;
            }

            const CorpseTone tone = corpseTone(character);
            painted += UI::paintRgb(std::string(1, character),
                                    tone.red, tone.green, tone.blue);
        }
        frame.addLine(painted);
    }
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::handleCorridorCorpse() {
    showHangedCorpseCG();
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("通道 · 尸体", UI::Color::BrightCyan),
        UI::paint("搜索", UI::Color::Gray),
        65);
    frame.addDivider();
    frame.addBlank();

    for (const std::string& line : Sprites::corpse()) {
        frame.addCentered(UI::paint(line, UI::Color::Gray));
    }

    frame.addBlank();
    const int textStart = frame.rowCount();
    frame.addWrapped("尸体靠在墙边，身上的破布已经和灰尘粘在一起。", "  ");
    frame.addWrapped("他手里像攥着什么，也可能只是临死前的姿势。", "  ");
    std::vector<int> typedRows;
    for (int index = textStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 搜身   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
    frame.renderTypewriter(typedRows);

    const int choice = UI::readChoice(
        UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
    if (choice == 0 || choice == -1) {
        return;
    }

    std::vector<std::string> lines;
    const int roll = Random::integer(0, 99);
    if (roll < 40) {
        const int gold = Random::integer(10, 30);
        player_.gainGold(gold);
        lines.push_back("尸体腰间的钱袋里还有 " + std::to_string(gold) + " 枚金币。");
    } else if (roll < 62) {
        std::unique_ptr<Item> item =
            std::make_unique<Potion>("红药水", 8, Potion::Effect::RestoreHp, 15);
        const std::string name = item->getName();
        if (player_.getInventory().canAdd()) {
            player_ + std::move(item);
            lines.push_back("你找到一瓶还没冻住的" + name + "。");
        } else {
            const int gold = item->getSellPrice();
            player_.gainGold(gold);
            lines.push_back("背包已满，你把" + name + "折成了 " +
                            std::to_string(gold) + " 枚金币。");
        }
    } else if (roll < 78) {
        std::unique_ptr<Item> item;
        if (Random::chance(0.5)) {
            item = std::make_unique<Weapon>("旧王都短剑", 30, 3);
        } else {
            item = std::make_unique<Armor>("旧王都护甲", 35, 3);
        }
        const std::string name = item->getName();
        if (player_.getInventory().canAdd()) {
            player_ + std::move(item);
            lines.push_back("你从他身下抽出一件" + name + "。");
        } else {
            const int gold = item->getSellPrice();
            player_.gainGold(gold);
            lines.push_back("背包已满，你把" + name + "折成了 " +
                            std::to_string(gold) + " 枚金币。");
        }
    } else {
        lines.push_back("他手里攥着一张被血浸透的纸条：");
        lines.push_back("‘" + corpseHint(storyStage_) + "’");
    }

    showResult("尸体 · 搜索", lines, UI::Color::Gray);
}

void Game::showAncestorRoom() {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("隐藏房间 · 地下五层", UI::Color::BrightCyan),
        UI::paint("先祖遗骸", UI::Color::Gray),
        65);
    frame.addDivider();
    frame.addBlank();

    for (const std::string& line : Sprites::ancestor()) {
        frame.addCentered(UI::paint(line, UI::Color::Gray));
    }

    frame.addBlank();
    const int textStart = frame.rowCount();
    frame.addWrapped("塌陷矿道的尽头，有一间被封死的休息室。", "  ");
    frame.addWrapped("梁上垂下半截绳索。遗骸下面的矿灯早已熄灭。", "  ");
    frame.addWrapped("墙上有一个缺角的炉形记号，和你梦里的很像。", "  ");
    std::vector<int> typedRows;
    for (int index = textStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 调查遗骸   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
    frame.renderTypewriter(typedRows);

    setStoryFlag("ancestor.found");
    unlockJournal("ancestor.room");

    const int choice = UI::readChoice(
        UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
    if (choice == 0 || choice == -1) {
        return;
    }

    showStoryPage(
        "调查 · 绞索与遗骸",
        {
            "绳索已经干硬。遗骸的衣料不是这个时代的样式。",
            "矿灯、镐、半张身份牌。牌上的名字被磨掉了。",
            "墙上有古国文字和一个家族记号。你梦里缺角的炉形，和它几乎一样。",
            "他不是第一个顺着同一个梦来到这里的人。"
        },
        UI::Color::Gray,
        "  按回车继续 > ");

    showStoryPage(
        "遗物 · 互相矛盾的说法",
        {
            "官方档案说他是煽动矿工、破坏开采的叛徒。",
            "商人说遗骸只是替身，家族记号是后来刻上去的。",
            "考古者说这可能是更早的矿工；四骑士认得记号，却未必愿意说。",
            "没有一种说法能完全对上。但有一件事很清楚：这条路，已经有人走过。"
        },
        UI::Color::BrightYellow,
        "  按回车继续 > ");

    if (!hasRelic()) {
        if (!player_.getInventory().canAdd()) {
            showStoryPage(
                "获得 · 刻纹石板",
                {
                    "你在遗骸下面摸到一块石板，但背包已经满了。",
                    "你记住了位置，决定下次再来取。"
                },
                UI::Color::BrightCyan,
                "  按回车继续 > ");
            return;
        }
        player_ + std::make_unique<Relic>("刻纹石板", 80);
        showStoryPage(
            "获得 · 刻纹石板",
            {
                "你在遗骸下面摸到一块石板，表面刻满细密的古国文字。",
                "普通拾荒者大概不敢收这种东西。地面商店的老板也许能看懂。"
            },
            UI::Color::BrightCyan,
            "  按回车继续 > ");
    }

    setStoryFlag("ancestor.read");
    unlockJournal("ancestor.truth");
}

bool Game::handleCorridorDoor(int& floor, std::string& message) {
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("通道 · 门", UI::Color::BrightCyan),
        UI::paint("第 " + std::to_string(floor) + " 层", UI::Color::Gray),
        65);
    frame.addDivider();
    frame.addBlank();

    for (const std::string& line : Sprites::door()) {
        frame.addCentered(UI::paint(line, UI::Color::Yellow));
    }

    frame.addBlank();
    const int textStart = frame.rowCount();
    frame.addWrapped("通道尽头多了一扇门。门缝里没有光，也没有风。", "  ");
    frame.addWrapped("门后可能是房间、废弃营地，或者向下的石阶。", "  ");
    std::vector<int> typedRows;
    for (int index = textStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 推门进入   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 继续沿通道前进");
    frame.renderTypewriter(typedRows);

    const int choice = UI::readChoice(
        UI::paint("  选择 > ", UI::Color::BrightGreen), 1, 2);
    if (choice == 2 || choice == -1) {
        return true;
    }

    const int roll = Random::integer(0, 99);
    if (roll < 35) {
        bool defeated = false;
        message = showRewardRoom(defeated);
        return !defeated;
    }

    if (roll < 75) {
        const int hpHeal = player_.heal(player_.getMaxHp() / 2);
        const int mpHeal = player_.restoreMp(player_.getMaxMp() / 2);
        showStoryPage(
            "房间 · 废弃营地",
            {
                "门后是一间被遗弃的营地。火堆早灭了，但墙角还有干柴。",
                "你重新点燃火堆，休息了一会儿。",
                "恢复了 " + std::to_string(hpHeal) + " 点 HP、" +
                    std::to_string(mpHeal) + " 点 MP。"
            },
            UI::Color::BrightYellow,
            "  按回车继续 > ");
        return true;
    }

    ++floor;
    showStoryPage(
        "通道 · 向下的石阶",
        {
            "门后没有房间，只有一段向下的石阶。",
            "你沿着石阶走进更深的一层。空气变得更热，回声也更近。",
            "你现在位于第 " + std::to_string(floor) + " 层。"
        },
        UI::Color::BrightCyan,
        "  按回车继续 > ");
    return true;
}

std::string Game::handleDefeat(const std::string& enemyName) {
    const int lostGold = std::min(player_.getGold(), Random::integer(5, 15));
    if (lostGold > 0) {
        player_.spendGold(lostGold);
    }

    std::vector<std::string> lostItems;
    const int itemLoss = Random::integer(0, 2);
    for (int index = 0; index < itemLoss && !player_.getInventory().empty(); ++index) {
        const int pick = Random::integer(0, player_.getInventory().size() - 1);
        std::unique_ptr<Item> item = player_.getInventory().takeItem(pick);
        if (item) {
            lostItems.push_back(item->getName());
        }
    }

    player_.restoreAll();

    std::vector<std::string> lines;
    lines.push_back("你被" + enemyName + "击倒。");
    lines.push_back("醒来时，你回到了地面，城中的营地。");
    if (lostGold > 0) {
        lines.push_back("丢失了 " + std::to_string(lostGold) + " 枚金币。");
    }
    if (!lostItems.empty()) {
        lines.push_back("丢失物品：" + joinNames(lostItems) + "。");
    } else {
        lines.push_back("背包里的东西还在。");
    }
    showResult("战  败", lines, UI::Color::BrightRed);

    return "你在城中的营地醒来。检查损失后，准备再次出发。";
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
        "HP   " + UI::bar(player_.getHp(), player_.getMaxHp(), 30,
                          UI::healthColor(player_.getHp(), player_.getMaxHp())) +
        "  " + std::to_string(player_.getHp()) + "/" + std::to_string(player_.getMaxHp()));
    frame.addLine(
        "MP   " + UI::bar(player_.getMp(), player_.getMaxMp(), 30, UI::Color::BrightBlue) +
        "  " + std::to_string(player_.getMp()) + "/" + std::to_string(player_.getMaxMp()));
    frame.addLine(
        "EXP  " + UI::bar(player_.getExperience(), player_.getExperienceToNextLevel(), 30,
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

        std::vector<int> typedRows;
        int start = frame.rowCount();
        frame.addWrapped("他没有脸。胸前那块身份牌磨得发白，只剩一个看不清的姓。", "  ");
        for (int index = start; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }
        frame.addBlank();
        start = frame.rowCount();
        frame.addWrapped(message, "  ");
        for (int index = start; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }

        frame.addDivider();
        const UI::Color disasterColor =
            askedSurvivorDisaster_ ? UI::Color::Gray : UI::Color::BrightYellow;
        const UI::Color ruinsColor =
            askedSurvivorRuins_ ? UI::Color::Gray : UI::Color::BrightYellow;
        const UI::Color knightsColor =
            askedSurvivorKnights_ ? UI::Color::Gray : UI::Color::BrightYellow;
        frame.addLine(
            UI::paint("[1]", disasterColor) + " " +
            UI::paint("问灾难", disasterColor) + "   " +
            UI::paint("[2]", ruinsColor) + " " +
            UI::paint("问遗迹", ruinsColor) + "   " +
            UI::paint("[3]", knightsColor) + " " +
            UI::paint("问四骑士", knightsColor) + "   " +
            UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
        frame.renderTypewriter(typedRows);

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 3);
        if (choice == 0 || choice == -1) {
            return;
        }

        switch (choice) {
            case 1:
                askedSurvivorDisaster_ = true;
                survivorTalked_ = true;
                setStoryFlag("talk.disaster");
                setStoryFlag("talk.survivor");
                unlockJournal("talk.disaster");
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
                askedSurvivorRuins_ = true;
                survivorTalked_ = true;
                setStoryFlag("talk.ruins");
                setStoryFlag("talk.survivor");
                unlockJournal("talk.ruins");
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
                askedSurvivorKnights_ = true;
                survivorTalked_ = true;
                setStoryFlag("talk.knights");
                setStoryFlag("talk.survivor");
                unlockJournal("talk.knights");
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

void Game::unlockJournal(const std::string& id) {
    setStoryFlag("journal." + id);
}

void Game::showJournal() {
    const std::vector<JournalEntry>& catalog = journalEntries();

    while (true) {
        std::vector<const JournalEntry*> unlocked;
        for (const JournalEntry& entry : catalog) {
            if (hasStoryFlag("journal." + entry.id)) {
                unlocked.push_back(&entry);
            }
        }

        UI::clearScreen();
        Frame frame;
        frame.addColumns(
            UI::paint("见闻", UI::Color::BrightCyan),
            UI::paint("已收集 " + std::to_string(unlocked.size()) + " / " +
                          std::to_string(catalog.size()),
                      UI::Color::Gray),
            65);
        frame.addDivider();
        frame.addBlank();

        if (unlocked.empty()) {
            frame.addLine("还没有见闻。去和营地里的人聊聊，或者深入遗迹。");
        } else {
            for (int index = 0; index < static_cast<int>(unlocked.size()); ++index) {
                const JournalEntry& entry = *unlocked[static_cast<std::size_t>(index)];
                const bool unread = !hasStoryFlag("journalread." + entry.id);
                std::string line = UI::paint("[" + std::to_string(index + 1) + "]",
                                             UI::Color::BrightYellow) +
                                   " ";
                if (unread) {
                    line += UI::paint("[新] ", UI::Color::BrightGreen);
                }
                line += UI::paint("[" + entry.category + "] ", UI::Color::Gray) +
                        entry.title;
                frame.addLine(line);
            }
        }

        frame.addBlank();
        frame.addDivider();
        frame.addLine(UI::paint("[0]", UI::Color::BrightYellow) + " 返回");
        frame.render();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0,
            static_cast<int>(unlocked.size()));
        if (choice == 0 || choice == -1) {
            return;
        }

        const JournalEntry& entry = *unlocked[static_cast<std::size_t>(choice - 1)];
        showStoryPage(entry.category + " · " + entry.title, entry.lines,
                      UI::Color::BrightCyan, "  按回车返回见闻 > ");
        setStoryFlag("journalread." + entry.id);
    }
}

void Game::renderHub(int locationIndex, int playerScreen,
                     const std::string& message) const {
    const std::vector<std::string>& names = hubLocationNames();
    const std::vector<int>& positions = hubLocationPositions();

    UI::clearScreen();
    Frame frame;
    frame.addColumns(
        UI::paint("地面枢纽 · 灰烬街道", UI::Color::BrightCyan),
        UI::paint("位置：" + names[static_cast<std::size_t>(locationIndex)] +
                      "    金币 " + std::to_string(player_.getGold()),
                  UI::Color::Yellow),
        55);
    frame.addDivider();

    constexpr int kSceneRows = 20;
    std::vector<std::string> scene(kSceneRows);

    // 场景元素按“行 → 列”排序后再拼接，避免后画的人物被追加到右侧灯杆之后。
    struct SceneElement {
        int row;
        int column;
        int width;
        std::string text;
    };
    std::vector<SceneElement> elements;
    const auto add = [&](int row, int column, const std::string& text) {
        if (row < 0 || row >= kSceneRows || column < 0 || column >= 99) {
            return;
        }
        elements.push_back({row, column, UI::displayWidth(text), text});
    };

    // 路灯：\灯/ 灯头、柱、|| 短杆；杆固定 5 行高，并让杆底踩在地面线上。
    constexpr int kLampPoleRows = 5;
    const int lampTop = kSceneRows - (kLampPoleRows + 2);
    for (int x : {6, 26, 46, 66, 86}) {
        add(lampTop, x, UI::paint("\\灯/", UI::Color::BrightYellow));
        add(lampTop + 1, x + 1, UI::paint("柱", UI::Color::Gray));
        for (int row = lampTop + 2; row < kSceneRows; ++row) {
            add(row, x + 1, UI::paint("||", UI::Color::Gray));
        }
    }

    const std::vector<std::string> playerArt = PlayerSprite::lines(player_);
    const int playerFirstRow = kSceneRows - 4;

    // 人物走到灯杆前面时，先移除同一格上的灯杆，避免拼接时被挤到后面。
    elements.erase(
        std::remove_if(elements.begin(), elements.end(),
                       [&](const SceneElement& element) {
                           if (element.row < playerFirstRow) {
                               return false;
                           }
                           const int playerWidth = UI::displayWidth(
                               playerArt[static_cast<std::size_t>(
                                   element.row - playerFirstRow)]);
                           return element.column < playerScreen + playerWidth &&
                                  playerScreen < element.column + element.width;
                       }),
        elements.end());

    for (int index = 0; index < 4; ++index) {
        const std::string& line = playerArt[static_cast<std::size_t>(index)];
        elements.push_back({playerFirstRow + index, playerScreen,
                            UI::displayWidth(line), line});
    }

    std::sort(elements.begin(), elements.end(),
              [](const SceneElement& left, const SceneElement& right) {
                  if (left.row != right.row) {
                      return left.row < right.row;
                  }
                  return left.column < right.column;
              });
    for (const SceneElement& element : elements) {
        appendAtColumn(scene[static_cast<std::size_t>(element.row)],
                       element.text, element.column);
    }

    for (const std::string& row : scene) {
        frame.addLine(row);
    }

    frame.addLine(UI::paint(floorLine(96), UI::Color::Gray));

    std::string labelRow;
    for (int index = 0; index < static_cast<int>(positions.size()); ++index) {
        const UI::Color color = (index == locationIndex)
                                    ? UI::Color::BrightCyan
                                    : UI::Color::Gray;
        appendAtColumn(labelRow, UI::paint(names[static_cast<std::size_t>(index)], color),
                       positions[static_cast<std::size_t>(index)]);
    }
    frame.addLine(labelRow);

    frame.addBlank();
    frame.addWrapped(message.empty()
                         ? "灰烬落在路灯下。四处的门都半掩着，街道尽头是封闭的高塔。"
                         : message,
                     "  ");
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 向左   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 向右   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 交互   " +
        UI::paint("[4]", UI::Color::BrightYellow) + " 见闻   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开游戏");
    frame.render();
}

void Game::showGroundHub() {
    const std::vector<int>& positions = hubLocationPositions();
    int location = 0;
    std::string message = "灰烬落在路灯下。你回到地面枢纽。";
    bool running = true;

    const auto saveAfterAction = [&]() {
        std::string error;
        if (!SaveGame::save(player_, error)) {
            message = "自动存档失败：" + error;
        }
    };

    const auto animateMove = [&](int from, int to) {
        if (!UI::isInteractive()) {
            return;
        }
        constexpr int kFrames = 5;
        for (int frame = 1; frame <= kFrames; ++frame) {
            const int column = positions[static_cast<std::size_t>(from)] +
                               (positions[static_cast<std::size_t>(to)] -
                                positions[static_cast<std::size_t>(from)]) *
                                   frame / kFrames;
            renderHub(from, column, "");
            UI::sleepFor(45);
        }
    };

    while (running) {
        renderHub(location, positions[static_cast<std::size_t>(location)], message);
        message.clear();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 4);
        switch (choice) {
            case 1:
                if (location == 0) {
                    message = "已经到街道最左边了。";
                } else {
                    animateMove(location, location - 1);
                    --location;
                    message = "你沿着街道向左走。";
                }
                break;
            case 2:
                if (location + 1 >= static_cast<int>(positions.size())) {
                    message = "已经到街道最右边了。";
                } else {
                    animateMove(location, location + 1);
                    ++location;
                    message = "你沿着街道向右走。";
                }
                break;
            case 3:
                if (location == 0) {
                    if (showCityGateDialogue()) {
                        running = false;
                    } else {
                        message = "你从城门回到灰烬街道。";
                    }
                } else if (location == 1) {
                    showShop(true);
                    message = "你从商店回到灰烬街道。";
                } else if (location == 2) {
                    showCampMenu();
                    message = "你从废墟营地回到灰烬街道。";
                } else if (location == 3) {
                    showTavern();
                    message = "你从酒馆回到灰烬街道。";
                } else {
                    showTowerGate();
                    message = "你回到灰烬街道。";
                }
                break;
            case 4:
                showJournal();
                message = "你合上见闻记录。";
                break;
            case 0:
            case -1:
                running = false;
                break;
            default:
                break;
        }

        if (running) {
            saveAfterAction();
        }
    }

    std::string saveError;
    std::string warning;
    if (!SaveGame::save(player_, saveError)) {
        warning = "自动存档失败：" + saveError;
    }
    renderFarewell(warning);
}

void Game::showTavern() {
    std::string message = "酒馆老板擦着杯子。‘火山灰把路都埋了，进来坐坐？’";

    while (true) {
        UI::clearScreen();
        Frame frame;
        frame.addColumns(
            UI::paint("地面枢纽 · 酒馆", UI::Color::BrightCyan),
            UI::paint("金币 " + std::to_string(player_.getGold()), UI::Color::Yellow),
            55);
        frame.addDivider();
        frame.addBlank();

        const std::vector<std::string>& keeper = Sprites::merchant();
        for (const std::string& line : keeper) {
            frame.addCentered(UI::paint(line, UI::Color::Yellow));
        }
        frame.addCentered(UI::paint("酒馆老板", UI::Color::Yellow));
        frame.addBlank();
        frame.addWrapped(message, "  ");
        frame.addBlank();
        frame.addDivider();
        frame.addLine(
            UI::paint("[1]", UI::Color::BrightYellow) + " 休息（5 金币）   " +
            UI::paint("[2]", UI::Color::BrightYellow) + " 打听消息   " +
            UI::paint("[3]", UI::Color::BrightYellow) + " 见闻   " +
            UI::paint("[0]", UI::Color::BrightYellow) + " 返回街道");
        frame.render();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 3);
        if (choice == 0 || choice == -1) {
            return;
        }
        if (choice == 1) {
            if (!player_.spendGold(5)) {
                message = "金币不够。老板说：‘先去找点活干吧。’";
            } else {
                player_.restoreAll();
                message = "你靠在墙角睡了一会儿。HP 与 MP 已完全恢复。";
            }
        } else if (choice == 2) {
            unlockJournal("tavern.ash");
            showStoryPage(
                "酒馆 · 火山灰",
                {
                    "老板说，地震撕开山体后，火山灰就再没真正散过。",
                    "居民外出必须戴面具，身份牌只负责登记，不再代替脸。",
                    "他还说，地下五层有一扇被钉死的门，只有少数人知道。"
                },
                UI::Color::BrightYellow,
                "  按回车继续 > ");
            message = "老板压低了声音，又擦起了杯子。";
        } else if (choice == 3) {
            showJournal();
            message = "你合上见闻记录。";
        }
    }
}

bool Game::showCityGateDialogue() {
    const std::vector<std::vector<std::string>> talks = {
        {
            "机械门卫：>「临时身份牌。记录里没有你的旧档案。」<",
            "「地下又震了。火山灰把外城的路埋了一半。」",
            "「要出城？现在外面不比城里安全。」"
        },
        {
            "「高塔的锁链是旧王都的，不是我们挂的。」",
            "「你那张临时身份牌，进城可以，出城不保证你还能回来。」",
            "「火山灰落进地缝，地下的东西最近醒得很勤。」"
        },
        {
            "「城门登记只认牌子，不认脸。」",
            "「外面的人越来越少，回来的人更少。」",
            "「要出城就按 2。别在门口站太久。」"
        }
    };
    std::size_t talkIndex = 0;

    while (true) {
        UI::clearScreen();
        Frame frame;
        frame.addColumns(
            UI::paint("地面枢纽 · 城门", UI::Color::BrightCyan),
            UI::paint("身份牌：临时", UI::Color::Gray),
            55);
        frame.addDivider();
        frame.addBlank();

        for (const std::string& line : Sprites::gatekeeper()) {
            frame.addCentered(UI::paint(line, UI::Color::Gray));
        }
        frame.addCentered(UI::paint("机械门卫", UI::Color::Yellow));
        frame.addBlank();

        const int textStart = frame.rowCount();
        for (const std::string& line : talks[talkIndex]) {
            frame.addWrapped(line, "  ");
        }
        std::vector<int> typedRows;
        for (int index = textStart; index < frame.rowCount(); ++index) {
            typedRows.push_back(index);
        }

        frame.addBlank();
        frame.addDivider();
        frame.addLine(
            UI::paint("[1]", UI::Color::BrightYellow) + " 交谈   " +
            UI::paint("[2]", UI::Color::BrightYellow) + " 出城   " +
            UI::paint("[0]", UI::Color::BrightYellow) + " 返回街道");
        frame.renderTypewriter(typedRows);

        // 选项同时写进输入提示：终端较矮、画框底部被顶出屏幕时也能看见。
        const int choice = UI::readChoice(
            UI::paint("  选择（1 交谈 / 2 出城 / 0 返回街道） > ",
                      UI::Color::BrightGreen),
            0, 2);
        if (choice == 0 || choice == -1) {
            return false;
        }
        if (choice == 2) {
            return true;
        }

        talkIndex = (talkIndex + 1) % talks.size();
    }
}

void Game::showTowerLookingUp() const {
    UI::clearScreen();

    Frame frame;
    const std::vector<std::string> art = {
        "x       ....     .:+:x...::::::................:..+x+::.+.:: :.. : .   . x ",
        "xx  . .. .:  :.  .x:+x....:::.::.....+.........x...xx::.:. .: ::.::. :    x",
        "xx.  :.. ......   ..::xx.:.:::.....+.x+x...........x::...+x .:..:: ..::   x",
        "xx     :..: ..   .....:+.........x+.+x++x:........:....+.. .:.+:+.: :...  x",
        "xx ...  :.:..: ..:+......++xx+:.x+ ++x++.x.xx+.+:   .+...+...+.++ +:+::.  x",
        "xx.  :.::.:: . ... :::: : .. :  ...+xxx+++ :x .::... ..::..:: +x++++..:+. x",
        "x#: . ...: ::::  .:::::+.:x++:... .+:x:.+:. .::x::+...:: :x ++:+:+ +++:+: x",
        "#x....: . .... ..:.::+.x++x++:.   :.xx ..   :x:xxx++::.:+x+:++.++.++ +:++ x",
        "xx .  ..: :::.  .. ..##x++::.. ..::::::::.:..+x+x+xx+..x.x.xxx::++++:...  x",
        "xx .   . .::::.  .  .x+++ :... :++xxx+:+::.++.:+x..x++:x+.xxx+++.x++x   + #",
        "xx     ..::..::::.: .+++++....++ ++++ xxx+x+:::+:#xx..++.:+x:xx.x.++.     #",
        "xx     :..:..: .:... .. . ...::+++++xx+xx+++++::::x++x.++:xxxxx+++x.:.    x",
        "xx    ..:..:::.:..:::. .+.. :+++++++xxxxx++++:::.:x.::xxxxxx:+::++.+.+:   x",
        "xx.  . .:::::.. ::  ::.. . :+::++.x+++x:+++++++::: .+xx::xx.+xxxxx+++++.. #",
        "xx .. .....: .:  :::..... ....:+++xxx+x++++.:+:+:.:++:+x+.+x: .++++++:+.+ #",
        "xx. ..:...:      ::.:. .. ..:+++:+++++++++++::+::: +x+:x+x: .:++.+:+.++:  #",
        "x# ..... :      . :.:: ......:::++++.++x:+++:+::++.+++x+x+ .++. ::::+ ++: #",
        "##. . . :    .. .:.:..  ..:...::+++.     +.+::++++: : ::+.: :+::+++:+:.:: #",
        "##     :     .... .:::. ..::.:::+.      ..+.:+++::+: ++++++: :++: :.: +:  #",
        "xx  . .       ....:... ....::.:::      ..:+:.::.:.:+.::+:..+.  :+:+xx+:.  #",
        "x#   :  .      . .. .    ..: :..:...   .:.++:.+::..+:.::: .:+. x:++++:   .#",
        "x#  . ..:.....  . .. ... ..:..:.: ..   :.++...::::.:..:....++.x:+.+:.   . x",
        "x#       ::.... ::..     .:.: .::  .  .  . :: x.. . ::++:x:xxxx+++: .     x",
        "xx       . .: ...:.:::... .:..: .:::::::+++ ++:+xxxxxx:+:+xx+++++. :.   . x",
        "xx   .   . ..:::::++::+++:+++++:++x:xxxx ++xxxxx++++:+++++++++ . ..  .    x"
    };
    const std::vector<std::string> canvas = stretchArtToCanvas(art, 99, 28);
    for (int row = 0; row < static_cast<int>(canvas.size()); ++row) {
        const std::string& line = canvas[static_cast<std::size_t>(row)];
        std::string painted;
        painted.reserve(line.size() * 24);
        for (int column = 0; column < static_cast<int>(line.size()); ++column) {
            const char character = line[static_cast<std::size_t>(column)];
            if (character == ' ') {
                painted += ' ';
                continue;
            }

            int red = 120;
            int green = 110;
            int blue = 100;
            if (character == '.') {
                red = 205; green = 190; blue = 160;
            } else if (character == ':') {
                red = 165; green = 145; blue = 115;
            } else if (character == '+') {
                red = 115; green = 88; blue = 62;
            } else if (character == 'x') {
                red = 65; green = 48; blue = 34;
            } else if (character == '#') {
                red = 28; green = 20; blue = 16;
            }

            // 参考图左上方有一弯月亮；在塔顶左侧点一小片暖黄。
            const double moonX = (column - 14) / 5.0;
            const double moonY = (row - 2) / 3.5;
            if (character != 'x' && character != '#' &&
                moonX * moonX + moonY * moonY <= 1.0) {
                red = 242; green = 226; blue = 170;
            }

            painted += UI::paintRgb(std::string(1, character), red, green, blue);
        }
        frame.addLine(painted);
    }
    frame.render();

    UI::waitForEnter(UI::paint("  按回车继续 > ", UI::Color::BrightGreen));
}

void Game::showTowerGate() {
    unlockJournal("tower.gate");
    showTowerLookingUp();

    // 纯 CG 之后单独播放说明页，CG 本身不再塞说明文字。
    showStoryPage(
        "高塔 · 入口",
        {
            "你站在塔底抬头望去，塔身消失在灰烬里。",
            "入口被灰烬和锁链封住，暂时上不去。",
            "塔顶偶尔有金属反光。这里以后会通向征服骑士。"
        },
        UI::Color::Gray,
        "  按回车返回街道 > ");
}

void Game::showCampMenu() {
    std::string message;
    if (storyStage_ >= kStoryClueCount + 1) {
        message = endingCampLine(storyEnding_);
    } else if (storyStage_ == kStoryClueCount) {
        message = "核心的门已经打开。最深处传来缓慢的脉动。";
    } else {
        message = "你在废墟营地的火堆旁坐下。";
    }

    bool running = true;
    while (running) {
        renderCamp(message);
        message.clear();

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 7);
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
                showJournal();
                break;
            case 6:
                message = explore();
                refreshCampShop();
                break;
            case 7:
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
    SaveGame::save(player_, saveError);
}

void Game::showShop(bool owner) {
    showShopScreen(true, owner);
}

void Game::showTrader() {
    showShopScreen(false, false);
}

void Game::refreshCampShop() {
    shopAvailable_ = Random::chance(0.5);
}

void Game::showShopScreen(bool full, bool owner) {
    std::unique_ptr<Shop> shop;
    if (full) {
        shop = std::make_unique<Shop>();
    } else {
        shop = std::make_unique<Shop>(Random::integer(3, 5),
                                      Random::real(0.75, 1.4));
    }

    std::string message = full ? (owner ? "看看货吧。我算半个考古爱好者，有古物也可以拿给我看看。"
                                        : "看看货吧，都是遗迹里少见的好东西。")
                               : "就这些了。价格嘛，看你顺不顺眼。";

    while (true) {
        renderShop(*shop, message, full, owner);

        const int maxChoice = (full && owner) ? 4 : 3;
        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 0, maxChoice);
        switch (choice) {
            case 1:
                message = buyFromShop(*shop, full && owner && hasStoryFlag("shop.discount"));
                break;
            case 2:
                message = sellToShop(owner);
                break;
            case 3:
                showInventoryScreen();
                message = full ? "还要买点什么吗？" : "还要换点什么吗？";
                break;
            case 4:
                message = tradeRelicWithOwner();
                break;
            case 0:
            case -1:
                return;
            default:
                break;
        }
    }
}

void Game::renderShop(const Shop& shop, const std::string& message,
                      bool full, bool owner) const {
    UI::clearScreen();

    Frame frame;
    frame.setBottomPadding(4);
    frame.addColumns(
        UI::paint(owner ? "地面商店"
                        : (full ? "营地 · 游商" : "营地 · 拾荒者"),
                  UI::Color::BrightCyan),
        UI::paint("金币 " + std::to_string(player_.getGold()) +
                  "   背包 " + std::to_string(player_.getInventory().size()) + "/" +
                  std::to_string(player_.getInventory().capacity()),
                  UI::Color::Yellow),
        45);
    frame.addDivider();
    frame.addBlank();

    const std::vector<std::string>& art =
        full ? Sprites::merchant() : Sprites::survivor();
    const std::string traderLabel = owner ? "商店老板"
                                         : (full ? "游商" : "拾荒者");
    const bool discount = full && owner && hasStoryFlag("shop.discount");
    constexpr int kMerchantWidth = 33;
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
            right = shopItemLine(row + 1, shop[row], discount);
        }
        frame.addSplit(left, right, kMerchantWidth);
    }

    frame.addBlank();
    const int messageStart = frame.rowCount();
    frame.addWrapped(traderLabel + "：" + message, "  ");
    std::vector<int> typedRows;
    for (int index = messageStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.addDivider();
    std::string menu =
        UI::paint("[1]", UI::Color::BrightYellow) + " 购买   " +
        UI::paint("[2]", UI::Color::BrightYellow) + " 出售   " +
        UI::paint("[3]", UI::Color::BrightYellow) + " 查看背包";
    if (full && owner) {
        menu += "   " + UI::paint("[4]", UI::Color::BrightYellow) + " 古物";
    }
    menu += "   " + UI::paint("[0]", UI::Color::BrightYellow) + " 离开";
    frame.addLine(menu);
    frame.renderTypewriter(typedRows);
}

std::string Game::buyFromShop(Shop& shop, bool discount) {
    const int index = UI::readChoice("  商品编号（0 取消） > ", 0, shop.size());
    if (index == 0 || index == -1) {
        return "慢慢挑。";
    }

    Item& item = shop[index - 1];
    if (player_.getInventory().full()) {
        return "你的背包满了，先整理一下吧。";
    }

    int price = item.getPrice();
    if (discount) {
        price = std::max(1, price * 9 / 10);
    }
    if (!player_.getWallet().canAfford(price)) {
        return "金币不够，先去遗迹里转转吧。";
    }

    const std::string name = item.getName();
    std::unique_ptr<Item> copy = item.clone();

    player_.getWallet() -= price;
    player_ + std::move(copy);

    return "成交，" + name + " 归你了。";
}

std::string Game::sellToShop(bool owner) {
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

    if (item->getKind() == ItemKind::Relic) {
        if (!owner) {
            return "这个我不收。你拿去地面商店给老板看看，他懂古物。";
        }
        return "这块石板不按普通货收。点 [4] 古物，让老板亲自看看。";
    }

    const std::string name = item->getName();
    const int price = item->getSellPrice();

    inventory.takeItem(index - 1);
    player_.getWallet() += price;

    return "这件" + name + "，我出 " + std::to_string(price) + " 金币。";
}

std::string Game::tradeRelicWithOwner() {
    if (hasStoryFlag("relic.interpreted")) {
        return "老板已经把石板收进柜台下面的木匣里了。";
    }
    if (!hasRelic()) {
        return "老板看了看你的背包：‘古物呢？没有就别逗我。’";
    }

    std::string relicName;
    if (!takeRelic(relicName)) {
        return "老板皱了皱眉：‘这块石板有点不对劲。’";
    }

    player_.gainGold(40);
    setStoryFlag("relic.interpreted");
    setStoryFlag("shop.discount");
    unlockJournal("relic.slate");

    showStoryPage(
        "地面商店 · 古文字解读",
        {
            "老板放下账本，把石板翻了过来。",
            "‘这不是普通石头。上面是古国文字，讲的是地下五层的一条旧矿井路线。’",
            "‘还有一段被钉死的休息室。有人在那里藏过东西，也死过。’",
            "他抬头看了你一眼：‘我年轻时挖过几年遗址。这块石板，我收了。’",
            "老板给了你 40 金币。以后在他的店里买东西，算九折。"
        },
        UI::Color::BrightCyan,
        "  按回车继续 > ");

    return "老板收下了" + relicName + "，给了你 40 金币。以后在这里买东西打九折。";
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
                } else if (item->getKind() == ItemKind::Relic) {
                    message = "这是重要遗物，不能直接使用。拿去地面商店给老板看看。";
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
        45);
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
    const int textStart = frame.rowCount();
    frame.addWrapped(monster.getEncounterText(), "  ");
    std::vector<int> typedRows;
    for (int index = textStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.renderTypewriter(typedRows);

    UI::waitForEnter(UI::paint("  按回车迎战 > ", UI::Color::BrightRed));
}

std::unique_ptr<Monster> Game::createRandomMonster(int floor) const {
    std::unique_ptr<Monster> monster;
    const int roll = Random::integer(0, 99);
    if (roll < 40) {
        monster = std::make_unique<RuinsRat>();
    } else if (roll < 75) {
        const SlimeElement element = Random::chance(0.5)
                                          ? SlimeElement::Water
                                          : SlimeElement::Fire;
        monster = std::make_unique<Slime>(element);
    } else {
        monster = std::make_unique<Goblin>();
    }
    monster->scaleForFloor(floor);
    return monster;
}

std::unique_ptr<Monster> Game::createGuardian() const {
    return std::make_unique<Guardian>(player_.getLevel());
}

std::unique_ptr<Monster> Game::createMimic() const {
    return std::make_unique<Mimic>(player_.getLevel());
}

void Game::grantChestLoot(std::vector<std::string>& lines) {
    const int roll = Random::integer(0, 99);

    if (!hasRelic() && !hasStoryFlag("relic.interpreted") &&
        player_.getInventory().canAdd() && roll < 8) {
        player_ + std::make_unique<Relic>("刻纹石板", 80);
        lines.push_back("宝箱底层压着一块刻满古国文字的" + std::string("刻纹石板") + "。");
        return;
    }

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

std::string Game::showRewardRoom(bool& defeated) {
    defeated = false;
    UI::clearScreen();

    Frame frame;
    frame.addColumns(
        UI::paint("奖励房间", UI::Color::BrightCyan),
        UI::paint("宝箱", UI::Color::Yellow),
        65);
    frame.addDivider();
    frame.addBlank();

    for (const std::string& line : Sprites::chest()) {
        frame.addCentered(UI::paint(line, UI::Color::Yellow));
    }

    frame.addBlank();
    const int messageStart = frame.rowCount();
    frame.addWrapped("房间中央放着一只旧木箱。锁扣上没有灰，像刚被人擦过。", "  ");
    frame.addWrapped("箱盖轻轻动了一下。也可能只是风。", "  ");
    std::vector<int> typedRows;
    for (int index = messageStart; index < frame.rowCount(); ++index) {
        typedRows.push_back(index);
    }
    frame.addBlank();
    frame.addDivider();
    frame.addLine(
        UI::paint("[1]", UI::Color::BrightYellow) + " 打开宝箱   " +
        UI::paint("[0]", UI::Color::BrightYellow) + " 离开");
    frame.renderTypewriter(typedRows);

    const int choice = UI::readChoice(
        UI::paint("  选择 > ", UI::Color::BrightGreen), 0, 1);
    if (choice == 0 || choice == -1) {
        return "你没有打开宝箱，退回通道。";
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
        defeated = true;
        return handleDefeat(mimic->getName());
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
    frame.renderTypewriter();

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
    const int runStage = storyStage_;
    showAreaIntro(runStage);

    const bool bossRun = (runStage == kStoryClueCount - 1);
    bool bossDefeated = false;
    bool clueFound = false;
    int floor = 1;
    int depth = 0;

    const auto grantVictoryRewards = [&](Monster& monster) {
        const int levelsGained = player_.gainExperience(monster.getExpReward());
        player_.gainGold(monster.getGoldReward());

        std::vector<std::string> lines;
        lines.push_back("你击败了" + monster.getName() + "。");
        lines.push_back("获得 " + std::to_string(monster.getGoldReward()) + " 金币、" +
                        std::to_string(monster.getExpReward()) + " 经验。");

        std::unique_ptr<Item> drop = monster.rollDrop();
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
    };

    enum class PendingEvent {
        None,
        Monster,
        Door,
        Trap,
        CaveIn,
        Corpse,
        Quiet,
        Boss,
        AncestorRoom
    };

    struct Pose {
        int playerScreen;
        int camera;
    };
    static const Pose poses[5] = {
        {4, 0},
        {30, 0},
        {48, 0},
        {48, 12},
        {48, 24}
    };

    PendingEvent pending = PendingEvent::None;
    std::unique_ptr<Monster> pendingMonster;
    const std::vector<std::string>* previewArt = nullptr;
    UI::Color previewColor = UI::Color::Gray;
    int step = 0;
    int eventsResolved = 0;

    const auto rollNextEvent = [&]() {
        step = 0;
        pendingMonster.reset();
        previewArt = nullptr;
        previewColor = UI::Color::Gray;

        if (bossRun && !bossDefeated && eventsResolved >= 2) {
            pending = PendingEvent::Boss;
            previewArt = &Sprites::guardian();
            previewColor = UI::Color::BrightMagenta;
            return;
        }

        if (floor == 5 && !hasStoryFlag("ancestor.read") && Random::chance(0.35)) {
            pending = PendingEvent::AncestorRoom;
            previewArt = &Sprites::ancestor();
            previewColor = UI::Color::Gray;
            return;
        }

        const int monsterChance = 25;
        const int doorChance = 15 + std::min(depth, 6) * 2;
        const int trapChance = 12;
        const int caveChance = 8;
        const int corpseChance = 10;
        const int roll = Random::integer(0, 99);

        if (roll < monsterChance) {
            pending = PendingEvent::Monster;
            pendingMonster = createRandomMonster(floor);
            previewArt = &pendingMonster->getArt();
            previewColor = MonsterVisual::tint(pendingMonster->getColor());
            return;
        }

        int threshold = monsterChance + doorChance;
        if (roll < threshold) {
            pending = PendingEvent::Door;
            previewArt = &Sprites::door();
            previewColor = UI::Color::Yellow;
            return;
        }

        threshold += trapChance;
        if (roll < threshold) {
            pending = PendingEvent::Trap;
            return;
        }

        threshold += caveChance;
        if (roll < threshold) {
            pending = PendingEvent::CaveIn;
            return;
        }

        threshold += corpseChance;
        if (roll < threshold) {
            pending = PendingEvent::Corpse;
            previewArt = &Sprites::corpse();
            previewColor = UI::Color::Gray;
            return;
        }

        pending = PendingEvent::Quiet;
    };

    const auto animateToStep = [&](int from, int to) {
        if (!UI::isInteractive()) {
            renderCorridor(floor, depth, runStage, poses[to].playerScreen,
                           poses[to].camera, previewArt, previewColor);
            return;
        }

        constexpr int kFrames = 5;
        for (int frame = 1; frame <= kFrames; ++frame) {
            const int playerScreen =
                poses[from].playerScreen +
                (poses[to].playerScreen - poses[from].playerScreen) * frame / kFrames;
            const int camera =
                poses[from].camera +
                (poses[to].camera - poses[from].camera) * frame / kFrames;
            renderCorridor(floor, depth, runStage, playerScreen, camera,
                           previewArt, previewColor);
            UI::sleepFor(45);
        }
        renderCorridor(floor, depth, runStage, poses[to].playerScreen,
                       poses[to].camera, previewArt, previewColor);
    };

    rollNextEvent();

    while (true) {
        renderCorridor(floor, depth, runStage, poses[step].playerScreen,
                       poses[step].camera, previewArt, previewColor);

        const int choice = UI::readChoice(
            UI::paint("  选择 > ", UI::Color::BrightGreen), 1, 4);
        if (choice == 2) {
            showStatusScreen();
            continue;
        }
        if (choice == 3) {
            showInventoryScreen();
            continue;
        }
        if (choice == 4 || choice == -1) {
            showReturnCutscene();
            return "你沿着通道退回营地。";
        }

        ++depth;
        if (step < 4) {
            animateToStep(step, step + 1);
            ++step;
            if (Random::chance(0.08)) {
                triggerTravelDisaster();
            }
            if (step < 4) {
                continue;
            }
        }

        switch (pending) {
            case PendingEvent::Monster: {
                if (!pendingMonster) {
                    break;
                }
                showEncounterCutscene(*pendingMonster);

                Battle battle(player_, storyStage_);
                const BattleResult result = battle.fight(*pendingMonster);
                if (result == BattleResult::Victory) {
                    grantVictoryRewards(*pendingMonster);
                    if (!clueFound && !bossRun) {
                        showStoryBeat();
                        clueFound = true;
                    }
                    break;
                }
                if (result == BattleResult::Defeat) {
                    return handleDefeat(pendingMonster->getName());
                }

                showReturnCutscene();
                return "你退回通道，决定先回营地。";
            }

            case PendingEvent::Boss: {
                showStoryPage(
                    "核心门前",
                    {
                        "通道在尽头收窄，一扇没有把手的门嵌在岩壁里。",
                        "锁链已经松开，门后传来沉重而缓慢的呼吸声。",
                        "封存骑士从黑暗里驶出。它仍守着旧王朝的最后一道命令。"
                    },
                    UI::Color::BrightMagenta,
                    "  按回车迎战 > ");

                std::unique_ptr<Monster> boss = createGuardian();
                showEncounterCutscene(*boss);

                Battle battle(player_, storyStage_);
                const BattleResult result = battle.fight(*boss);
                if (result == BattleResult::Victory) {
                    grantVictoryRewards(*boss);
                    bossDefeated = true;
                    showStoryPage(
                        "守护者 · 封存解除",
                        {
                            "封存骑士的甲胄从胸口裂开。里面没有身体，只有一枚停转的齿轮。",
                            "它最后重复了一次旧命令：‘封存……完成。’",
                            "挡在门前的锁链松开了一道。门上的识别装置随即亮起。"
                        },
                        UI::Color::BrightMagenta,
                        "  按回车继续 > ");
                    if (!clueFound) {
                        showStoryBeat();
                        clueFound = true;
                    }
                    break;
                }
                if (result == BattleResult::Defeat) {
                    return handleDefeat(boss->getName());
                }

                showReturnCutscene();
                return "你退回通道。封存骑士仍守在核心门前。";
            }

            case PendingEvent::Door: {
                std::string doorMessage;
                if (!handleCorridorDoor(floor, doorMessage)) {
                    return doorMessage;
                }
                break;
            }

            case PendingEvent::Trap:
                handleCorridorTrap(floor);
                break;

            case PendingEvent::CaveIn: {
                int nextFloor = floor;
                handleCorridorCaveIn(floor, nextFloor);
                floor = nextFloor;
                break;
            }

            case PendingEvent::Corpse:
                handleCorridorCorpse();
                break;

            case PendingEvent::AncestorRoom:
                showAncestorRoom();
                break;

            case PendingEvent::Quiet: {
                static const std::vector<std::string> quietLines = {
                    "你继续往前走。火把的光只够照亮脚下几步。",
                    "通道里只有水滴声。你听了一会儿，确定那不是脚步。",
                    "墙上有一道旧裂缝，形状像某种被遗忘的文字。",
                    "你停下来辨认方向。回声从更深的地方传回来。"
                };
                showStoryPage(
                    "通道",
                    {quietLines[static_cast<std::size_t>(
                        Random::integer(0, static_cast<int>(quietLines.size()) - 1))]},
                    UI::Color::Gray,
                    "  按回车继续 > ");
                break;
            }

            case PendingEvent::None:
                break;
        }

        ++eventsResolved;

        rollNextEvent();
    }
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
    frame.renderTypewriter();
}
