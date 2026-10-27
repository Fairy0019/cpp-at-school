#include "Frame.h"

#include "UI.h"

#include <algorithm>
#include <iostream>

namespace {

std::string horizontalBorder(int width) {
    std::string line;
    line.reserve(static_cast<std::size_t>(width) * 3);
    for (int index = 0; index < width; ++index) {
        line += "═";
    }
    return line;
}

}  // namespace

Frame::Frame(int width, int minHeight)
    : width_(std::max(20, width)),
      minHeight_(std::max(0, minHeight)) {
}

int Frame::contentWidth() const {
    // 左右边框各 1 列，内容区左右各留 2 列内边距。
    return width_ - 6;
}

int Frame::addLine(const std::string& text) {
    rows_.push_back({Row::Kind::Text, text});
    return static_cast<int>(rows_.size()) - 1;
}

int Frame::addBlank() {
    return addLine();
}

int Frame::addCentered(const std::string& text) {
    rows_.push_back({Row::Kind::Text, UI::center(text, contentWidth())});
    return static_cast<int>(rows_.size()) - 1;
}

int Frame::addColumns(const std::string& left, const std::string& right, int leftWidth) {
    const int clampedLeftWidth = std::max(0, std::min(leftWidth, contentWidth()));
    std::string line = UI::padRight(left, clampedLeftWidth);
    const int rightWidth = contentWidth() - clampedLeftWidth;
    if (rightWidth > 0) {
        line += UI::fit(right, rightWidth);
    }
    rows_.push_back({Row::Kind::Text, line});
    return static_cast<int>(rows_.size()) - 1;
}

int Frame::addSplit(const std::string& left, const std::string& right, int leftWidth) {
    const int maxLeftWidth = contentWidth() - 1;
    const int clampedLeftWidth = std::max(0, std::min(leftWidth, maxLeftWidth));

    std::string line = UI::padRight(left, clampedLeftWidth);
    line += UI::paint("│", UI::Color::Gray);

    const int rightWidth = contentWidth() - clampedLeftWidth - 1;
    if (rightWidth > 0) {
        line += UI::fit(right, rightWidth);
    }
    rows_.push_back({Row::Kind::Text, line});
    return static_cast<int>(rows_.size()) - 1;
}

int Frame::addWrapped(const std::string& text, const std::string& prefix) {
    const int prefixWidth = UI::displayWidth(prefix);
    int availableWidth = contentWidth() - prefixWidth;
    if (availableWidth < 8) {
        availableWidth = contentWidth();
    }

    const std::vector<std::string> lines = UI::wrap(text, availableWidth);
    const int start = static_cast<int>(rows_.size());
    if (lines.empty()) {
        addLine(prefix);
        return start;
    }

    addLine(prefix + lines.front());
    const std::string indent(static_cast<std::size_t>(prefixWidth), ' ');
    for (std::size_t index = 1; index < lines.size(); ++index) {
        addLine(indent + lines[index]);
    }
    return start;
}

int Frame::addDivider() {
    rows_.push_back({Row::Kind::Divider, std::string()});
    return static_cast<int>(rows_.size()) - 1;
}

int Frame::rowCount() const {
    return static_cast<int>(rows_.size());
}

void Frame::render() const {
    const std::string horizontal = horizontalBorder(width_ - 2);
    const std::string side = UI::paint("║", UI::Color::Cyan);
    const std::string blank = side + std::string(static_cast<std::size_t>(contentWidth() + 4), ' ') + side;

    const int extra = std::max(0, minHeight_ - static_cast<int>(rows_.size()));
    const int topPadding = extra / 2;
    const int bottomPadding = extra - topPadding;

    std::cout << UI::paint("╔" + horizontal + "╗", UI::Color::Cyan) << "\n";
    for (int index = 0; index < topPadding; ++index) {
        std::cout << blank << "\n";
    }
    for (const Row& row : rows_) {
        if (row.kind == Row::Kind::Divider) {
            std::cout << UI::paint("╠" + horizontal + "╣", UI::Color::Cyan) << "\n";
        } else {
            std::cout << side << "  " << UI::fit(row.text, contentWidth()) << "  " << side << "\n";
        }
    }
    for (int index = 0; index < bottomPadding; ++index) {
        std::cout << blank << "\n";
    }
    std::cout << UI::paint("╚" + horizontal + "╝", UI::Color::Cyan) << "\n";
}

void Frame::renderTypewriter() const {
    std::vector<int> typedRows;
    for (int index = 0; index < static_cast<int>(rows_.size()); ++index) {
        if (rows_[static_cast<std::size_t>(index)].kind == Row::Kind::Text) {
            typedRows.push_back(index);
        }
    }
    renderTypewriter(typedRows);
}

void Frame::renderTypewriter(const std::vector<int>& typedRows) const {
    const std::string horizontal = horizontalBorder(width_ - 2);
    const std::string side = UI::paint("║", UI::Color::Cyan);
    const std::string blank = side + std::string(static_cast<std::size_t>(contentWidth() + 4), ' ') + side;

    const int extra = std::max(0, minHeight_ - static_cast<int>(rows_.size()));
    const int topPadding = extra / 2;
    const int bottomPadding = extra - topPadding;

    std::cout << UI::paint("╔" + horizontal + "╗", UI::Color::Cyan) << "\n";
    for (int index = 0; index < topPadding; ++index) {
        std::cout << blank << "\n";
    }

    for (int index = 0; index < static_cast<int>(rows_.size()); ++index) {
        const Row& row = rows_[static_cast<std::size_t>(index)];
        if (row.kind == Row::Kind::Divider) {
            std::cout << UI::paint("╠" + horizontal + "╣", UI::Color::Cyan) << "\n";
            continue;
        }

        std::cout << side << "  ";

        const bool typed = std::find(typedRows.begin(), typedRows.end(), index) != typedRows.end();
        if (!typed) {
            std::cout << UI::fit(row.text, contentWidth());
        } else {
            std::string line = UI::fit(row.text, contentWidth());
            while (!line.empty() && line.back() == ' ') {
                line.pop_back();
            }

            std::size_t leading = 0;
            while (leading < line.size() && line[leading] == ' ') {
                ++leading;
            }
            std::cout << line.substr(0, leading);
            UI::typeText(line.substr(leading), 6);

            const int used = UI::displayWidth(line);
            if (used < contentWidth()) {
                std::cout << std::string(static_cast<std::size_t>(contentWidth() - used), ' ');
            }
        }

        std::cout << "  " << side << "\n";
    }

    for (int index = 0; index < bottomPadding; ++index) {
        std::cout << blank << "\n";
    }
    std::cout << UI::paint("╚" + horizontal + "╝", UI::Color::Cyan) << "\n";
}
