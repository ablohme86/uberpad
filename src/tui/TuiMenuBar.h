#pragma once

#include <QString>
#include <vector>

namespace UberPad {

enum class TuiAction {
    None,
    FileNew,
    FileOpen,
    FileSave,
    FileSaveAs,
    FileCloseTab,
    FileExit,
    EditDuplicateLine,
    EditToggleComment,
    EditGoToLine,
    SearchFind,
    ViewToggleSidebar,
    ViewToggleLineNumbers,
    LangAuto,
    LangCpp,
    LangPython,
    LangRust,
    LangGo,
    LangJS,
    LangHTML,
    LangMarkdown,
    LangPlainText,
    HelpAbout
};

struct TuiMenuItem {
    QString title;
    QString shortcut;
    TuiAction action = TuiAction::None;
};

struct TuiMenu {
    QString title;
    std::vector<TuiMenuItem> items;
};

class TuiMenuBar {
public:
    TuiMenuBar();

    bool isActive() const { return m_active; }
    void setActive(bool active);

    void moveLeft();
    void moveRight();
    void moveUp();
    void moveDown();

    TuiAction triggerCurrent();

    void render(int width);

private:
    std::vector<TuiMenu> m_menus;
    bool m_active = false;
    int m_currentMenu = 0;
    int m_currentItem = 0;
};

} // namespace UberPad
