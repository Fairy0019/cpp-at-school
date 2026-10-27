#include "Frame.h"

#include "UI.h"

#include <algorithm>
#include <iostream>

Frame::Frame(int width)
    : width_(std::max(20, width)) {
}

int Frame::contentWidth() const {
    // 左右边框各 1 列，内容区左右各留 2 列内边距。
    return width_ - 6;
}

void Frame::addLine(const std::string& text) {
    rows_.push_back({Row::Kind::Text, text});
}

void Frame::addBlank() {
    addLine();
}

void Frame::addCentered(const std::string& text) {
    rows_.push_back({Row::Kind::Text, UI::center(text, contentWidth())});
}

void Frame::addColumns(const std::string& left, const std::string& right, int leftWidth) {
    const int clampedLeftWidth = std::max(0, std::min(leftWidth, contentWidth()));
    std::string line = UI::padRight(left, clampedLeftWidth);
    const int rightWidth = contentWidth() - clampedLeftWidth;
    if (rightWidth > 0) {
        line += UI::fit(right, rightWidth);
    }
    rows_.push_back({Row::Kind::Text, line});
}

void Frame::addSplit(const std::string& left, const std::string& right, int leftWidth) {
    const int maxLeftWidth = contentWidth() - 1;
    const int clampedLeftWidth = std::max(0, std::min(leftWidth, maxLeftWidth));

    std::string line = UI::padRight(left, clampedLeftWidth);
    line += UI::paint("│", UI::Color::Gray);

    const int rightWidth = contentWidth() - clampedLeftWidth - 1;
    if (rightWidth > 0) {
        line += UI::fit(right, rightWidth);
    }
    rows_.push_back({Row::Kind::Text, line});
}

void Frame::addWrapped(const std::string& text, const std::string& prefix) {
    const int prefixWidth = UI::displayWidth(prefix);
    int availableWidth = contentWidth() - prefixWidth;
    if (availableWidth < 8) {
        availableWidth = contentWidth();
    }

    const std::vector<std::string> lines = UI::wrap(text, availableWidth);
    if (lines.empty()) {
        addLine(prefix);
        return;
    }

    addLine(prefix + lines.front());
    const std::string indent(static_cast<std::size_t>(prefixWidth), ' ');
    for (std::size_t index = 1; index < lines.size(); ++index) {
        addLine(indent + lines[index]);
    }
}

void Frame::addDivider() {
    rows_.push_back({Row::Kind::Divider, std::string()});
}

void Frame::render() const {
    std::string horizontal;
    horizontal.reserve(static_cast<std::size_t>(width_ - 2) * 3);
    for (int index = 0; index < width_ - 2; ++index) {
        horizontal += "═";
    }

    const std::string side = UI::paint("║", UI::Color::Cyan);

    std::cout << UI::paint("╔" + horizontal + "╗", UI::Color::Cyan) << "\n";
    for (const Row& row : rows_) {
        if (row.kind == Row::Kind::Divider) {
            std::cout << UI::paint("╠" + horizontal + "╣", UI::Color::Cyan) << "\n";
        } else {
            std::cout << side << "  " << UI::fit(row.text, contentWidth()) << "  " << side << "\n";
        }
    }
    std::cout << UI::paint("╚" + horizontal + "╝", UI::Color::Cyan) << "\n";
}
