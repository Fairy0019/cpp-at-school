#pragma once

#include <string>

enum class DisasterType {
    Snow,
    Earthquake,
    Fire
};

struct DisasterEvent {
    DisasterType type;
    std::string name;
    std::string description;
    int hpDamage;
    int mpDamage;
};

// 三灾是地心炉失稳的外在表现，没有固定周期。
namespace Disaster {

DisasterEvent generate(int storyStage);

}  // namespace Disaster
