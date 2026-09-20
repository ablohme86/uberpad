#include "TuiApp.h"
#include "../core/SyntaxManager.h"

#include <ncurses.h>
#include <clocale>
#include <algorithm>
#include <cmath>

namespace UberPad {

TuiApp::TuiApp() {
    m_fileTree.setRootPath(QDir::currentPath());
}

TuiApp::~TuiApp() {
    cleanupCurses();
}

void TuiApp::initCurses() {
    setlocale(LC_ALL, "");
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    set_escdelay(50);
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

TuiBuffer* TuiApp::currentBuffer() {
    if (m_buffers.empty()) return nullptr;
    m_activeBufferIndex = std::clamp(m_activeBufferIndex, 0, (int)m_buffers.size() - 1);
    return m_buffers[m_activeBufferIndex].get();
}

const TuiBuffer* TuiApp::currentBuffer() const {
    if (m_buffers.empty()) return nullptr;
    int idx = std::clamp(m_activeBufferIndex, 0, (int)m_buffers.size() - 1);
    return m_buffers[idx].get();
}

void TuiApp::newBuffer() {
    auto buf = std::make_unique<TuiBuffer>();
    m_buffers.push_back(std::move(buf));
    m_activeBufferIndex = (int)m_buffers.size() - 1;
    m_highlighter.setDefinition(currentBuffer()->definition());
    m_focus = TuiFocus::Editor;
}

void TuiApp::openFile(const QString &filePath) {
    if (filePath.isEmpty()) return;

    // Check if already open
    for (size_t i = 0; i < m_buffers.size(); ++i) {
        if (m_buffers[i]->filePath() == filePath) {
            m_activeBufferIndex = (int)i;
            m_highlighter.setDefinition(currentBuffer()->definition());
            m_focus = TuiFocus::Editor;
            return;
        }
    }

    // Reuse if only 1 empty untitled buffer
    if (m_buffers.size() == 1 && currentBuffer()->filePath().isEmpty() &&
        !currentBuffer()->isModified() && currentBuffer()->lineCount() <= 1 &&
        currentBuffer()->line(0).isEmpty()) {
        if (currentBuffer()->loadFromFile(filePath)) {
            m_highlighter.setDefinition(currentBuffer()->definition());
            m_statusMessage = QStringLiteral("Opened: %1").arg(filePath);
            m_focus = TuiFocus::Editor;
            return;
        }
    }

    auto buf = std::make_unique<TuiBuffer>();
    if (buf->loadFromFile(filePath)) {
        m_buffers.push_back(std::move(buf));
        m_activeBufferIndex = (int)m_buffers.size() - 1;
        m_highlighter.setDefinition(currentBuffer()->definition());
        m_statusMessage = QStringLiteral("Opened: %1").arg(filePath);
        m_focus = TuiFocus::Editor;
    } else {
        m_statusMessage = QStringLiteral("Could not open: %1").arg(filePath);
    }
}

bool TuiApp::closeCurrentTab() {
    if (m_buffers.empty()) return true;

    TuiBuffer *buf = currentBuffer();
    if (buf && buf->isModified()) {
        if (!confirm(QStringLiteral("File '%1' has unsaved changes. Discard?").arg(buf->fileName()))) {
            return false;
        }
    }

    m_buffers.erase(m_buffers.begin() + m_activeBufferIndex);

    if (m_buffers.empty()) {
        newBuffer();
    } else {
        m_activeBufferIndex = std::min(m_activeBufferIndex, (int)m_buffers.size() - 1);
        m_highlighter.setDefinition(currentBuffer()->definition());
    }
    return true;
}

int TuiApp::run(const QStringList &files) {
    initCurses();

    if (!files.isEmpty()) {
        for (const QString &f : files) {
            openFile(f);
        }
    } else {
        newBuffer();
    }

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

    // 1. Render Menu Bar (Row 0)
    m_menuBar.render(m_termCols);

    // 2. Render Document Tab Bar (Row 1)
    attron(COLOR_PAIR(12));
    mvhline(1, 0, ' ', m_termCols);
    int tabCol = 1;
    for (size_t i = 0; i < m_buffers.size(); ++i) {
        bool isActive = ((int)i == m_activeBufferIndex);
        QString tabText = QStringLiteral(" %1: %2%3 ")
                          .arg(i + 1)
                          .arg(m_buffers[i]->fileName())
                          .arg(m_buffers[i]->isModified() ? QStringLiteral(" *") : QString());

        if (isActive) {
            attron(COLOR_PAIR(11) | A_BOLD);
        } else {
            attron(COLOR_PAIR(12));
        }

        mvaddstr(1, tabCol, tabText.toUtf8().constData());

        if (isActive) {
            attroff(COLOR_PAIR(11) | A_BOLD);
        }

        tabCol += tabText.size() + 1;
        if (tabCol >= m_termCols - 10) break;
    }
    attroff(COLOR_PAIR(12));

    // 3. Layout Dimensions
    int contentTop = 2;
    int contentBottom = m_termRows - 2;
    int contentHeight = std::max(1, contentBottom - contentTop + 1);

    int editorStartCol = 0;
    if (m_showSidebar) {
        m_sidebarWidth = std::clamp(m_sidebarWidth, 18, std::max(18, m_termCols / 3));
        m_fileTree.render(contentTop, 0, m_sidebarWidth, contentHeight, m_focus == TuiFocus::Sidebar);

        // Vertical divider line between sidebar and editor
        attron(COLOR_PAIR(1));
        for (int r = contentTop; r <= contentBottom; ++r) {
            mvaddch(r, m_sidebarWidth, ACS_VLINE);
        }
        attroff(COLOR_PAIR(1));

        editorStartCol = m_sidebarWidth + 1;
    }

    int editorWidth = std::max(10, m_termCols - editorStartCol);

    // 4. Render Editor Content
    TuiBuffer *buf = currentBuffer();
    if (buf) {
        // Calculate gutter width
        int digits = 1;
        int maxL = std::max(1, buf->lineCount());
        while (maxL >= 10) {
            maxL /= 10;
            ++digits;
        }
        digits = std::max(3, digits);
        m_gutterWidth = m_showLineNumbers ? (digits + 3) : 0;

        int textCols = std::max(1, editorWidth - m_gutterWidth);
        int textRows = contentHeight;

        // Viewport scroll adjustment
        int curRow = buf->cursorRow();
        int curCol = buf->cursorCol();

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

        // Compute highlighting state up to top of viewport
        KSyntaxHighlighting::State state;
        for (int r = 0; r < buf->lineCount() && r < m_viewTopRow; ++r) {
            state = m_highlighter.advanceState(buf->line(r), state);
        }

        // Render visible text lines
        for (int i = 0; i < textRows; ++i) {
            int lineIdx = m_viewTopRow + i;
            int screenY = contentTop + i;

            // Clear editor row
            mvhline(screenY, editorStartCol, ' ', editorWidth);

            if (lineIdx < buf->lineCount()) {
                bool isCurrent = (lineIdx == curRow);

                // Line number
                if (m_showLineNumbers) {
                    if (isCurrent && m_focus == TuiFocus::Editor) {
                        attron(COLOR_PAIR(13) | A_BOLD);
                    } else {
                        attron(COLOR_PAIR(9));
                    }

                    char gutterBuf[32];
                    snprintf(gutterBuf, sizeof(gutterBuf), "%*d │ ", digits, lineIdx + 1);
                    mvaddstr(screenY, editorStartCol, gutterBuf);

                    if (isCurrent && m_focus == TuiFocus::Editor) {
                        attroff(COLOR_PAIR(13) | A_BOLD);
                    } else {
                        attroff(COLOR_PAIR(9));
                    }
                }

                // Highlighted code
                QString lineText = buf->line(lineIdx);
                KSyntaxHighlighting::State nextState;
                auto spans = m_highlighter.highlight(lineText, state, nextState);
                state = nextState;

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

                // Highlight search matches
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

                int textStartX = editorStartCol + m_gutterWidth;
                for (int col = 0; col < textCols; ++col) {
                    int charIdx = m_viewLeftCol + col;
                    if (charIdx < lineText.size()) {
                        short pair = fgAttrs[charIdx];
                        int attr = COLOR_PAIR(pair);
                        if (boldAttrs[charIdx]) attr |= A_BOLD;
                        if (underlineAttrs[charIdx]) attr |= A_UNDERLINE;

                        attron(attr);
                        QChar qc = lineText[charIdx];
                        mvaddch(screenY, textStartX + col, qc.toLatin1() ? qc.toLatin1() : ' ');
                        attroff(attr);
                    } else {
                        break;
                    }
                }
            } else {
                attron(COLOR_PAIR(9));
                mvaddch(screenY, editorStartCol, '~');
                attroff(COLOR_PAIR(9));
            }
        }
    }

    // 5. Render Bottom Status Bar (Row Rows-1)
    attron(COLOR_PAIR(12) | A_BOLD);
    mvhline(m_termRows - 1, 0, ' ', m_termCols);

    if (!m_statusMessage.isEmpty()) {
        QString msg = QStringLiteral(" %1 ").arg(m_statusMessage);
        mvaddnstr(m_termRows - 1, 0, msg.toUtf8().constData(), m_termCols);
    } else if (buf) {
        int curRow = buf->cursorRow() + 1;
        int curCol = buf->cursorCol() + 1;
        QString status = QStringLiteral(" Ln %1, Col %2   │   Lines: %3, Length: %4   │   Unix (LF)   │   UTF-8   │   %5   │   INS")
                         .arg(curRow)
                         .arg(curCol)
                         .arg(buf->lineCount())
                         .arg(buf->characterCount())
                         .arg(buf->languageName());
        mvaddnstr(m_termRows - 1, 0, status.toUtf8().constData(), m_termCols);
    }
    attroff(COLOR_PAIR(12) | A_BOLD);

    // 6. Draw Menu Dropdown on top if open
    if (m_menuBar.isActive()) {
        m_menuBar.render(m_termCols);
    }

    // 7. Cursor placement
    if (m_focus == TuiFocus::Editor && buf && !m_menuBar.isActive()) {
        int screenY = contentTop + (buf->cursorRow() - m_viewTopRow);
        int screenX = editorStartCol + m_gutterWidth + (buf->cursorCol() - m_viewLeftCol);
        move(screenY, screenX);
    } else if (m_focus == TuiFocus::Sidebar && !m_menuBar.isActive()) {
        int screenY = contentTop + 1 + (m_fileTree.selectedIndex());
        move(screenY, 1);
    }

    refresh();
}

void TuiApp::executeAction(TuiAction act) {
    switch (act) {
        case TuiAction::FileNew:
            newBuffer();
            break;
        case TuiAction::FileOpen: {
            QString path = prompt(QStringLiteral("Open file path: "));
            if (!path.isEmpty()) openFile(path);
            break;
        }
        case TuiAction::FileSave:
            onSave();
            break;
        case TuiAction::FileSaveAs:
            onSaveAs();
            break;
        case TuiAction::FileCloseTab:
            closeCurrentTab();
            break;
        case TuiAction::FileExit:
            if (currentBuffer() && currentBuffer()->isModified()) {
                if (confirm(QStringLiteral("File has unsaved changes. Discard and quit?"))) {
                    m_running = false;
                }
            } else {
                m_running = false;
            }
            break;
        case TuiAction::EditDuplicateLine:
            if (currentBuffer()) currentBuffer()->duplicateLine();
            break;
        case TuiAction::EditToggleComment:
            if (currentBuffer()) currentBuffer()->toggleComment();
            break;
        case TuiAction::EditGoToLine:
            onGoToLine();
            break;
        case TuiAction::SearchFind:
            onFind();
            break;
        case TuiAction::ViewToggleSidebar:
            m_showSidebar = !m_showSidebar;
            break;
        case TuiAction::ViewToggleLineNumbers:
            m_showLineNumbers = !m_showLineNumbers;
            break;
        case TuiAction::LangAuto:
            if (currentBuffer()) {
                auto def = SyntaxManager::instance().definitionForFileName(currentBuffer()->filePath());
                currentBuffer()->setDefinition(def.isValid() ? def : SyntaxManager::instance().defaultDefinition());
                m_highlighter.setDefinition(currentBuffer()->definition());
            }
            break;
        case TuiAction::LangCpp:
        case TuiAction::LangPython:
        case TuiAction::LangRust:
        case TuiAction::LangGo:
        case TuiAction::LangJS:
        case TuiAction::LangHTML:
        case TuiAction::LangMarkdown:
        case TuiAction::LangPlainText: {
            QString lang = QStringLiteral("Plain Text");
            if (act == TuiAction::LangCpp) lang = QStringLiteral("C++");
            else if (act == TuiAction::LangPython) lang = QStringLiteral("Python");
            else if (act == TuiAction::LangRust) lang = QStringLiteral("Rust");
            else if (act == TuiAction::LangGo) lang = QStringLiteral("Go");
            else if (act == TuiAction::LangJS) lang = QStringLiteral("JavaScript");
            else if (act == TuiAction::LangHTML) lang = QStringLiteral("HTML");
            else if (act == TuiAction::LangMarkdown) lang = QStringLiteral("Markdown");

            auto def = SyntaxManager::instance().definitionForName(lang);
            if (currentBuffer()) {
                currentBuffer()->setDefinition(def);
                m_highlighter.setDefinition(def);
            }
            break;
        }
        case TuiAction::HelpAbout:
            onAbout();
            break;
        default:
            break;
    }
}

void TuiApp::handleInput(int ch) {
    m_statusMessage.clear();

    if (ch == KEY_RESIZE) {
        getmaxyx(stdscr, m_termRows, m_termCols);
        return;
    }

    // 1. Menu Bar Navigation
    if (m_menuBar.isActive()) {
        if (ch == KEY_LEFT) {
            m_menuBar.moveLeft();
        } else if (ch == KEY_RIGHT) {
            m_menuBar.moveRight();
        } else if (ch == KEY_UP) {
            m_menuBar.moveUp();
        } else if (ch == KEY_DOWN) {
            m_menuBar.moveDown();
        } else if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
            TuiAction act = m_menuBar.triggerCurrent();
            executeAction(act);
        } else if (ch == 27) { // Escape
            m_menuBar.setActive(false);
        }
        return;
    }

    // 2. Global Hotkeys (Work anywhere)
    if (ch == KEY_F(10)) {
        m_menuBar.setActive(true);
        return;
    }
    if (ch == KEY_F(9)) {
        m_showSidebar = !m_showSidebar;
        if (!m_showSidebar) m_focus = TuiFocus::Editor;
        return;
    }
    if (ch == 9) { // Tab key toggles focus between Sidebar and Editor
        if (m_showSidebar) {
            m_focus = (m_focus == TuiFocus::Editor) ? TuiFocus::Sidebar : TuiFocus::Editor;
            return;
        }
    }
    if (ch == 17) { // Ctrl+Q: Exit
        executeAction(TuiAction::FileExit);
        return;
    }
    if (ch == 19) { // Ctrl+S: Save
        onSave();
        return;
    }
    if (ch == 14) { // Ctrl+N: New Document
        newBuffer();
        return;
    }
    if (ch == 15) { // Ctrl+O: Open File
        executeAction(TuiAction::FileOpen);
        return;
    }
    if (ch == 23) { // Ctrl+W: Close Tab
        closeCurrentTab();
        return;
    }
    if (ch == KEY_F(7) || ch == 2) { // F7 or Ctrl+B: Prev Tab
        if (!m_buffers.empty()) {
            m_activeBufferIndex = (m_activeBufferIndex > 0) ? m_activeBufferIndex - 1 : (int)m_buffers.size() - 1;
            m_highlighter.setDefinition(currentBuffer()->definition());
        }
        return;
    }
    if (ch == KEY_F(8) || ch == 20) { // F8 or Ctrl+T: Next Tab
        if (!m_buffers.empty()) {
            m_activeBufferIndex = (m_activeBufferIndex + 1) % m_buffers.size();
            m_highlighter.setDefinition(currentBuffer()->definition());
        }
        return;
    }

    // 3. Sidebar Focus Navigation
    if (m_focus == TuiFocus::Sidebar) {
        if (ch == KEY_UP) {
            m_fileTree.moveUp();
        } else if (ch == KEY_DOWN) {
            m_fileTree.moveDown();
        } else if (ch == 10 || ch == 13 || ch == KEY_ENTER || ch == KEY_RIGHT) {
            QString opened = m_fileTree.activateCurrent();
            if (!opened.isEmpty()) {
                openFile(opened);
                m_focus = TuiFocus::Editor;
            }
        } else if (ch == 27) { // Esc
            m_focus = TuiFocus::Editor;
        }
        return;
    }

    // 4. Editor Focus Navigation & Shortcuts
    TuiBuffer *buf = currentBuffer();
    if (!buf) return;

    if (ch == KEY_UP) {
        buf->moveUp();
    } else if (ch == KEY_DOWN) {
        buf->moveDown();
    } else if (ch == KEY_LEFT) {
        buf->moveLeft();
    } else if (ch == KEY_RIGHT) {
        buf->moveRight();
    } else if (ch == KEY_HOME) {
        buf->moveHome();
    } else if (ch == KEY_END) {
        buf->moveEnd();
    } else if (ch == KEY_PPAGE) {
        buf->movePageUp(m_termRows - 4);
    } else if (ch == KEY_NPAGE) {
        buf->movePageDown(m_termRows - 4);
    }
    // Deletion
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        buf->backspace();
    } else if (ch == KEY_DC) {
        buf->deleteChar();
    }
    // Enter
    else if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
        buf->insertNewline();
    }
    // Editor shortcuts
    else if (ch == 6) { // Ctrl+F
        onFind();
    } else if (ch == 7) { // Ctrl+G
        onGoToLine();
    } else if (ch == 4) { // Ctrl+D
        buf->duplicateLine();
    } else if (ch == 31) { // Ctrl+/
        buf->toggleComment();
    }
    // Printable characters
    else if (ch >= 32 && ch <= 126) {
        buf->insertChar(QChar(ch));
    }
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
    TuiBuffer *buf = currentBuffer();
    if (!buf) return;

    if (buf->filePath().isEmpty()) {
        onSaveAs();
    } else {
        if (buf->saveToFile()) {
            m_statusMessage = QStringLiteral("Saved: %1").arg(buf->filePath());
        } else {
            m_statusMessage = QStringLiteral("Error saving file");
        }
    }
}

