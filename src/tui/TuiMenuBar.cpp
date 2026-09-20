#include "TuiMenuBar.h"
#include <ncurses.h>
#include <cctype>
#include <algorithm>

namespace UberPad {

TuiMenuBar::TuiMenuBar() {
    // 1. File Menu (Alt+F)
    TuiMenu fileMenu;
    fileMenu.title = QStringLiteral("File");
    fileMenu.mnemonic = 'F';
    fileMenu.items = {
        { QStringLiteral("New"), QStringLiteral("Ctrl+N"), TuiAction::FileNew, 'N' },
        { QStringLiteral("Open..."), QStringLiteral("Ctrl+O"), TuiAction::FileOpen, 'O' },
        { QStringLiteral("Save"), QStringLiteral("Ctrl+S"), TuiAction::FileSave, 'S' },
        { QStringLiteral("Save As..."), QStringLiteral(""), TuiAction::FileSaveAs, 'A' },
        { QStringLiteral("Close Tab"), QStringLiteral("Ctrl+W"), TuiAction::FileCloseTab, 'C' },
        { QStringLiteral("Exit"), QStringLiteral("Ctrl+Q"), TuiAction::FileExit, 'X' }
    };
    m_menus.push_back(fileMenu);

    // 2. Edit Menu (Alt+E)
    TuiMenu editMenu;
    editMenu.title = QStringLiteral("Edit");
    editMenu.mnemonic = 'E';
    editMenu.items = {
        { QStringLiteral("Duplicate Line"), QStringLiteral("Ctrl+D"), TuiAction::EditDuplicateLine, 'D' },
        { QStringLiteral("Toggle Comment"), QStringLiteral("Ctrl+/"), TuiAction::EditToggleComment, 'C' },
        { QStringLiteral("Go to Line..."), QStringLiteral("Ctrl+G"), TuiAction::EditGoToLine, 'G' }
    };
    m_menus.push_back(editMenu);

    // 3. Search Menu (Alt+S)
    TuiMenu searchMenu;
    searchMenu.title = QStringLiteral("Search");
    searchMenu.mnemonic = 'S';
    searchMenu.items = {
        { QStringLiteral("Find..."), QStringLiteral("Ctrl+F"), TuiAction::SearchFind, 'F' }
    };
    m_menus.push_back(searchMenu);

    // 4. View Menu (Alt+V)
    TuiMenu viewMenu;
    viewMenu.title = QStringLiteral("View");
    viewMenu.mnemonic = 'V';
    viewMenu.items = {
        { QStringLiteral("Toggle Sidebar"), QStringLiteral("F9"), TuiAction::ViewToggleSidebar, 'S' },
        { QStringLiteral("Toggle Line Numbers"), QStringLiteral(""), TuiAction::ViewToggleLineNumbers, 'L' }
    };
    m_menus.push_back(viewMenu);

    // 5. Language Menu (Alt+L)
    TuiMenu langMenu;
    langMenu.title = QStringLiteral("Language");
    langMenu.mnemonic = 'L';
    langMenu.items = {
        { QStringLiteral("Auto-Detect"), QStringLiteral(""), TuiAction::LangAuto, 'A' },
        { QStringLiteral("C++"), QStringLiteral(""), TuiAction::LangCpp, 'C' },
        { QStringLiteral("Python"), QStringLiteral(""), TuiAction::LangPython, 'P' },
        { QStringLiteral("Rust"), QStringLiteral(""), TuiAction::LangRust, 'R' },
        { QStringLiteral("Go"), QStringLiteral(""), TuiAction::LangGo, 'G' },
        { QStringLiteral("JavaScript"), QStringLiteral(""), TuiAction::LangJS, 'J' },
        { QStringLiteral("HTML"), QStringLiteral(""), TuiAction::LangHTML, 'H' },
        { QStringLiteral("Markdown"), QStringLiteral(""), TuiAction::LangMarkdown, 'M' },
        { QStringLiteral("Plain Text"), QStringLiteral(""), TuiAction::LangPlainText, 'T' }
    };
    m_menus.push_back(langMenu);

    // 6. Help Menu (Alt+H)
    TuiMenu helpMenu;
    helpMenu.title = QStringLiteral("Help");
    helpMenu.mnemonic = 'H';
    helpMenu.items = {
        { QStringLiteral("About UberPad"), QStringLiteral(""), TuiAction::HelpAbout, 'A' }
    };
    m_menus.push_back(helpMenu);
}

void TuiMenuBar::setActive(bool active) {
    m_active = active;
    if (m_active) {
        m_currentItem = 0;
    }
}

bool TuiMenuBar::openMenuByMnemonic(char key) {
    char upperKey = (char)std::toupper((unsigned char)key);
    for (size_t i = 0; i < m_menus.size(); ++i) {
        if (std::toupper((unsigned char)m_menus[i].mnemonic) == upperKey) {
            m_active = true;
            m_currentMenu = (int)i;
            m_currentItem = 0;
            return true;
        }
    }
    return false;
}

TuiAction TuiMenuBar::triggerByMnemonic(char key) {
    if (!m_active || m_currentMenu < 0 || m_currentMenu >= (int)m_menus.size()) {
        return TuiAction::None;
    }
    char upperKey = (char)std::toupper((unsigned char)key);
    const auto &items = m_menus[m_currentMenu].items;
    for (size_t i = 0; i < items.size(); ++i) {
        if (std::toupper((unsigned char)items[i].mnemonic) == upperKey) {
            m_active = false;
            return items[i].action;
        }
    }
    return TuiAction::None;
}

void TuiMenuBar::moveLeft() {
    if (m_currentMenu > 0) {
        m_currentMenu--;
    } else {
        m_currentMenu = (int)m_menus.size() - 1;
    }
    m_currentItem = 0;
}

void TuiMenuBar::moveRight() {
    if (m_currentMenu + 1 < (int)m_menus.size()) {
        m_currentMenu++;
    } else {
        m_currentMenu = 0;
    }
    m_currentItem = 0;
}

void TuiMenuBar::moveUp() {
    if (m_currentItem > 0) {
        m_currentItem--;
    } else if (!m_menus[m_currentMenu].items.empty()) {
        m_currentItem = (int)m_menus[m_currentMenu].items.size() - 1;
    }
}

void TuiMenuBar::moveDown() {
    if (m_currentItem + 1 < (int)m_menus[m_currentMenu].items.size()) {
        m_currentItem++;
    } else {
        m_currentItem = 0;
    }
}

TuiAction TuiMenuBar::triggerCurrent() {
    if (m_currentMenu >= 0 && m_currentMenu < (int)m_menus.size()) {
        const auto &items = m_menus[m_currentMenu].items;
        if (m_currentItem >= 0 && m_currentItem < (int)items.size()) {
            TuiAction act = items[m_currentItem].action;
            m_active = false;
            return act;
        }
    }
    m_active = false;
    return TuiAction::None;
}

void TuiMenuBar::render(int width) {
    // Draw top row background
    attron(COLOR_PAIR(12));
    mvhline(0, 0, ' ', width);

    int col = 1;
    std::vector<int> menuPositions;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        menuPositions.push_back(col);
        const QString &title = m_menus[i].title;
        char mnem = m_menus[i].mnemonic;
        bool isSelected = (m_active && (int)i == m_currentMenu);

        int baseAttr = isSelected ? (COLOR_PAIR(11) | A_BOLD) : COLOR_PAIR(12);
        int hotkeyAttr = isSelected ? (COLOR_PAIR(10) | A_BOLD | A_UNDERLINE) : (COLOR_PAIR(14) | A_BOLD | A_UNDERLINE);

        attron(baseAttr);
        mvaddch(0, col, ' ');
        col++;

        // Draw title with highlighted mnemonic character (DOS EDIT style)
        bool mnemDrawn = false;
        for (int c = 0; c < title.size(); ++c) {
            char ch = title[c].toLatin1();
            if (!mnemDrawn && std::toupper((unsigned char)ch) == std::toupper((unsigned char)mnem)) {
                attroff(baseAttr);
                attron(hotkeyAttr);
                mvaddch(0, col, ch);
                attroff(hotkeyAttr);
                attron(baseAttr);
                mnemDrawn = true;
            } else {
                mvaddch(0, col, ch);
            }
            col++;
        }

        mvaddch(0, col, ' ');
        col += 2;
        attroff(baseAttr);
    }

