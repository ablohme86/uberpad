#pragma once

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/State>

#include <QString>
#include <vector>

namespace UberPad {

struct TuiSpan {
    int offset = 0;
    int length = 0;
    short colorPair = 0;
    bool bold = false;
    bool underline = false;
};

class TuiHighlighter : public KSyntaxHighlighting::AbstractHighlighter {
public:
    TuiHighlighter();
    ~TuiHighlighter() override = default;

    static void initColors();
    static short rgbToColorPair(const QColor &fg, const QColor &bg = QColor());

    std::vector<TuiSpan> highlight(const QString &line, const KSyntaxHighlighting::State &state, KSyntaxHighlighting::State &outState);
    KSyntaxHighlighting::State advanceState(const QString &line, const KSyntaxHighlighting::State &state) {
        return highlightLine(line, state);
    }

protected:
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) override;

private:
    std::vector<TuiSpan> m_currentSpans;
};

} // namespace UberPad
