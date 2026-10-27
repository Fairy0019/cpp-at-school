#pragma once

#include <string>
#include <vector>

// 控制台界面工具：UTF-8 / ANSI 颜色、文本宽度、进度条与输入。
namespace UI {

enum class Color {
    Default,
    White,
    Gray,
    Red,
    BrightRed,
    Green,
    BrightGreen,
    Yellow,
    BrightYellow,
    Blue,
    BrightBlue,
    Magenta,
    BrightMagenta,
    Cyan,
    BrightCyan
};

// 初始化控制台：UTF-8 编码，并在支持时开启 ANSI 颜色。
void setupConsole();

// 清屏；输出被重定向时只换行，避免把日志清掉。
void clearScreen();

// 当前输出是否连接到真实控制台（用于播放走路动画）。
bool isInteractive();

// 仅在交互式控制台暂停指定毫秒。
void sleepFor(int milliseconds);

// 给文本着色。颜色未启用时原样返回。
std::string paint(const std::string& text, Color color, bool bold = false);

// 生成进度条，例如 ████████░░░░░░░░。
std::string bar(int value, int maxValue, int width, Color color);

// 根据当前生命值比例选择绿 / 黄 / 红。
Color healthColor(int value, int maxValue);

// 计算终端显示宽度：中文等全角字符按 2 列计，ANSI 转义序列按 0 列计。
int displayWidth(const std::string& text);

// 截断或补空格到指定显示宽度。
std::string fit(const std::string& text, int width);

// 右补空格到指定显示宽度。
std::string padRight(const std::string& text, int width);

// 在指定显示宽度内居中。
std::string center(const std::string& text, int width);

// 按显示宽度自动换行。
std::vector<std::string> wrap(const std::string& text, int width);

// 读取一个位于 [minChoice, maxChoice] 的整数选项。
// 输入非法时提示并重新输入；遇到输入流结束（EOF）返回 -1。
int readChoice(const std::string& prompt, int minChoice, int maxChoice);

// 等待用户按回车（输入流结束时不阻塞）。
void waitForEnter(const std::string& prompt);

}  // namespace UI
