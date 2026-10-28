#pragma once

#include <string>
#include <vector>

// 画面框：负责边框、内边距、居中与分栏对齐。
// 所有内容按“显示宽度”计算，中文不会被当成两个字节宽度。
class Frame {
public:
    static constexpr int kDefaultWidth = 105;
    // 画框内容区的目标高度；外框固定为 34 行（内容 32 行 + 上下边框）。
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

    // 上景 / 下景固定留白；复杂界面可把下景调到 4 行。
    void setTopPadding(int rows);
    void setBottomPadding(int rows);
    // 指定上景 / 下景占用的行数；默认自动识别标题分隔线和底部短菜单。
    void setHeaderRows(int rows);
    void setFooterRows(int rows);

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

    struct Padding {
        int top;
        int header;
        int middleTop;
        int middleBottom;
        int footer;
        int bottom;
    };

    int contentWidth() const;
    int resolvedHeaderRows() const;
    int resolvedFooterRows(int headerRows) const;
    Padding computePadding() const;

    int width_;
    int minHeight_;
    int topPadding_ = 2;
    int bottomPadding_ = 2;
    int headerRows_ = -1;
    int footerRows_ = -1;
    std::vector<Row> rows_;
};
