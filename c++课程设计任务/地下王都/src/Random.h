#pragma once

// 随机数工具：统一使用 <random> 的 mt19937 引擎，避免 rand() 的低质量序列。
namespace Random {

// 返回 [minValue, maxValue] 区间内的随机整数（包含两端）。
int integer(int minValue, int maxValue);

// 返回 [minValue, maxValue] 区间内的随机实数。
double real(double minValue, double maxValue);

// 以 probability 的概率返回 true，probability 取值 0.0 ~ 1.0。
bool chance(double probability);

}  // namespace Random
