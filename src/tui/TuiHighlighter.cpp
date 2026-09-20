#include "TuiHighlighter.h"
#include <ncurses.h>
#include <algorithm>

namespace UberPad {

void TuiHighlighter::initColors() {
    if (!has_colors()) return;

    start_color();
    use_default_colors();

    init_pair(1, COLOR_WHITE, -1);      // Normal
    init_pair(2, COLOR_RED, -1);        // Red / error
    init_pair(3, COLOR_GREEN, -1);      // Green / string
    init_pair(4, COLOR_YELLOW, -1);     // Yellow / number / warning
    init_pair(5, COLOR_BLUE, -1);       // Blue / keyword
    init_pair(6, COLOR_MAGENTA, -1);    // Magenta / control
    init_pair(7, COLOR_CYAN, -1);       // Cyan / type
    init_pair(8, COLOR_WHITE, -1);      // Bright text
    init_pair(9, COLOR_BLACK, -1);      // Gray / comment (with bold)
    init_pair(10, COLOR_BLACK, COLOR_YELLOW); // Search match
    init_pair(11, COLOR_WHITE, COLOR_BLUE);   // Header bar
    init_pair(12, COLOR_BLACK, COLOR_WHITE);  // Status bar
    init_pair(13, COLOR_CYAN, -1);            // Line numbers
    init_pair(14, COLOR_WHITE, COLOR_RED);    // Error highlight
}

short TuiHighlighter::rgbToColorPair(const QColor &fg, const QColor &/* bg */) {
    if (!fg.isValid()) return 1;

    int r = fg.red();
    int g = fg.green();
    int b = fg.blue();

    // Gray / dark
    if (r < 140 && g < 140 && b < 140) {
        return 9;
    }
    // Red dominant
    if (r > 160 && g < 110 && b < 110) {
        return 2;
    }
    // Green dominant
    if (g > 140 && r < 140 && b < 150) {
        return 3;
    }
    // Yellow (high red and green, low blue)
    if (r > 160 && g > 140 && b < 130) {
        return 4;
    }
    // Blue dominant
    if (b > 150 && r < 130 && g < 150) {
        return 5;
    }
    // Magenta / Purple (high red and blue, low green)
    if (r > 150 && b > 140 && g < 130) {
        return 6;
    }
    // Cyan (high green and blue, low red)
    if (g > 140 && b > 150 && r < 140) {
        return 7;
    }

    return 1;
}

TuiHighlighter::TuiHighlighter() {
}

void TuiHighlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) {
    if (!format.isValid() || length <= 0) return;

    QColor fg = format.textColor(theme());
    short pair = rgbToColorPair(fg);
    bool bold = format.isBold(theme());
    bool underline = format.isUnderline(theme());

    m_currentSpans.push_back({ offset, length, pair, bold, underline });
}

std::vector<TuiSpan> TuiHighlighter::highlight(const QString &line, const KSyntaxHighlighting::State &state, KSyntaxHighlighting::State &outState) {
    m_currentSpans.clear();
    outState = highlightLine(line, state);
    return m_currentSpans;
}

} // namespace UberPad