    // Right side hints for classic DOS / Windows navigation
    QString hint = QStringLiteral("[Alt+F: File  Alt+E: Edit  Alt+S: Search  F9: Tree  Tab: Switch] ");
    if (width > col + hint.size()) {
        attron(COLOR_PAIR(12));
        mvaddstr(0, width - hint.size(), hint.toUtf8().constData());
        attroff(COLOR_PAIR(12));
    }

    // If active: draw dropdown menu box
    if (m_active && m_currentMenu >= 0 && m_currentMenu < (int)m_menus.size()) {
        const TuiMenu &menu = m_menus[m_currentMenu];
        int menuCol = menuPositions[m_currentMenu];

        int menuWidth = 26;
        for (const auto &item : menu.items) {
            int len = item.title.size() + item.shortcut.size() + 4;
            menuWidth = std::max(menuWidth, len);
        }
        if (menuCol + menuWidth > width) {
            menuCol = std::max(0, width - menuWidth - 1);
        }

        int menuHeight = (int)menu.items.size() + 2;

        // Draw box background
        attron(COLOR_PAIR(11) | A_BOLD);
        for (int r = 0; r < menuHeight; ++r) {
            mvhline(1 + r, menuCol, ' ', menuWidth);
        }

        // Draw borders
        mvaddch(1, menuCol, ACS_ULCORNER);
        mvhline(1, menuCol + 1, ACS_HLINE, menuWidth - 2);
        mvaddch(1, menuCol + menuWidth - 1, ACS_URCORNER);

        for (int r = 0; r < (int)menu.items.size(); ++r) {
            mvaddch(2 + r, menuCol, ACS_VLINE);
            mvaddch(2 + r, menuCol + menuWidth - 1, ACS_VLINE);
        }

        mvaddch(menuHeight, menuCol, ACS_LLCORNER);
        mvhline(menuHeight, menuCol + 1, ACS_HLINE, menuWidth - 2);
        mvaddch(menuHeight, menuCol + menuWidth - 1, ACS_LRCORNER);
        attroff(COLOR_PAIR(11) | A_BOLD);

        // Draw items with mnemonic accelerators highlighted
        for (size_t i = 0; i < menu.items.size(); ++i) {
            const auto &item = menu.items[i];
            bool selected = ((int)i == m_currentItem);

            int itemAttr = selected ? (COLOR_PAIR(10) | A_BOLD) : COLOR_PAIR(11);
            int hotkeyAttr = selected ? (COLOR_PAIR(10) | A_BOLD | A_UNDERLINE) : (COLOR_PAIR(13) | A_BOLD | A_UNDERLINE);

            attron(itemAttr);
            mvhline(2 + i, menuCol + 1, ' ', menuWidth - 2);

            int textCol = menuCol + 2;
            char itemMnem = item.mnemonic;
            bool itemMnemDrawn = false;

            for (int c = 0; c < item.title.size(); ++c) {
                char ch = item.title[c].toLatin1();
                if (!itemMnemDrawn && itemMnem != '\0' &&
                    std::toupper((unsigned char)ch) == std::toupper((unsigned char)itemMnem)) {
                    attroff(itemAttr);
                    attron(hotkeyAttr);
                    mvaddch(2 + i, textCol, ch);
                    attroff(hotkeyAttr);
                    attron(itemAttr);
                    itemMnemDrawn = true;
                } else {
                    mvaddch(2 + i, textCol, ch);
                }
                textCol++;
            }

            if (!item.shortcut.isEmpty()) {
                int scCol = menuCol + menuWidth - 2 - item.shortcut.size();
                mvaddstr(2 + i, scCol, item.shortcut.toUtf8().constData());
            }

            attroff(itemAttr);
        }
    }
}

} // namespace UberPad
