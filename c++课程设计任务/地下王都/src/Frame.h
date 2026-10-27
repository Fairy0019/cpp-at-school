#pragma once

#include <string>
#include <vector>

// 画面框：负责边框、内边距、居中与分栏对齐。
// 所有内容按“显示宽度”计算，中文不会被当成两个字节宽度。
class Frame {
public:
    static constexpr int kDefaultWidth = 105;
    static constexpr int kDefaultHeight = 32;

    explicit Frame(int width = kDefaultWidth, int minHeight = kDefaultHeight);

    int addLine(const std::string& text = "");
    int addBlank();
    int addCentered(const std::string& text);
    int addColumns(const std::string& left, const std::string& right, int leftWidth);
    int addSplit(const std::string& left, const std::string& right, int leftWidth);
    int addWrapped(const std::string& text, const std::string& prefix = "");
    int addDivider();

    int rowCount() const;

    void render() const;

    // 逐字打出所有文字行；边框和分隔线仍然立即出现。
    void renderTypewriter() const;

    // 只逐字打出指定行，其余行立即输出。
    void renderTypewriter(const std::vector<int>& typedRows) const;

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
    int minHeight_;
    std::vector<Row> rows_;
};
