#include "Disaster.h"

#include "Random.h"

#include <algorithm>

namespace {

int stageValue(int storyStage) {
    return std::max(0, std::min(storyStage, 8));
}

}  // namespace

namespace Disaster {

DisasterEvent generate(int storyStage) {
    const int stage = stageValue(storyStage);
    const int roll = Random::integer(0, 2);

    DisasterEvent event;
    if (roll == 0) {
        event.type = DisasterType::Snow;
        event.name = "雪灾";
        event.description = "反季节的暴雪压下来，寒气顺着石缝灌进肺里。";
        event.hpDamage = 2 + stage / 3;
        event.mpDamage = 0;
    } else if (roll == 1) {
        event.type = DisasterType::Earthquake;
        event.name = "地震";
        event.description = "地面猛地倾斜，碎石和尘土从头顶落下。";
        event.hpDamage = 3 + stage / 3;
        event.mpDamage = 0;
    } else {
        event.type = DisasterType::Fire;
        event.name = "火灾";
        event.description = "地底的热浪冲破裂缝，火山灰和焦灼的气流卷过通道。";
        event.hpDamage = 2 + stage / 3;
        event.mpDamage = 1 + (stage >= 6 ? 1 : 0);
    }
    return event;
}

}  // namespace Disaster
