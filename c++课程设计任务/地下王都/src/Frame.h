#pragma once

#include <string>
#include <vector>

// 画面框：负责边框、内边距、居中与分栏对齐。
// 所有内容按“显示宽度”计算，中文不会被当成两个字节宽度。
class Frame {
public:
    explicit Frame(int width = 70);

    void addLine(const std::string& text = "");
    void addBlank();
    void addCentered(const std::string& text);
    void addColumns(const std::string& left, const std::string& right, int leftWidth);
    void addSplit(const std::string& left, const std::string& right, int leftWidth);
    void addWrapped(const std::string& text, const std::string& prefix = "");
    void addDivider();

    void render() const;

private:
    struct Row {
        enum class Kind {
            Text,
            Divider
        };

        Kind kind;
        std::string text;
    };

    int contentWidth() const;

    int width_;
    std::vector<Row> rows_;
};
