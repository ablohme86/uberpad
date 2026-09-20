#include "TuiApp.h"
#include "../core/SyntaxManager.h"

#include <ncurses.h>
#include <algorithm>
#include <cmath>

namespace UberPad {

TuiApp::TuiApp() {
}

TuiApp::~TuiApp() {
    cleanupCurses();
}

void TuiApp::initCurses() {
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    TuiHighlighter::initColors();

    auto theme = SyntaxManager::instance().defaultDarkTheme();
    m_highlighter.setTheme(theme);

    getmaxyx(stdscr, m_termRows, m_termCols);
}

void TuiApp::cleanupCurses() {
    if (!isendwin()) {
        endwin();
    }
}

int TuiApp::run(const QStringList &files) {
    initCurses();

    if (!files.isEmpty()) {
        m_buffer.loadFromFile(files.first());
    }
    m_highlighter.setDefinition(m_buffer.definition());

    while (m_running) {
        render();
        int ch = getch();
        handleInput(ch);
    }

    cleanupCurses();
    return 0;
}

void TuiApp::render() {
    erase();
    getmaxyx(stdscr, m_termRows, m_termCols);

    int textRows = std::max(1, m_termRows - 2);

    // 1. Render Top Header
    attron(COLOR_PAIR(11) | A_BOLD);
    mvhline(0, 0, ' ', m_termCols);

    QString title = QStringLiteral(" UberPad [TUI]  |  File: %1%2  |  Lang: %3  |  ^S:Save  ^F:Find  ^G:Goto  ^Q:Quit")
                    .arg(m_buffer.fileName())
                    .arg(m_buffer.isModified() ? QStringLiteral(" [*]") : QString())
                    .arg(m_buffer.languageName());
    mvaddnstr(0, 0, title.toUtf8().constData(), m_termCols);
    attroff(COLOR_PAIR(11) | A_BOLD);

    // Calculate line number gutter width
    int digits = 1;
    int maxL = std::max(1, m_buffer.lineCount());
    while (maxL >= 10) {
        maxL /= 10;
        ++digits;
    }
    digits = std::max(3, digits);
    m_gutterWidth = digits + 3;

    int textCols = std::max(1, m_termCols - m_gutterWidth);

    // Adjust scrolling
    int curRow = m_buffer.cursorRow();
    int curCol = m_buffer.cursorCol();

    if (curRow < m_viewTopRow) {
        m_viewTopRow = curRow;
    } else if (curRow >= m_viewTopRow + textRows) {
        m_viewTopRow = curRow - textRows + 1;
    }

    if (curCol < m_viewLeftCol) {
        m_viewLeftCol = curCol;
    } else if (curCol >= m_viewLeftCol + textCols) {
        m_viewLeftCol = curCol - textCols + 1;
    }

    // 2. Render Text Lines
    KSyntaxHighlighting::State state;
    for (int r = 0; r < m_buffer.lineCount() && r < m_viewTopRow; ++r) {
        state = m_highlighter.advanceState(m_buffer.line(r), state);
    }

    for (int i = 0; i < textRows; ++i) {
        int lineIdx = m_viewTopRow + i;
        int screenY = 1 + i;

        if (lineIdx < m_buffer.lineCount()) {
            bool isCurrent = (lineIdx == curRow);

            // Draw line number
            if (isCurrent) {
                attron(COLOR_PAIR(13) | A_BOLD);
            } else {
                attron(COLOR_PAIR(9));
            }

            char gutterBuf[32];
            snprintf(gutterBuf, sizeof(gutterBuf), "%*d | ", digits, lineIdx + 1);
            mvaddstr(screenY, 0, gutterBuf);

            if (isCurrent) {
                attroff(COLOR_PAIR(13) | A_BOLD);
            } else {
                attroff(COLOR_PAIR(9));
            }

            // Draw syntax-highlighted line
            QString lineText = m_buffer.line(lineIdx);
            KSyntaxHighlighting::State nextState;
            auto spans = m_highlighter.highlight(lineText, state, nextState);
            state = nextState;

            // Character by character attribute buffer
            std::vector<short> fgAttrs(lineText.size(), 1);
            std::vector<bool> boldAttrs(lineText.size(), false);
            std::vector<bool> underlineAttrs(lineText.size(), false);

            for (const auto &span : spans) {
                for (int pos = span.offset; pos < span.offset + span.length && pos < (int)lineText.size(); ++pos) {
                    fgAttrs[pos] = span.colorPair;
                    boldAttrs[pos] = span.bold;
                    underlineAttrs[pos] = span.underline;
                }
            }

            // Mark search matches
            if (!m_searchQuery.isEmpty()) {
                int searchIdx = 0;
                while ((searchIdx = lineText.indexOf(m_searchQuery, searchIdx, Qt::CaseInsensitive)) >= 0) {
                    for (int s = searchIdx; s < searchIdx + m_searchQuery.size() && s < (int)lineText.size(); ++s) {
                        fgAttrs[s] = 10; // Yellow match
                        boldAttrs[s] = true;
                    }
                    searchIdx += m_searchQuery.size();
                }
            }

            // Output visible slice of characters
            for (int col = 0; col < textCols; ++col) {
                int charIdx = m_viewLeftCol + col;
                if (charIdx < lineText.size()) {
                    short pair = fgAttrs[charIdx];
                    int attr = COLOR_PAIR(pair);
                    if (boldAttrs[charIdx]) attr |= A_BOLD;
                    if (underlineAttrs[charIdx]) attr |= A_UNDERLINE;

                    attron(attr);
                    QChar qc = lineText[charIdx];
                    mvaddch(screenY, m_gutterWidth + col, qc.toLatin1() ? qc.toLatin1() : ' ');
                    attroff(attr);
                } else {
                    break;
                }
            }
        } else {
            // Tilde ~ for empty rows past end of file
            attron(COLOR_PAIR(9));
            mvaddch(screenY, 0, '~');
            attroff(COLOR_PAIR(9));
        }
    }

    // 3. Render Bottom Status Bar
    attron(COLOR_PAIR(12) | A_BOLD);
    mvhline(m_termRows - 1, 0, ' ', m_termCols);

    QString status;
    if (!m_statusMessage.isEmpty()) {
        status = QStringLiteral(" %1").arg(m_statusMessage);
    } else {
        status = QStringLiteral(" Ln %1, Col %2  |  %3 lines  |  UTF-8  |  LF")
                 .arg(curRow + 1)
                 .arg(curCol + 1)
                 .arg(m_buffer.lineCount());
    }
    mvaddnstr(m_termRows - 1, 0, status.toUtf8().constData(), m_termCols);
    attroff(COLOR_PAIR(12) | A_BOLD);

    // 4. Place Hardware Cursor
    int screenCursorY = 1 + (curRow - m_viewTopRow);
    int screenCursorX = m_gutterWidth + (curCol - m_viewLeftCol);
    move(screenCursorY, screenCursorX);

    refresh();
}

QString TuiApp::prompt(const QString &msg, const QString &initial) {
    attron(COLOR_PAIR(11) | A_BOLD);
    mvhline(m_termRows - 1, 0, ' ', m_termCols);
    mvaddstr(m_termRows - 1, 0, msg.toUtf8().constData());
    attroff(COLOR_PAIR(11) | A_BOLD);

    QString input = initial;
    int promptLen = (int)msg.size();

    while (true) {
        mvhline(m_termRows - 1, promptLen, ' ', m_termCols - promptLen);
        mvaddnstr(m_termRows - 1, promptLen, input.toUtf8().constData(), m_termCols - promptLen);
        move(m_termRows - 1, promptLen + input.size());
        refresh();

        int ch = getch();
        if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
            break;
        } else if (ch == 27) { // Escape
            return QString();
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!input.isEmpty()) {
                input.chop(1);
            }
        } else if (ch >= 32 && ch <= 126) {
            input.append(QChar(ch));
        }
    }
    return input;
}

