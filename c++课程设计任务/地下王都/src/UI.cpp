#include "UI.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

namespace {

bool g_ansiEnabled = false;
bool g_consoleAttached = false;

#ifdef _WIN32
COORD g_savedCursor = {0, 0};
bool g_hasSavedCursor = false;
#endif

// 返回从 index 开始的 ANSI 转义序列结束位置（不含）。
std::size_t ansiEnd(const std::string& text, std::size_t index) {
    if (index + 1 >= text.size() || text[index] != '\x1b' || text[index + 1] != '[') {
        return index;
    }
    std::size_t cursor = index + 2;
    while (cursor < text.size()) {
        const unsigned char character = static_cast<unsigned char>(text[cursor]);
        if (character >= 0x40 && character <= 0x7E) {
            return cursor + 1;
        }
        ++cursor;
    }
    return text.size();
}

// 解码一个 UTF-8 字符，index 前进到下一个字符。
char32_t decodeUtf8(const std::string& text, std::size_t& index) {
    const unsigned char first = static_cast<unsigned char>(text[index]);
    if (first < 0x80) {
        ++index;
        return first;
    }

    int extraBytes = 0;
    char32_t codePoint = 0;
    if ((first & 0xE0) == 0xC0) {
        extraBytes = 1;
        codePoint = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        extraBytes = 2;
        codePoint = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        extraBytes = 3;
        codePoint = first & 0x07;
    } else {
        ++index;
        return 0xFFFD;
    }

    if (index + static_cast<std::size_t>(extraBytes) >= text.size()) {
        ++index;
        return 0xFFFD;
    }

    for (int offset = 1; offset <= extraBytes; ++offset) {
        const unsigned char next = static_cast<unsigned char>(text[index + offset]);
        if ((next & 0xC0) != 0x80) {
            ++index;
            return 0xFFFD;
        }
        codePoint = (codePoint << 6) | (next & 0x3F);
    }

    index += static_cast<std::size_t>(extraBytes) + 1;
    return codePoint;
}

// 常见东亚全角字符按 2 列显示。盒线、方块元素按 1 列显示。
int codePointWidth(char32_t codePoint) {
    if (codePoint == 0) {
        return 0;
    }
    if (codePoint < 32 || (codePoint >= 0x7F && codePoint < 0xA0)) {
        return 0;
    }
    if ((codePoint >= 0x0300 && codePoint <= 0x036F) ||
        (codePoint >= 0x1AB0 && codePoint <= 0x1AFF) ||
        (codePoint >= 0x1DC0 && codePoint <= 0x1DFF) ||
        (codePoint >= 0x20D0 && codePoint <= 0x20FF) ||
        (codePoint >= 0xFE20 && codePoint <= 0xFE2F)) {
        return 0;  // 组合附加符号
    }

    const bool wide =
        (codePoint >= 0x1100 && codePoint <= 0x115F) ||
        codePoint == 0x2329 || codePoint == 0x232A ||
        (codePoint >= 0x2E80 && codePoint <= 0xA4CF && codePoint != 0x303F) ||
        (codePoint >= 0xAC00 && codePoint <= 0xD7A3) ||
        (codePoint >= 0xF900 && codePoint <= 0xFAFF) ||
        (codePoint >= 0xFE10 && codePoint <= 0xFE19) ||
        (codePoint >= 0xFE30 && codePoint <= 0xFE6F) ||
        (codePoint >= 0xFF00 && codePoint <= 0xFF60) ||
        (codePoint >= 0xFFE0 && codePoint <= 0xFFE6) ||
        (codePoint >= 0x1F300 && codePoint <= 0x1FAFF) ||
        (codePoint >= 0x20000 && codePoint <= 0x3FFFD);

    return wide ? 2 : 1;
}

bool isBlank(const std::string& text) {
    for (char character : text) {
        if (!std::isspace(static_cast<unsigned char>(character))) {
            return false;
        }
    }
    return true;
}

}  // namespace

namespace UI {

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode)) {
        g_consoleAttached = true;
        if (SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            g_ansiEnabled = true;
        }

        // 放大画框后需要更高的窗口；失败时保持原尺寸即可。
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(output, &info)) {
            COORD size = info.dwSize;
            if (size.X < 120) {
                size.X = 120;
            }
            if (size.Y < 44) {
                size.Y = 44;
            }
            SetConsoleScreenBufferSize(output, size);

            SMALL_RECT window;
            window.Left = 0;
            window.Top = 0;
            window.Right = static_cast<SHORT>(std::min<int>(size.X - 1, 119));
            window.Bottom = static_cast<SHORT>(std::min<int>(size.Y - 1, 43));
            SetConsoleWindowInfo(output, TRUE, &window);
        }
    }
#endif
}

void clearScreen() {
    if (g_ansiEnabled) {
        std::cout << "\x1b[2J\x1b[H";
    } else if (g_consoleAttached) {
        std::system("cls");
    } else {
        std::cout << "\n";
    }
    std::cout.flush();
}

bool isInteractive() {
    return g_consoleAttached;
}

bool ansiEnabled() {
    return g_ansiEnabled;
}