void TuiApp::onSaveAs() {
    TuiBuffer *buf = currentBuffer();
    if (!buf) return;

    QString path = prompt(QStringLiteral("Save file as: "));
    if (!path.isEmpty()) {
        if (buf->saveToFile(path)) {
            m_highlighter.setDefinition(buf->definition());
            m_statusMessage = QStringLiteral("Saved: %1").arg(path);
        } else {
            m_statusMessage = QStringLiteral("Error saving file: %1").arg(path);
        }
    }
}

void TuiApp::onFind() {
    QString query = prompt(QStringLiteral("Find: "), m_searchQuery);
    if (!query.isEmpty()) {
        m_searchQuery = query;
        if (currentBuffer()) {
            if (!currentBuffer()->find(query, true)) {
                m_statusMessage = QStringLiteral("Not found: %1").arg(query);
            } else {
                m_statusMessage = QStringLiteral("Found match for: %1").arg(query);
            }
        }
    } else {
        m_searchQuery.clear();
    }
}

void TuiApp::onGoToLine() {
    QString val = prompt(QStringLiteral("Go to line: "));
    bool ok = false;
    int line = val.toInt(&ok);
    if (ok && line >= 1 && currentBuffer()) {
        currentBuffer()->setCursor(line - 1, 0);
    }
}

void TuiApp::onAbout() {
    prompt(QStringLiteral("UberPad [TUI] - Notepad++ in C++/Qt6 with 460+ syntax highlighters. Press Enter."));
}

} // namespace UberPad