bool TuiApp::confirm(const QString &msg) {
    QString ans = prompt(msg + QStringLiteral(" (y/n): "));
    return (ans.compare(QStringLiteral("y"), Qt::CaseInsensitive) == 0);
}

void TuiApp::onSave() {
    if (m_buffer.filePath().isEmpty()) {
        QString path = prompt(QStringLiteral("Save as: "));
        if (!path.isEmpty()) {
            if (m_buffer.saveToFile(path)) {
                m_highlighter.setDefinition(m_buffer.definition());
                m_statusMessage = QStringLiteral("Saved: %1").arg(path);
            } else {
                m_statusMessage = QStringLiteral("Error saving file: %1").arg(path);
            }
        }
    } else {
        if (m_buffer.saveToFile()) {
            m_statusMessage = QStringLiteral("Saved: %1").arg(m_buffer.filePath());
        } else {
            m_statusMessage = QStringLiteral("Error saving file");
        }
    }
}

void TuiApp::onFind() {
    QString query = prompt(QStringLiteral("Find: "), m_searchQuery);
    if (!query.isEmpty()) {
        m_searchQuery = query;
        if (!m_buffer.find(query, true)) {
            m_statusMessage = QStringLiteral("Pattern not found: %1").arg(query);
        } else {
            m_statusMessage.clear();
        }
    } else {
        m_searchQuery.clear();
    }
}

void TuiApp::onGoToLine() {
    QString val = prompt(QStringLiteral("Go to line: "));
    bool ok = false;
    int line = val.toInt(&ok);
    if (ok && line >= 1) {
        m_buffer.setCursor(line - 1, 0);
        m_statusMessage.clear();
    }
}

void TuiApp::handleInput(int ch) {
    m_statusMessage.clear();

    if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, m_termRows, m_termCols);
        return;
    }

    // Navigation
    if (ch == KEY_UP) {
        m_buffer.moveUp();
    } else if (ch == KEY_DOWN) {
        m_buffer.moveDown();
    } else if (ch == KEY_LEFT) {
        m_buffer.moveLeft();
    } else if (ch == KEY_RIGHT) {
        m_buffer.moveRight();
    } else if (ch == KEY_HOME) {
        m_buffer.moveHome();
    } else if (ch == KEY_END) {
        m_buffer.moveEnd();
    } else if (ch == KEY_PPAGE) {
        m_buffer.movePageUp(m_termRows - 2);
    } else if (ch == KEY_NPAGE) {
        m_buffer.movePageDown(m_termRows - 2);
    }
    // Deletion
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        m_buffer.backspace();
    } else if (ch == KEY_DC) {
        m_buffer.deleteChar();
    }
    // Enter & Tab
    else if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
        m_buffer.insertNewline();
    } else if (ch == 9) { // Tab
        m_buffer.insertTab();
    }
    // Control Commands
    else if (ch == 19) { // Ctrl+S
        onSave();
    } else if (ch == 6) { // Ctrl+F
        onFind();
    } else if (ch == 7) { // Ctrl+G
        onGoToLine();
    } else if (ch == 17) { // Ctrl+Q
        if (m_buffer.isModified()) {
            if (confirm(QStringLiteral("File has unsaved changes. Discard and quit?"))) {
                m_running = false;
            }
        } else {
            m_running = false;
        }
    }
    // Printable characters
    else if (ch >= 32 && ch <= 126) {
        m_buffer.insertChar(QChar(ch));
    }
}

} // namespace UberPad
