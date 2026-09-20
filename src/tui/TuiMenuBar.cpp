#include "TuiMenuBar.h"
#include <ncurses.h>
#include <algorithm>

namespace UberPad {

TuiMenuBar::TuiMenuBar() {
    // 1. File Menu
    TuiMenu fileMenu;
    fileMenu.title = QStringLiteral("File");
    fileMenu.items = {
        { QStringLiteral("New"), QStringLiteral("Ctrl+N"), TuiAction::FileNew },
        { QStringLiteral("Open..."), QStringLiteral("Ctrl+O"), TuiAction::FileOpen },
        { QStringLiteral("Save"), QStringLiteral("Ctrl+S"), TuiAction::FileSave },
        { QStringLiteral("Save As..."), QStringLiteral(""), TuiAction::FileSaveAs },
        { QStringLiteral("Close Tab"), QStringLiteral("Ctrl+W"), TuiAction::FileCloseTab },
        { QStringLiteral("Exit"), QStringLiteral("Ctrl+Q"), TuiAction::FileExit }
    };
    m_menus.push_back(fileMenu);

    // 2. Edit Menu
    TuiMenu editMenu;
    editMenu.title = QStringLiteral("Edit");
    editMenu.items = {
        { QStringLiteral("Duplicate Line"), QStringLiteral("Ctrl+D"), TuiAction::EditDuplicateLine },
        { QStringLiteral("Toggle Comment"), QStringLiteral("Ctrl+/"), TuiAction::EditToggleComment },
        { QStringLiteral("Go to Line..."), QStringLiteral("Ctrl+G"), TuiAction::EditGoToLine }
    };
    m_menus.push_back(editMenu);

    // 3. Search Menu
    TuiMenu searchMenu;
    searchMenu.title = QStringLiteral("Search");
    searchMenu.items = {
        { QStringLiteral("Find..."), QStringLiteral("Ctrl+F"), TuiAction::SearchFind }
    };
    m_menus.push_back(searchMenu);

    // 4. View Menu
    TuiMenu viewMenu;
    viewMenu.title = QStringLiteral("View");
    viewMenu.items = {
        { QStringLiteral("Toggle Sidebar"), QStringLiteral("F9"), TuiAction::ViewToggleSidebar },
        { QStringLiteral("Toggle Line Numbers"), QStringLiteral(""), TuiAction::ViewToggleLineNumbers }
    };
    m_menus.push_back(viewMenu);

    // 5. Language Menu
    TuiMenu langMenu;
    langMenu.title = QStringLiteral("Language");
    langMenu.items = {
        { QStringLiteral("Auto-Detect"), QStringLiteral(""), TuiAction::LangAuto },
        { QStringLiteral("C++"), QStringLiteral(""), TuiAction::LangCpp },
        { QStringLiteral("Python"), QStringLiteral(""), TuiAction::LangPython },
        { QStringLiteral("Rust"), QStringLiteral(""), TuiAction::LangRust },
        { QStringLiteral("Go"), QStringLiteral(""), TuiAction::LangGo },
        { QStringLiteral("JavaScript"), QStringLiteral(""), TuiAction::LangJS },
        { QStringLiteral("HTML"), QStringLiteral(""), TuiAction::LangHTML },
        { QStringLiteral("Markdown"), QStringLiteral(""), TuiAction::LangMarkdown },
        { QStringLiteral("Plain Text"), QStringLiteral(""), TuiAction::LangPlainText }
    };
    m_menus.push_back(langMenu);

    // 6. Help Menu
    TuiMenu helpMenu;
    helpMenu.title = QStringLiteral("Help");
    helpMenu.items = {
        { QStringLiteral("About UberPad"), QStringLiteral(""), TuiAction::HelpAbout }
    };
    m_menus.push_back(helpMenu);
}

void TuiMenuBar::setActive(bool active) {
    m_active = active;
    if (m_active) {
        m_currentItem = 0;
    }
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
    // Draw top row bar
    attron(COLOR_PAIR(12));
    mvhline(0, 0, ' ', width);

    int col = 1;
    std::vector<int> menuPositions;

    for (size_t i = 0; i < m_menus.size(); ++i) {
        menuPositions.push_back(col);
        QString title = QStringLiteral(" %1 ").arg(m_menus[i].title);
        bool isSelected = (m_active && (int)i == m_currentMenu);

        if (isSelected) {
            attron(COLOR_PAIR(11) | A_BOLD);
        } else {
            attron(COLOR_PAIR(12));
        }

        mvaddstr(0, col, title.toUtf8().constData());

        if (isSelected) {
            attroff(COLOR_PAIR(11) | A_BOLD);
        }

        col += title.size() + 1;
    }

    // Right side hint
    QString hint = QStringLiteral("[F10: Menu  F9: Sidebar  Tab: Switch] ");
    if (width > col + hint.size()) {
        mvaddstr(0, width - hint.size(), hint.toUtf8().constData());
    }
    attroff(COLOR_PAIR(12));

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

        // Draw box border
        attron(COLOR_PAIR(11) | A_BOLD);
        for (int r = 0; r < menuHeight; ++r) {
            mvhline(1 + r, menuCol, ' ', menuWidth);
        }

        // Draw top & bottom borders
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

        // Draw items
        for (size_t i = 0; i < menu.items.size(); ++i) {
            const auto &item = menu.items[i];
            bool selected = ((int)i == m_currentItem);

            int itemAttr = selected ? (COLOR_PAIR(10) | A_BOLD) : COLOR_PAIR(11);
            attron(itemAttr);

            mvhline(2 + i, menuCol + 1, ' ', menuWidth - 2);
            mvaddstr(2 + i, menuCol + 2, item.title.toUtf8().constData());

            if (!item.shortcut.isEmpty()) {
                int scCol = menuCol + menuWidth - 2 - item.shortcut.size();
                mvaddstr(2 + i, scCol, item.shortcut.toUtf8().constData());
            }

            attroff(itemAttr);
        }
    }
}

} // namespace UberPad
