#include "TuiHighlighter.h"
#include <ncurses.h>
#include <algorithm>

namespace UberPad {

namespace {
// Pair IDs 1–9 are also the syntax palette, shared by RGB matching below.
const QColor syntaxColors[] = {
    QColor("#F2E9DF"), QColor("#F1A08C"), QColor("#BAC99A"),
    QColor("#D9BC86"), QColor("#E6A16A"), QColor("#D7AD86"),
    QColor("#AFC8CC"), QColor("#FFF5EB"), QColor("#AAA49C")
};

int colorDistance(const QColor &a, const QColor &b) {
    const int r = a.red() - b.red();
    const int g = a.green() - b.green();
    const int blue = a.blue() - b.blue();
    return r * r + g * g + blue * blue;
}

short nearest256Color(const QColor &color) {
    // Avoid the first 16 entries: terminal profiles can redefine those.
    short best = 16;
    int distance = 3 * 255 * 255 + 1;
    constexpr int levels[] = {0, 95, 135, 175, 215, 255};
    for (short index = 16; index < 256; ++index) {
        const int cube = index - 16;
        const QColor candidate = index < 232
            ? QColor(levels[cube / 36], levels[(cube / 6) % 6], levels[cube % 6])
            : QColor(8 + (index - 232) * 10, 8 + (index - 232) * 10, 8 + (index - 232) * 10);
        const int current = colorDistance(color, candidate);
        if (current < distance) {
            best = index;
            distance = current;
        }
    }
    return best;
}
}

void TuiHighlighter::initColors() {
    if (!has_colors() || start_color() == ERR) return;

    const QColor background("#302D2B");
    const QColor panel("#38332F");
    const QColor accent("#A94710");
    const QColor errorBackground("#754A43");
    // Use the fixed xterm palette. Some terminals advertise mutable colors
    // but ignore OSC palette updates, leaving custom entries black or blue.
    const auto terminalColor = [](const QColor &color, short fallback) {
        return COLORS >= 256 ? nearest256Color(color) : fallback;
    };
    const short bg = terminalColor(background, COLOR_BLACK);
    const short panelBg = terminalColor(panel, COLOR_BLACK);
    const short accentBg = terminalColor(accent, COLOR_RED);
    const short errorBg = terminalColor(errorBackground, COLOR_RED);
    constexpr short fallbacks[] = {
        COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_YELLOW,
        COLOR_YELLOW, COLOR_CYAN, COLOR_WHITE, COLOR_WHITE
    };
    short foregrounds[9];
    for (short i = 0; i < 9; ++i) {
        foregrounds[i] = terminalColor(syntaxColors[i], fallbacks[i]);
        init_pair(i + 1, foregrounds[i], bg);
    }
    init_pair(10, foregrounds[7], accentBg); // Search / focused item
    init_pair(11, foregrounds[7], accentBg); // Selected tab / header
    init_pair(12, foregrounds[0], panelBg);  // Menu / status bar
    init_pair(13, foregrounds[4], bg);       // Current line number / hotkeys
    init_pair(14, foregrounds[7], errorBg);  // Error highlight

    // Also paint unused cells and cleared rows with the theme background.
    bkgd(COLOR_PAIR(1));
}

short TuiHighlighter::rgbToColorPair(const QColor &fg, const QColor &/* bg */) {
    if (!fg.isValid()) return 1;
    short best = 1;
    int distance = 3 * 255 * 255 + 1;
    for (short i = 0; i < 9; ++i) {
        const int current = colorDistance(fg, syntaxColors[i]);
        if (current < distance) {
            best = i + 1;
            distance = current;
        }
    }
    return best;
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