void saveCursor() {
#ifdef _WIN32
    if (g_consoleAttached) {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (output != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(output, &info)) {
            g_savedCursor = info.dwCursorPosition;
            g_hasSavedCursor = true;
        }
        std::cout.flush();
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[s";
        std::cout.flush();
    }
}

void restoreCursor() {
#ifdef _WIN32
    if (g_consoleAttached) {
        if (g_hasSavedCursor) {
            HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
            SetConsoleCursorPosition(output, g_savedCursor);
        }
        std::cout.flush();
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[u";
        std::cout.flush();
    }
}

void moveCursorUp(int lines) {
    if (lines <= 0) {
        return;
    }
#ifdef _WIN32
    if (g_consoleAttached) {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (output != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(output, &info)) {
            COORD position = info.dwCursorPosition;
            position.Y = static_cast<SHORT>(
                std::max<int>(0, position.Y - lines));
            SetConsoleCursorPosition(output, position);
        }
        std::cout.flush();
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[" << lines << "A";
        std::cout.flush();
    }
}

void moveCursorRight(int columns) {
    if (columns <= 0) {
        return;
    }
#ifdef _WIN32
    if (g_consoleAttached) {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (output != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(output, &info)) {
            COORD position = info.dwCursorPosition;
            position.X = static_cast<SHORT>(
                std::min<int>(info.dwSize.X - 1, position.X + columns));
            SetConsoleCursorPosition(output, position);
        }
        std::cout.flush();
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[" << columns << "C";
        std::cout.flush();
    }
}

void hideCursor() {
#ifdef _WIN32
    if (g_consoleAttached) {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        if (output != INVALID_HANDLE_VALUE &&
            GetConsoleCursorInfo(output, &info)) {
            info.bVisible = FALSE;
            SetConsoleCursorInfo(output, &info);
        }
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[?25l";
        std::cout.flush();
    }
}

void showCursor() {
#ifdef _WIN32
    if (g_consoleAttached) {
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        if (output != INVALID_HANDLE_VALUE &&
            GetConsoleCursorInfo(output, &info)) {
            info.bVisible = TRUE;
            SetConsoleCursorInfo(output, &info);
        }
        return;
    }
#endif
    if (g_ansiEnabled) {
        std::cout << "\x1b[?25h";
        std::cout.flush();
    }
}

void sleepFor(int milliseconds) {
    if (!g_consoleAttached || milliseconds <= 0) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::string paint(const std::string& text, Color color, bool bold) {
    if (!g_ansiEnabled || text.empty()) {
        return text;
    }

    std::string prefix;
    if (bold) {
        prefix += "\x1b[1m";
    }

    switch (color) {
        case Color::White:         prefix += "\x1b[37m"; break;
        case Color::Gray:          prefix += "\x1b[90m"; break;
        case Color::Red:           prefix += "\x1b[31m"; break;
        case Color::BrightRed:     prefix += "\x1b[91m"; break;
        case Color::Green:         prefix += "\x1b[32m"; break;
        case Color::BrightGreen:   prefix += "\x1b[92m"; break;
        case Color::Yellow:        prefix += "\x1b[33m"; break;
        case Color::BrightYellow:  prefix += "\x1b[93m"; break;
        case Color::Blue:          prefix += "\x1b[34m"; break;
        case Color::BrightBlue:    prefix += "\x1b[94m"; break;
        case Color::Magenta:       prefix += "\x1b[35m"; break;
        case Color::BrightMagenta: prefix += "\x1b[95m"; break;
        case Color::Cyan:          prefix += "\x1b[36m"; break;
        case Color::BrightCyan:    prefix += "\x1b[96m"; break;
        case Color::Default:       break;
    }

    if (prefix.empty()) {
        return text;
    }
    return prefix + text + "\x1b[0m";
}

std::string paintRgb(const std::string& text, int red, int green, int blue,
                     bool background) {
    if (!g_ansiEnabled || text.empty()) {
        return text;
    }

    red = std::max(0, std::min(255, red));
    green = std::max(0, std::min(255, green));
    blue = std::max(0, std::min(255, blue));

    std::string prefix = "\x1b[";
    prefix += background ? "48" : "38";
    prefix += ";2;";
    prefix += std::to_string(red);
    prefix += ";";
    prefix += std::to_string(green);
    prefix += ";";
    prefix += std::to_string(blue);
    prefix += "m";
    return prefix + text + "\x1b[0m";
}

void typeText(const std::string& text, int delayMs) {
    if (!g_consoleAttached || delayMs <= 0) {
        std::cout << text;
        std::cout.flush();
        return;
    }

    std::size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\x1b') {
            const std::size_t end = ansiEnd(text, index);
            if (end > index) {
                std::cout.write(text.data() + index,
                                static_cast<std::streamsize>(end - index));
                index = end;
                continue;
            }
        }

        if (text[index] == '\n') {
            std::cout.put('\n');
            std::cout.flush();
            ++index;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs * 2));
            continue;
        }

        const std::size_t start = index;
        const char32_t codePoint = decodeUtf8(text, index);
        std::cout.write(text.data() + start,
                        static_cast<std::streamsize>(index - start));
        std::cout.flush();

        int pause = delayMs;
        if (codePoint == ' ') {
            pause = std::max(1, delayMs / 3);
        } else if (codePoint == U'。' || codePoint == U'！' ||
                   codePoint == U'？' || codePoint == U'…') {
            pause = delayMs * 3;
        } else if (codePoint == U'，' || codePoint == U'、' ||
                   codePoint == U'；' || codePoint == U'：') {
            pause = delayMs * 2;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pause));
    }
    std::cout.flush();
}

std::string bar(int value, int maxValue, int width, Color color) {
    if (width <= 0) {
        return std::string();
    }

    int filled = 0;
    if (maxValue > 0) {
        filled = (std::max(0, std::min(value, maxValue)) * width + maxValue / 2) / maxValue;
    }

    std::string result;
    result.reserve(static_cast<std::size_t>(width) * 3);
    for (int index = 0; index < filled; ++index) {
        result += "█";
    }
    for (int index = filled; index < width; ++index) {
        result += "░";
    }
    return paint(result, color);
}

Color healthColor(int value, int maxValue) {
    if (maxValue <= 0) {
        return Color::Red;
    }
    const double ratio = static_cast<double>(value) / static_cast<double>(maxValue);
    if (ratio > 0.5) {
        return Color::BrightGreen;
    }
    if (ratio > 0.25) {
        return Color::BrightYellow;
    }
    return Color::BrightRed;
}

int displayWidth(const std::string& text) {
    int width = 0;
    std::size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\x1b') {
            const std::size_t end = ansiEnd(text, index);
            if (end > index) {
                index = end;
                continue;
            }
        }
        width += codePointWidth(decodeUtf8(text, index));
    }
    return width;
}

