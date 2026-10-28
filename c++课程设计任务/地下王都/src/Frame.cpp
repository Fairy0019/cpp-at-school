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

void Frame::setTopPadding(int rows) {
    topPadding_ = std::max(0, rows);
}

void Frame::setBottomPadding(int rows) {
    bottomPadding_ = std::max(0, rows);
}

void Frame::setHeaderRows(int rows) {
    headerRows_ = std::max(0, rows);
}

void Frame::setFooterRows(int rows) {
    footerRows_ = std::max(0, rows);
}

int Frame::resolvedHeaderRows() const {
    const int count = static_cast<int>(rows_.size());
    if (headerRows_ >= 0) {
        return std::min(headerRows_, count);
    }

    // 默认把第一条分隔线（含）之前当作上景。
    for (int index = 0; index < count; ++index) {
        if (rows_[static_cast<std::size_t>(index)].kind == Row::Kind::Divider) {
            return index + 1;
        }
    }
    return 0;
}

int Frame::resolvedFooterRows(int headerRows) const {
    const int count = static_cast<int>(rows_.size());
    if (footerRows_ >= 0) {
        return std::min(footerRows_, std::max(0, count - headerRows));
    }

    // 默认把最后一个分隔线后面的短菜单当作下景，固定在画框底部。
    for (int index = count - 1; index >= headerRows; --index) {
        if (rows_[static_cast<std::size_t>(index)].kind == Row::Kind::Divider) {
            const int footer = count - index;
            return footer <= 4 ? footer : 0;
        }
    }
    return 0;
}

Frame::Padding Frame::computePadding() const {
    const int count = static_cast<int>(rows_.size());
    const int header = resolvedHeaderRows();
    const int footer = resolvedFooterRows(header);
    const int bodyRows = std::max(0, count - header - footer);
    const int middleRows = std::max(minHeight_, bodyRows);
    const int extra = middleRows - bodyRows;
    return {topPadding_, header, extra / 2, extra - extra / 2, footer, bottomPadding_};
}

void Frame::render() const {
    const std::string horizontal = horizontalBorder(width_ - 2);
    const std::string side = UI::paint("║", UI::Color::Cyan);
    const std::string blank = side + std::string(static_cast<std::size_t>(contentWidth() + 4), ' ') + side;

    const Padding padding = computePadding();
    const int count = static_cast<int>(rows_.size());
    const int bodyStart = padding.header;
    const int bodyEnd = count - padding.footer;

    const auto renderRow = [&](const Row& row) {
        if (row.kind == Row::Kind::Divider) {
            std::cout << UI::paint("╠" + horizontal + "╣", UI::Color::Cyan) << "\n";
        } else {
            std::cout << side << "  " << UI::fit(row.text, contentWidth()) << "  " << side << "\n";
        }
    };

    std::cout << UI::paint("╔" + horizontal + "╗", UI::Color::Cyan) << "\n";
    for (int index = 0; index < padding.top; ++index) {
        std::cout << blank << "\n";
    }
    for (int index = 0; index < bodyStart; ++index) {
        renderRow(rows_[static_cast<std::size_t>(index)]);
    }
    for (int index = 0; index < padding.middleTop; ++index) {
        std::cout << blank << "\n";
    }
    for (int index = bodyStart; index < bodyEnd; ++index) {
        renderRow(rows_[static_cast<std::size_t>(index)]);
    }
    for (int index = 0; index < padding.middleBottom; ++index) {
        std::cout << blank << "\n";
    }
    for (int index = bodyEnd; index < count; ++index) {
        renderRow(rows_[static_cast<std::size_t>(index)]);
    }
    for (int index = 0; index < padding.bottom; ++index) {
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

    const Padding padding = computePadding();
    const int count = static_cast<int>(rows_.size());
    const int bodyStart = padding.header;
    const int bodyEnd = count - padding.footer;
    const int bodyRows = std::max(0, bodyEnd - bodyStart);

    const bool animate = UI::isInteractive() && !typedRows.empty();
    std::vector<bool> typed(rows_.size(), false);
    for (int index : typedRows) {
        if (index >= 0 && index < count &&
            rows_[static_cast<std::size_t>(index)].kind == Row::Kind::Text) {
            typed[static_cast<std::size_t>(index)] = true;
        }
    }

    std::vector<std::string> lines;
    lines.reserve(static_cast<std::size_t>(padding.top + bodyStart +
                                            padding.middleTop + bodyRows +
                                            padding.middleBottom + padding.footer +
                                            padding.bottom + 2));
    lines.push_back(UI::paint("╔" + horizontal + "╗", UI::Color::Cyan));
    for (int index = 0; index < padding.top; ++index) {
        lines.push_back(blank);
    }

    const auto appendRow = [&](int index) {
        const Row& row = rows_[static_cast<std::size_t>(index)];
        if (row.kind == Row::Kind::Divider) {
            lines.push_back(UI::paint("╠" + horizontal + "╣", UI::Color::Cyan));
        } else if (animate && typed[static_cast<std::size_t>(index)]) {
            lines.push_back(side + "  " +
                            std::string(static_cast<std::size_t>(contentWidth()), ' ') +
                            "  " + side);
        } else {
            lines.push_back(side + "  " + UI::fit(row.text, contentWidth()) + "  " + side);
        }
    };

    for (int index = 0; index < bodyStart; ++index) {
        appendRow(index);
    }
    for (int index = 0; index < padding.middleTop; ++index) {
        lines.push_back(blank);
    }
    for (int index = bodyStart; index < bodyEnd; ++index) {
        appendRow(index);
    }
    for (int index = 0; index < padding.middleBottom; ++index) {
        lines.push_back(blank);
    }
    for (int index = bodyEnd; index < count; ++index) {
        appendRow(index);
    }
    for (int index = 0; index < padding.bottom; ++index) {
        lines.push_back(blank);
    }
    lines.push_back(UI::paint("╚" + horizontal + "╝", UI::Color::Cyan));

    for (const std::string& line : lines) {
        std::cout << line << "\n";
    }
    std::cout.flush();

    if (!animate) {
        return;
    }

    UI::saveCursor();
    UI::hideCursor();

    const int totalLines = static_cast<int>(lines.size());
    for (int index = 0; index < count; ++index) {
        if (!typed[static_cast<std::size_t>(index)]) {
            continue;
        }

        UI::restoreCursor();
        const int lineIndex =
            index < bodyStart
                ? 1 + padding.top + index
                : (index < bodyEnd
                       ? 1 + padding.top + bodyStart + padding.middleTop +
                             (index - bodyStart)
                       : 1 + padding.top + bodyStart + padding.middleTop +
                             bodyRows + padding.middleBottom + (index - bodyEnd));
        UI::moveCursorUp(totalLines - lineIndex);
        UI::moveCursorRight(3);

        std::string content = UI::fit(rows_[static_cast<std::size_t>(index)].text,
                                      contentWidth());
        while (!content.empty() && content.back() == ' ') {
            content.pop_back();
        }

        std::size_t leading = 0;
        while (leading < content.size() && content[leading] == ' ') {
            ++leading;
        }
        std::cout << content.substr(0, leading);
        UI::typeText(content.substr(leading), 6);
    }

    UI::restoreCursor();
    UI::showCursor();
    std::cout.flush();
}