std::string fit(const std::string& text, int width) {
    if (width <= 0) {
        return std::string();
    }

    std::string result;
    result.reserve(text.size() + static_cast<std::size_t>(width));
    int usedWidth = 0;
    bool truncated = false;
    std::size_t index = 0;

    while (index < text.size()) {
        if (text[index] == '\x1b') {
            const std::size_t end = ansiEnd(text, index);
            if (end > index) {
                result.append(text, index, end - index);
                index = end;
                continue;
            }
        }

        const std::size_t start = index;
        const char32_t codePoint = decodeUtf8(text, index);
        const int characterWidth = codePointWidth(codePoint);
        if (usedWidth + characterWidth > width) {
            truncated = true;
            break;
        }
        result.append(text, start, index - start);
        usedWidth += characterWidth;
    }

    if (truncated && g_ansiEnabled) {
        result += "\x1b[0m";
    }
    if (usedWidth < width) {
        result.append(static_cast<std::size_t>(width - usedWidth), ' ');
    }
    return result;
}

std::string padRight(const std::string& text, int width) {
    return fit(text, width);
}

std::string center(const std::string& text, int width) {
    const int textWidth = displayWidth(text);
    if (textWidth >= width) {
        return fit(text, width);
    }
    const int leftPadding = (width - textWidth) / 2;
    return std::string(static_cast<std::size_t>(leftPadding), ' ') + text;
}

std::vector<std::string> wrap(const std::string& text, int width) {
    std::vector<std::string> lines;
    if (width <= 0) {
        lines.push_back(text);
        return lines;
    }

    std::string current;
    int usedWidth = 0;
    std::size_t index = 0;

    while (index < text.size()) {
        if (text[index] == '\n') {
            lines.push_back(current);
            current.clear();
            usedWidth = 0;
            ++index;
            continue;
        }

        const std::size_t start = index;
        const char32_t codePoint = decodeUtf8(text, index);
        const int characterWidth = codePointWidth(codePoint);

        if (usedWidth > 0 && usedWidth + characterWidth > width) {
            lines.push_back(current);
            current.clear();
            usedWidth = 0;
        }

        current.append(text, start, index - start);
        usedWidth += characterWidth;
    }

    if (!current.empty() || lines.empty()) {
        lines.push_back(current);
    }
    return lines;
}

int readChoice(const std::string& prompt, int minChoice, int maxChoice) {
    while (true) {
        std::cout << prompt;

        std::string line;
        if (!std::getline(std::cin, line)) {
            return -1;
        }

        if (isBlank(line)) {
            std::cout << "  输入不能为空，请重新输入。\n";
            continue;
        }

        try {
            std::size_t usedCharacters = 0;
            const int value = std::stoi(line, &usedCharacters);

            while (usedCharacters < line.size() &&
                   std::isspace(static_cast<unsigned char>(line[usedCharacters]))) {
                ++usedCharacters;
            }
            if (usedCharacters != line.size()) {
                throw std::invalid_argument("trailing characters");
            }

            if (value < minChoice || value > maxChoice) {
                std::cout << "  请输入 " << minChoice << " 到 " << maxChoice << " 之间的数字。\n";
                continue;
            }
            return value;
        } catch (const std::exception&) {
            std::cout << "  输入无效，请输入数字。\n";
        }
    }
}

void waitForEnter(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string line;
    std::getline(std::cin, line);
}

}  // namespace UI
